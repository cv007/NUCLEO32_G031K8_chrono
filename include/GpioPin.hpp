#pragma once

#include "Util.hpp"
#include MY_MCU_HEADER


#if 0
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

                auto //read
altFunc         ()
                {
                auto bp = pin_*4;
                return MCU::ALTFUNC( (reg_.AFR>>bp) bitand 0xF );
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
#endif

//......................................................................................

////////////////
class
GpioPin         
////////////////
                {

                struct Reg { 
                    u32 MODER, OTYPER, OSPEEDR, PUPDR, IDR, ODR, BSRR, LCKR;
                    u64 AFR;
                    u32 BRR; 
                    };   

                const u32       pin_;   //pin 0-15
                const u32       pinbm_; //bitmask for bsrr/brr
                const bool      inv_;   //invert?
                volatile Reg&   reg_;   //register struct access

                //only called by input() output() alternate() analog(), with interrupts disabled
                //gets all registers to a known state (excluding LCKR)
                //the output() function will take care of getting the ODR bit set to an 'off' state
                auto 
deinit          () 
                {
                u32 pin1_clrbm{ compl pinbm_ };
                u32 pin2_clrbm{ compl (3u<<(pin_*2)) };
                u64 pin4_clrbm{ compl (15u<<(pin_*4)) };
                reg_.MODER   and_eq pin2_clrbm; //input
                reg_.OSPEEDR and_eq pin2_clrbm; //speed0
                reg_.OTYPER  and_eq pin1_clrbm; //pushpull
                reg_.PUPDR   and_eq pin2_clrbm; //nopull
                reg_.AFR     and_eq pin4_clrbm; //gpio
                }
public:


                enum
INVERT          { HIGHISON, LOWISON };

GpioPin         (MCU::PIN pin, INVERT inv = HIGHISON)
                : pin_( pin bitand 15 ),    //pin 0-15
                  pinbm_( 1<<pin_ ),        //pin bitmask
                  inv_( inv ),              //invert?
                  reg_( *reinterpret_cast<Reg*>(MCU::GPIO_BASE + MCU::GPIO_SPACING*(pin/16)) )
                {
                //no pin init done here (just enabling this gpio port in rcc)
                //the RCC IOPENR register is shared by all Gpio ports, but this should be the
                //only code that touches this register and we only set bits here, so irq protection
                //is not needed, but... we do not know if there is/will be other code changing these
                //IOPENR bits for other reasons (like power saving), so just irq protect for now
                //(this gpio enable belongs in an Rcc class, as we are not in charge of the RCC peripheral)
                CPU::InterruptLock lock;
                MCU::RCCreg.IOPENR or_eq (1<<(pin_/16));    //gpioN rcc clock enable
                }
            
                enum //pull applies to input/output/alternate (and a single function)
PULL            { NOPULL, PULLUP, PULLDOWN };                
                enum //speed applies to output/alternate
SPEED           { SPEED0, SPEED1, SPEED2, SPEED3 };                
                enum //odrain applies to output/alternate
OTYPE           { PUSHPULL, ODRAIN };

                private:
                //output and alternate modifiers- PULL|OTYPE|SPEED
                //allows optional properties to be applied in any order
                template<typename...Ts> auto
                modifiers_(PULL e, Ts...ts){ reg_.PUPDR or_eq (e<<(pin_*2)); return modifiers_( ts... ); }
                template<typename...Ts> auto
                modifiers_(OTYPE e, Ts...ts){ reg_.OTYPER or_eq (1<<pin_); return modifiers_( ts... ); }
                template<typename...Ts> auto
                modifiers_(SPEED e, Ts...ts){ reg_.OSPEEDR or_eq (e<<(pin_*2)); return modifiers_( ts... ); }
                auto modifiers_(){} //no more arguments, break the call chain with empty function
                public:


                //all pin config goes through deinit(), which rcc enables the port and gets the pins
                //registers to a known default state 

                //provide optional modifier (PULL enum value)
                //if no arguments provided you get NOPULL
                // obj.input(); obj.input( PULLUP|PULLDOWN );          
                auto
input           (PULL e = NOPULL)
                {
                CPU::InterruptLock lock;
                deinit();
                if( e != NOPULL ) reg_.PUPDR or_eq (e<<(pin_*2));
                return *this;     
                }

                //provide optional modifiers (enum values) in any order
                //if no arguments provided you get NOPULL,PUSHPULL,SPEED0
                // obj.output( [PULLUP|PULLDOWN], [ODRAIN], [SPEED1|2|3] )
                template<typename...Ts> auto
output          (Ts...ts)
                {
                CPU::InterruptLock lock;
                deinit();
                off(); //depends on inv_, so make sure pin is high if inv_=true
                reg_.MODER or_eq (1<<(pin_*2)); //output
                modifiers_( ts... );
                return *this; //allow method chaining (so can init to on() or off() if needed)    
                }

                //provide optional modifiers (enum values) in any order
                //if no arguments provided you get NOPULL,PUSHPULL,SPEED0
                // obj.output( [PULLUP|PULLDOWN], [ODRAIN], [SPEED1|2|3] )
                template<typename...Ts>
                auto
alternate       (MCU::ALTFUNC e, Ts...ts)
                {
                CPU::InterruptLock lock;
                deinit();
                reg_.AFR or_eq (e<<(pin_*4));
                reg_.MODER or_eq (2<<(pin_*2));
                modifiers_( ts... );
                //return *this; //nothing else can be done so disallow method chaining
                }

                //no modifiers available
                auto 
analog          ()
                {
                CPU::InterruptLock lock;
                deinit();
                reg_.MODER or_eq (3<<(pin_*2));
                //return *this; //no modifiers available, disallow method chaining
                }



                //the following functions used after pin configuration is applied

                /*
                if there is a need to allow a specific config change on an existing configured pin
                add here as needed
                such as a need to change pullups without a pin reconfig-
        
                auto
pull            (PULL e)
                {
                CPU::InterruptLock lock;
                reg_.PUPDR = (reg_.PUPDR bitand compl (1<<(pin_*2))) bitor (e<<(pin_*2));
                return *this;     
                }
                */


                // write (BSRR/BRR)
                auto
high            () { reg_.BSRR = pinbm_; return *this; }
                auto
low             () { reg_.BRR = pinbm_; return *this; }
                // variations
                auto
on              () { return inv_ ? low() : high(); }
                auto
off             () { return inv_ ? high() : low(); }
                auto
on              (bool tf) { return tf ? on() : off(); }
                auto
toggle          () { return isLatHigh() ? low() : high(); }

                // read pin (IDR)
                auto
isHigh          () { return reg_.IDR bitand pinbm_; }
                // variations
                auto
isLow           () { return not isHigh(); }
                auto
isOn            () { return inv_ ? isLow() : isHigh(); }
                auto
isOff           () { return not isOn(); }

                // read lat (ODR)
                bool
isLatHigh       () { return reg_.ODR bitand pinbm_; }
                // variations
                auto
isLatLow        () { return not isLatHigh(); }
                auto
isLatOn         () { return inv_ ? isLatLow() : isLatHigh(); }
                auto
isLatOff        () { return not isLatOn(); }
                
                }; //GpioPin
//......................................................................................
