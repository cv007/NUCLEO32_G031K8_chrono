#pragma once
//included by MY_MCU_HEADER


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
delayCycles     (volatile u32 n) { while( n -= CYCLES_PER_LOOP, n >= CYCLES_PER_LOOP ){} }

                [[ gnu::always_inline ]] inline static auto
delayMS         (const u16 ms, const u32 cpuHz = MCU::CPUHZ_DEFAULT){ delayCycles(cpuHz/1000*ms); }
                #pragma GCC pop_options

//........................................................................................

//////////////// CPU::
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

//////////////// CPU::
class
Scb   
////////////////
                {

                struct ScbReg {
                    u32 CPUID, ICSR, VTOR, AIRCR, SCR, CCR, unused1_, SHPR2, 
                    SHPR3, SHCSR, unused2_[2], DFSR;
                    };
                static inline volatile ScbReg& scb_{ *reinterpret_cast<ScbReg*>(0xE000'ED00) };

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

                //matches what is in Startup.hpp
                extern "C" vvfunc_t _sramvectors[VECTORS_SIZE];

//////////////// CPU::
class
Nvic           
////////////////
                {

                struct NvicReg { 
                    u32 ISER, reserved0[31], ICER, reserved1[31], ISPR,
                    reserved2[31], ICPR, reserved3[31+64], IP[8]; 
                    };

                static inline volatile NvicReg& nvic_{ *(reinterpret_cast<NvicReg*>(0xE000'E100)) };

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

                static auto
setFunction     (MCU::IRQn n, vvfunc_t f, IRQ_PRIORITY pri = PRIORITY0)
                {
                CPU::_sramvectors[16+n] = f;
                priority( n, pri );
                enableIrq(n);
                }

                static auto
deleteFunction  (MCU::IRQn n)
                {
                extern void errorFunc(); //default interrupt handler in startup
                disableIrq(n);
                CPU::_sramvectors[16+n] = errorFunc;
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

                } //CPU

                //bring these classes into global namespace
                using CPU::InterruptLock;
                using CPU::Nvic;
                using CPU::Scb;

//........................................................................................
