////////////////
// main.cpp
////////////////
#include "NiceTypes.hpp"
#include <chrono>
#include "Random.hpp"
#include "System.hpp"
#include "Systick.hpp"
#include "Boards.hpp"
#include "Tasks.hpp"
#include "Print.hpp"
#include "MorseCode.hpp"
#include "Lptim.hpp"

//........................................................................................
                #if 0 //using Lptim1ClockLSI
                using Tasks_t = Tasks<Systick,16>;  //using Systick clock, max 16 tasks
                static Systick systimer;
                #else
                using Tasks_t = Tasks<Lptim1ClockLSI,16>;  //using Lptim1ClockLSI clock, max 16 tasks
                static Lptim1ClockLSI systimer;
                #endif

                //alias names for easier use
                auto& now = systimer.now;           //returns a chrono::time_point
                auto& delay = systimer.delay;

                using Task_t = Tasks_t::Task;       //single task type

                static Tasks_t tasks;               //list of Task_t's
                Board board;                        //the only board instance (extern in Boards)

                using namespace FMT;                //Print use
                using namespace ANSI;               //FMT::ANSI
                using namespace std::chrono;

                //use debug pin to time tasks via logic analyzer
                //turns on pin at constructor, off at desctructor
                //place in any scope you want to time
                // void func(){
                //  DebugPin dp; //object name not important, pin turns on here
                //  ...
                // } //pin turns off at end of scope
                static constexpr auto DEBUG_PIN{ true }; //false to disable
                struct DebugPin {
                    DebugPin(){ if constexpr( DEBUG_PIN ) board.debugPin.on(); }
                    ~DebugPin(){ if constexpr( DEBUG_PIN ) board.debugPin.off(); }
                };

//........................................................................................

                static bool
printTask       (Task_t& task)
                {
                Open device{ board.uart };                
                if( not device ) return false; //false = try again
                auto& uart{ *device.pointer() };

                DebugPin dp;                
                auto ti = task.interval + milliseconds(20);
                if( ti > milliseconds(600) ) ti = milliseconds(400);
                task.interval = ti; //set new interval
                static u16 n = 0;

                // , style (for << style just replace all , with <<
                uart,
                    fg(WHITE*0.4), now(), space,
                    fg(WHITE*0.4), "[", Hex0x(8,reinterpret_cast<u32>(task.func)), "] ",
                    fg(50,90,150), dec_(5,n), space,
                    fg(GREEN*1.5), Hex0x(4,n), endl;

                n++;
                device.close();
                return true;
                } //printTask

//........................................................................................

                static bool
printRandom     (Task_t& task)
                {
                Open device{ board.uart };                
                if( not device ) return false; //false = try again
                auto& uart{ *device.pointer() };

                DebugPin dp;
                auto r = random.read();

                uart,
                    fg(WHITE), now(), space,
                    fg(WHITE), "[", Hex0x(8,reinterpret_cast<u32>(task.func)), "] ",
                    fg(20,200,255), "random ",
                    fg(20,255,200), Hex0x(8,r), space,
                    fg(50,75,200), "uart buffer max used: ", uart.bufferUsedMax(),
                        endl, FMT::reset, normal;

                device.close();
                return true;
                } //printRandom



                //interval used when adding to tasks is the DOT period
                //all periods are based on the DOT period
                static bool
ledMorseCode    (Task_t&)
                {
                // "sos"
                // 10101 000 11101110111 000 10101 0..
                // |---|     |---------|     |---|
                // 1=1t on, 0=1t off (interchar spacing)
                // 000=3t off (char spacing)
                // 0000000=7t off (word spacing)
                // 00000000000000=14t off (message spacing)
                static constexpr auto msg{ "stm32 is ok" };
                static int msgIdx;
                static u32 binmask;
                static u32 bin;
                static char nextc;
                static bool isExtraSpacing;

                static Open device{ board.led };
                if( not device ) return false;
                auto& led{ *device.pointer() };

                DebugPin dp;

                if( binmask == 0 ){ 
                    if( nextc == 0 ) msgIdx = 0;
                    nextc = msg[msgIdx++];
                    auto m = MorseCode::lookup(nextc);
                    nextc = msg[msgIdx]; //look ahead
                    bin = m.bin;
                    binmask = 1<<((m.len bitand 31)-1);
                    isExtraSpacing = false;
                    }
                
                led.on( bin bitand binmask );                 
                binmask >>= 1;
                if( binmask == 0 and not isExtraSpacing ){
                    //the morse char includes the 3 off periods between chars
                    //if a space is next, add extra off periods and skip the space char
                    //if end of msg, add extra off periods, msgIdx handled elsewhere
                    bin = 0;
                    if( nextc == ' ' ){ binmask = 1<<(7-3-1); msgIdx++; }
                    if( nextc == 0 ) binmask = 1<<(14-3-1);
                    isExtraSpacing = binmask;
                    } 
                device.close();
                return true;  

                }; //LedTask

//........................................................................................

                //a class with its own tasks, so we can duplicae a number of tasks
                //to test handling the uart exclusively for each task
                //each task prints a message split in 2 so carries ownership
                //across 2 calls

////////////////
class 
SomeTasks       
////////////////
                {
                static inline SomeTasks* instances_[16];
                Task_t task_; //func unused
                u32 runCount_;
                Open<Uart> own_{ board.uart };

                bool
insert          ()
                {
                for( auto& i : instances_ ){
                    if( i ) continue;
                    i = this;
                    return true;
                    }
                return false;
                }

                auto
myInstanceNum   (SomeTasks* st)
                {
                for( auto i = 0; i < arraySize(instances_); i++ ){
                    if( st == instances_[i] ) return i;
                    }
                return arraySize(instances_); //not possible
                }

                bool
run             (SomeTasks* st)
                {
                if( not own_ ) return false;
                auto& uart{ *own_.pointer() };

                auto n = myInstanceNum(st); 
                Rgb color{ random.read<u8>(10,255), 
                            random.read<u8>(10,255), 
                            static_cast<u8>(n*30) };
                runCount_++;

                if( runCount_ bitand 1 ){
                    uart,
                        fg(WHITE), now(), fg(color), 
                        " [", Hex0x(8,reinterpret_cast<u32>(this)),
                        "][", dec_(2,n), "][", dec0(10,runCount_);
                    return false;
                    }
                uart, "] Hello World", endl, normal;

                own_.close();
                st->task_.interval = milliseconds( random.read<u16>(500,2000) );
                // st->task_.interval = milliseconds( 1000 );
                return true; //update interval
                }

public:

SomeTasks       (microseconds interval = 0ms)
                {
                task_.interval = interval;
                insert();
                }

                static bool
runAll          (Task_t&)
                {
                DebugPin dp;
                for( auto& st : instances_ ){
                    if( not st ) continue;
                    auto tp = now();
                    if( tp < st->task_.runat ) continue;
                    if( st->run(st) ) st->task_.runat = tp + st->task_.interval;
                    }
                return true;
                }

                }; //SomeTasks


//........................................................................................

                SomeTasks someTasks[16];

//........................................................................................

                static inline bool
showRandSeeds   (Task_t& task)
                { //run once, show 2 seed values use in Random (RandomGenLFSR16)
                Open device{ board.uart };                
                if( not device ) return false; //false = try again
                auto& uart{ *device.pointer() };

                if( task.interval != 0ms ){ //10s wait period done
                    task.interval = 0ms; //so task is deleted
                    device.close();
                    return true; //this task gets deleted
                    }

                //initial interval of 0ms, print data
                uart,
                    normal, endl,
                    "   initial seed values for random:", endl, endl,
                    "     seed0: ", Hex0x(8,random.seed0()), endl,
                    "     seed1: ", Hex0x(8,random.seed1()), endl, 
                    endl, dec;

                //hold uart for 5s so we can view
                task.interval = 5s;
                return true;
                }

//........................................................................................

                static inline void
timePrintu32    () //test Print's u32 conversion speed (view with logic analyzer)
                {  //will assume we are the only function used, no return
                Open device{ board.uart };                
                if( not device ) return;
                auto& uart{ *device.pointer() };

                uint32_t v = 0xFFFFFFFF;
                uart << bin; //set as needed
                while(1){
                    board.debugPin.on();
                    uart << v; //dec 307us, hex 298us, oct 417, bin 693us
                    board.debugPin.off();
                    delay( 20ms );
                    }
                }

//........................................................................................

                static bool
printDouble     (Task_t&)
                {
                Open device{ board.uart };                
                if( not device ) return false; //false = try again
                auto& uart{ *device.pointer() };

                static auto d{ 0.1 };
                DebugPin dp;

                uart,
                    fg(WHITE), now(), space, "double: ", setwf(10,' '), setprecision(6), d, endl;
                d = d*1.0001;

                device.close();
                return true;
                } //printRandom

//........................................................................................

                static bool
checkRstPin     (Task_t&)
                {                  
                //nRST currently configured as GPIO mode in options byte (0b10)
                //so just duplicating reset functionality via gpio pin 
                //(not a true reset pin)
                if( board.resetPin.isOn() ) CPU::Scb::swReset();
                return true;
                }

//........................................................................................


                int
main            ()
                {

                //boot up code = 22 (in System.hpp)
                Open device{ board.led };
                if( device ){
                    device.pointer()->infoCode( System::BOOT_CODE ); 
                    device.close();
                    }

                //systimer started at first use (infoCode uses systimer, so already started)
                //restart systimer after any clock speed changes do it recomputes its values
                //(if systimer depends on cpu speed, lptim based systimer have a fixed clock speed)
                //systimer.restart(); //if using systick and cpu speed is changed

                //lptim always uses the same clock, but can use restart() to change
                //its default interrupt priority

                //add tasks

                //show Random seed values at boot to see if they look ok
                //run now, run only once 
                //(tasl will hold onto the uart for 5s so we have a chance to read the output)
                // tasks.insert( showRandSeeds );
                
                tasks.insert( ledMorseCode, 80ms ); //interval is morse code DOT length
                tasks.insert( SomeTasks::runAll, 10ms ); //a separate set of tasks, 10ms interval
                tasks.insert( printTask, 500ms );
                tasks.insert( printRandom, 4000ms );
                tasks.insert( checkRstPin, 1000ms );
                // tasks.insert( printDouble, 100ms );

                //all tasks run in idle (not in an interrupt)
                //so all tasks are interruptable, but not from other tasks
                while(1){ 
                    auto nextRunAt = tasks.run(); //run returns time of next task
                    systimer.nextWakeup( nextRunAt ); //code in progess
                    while( nextRunAt > now() ){ //no need to run tasks until nextRunAt
                        //no need to check time until the next systick irq
                        //(other interrupts may be in use

                        //using wasIrq to check if a 'time' irq was run
                        //(so we can go back to sleep if was something like a uart irq)
                        while( not systimer.wasIrq() ) CPU::waitIrq();
                        }
                    }

                } //main



