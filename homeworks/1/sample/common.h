#ifndef __common_h__2305723948572390456230478956203__
#define __common_h__2305723948572390456230478956203__

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <set>
#include <algorithm>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>


#define QUARKS_NR     6
 
struct TGenerator
 {
   unsigned int    m_Energy[QUARKS_NR][QUARKS_NR][QUARKS_NR];
 }; 

struct CReactNode
 {
   //----------------------------------------------------------------------------------------------
   /** Construct tree node/leaf
    */
                   CReactNode                              ( uint8_t           product, 
                                                             unsigned int      energy,
                                                             CReactNode      * l = NULL,
                                                             CReactNode      * r = NULL )
                    : m_L ( l ), m_R ( r ), m_Energy ( energy ), m_Product ( product ) 
    { }
   //----------------------------------------------------------------------------------------------
   /** Free the tree with all subtrees
    */
                  ~CReactNode                              ( void ) 
    {
      delete m_L; 
      delete m_R;
    }               
   //----------------------------------------------------------------------------------------------
   /** Debug - display the tree 
    * @param os  [in]  the stream to display the tree to
    * @param pos [in]  position in the input circular string
    * @param len [in[  the length of the input circular string
    * @param nodePrefix [in] the string prefix for the first node
    * @param subNodePrefix [in] the string prefix for the subnodes
    */ 
   int             PrintTree                               ( std::ostream    & os,
                                                             int               pos,
                                                             int               len,
                                                             const std::string & nodePrefix = "",
                                                             const std::string & subNodePrefix = "" )
    {
      if ( m_L )
       { // inner node
         os << nodePrefix << "<" << (int) m_Product << ", " << m_Energy << ">\n"; 
         int quarks = m_L -> PrintTree ( os, pos, len, subNodePrefix + " +-", subNodePrefix + " | " );
         return quarks + m_R -> PrintTree ( os, (pos + quarks) % len, len, subNodePrefix + " +-", subNodePrefix + "   " );
       }
      // leaf 
      os << nodePrefix << "<" << (int) m_Product << " @ " << pos << ">\n";
      return 1; 
    }                                                             
   //----------------------------------------------------------------------------------------------
   CReactNode    * m_L;
   CReactNode    * m_R;
   unsigned int    m_Energy;   // energy released (sum of the subtree)
   uint8_t         m_Product;  // quark produced from the subtree
 };

struct TRequest
 {
   uint8_t       * m_Fuel;         // quarks in the fuel ring
   int             m_FuelNr;       // length of the fuel ring
   uint8_t         m_FinalProduct; // quark required at the end of the fusion
 };

struct TSetup 
 {   
   CReactNode    * m_Root;         // fusion described in the form of a tree, NULL if the fusion cannot proceed
   unsigned int    m_Energy;       // energy released in the fusion
   int             m_Generator;    // generator # to use (0 based index)
   int             m_StartPos;     // starting index in the m_Fuel 
 };

/**
 * Sequential computation: optimize the fusion of a single fuel ring.
 * @param generators [in] an array of generators 
 * @param generatorsNr [in] # of generators available
 * @param request [in] input fuel ring description
 * @param setup [out] structure filled with the description of the optimal fusion
 */

void               optimizeEnergySeq                       ( const TGenerator* generators,
                                                             int               generatorsNr,
                                                             const TRequest  * request,
                                                             TSetup          * setup );


/**
 * Threaded computation: optimize the fusion of several fuel rings.
 * @param threads [in] numebr of worker threads to prepare
 * @param generators [in] an array of generators 
 * @param generatorsNr [in] # of generators available
 * @param dispatcher [in] function pointer - call to gen next input fuel ring
 * @param engines [in] function pointer - call to submit finished fuel ring
 */
void               optimizeEnergy                          ( int               threads,
                                                             const TGenerator* generators,
                                                             int               generatorsNr,
                                                             const TRequest *(* dispatcher)( void ),
                                                             void           (* engines ) ( const TRequest *, TSetup * ) );

/**
 * Helper function to read generator description from a stream (not present in the Progtest environment).
 * @param is [in] input stream to read data from
 * @param gen [out] generator structure to fill
 * @return true = success, false = failure
 */
bool               readGenerator                           ( std::istream    & is,
                                                             std::vector<TGenerator> & gen );

/**
 * Helper function to read fuel ring  description from a stream (not present in the Progtest environment).
 * @param is [in] input stream to read data from
 * @param final product [out] the quark required as a final product
 * @param final fuel [out] the list of quarks in the fuel ring
 * @return true = success, false = failure
 */
bool               readFuel                                ( std::istream    & is,
                                                             uint8_t         & finalProduct,
                                                             std::vector<uint8_t> & fuel );

#endif /* __common_h__2305723948572390456230478956203__ */
