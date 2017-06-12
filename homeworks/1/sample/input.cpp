#include "common.h"
using namespace std;
                
//-------------------------------------------------------------------------------------------------
static bool        readGenCell                             ( istream         & is,
                                                             unsigned int    * cell )
 {
   char c;
   
   if ( ! ( is >> c ) || c != '[' ) return false;
   
   for ( int i = 0; i < QUARKS_NR; i ++, cell ++ )
    {
      if ( ! ( is >> * cell ) )
       {
         is . clear ();
         if ( ! ( is >> c ) || c != 'x' ) return false; 
         *cell = 0;
       }
    }
   return is >> c && c == ']';   
 }                                                             
//-------------------------------------------------------------------------------------------------
static bool        readGenRow                              ( const string    & line,
                                                             unsigned int   (* row)[QUARKS_NR] )
 {
   istringstream is ( line );
   for ( int i = 0; i < QUARKS_NR; i ++ )
    if ( ! readGenCell ( is, row[i] ) )
     return false;
   return true;
 }                                                             
//-------------------------------------------------------------------------------------------------
bool               readGenerator                           ( istream         & is,
                                                             vector<TGenerator> & gen )
 {
   TGenerator g;
   string line;
    
   if ( ! getline ( is, line ) 
        || line . length () == 0 ) return false;
    
   if ( ! readGenRow ( line, g . m_Energy[0] ) )
    {
      is . setstate ( ios::failbit );
      return false;
    }
    
   for ( int i = 1; i < QUARKS_NR; i ++ )
    if ( ! getline ( is, line )
         || ! readGenRow ( line, g . m_Energy[i] ) )
     {
       is . setstate ( ios::failbit );
       return false;
     }

   if ( ! getline ( is, line ) 
        || line . length () != 0 ) return false;
     
   gen . push_back ( g );
   return true;
 }                                                             
//-------------------------------------------------------------------------------------------------
bool               readFuel                                ( istream         & is,
                                                             uint8_t         & finalProduct, 
                                                             vector<uint8_t> & fuel )
 {
   string line;
   unsigned int x;
   char dummy;
   
   if ( ! getline ( is, line ) ) return false;
   istringstream in ( line );

   if ( ! ( in >> x >> dummy ) 
        || x < 0 
        || x > QUARKS_NR 
        || dummy != ':' ) return false;

   finalProduct = x;
   while ( in >> x )
    {
      if ( x >= QUARKS_NR ) return false;
      fuel . push_back ( x );
    }
   return fuel . size () > 0 && in . eof ();
 }
//-------------------------------------------------------------------------------------------------
