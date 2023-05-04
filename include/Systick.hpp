#pragma once

#include "Util.hpp"
#include "System.hpp"
#include <chrono>


//........................................................................................

////////////////
class
Systick        
////////////////
                {

                //default Systick irq priority if not specified, lowest priority
                static constexpr auto DEFAULT_PRIORITY{ Nvic::PRIORITY3 };

                //stm32g0 has the option to use HCLK/8 as the clock source,
                //but HCLK is in use here (cpu speed)

                //systick irq rate, specified by a ratio
                //std::ratio<1,500> would be 2ms irq rate
                //milliseconds is a chrono defined duration, period=<1,1000>
                //duration_irq::period::num,den will be used (num normally 1)
                using duration_irq = std::chrono::milliseconds; 

                //chrono resolution, will convert cpu cycles to this duration
                //for the chrono clock 'now' function (unrelated to above irq duration)
                using duration_chrono = std::chrono::microseconds;

                //SysTick registers
                struct Reg { u32 CSR, RVR, CVR, CALIB; };

                //registers reference
                static inline volatile Reg& reg_{ *(reinterpret_cast<Reg*>(0xE000'E010)) };

                //enums, constants
                enum { CVR_MASK = 0xFFFFFF };

                //private vars
                static inline CPU::AtomRW<volatile i64> atom_cpuCyclesTotal_;
                static inline volatile bool             wasIrq_; //can see if was cause of wakeup
                static inline u32                       cyclesPerIrq_;
                static inline u32                       cpuHz_;
                static inline u32                       shift1us_; //bit shift for cycles to 1us
                

                static auto
isr             ()
                {
                atom_cpuCyclesTotal_ += cyclesPerIrq_;
                wasIrq_ = true;
                }

                //could be called with irq's disabled, so cannot assume atom_cpuCyclesTotal_ 
                //will be updated when cvr rolls over
                static i64
cpuCycles       ()
                {
                InterruptLock lock;             //simpler to just protect the whole function
                auto counter = reg_.CVR;        //hardware. always changing (unless HCLK/8 used)
                //since irq's are aready off, skip the protection for cpuCyclesTotal_
                //which is AtomRW type
                auto total = atom_cpuCyclesTotal_.value;  //irq's are off, will not change
                if( Scb::isSystickPending() ){  //if pending flag is set
                    counter = reg_.CVR;         //read cvr again (whether necessary or not)
                    total += cyclesPerIrq_;     //account for missed irq
                    //leave systick pending so isr will still run (we only updated our local
                    //copy of cpuCyclesTotal_, and we also want to allow the isr to call the
                    //callback to do tasks)
                    }
                counter = (compl counter) bitand CVR_MASK; //down count value to up count value
                return total + counter;         //total cpu cycles since systick started
                }

                static auto
onCheck         ()
                { 
                if( (reg_.CSR bitand 3) == 3 ) return;  //irq on, enabled
                restart(); 
                }

                //called by restart()
                //check if clock speed allows using a bit shift to convert cpu cycles to
                //chrono clock resolution (microseconds in current config)
                static void
cpuSpeedCheck   ()
                {
                shift1us_ = 0; //0 means cannot use shift (will use division)
                if( cpuHz_ % duration_chrono::period::den ) return; //cannot use shift
                auto v = cpuHz_ / duration_chrono::period::den;     //get MHz
                if( v bitand (v>>1) ) return;                       //not a power of 2
                while( v >>= 1, v ) shift1us_++;                    //create a bit shift value
                }

                static Systick
restart         (Nvic::IRQ_PRIORITY priority = DEFAULT_PRIORITY)
                {
                reg_.CSR = 0; //off  
                cpuHz_ =  System::cpuHz();          
                cyclesPerIrq_ = duration_irq::period::num * cpuHz_ / duration_irq::period::den;
                cpuSpeedCheck(); //check if cpu speed allows using bit shift for cycles to us conversions
                Nvic::setFunction( MCU::SYSTICK_IRQ, isr, priority );
                reg_.RVR = cyclesPerIrq_ - 1;
                reg_.CVR = 0;
                reg_.CSR = 7; //processor clock (already set), irq, enable
                return Systick();
                }

                //cpu cycles to chrono duration (duration_chrono)
                //ratio's used for this chrono clock are always <num=1,den=n>,
                //so will only need period::den
                //division is used if needed- timing a now() call takes about 50us 
                //w/16MHz cpu speed, which is about 800 cpu clocks (seesm high, but i64 is in use)
                //if can use shift, 9us w/16MHz cpu speed, which is about 144 cpu clocks
                static auto
cycles2chrono   ()
                { 
                auto cyc = cpuCycles();
                return duration_chrono( shift1us_ ? cyc >> shift1us_ : cyc * duration_chrono::period::den / cpuHz_ );
                }

                //above private functions allowed from main only
                //change as needed, or move function(s) to public instead
                //restart() and callback() functions are primarily in mind so only specific
                //function(s) are allowed to restart systick or set a callback, for example
                friend int main();

public:

                //these types will allow us to use Systick as a chrono clock
                using duration = duration_chrono; // rep=i64,period=ratio<1,1000000>
                using rep = duration::rep; //i64
                using time_point = std::chrono::time_point<Systick, duration>;
                static constexpr bool is_steady = true; //monotonic, no rollover

                //also check if systick is on for the following function, so if you forget 
                //to start systick it will be started for you

                static time_point
now             ()
                {
                onCheck();
                return time_point( cycles2chrono() ); 
                }

                //TODO-
                //if irq's are off, delay will never return

                //if delay is > 10ms, let cpu idle (up to 1ms error due to wating for irq)
                static auto
delay           (duration d)
                {
                onCheck();
                auto tp_start = now(); //time_point
                bool lowPower = d > std::chrono::milliseconds(10);
                while( (now() - tp_start) < d ){
                    if( lowPower ) CPU::waitIrq(); 
                    }
                }

                bool
wasIrq          ()
                {
                InterruptLock lock;
                bool ret = wasIrq_;
                wasIrq_ = false; 
                return ret; 
                }

                }; //Systick

//........................................................................................

                #include "Print.hpp"

                //any duration that can implicitly convert to microseconds
                // "d hh:mm:ss.ususus" d is width 4, left space padded
                // "0 00:01:07.825696"
                inline FMT::Print&   
operator<<      (FMT::Print& p, std::chrono::microseconds dur) 
                {
                using namespace std::chrono;
                using namespace FMT;
                // < c++ 20, we have to make our own days
                using days = duration<u32, std::ratio<24*60*60>>;
                //need to explicitly cast when there is a loss in precision
                auto da = duration_cast<days        >(dur); dur -= da;
                auto hr = duration_cast<hours       >(dur); dur -= hr;
                auto mi = duration_cast<minutes     >(dur); dur -= mi;
                auto se = duration_cast<seconds     >(dur); dur -= se;
                auto us = microseconds(dur);
                return p << dec //decimal
                        << setwf(4,' ') << da.count() << " "
                        << setwf(2,'0') << hr.count() << ":"
                        << setwf(2,'0') << mi.count() << ":"
                        << setwf(2,'0') << se.count() << "."
                        << setwf(6,'0') << us.count();                
                }

                inline FMT::Print&   
operator,       (FMT::Print& p, std::chrono::microseconds dur){ return p << dur; }


//........................................................................................