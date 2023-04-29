#pragma once
//included by MY_MCU_HEADER
#include <type_traits>

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

////////////////
template<
typename T 
>
class 
AtomRW          
////////////////
                {

                static_assert( std::is_integral<T>::value, "AtomRW T type is not integral" );

public:
                using type = volatile T; //make value type volatile if not already
                static constexpr auto is_mcu_atomic_read{ alignof(type) <= 4 }; //mcu specific

                //allow direct access if needed (like if irq's are already off so do not
                //want irq protection used)
                type value; 


operator T      () const 
                {
                if constexpr( is_mcu_atomic_read ) return value;
                InterruptLock lock; //64bits, so protect read
                return value; 
                }  

                T    
operator =      (const T v) 
                {
                if constexpr( is_mcu_atomic_read ) return value = v;
                InterruptLock lock;
                return value = v; 
                }  

                // u32     operator &      () const    { return &value; }   
                T       operator ++     (const int) { InterruptLock lock; return value = value + 1;  }
                T       operator --     (const int) { InterruptLock lock; return value = value - 1;  }
                T       operator +=     (const T v) { InterruptLock lock; return value = value + v;  }
                T       operator -=     (const T v) { InterruptLock lock; return value = value - v;  }
                T       operator ++     ()          { InterruptLock lock; return value = value + 1;  }
                T       operator --     ()          { InterruptLock lock; return value = value - 1;  }
                T       operator *=     (const T v) { InterruptLock lock; return value = value * v;  }
                T       operator /=     (const T v) { InterruptLock lock; return value = value / v;  }
                T       operator %=     (const T v) { InterruptLock lock; return value = value % v;  }
                T       operator ^=     (const T v) { InterruptLock lock; return value = value ^ v;  }
                T       operator &=     (const T v) { InterruptLock lock; return value = value & v;  }
                T       operator |=     (const T v) { InterruptLock lock; return value = value | v;  }
                T       operator >>=    (const T v) { InterruptLock lock; return value = value >> v; }
                T       operator <<=    (const T v) { InterruptLock lock; return value = value << v; }

                T 
setBitmask      (T clrbm, T setbm)
                {  
                InterruptLock lock;
                value = (value bitand compl clrbm) bitor setbm; 
                return value;
                }

                T 
setBitmask      (T setbm)
                {  
                InterruptLock lock;
                value = value bitor setbm; 
                return value;
                }


                }; //AtomRW

//........................................................................................

////////////////
template<
typename T 
>
class 
AtomRO          
////////////////
                {

                static_assert( std::is_integral<T>::value, "AtomRO T type is not integral" );

public:

                using type = volatile T; //make value type volatile if not already
                static constexpr auto is_mcu_atomic_read{ alignof(type) <= 4 }; //mcu specific

                //allow direct access if needed (like if irq's are already off so do not
                //want irq protection used)
                const type value; 

operator T      () const 
                { 
                if constexpr( is_mcu_atomic_read ) return value;
                InterruptLock lock;
                return value; 
                }

                }; //AtomRO

//........................................................................................

//////////////// CPU::
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

                //peripheral class which does not use static methods needs a way to hook
                //up the vector table function pointer to a class object
                //any peripheral class that needs to do so, can inherit this class and
                //create the required virtual isr function

                // interrupt-
                //      address in vector table read (_sramvectors[n])
                //      jump to address from vector table read, which is func()
                //      func()- reads active irq number (IRQn type- -15 to 32)   
                //      lookup ClassIsr pointer in vectorTable_, if not 0 call
                //      isr virtual function

//////////////// CPU::
class
ClassIsr           
////////////////
                {

                static inline ClassIsr* vectorTable_[VECTORS_SIZE];

                virtual void
isr             () = 0;

public:

                static auto
setFunction     (MCU::IRQn n, ClassIsr* p)
                { vectorTable_[16+n] = p; }

                static auto
func            ()
                {
                auto n = Scb::activeIrq();
                auto objPtr = vectorTable_[16+n]; 
                if( objPtr ) objPtr->isr();
                }

                };

//........................................................................................

                //matches what is in Startup.hpp
                extern "C" vvfunc_t _sramvectors[VECTORS_SIZE];

//////////////// CPU::
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
                _sramvectors[16+n] = f;
                priority( n, pri );
                enableIrq(n);
                }

                static auto
deleteFunction  (MCU::IRQn n)
                {
                extern void errorFunc(); //default interrupt handler in startup
                disableIrq(n);
                _sramvectors[16+n] = errorFunc;
                ClassIsr::setFunction(n, 0);
                }

                static auto
setFunction     (MCU::IRQn n, ClassIsr* p, IRQ_PRIORITY pri = PRIORITY0)
                {
                ClassIsr::setFunction(n, p);
                _sramvectors[16+n] = ClassIsr::func;
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

                } //CPU

                //bring these classes into global namespace
                using CPU::InterruptLock;
                using CPU::Nvic;
                using CPU::Scb;

//........................................................................................
