
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
Nvic           
////////////////
                {

                struct Reg { 
                    u32 ISER, reserved0[31], ICER, reserved1[31], ISPR,
                    reserved2[31], ICPR, reserved3[31+64], IP[8]; 
                    };

                static inline volatile Reg& nvic_{ *(reinterpret_cast<Reg*>(0xE000'E100)) };

public:

                enum
IRQ_PRIORITY    { PRIORITY0, PRIORITY1, PRIORITY2, PRIORITY3 };

private:
                static void
enableIrq       (MCU::IRQn n) { if( n >= 0 ) nvic_.ISER = 1<<n; }

                static void
disableIrq      (MCU::IRQn n)
                {
                if( n < 0 ) return;
                nvic_.ICER = 1<<n;
                //taken from cmsis equivalent-
                asm volatile("dsb 0xF":::"memory");
                asm volatile("isb 0xF":::"memory");
                }

public:
                // setFunction() - set interrupt function in ram vector table
                // function addresses will already have bit0 set
                // table offset [16] is for IRQn 0, so using [16+n]
                // the nvic irq will also be enabled

                // deleteFunction() - set to default interrupt handler, disable irq

                // no other options for enable/disable, but can do on your own
                // these simply provide a function entry into the table, or remove it

                //also can use a class isr, which will store a ClassIsr pointer to get
                //to the class isr function

                static auto
setFunction     (MCU::IRQn n, vvfunc_t f, IRQ_PRIORITY pri = PRIORITY0)
                {
                Isr::setFunction(n, f);
                priority( n, pri );
                enableIrq(n);
                }

                static auto
deleteFunction  (MCU::IRQn n)
                {
                extern void errorFunc(); //default interrupt handler in startup
                disableIrq(n);
                Isr::setFunction(n, errorFunc);
                }

                static auto
setFunction     (MCU::IRQn n, Isr* p, IRQ_PRIORITY pri = PRIORITY0)
                {                
                Isr::setFunction(n, p);
                priority( n, pri );
                enableIrq(n);
                }

                static void
priority        (MCU::IRQn n, IRQ_PRIORITY pri)
                {
                if( n < 0 ) return; //not handling negative number irq's here
                auto& ip = nvic_.IP[n / 4]; //0=0,1=0,...,31=7
                //bits 7:6 of each byte holds priority, 5:0 unused/0
                auto bp = 8*(n % 4)+6; //0,8,16,24 -> 6,14,22,30
                auto bmclr = 3<<bp;
                auto bmset = pri<<bp;
                InterruptLock lock;
                ip = (ip bitand compl bmclr) bitor bmset;
                }

                static IRQ_PRIORITY
priority        (MCU::IRQn n)
                {
                if( n < 0 ) return PRIORITY0; //not handling negative number irq's here
                auto& ip = nvic_.IP[n / 4]; //0=0,1=0,...,31=7
                auto bp = 8*(n % 4)+6; //0,8,16,24 -> 6,14,22,30
                //return static_assert<IRQ_PRIORITY>( (ip >> bp) bitand 3 );
                return IRQ_PRIORITY( (ip >> bp) bitand 3 );
                }

                static IRQ_PRIORITY
activePriority  (){ return priority( Scb::activeIrq() ); }

                static auto
clearPending    (MCU::IRQn n){ nvic_.ICPR = (1<<n); }

                }; //Nvic

//........................................................................................

                } //namespace CPU

//........................................................................................
