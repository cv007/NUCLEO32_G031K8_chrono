#pragma once

#include "NiceTypes.hpp"

//........................................................................................
////////////////
namespace
CPU             
////////////////
                {

//........................................................................................

////////////////
class
InterruptLock   
////////////////
                {
                //for atomic needs
                //interrupts disabled at point of object declaration
                //end of scope will call destructor to restore irq state
                // void someFunc(){ InterruptLock lock; volatileVar++; }
                
                u32 status_;
                InterruptLock(const InterruptLock&){} //prevent copy

                //these are already in cmsis header, added here as we do not have the cmsis header
                auto
primask         (){ u32 result; asm("MRS %0, primask" : "=r" (result) :: "memory"); return(result); }
                auto
primask         (u32 priMask){ asm("MSR primask, %0" : : "r" (priMask) : "memory"); }
                auto
irqOff          (){ asm("cpsid i" : : : "memory"); }

public:

InterruptLock   (bool disableIrq = true) 
                : status_( primask() )  //1=disabled, 0=enabled
                { 
                if( disableIrq ) irqOff(); 
                }

~InterruptLock  () { primask(status_); }

                auto
wasDisabled     (){ return status_; }
                auto
wasEnabled      (){ return not wasDisabled(); }

                }; //InterruptLock

//........................................................................................

                } //namespace CPU

//........................................................................................
