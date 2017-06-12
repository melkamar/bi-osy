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
#include <iostream>
#include <stdint.h>
#include <iomanip>

bool writeOutput = false;

class SladkeCPUpripraveneSpecialneProLadu : public CCPU {
public:
    SladkeCPUpripraveneSpecialneProLadu(uint8_t * memStart, uint32_t pageTableRoot);
    ~SladkeCPUpripraveneSpecialneProLadu();
    virtual uint32_t GetMemLimit(void) const;
    virtual bool SetMemLimit(uint32_t pages);
    virtual bool NewProcess(void * processArg,
            void (* entryPoint) (CCPU *, void *),
            bool copyMem);
    static const uint32_t OFFSET_MASK = (1 << OFFSET_BITS) - 1;


    bool incMemLimit(uint32_t pages);
    bool decMemLimit(uint32_t pages);

    uint32_t* getL1PageEntryRealHWAddress(uint32_t entryNr);
    uint32_t* getL2PageEntryRealHWAddress(uint32_t L1Index, uint32_t L2Index);

    bool increasePagersPosition(uint32_t& l1Index, uint32_t& l2Index);
    bool decreasePagersPosition(uint32_t& l1Index, uint32_t& l2Index);

    uint32_t countRequiredPagesForPageTables(uint32_t);
    void countLastAllocatedPagePosition(uint32_t& l1Index, uint32_t& l2Index);

    bool CopyMemorySpace(SladkeCPUpripraveneSpecialneProLadu* destCPU);
    bool CopyOnWriteDuplicate(SladkeCPUpripraveneSpecialneProLadu* destCPU);

    void printTables();

    uint32_t m_pagesAllocated = 0; /// pages holding data allocated
    uint32_t m_dirPagesAllocated = 1; /// pages containing pagetables allocated

    virtual bool pageFaultHandler(uint32_t address, bool write);
};

/* -------------------------------------------------------------------------  */
uint32_t * pagesUsed;
uint32_t pagesTotal;
uint8_t * pageReferenceCounters;

int runningThreads;
pthread_cond_t runningThreadsCond;
pthread_mutex_t threadMutex;
pthread_mutex_t pageMutex;

pthread_mutex_t globalMutex;

/* -------------------------------------------------------------------------  */


void printBinary(uint32_t num) {
    for (int i = 31; i >= 0; i--) {
        cout << ((num & (1 << i)) >> i);
    }
}

void setPageUsed(uint32_t pageNr, bool used) {
    int index = pageNr / 32;
    int bit = pageNr % 32;

    uint32_t newVal;
    if (used == false) {
        newVal = 0xFFFFFFFF;
        newVal -= 1 << bit;
        pagesUsed[index] &= newVal;
    } else {
        newVal = 0;
        newVal += 1 << bit;
        pagesUsed[index] |= newVal;
    }
}

bool getPageUsed(uint32_t pageNr) {
    int index = pageNr / 32;
    int bit = pageNr % 32;

    return (pagesUsed[index] & (1 << bit)) != 0;
}

/**
 * Sets given page entry to writable or read-only
 * @param pageEntryHWAddr
 * @param writable
 */
void setPageEntryWritable(uint32_t *pageEntryHWAddr, bool writable) {
    uint32_t writableBitMask = 0x2;
    if (writable) {
        uint32_t pageEntry = (*pageEntryHWAddr);
        pageEntry |= writableBitMask;
        (*pageEntryHWAddr) = pageEntry;
    } else {
        uint32_t pageEntry = (*pageEntryHWAddr);
        pageEntry &= (~writableBitMask);
        (*pageEntryHWAddr) = pageEntry;
    }
}

/**
 * Find first free page number.
 * @return Returns index of the free page.
 * Returns 0 if no page could be found.
 */
uint32_t getFreePageNumber() {
    for (uint32_t i = 0; i < pagesTotal; i++) {
        if (getPageUsed(i) == false) {
            return i;
        }
    }
    return 0;
}

/**
 * Calculates simulated physical address, i.e.
 * - 0, 4096, 8192, 12288...
 * @param pageNr
 * @return Simulated physical address
 */
uint32_t getPagePhysicalAddress(uint32_t pageNr) {
    return pageNr * CCPU::PAGE_SIZE;
}

/**
 * Gets page number from simulated physical address,
 * 1578 -> 0, 4100->1, 8190->1...
 * @param physAddr
 * @return Returns page number
 */
uint32_t getPageNrFromPhysicalAddress(uint32_t physAddr) {
    return physAddr / CCPU::PAGE_SIZE;
}

/**
 * Gets simulated physical address of a page entry from a page
 * @param pageNr 
 * @param pageEntryNr Number of the entry to get (0..1023)
 * @return 
 */
uint32_t getPageEntryPhysicalAddress(uint32_t pageNr, uint32_t pageEntryNr) {
    //    assert(pageEntryNr < CCPU::PAGE_DIR_ENTRIES);
    return pageNr * CCPU::PAGE_SIZE + pageEntryNr * 4;
}

/**
 * Gets real hw address of a page
 * @param pageNr
 * @param mem
 * @return 
 */
uint32_t* getPageRealHWAddress(uint32_t pageNr, void * mem) {
    return (uint32_t*) ((uint8_t*) mem + pageNr * CCPU::PAGE_SIZE);
}

/**
 * Gets real hw address of a page entry
 * @param pageNr
 * @param pageEntryNr
 * @param mem
 * @return 
 */
uint32_t* getPageEntryRealHWAddress(uint32_t pageNr, uint32_t pageEntryNr, void * mem) {
    //    assert(pageEntryNr < CCPU::PAGE_DIR_ENTRIES);
    uint32_t* page0 = getPageRealHWAddress(pageNr, mem);
    return page0 + pageEntryNr;
}

bool isPageShared(uint32_t* pageEntryHWAddr) {
    return ((*pageEntryHWAddr) & 0x3) == 1;
}

/**
 * Creates a new entry
 * @param physAddress
 * @param dirty
 * @param referenced
 * @param write
 * @param present
 * @return 
 */
uint32_t createEntry(uint32_t physAddress, bool dirty, bool referenced,
        bool write, bool present) {

    uint32_t res = 0;
    res = physAddress & CCPU::ADDR_MASK;

    if (dirty) res += 1 << 6;
    if (referenced) res += 1 << 5;
    res += 1 << 2;
    if (write) res += 1 << 1;
    if (present) res += 1;

    return res;
}

void setPageRefCount(uint32_t pageNr, uint8_t newCount) {
    pageReferenceCounters[pageNr] = newCount;
}

uint8_t getPageRefCount(uint32_t pageNr) {
    return pageReferenceCounters[pageNr];
}

/**
 * Gets a real hw address of a L1 pagetable entry
 * @param entryNr Number of the entry (0..1023)
 * @return Address of the entry.
 */
uint32_t* SladkeCPUpripraveneSpecialneProLadu::getL1PageEntryRealHWAddress(uint32_t entryNr) {
    //    assert(entryNr < CCPU::PAGE_DIR_ENTRIES);
    uint32_t* l1PageAddr = (uint32_t*) (m_MemStart + m_PageTableRoot);
    uint32_t* l1EntryAddr = l1PageAddr + entryNr;
    return l1EntryAddr;
}

uint32_t* SladkeCPUpripraveneSpecialneProLadu::getL2PageEntryRealHWAddress(uint32_t L1Index, uint32_t L2Index) {
    uint32_t pageEntryL1 = *getL1PageEntryRealHWAddress(L1Index);
    uint32_t physAddrL2 = pageEntryL1 & CCPU::ADDR_MASK;

    uint32_t* l2PageAddr = (uint32_t*) (m_MemStart + physAddrL2);
    uint32_t* l2EntryAddr = l2PageAddr + L2Index;

    return l2EntryAddr;
}

/*******************************************************************************
 *              __  __                __  __            
 *             |  \/  | ___ _ __ ___ |  \/  | __ _ _ __ 
 *             | |\/| |/ _ \ '_ ` _ \| |\/| |/ _` | '__|
 *             | |  | |  __/ | | | | | |  | | (_| | |   
 *             |_|  |_|\___|_| |_| |_|_|  |_|\__, |_|   
 *                                           |___/      
 ******************************************************************************/

/**
 * 
 * Entry point. Should create CCPU instance and run mainProcess
 * 
 * @param mem Address of the provided memory
 * @param totalPages Number of pages provided
 * @param processArg Arguments for the new process
 * @param mainProcess Pointer to the init function.
 */
void MemMgr(void * mem,
        uint32_t totalPages,
        void * processArg,
        void (* mainProcess) (CCPU *, void *)) {

    /* ----------- Synchronization initialization --------------------------- */
    pthread_cond_init(&runningThreadsCond, NULL);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&threadMutex, &attr);
    pthread_mutex_init(&pageMutex, &attr);
    pthread_mutex_init(&globalMutex, NULL);
    pthread_mutexattr_destroy(&attr);
    /* ---------------------------------------------------------------------- */


    cout << endl << endl << endl;
    cout << "************************************************************************************" << endl;
    cout << "               MEMMGR              " << endl;


    pagesUsed = (uint32_t *) mem;
    memset(mem, 0, totalPages * CCPU::PAGE_SIZE);
    pagesTotal = totalPages;

    uint32_t pagesNeeded = totalPages / (CCPU::PAGE_SIZE * 8);

    if (totalPages % (CCPU::PAGE_SIZE * 8) > 0) pagesNeeded++;

    /* ------- Counted references ------------------------------------------- */
    pageReferenceCounters = (uint8_t*) (pagesUsed + pagesNeeded);
    uint32_t refCounterPagesNeeded = totalPages / CCPU::PAGE_SIZE;
    if (totalPages % CCPU::PAGE_SIZE > 0) refCounterPagesNeeded++;

    cout << "Total pages allocated: " << totalPages << endl;
    cout << "  Pages needed for used/unused: " << pagesNeeded << endl;
    cout << "  Pages needed for reference counting: " << refCounterPagesNeeded << endl;
    cout << "  Starting at " << hex << mem << dec << endl;

    for (uint32_t i = 0; i < pagesNeeded + refCounterPagesNeeded; i++) {
        setPageUsed(i, true);
    }

    runningThreads = 0;

    /*
     * Creating init process.
     * Get a free page, set as used.
     * Create L1 pager in it.
     */
    uint32_t pg1 = getFreePageNumber();
    setPageUsed(pg1, true);

    uint32_t l1PagerPhys = getPagePhysicalAddress(pg1);
    SladkeCPUpripraveneSpecialneProLadu * initCPU = new SladkeCPUpripraveneSpecialneProLadu((uint8_t*) mem, l1PagerPhys);

    mainProcess(initCPU, processArg);


    pthread_mutex_lock(&threadMutex);
    while (runningThreads != 0) {
        pthread_cond_wait(&runningThreadsCond, &threadMutex);
    }
    pthread_mutex_unlock(&threadMutex);


    delete initCPU;
    return;
}

SladkeCPUpripraveneSpecialneProLadu::SladkeCPUpripraveneSpecialneProLadu(uint8_t * memStart, uint32_t pageTableRoot) : CCPU(memStart, pageTableRoot) {
}

SladkeCPUpripraveneSpecialneProLadu::~SladkeCPUpripraveneSpecialneProLadu() {

    SetMemLimit(0);

    // Need to delete L1 paging
    pthread_mutex_lock(&pageMutex);

    uint32_t l1PageNr = getPageNrFromPhysicalAddress(m_PageTableRoot);
    uint32_t * l1PageHWAddr = getPageRealHWAddress(l1PageNr, m_MemStart);
    memset((void*) l1PageHWAddr, 0, 4096);
    setPageUsed(l1PageNr, false);

    pthread_mutex_unlock(&pageMutex);
}

uint32_t SladkeCPUpripraveneSpecialneProLadu::GetMemLimit(void) const {
    return m_pagesAllocated;
}

bool SladkeCPUpripraveneSpecialneProLadu::SetMemLimit(uint32_t pages) {
    if (pages < 0) {
        return false;
    }

    uint32_t requiredPages = pages - m_pagesAllocated;

    if (requiredPages == 0) { // No change
        return true;
    } else if (pages > m_pagesAllocated) { // Increasing memory space
        pthread_mutex_lock(&pageMutex);
        bool res = incMemLimit(pages);
        pthread_mutex_unlock(&pageMutex);
        return res;
    } else /* requiredPages < 0 */ { // Decreasing memory space
        pthread_mutex_lock(&pageMutex);
        bool res = decMemLimit(pages);
        pthread_mutex_unlock(&pageMutex);
        return res;
    }
}

/**
 * Counts required number of page tables (L1+L2) to hold data
 * @param dataPages Count of data pages needed to accomodate
 * @return 
 */
uint32_t SladkeCPUpripraveneSpecialneProLadu::countRequiredPagesForPageTables(uint32_t dataPages) {
    uint32_t res = 1; // Main page table (L1)

    if (dataPages == 0) {
        return res;
    }

    res += (dataPages / 1024); // returns ceil of (dataPages/1024) - yes, really /1025.
    if (dataPages % 1024 > 0) res++;
    return res;
}

/**
 * Counts pagetable position of the last pagetable currently allocated
 * @param l1Index
 * @param l2Index
 */
void SladkeCPUpripraveneSpecialneProLadu::countLastAllocatedPagePosition(uint32_t& l1Index, uint32_t& l2Index) {
    l1Index = (m_pagesAllocated - 1) / 1024;
    l2Index = (m_pagesAllocated - 1) % 1024;
}

bool SladkeCPUpripraveneSpecialneProLadu::increasePagersPosition(uint32_t& l1Index, uint32_t& l2Index) {
    if (l2Index == 1023) { // If l2 is 1023, need to skip to a new l2
        l1Index++;
        l2Index = 0;
    } else { // l2 is in [0..1022], can safely increment
        l2Index++;
    }

    return true;
}

bool SladkeCPUpripraveneSpecialneProLadu::decreasePagersPosition(uint32_t& l1Index, uint32_t& l2Index) {
    if (l2Index == 0) { // If l2 is 0, need to decrease level1 and start from full l2
        if (l1Index == 0) {
            return false;
        } else {
            l1Index--;
            l2Index = 1023;
        }
    } else { // l2Index > 0  ->  just decrease this index, keep l1
        l2Index--;
    }
    return true;
}

bool SladkeCPUpripraveneSpecialneProLadu::incMemLimit(uint32_t pages) {
    uint32_t pagesForDataToCreate = pages - m_pagesAllocated; // Number of pages needed yet to createfor data
    uint32_t pagesForPageTablesToCreate = countRequiredPagesForPageTables(pages) - m_dirPagesAllocated;
    uint32_t totalPagesToCreate = pagesForDataToCreate + pagesForPageTablesToCreate;


    uint32_t* availablePages = new uint32_t[totalPagesToCreate]; // Array to hold found empty pages
    for (uint32_t i = 0; i < totalPagesToCreate; i++) {
        uint32_t pgNum = getFreePageNumber();

        if (pgNum == 0) { // if no empty page found, roll back
            for (uint32_t j = 0; j < i; j++) {
                setPageUsed(availablePages[j], false);
            }
        }
        availablePages[i] = pgNum;
        setPageUsed(pgNum, true);
    }

    // emptying all new pages
    for (uint32_t i = 0; i < totalPagesToCreate; i++) {
        uint32_t* pageHWAddr = getPageRealHWAddress(availablePages[i], m_MemStart);
        memset((void*) pageHWAddr, 0, 4096);
    }

    // Get position of last allocated page
    uint32_t currentPageNr = 0;
    uint32_t l1Index;
    uint32_t l2Index;
    countLastAllocatedPagePosition(l1Index, l2Index);
    for (uint32_t i = 0; i < pagesForDataToCreate; i++) {

        if (m_pagesAllocated == 0) {
            l1Index = 0;
            l2Index = 0;
        } else {
            increasePagersPosition(l1Index, l2Index);
        }

        if (l2Index == 0) { // Need a new pagetable
            uint32_t* l1EntryHWAddr = getL1PageEntryRealHWAddress(l1Index);

            uint32_t l2PageNr = availablePages[currentPageNr];
            uint32_t l2PagePhysAddr = getPagePhysicalAddress(l2PageNr);

            uint32_t l1PageTableEntry = createEntry(l2PagePhysAddr, 0, 0, 1, 1);
            (*l1EntryHWAddr) = l1PageTableEntry;

            m_dirPagesAllocated++;
            currentPageNr++;
        }


        // Create a new page table, already set to contain 0.
        uint32_t* l2EntryHWAddr = getL2PageEntryRealHWAddress(l1Index, l2Index);

        uint32_t dataPageNr = availablePages[currentPageNr];
        uint32_t dataPagePhysAddr = getPagePhysicalAddress(dataPageNr);

        uint32_t l2PageTableEntry = createEntry(dataPagePhysAddr, 0, 0, 1, 1);
        (*l2EntryHWAddr) = l2PageTableEntry;

        m_pagesAllocated++;
        currentPageNr++;
    }

    delete [] availablePages;
    return true;
}

/**
 * @TODO reference counting - need to check if page is not referenced when deleting.
 * @param pages
 * @return 
 */
bool SladkeCPUpripraveneSpecialneProLadu::decMemLimit(uint32_t pages) {
    if (pages < 0 || m_pagesAllocated == 0) {
        return false;
    }

    uint32_t dataPagesToDelete = m_pagesAllocated - pages;
    uint32_t l1Index;
    uint32_t l2Index;
    countLastAllocatedPagePosition(l1Index, l2Index);

    for (uint32_t i = 0; i < dataPagesToDelete; i++) {

        uint32_t* l2EntryHWAddr = getL2PageEntryRealHWAddress(l1Index, l2Index);

        bool pageShared = isPageShared(l2EntryHWAddr);

        if (!pageShared) {
            // Deleting data page
            uint32_t dataPagePhysAddr = (*l2EntryHWAddr) & CCPU::ADDR_MASK;
            uint32_t dataPageNr = getPageNrFromPhysicalAddress(dataPagePhysAddr);
            uint32_t* dataPageHWAddr = getPageRealHWAddress(dataPageNr, m_MemStart);
            memset((void*) dataPageHWAddr, 0, 4096);

            setPageUsed(dataPageNr, false);
            (*l2EntryHWAddr) = 0;
        } else {
        }
        m_pagesAllocated--;

        // If this is the last page in L2 pagetable, delete L2 pagetable and its entry in L1
        if (l2Index == 0) {
            uint32_t* l1EntryHWAddr = getL1PageEntryRealHWAddress(l1Index);

            // Deleting L2 pagetable page
            uint32_t l2PagePhysAddr = (*l1EntryHWAddr) & CCPU::ADDR_MASK;
            uint32_t l2PageNr = getPageNrFromPhysicalAddress(l2PagePhysAddr);
            uint32_t* l2PageHWAddr = getPageRealHWAddress(l2PageNr, m_MemStart);
            memset((void*) l2PageHWAddr, 0, 4096);

            setPageUsed(l2PageNr, false);
            (*l1EntryHWAddr) = 0;
            m_dirPagesAllocated--;
        }

        decreasePagersPosition(l1Index, l2Index);
    }

    return true;
}

/******************************************************************************/

struct TProcessParams {
    CCPU * ccpu;
    void * rest;
    void (* entryPoint) (CCPU *, void *);
};

void * threadRoutine(void* pParams) {
    TProcessParams * params = (TProcessParams*) pParams;

    pthread_mutex_lock(&threadMutex);
    runningThreads++;
    pthread_mutex_unlock(&threadMutex);

    // Running the given method
    params->entryPoint(params->ccpu, params->rest);

    delete params->ccpu;

    pthread_mutex_lock(&threadMutex);

    runningThreads--;
    pthread_cond_signal(&runningThreadsCond);
    pthread_mutex_unlock(&threadMutex);
    
    delete params;

    return NULL;
}

/**
 * @param address
 * @param write
 * @return 
 */
bool SladkeCPUpripraveneSpecialneProLadu::pageFaultHandler(uint32_t address, bool write) {
    if (!write) return false; // Can only fix pages if we want to write into them
    
    
    uint32_t l1Index = address >> 22;
    uint32_t l2Index = (address >> 12) & 0x3FF; // 0x3FF ~ 11 1111 1111
    
    if (l1Index * 1024 + l2Index >= m_pagesAllocated) { // Trying to write to unallocated mem
        return false;
    }
    
    
    // Want to write into a page - need to create a new one.
    pthread_mutex_lock(&pageMutex);
    uint32_t newDataPageNr = getFreePageNumber();

    if (newDataPageNr == 0) {
        pthread_mutex_unlock(&pageMutex);
        return false; // If no empty page was found, return fail.
    }

    

    setPageUsed(newDataPageNr, true);
    pthread_mutex_unlock(&pageMutex);

    uint32_t* l2EntryHWAddr = getL2PageEntryRealHWAddress(l1Index, l2Index);

    uint32_t oldDataPagePhysAddr = (*l2EntryHWAddr) & CCPU::ADDR_MASK;
    uint32_t oldDataPageNr = getPageNrFromPhysicalAddress(oldDataPagePhysAddr);
    uint32_t* oldDataPageHWAddr = getPageRealHWAddress(oldDataPageNr, m_MemStart);

    uint32_t * newDataPageHWAddr = getPageRealHWAddress(newDataPageNr, m_MemStart);

    // Copy data from old to new data page
    memcpy((void*) newDataPageHWAddr, (void*) oldDataPageHWAddr, 4096);

    // Edit l2 entry
    uint32_t newL2Entry = createEntry(getPagePhysicalAddress(newDataPageNr), 0, 0, 1, 1);
    (*l2EntryHWAddr) = newL2Entry;

    return true;
}

bool SladkeCPUpripraveneSpecialneProLadu::CopyMemorySpace(SladkeCPUpripraveneSpecialneProLadu* destCPU) {
    bool res = true;

    res = destCPU->SetMemLimit(this->m_pagesAllocated);
    if (!res) return false;

    // destCPU's pages allocated. Now need to physically copy data
    if (m_pagesAllocated == 0) return true;

    uint32_t l1Index = 0;
    uint32_t l2Index = 0;

    for (uint32_t i = 0; i < m_pagesAllocated; i++) {
        // Original page
        uint32_t* origL2EntryHWAddr = getL2PageEntryRealHWAddress(l1Index, l2Index);

        uint32_t origDataPagePhysAddr = (*origL2EntryHWAddr) & CCPU::ADDR_MASK;
        uint32_t origDataPageNr = getPageNrFromPhysicalAddress(origDataPagePhysAddr);
        uint32_t* origDataPageHWAddr = getPageRealHWAddress(origDataPageNr, m_MemStart);

        // New, to-be-copied-into page
        uint32_t* newL2EntryHWAddr = destCPU->getL2PageEntryRealHWAddress(l1Index, l2Index);

        uint32_t newDataPagePhysAddr = (*newL2EntryHWAddr) & CCPU::ADDR_MASK;
        uint32_t newDataPageNr = getPageNrFromPhysicalAddress(newDataPagePhysAddr);
        uint32_t* newDataPageHWAddr = getPageRealHWAddress(newDataPageNr, m_MemStart);
        increasePagersPosition(l1Index, l2Index);

        // Data copying
        memcpy((void*) newDataPageHWAddr, (void*) origDataPageHWAddr, 4096);
    }

    return true;
}

/**
 * Copy process' memory space using copy-on-write
 * @param destCPU
 * @return 
 */
bool SladkeCPUpripraveneSpecialneProLadu::CopyOnWriteDuplicate(SladkeCPUpripraveneSpecialneProLadu* destCPU) {
    destCPU->m_dirPagesAllocated = this->m_dirPagesAllocated;
    destCPU->m_pagesAllocated = this->m_pagesAllocated;


    uint32_t l2PagesNeeded = m_dirPagesAllocated - 1; // 1 - L1 dir, already created

    uint32_t * freePages = new uint32_t[l2PagesNeeded];
    for (uint32_t i = 0; i < l2PagesNeeded; i++) {
        uint32_t l2PageNum = getFreePageNumber();

        if (l2PageNum == 0) {
            for (uint32_t j = 0; j < i; j++) {
                setPageUsed(freePages[j], false);
            }
            return false;
        }

        freePages[i] = l2PageNum;
        setPageUsed(l2PageNum, true);
    }

    /** Pages allocated -- < creating L1 entries > */
    for (uint32_t i = 0; i < l2PagesNeeded; i++) {
        uint32_t l2PageNr = freePages[i];
        uint32_t l2PageHWAddr = getPagePhysicalAddress(l2PageNr);

        uint32_t newL1Entry = createEntry(l2PageHWAddr, 0, 0, 1, 1);

        uint32_t* l1EntryAddr = destCPU->getL1PageEntryRealHWAddress(i);
        (*l1EntryAddr) = newL1Entry;
    }

    /* L1 entries created - need to create L2 entries and adjust WRITABLE flag */
    uint32_t l1Index = 0;
    uint32_t l2Index = 0;

    for (uint32_t i = 0; i < m_pagesAllocated; i++) {
        uint32_t* origL2EntryHWAddr = getL2PageEntryRealHWAddress(l1Index, l2Index);
        setPageEntryWritable(origL2EntryHWAddr, false);

        uint32_t* newL2EntryHWAddr = destCPU->getL2PageEntryRealHWAddress(l1Index, l2Index);
        (*newL2EntryHWAddr) = (*origL2EntryHWAddr);

        increasePagersPosition(l1Index, l2Index);
    }


    return true;
}

bool SladkeCPUpripraveneSpecialneProLadu::NewProcess(void * processArg,
        void (* entryPoint) (CCPU *, void *),
        bool copyMem) {
    /*
     * Creating new CPU.
     * Get a free page, set as used.
     * Create L1 pager in it.
     */

    /************     CRITICAL    ****************************/
    pthread_mutex_lock(&pageMutex);


    uint32_t l1PageNr = getFreePageNumber();

    if (l1PageNr == 0) { // no free pages
        pthread_mutex_unlock(&pageMutex);
        return false;
    }
    setPageUsed(l1PageNr, true);

    uint32_t l1PagePhysAddr = getPagePhysicalAddress(l1PageNr);

    SladkeCPUpripraveneSpecialneProLadu * cpu = new SladkeCPUpripraveneSpecialneProLadu(m_MemStart, l1PagePhysAddr);


    /*       COPYING MEMORY SPACE   */
    if (copyMem) {
        /*  Basic copy-everything approach  */
        /*
        if (!CopyMemorySpace(cpu)) {
            delete cpu;
            pthread_mutex_unlock(&pageMutex);
            return false;
        }
         */

        /*  --- Copy-on-write approach --- */
        if (!CopyOnWriteDuplicate(cpu)) {
            delete cpu;
            pthread_mutex_unlock(&pageMutex);
            return false;
        }
    }


    pthread_mutex_unlock(&pageMutex);
    /************     END CRITICAL    ****************************/


    TProcessParams *params = new TProcessParams;
    params->ccpu = cpu;
    params->rest = processArg;
    params->entryPoint = entryPoint;

    // Creating thread
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_t process;
    pthread_create(&process, &attr, threadRoutine, (void*) params);
    pthread_attr_destroy(&attr);
    return true;
}

/******************************************************************************/

void SladkeCPUpripraveneSpecialneProLadu::printTables() {
    uint32_t l1Index = m_pagesAllocated / 1024;
    uint32_t l2Index = m_pagesAllocated % 1024;

    cout << "MyCPU table listing" << endl;
    cout << "Top-level table" << endl;
    for (uint32_t i = 0; i <= l1Index; i++) {
        cout << "\t" << i << "\t" << ((*getL1PageEntryRealHWAddress(i)) & CCPU::ADDR_MASK) << "\t|" << endl;
    }
    cout << "----------------------------------------" << endl;

    cout << "L2 tables" << endl;
    for (uint32_t i = 0; i <= l1Index; i++) {
        uint32_t l2ItemsCount = CCPU::PAGE_DIR_ENTRIES;
        if (i == l1Index) l2ItemsCount = l2Index;

        cout << "\tL2 (" << i << ") - " << l2ItemsCount << " entries." << endl;
        for (uint32_t j = 0; j < l2ItemsCount + 10; j++) {
            cout << j << "\t\t" << i * 1024 * 4096 + j * 4096 << "\t" << ((*getL2PageEntryRealHWAddress(i, j)) & CCPU::ADDR_MASK)
                    << "\t";

            uint32_t* debugAddr = getL2PageEntryRealHWAddress(i, j);
            printBinary(*debugAddr);

            cout << endl;
        }
    }
    cout << "----------------------------------------" << endl;

}