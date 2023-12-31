#pragma once

#include "Util.hpp"
#include <string_view>
#include <limits>


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
                const char* ctbl{ uppercase_ ? charTableUC : charTableLC }; //use uppercase or lowercase char table
                auto u = v;                                     //make copy to use (v is const)
                auto w = just_ == internal ? width_ : 0;        //use width_ here if internal justify
                if( w ){
                    width_ = 0;                                 //if internal, clear width_ so not used when value printed
                    just_ = right;                              //reset just_ if internal (so no need to reset in other code)
                    }                                           //(don't see where it would ever be useful to leave as internal)

                //function to insert char to buf (idx decrementing)
                //buffer insert in reverse order (high bytes to low bytes)
                auto insert = [&](char c){ buf[--idx] = c; }; 

                do{ //do at least once (u can be 0)
                    //group % / so compiler makes only 1 call to divmod (?)
                    u32 rem = u % base_; u /= base_; 
                    insert( ctbl[rem] ); 
                    w--;
                    } while( u );
                //width_+internal justify could underflow buf, so limit width to allow for 2 more chars in buffer
                while( w-- > 0 and (idx > 2) ) insert( fill_ ); //internal fill

                switch( base_ ){
                    case bin: if(showbase_)         { insert('b'); insert('0'); }   break; //0b
                    case oct: if(showbase_ and v)   { insert('0'); }                break; //leading 0 unless v was 0
                    case hex: if(showbase_)         { insert('x'); insert('0'); }   break; //0x
                    //base_ is an enum so should not ever see default used, but just treat dec as default anyway so
                    //everything is handled
                    case dec:
                    default:  if(isNeg_) insert('-'); else if(pos_ and v) insert('+');  //- if negative, + if pos_ set and not 0
                    }
                return print( {&buf[idx], BUFSZ-idx} ); //call string_view version of print
                }


                //double, prints integer part and decimal part separately as integers
                Print&
print           (const double cd)
                {
                static const char* errs[]{ "nan", "inf", "ovf" };
                const char* perr{ nullptr };
                //check for nan/inf
                if( __builtin_isnan(cd) ) perr = errs[0];
                else if( __builtin_isinf_sign(cd) ) perr = errs[1];
                //we are limited by choice to using 32bit integers, so check for limits we can handle
                //raw float value of 0x4F7FFFFF is 4294967040.0, and the next value we will exceed a 32bit integer
                else if( cd > 4294967040.0 or (cd < -4294967040.0) ) perr = errs[2];
                if( perr ){
                    width_ = 0;
                    return print( perr ); 
                    }

                isNeg_ = cd < 0;                // set isNeg_ for first integer print
                double d { isNeg_ ? -cd : cd }; // [d]ouble, copy cd, any negative value to positive
                u32 di { static_cast<u32>(d) }; // [d]ouble_as_[i]nteger, integer part to print
                double dd { d - di };           // [d]ouble[d]ecimal part
                u32 i09 { 0 };                  // current integer digit of decimal part x10
                auto pre { precision_ };        // a copy to decrement 
                u32 ddi { 0 };                  // [d]ouble[d]ecimal_as[i]nteger, conversion of decimal part to integer

                //first print integer part, uses pos_/isNeg_/width_/just_
                //second decimal part (also imteger) needs pos_=noshowbase,just_=right,width_=precision_,fill_='0'
                //width_ and isNeg_ reset on each use, so no need to save/restore
                auto base_save{ base_ };    base_ = dec;
                auto justify_save{ just_ }; just_ = right;
                auto pos_save = pos_;
                auto save_fill = fill_;

                //handle decimal part of float
                while( pre-- ){                 //loop precision_ times
                    dd *= 10;                   //shift each significant decimal digit into integer
                    i09 = static_cast<u32>(dd); //integer 0-9
                    ddi = ddi*10 + i09;         //shift in new integer to integer-decimal print value
                    dd -= i09;                  //and remove the 0-9 integer (so only decimal remains)
                    }

                //apply rounding rules  
                //  round up/nearest if >0.5 remains
                //  round to nearest even value if 0.5 remains
                if( dd > 0.5 ){
                    if( precision_ ) ddi++;     //inc integer-decimal part
                    else di++ ;                 //inc integer part if no decimals in use
                    }
                else if( dd == 0.5 ){
                    if( precision_ ) ddi += i09 & 1;//make integer-decimal even based on last digit
                    else di += di & 1;           //same, but on di since no decimals in use
                    }

                print( di );                   //print integer part of float

                //if precision is not 0, print the decimal part of the float
                if( precision_ ){
                    print( '.' ); 
                    pos_ = noshowpos;           //no +
                    fill_ = '0';                //0 pad
                    width_ = precision_;        //width same as precision (width_ always resets to 0 on each use)
                    print( ddi );               //print decimal part as integer
                    }

                base_ = base_save;              //restore base_            
                just_ = justify_save;           //restore just_
                pos_ = pos_save;                //restore pos_
                fill_ = save_fill;              //restore fill_   
                return *this;
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

                template<typename T> PrintNull& //anything passed in
operator ,      (PrintNull& p, T){ return p; }

//........................................................................................

                //for Print-
                //use structs or enums to contain a value or values and make it unique
                // so values can be distinguished from values to print,
                // also allows more arguments to be passed into operator <<

                //everything else pass to print() and let the Print class sort it out
                template<typename T> Print&
                operator << (Print& p, T t) { return p.print( t ); }

                //also allow comma usage-
                // uart, "123", setwf(10,''), 456, endl;
                //simply 'convert' , usage to << usage
                //note- remember to add operator , to any additional Print types created
                //(see end of Systick class for example, where operator << is added and 
                // operator , is also added)
                template<typename T> Print&
                operator , (Print& p, T t) { return p << t; }

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

                // << hex0(8,0x1a)  -->> 0000001a
                template<typename T> struct Padhv { int n; T v; };
                template<typename T> inline Padhv<T> 
hex0            (int n, T v) { return {n,v}; }
                template<typename T> inline Print&
                operator<< (Print& p, Padhv<T> s) { return p << hex << nouppercase << noshowbase << internal << setwf(s.n,'0') << s.v; }

                // << Hex0(8,0x1a)  -->> 0000001A
                template<typename T> struct PadHv { int n; T v; };
                template<typename T> inline PadHv<T> 
Hex0            (int n, T v) { return {n,v}; }
                template<typename T> inline Print&
                operator<< (Print& p, PadHv<T> s) { return p << hex << uppercase << noshowbase << internal << setwf(s.n,'0') << s.v; }

                // << hex0x(8,0x1a)  -->> 0x0000001a
                template<typename T> struct Padh0xv { int n; T v; };
                template<typename T> inline Padh0xv<T> 
hex0x           (int n, T v) { return {n,v}; }
                template<typename T> inline Print&
                operator<< (Print& p, Padh0xv<T> s) { return p << hex << nouppercase << showbase << internal << setwf(s.n,'0') << s.v; }

                // << Hex0x(8,0x1a) -->> 0x0000001A
                template<typename T> struct PadH0xv { int n; T v; };
                template<typename T> inline PadH0xv<T> 
Hex0x           (int n, T v) { return {n,v}; }
                template<typename T> inline Print&
                operator<< (Print& p, PadH0xv<T> s) { return p << hex << uppercase << showbase << internal << setwf(s.n,'0') << s.v; }

                // << dec_(8,123)  -->>      123
                template<typename T> struct PadDv { int n; T v; };
                template<typename T> inline PadDv<T> 
dec_            (int n, T v) { return {n,v}; }
                template<typename T> inline Print&
                operator<< (Print& p, PadDv<T> s) { return p << dec << internal << setwf(s.n,' ') << s.v; }

                // << dec0(8,123)  -->> 00000123
                template<typename T> struct PadD0v { int n; T v; };
                template<typename T> inline PadD0v<T> 
dec0            (int n, T v) { return {n,v}; }
                template<typename T> inline Print&
                operator<< (Print& p, PadD0v<T> s) { return p << dec << internal << setwf(s.n,'0') << s.v; }

                // << bin_(8,123)  -->>  1111011
                template<typename T> struct PadBv { int n; T v; };
                template<typename T> inline PadBv<T> 
bin_            (int n, T v) { return {n,v}; }
                template<typename T> inline Print&
                operator<< (Print& p, PadBv<T> s) { return p << bin << noshowbase << internal << setwf(s.n,' ') << s.v; }

                // << bin0(8,123)  -->> 01111011
                template<typename T> struct PadB0v { int n; T v; };
                template<typename T> inline PadB0v<T> 
bin0            (int n, T v) { return {n,v}; }
                template<typename T> inline Print&
                operator<< (Print& p, PadB0v<T> s) { return p << bin << noshowbase << internal << setwf(s.n,'0') << s.v; }

                // << bin0b(8,123)  -->> 0b01111011
                template<typename T> struct PadB0bv { int n; T v; };
                template<typename T> inline PadB0bv<T> 
bin0b           (int n, T v) { return {n,v}; }
                template<typename T> inline Print&
                operator<< (Print& p, PadB0bv<T> s) { return p << bin << showbase << internal << setwf(s.n,'0') << s.v; }

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

                //ansi rgb colors (used by fg,bg)
                struct Rgb { u8 r; u8 g; u8 b; };

                //ansi fg(Rgb) or fg(r,g,b)
                struct Fg { Rgb rgb; };
                inline Fg 
fg              (Rgb s) { return {s}; }
                inline Fg 
fg              (u8 r, u8 g, u8 b) { return { Rgb{r,g,b} }; }
                inline Print& 
                operator<< (Print& p, Fg s) { return not ANSI_ON ? p : p << "\033[38;2;" << dec << s.rgb.r << ';' << s.rgb.g << ';' << s.rgb.b << 'm'; }

                //ansi bg(Rgb) or bg(r,g,b)
                struct Bg { Rgb rgb; };
                inline Bg 
bg              (Rgb s) { return {s}; }
                inline Bg 
bg              (u8 r, u8 g, u8 b) { return { Rgb{r,g,b} }; }
                inline Print&
                operator<< (Print& p, Bg s) { return not ANSI_ON ? p : p << "\033[48;2;" << dec << s.rgb.r << ';' << s.rgb.g << ';' << s.rgb.b << 'm'; }

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

                //ansi normal
                enum BOLD { 
bold            };
                inline Print&
                operator<< (Print& p, BOLD) { return not ANSI_ON ? p : p << "\033[1m"; }

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

                // can adjust colors by multiplication (at compile time)
                // no need to get more complicated with other operators as this is just
                // a simple way to brighten/darken an existing named color
                // WHITE * 0.5
                static constexpr Rgb 
operator *      (const Rgb& r, const double v)
                {
                return Rgb{ static_cast<u8>(r.r*v), static_cast<u8>(r.g*v), static_cast<u8>(r.b*v) };
                }

                } //namespace ANSI

                } //namespace FMT

//........................................................................................



// additional types can either be added here
// or can be added in source/header file where needed 
// (will then need to #include "Print.hpp")
//........................................................................................

                //any std::chrono::time_point
                // "  0dhh:mm:ss.ususus" d (day) is width 3, left space padded
                // "  1d00:01:07.825696"
                // "123d00:01:07.825696"
                template<typename T, typename D>
                inline FMT::Print&   
operator<<      (FMT::Print& p, std::chrono::time_point<T,D> tp) //{ return p << tp.time_since_epoch(); }
                {
                using namespace std::chrono;
                using namespace FMT;
                auto dur = tp.time_since_epoch();
                // < c++ 20, we have to make our own days
                using days = duration<u32, std::ratio<24*60*60>>;
                //need to explicitly cast when there is a loss in precision
                auto da = duration_cast<days        >(dur); dur -= da;
                auto hr = duration_cast<hours       >(dur); dur -= hr;
                auto mi = duration_cast<minutes     >(dur); dur -= mi;
                auto se = duration_cast<seconds     >(dur); dur -= se;
                auto us = microseconds(dur);
                return p
                        << dec_(4,da.count()) << "d"
                        << dec0(2,hr.count()) << ":"
                        << dec0(2,mi.count()) << ":"
                        << dec0(2,se.count()) << "."
                        << dec0(6,us.count());                
                }

                // , syntax 'convert' to <<
                template<typename T, typename D>
                inline FMT::Print&   
operator,       (FMT::Print& p, std::chrono::time_point<T,D> tp){ return p << tp; }

//........................................................................................