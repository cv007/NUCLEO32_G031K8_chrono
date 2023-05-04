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
Scb   
////////////////
                {

                struct Reg {
                    u32 CPUID, ICSR, VTOR, AIRCR, SCR, CCR, unused1_, SHPR2, 
                    SHPR3, SHCSR, unused2_[2], DFSR;
                    };
                static inline volatile Reg& scb_{ *reinterpret_cast<Reg*>(0xE000'ED00) };

                enum { AIRCR_RESET_CODE = 0x05FA0004 };
                enum { ICSR_PENDSTSETbm = 1<<26, ICSR_PENDSTCLRbm = 1<<25 };

public:
                //software reset
                [[ using gnu : used, noreturn ]] static void
swReset         ()
                {
                asm( "dsb 0xF":::"memory");
                scb_.AIRCR = AIRCR_RESET_CODE;
                asm( "dsb 0xF":::"memory");
                while( true ){}
                }

                template<typename T> //any type converted to a u32
                [[ gnu::always_inline ]] static auto
vectorTable     (T addr){ scb_.VTOR = reinterpret_cast<u32>(addr); }

                // get systick irq pending bit, optionally clear also                
                static auto
isSystickPending(bool clear = false) 
                { 
                bool tf = scb_.ICSR bitand ICSR_PENDSTSETbm;
                if( clear ) scb_.ICSR = ICSR_PENDSTCLRbm;
                return tf; 
                }

                // get current active isr number - convert 0 based value to IRQn
                static MCU::IRQn
activeIrq       () { return MCU::IRQn( (scb_.ICSR bitand 0x3F) - 16 ); }

                }; //Scb

//........................................................................................

                } //namespace CPU

//........................................................................................