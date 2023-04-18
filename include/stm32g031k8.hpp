#pragma once
#include "Util.hpp"

//........................................................................................

////////////////
namespace
MCU             
////////////////
                {
                //mcu specific info, stm32g031k8

                //some addresses to make this example work

                enum
                { GPIOA = 0x5000'0000, GPIO_SPACING = 0x400 };

                enum
                { RCC_BASE = 0x4002'1000, RCC_IOPENR = RCC_BASE+0x34,
                  RCC_APBENR1 = RCC_BASE+0x3C, RCC_USART2ENbm = 1<<17 };

                enum 
                { USART1_BASE = 0x4001'3800, USART2_BASE = 0x4000'4400,
                  USARTn_COUNT = 2 };


                enum
PIN             { // 0bPPPPpppp P=port 0-n, p=pin 0-15, enum=port*16+p, port=enum/16, pin=enum%16
                PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7,
                PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
                PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7,
                PB8, PB9,
                PC6 = (2*16)+6, PC14 = (2*16)+14, PC15,
                //can exclude any pins above you wish to prevent using,
                //or add an alias if wanted-
                SWDIO = PA13, SWCLK = PA14
                };

                enum //AF3 unused for this series
ALTFUNC         { AF0, AF1, AF2, AF4 = 4, AF5, AF6, AF7 }; 

                enum { CPUHZ_DEFAULT = 16'000'000 }; //default cpu speed


                enum
IRQn            { SYSTICK_IRQ = -1, USART1_IRQ = 27, USART2_IRQ };


                //used by Uart class
                using 
uart_t          = struct {
                    u32         uartAddr;
                    PIN         txPin;
                    ALTFUNC     txAltFunc;
                    PIN         rxPin;
                    ALTFUNC     rxAltFunc;
                    vvfunc_t    init; //such as enable uart in rcc
                    IRQn        irqn;
                    };

                static constexpr uart_t
Uart2_A2A3      { //Uart2, TX=PA2,RX=PA3
                USART2_BASE,    //Usart2 base address
                PA2, AF1,       //tx pin, alt function
                PA3, AF1,       //rx pin, alt function
                []{ vu32Ref(RCC_APBENR1) or_eq RCC_USART2ENbm; }, //init rcc
                USART2_IRQ      //IRQn
                };
               
                } //MCU

//........................................................................................

                #include "CpuM0plus.hpp"

//........................................................................................