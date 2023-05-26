#pragma once

#include "Util.hpp"
#include "GpioPin.hpp"
#include "Systick.hpp"
#include "Lptim.hpp"

//........................................................................................

////////////////
class
Led             : GpioPin
////////////////               
                {

                // static inline auto& delay = Systick::delay; //'alias' name
                static inline auto& delay = Lptim1ClockLSI::delay; //'alias' name
                // using duration = Systick::duration;
                using duration = Lptim1ClockLSI::duration;
                using milliseconds = std::chrono::milliseconds;
public:

Led             (MCU::PIN pin, INVERT inv = HIGHISON)
                : GpioPin(pin,inv)
                {
                off().mode(OUTPUT); //off before output, so no glitch if inverted
                }

                //only these GpioPin functions allowed
                using 
GpioPin::on,    
GpioPin::off, 
GpioPin::toggle,
GpioPin::isOn, 
GpioPin::isOff;

                //on+off duration (1 blink), n times (1-255)
                void
blink           (duration d, u8 n)
                {
                if( n == 0 or d.count() == 0 ) return;
                u32 n2 = n*2;       //toggle, so *2
                d = d/2;            //is total on+off time, split to get each period
                auto save = isOn(); //save state
                off(); delay( d*4 );//in case was on, give some off time first
                while( n2-- ){ toggle(); delay( d ); }
                if( save ){ off(); delay( d*4 ); on(); }//off time space if turing back on                
                }

                //info codes 11-98, do not use 0 in any code value (20,30,...)
                //any use of 0 or code value out of range will produce code 99 (bad code)
                auto 
infoCode        (u8 code, u8 repeatN = 1, u16 ms = 300)
                {
                if( code < 11 ) code = 99;
                if( code > 98 ) code = 99;
                if( (code % 10) == 0 ) code = 99;
                auto dur = milliseconds(ms);
                while( repeatN-- ){
                    blink( dur, (code/10) );    //high number first (tens)                  
                    blink( dur, (code % 10) );  //ones                              
                    delay( dur*4 );             //pause between any repeats
                    }
                }

                //for Ownerhip class, led is always idle
                bool
isIdle          (){ return true; }

                }; //Led

//........................................................................................
