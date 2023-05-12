#pragma once

#include "Util.hpp"
#include MY_MCU_HEADER
#include <array>


//........................................................................................

                // the BufferBytes class only deals with a u8 buffer created outside this
                // class- there is no memory allocation done here

                // first create your buffer-
                // std::array<u8,128> buf;
                // then use the reference to creat a Buffer instance-
                // Buffer mybuffer{ buf };
                // and use as needed-
                // auto ret = myBuffer.write(1); //true=written, false=full

////////////////
class
BufferBytes
////////////////
                {

                u8* const           buf_;
                u32 const           size_;

                u32                 wrIdx_{0};
                u32                 rdIdx_{0};
                CPU::Atom<u32>      atom_count_{0}; //count of unread data in the buffer
                u32                 maxCount_{0};   //keep track of max buffer used

public:

                template<unsigned N>
BufferBytes     (std::array<u8,N>& buf) 
                : buf_{ buf.data() }, 
                  size_{ N }
                {
                }

                bool
read            (u8& v)
                {
                if( atom_count_ == 0 ) return false;
                v = buf_[rdIdx_++];
                if( rdIdx_ >= size_ ) rdIdx_ = 0;
                atom_count_--;
                return true;
                }

                bool
write           (u8 v)
                {
                if( atom_count_ >= size_ ) return false;
                buf_[wrIdx_++] = v;
                if( wrIdx_ >= size_ ) wrIdx_ = 0;
                auto c = ++atom_count_;
                if( c > maxCount_ ) maxCount_ = c;
                return true;
                }

                auto
clear           () 
                {
                InterruptLock lock;
                //use atom_count_.value directly to bypass atomic since we take care of it here
                atom_count_.value = maxCount_ = wrIdx_ = rdIdx_ = 0; 
                }

                auto  
sizeUsedMax     () { return maxCount_; }
                auto  
sizeUsed        () { return atom_count_; }
                auto  
size            () { return size_; }
                auto  
sizeFree        () { return size_ - sizeUsed(); }
                auto  
isFull          () { return sizeUsed() >= size_; }
                auto    
isEmpty         () { return sizeUsed() == 0; }

                }; //Buffer

//........................................................................................
