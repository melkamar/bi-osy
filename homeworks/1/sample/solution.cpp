#ifndef __PROGTEST__
#include "common.h"
using namespace std;
#endif /* __PROGTEST__ */


void               optimizeEnergySeq                       ( const TGenerator* generators,
                                                             int               generatorsNr,
                                                             const TRequest  * request,
                                                             TSetup          * setup )
 {
   // todo
 }                                                              

void               optimizeEnergy                          ( int               threads,
                                                             const TGenerator* generators,
                                                             int               generatorsNr,
                                                             const TRequest *(* dispatcher)( void ),
                                                             void           (* engines ) ( const TRequest *, TSetup * ) )
 {
   // todo
 }
