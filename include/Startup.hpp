#pragma once
#include "Util.hpp"
#include MY_MCU_HEADER

//........................................................................................

                // vars and constants

                /*------------------------------------------------------------------------
                .ramvector section added to linker script, the section starts at the
                first location in ram so is aligned properly, _sramvector/_eramvector
                values added so we can init ram vector table in this startup code

                .debugram section added to linker script so we can have some ram at a
                known/fixed location to put debug info in when needed (such as exceptions)

                debugramStart[0-7]  exception stack data (r0-r3,r12,lr,pc,xpsr)
                debugramStart[9-29] free for any use
                debugramEnd[-1]     a fixed key value to detect power up
                debugramEnd[-2]     reset count (not power on reset)

                symbols declared as an array as it seems the most flexible and easy
                to deal with (can be treated as a pointer or array, and we get its
                address without & and can use as an array without array-bounds warnings)

                relocate and bss are described as u8's as we are using memcpy/memset
                only on these, and as a u8 there is no need to translate the size
                when subtracting - as u32's, need (_ebss-_sbss)*4 for byte size
                ------------------------------------------------------------------------*/

                //linker script symbols
                extern "C" u32 _etext       []; //end of text
                extern "C" u32 _sramdebug   []; //ram set aside for debug use
                extern "C" u32 _eramdebug   [];
                extern "C" u8  _srelocate   []; //data (initialized)
                extern "C" u8  _erelocate   [];
                extern "C" u8  _sbss        []; //bss (zeroed)
                extern "C" u8  _ebss        [];
                extern "C" u32 _sstack      []; //stack address start
                extern "C" u32 _estack      []; //stack address end

                //vector table in ram, stores void(*)() function pointers
                extern "C" vvfunc_t _sramvectors[CPU::VECTORS_SIZE];

                //setup linker symbols with nicer names
                static constexpr auto dataFlashStart      { _etext };
                static constexpr auto debugramStart       { _sramdebug };
                static constexpr auto debugramEnd         { _eramdebug };
                static constexpr auto dataStart           { _srelocate };
                static constexpr auto dataEnd             { _erelocate };
                static constexpr auto bssStart            { _sbss };
                static constexpr auto bssEnd              { _ebss };
                static constexpr auto stackTop            { _estack };

                using DebugRam = struct {
                    u32 EXCEPTION_STACK[8]; //exception stack data (r0-r3,r12,lr,pc,xpsr)
                    u32 FREE[21]; //free for any use
                    u32 RESET_COUNT; //other than power on
                    u32 KEY;
                    };
                static inline DebugRam& debugram{ *reinterpret_cast<DebugRam*>(_sramdebug) };

                //used to detect power on (without the use of reset flags)
                //if this value not seen in debugram.KEY, debugram struct will be cleared
                //in startup code, after which the debugram will act like noinit data
                //and its values will survive as long as power is provided
                static constexpr auto DEBUGRAM_KEY{ 0x13245768 };
  
//........................................................................................