#pragma once

#include <type_traits>
               

//........................................................................................

////////////////
namespace
CPU             
////////////////
                {

                class InterruptLock; //declare (if not inlcuded ahead of this header)

//........................................................................................

                // prepending atom_ to var names to indicate var is atomic
                // mainly to prevent double protecting because you may forget
                // the var was declared as AtomRW type
                //
                // AtomRW<u32> atom_mySharedVar;
                // atom_mySharedVar++; //atomic
                // struct Reg { AtomRW<u32> atom_CTRLA; };
                // Reg& reg{ *reinterpret_cast<Reg*>(0x40001000) };
                // reg.atom_CTRLA or_eq 1; //atomic

////////////////
template
<typename T>
class 
Atom          
////////////////
                {

                static_assert( std::is_integral<T>::value, "AtomRW T type is not integral" );

public:

                static constexpr auto is_cpu_atomic_read{ alignof(T) <= 4 }; //cpu specific

                //allow direct access if needed (like if irq's are already off so do not
                //want irq protection used)
                //make T volatile if not already (otherwise no need to interrupt protect)
                volatile T value; 


                //read
operator T      () const 
                {
                if constexpr( is_cpu_atomic_read ) return value;
                InterruptLock lock; //64bits, so protect read
                return value; 
                }  

                //write (assignment)
                T    
operator =      (const T v) 
                {
                if constexpr( is_cpu_atomic_read ) return value = v;
                InterruptLock lock;
                return value = v; 
                }  

                //prefix ++ / --
                T       operator ++     ()          { InterruptLock lock; return ++value;    }
                T       operator --     ()          { InterruptLock lock; return --value;    }
                //postfix ++ / -- (returns pre inc/dec value)
                T       operator ++     (const int) { InterruptLock lock; T v = value++; return v; }
                T       operator --     (const int) { InterruptLock lock; T v = value--; return v; }
                //other writes
                T       operator +=     (const T v) { InterruptLock lock; return value += v;  }
                T       operator -=     (const T v) { InterruptLock lock; return value -= v;  }
                T       operator *=     (const T v) { InterruptLock lock; return value *= v;  }
                T       operator /=     (const T v) { InterruptLock lock; return value /= v;  }
                T       operator %=     (const T v) { InterruptLock lock; return value %= v;  }
                T       operator ^=     (const T v) { InterruptLock lock; return value ^= v;  }
                T       operator &=     (const T v) { InterruptLock lock; return value &= v;  }
                T       operator |=     (const T v) { InterruptLock lock; return value |= v;  }
                T       operator >>=    (const T v) { InterruptLock lock; return value >>= v; }
                T       operator <<=    (const T v) { InterruptLock lock; return value <<= v; }

                //atomic clear mask, then set mask
                //caller responsible for getting both arguments correct
                //var.setBitmask( 0b11, 0b10 ); bit[1:0] will be set to 2 (0b10), 
                //all other bits unchanged
                T 
setBitmask      (T clrbm, T setbm)
                {  
                InterruptLock lock;
                auto v = (value bitand compl clrbm) bitor setbm;
                value = v; 
                return v;
                }

                }; //Atom

//........................................................................................

                //can use to create a reference to an AtomRW var which
                //only allows read-only access, but may still require an
                //atomic read 

                //not sure if this is useful

////////////////
template
<typename T>
class 
AtomRO          : Atom<T>
////////////////
                {

                //do not allow creating an instance (private constructor)
                //(if you want to create a read-only var, there is no need for this class)
AtomRO          (){} 

public:
                //can only read
                using Atom<T>::operator T;

                }; //AtomRO

//........................................................................................

                } //namespace CPU

//........................................................................................
                