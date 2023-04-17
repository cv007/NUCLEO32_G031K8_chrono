#pragma once
#include "NiceTypes.hpp"
#include <random>
#include <type_traits>


//........................................................................................

                //linker symbols, unitialized ram start/end of stack space 
                //can use for seed values
                extern u32 _sstack[]; 
                extern u32 _estack[]; 

//........................................................................................

////////////////
class 
RandomGenLFSR16
////////////////
                {

                //using something more lightweight than what is in <random>
                //most likely not as good, but good enough for simple needs

                // poly values and idea from-
                // https://www.maximintegrated.com/en/design/technical-documents/app-notes/4/4400.html

                u32 seed0_, seed1_; //save so we can see what the seed values were
                u32 lfsr31_, lfsr32_;
                static constexpr u32 POLY_MASK32 { 0xB4BCD35C };
                static constexpr u32 POLY_MASK31 { 0x7A5BC2E3 };

public:

                //initial seed values from unitialized ram
                //whether power on or reboot, these values will be unpredictable
RandomGenLFSR16 ()
                {
                u32 s0{0}, s1{0};
                for( auto ramptr = _sstack; ramptr < (_estack-2); ){
                    s0 xor_eq *ramptr++;
                    s1 xor_eq *ramptr++;
                    }
                seed0_ = s0 ? s0 : 0x12345678; //seeds cannot be 0
                seed1_ = s1 ? s1 : 0x87654321; //so set to some value if 0
                lfsr31_ = seed0_;
                lfsr32_ = seed1_;
                }

                using result_type = u16;

                static constexpr result_type
min             (){ return 0; }
                static constexpr result_type
max             (){ return 0xFFFF; }

                auto 
seed0           (){ return seed0_; }
                auto 
seed1           (){ return seed1_; }

                result_type 
operator()      ()
                {
                auto shift = [] (u32& v, u32 mask) { //local function
                    auto bit0 = v bitand 1;
                    v >>= 1;
                    return bit0 ? v xor_eq mask : v;
                    };
                shift(lfsr32_, POLY_MASK32); //this one done 2x
                return shift(lfsr32_, POLY_MASK32) xor shift(lfsr31_, POLY_MASK31);
                }

                }; //RandomGenLFSR16

//........................................................................................

////////////////
class 
Random
////////////////
                {

                // using param_type = std::uniform_int_distribution<u32>::param_type;
                // std::uniform_int_distribution<u32> dist_;

                RandomGenLFSR16 rg_;

public:

                //added just to view the seed values generated from ram
                //in RandomGenLFSR16
                auto 
seed0           (){ return rg_.seed0(); }
                auto 
seed1           (){ return rg_.seed1(); }

                // ~220 cpu clocks
                // read()       - returns u32
                // read<u16>()  - returns u16
                // read<u8>()   - returns u8
                // 
                template<typename T = u32> T
read            (T min = 0, T max = sizeof(T) == 1 ? 0xFF : sizeof(T) == 2 ? 0xFFFF : 0xFFFFFFFF )
                { 
                static_assert( std::is_integral<T>::value,
                               "Random::read<T>() only allows integral types for the template parameter" );
                //~115 cpu cycles
                u32 r = rg_(); //rg_ produces 16bit value
                if constexpr ( sizeof(T) == 4 ) r = (r<<16) + rg_(); //need another 16bits
                if( min == 0 and ((max+1) == 0) ) return r; //full range, so just return r
                return static_cast<T>( (r % (max-min+1)) + min ); //keep inside range

                // ~220 cpu cycles
                // using uniform_int_distribution (may not be great idea to change param this often)
                // const param_type p{ min, max };
                // dist_.param( p );
                // return dist_( rg_ );
                }

                }; //Random0

//........................................................................................

                inline Random randgen;

//........................................................................................