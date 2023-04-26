//-Os -mcpu=cortex-m0plus -std=c++17 -Wall -Wextra -Wpedantic
//........................................................................................

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

                using Tasks_t = Tasks<Systick,16>;  //using Systick clock, max 16 tasks
                using Task_t = Tasks_t::Task;       //single task type

                static Systick systick;
                static Tasks_t tasks;               //list of Task_t's
                Board board;

                //alias names for easier use
                auto& now = systick.now;            //returns a chrono::time_point
                auto& delay = systick.delay;

                using namespace FMT;                //Print use
                using namespace ANSI;               //FMT::ANSI
                using namespace std::chrono;

//........................................................................................

                static bool
printTask       (Task_t& task)
                {
                static const auto id{ reinterpret_cast<u32>(printTask) };
                Uart* u = board.uart.take( id );
                if( not u ) return false;
                auto& uart = *u;
                
                auto ti = task.interval + milliseconds(20);
                if( ti > milliseconds(600) ) ti = milliseconds(400);
                task.interval = ti;
                static u16 n = 0;

                uart
                    << fg(255,165,0) << now().time_since_epoch()
                    << fg(50,90,150) << spacew(5) << n << space
                    << fg(GREEN) << Hex0xpad(4) << n << space
                    << fg(GREEN_YELLOW) << bin0bpad(16) << n 
                    << endl << FMT::reset << normal;

                n++;
                board.uart.release( id );
                return true;
                } //printTask

//........................................................................................

                static bool
printRandom     (Task_t&)
                {
                static const auto id{ reinterpret_cast<u32>(printRandom) };

                Uart* u = board.uart.take( id );
                if( not u ) return false;
                auto& uart = *u;

                auto r = random.read();

                uart
                    << fg(255,50,0) << now().time_since_epoch()
                    << fg(20,200,255) << " random " << bin0bpad(32) << r << space 
                    << fg(20,255,200) << Hex0xpad(8) << r 
                    << endl << FMT::reset << normal;

                board.uart.release( id );
                return true;
                } //printRandom

//........................................................................................

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

                if( binmask == 0 ){ 
                    if( nextc == 0 ) msgIdx = 0;
                    nextc = msg[msgIdx++];
                    auto m = MorseCode::lookup(nextc);
                    nextc = msg[msgIdx]; //look ahead
                    bin = m.bin;
                    binmask = 1<<((m.len bitand 31)-1);
                    isExtraSpacing = false;
                    }
                
                board.led.on( bin bitand binmask );                 
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
                return true;  

                }; //LedTask

//........................................................................................

                //a class with its own tasks, so we can duplicae a number of tasks
                //to test handling the uart exclusively for each task
////////////////
class 
SomeTasks       
////////////////
                {

                static inline SomeTasks* instances_[8];
                Task_t task_; //func unused
                u32 runCount_;

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
                static int idx{ -1 }; //-1 will printer header
                static const char* words[]{ "Hello", "World" };

                auto id{ reinterpret_cast<u32>(st) };
                Uart* u = board.uart.take( id );
                if( not u ) return false;
                auto& uart = *u;

                if( idx < 0 ){ 
                    auto n = myInstanceNum(st); 
                    Rgb color{ random.read<u8>(10,255), 
                               random.read<u8>(10,255), 
                               static_cast<u8>(n*30) };
                    auto dur = now().time_since_epoch();

                    uart
                        << fg(color) << dur << " [" << Hex0xpad(8) << reinterpret_cast<u32>(this) << dec << "]["
                        << n << "][" << decpad(10) << runCount_ << "] ";

                    idx++;
                    return false;
                    }

                if( idx < arraySize(words) ){ 
                    uart << words[idx++] << space; 
                    return false; 
                    }

                if( idx++ < (arraySize(words) + 3) ){
                    uart << spacew(3) << random.read<u8>() << space;
                    return false;
                    }

                uart << endl;
                idx = -1;
                uart << normal;
                board.uart.release( id );
                runCount_++;  
                st->task_.interval = milliseconds( random.read<u16>(500,2000) );
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

                //next interval set by each task to a random value so the default
                //interval of 0 is ok here for all 8 tasks
                SomeTasks someTasks[8];

//........................................................................................

                static bool
showRandSeeds   (Task_t& task)
                { //run once, show 2 seed values use in Random (RandomGenLFSR16)
                static const auto id{ reinterpret_cast<u32>(showRandSeeds) };
                Uart* u = board.uart.take( id );
                if( not u ) return false; //keep trying
                auto& uart = *u;
                if( task.interval != 0ms ){ //10s wait period done
                    task.interval = 0ms; //so task is deleted
                    board.uart.release( id ); //now release uart
                    return true; //this task gets deleted
                    }
                //intial interval of 0ms, print data
                uart << normal << endl 
                     << "   seed0: " << Hex0xpad(8) << random.seed0() << endl
                     << "   seed1: " << Hex0xpad(8) << random.seed1() << endl 
                     << endl << dec;
                //hold uart for 5s so we can view
                task.interval = 5s;
                return true;
                }

//........................................................................................

                void
timePrintu32    () //test Print's u32 conversion speed (view with logic analyzer)
                {  //will assume we are the only function used, no return
                static const auto id{ reinterpret_cast<u32>(testPrintu32) };
                Uart* u = board.uart.take( id ); //if we are the only function in use, should not fail
                if( not u ) return; 
                auto& uart = *u;
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

                int
main            ()
                {

                //boot up code = 33
                //any use of 0 is an invalid code, so will get code 99 (invalid code)
                board.led.infoCode( System::BOOT_CODE ); 

                //systick started at first use (infoCode uses systick, so already started)
                //restart systick after any clock speed changes
                //systick.restart();                

                //add tasks

                //show Random seed values at boot to see if they look ok
                //run now, run once (will hold onto the uart for 5s)
                tasks.insert( showRandSeeds ); 
                
                tasks.insert( ledMorseCode, 80ms ); //interval is morse DOT length
                tasks.insert( printTask, 500ms );
                //tasks.insert( SomeTasks::runAll, 1ms ); //a separate set of tasks
                tasks.insert( printRandom, 250ms );

                while(1){ 
                    //watch run periods via logic analyzer
                    board.debugPin.on();
                    auto nextRunAt = tasks.run();
                    board.debugPin.off();
                    while(1){
                        CPU::waitIrq();
                        if( not systick.wasIrq() ) continue;
                        if( nextRunAt <= now() ) break;
                        }
                    }

                } //main



