#pragma once
#include "Util.hpp"
#include <string_view>
#include <limits>
#include <cstdlib> //std::div

//........................................................................................

////////////////
namespace
FMT             
////////////////
                {

                //enum values which will be used to choose a particular overloaded
                // print function, where the enum value is either the only value
                // needed (2 or more enums) or only needed to choose an overloaded
                // function but not otherwise used (single value enums)
                enum FMT_JUSTIFY    { right, left, internal };                  //if width padding, justify left, right, or internal to a number
                enum FMT_BASE       { bin = 2 , oct = 8, dec = 10, hex = 16 };  //number base
                enum FMT_ENDL       { endl };                                   //newline
                enum FMT_SHOWBASE   { noshowbase, showbase };                   //0x (hex), 0b (bin), 0 (oct)
                enum FMT_UPPERCASE  { nouppercase, uppercase };                 //hex uppercase or lowercase
                enum FMT_SHOWALPHA  { noshowalpha, showalpha };                 //bin 1|0, "true"|"false"
                enum FMT_SHOWPOS    { noshowpos, showpos };                     //+
                enum FMT_RESET      { reset };                                  //all options reset
                enum FMT_COUNTCLR   { countclr };                               //clear char out count
                enum FMT_ENDL2      { endl2 };                                  //endl x2


//........................................................................................

//////////////// FMT::
class
Print           
////////////////
                {

                //no true 64bit support, but if a 64bit value can be converted to a
                //u32/i32 without truncation it will be processed, otherwise ignored
                //for now

                //constant values
                enum { PRECISION_MAX = 9 }; //max float precision, limited to 9 by use of 32bit integers in calculations

public:

                //print a string_view (which also contains size info)
                Print&
print           (std::string_view sv)
                {
                int pad = width_ - sv.size();
                width_ = 0;                                     //always reset after use
                isNeg_ = false;                                 //clear for the other 2 functions (since they both will end up here)
                auto wr = [&]{ for( char c : sv ) write_( c ); }; //function to write the string
                if( pad <= 0 or just_ == left ) wr();           //print sv first
                if( pad > 0 ){                                  //need to deal with padding
                    while( pad-- > 0 ) write_( fill_ );         //print any needed padding
                    if( just_ == right ) wr();                  //and print sv if was not done already
                    }
                return *this;
                }
                
                //const char* without size info, convert to string_view
                //hopefully compiler produces the strlen at compile time (?) since
                //it knows the string literal size
                Print&
print           (const char* str){ return print( std::string_view{ str, __builtin_strlen(str)} ); }

                //unsigned int (32bits), prints as string (to above string_view print function)
                Print&
print           (const u32 v)
                {
                static constexpr char charTableUC[]{ "0123456789ABCDEF" };
                static constexpr char charTableLC[]{ "0123456789abcdef" };
                static constexpr u32 BUFSZ{ 32+2 };             //0bx...x-> 32+2 digits max (no 0 termination needed)
                char buf[BUFSZ];                                //will be used as a string_view
                u32 idx = BUFSZ;                                //start past end, so pre-decrement bufidx
                const char* ctbl{ uppercase_ ? charTableUC : charTableLC };
                auto u = v;                                     //make copy to use (v is const)
                auto w = just_ == internal ? width_ : 0;        //use width_ here if internal justify
                if( w ) width_ = 0;                             //if internal, clear width_ so not used when value printed

                auto insert = [&](char c){ buf[--idx] = c; };   //function to insert char to buf (idx decrementing)
                auto internalFill = [&](){ while( w-- > 0 ) insert( fill_ ); }; //internal fill

                switch( base_ ){ //bin,oct,hex use shifts, dec uses division
                    case bin: 
                        while( insert('0' + (u bitand 1)), w--, u >>= 1 ){} 
                        internalFill();
                        if(showbase_){ insert('b'); insert('0'); } //0b
                        break;
                    case oct:
                        while( insert(ctbl[u bitand 7]), w--, u >>= 3 ){} 
                        internalFill(); 
                        if(showbase_ and v) insert('0'); //leading 0 unless v was 0
                        break;
                    case hex: 
                        while( insert(ctbl[u bitand 15]), w--, u >>= 4 ){} 
                        internalFill();
                        if(showbase_){ insert('x'); insert('0'); } //0x
                        break;
                    default: //dec
                        do{
                            //group % / so compiler makes only 1 call to divmod
                            u32 rem = u % base_; 
                            u /= base_; 
                            insert( ctbl[rem] ); 
                            w--;
                            } while( u );
                        internalFill();
                        if(isNeg_) insert('-'); else if(pos_) insert('+'); //- if negative, + if pos_ set and not 0
                        break;
                    }
                return print( {&buf[idx], BUFSZ-idx} ); //call string_view version of print
                }

                //float, prints as string (to above string_view print function)
                Print&
print           (const float cf)
                {
                //check for nan/inf
                if( __builtin_isnan(cf) ) return print( uppercase_ ? "NAN" : "nan" );
                if( __builtin_isinf_sign(cf) ) return print( uppercase_ ? "INF" : "inf" );
                //we are limited by choice to using 32bit integers, so check for limits we can handle
                //raw float value of 0x4F7FFFFF is 4294967040.0, and the next value we will exceed a 32bit integer
                if( (cf > 4294967040.0) or (cf < -4294967040.0) ) return print( "ovf" );

                //values used to get fractional part into integer, based on precision 1-9
                static constexpr u32 mulTbl[PRECISION_MAX+1]
                    { 1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000 };
                static constexpr u32 BUFSZ{ 21 };           //10.10
                char str[BUFSZ];                            //fill top to bottom (high to low)
                u32 idx = BUFSZ;                            //top+1, since pre-decrementing
                auto insert = [&](char c){ str[--idx] = c; }; //pre-decrement idx so is always pointing to start

                auto pre = precision_;                      //copy
                auto f = cf;                                //copy
                if( f < 0.0 ){ isNeg_ = true; f = -f; }     //make positive if needed
                auto fi = static_cast<u32>(f);              //fi = integer part

                //deal with fractional part if precision is not 0
                if( pre ){
                    auto pw = mulTbl[pre];                  //table mul value to get desired fractional part moved up
                    f = (f - fi) * pw;                      //f = desired fractional part moved up to integer
                    auto i = static_cast<u32>(f);           //i = integer part of desired fraction
                    f -= i;                                 //f now contains the remaining/unused fraction

                    if( (f >= 0.5) and (++i >= pw) ){       //if need to round up- inc i, check for overflow (the table value)
                        i = 0;                              //i overflow, set i to 0
                        fi++;                               //propogate into (original) integer part
                        }
                    while( insert((i % 10) + '0'), i /= 10, --pre ){} //add each fractional digit
                    insert('.');                            //add dp
                    }

                //deal with integer part
                while( insert((fi % 10) + '0'), fi /= 10 ){}//add each integer digit
                if( isNeg_ ) insert('-');                   //if neg, now add '-'
                else if( pos_ ) insert('+');                //if positive and pos wanted, add '+'

                return print( {&str[idx], BUFSZ-idx} );     //call string_view version of print
                }

                //reset all options to default (except newline), clear count
                Print&
print           (FMT_RESET)
                {
                just_ = right;
                showbase_ = noshowbase;
                uppercase_ = nouppercase;
                alpha_ = noshowalpha;
                pos_ = noshowpos;
                isNeg_ = false;
                count_ = 0;
                width_ = 0;
                fill_ = ' ';
                base_ = dec;
                precision_ = PRECISION_MAX;
                return *this;
                }

                //all other printing overloaded print functions
                Print& print     (const i32 n)       { u32 nu = n; if( n < 0 ){ isNeg_ = true; nu = -nu; } return print( nu ); }
                Print& print     (const int n)       { u32 nu = n; if( n < 0 ){ isNeg_ = true; nu = -nu; } return print( nu ); }
                Print& print     (const u16 n)       { return print( static_cast<u32>(n) ); }
                Print& print     (const i16 n)       { return print( static_cast<i32>(n) ); }
                Print& print     (const u8 n)        { return print( static_cast<u32>(n) ); }
                Print& print     (const i8 n)        { return print( static_cast<i32>(n) ); }
                Print& print     (const char c)      { write_( c ); return *this; } //direct output, no conversion
                Print& print     (const bool tf)     { return alpha_ ? print( tf ? "true" : "false" ) : print( tf ? '1' : '0'); }

                //64bit support- only prints if can be expressed as a 32bit value
                Print& print     (const u64 n)       { return n <= std::numeric_limits<u32>::max() ? print( static_cast<u32>(n) ) : *this; }
                Print& print     (const i64 n)       { return n <= std::numeric_limits<i32>::max() and 
                                                              n >= std::numeric_limits<i32>::min() ? print( static_cast<i32>(n) ) : *this; }

                //non-printing overloaded print functions deduced via enum
                Print& print     (FMT_BASE e)        { base_ = e; return *this;}
                Print& print     (FMT_SHOWBASE e)    { showbase_ = e; return *this;}
                Print& print     (FMT_UPPERCASE e)   { uppercase_ = e; return *this;}
                Print& print     (FMT_SHOWALPHA e)   { alpha_ = e; return *this;}
                Print& print     (FMT_SHOWPOS e)     { pos_ = e; return *this;}
                Print& print     (FMT_JUSTIFY e)     { just_ = e; return *this;}
                Print& print     (FMT_ENDL)          { return print( nl_ ); }
                Print& print     (FMT_ENDL2)         { return print( nl_), print( nl_ ); }
                Print& print     (FMT_COUNTCLR)      { count_ = 0; return *this;}

                //non-printing functions needing a non-enum value, typically called by operator<< code
                Print& width     (int v)             { width_ = v; return *this; } //setw
                Print& fill      (int v)             { fill_ = v; return *this; } //setfill
                Print& precision (int v)             { precision_ = v > PRECISION_MAX ? PRECISION_MAX : v; return *this; } //setprecision

                //setup newline char(s) (up to 2)
                Print& newline   (const char* str)   { nl_[0] = str[0]; nl_[1] = str[1]; return *this; } //setnewline

                //return current char count (the only public function that does not return *this)
                int   count     ()                  { return count_; }

private:
                //a helper write so we can keep a count of chars written (successfully)
                //(if any write fails as defined by the parent class (returns false), the failure
                // is only reflected in the count and not used any further)
                void write_  (const char c)     { if( write(c) ) count_++; }

                virtual
                bool write  (const char) = 0; //parent class creates this function

                char            nl_[3]      { '\n', '\0', '\0' };
                FMT_JUSTIFY     just_       { left };
                FMT_SHOWBASE    showbase_   { noshowbase };
                FMT_UPPERCASE   uppercase_  { nouppercase };
                FMT_SHOWALPHA   alpha_      { noshowalpha };
                FMT_SHOWPOS     pos_        { noshowpos };
                bool            isNeg_      { false };  //inform other functions a number was originally negative
                int             width_      { 0 };
                char            fill_       { ' ' };
                int             count_      { 0 };
                u8              precision_  { PRECISION_MAX };
                FMT_BASE        base_       { dec };

                }; //FMT::Print

//........................................................................................

                //FMT::PrintNull class (no  output, optimizes away all uses)
                //inherit this instead of Print to make all uses vanish
                //class Uart : FMT::Print {} //prints
                //class Uart : FMT::PrintNull {} //any use of << will be optimized away
                //this allows using debug output with an easy way to enable/disable

//////////////// FMT::
class
PrintNull       {}; //just a class type, nothing inside
////////////////

                // operator <<

                //for PrintNull- do nothing, and
                // return the same PrintNull reference passed in
                // (nothing done, no code produced when optimized)
                template<typename T> PrintNull& //anything passed in
operator <<     (PrintNull& p, T){ return p; }

//........................................................................................

                //for Print-
                //use structs or enums to contain a value or values and make it unique
                // so values can be distinguished from values to print,
                // also allows more arguments to be passed into operator <<

                //everything else pass to print() and let the Print class sort it out
                template<typename T> Print&
                operator << (Print& p, T t) { return p.print( t ); }

                //the functions which require a value will return
                //a struct with a value or values which then gets used by <<
                //all functions called in Print are public, so no need to 'friend' these

                // << setw(10)
                struct Setw { int n; };
                inline Setw
setw            (int n) { return {n}; }
                inline Print&
                operator<< (Print& p, Setw s) { return p.width(s.n); }

                // << setfill(' ')
                struct Setf { char c; };
                inline Setf
setfill         (char c) { return {c}; }
                inline Print&
                operator<< (Print& p, Setf s) { return p.fill(s.c); }

                // << setprecision(4)
                struct Setp { int n; };
                inline Setp
setprecision    (int n) { return {n}; }
                inline Print&
                operator<< (Print& p, Setp s) { return p.precision(s.n); }

                // << cdup('=', 40)
                struct Setdup { char c; int n; };
                inline Setdup
cdup            (char c, int n) { return {c,n}; }
                inline Print&
                operator<< (Print& p, Setdup s) { return p << setw(s.n) << setfill(s.c) << ""; }

                // << setwf(8,'0')
                struct Setwf { int n; char c; };
                inline Setwf
setwf           (int n, char c) { return {n,c}; }
                inline Print&
                operator<< (Print& p, Setwf s) { p.width(s.n); return p.fill(s.c); }

                // << hexpad(8) << 0x1a  -->> 0000001a
                struct Padh { int n; };
                inline Padh
hexpad          (int n) { return {n}; }
                inline Print&
                operator<< (Print& p, Padh s) { return p << hex << nouppercase << noshowbase << internal << setwf(s.n,'0'); }

                // << Hexpad(8) << 0x1a  -->> 0000001A
                struct PadH { int n; };
                inline PadH
Hexpad          (int n) { return {n}; }
                inline Print&
                operator<< (Print& p, PadH s) { return p << hex << uppercase << noshowbase << internal << setwf(s.n,'0'); }

                // << hex0xpad(8) << 0x1a  -->> 0x0000001a
                struct Padh0x { int n; };
                inline Padh0x
hex0xpad        (int n) { return {n}; }
                inline Print&
                operator<< (Print& p, Padh0x s) { return p << hex << nouppercase << showbase << internal << setwf(s.n,'0'); }

                // << Hex0xpad(8) << 0x1a  -->> 0x0000001A
                struct PadH0x { int n; };
                inline PadH0x
Hex0xpad        (int n) { return {n}; }
                inline Print&
                operator<< (Print& p, PadH0x s) { return p << hex << uppercase << showbase << internal << setwf(s.n,'0'); }

                // << decpad(8) << 123  -->> 00000123
                struct PadD { int n; };
                inline PadD
decpad          (int n) { return {n}; }
                inline Print&
                operator<< (Print& p, PadD s) { return p << dec << internal << setwf(s.n,'0'); }

                // << binpad(8) << 123  -->> 01111011
                struct PadB { int n; };
                inline PadB 
binpad          (int n) { return {n}; }
                inline Print&
                operator<< (Print& p, PadB s) { return p << bin << noshowbase << internal << setwf(s.n,'0'); }

                // << bin0bpad(8) << 123  -->> 0b01111011
                struct PadB0b { int n; };
                inline PadB0b 
bin0bpad        (int n) { return {n}; }
                inline Print&
                operator<< (Print& p, PadB0b s) { return p << bin << showbase << internal << setwf(s.n,'0'); }

                // << space -->> ' '
                enum Space { 
space           };
                inline Print&
                operator<< (Print& p, Space) { return p << ' '; }

                // << spaces(5) -->> "     "
                struct Spaces { int n; };
                inline Spaces
spaces          (int n){ return {n}; }
                inline Print&
                operator<< (Print& p, Spaces s) { return p << cdup(s.n,' '); }

                // << right << spacew(5) << "12"  -->> "   12"
                struct Spacew { int n; };
                inline Spacew
spacew          (int n){ return {n}; }
                inline Print&
                operator<< (Print& p, Spacew s) { return p << setwf(s.n,' '); }

                // << tab -->> '\t'
                enum Tab { 
tab             };
                inline Print&
                operator<< (Print& p, Tab) { return p << '\t'; }

                // << tabs(2) -->> "\t\t"
                struct Tabs { int n; };
                inline Tabs
tabs            (int n){ return {n}; }
                inline Print&
                operator<< (Print& p, Tabs s) { return p << cdup(s.n,'\t'); }

//////////////// FMT::
namespace 
ANSI  
////////////////
                {

                static constexpr auto ANSI_ON{ true };

                //the functions which require a value will return
                //a struct with a value or values which then gets used by <<
                //all functions called in Print are public, so no need to 'friend' these
                //enums also used to 'find' the specific operator<< when no values are needed

                //ansi rgb (used by fg,bg - used without will not produce correct ansi codes)
                struct Rgb { u8 r; u8 g; u8 b; };
                inline Rgb
rgb             (u8 r, u8 g, u8 b){ return {r,g,b}; }
                inline Print&
                operator<< (Print& p, Rgb s) { return not ANSI_ON ? p : p << dec << s.r << ';' << s.g << ';' << s.b << 'm'; }
                
                //ansi fg(Rgb) or fg(r,g,b)
                struct Fg { Rgb s; };
                inline Fg 
fg              (Rgb s) { return {s}; }
                inline Fg 
fg              (u8 r, u8 g, u8 b) { return { Rgb{r,g,b} }; }
                inline Print& 
                operator<< (Print& p, Fg s) { return not ANSI_ON ? p : p << "\033[38;2;" << s.s; }

                //ansi bg(Rgb) or bg(r,g,b)
                struct Bg { Rgb s; };
                inline Bg 
bg              (Rgb s) { return {s}; }
                inline Bg 
bg              (u8 r, u8 g, u8 b) { return { Rgb{r,g,b} }; }
                inline Print&
                operator<< (Print& p, Bg s) { return not ANSI_ON ? p : p << "\033[48;2;" << s.s; }

                //ansi cls
                enum CLS { 
cls             };
                inline Print&
                operator<< (Print& p, CLS) { return not ANSI_ON ? p : p << "\033[2J"; }

                //ansi home
                enum HOME { 
home            };
                inline Print&
                operator<< (Print& p, HOME) { return not ANSI_ON ? p : p << "\033[1;1H"; }

                //ansi italic
                enum ITALIC { 
italic          };
                inline Print&
                operator<< (Print& p, ITALIC) { return not ANSI_ON ? p : p << "\033[3m"; }

                //ansi normal
                enum NORMAL { 
normal          };
                inline Print&
                operator<< (Print& p, NORMAL) { return not ANSI_ON ? p : p << "\033[0m"; }

                //ansi underline
                enum UNDERLINE { 
underline       };
                inline Print&
                operator<< (Print& p, UNDERLINE) { return not ANSI_ON ? p : p << "\033[4m"; }

                //ansi reset
                enum RESET { 
reset           };
                inline Print&
                operator<< (Print& p, RESET) { return not ANSI_ON ? p : p << cls << home << normal; }


                //svg colors
                static constexpr Rgb ALICE_BLUE               { 240,248,255 };
                static constexpr Rgb ANTIQUE_WHITE            { 250,235,215 };
                static constexpr Rgb AQUA                     { 0,255,255 };
                static constexpr Rgb AQUAMARINE               { 127,255,212 };
                static constexpr Rgb AZURE                    { 240,255,255 };
                static constexpr Rgb BEIGE                    { 245,245,220 };
                static constexpr Rgb BISQUE                   { 255,228,196 };
                static constexpr Rgb BLACK                    { 0,0,0 };
                static constexpr Rgb BLANCHED_ALMOND          { 255,235,205 };
                static constexpr Rgb BLUE                     { 0,0,255 };
                static constexpr Rgb BLUE_VIOLET              { 138,43,226 };
                static constexpr Rgb BROWN                    { 165,42,42 };
                static constexpr Rgb BURLY_WOOD               { 222,184,135 };
                static constexpr Rgb CADET_BLUE               { 95,158,160 };
                static constexpr Rgb CHARTREUSE               { 127,255,0 };
                static constexpr Rgb CHOCOLATE                { 210,105,30 };
                static constexpr Rgb CORAL                    { 255,127,80 };
                static constexpr Rgb CORNFLOWER_BLUE          { 100,149,237 };
                static constexpr Rgb CORNSILK                 { 255,248,220 };
                static constexpr Rgb CRIMSON                  { 220,20,60 };
                static constexpr Rgb CYAN                     { 0,255,255 };
                static constexpr Rgb DARK_BLUE                { 0,0,139 };
                static constexpr Rgb DARK_CYAN                { 0,139,139 };
                static constexpr Rgb DARK_GOLDEN_ROD          { 184,134,11 };
                static constexpr Rgb DARK_GRAY                { 169,169,169 };
                static constexpr Rgb DARK_GREEN               { 0,100,0 };
                static constexpr Rgb DARK_KHAKI               { 189,183,107 };
                static constexpr Rgb DARK_MAGENTA             { 139,0,139 };
                static constexpr Rgb DARK_OLIVE_GREEN         { 85,107,47 };
                static constexpr Rgb DARK_ORANGE              { 255,140,0 };
                static constexpr Rgb DARK_ORCHID              { 153,50,204 };
                static constexpr Rgb DARK_RED                 { 139,0,0 };
                static constexpr Rgb DARK_SALMON              { 233,150,122 };
                static constexpr Rgb DARK_SEA_GREEN           { 143,188,143 };
                static constexpr Rgb DARK_SLATE_BLUE          { 72,61,139 };
                static constexpr Rgb DARK_SLATE_GRAY          { 47,79,79 };
                static constexpr Rgb DARK_TURQUOISE           { 0,206,209 };
                static constexpr Rgb DARK_VIOLET              { 148,0,211 };
                static constexpr Rgb DEEP_PINK                { 255,20,147 };
                static constexpr Rgb DEEP_SKY_BLUE            { 0,191,255 };
                static constexpr Rgb DIM_GRAY                 { 105,105,105 };
                static constexpr Rgb DODGER_BLUE              { 30,144,255 };
                static constexpr Rgb FIRE_BRICK               { 178,34,34 };
                static constexpr Rgb FLORAL_WHITE             { 255,250,240 };
                static constexpr Rgb FOREST_GREEN             { 34,139,34 };
                static constexpr Rgb FUCHSIA                  { 255,0,255 };
                static constexpr Rgb GAINSBORO                { 220,220,220 };
                static constexpr Rgb GHOST_WHITE              { 248,248,255 };
                static constexpr Rgb GOLD                     { 255,215,0 };
                static constexpr Rgb GOLDEN_ROD               { 218,165,32 };
                static constexpr Rgb GRAY                     { 128,128,128 };
                static constexpr Rgb GREEN                    { 0,128,0 };
                static constexpr Rgb GREEN_YELLOW             { 173,255,47 };
                static constexpr Rgb HONEY_DEW                { 240,255,240 };
                static constexpr Rgb HOT_PINK                 { 255,105,180 };
                static constexpr Rgb INDIAN_RED               { 205,92,92 };
                static constexpr Rgb INDIGO                   { 75,0,130 };
                static constexpr Rgb IVORY                    { 255,255,240 };
                static constexpr Rgb KHAKI                    { 240,230,140 };
                static constexpr Rgb LAVENDER                 { 230,230,250 };
                static constexpr Rgb LAVENDER_BLUSH           { 255,240,245 };
                static constexpr Rgb LAWN_GREEN               { 124,252,0 };
                static constexpr Rgb LEMON_CHIFFON            { 255,250,205 };
                static constexpr Rgb LIGHT_BLUE               { 173,216,230 };
                static constexpr Rgb LIGHT_CORAL              { 240,128,128 };
                static constexpr Rgb LIGHT_CYAN               { 224,255,255 };
                static constexpr Rgb LIGHT_GOLDENROD_YELLOW   { 250,250,210 };
                static constexpr Rgb LIGHT_GRAY               { 211,211,211 };
                static constexpr Rgb LIGHT_GREEN              { 144,238,144 };
                static constexpr Rgb LIGHT_PINK               { 255,182,193 };
                static constexpr Rgb LIGHT_SALMON             { 255,160,122 };
                static constexpr Rgb LIGHT_SEA_GREEN          { 32,178,170 };
                static constexpr Rgb LIGHT_SKY_BLUE           { 135,206,250 };
                static constexpr Rgb LIGHT_SLATE_GRAY         { 119,136,153 };
                static constexpr Rgb LIGHT_STEEL_BLUE         { 176,196,222 };
                static constexpr Rgb LIGHT_YELLOW             { 255,255,224 };
                static constexpr Rgb LIME                     { 0,255,0 };
                static constexpr Rgb LIME_GREEN               { 50,205,50 };
                static constexpr Rgb LINEN                    { 250,240,230 };
                static constexpr Rgb MAGENTA                  { 255,0,255 };
                static constexpr Rgb MAROON                   { 128,0,0 };
                static constexpr Rgb MEDIUM_AQUAMARINE        { 102,205,170 };
                static constexpr Rgb MEDIUM_BLUE              { 0,0,205 };
                static constexpr Rgb MEDIUM_ORCHID            { 186,85,211 };
                static constexpr Rgb MEDIUM_PURPLE            { 147,112,219 };
                static constexpr Rgb MEDIUM_SEA_GREEN         { 60,179,113 };
                static constexpr Rgb MEDIUM_SLATE_BLUE        { 123,104,238 };
                static constexpr Rgb MEDIUM_SPRING_GREEN      { 0,250,154 };
                static constexpr Rgb MEDIUM_TURQUOISE         { 72,209,204 };
                static constexpr Rgb MEDIUM_VIOLET_RED        { 199,21,133 };
                static constexpr Rgb MIDNIGHT_BLUE            { 25,25,112 };
                static constexpr Rgb MINT_CREAM               { 245,255,250 };
                static constexpr Rgb MISTY_ROSE               { 255,228,225 };
                static constexpr Rgb MOCCASIN                 { 255,228,181 };
                static constexpr Rgb NAVAJO_WHITE             { 255,222,173 };
                static constexpr Rgb NAVY                     { 0,0,128 };
                static constexpr Rgb OLD_LACE                 { 253,245,230 };
                static constexpr Rgb OLIVE                    { 128,128,0 };
                static constexpr Rgb OLIVE_DRAB               { 107,142,35 };
                static constexpr Rgb ORANGE                   { 255,165,0 };
                static constexpr Rgb ORANGE_RED               { 255,69,0 };
                static constexpr Rgb ORCHID                   { 218,112,214 };
                static constexpr Rgb PALE_GOLDENROD           { 238,232,170 };
                static constexpr Rgb PALE_GREEN               { 152,251,152 };
                static constexpr Rgb PALE_TURQUOISE           { 175,238,238 };
                static constexpr Rgb PALE_VIOLET_RED          { 219,112,147 };
                static constexpr Rgb PAPAYA_WHIP              { 255,239,213 };
                static constexpr Rgb PEACH_PUFF               { 255,218,185 };
                static constexpr Rgb PERU                     { 205,133,63 };
                static constexpr Rgb PINK                     { 255,192,203 };
                static constexpr Rgb PLUM                     { 221,160,221 };
                static constexpr Rgb POWDER_BLUE              { 176,224,230 };
                static constexpr Rgb PURPLE                   { 128,0,128 };
                static constexpr Rgb REBECCA_PURPLE           { 102,51,153 };
                static constexpr Rgb RED                      { 255,0,0 };
                static constexpr Rgb ROSY_BROWN               { 188,143,143 };
                static constexpr Rgb ROYAL_BLUE               { 65,105,225 };
                static constexpr Rgb SADDLE_BROWN             { 139,69,19 };
                static constexpr Rgb SALMON                   { 250,128,114 };
                static constexpr Rgb SANDY_BROWN              { 244,164,96 };
                static constexpr Rgb SEA_GREEN                { 46,139,87 };
                static constexpr Rgb SEA_SHELL                { 255,245,238 };
                static constexpr Rgb SIENNA                   { 160,82,45 };
                static constexpr Rgb SILVER                   { 192,192,192 };
                static constexpr Rgb SKY_BLUE                 { 135,206,235 };
                static constexpr Rgb SLATE_BLUE               { 106,90,205 };
                static constexpr Rgb SLATE_GRAY               { 112,128,144 };
                static constexpr Rgb SNOW                     { 255,250,250 };
                static constexpr Rgb SPRING_GREEN             { 0,255,127 };
                static constexpr Rgb STEEL_BLUE               { 70,130,180 };
                static constexpr Rgb TAN                      { 210,180,140 };
                static constexpr Rgb TEAL                     { 0,128,128 };
                static constexpr Rgb THISTLE                  { 216,191,216 };
                static constexpr Rgb TOMATO                   { 255,99,71 };
                static constexpr Rgb TURQUOISE                { 64,224,208 };
                static constexpr Rgb VIOLET                   { 238,130,238 };
                static constexpr Rgb WHEAT                    { 245,222,179 };
                static constexpr Rgb WHITE                    { 255,255,255 };
                static constexpr Rgb WHITE_SMOKE              { 245,245,245 };
                static constexpr Rgb YELLOW                   { 255,255,0 };
                static constexpr Rgb YELLOW_GREEN             { 154,205,50 };
                } //namespace ANSI

                } //namespace FMT

//........................................................................................