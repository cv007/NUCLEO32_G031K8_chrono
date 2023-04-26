#pragma once
#include "Util.hpp"
#include MY_MCU_HEADER
#include <array>

//........................................................................................

////////////////
class
Buffer            
////////////////
                {

                u8* const   buf_;
                u16 const   size_;
                u8*         inPtr_;
                u8*         outPtr_;
                unsigned    count_{ 0 };    //count of unread data in the buffer
                unsigned    maxcount_{ 0 }; //keep track of max buffer used

public:

                template<unsigned N>
Buffer          (std::array<u8,N>& buf) 
                : buf_{ buf.data() }, 
                  size_{ N },
                  inPtr_{ buf_ },
                  outPtr_{ buf_ }
                {}

                bool
read            (u8& v)
                {
                InterruptLock lock;
                if( count_ == 0 ) return false;
                count_--;
                v = *outPtr_++;
                if( outPtr_ >= &buf_[size_] ) outPtr_ = buf_;
                return true;
                }

                bool
write           (u8 v)
                {
                InterruptLock lock;
                if( count_ >= size_ ) return false;
                count_++;
                if( count_ > maxcount_ ) maxcount_ = count_;
                *inPtr_++ = v;
                if( inPtr_ >= &buf_[size_] ) inPtr_ = buf_;
                return true;
                }

                //reset the buffer
                auto
clear           () 
                {
                InterruptLock lock;
                count_ = 0;
                maxcount_ = 0;
                inPtr_ = buf_;
                outPtr_ = buf_;
                }

                auto  
maxUsed         () { InterruptLock lock; return maxcount_; }
                auto  
sizeUsed        () { InterruptLock lock; return count_; }
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
