#pragma once

#include "Util.hpp"
#include MY_MCU_HEADER
#include "GpioPin.hpp"
#include "Uart.hpp"
#include "Led.hpp"
#include <array>
#include "BufferBytes.hpp"
#include "Ownership.hpp"


//........................................................................................

////////////////
namespace 
Boards
////////////////
                {

//........................................................................................

////////////////
struct 
Nucleo32g031    
////////////////
                {

private:        //not directly accessible

                //uart buffer
                std::array<u8,256> uartBuffer_;
                //Uart2, TX=PA2,RX=PA3 
                Uart uart_{ MCU::Uart2_A2A3, 230400, uartBuffer_, Nvic::PRIORITY2 };

                //the uart is available for a new owner when the uart goes idle (buffer
                //empty and tx complete), so a task can fill the buffer quickly and return
                //so that the task/function is not blocking on the buffer (other tasks cannot run)
                //if the buffer is not large enough for the message to send, the task will
                //still end up waiting when the buffer fills, so size the buffer for the largest
                //message

                //uart_ (the real Uart class) only accessible via Ownership class (uart)
                //      auto id{ reinterpret_cast<u32>(myFuncName) }; //create a unique id
                //      Uart* u = board.uart.take( id ); //try to get a pointer to the uart
                //      if( not u ) return false; //null pointer returned, not available
                //      auto& uart = *u; //we are the owner, convert to reference for easier use
                //      uart << "Hello World" << endl; //use
                //      board.uart.release( id ); //release ownership
public:
                Ownership<Uart> uart{ uart_ };
               
                //fixed green led- LD3
                Led led{ MCU::PC6 };

                //board pin labels to actual pins
                static constexpr MCU::PIN D[]{ //0-12
                    MCU::PB7, MCU::PB6, MCU::PA15, MCU::PB1,
                    MCU::PA10, MCU::PA9, MCU::PB0, MCU::PB2,
                    MCU::PB8, MCU::PA8, MCU::PB9, MCU::PB5,
                    MCU::PB4
                    };
                static constexpr MCU::PIN A[]{ //0-7
                    MCU::PA0, MCU::PA1, MCU::PA4, MCU::PA5,
                    MCU::PA12, MCU::PA11, MCU::PA6, MCU::PA7
                    };

                //debug via pin, like to time various things via logic analyzer
                GpioPin debugPin = GpioPin( D[2] ).mode( GpioPin::OUTPUT );

                }; //Nucleo32g031

//........................................................................................

                }; //Boards

//........................................................................................

                //select which board to use
                using Board = Boards::Nucleo32g031;

                //and let everone know 'board' will be the name to use for the single
                //instace of Board created (in some source file)
                extern Board board;

//........................................................................................