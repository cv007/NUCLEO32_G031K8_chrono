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

////////////////
template
<typename T>
class 
AtomRW          
////////////////
                {

                static_assert( std::is_integral<T>::value, "AtomRW T type is not integral" );

public:

                static constexpr auto is_mcu_atomic_read{ alignof(T) <= 4 }; //mcu specific

                //allow direct access if needed (like if irq's are already off so do not
                //want irq protection used)
                volatile T value; 


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
                auto v = (value bitand compl clrbm) bitor setbm;
                value = v; 
                return v;
                }

                }; //AtomRW

//........................................................................................

////////////////
template
<typename T>
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

                } //namespace CPU

//........................................................................................
                