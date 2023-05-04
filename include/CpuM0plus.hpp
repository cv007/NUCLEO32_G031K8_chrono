#pragma once

                //this header included by MY_MCU_HEADER
                //uses MCU::CPUHZ_DEFAULT, MCU::IRQn, vvfunc_t

//........................................................................................

////////////////
namespace
CPU             
////////////////
                {

                enum { VECTORS_SIZE = 16+32 };

                static inline auto
waitIrq         (){ asm volatile ("wfi"); }

                #pragma GCC push_options
                #pragma GCC optimize ("-Os")
                static constexpr u32  CYCLES_PER_LOOP {4}; //for delayCycles
                [[ gnu::always_inline ]] inline static auto
delayCycles     (u32 n) { volatile u32 vn = n; while( vn = vn - CYCLES_PER_LOOP, vn >= CYCLES_PER_LOOP ){} }

                [[ gnu::always_inline ]] inline static auto
delayMS         (const u16 ms, const u32 cpuHz = MCU::CPUHZ_DEFAULT){ delayCycles(cpuHz/1000*ms); }
                #pragma GCC pop_options

                } //namespace CPU

//........................................................................................

#include "CpuM0plus_InterruptLock.hpp"  //no dependancies
#include "CpuM0plus_Atom.hpp"           //uses InterruptLock
#include "CpuM0plus_Scb.hpp"            //uses MCU::IRQn
#include "CpuM0plus_Isr.hpp"            //uses MCU::IRQn, VECTORS_SIZE, vvfunc_t
#include "CpuM0plus_Nvic.hpp"           //uses MCU::IRQn, vvfunc_t, Isr, InterruptLock


//bring these classes into global namespace
using CPU::InterruptLock;
using CPU::Nvic;
using CPU::Scb;

//........................................................................................
