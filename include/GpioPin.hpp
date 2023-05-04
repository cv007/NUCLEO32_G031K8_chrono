#pragma once

#include "Util.hpp"
#include MY_MCU_HEADER


//........................................................................................

////////////////
class
GpioPin         
////////////////
                {

                struct Reg { 
                    CPU::AtomRW<u32> atom_MODER, atom_OTYPER, atom_OSPEEDR, atom_PUPDR;
                    volatile u32 IDR;
                    volatile u32 ODR;
                    volatile u32 BSRR;
                    CPU::AtomRW<u32> atom_LCKR;
                    CPU::AtomRW<u64> atom_AFR;
                    volatile u32 BRR; 
                    };

                const u32   pin_;       //pin 0-15
                const u32   pinbm_;     //bitmask for bsrr/brr
                const bool  inv_;       //invert?
                Reg&        reg_;       //register struct access

                //rcc io enable register, same for all instances so static
                static inline auto& atom_rccEn_{ 
                    *( reinterpret_cast<CPU::AtomRW<u32>*>(MCU::RCC_IOPENR) ) 
                    };

public:

                enum
INVERT          { HIGHISON, LOWISON };

GpioPin         (MCU::PIN pin, INVERT inv = HIGHISON)
                : pin_( pin bitand 15 ),    //pin 0-15
                  pinbm_( 1<<pin_ ),        //pin bitmask
                  inv_( inv ),              //invert?
                  reg_( *reinterpret_cast<Reg*>(MCU::GPIOA + MCU::GPIO_SPACING*(pin/16)) )
                {
                atom_rccEn_ or_eq 1<<(pin/16);   //enable port in RCC
                }

                //properities, all will go through writeBmv and are interrupt protected
                enum
MODE            { INPUT, OUTPUT, ALTERNATE, ANALOG };
                auto
mode            (MODE e) { auto bp = pin_*2; reg_.atom_MODER.setBitmask( 3<<bp, e<<bp ); return *this; }

                enum
OTYPE           { PUSHPULL, ODRAIN };
                auto
outType         (OTYPE e) { reg_.atom_OTYPER.setBitmask( 1<<pin_, (e == ODRAIN)<<pin_ ); return *this; }

                enum
PULL            { NOPULL, PULLUP, PULLDOWN };
                auto
pull            (PULL e) { auto bp = pin_*2; reg_.atom_PUPDR.setBitmask( 3<<bp, e<<bp ); return *this; }

                enum
SPEED           { SPEED0, SPEED1, SPEED2, SPEED3 }; //VLOW to VHIGH
                auto
speed           (SPEED e) { auto bp = pin_*2; reg_.atom_OSPEEDR.setBitmask( 3<<bp, e<<bp ); return *this; }

                auto //also sets mode to alternate
altFunc         (MCU::ALTFUNC e)
                {
                auto bp = pin_*4;
                reg_.atom_AFR.setBitmask( 15<<bp, e<<bp );
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
