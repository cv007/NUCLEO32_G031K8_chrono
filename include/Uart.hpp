#pragma once

#include "Util.hpp"
#include "Print.hpp"
#include "GpioPin.hpp"
#include "System.hpp"
#include <array>
#include "BufferBytes.hpp"
#include MY_MCU_HEADER


//........................................................................................

////////////////
class
Uart            
////////////////
                : public FMT::Print, 
                  public CPU::Isr
                {

                //no atomic protection needed as none of these registers are shared
                //with any other Uart instance
                struct Reg { u32 CR1, CR2, CR3, BRR, GTPR, RTOR, RQR, ISR, ICR, RDR, 
                             TDR, PRESC; };
                
                volatile Reg&       reg_; //need volatile as Reg struct members are not
                BufferBytes         buffer_;
                Nvic::IRQ_PRIORITY  irqPriorty_;
                MCU::IRQn           irqn_;
                u32                 baud_;

                enum { TEbm = 1<<3, UEbm = 1, TXEbm = 1<<7, TCbm = 1<<6 };

                auto
txeIrqOn        () { reg_.CR1 = reg_.CR1 bitor TXEbm; }
                auto
txeIrqOff       () { reg_.CR1 = reg_.CR1 bitand compl TXEbm; }
                auto
txeIrqIsOff     () { return not (reg_.CR1 bitand TXEbm); }
                auto
txOn            () { reg_.CR1 = TEbm bitor UEbm; }
                auto
isTxFull        (){ return (reg_.ISR bitand TXEbm) == 0; }
                auto 
isTxComplete    (){ return reg_.ISR bitand TCbm; }
                auto
bufferTx        ()
                {
                u8 v = 0;
                if( buffer_.read(v) ) reg_.TDR = v;
                else txeIrqOff();
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
                    bufferTx();
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
                void
isr             () override 
                { bufferTx(); }

                auto
baud            ()
                { //default 16 sample rate
                //most likely any normal baud rate will fall within the brr available 16bits 
                //but if not, the user will find out (uart will not work as intended)
                //(if baud too high or low for cpu clock in use, will not get desired baud rate)
                auto v = System::cpuHz()/baud_;
                reg_.BRR = v; 
                }

public:

                auto
cpuSpeedUpdate  (){ baud(); }

                auto
isIdle          (){ return txeIrqIsOff() and isTxComplete(); }

                virtual bool
write           (const char c){ return writeBuffer(c); }

                //binary array of data
                template<unsigned N> bool
write           (std::array<u8,N>& arr)
                { 
                for( auto c : arr ) writeBuffer(c); 
                return true;
                }

                auto 
bufferUsedMax   (){ return buffer_.sizeUsedMax(); }
                auto 
bufferUsed      (){ return buffer_.sizeUsed(); }

                template<unsigned N>
Uart            (MCU::uart_t u, u32 baudVal, std::array<u8,N>& buffer, Nvic::IRQ_PRIORITY irqPriority)
                : reg_( *(reinterpret_cast<Reg*>(u.addr)) ),
                  buffer_( buffer ),
                  irqPriorty_( irqPriority ),
                  irqn_( u.irqn ),
                  baud_( baudVal )
                {
                { InterruptLock lock; u.init(); } //rcc
                GpioPin(u.txPin).alternate( u.txAltFunc );
                baud();
                Nvic::setFunction( irqn_, this, irqPriorty_ );
                txOn();
                }

                }; //Uart

//........................................................................................
