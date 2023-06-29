#pragma once

#include "NiceTypes.hpp"
#include MY_MCU_HEADER //InterruptLock


//........................................................................................

////////////////
template
<typename T>    //device (device requires an isIdle() function- returns true when its 
                //ready to take on a new owner, or simply returns true
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

                //if no id passed in, assume is not a current owner
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

                // without this helper class-

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

                // own inside function only, or make Open static to retain ownership past the return
                //      Open device{ dev.uart };            //static Open device{ dev.uart };
                //      if( not device ) return;            //device not available
                //      auto& uart{ *device.pointer() };    //convert pointer to reference if wanted
                //      uart << "Hello" << endl;
                //      device.close();                     //give up ownership

template
<typename T>
class Open      {

                // using odesc_t = Ownership<T>::Descriptor;
                // using ottype_t = Ownership<T>;
                Ownership<T>::Descriptor desc_{0,0}; //u32 id; T* dev;
                Ownership<T>& own_;
public:

Open            (Ownership<T>& own) 
                : own_(own) 
                {
                //constructor may run when not wanted if class object is static
                //so do not attempt to own device here via dev()
                //may end up owning a device when not wanted
                }

                T*
pointer         ()
                { 
                if( not desc_.dev ) desc_ = own_.open();
                return desc_.dev; 
                }

                T* 
close           ()
                { 
                if( desc_.dev ) desc_ = own_.close(desc_);
                return desc_.dev;
                }

operator bool   (){ return pointer(); }

                };

//........................................................................................


