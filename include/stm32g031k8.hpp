#pragma once

#include "Util.hpp"
#include "CpuM0plus_Atom.hpp"

//........................................................................................

////////////////
namespace
MCU             
////////////////
                {
                //mcu specific info, stm32g031k8

                //some of this may find a better home (like Rcc)

                enum {
                    GPIO_BASE = 0x5000'0000, GPIO_SPACING = 0x400 
                    };

                enum { 
                    RCC_BASE = 0x4002'1000, 
                    RCC_USART2ENbm = 1<<17,
                    RCC_LPTIM2ENbm = 1<<30, RCC_LPTIM1ENbm = 1<<31, LPTIM2SELbp = 20, LPTIM1SELbp = 18, LPTIMSELbm = 3,
                    LSIONbm = 1 
                    };

                //make all writes atomic so any Rcc writes can occur at multiple irq levels
                //without concern for register corruption
                struct RccReg {
                    CPU::Atom<u32> CR, ICSCR, CFGR, PLLCFGR, RESERVED0, RESERVED1, CIER, CIFR, CICR,     
                    IOPRSTR, AHBRSTR, APBRSTR1, APBRSTR2, IOPENR, AHBENR, APBENR1, APBENR2,
                    IOPSMENR, AHBSMENR, APBSMENR1, APBSMENR2, CCIPR, RESERVED2, BDCR, CSR;
                    };
                static inline RccReg& RCCreg{ *reinterpret_cast<RccReg*>(RCC_BASE) };

                enum 
USARTn          { USART1_BASE = 0x4001'3800, USART2_BASE = 0x4000'4400 };

                enum 
LPTIMn          { LPTIM1_BASE = 0x4000'7C00, LPTIM2_BASE = 0x4000'9400 };

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
IRQn            : int { SYSTICK_IRQ = -1, USART1_IRQ = 27, USART2_IRQ, LPTIM1_IRQ = 17, LPTIM2_IRQ };


                //used by Uart class
                using 
uart_t          = struct {
                    USARTn      addr;
                    PIN         txPin;
                    ALTFUNC     txAltFunc;
                    PIN         rxPin;
                    ALTFUNC     rxAltFunc;
                    vvfunc_t    init; //such as enable uart in rcc
                    IRQn        irqn;
                    };

                //Uart2, TX=PA2,RX=PA3
                static constexpr uart_t
Uart2_A2A3      {   USART2_BASE,    //Usart2 base address
                    PA2, AF1,       //tx pin, alt function
                    PA3, AF1,       //rx pin, alt function
                    []{ RCCreg.APBENR1 or_eq RCC_USART2ENbm; }, //init rcc
                    USART2_IRQ      //IRQn
                    };
               

                using 
lptim_t         = struct {
                    LPTIMn      addr;
                    vvfunc_t    init; //such as enable uart in rcc
                    IRQn        irqn;
                    };

                static constexpr lptim_t
Lptim1LSI       {   LPTIM1_BASE,
                    []{ //init
                        RCCreg.CSR = LSIONbm;
RCCreg.APBRSTR1 or_eq RCC_LPTIM1ENbm;
RCCreg.APBRSTR1 and_eq compl RCC_LPTIM1ENbm;
                        RCCreg.APBENR1 or_eq RCC_LPTIM1ENbm;
                        RCCreg.CCIPR = (RCCreg.CCIPR bitand compl (LPTIMSELbm<<LPTIM1SELbp)) bitor (1<<LPTIM1SELbp);
                        }, 
                    LPTIM1_IRQ
                    };

                static constexpr lptim_t
Lptim2LSI       {   LPTIM2_BASE,
                    []{ //init
                        RCCreg.CSR = LSIONbm;
RCCreg.APBRSTR1 or_eq RCC_LPTIM2ENbm;
RCCreg.APBRSTR1 and_eq compl RCC_LPTIM2ENbm;
                        RCCreg.APBENR1 or_eq RCC_LPTIM2ENbm;
                        RCCreg.CCIPR = (RCCreg.CCIPR bitand compl (LPTIMSELbm<<LPTIM2SELbp)) bitor (1<<LPTIM2SELbp);
                        },
                    LPTIM2_IRQ
                    };

                } //MCU

//........................................................................................

#include "CpuM0plus.hpp"

//........................................................................................