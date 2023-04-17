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

                u8* const       buf_;
                const unsigned  siz_;
                unsigned        idxIn_;
                unsigned        idxOut_;
                unsigned        count_;     //count of unread data in the buffer
                unsigned        maxcount_;  //keep track of max buffer used

public:

                template<unsigned N>
Buffer          (std::array<u8,N>& buf) 
                : buf_(buf.data()), siz_(N) 
                {}

                bool
read            (u8& v)
                {
                InterruptLock lock;
                if( count_ == 0 ) return false;
                count_--;
                v = buf_[idxOut_++];
                if( idxOut_ >= siz_ ) idxOut_ = 0;
                return true;
                }

                bool
write           (u8 v)
                {
                InterruptLock lock;
                if( count_ >= siz_ ) return false;
                count_++;
                if( count_ > maxcount_ ) maxcount_ = count_;
                buf_[idxIn_++] = v;
                if( idxIn_ >= siz_ ) idxIn_ = 0;
                return true;
                }

                //reset the buffer
                auto
clear           () 
                {
                InterruptLock lock;
                count_ = 0;
                maxcount_ = 0;
                idxIn_ = 0;
                idxOut_ = 0;
                }

                auto  
maxUsed         () { InterruptLock lock; return maxcount_; }
                auto  
sizeUsed        () { InterruptLock lock; return count_; }
                auto  
isFull          () { return sizeUsed() >= siz_; }
                auto    
isEmpty         () { return sizeUsed() == 0; }
                auto  
sizeFree        () { return siz_ - sizeUsed(); }
                auto  
sizeMax         () { return siz_; }

                }; //Buffer

//........................................................................................
