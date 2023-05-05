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

                u8* const           buf_;
                u32 const           size_;

                u32                 wrIdx_;
                u32                 rdIdx_;
                CPU::AtomRW<u32>    count_;    //count of unread data in the buffer
                u32                 maxCount_; //keep track of max buffer used

public:

                template<unsigned N>
Buffer          (std::array<u8,N>& buf) 
                : buf_{ buf.data() }, size_{ N }
                {
                flush();
                }

                bool
read            (u8& v)
                {
                if( count_ == 0 ) return false;
                v = buf_[rdIdx_++];
                if( rdIdx_ >= size_ ) rdIdx_ = 0;
                count_--;
                return true;
                }

                bool
write           (u8 v)
                {
                if( count_ >= size_ ) return false;
                if( count_ > maxCount_ ) maxCount_ = count_;
                buf_[wrIdx_++] = v;
                if( wrIdx_ >= size_ ) wrIdx_ = 0;
                count_++;
                return true;
                }

                auto
flush           () 
                {
                InterruptLock lock;
                //use count_.value directly to bypass atomic since we take care of it here
                count_.value = maxCount_ = wrIdx_ = rdIdx_ = 0; 
                }

                auto  
maxUsed         () { return maxCount_; }
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
