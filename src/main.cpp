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
                Board dev;

                //alias names for easier use
                auto& now = systick.now;            //returns a chrono::time_point
                auto& delay = systick.delay;

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
                    DebugPin(){ if constexpr( DEBUG_PIN ) dev.debugPin.on(); }
                    ~DebugPin(){ if constexpr( DEBUG_PIN ) dev.debugPin.off(); }
                };

                //create a u32 id from a type T (such as a function)
                template<typename T> 
                static constexpr auto
makeID          (T t){ return reinterpret_cast<u32>(t); }

//........................................................................................

                static bool
printTask       (Task_t& task)
                {
                auto uartp{ dev.uart.open(task.id) };
                if( not uartp ) return false; //false=try again
                auto& uart{ *uartp };

                DebugPin dp;                
                auto ti = task.interval + milliseconds(20);
                if( ti > milliseconds(600) ) ti = milliseconds(400);
                task.interval = ti; //set new interval
                static u16 n = 0;

                // , style (for << style just replace all , with <<
                uart,
                    fg(WHITE*0.4), now().time_since_epoch(), space,
                    fg(WHITE*0.4), "[", Hex0x(8,task.id), "] ",
                    fg(50,90,150), dec_(5,n), space,
                    fg(GREEN*1.5), Hex0x(4,n), endl;

                n++;
                dev.uart.close( task.id );
                return true;
                } //printTask

//........................................................................................

                static bool
printRandom     (Task_t& task)
                {
                auto uartp{ dev.uart.open(task.id) };
                if( not uartp ) return false; //false=try again
                auto& uart{ *uartp };

                DebugPin dp;
                auto r = random.read();

                uart,
                    fg(WHITE), now().time_since_epoch(), space,
                    fg(WHITE), "[", Hex0x(8,task.id), "] ",
                    fg(20,200,255), "random ",
                    fg(20,255,200), Hex0x(8,r), space,
                    fg(50,75,200), "uart buffer max used: ", uart.bufferUsedMax(),
                        endl, FMT::reset, normal;

                dev.uart.close( task.id );
                return true;
                } //printRandom

//........................................................................................

                //interval used when adding to tasks is the DOT period
                //all periods are based on the DOT period
                static bool
ledMorseCode    (Task_t& task)
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

                auto ledp{ dev.led.open(task.id) };
                if( not ledp ) return false;
                auto& led{ *ledp };

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
                auto uartp{ dev.uart.open(id) };
                if( not uartp ) return false; //false=try again
                auto& uart{ *uartp };

                if( idx < 0 ){ 
                    auto n = myInstanceNum(st); 
                    Rgb color{ random.read<u8>(10,255), 
                               random.read<u8>(10,255), 
                               static_cast<u8>(n*30) };
                    auto dur = now().time_since_epoch();

                    uart,
                        fg(WHITE), dur, fg(color), 
                        " [", Hex0x(8,reinterpret_cast<u32>(this)), dec,
                        "][", n, "][", dec0(10,runCount_), "] ";

                    idx++;
                    return false;
                    }

                if( idx < arraySize(words) ){ 
                    uart << words[idx++] << space; 
                    return false; 
                    }

                if( idx++ < (arraySize(words) + 3) ){
                    uart << dec0(3,random.read<u8>()) << space;
                    return false;
                    }

                uart << endl;
                idx = -1;
                uart << normal;
                dev.uart.close( id );
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

                //next interval set by each task to a random value so the default
                //interval of 0 is ok here for all 8 tasks
                SomeTasks someTasks[8];

//........................................................................................

                static inline bool
showRandSeeds   (Task_t& task)
                { //run once, show 2 seed values use in Random (RandomGenLFSR16)
                auto uartp{ dev.uart.open(task.id) };
                if( not uartp ) return false; //false=try again
                auto& uart{ *uartp };

                if( task.interval != 0ms ){ //10s wait period done
                    task.interval = 0ms; //so task is deleted
                    dev.uart.close( task.id ); //now release uart
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

                static void
timePrintu32    () //test Print's u32 conversion speed (view with logic analyzer)
                {  //will assume we are the only function used, no return
                auto uartp{ dev.uart.open(makeID(timePrintu32)) };
                if( not uartp ){ while(1){} }
                auto& uart{ *uartp };

                uint32_t v = 0xFFFFFFFF;
                uart << bin; //set as needed
                while(1){
                    dev.debugPin.on();
                    uart << v; //dec 307us, hex 298us, oct 417, bin 693us
                    dev.debugPin.off();
                    delay( 20ms );
                    }
                }

//........................................................................................

                int
main            ()
                {
                //boot up code = 22 (in System.hpp)
                //any use of 0 is an invalid code, so will get code 99 (invalid code)   
                const auto id{ makeID(main) };
                auto ledp = dev.led.open( id ); //should not fail, we are the first to use
                if( ledp ){ 
                    auto& led{ *ledp };
                    led.infoCode( System::BOOT_CODE ); 
                    dev.led.close( id );
                    }

                //systick started at first use (infoCode uses systick, so already started)
                //restart systick after any clock speed changes
                //systick.restart();                

                //add tasks

                //show Random seed values at boot to see if they look ok
                //run now, run once (will hold onto the uart for 5s)
                //tasks.insert( showRandSeeds ); 
                
                tasks.insert( ledMorseCode, 80ms ); //interval is morse DOT length
                tasks.insert( printTask, 500ms );
                tasks.insert( SomeTasks::runAll, 10ms ); //a separate set of tasks, 10ms interval
                tasks.insert( printRandom, 250ms );

                while(1){ 
                    auto nextRunAt = tasks.run();
                    while( nextRunAt > now() ){ 
                        //continue loop only if we wake from systick irq
                        while( not systick.wasIrq() ) CPU::waitIrq();
                        }
                    }

                } //main


