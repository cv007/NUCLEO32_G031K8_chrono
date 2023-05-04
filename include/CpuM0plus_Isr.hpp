#pragma once 

#include "NiceTypes.hpp"
#include "CpuM0plus_Scb.hpp"

//uses MCU::IRQn, VECTORS_SIZE, Scb


//........................................................................................

////////////////
namespace
CPU             
////////////////
                {

//........................................................................................

                //peripheral class which does not use static methods needs a way to hook
                //up the vector table function pointer to a class object
                //any peripheral class that needs to do so, can inherit this class and
                //create the required virtual isr function

                // interrupt-
                //      address in vector table read (_sramvectors[n])
                //      jump to address from vector table read, which is dispatch()
                //      dispatch()- reads active irq number (IRQn type- -15 to 32)   
                //      lookup Isr pointer in objTable_, if not 0 call
                //      isr virtual function

                //matches what is in Startup.hpp
                extern "C" vvfunc_t _sramvectors[VECTORS_SIZE];

////////////////
class
Isr           
////////////////
                {

                static inline Isr* objTable_[VECTORS_SIZE];

                virtual void
isr             () = 0;

                static void
dispatch        ()
                {
                auto n = Scb::activeIrq();
                auto objPtr = objTable_[16+n]; 
                if( objPtr ) objPtr->isr();
                }

public:
                //our static dispatch function into vector table
                //Isr* into object table
                static auto
setFunction     (MCU::IRQn n, Isr* p)
                { 
                objTable_[16+n] = p; 
                _sramvectors[16+n] = dispatch;
                }

                //function address into vector table
                //used for static functions, which can be called directly
                //via existing ram vectors
                static auto
setFunction     (MCU::IRQn n, vvfunc_t f)
                {
                _sramvectors[16+n] = f;
                }

                }; //Isr

//........................................................................................

                } //namespace CPU

//........................................................................................
