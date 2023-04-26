#pragma once
#include "Util.hpp"
#include "Print.hpp"
#include "GpioPin.hpp"
#include "System.hpp"
#include <array>
#include "Buffer.hpp"
#include MY_MCU_HEADER


//........................................................................................

////////////////
class
Uart            
////////////////
                : public FMT::Print
                {

                struct Reg { u32 CR1, CR2, CR3, BRR, GTPR, RTOR, RQR, ISR, ICR, RDR, 
                             TDR, PRESC; };
                
                volatile Reg&       reg_;
                Buffer              buffer_;
                Nvic::IRQ_PRIORITY  irqPriorty_;
                MCU::IRQn           irqn_;

                //static, for isr use, stores 'this' object for each uart class in use
                //isr will match irqn_ to active irq number for inst to use
                using irqinst_t = struct { MCU::IRQn irqn; Uart* inst; };
                static inline irqinst_t instances_[MCU::USARTn_COUNT]; 

                enum { TEbm = 1<<3, UEbm = 1, TXEbm = 1<<7, TCbm = 1<<6 };

                auto
txeIrqOn        () { reg_.CR1 = reg_.CR1 bitor TXEbm; }
                auto
txeIrqOff       () { reg_.CR1 = reg_.CR1 bitand compl TXEbm; }
                auto
txOn            () { reg_.CR1 = TEbm bitor UEbm; }
                auto
isTxFull        (){ return (reg_.ISR bitand TXEbm) == 0; }
                auto 
isTxComplete    (){ return reg_.ISR bitand TCbm; }

                static auto
bufferTx        (Uart& u)
                {
                u8 v = 0;
                if( u.buffer_.read(v) ) u.reg_.TDR = v;
                else u.txeIrqOff();
                }

                bool
writeBuffer     (const char c)
                {
                //if buffer full and this irq level <= uart irq level
                //we have to take care of the isr duties ourselves since the
                //buffer will not be serviced by the isr
                while( buffer_.isFull() ){
                    if( Nvic::activePriority() > irqPriorty_ ) continue; //let isr handle it
                    //isr cannot help
                    while( isTxFull() ){} //first wait for hardware
                    bufferTx(*this);
                    Nvic::clearPending( irqn_ ); //clear irq pending
                    break;
                    } //now buffer has room for at least 1 byte...

                //protect write+irq enable combo (so uart isr cannot disable txeIrq inside
                //this sequence)
                InterruptLock lock; 
                buffer_.write( c );
                txeIrqOn();
                return true;
                }

                // buffer -> uart hardware
                //static so we can put address into vector table, but then need to get 
                //a Uart object stored in instances_
                static void 
isr             ()
                {
                //not checking ISR.TXE bit (should be 1) as we only use TXEIE so only one way to get here
                //flag is cleared when TDR is written
                auto u = readInst(); //get a Uart pointer for this irq
                if( u ) bufferTx( *u );
                }

                auto
baud            (u32 baudVal)
                { //default 16 sample rate
                reg_.BRR = (System::cpuHz()/baudVal) bitand 0xFFFF; //BRR is 16bits
                }

                //called by constructor, irqN/this pairs stored in instances_
                //(no remove function, so is a insert only process)
                auto
insertInst      ()
                {
                for( auto& ri : instances_ ){
                    if( ri.irqn == irqn_ ) return false; //already used, something is wrong
                    if( ri.irqn ) continue; //in use by another uart instance
                    ri.irqn = irqn_;
                    ri.inst = this;
                    return true;
                    }
                return false; //overbooked, something wrong 
                }

                //called by static isr() function, get Uart object store in instances_
                //match current irq number to get Uart pointer in instances_
                static Uart*
readInst        ()
                {
                Uart* u = nullptr;
                auto irqn = Scb::activeIrq(); //current irq number
                for( auto& ri : instances_ ){
                    if( ri.irqn == irqn ) return ri.inst; 
                    }
                return u; //somehting wrong (isr function will do nothing)
                }

public:

                auto
isIdle          (){ return buffer_.isEmpty() and isTxComplete(); }

                virtual bool
write           (const char c){ return writeBuffer(c); }

                //binary array of data
                template<unsigned N> bool
write           (std::array<u8,N>& arr)
                { 
                for( auto c : arr ) writeBuffer(c); 
                return true;
                }

                template<unsigned N>
Uart            (MCU::uart_t u, u32 baudVal, std::array<u8,N>& buffer, Nvic::IRQ_PRIORITY irqPriority)
                : reg_( *(reinterpret_cast<Reg*>(u.addr)) ),
                  buffer_( buffer ),
                  irqPriorty_( irqPriority ),
                  irqn_( u.irqn )
                {
                //setup instances_ for isr use- if already setup for this uart something 
                //is wrong so just skip all other init code
                if( not insertInst() ) return;  
                { InterruptLock lock; u.init(); } //rcc
                GpioPin(u.txPin)
                    .mode(GpioPin::INPUT)   //first set default state if/when tx not enabled
                    .pull(GpioPin::PULLUP)  //input, pullup (tx idle state)
                    .altFunc( u.txAltFunc); //now set pin to uart tx fnction
                baud( baudVal );
                Nvic::setFunction( irqn_, isr, irqPriorty_ );
                txOn();
                }

                };

//........................................................................................
