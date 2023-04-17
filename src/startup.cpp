//startup file for STM32G031K8
#include "Startup.hpp"

//........................................................................................

                // function declarations
                int main();
                static void resetFunc();
                static void errorFunc();
                extern "C" void __libc_init_array();

//........................................................................................
                /*
                reset vector block, 4 words -
                    stack value, reset handler/nmi/hardfault addresses
                ram will be used for vector table, so only need these 4 (really only need
                    reset handler address, but if hardfault takes place between reset and
                    setting vtor we will end up in a known location- errorFunc)
                section is KEEP in linker script, 'used' keeps compiler from complaining
                */
                using flashVector_t = struct { u32* stack; void(*vfunc[3])(); };

                [[ using gnu : section(".vectors"), used ]] flashVector_t 
                flashVector { stackTop, {resetFunc, errorFunc, errorFunc} };

//........................................................................................

                //unhandled interrupt, exception, or return from main
                [[ using gnu : used, noreturn ]] static void
errorFunc       ()
                {
                //get main stack pointer, get the 8 stack entries which
                //were possibly set by exception (r0-r3,r12,lr,pc,xpsr)
                //and copy to debug ram
                u32* msp;
                u32* ram = debugramStart;
                asm( "MRS %0, msp" : "=r" (msp) );
                for( auto i = 0; i < 8; i++ ) *ram++ = *msp++;
                //other data can be saved if wanted (20 more u32's available)
                //then software reset
                Scb::swReset();
                }

                static auto
initRamDebug    ()
                {
                //if debug ram key value is not set,
                //clear debug ram and set to key value
                if( debugram.KEY != DEBUGRAM_KEY ) {
                    __builtin_memset( debugramStart, 0, sizeof(DebugRam) );
                    debugram.KEY = DEBUGRAM_KEY;
                    }
                //else inc reset count, and leave debug ram as-is
                //(may be exception data stored we want to see)
                else debugram.RESET_COUNT++;
                }

                static auto
initRamVectors  ()
                {
                //set all ram vectors to default function (errorFunc)
                //also set stack/reset in case vtor address is used to read these values
                for( auto& v : _sramvectors ) v = errorFunc;
                _sramvectors[0] = reinterpret_cast<vvfunc_t>(stackTop);
                _sramvectors[1] = resetFunc;
                Scb::vectorTable( &_sramvectors[0] ); //move vectors to ram
                }

                static auto
initRamData     ()
                {
                //init data from flash, clear bss
                __builtin_memcpy( dataStart, dataFlashStart, dataEnd - dataStart );
                __builtin_memset( bssStart, 0, bssEnd - bssStart );
                }

                [[ using gnu : used, noreturn ]]
                static void
resetFunc       ()
                {
                CPU::delayMS(3000);    //allow time to allow swd hot-plug
                initRamDebug();
                initRamVectors();
                initRamData();
                __libc_init_array();        //libc init, c++ constructors, etc.

                //C++ will not allow using main with pendatic on, so disable pedantic
                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wpedantic"
                main();
                #pragma GCC diagnostic pop

                //should not return from main, so treat as an error
                errorFunc();
                }
