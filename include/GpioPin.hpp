#pragma once

#include "Util.hpp"
#include MY_MCU_HEADER


//........................................................................................

////////////////
class
GpioPin         
////////////////
                {

                //Atom protection needed as these registers are shared with all other pins
                //on the same port, but not all registers need Atom protection and those that 
                //do not will not see any atomic protection produced (bsrr/brr write only, 
                //idr read only, odr is read only here as we only use bsrr/brr since we only 
                //deal with individual pins)
                //any Atom type is also volatile, so all will be volatile
                struct Reg { 
                    CPU::Atom<u32> MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, BSRR, LCKR;
                    CPU::Atom<u64> AFR;
                    CPU::Atom<u32> BRR; 
                    };

                const u32   pin_;       //pin 0-15
                const u32   pinbm_;     //bitmask for bsrr/brr
                const bool  inv_;       //invert?
                Reg&        reg_;       //register struct access (volatile is in Reg struct)

 public:

                enum
INVERT          { HIGHISON, LOWISON };

GpioPin         (MCU::PIN pin, INVERT inv = HIGHISON)
                : pin_( pin bitand 15 ),    //pin 0-15
                  pinbm_( 1<<pin_ ),        //pin bitmask
                  inv_( inv ),              //invert?
                  reg_( *reinterpret_cast<Reg*>(MCU::GPIO_BASE + MCU::GPIO_SPACING*(pin/16)) )
                {
                MCU::RCCreg.IOPENR or_eq 1<<(pin/16);   //enable port in RCC
                }

                //properities, all will go through writeBmv and are interrupt protected
                enum
MODE            { INPUT, OUTPUT, ALTERNATE, ANALOG };
                auto
mode            (MODE e) { auto bp = pin_*2; reg_.MODER.setBitmask( 3<<bp, e<<bp ); return *this; }

                enum
OTYPE           { PUSHPULL, ODRAIN };
                auto
outType         (OTYPE e) { reg_.OTYPER.setBitmask( 1<<pin_, (e == ODRAIN)<<pin_ ); return *this; }

                enum
PULL            { NOPULL, PULLUP, PULLDOWN };
                auto
pull            (PULL e) { auto bp = pin_*2; reg_.PUPDR.setBitmask( 3<<bp, e<<bp ); return *this; }

                enum
SPEED           { SPEED0, SPEED1, SPEED2, SPEED3 }; //VLOW to VHIGH
                auto
speed           (SPEED e) { auto bp = pin_*2; reg_.OSPEEDR.setBitmask( 3<<bp, e<<bp ); return *this; }

                auto //also sets mode to alternate
altFunc         (MCU::ALTFUNC e)
                {
                auto bp = pin_*4;
                reg_.AFR.setBitmask( 15<<bp, e<<bp );
                return mode( ALTERNATE );
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
