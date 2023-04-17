#pragma once
#include "Util.hpp"
#include MY_MCU_HEADER

//........................................................................................

////////////////
class
GpioPin         
////////////////
                {

                //the Gpio hardware register layout
                //could also get from manufacturer header
                struct Reg { u32 MODER, OTYPER, OSPEEDR, PUPDR, IDR,
                             ODR, BSRR, LCKR, AFR[2], BRR; };

                const u32               pin_;       //pin 0-15
                const u32               pinbm_;     //bitmask for bsrr/brr
                const bool              inv_;       //invert?
                volatile Reg&           reg_;       //register struct access

                //rcc io enable register, same for all instances so static
                static inline volatile u32& rccEn_{ *(reinterpret_cast<u32*>(MCU::RCC_IOPENR)) };

                //write bitmask value
                //u32 register reference, bitfield size, new bitfield value, pin
                //(pin needed for altFunc use since it will use 0-7 for pin, cannot use pin_)
                //this single function can handle all bit manipulation where pinbm_ is not used
                //interrupts also disabled to prevent any corruption if another pin is being setup in another
                //'run level' (will prevent possible register corruption, not very likely but is possible)
                auto
writeBmv        (volatile u32& reg, u32 siz, u32 val, u32 pin)
                {
                auto bp = siz*pin; //we can figure out bit position
                auto bm = (1<<siz)-1; //1=1,2=3,4=15
                auto clrbm = bm<<bp;
                auto setbm = val<<bp; //no need to mask val, already limited
                InterruptLock lock;
                reg = (reg bitand compl clrbm ) bitor setbm;
                return *this;
                }

                auto
portEnable      (u32 port)
                {
                InterruptLock lock; //same reason as above to protect the bit manipulation
                rccEn_ or_eq (1<<port); //enable port in RCC, porta=bit0, partb=bit1, etc.
                }

public:

                enum
INVERT          { HIGHISON, LOWISON };

GpioPin         (MCU::PIN pin, INVERT inv = HIGHISON)
                : pin_( pin bitand 15 ),    //pin 0-15
                  pinbm_( 1<<pin_ ),        //pin bitmask
                  inv_( inv ),              //invert?
                  reg_( *(Reg*)(MCU::GPIOA + MCU::GPIO_SPACING*(pin/16)) )
                {
                portEnable( pin/16 );       //enable port in RCC
                }

                //properities, all will go through writeBmv and are interrupt protected
                enum
MODE            { INPUT, OUTPUT, ALTERNATE, ANALOG };
                auto
mode            (MODE e) { return writeBmv(reg_.MODER, 2, e, pin_); }

                enum
OTYPE           { PUSHPULL, ODRAIN };
                auto
outType         (OTYPE e) { return writeBmv(reg_.OTYPER, 1, e == ODRAIN, pin_); }

                enum
PULL            { NOPULL, PULLUP, PULLDOWN };
                auto
pull            (PULL e) { return writeBmv(reg_.PUPDR, 2, e, pin_); }

                enum
SPEED           { SPEED0, SPEED1, SPEED2, SPEED3 }; //VLOW to VHIGH
                auto
speed           (SPEED e) { return writeBmv(reg_.OSPEEDR, 2, e, pin_); }

                auto //also sets mode to alternate
altFunc         (MCU::ALTFUNC e)
                { //choose which afr[] to use, pin_ 0-7=[0], 8-15=[1], then mask the pin_ to 0-7
                return writeBmv(reg_.AFR[pin_<8?0:1], 4, e, pin_ bitand 7).mode( ALTERNATE );
                }

                //no interrupt protection needed for functions below, writes are using bssr/brr

                //write
                auto
high            () { reg_.BSRR = pinbm_; return *this; }
                auto
low             () { reg_.BRR = pinbm_; return *this; }
                auto
on              () { return inv_ ? low() : high(); }
                auto
off             () { return inv_ ? high() : low(); }
                auto
on              (bool tf) { return tf ? on() : off(); }
                auto
toggle          () { return isLatHigh() ? low() : high(); }

                // read pin
                auto
isHigh          () { return reg_.IDR bitand pinbm_; }
                auto
isLow           () { return not isHigh(); }
                auto
isOn            () { return inv_ ? isLow() : isHigh(); }
                auto
isOff           () { return not isOn(); }

                // read lat
                bool
isLatHigh       () { return reg_.ODR bitand pinbm_; }
                auto
isLatLow        () { return not isLatHigh(); }
                auto
isLatOn         () { return inv_ ? isLatLow() : isLatHigh(); }
                auto
isLatOff        () { return not isLatOn(); }

                }; //GpioPin

//........................................................................................
