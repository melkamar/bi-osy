#include "common.h"
using namespace std;

//-------------------------------------------------------------------------------------------------
const TRequest   * dispatcher                              ( void )
 {
   vector <uint8_t> fuel;
   uint8_t finalProduct;
   
   if ( ! readFuel ( cin, finalProduct, fuel ) ) return NULL;
   
   
   TRequest * req = new TRequest;
   req -> m_FuelNr = fuel . size ();
   req -> m_Fuel = new uint8_t [ fuel . size () ];
   memcpy ( req -> m_Fuel, &*fuel . begin (), fuel .size () );
   req -> m_FinalProduct = finalProduct;
   return req;
 }
//-------------------------------------------------------------------------------------------------
void               engines                                 ( const TRequest  * req,
                                                             TSetup          * setup )
 {
   cout << "Product " << (int)req -> m_FinalProduct << " " << ( setup -> m_Root ? "Ok" : "N/A" ) << endl;
   if ( setup -> m_Root )
    {
      cout << "\tGenerator: " << setup -> m_Generator << "\n" 
           << "\tEnergy: " << setup -> m_Energy << "\n" 
           << "\tStart pos: " << setup -> m_StartPos << endl;
           
      setup -> m_Root -> PrintTree ( cout, setup -> m_StartPos, req -> m_FuelNr );
    }
   delete  setup -> m_Root;
   setup -> m_Root = NULL; 
   delete [] req -> m_Fuel;
   delete req; 
 }                                                             
//-------------------------------------------------------------------------------------------------
int                main                                    ( int               argc, 
                                                             char            * argv []  )
 {
   vector<TGenerator> generators;
   int threads;

   if ( argc != 2 
        || sscanf ( argv[1], "%d", &threads ) != 1
        || threads < 1 )
    {
      cout << "Usage: " << argv[0] << " <# threads>" << endl;
      return 1;
    }    
   
   while ( readGenerator ( cin, generators ) ) {}
   if ( ! cin . good () )
    {
      cout << "Invalid input." << endl;
      return 1;
    }
   optimizeEnergy ( threads, &*generators . begin (), generators . size (), dispatcher, engines );
   return 0;
 }
//-------------------------------------------------------------------------------------------------
