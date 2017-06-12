#include "common.h"
using namespace std;

int                main                                    ( int               argc, 
                                                             char            * argv []  )
 {
   vector<TGenerator> generators;
   
   while ( readGenerator ( cin, generators ) ) {}
   if ( ! cin . good () )
    {
      cout << "Invalid input." << endl;
      return 1;
    }

   vector<uint8_t> fuel; 
   uint8_t finalProduct;
   while ( readFuel ( cin, finalProduct, fuel ) )
    {
      TRequest req;
      TSetup   setup;
         
      req . m_Fuel = &*fuel . begin ();
      req . m_FuelNr = fuel . size ();
      req . m_FinalProduct = finalProduct;
         
      optimizeEnergySeq ( &*generators . begin (), generators . size (), &req, &setup );
      cout << "Product " << (int) req . m_FinalProduct << " " << ( setup . m_Root ? "Ok" : "N/A" ) << endl;
       if ( setup . m_Root )
        {
          cout << "\tGenerator: " << setup . m_Generator << "\n" 
               << "\tEnergy: " << setup . m_Energy << "\n" 
               << "\tStart pos: " << setup . m_StartPos << endl;
               
          setup . m_Root -> PrintTree ( cout, setup . m_StartPos, fuel . size () );
        }
      delete setup . m_Root;
      fuel . resize ( 0 );
    }
   return 0;
 }
