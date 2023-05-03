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

                u8* const       buf_;
                unsigned const  size_;
                unsigned        inIdx_;
                unsigned        outIdx_;
                unsigned        count_{ 0 };    //count of unread data in the buffer
                unsigned        maxcount_{ 0 }; //keep track of max buffer used

public:

                template<unsigned N>
Buffer          (std::array<u8,N>& buf) 
                : buf_{ buf.data() }, size_{ N }, inIdx_{ 0 }, outIdx_{ 0 }
                {}

                bool
read            (u8& v)
                {
                InterruptLock lock;
                if( count_ == 0 ) return false;
                count_--;
                v = buf_[outIdx_++];
                if( outIdx_ >= size_ ) outIdx_ = 0;
                return true;
                }

                bool
write           (u8 v)
                {
                InterruptLock lock;
                if( count_ >= size_ ) return false;
                count_++;
                if( count_ > maxcount_ ) maxcount_ = count_;
                buf_[inIdx_++] = v;
                if( inIdx_ >= size_ ) inIdx_ = 0;
                return true;
                }

                auto
flush           () 
                {
                InterruptLock lock;
                count_ = maxcount_ = inIdx_ = outIdx_ = 0;
                }

                auto  
maxUsed         () { return maxcount_; }
                auto  
sizeUsed        () { return count_; }
                auto  
isFull          () { return sizeUsed() >= size_; }
                auto    
isEmpty         () { return sizeUsed() == 0; }
                auto  
sizeFree        () { return size_ - sizeUsed(); }
                auto  
sizeMax         () { return size_; }

                }; //Buffer

//........................................................................................
