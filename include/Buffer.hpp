#pragma once

#include "Util.hpp"
#include MY_MCU_HEADER
#include <array>


//........................................................................................

                // the Buffer class only deals with a u8 buffer created outside this
                // class- there is no memory allocation done here

                // first create your buffer-
                // std::array<u8,128> buf;
                // then use the reference to creat a Buffer instance-
                // Buffer mybuffer{ buf };
                // and use as needed-
                // auto ret = myBuffer.write(1); //true=written, false=full

////////////////
class
Buffer            
////////////////
                {

                u8* const           mbuf;
                unsigned const      msize;
                unsigned            minIdx;
                unsigned            moutIdx;
                volatile unsigned   mcount{ 0 };    //count of unread data in the buffer
                unsigned            mmaxCount{ 0 }; //keep track of max buffer used

public:

                template<unsigned N>
Buffer          (std::array<u8,N>& buf) 
                : mbuf{ buf.data() }, msize{ N }, minIdx{ 0 }, moutIdx{ 0 }
                {}

                bool
read            (u8& v)
                {
                if( mcount == 0 ) return false;
                v = mbuf[moutIdx++];
                if( moutIdx >= msize ) moutIdx = 0;
                InterruptLock lock;
                mcount--;
                return true;
                }

                bool
write           (u8 v)
                {
                if( mcount >= msize ) return false;
                if( mcount > mmaxCount ) mmaxCount = mcount;
                mbuf[minIdx++] = v;
                if( minIdx >= msize ) minIdx = 0;
                InterruptLock lock;
                mcount++;
                return true;
                }

                auto
flush           () 
                {
                InterruptLock lock;
                mcount = mmaxCount = minIdx = moutIdx = 0;
                }

                auto  
maxUsed         () { return mmaxCount; }
                auto  
sizeUsed        () { return mcount; }
                auto  
isFull          () { return sizeUsed() >= msize; }
                auto    
isEmpty         () { return sizeUsed() == 0; }
                auto  
sizeFree        () { return msize - sizeUsed(); }
                auto  
sizeMax         () { return msize; }

                }; //Buffer

//........................................................................................
