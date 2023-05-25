#pragma once

#include "Util.hpp"
#include "Lptim.hpp"
#include <chrono>


//........................................................................................

////////////////
template
<typename Clock, int N> //Clock = a chrono compatible clock, N = task list array size                               
class
Tasks           
////////////////
                {
public:
                struct Task; //declare, since we need to refer to inside struct
                using taskFunc_t = bool(*)(Task&);
                using duration = typename Clock::duration;
                using time_point = typename Clock::time_point;

                struct Task {
                    time_point  runat;      //next run time
                    taskFunc_t  func;       //function to call
                    duration    interval;   //interval
                    };
private:
                static inline Task tasks_[N]{};

                static inline auto now = Clock::now;

                //if task function returns true-
                //  task runat time point will be updated if
                //  interval is >0, or task removed if interval 
                //  is <=0 and runat was not updated by the task
                //if task function returns false-
                //  the runat time will not be changed so it will
                //  run again at the next systick irq
                //  (but the task can change its own runat time)
                static auto
run             (Task& t, bool force = false)
                {
                if( not t.func ) return;
                auto tp = now(); //time_point
                if( not force and (t.runat > tp) ) return;
                if( not t.func( t ) ) return;
                if( t.interval.count() > 0 ) t.runat = tp + t.interval;
                else if( t.runat <= tp ) remove( t.func );
                //else runat was incremented by task
                }

public:

                static u32 
id              (Task& t) { return reinterpret_cast<u32>(t.func); }

                //run a single task (if in the task list)
                static void
run             (taskFunc_t f){ for(auto& t : tasks_) if(t.func == f) run(t, true); } //true = force

                //each task will have access to its own Task struct, so it
                //can change the runat, interval, and func members on its own
                //if interval is <=0, then task will need to update runat otherwise
                //the task will be removed
                //interval > 0 -> next run (runat) will be at interval+'now'
                //interval <= 0 and if runat <= 'now' -> task will be removed
                //interval <= 0 and runat > 'now' -> next run will be at runat
                static time_point
run             ()
                { 
                for( auto& t : tasks_ ) run(t); 
                //init a 'next' time far in future so we can find the soonest next task
                //(and if no tasks in next 24hours, will run in 24hours anyway)
                time_point next{ now() + std::chrono::hours(24) };
                for( auto& t : tasks_ ){
                    if( not t.func ) continue;
                    if( t.runat < next ) next = t.runat; //find soonest next runat time
                    }
                return next; //return next time we need to run
                }


                static auto
remove          (taskFunc_t f)
                {
                for( auto& t : tasks_ ){
                    if( t.func != f ) continue;
                    t.func = 0;
                    return;
                    }
                }

                //if interval is 0- task runs right away and
                //will only run 1 time unless task sets interval or runat
                //if interval is not 0 the task next runs at now()+interval
                static auto
insert          (taskFunc_t f, duration interval = std::chrono::milliseconds(0))
                {
                remove( f ); //so we do not get multiple instances of function in tasks_
                for( auto& t : tasks_ ){
                    if( t.func ) continue;
                    t.func = f;
                    t.interval = interval;
                    t.runat = now() + interval;
                    return true;
                    }
                return false;
                }

                //run at a time_point time ( not a 'now' based time )
                //will only run 1 time unless task sets interval or runat
                static auto
insert          (taskFunc_t f, time_point tp)
                {
                auto from_now = tp - now(); //convert to now based time
                return insert(f, from_now); //so can resuse the above insert
                }

                }; //Tasks

//........................................................................................
