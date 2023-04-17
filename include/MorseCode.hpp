#pragma once
#include "Util.hpp"
#include <cctype> //std::toupper

//........................................................................................

////////////////
class
MorseCode        
////////////////
                {

                //bin is binary value of .- pattern, len is binary bit length
                // 1=on, 0=off, all interchar spacing and trailing char spacing included
                using morsecode_t = struct { u32 bin,len; };

                //create morsecode_t struct from string, at compile time
                //so can use a string to represent morse code letter/number        
                // ".-" returns { 0b10111000, 8 } , ".---" returns { 0b1011101110111000, 16 }                
                template<int N> static constexpr morsecode_t 
str2morse       (const char(&str)[N])
                {
                morsecode_t m = { 0, 0 };
                for( auto c : str ){                    
                    if( c == '.' ){ m.bin <<= 2; m.bin or_eq 0b10; m.len += 2; }
                    if( c == '-' ){ m.bin <<= 4; m.bin or_eq 0b1110; m.len += 4; }
                    }
                m.bin <<= 2; m.len += 2;
                return m;
                }

                static inline const morsecode_t morseLetters_[26]{
                    str2morse(".-"  ), str2morse("-..."), str2morse("-.-."), str2morse("-.." ),  //A B C D
                    str2morse("."   ), str2morse("..-."), str2morse("--." ), str2morse("...."),  //E F G H
                    str2morse(".."  ), str2morse(".---"), str2morse("-.-" ), str2morse(".-.."),  //I J K L
                    str2morse("--"  ), str2morse("-."  ), str2morse("---" ), str2morse(".--."),  //M N O P
                    str2morse("--.-"), str2morse(".-." ), str2morse("..." ), str2morse("-"   ),  //Q R S T
                    str2morse("..-" ), str2morse("...-"), str2morse(".--" ), str2morse("-..-"),  //U V W X
                    str2morse("-.--"), str2morse("--.."), //Y Z
                    };
                static inline const morsecode_t morseNumbers_[10]{
                    str2morse("-----"), str2morse(".----"), str2morse("..---"), str2morse("...--"), //0 1 2 3
                    str2morse("....-"), str2morse("....."), str2morse("-...."), str2morse("--..."), //4 5 6 7
                    str2morse("---.."), str2morse("----."), //8 9
                    };
                static inline const morsecode_t morseError_{ 0,0 };

public:
                static const morsecode_t& 
lookup          (char c)
                {
                c = toupper(c);
                if(c >= 'A' and c <= 'Z') return morseLetters_[c - 'A'];
                if(c >= '0' and c <= '9') return morseNumbers_[c - '0'];
                return morseError_;
                }

                };

//........................................................................................
