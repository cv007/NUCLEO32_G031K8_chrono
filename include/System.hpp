#pragma once
#include "Util.hpp"
#include MY_MCU_HEADER

//........................................................................................

////////////////
class
System          
////////////////
                {
                static inline u32 cpuHz_{ MCU::CPUHZ_DEFAULT };

                //function(s) allowed to set cpu speed
                friend int main(); //only main in this case

                static auto
cpuHz           (u32 hz){ cpuHz_ = hz; }

public:

                enum 
INFO_CODES      { BOOT_CODE = 33 };

                static auto
cpuHz           (){ return cpuHz_; }



                };

//........................................................................................
