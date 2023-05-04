#pragma once

#include "NiceTypes.hpp"


//........................................................................................

////////////////
namespace
UTIL            
////////////////
                {
                //cast a 32bit value to a volatile u32&
                //for easier use when using for example an enum value as a register
                // address which you want to write to-
                // vu32Ref(SOME_ENUM) = 0; //writes 0 to the address given
                static inline auto&
vu32Ref         (u32 addr){ return *(reinterpret_cast<volatile u32*>(addr)); }

                //types used often enough
                using vvfunc_t = void(*)();
                using bvfunc_t = bool(*)();

                template<typename T, int N> static constexpr auto
arraySize       (const T(&t)[N]){ (void)t; return N; }

                } //UTIL

//........................................................................................

                //optional- bring into global namespace
                using namespace UTIL;
