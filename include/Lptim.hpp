#pragma once
#include "Util.hpp"
#include "System.hpp"
#include <chrono>
#include MY_MCU_HEADER

//........................................................................................

////////////////
class
LptimClockLSI        
////////////////
                : public CPU::Isr
                {

                //default Systick irq priority if not specified, lowest priority
                static constexpr auto DEFAULT_PRIORITY{ Nvic::PRIORITY3 };

                //irq rate, specified by a ratio
                //std::ratio<1,500> would be 2ms irq rate
                //lptim is 16bits, using 32khz clock = 2 seconds per irq
                using duration_irq = std::chrono::duration<i64,std::ratio<2,1>>; 

                //chrono resolution, will convert cpu cycles to this duration
                //for the chrono clock 'now' function (unrelated to above irq duration)
                using duration_chrono = std::chrono::microseconds;

                //private useful enums
                enum { ARRbm = 1, CMPbm = 2 /* CF,MIE,M */ };

                //lptim registers
                struct Reg { u32 ISR, ICR, IER, CFGR, CR, CMP, ARR, CNT, reserved1_, CFGR2; };

                //vars
                volatile Reg&               reg_;
                Nvic::IRQ_PRIORITY          irqPriority_;
                MCU::IRQn                   irqn_;
                CPU::AtomRW<volatile i64>   atom_lsiCyclesTotal_;

                //consts
                static constexpr auto lsiHz_{ 32768 };  
                static constexpr auto cyclesPerIrq_{ duration_irq::period::num * lsiHz_ / duration_irq::period::den };                

                void 
isr             () override
                {
                auto flags = reg_.ISR;
                reg_.ISR = flags;
                if( flags bitand ARRbm ) atom_lsiCyclesTotal_ += cyclesPerIrq_;
                if( flags bitand CMPbm ){}
                }

                auto 
count           ()
                {
                u32 cnt;
                //read twice, if both same is valid CNT value
                while( cnt = reg_.CNT, cnt != reg_.CNT ){}
                return cnt;
                }

                //could be called with irq's disabled, so cannot assume atom_lsiCyclesTotal_ 
                //will be updated if arr irq occurred
                i64
lsiCycles       ()
                {
                InterruptLock lock;             //simpler to just protect the whole function
                auto counter = count();         //hardware. always changing
                //since irq's are aready off, skip the protection for atom_lsiCyclesTotal_
                //which is AtomRW type
                auto total = atom_lsiCyclesTotal_.value; //irq's are off, not changing
                if( reg_.ISR bitand ARRbm ){    //if arr flag is set
                    counter = count();          //read cnt again
                    total += cyclesPerIrq_;     //account for missed irq
                    //leave  flag set so isr will still run (we only updated our local
                    //copy of atom_lsiCyclesTotal_)
                    }
                return total + counter;         //total lsi cycles since lptim started
                }

                //lsi cycles to chrono duration (duration_chrono)
                //ratio's used for this chrono clock are always <num=1,den=n>,
                //so will only need period::den
                auto
cycles2chrono   ()
                { 
                auto cyc = lsiCycles();
                return duration_chrono( cyc * duration_chrono::period::den / lsiHz_ );
                }


public:

                //these types will allow us to use Lptim as a chrono clock
                using duration = duration_chrono; // rep=i64,period=ratio<1,1000000>
                using rep = duration::rep; //i64
                using time_point = std::chrono::time_point<LptimClockLSI, duration>;
                static constexpr bool is_steady = true; //monotonic, no rollover


LptimClockLSI   (MCU::lptim_t lptim, Nvic::IRQ_PRIORITY irqPriority = DEFAULT_PRIORITY)
                : reg_( *(reinterpret_cast<Reg*>(lptim.addr)) ),
                  irqPriority_( irqPriority ),
                  irqn_( lptim.irqn )
                {
                InterruptLock lock;
                lptim.init();
                Nvic::setFunction( irqn_, this, irqPriority_ );
                reg_.IER = ARRbm; //IER can be set only when lptim disabled
                reg_.CR = 5; //CNTSTRT, ENABLE
                reg_.ARR = 0xFFFF; //ARR can be set only when lptim enabled
                }

                time_point
now             ()
                {
                return time_point( cycles2chrono() ); 
                }

                //TODO-
                //if irq's are off, delay may never return
                //TODO
                //if delay is > N ms, let cpu idle (need wakeup via compare irq)

                auto
delay           (duration d)
                {
                auto tp_start = now(); //time_point
                while( (now() - tp_start) < d ){}
                }

                }; // LptimClockLSI