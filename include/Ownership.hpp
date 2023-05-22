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

                u32 owner_; //unique id (like function address)
                u32 ownerPending_; //new owner, but waiting on device
                T& device_;
public:

Ownership       (T& device)
                : device_( device )
                {}

                T*
open            (u32 id)
                {
                if( id == owner_ ) return &device_;         //already owned by id
                InterruptLock lock;                         //protect writes to owner_, ownerPending_
                if( not ownerPending_ ) ownerPending_ = id; //become a pending owner if available
                if( owner_ or (id != ownerPending_) ) return nullptr; //someone else owns or is pending owner
                //device ownership available and id is ownerPending_
                if( not device_.isIdle() ) return nullptr;  //device idle check failed
                //ownerPending_ -> owner_
                owner_ = ownerPending_;
                ownerPending_ = 0;
                return &device_;
                }

                bool
close           (u32 id)
                {
                //no interrupt lock required- only owner(id) can change owner_ to 0
                //and 'open' has owner_ interrupt protected
                if( id != owner_ ) return false; //only owner can release
                owner_ = 0;
                return true;
                }

                }; //Ownership

//........................................................................................
