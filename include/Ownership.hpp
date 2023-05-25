#pragma once

#include "NiceTypes.hpp"
#include MY_MCU_HEADER //InterruptLock


//........................................................................................

////////////////
template
<typename T>    //device (device requires an isIdle() function- returns true when its 
                //ready to take on a new owner, or simply returns true if not necessary
class           
Ownership
////////////////
                {           

                u32 owner_{1}; //unique id, excluding 0
                T& dev_;
                bool isOwned_{ false };               
public:

                struct Descriptor {
                    u32 id; //Descriptor id (value of owner_)
                    T* dev; //device pointer
                    };

Ownership       (T& dev)
                : dev_( dev )
                {}

                //if no id passed in, this is a first time call
                //if pass in id, caller has been here before and is simply calling
                //this function because it previously
                Descriptor
open            (u32 id = 0)
                {
                if( id == owner_ ){     //caller already owns
                    return {owner_,&dev_};
                    }
                if( isOwned_ ) return {0,0};
                // device ownership available
                if( not dev_.isIdle() ) return {0,0};  //device idle check failed
                isOwned_ = true;
                return {owner_,&dev_};
                }

                Descriptor
close           (Descriptor od)
                {
                //no interrupt lock required- only id can change owner_ to 0
                //and 'take' has owner_ interrupt protected
                if( od.id != owner_ ) return od;  //only owner can release
                if( ++owner_ == 0 ) owner_ = 1; //inc owner_ (to invalidate id), owner_cannot be 0
                isOwned_ = false; 
                return {0,0};
                }

                }; //Ownership

//........................................................................................

                // without this class-

                // own inside a function only
                //      auto od{ dev.uart.open() };
                //      if( not od.dev ) return;
                //      auto& uart{ *od.dev };   //convert pointer to reference if wanted
                //      uart << "Hello" << endl;
                //      dev.uart.close(od);     //needs to happen before return

                // own beyond the function return
                //      using od_t = decltype(dev.uart)::Descriptor;
                //      static od_t od; //static so can keep the id,dev
                //      if( not od.dev ){
                //          od = dev.uart.open();
                //          if( not od.dev ) return;
                //      }
                //      auto& uart{ *od.dev };  //convert pointer to reference if wanted
                //      uart << "Hello" << endl;
                //      od = dev.uart.close(od);  //can be done anytime (like next call into function)

                // using this class, using within the function or past the return are both the same,
                // with only the addition of making Own static-

                // own inside function only
                //      Own own{ dev.uart };
                //      if( not own.open() ) return;//device not available
                //      auto& uart{ *own.dev() };   //convert pointer to reference if wanted
                //      uart << "Hello" << endl;
                //      own.close();                //give up ownership

                // own beyond the function return
                //      static Own own{ dev.uart };
                //      if( not own.open() ) return;//device not available
                //      auto& uart{ *own.dev() };   //convert pointer to reference if wanted
                //      uart << "Hello" << endl;
                //      own.close();                //give up ownership

template
<typename T>
class Own       {

                using od_t = Ownership<T>::Descriptor;
                using ot_t = Ownership<T>;
                od_t desc_{0,0};
                ot_t& own_;
public:

Own             (ot_t& own) 
                : own_(own) 
                {
                //do not open() here as we may not realize when constructor runs
                //and may end up owning a device when not wanted
                }

                T*
dev             () const { return desc_.dev; }

                T* 
open            ()
                { 
                if( not desc_.dev ) desc_ = own_.open(); 
                return dev(); 
                }

                T* 
close           ()
                { 
                if( desc_.dev ) desc_ = own_.close(desc_);
                return dev();
                }

                };

//........................................................................................
