#ifndef __PROGTEST__
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <pthread.h>
#include <semaphore.h>
#include "common.h"
using namespace std;
#endif /* __PROGTEST__ */



class CWhateverNiceNameYouLike : public CCPU
{
  public:

    virtual uint32_t         GetMemLimit                   ( void ) const;
    virtual bool             SetMemLimit                   ( uint32_t          pages );
    virtual bool             NewProcess                    ( void            * processArg,
                                                             void           (* entryPoint) ( CCPU *, void * ),
                                                             bool              copyMem );
  protected:
   /*
    if copy-on-write is implemented:

    virtual bool             pageFaultHandler              ( uint32_t          address,
                                                             bool              write );
    */
};



void               MemMgr                                  ( void            * mem,
                                                             uint32_t          totalPages,
                                                             void            * processArg,
                                                             void           (* mainProcess) ( CCPU *, void * ))
{
  // todo
}
