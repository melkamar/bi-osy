#ifndef __PROGTEST__
#include "common.h"
using namespace std;
#endif /* __PROGTEST__ */


#define mint uint8_t

CReactNode * duplicateTree(CReactNode * root) {
    if (root == NULL) return NULL;
    return new CReactNode(
            root->m_Product,
            root->m_Energy,
            duplicateTree(root->m_L),
            duplicateTree(root->m_R));
}

struct CYKField {

    CYKField() {
        for (int i = 0; i < QUARKS_NR; i++) {
            nodes[i] = NULL;
        }
    }

    ~CYKField() {
        for (int i = 0; i < QUARKS_NR; i++) {
            if (nodes[i] != NULL) {
                nodes[i]->m_L = NULL;
                nodes[i]->m_R = NULL;

                delete nodes[i];
            }
        }
    }

    CReactNode * nodes[QUARKS_NR];
};

class CYK {
public:
    //    static const bool verbose = true;
    static const bool verbose = false;

    mint *quarks;
     int quarkCount;
    const TRequest * request;
    const TGenerator generator;

    CYKField *** cykTable;

    ~CYK() {
        for (int y = quarkCount - 1; y >= 0; y--) {
            for (int x = 0; x < quarkCount; x++) {
                delete cykTable[x][y];
            }
        }

        for (int y = 0; y < quarkCount; y++) {
            delete [] cykTable[y];
        }

        delete[] cykTable;
    }

    CYK(const TRequest * pRequest,
            const TGenerator &pGenerator)
    : quarks(pRequest->m_Fuel), quarkCount(pRequest->m_FuelNr), request(pRequest),
    generator(pGenerator) {

        cykTable = new CYKField**[quarkCount];
        for (int i = 0; i < quarkCount; i++) {
            cykTable[i] = new CYKField*[quarkCount];
        }

        for (int i = 0; i < quarkCount; i++) {
            for (int j = 0; j < quarkCount; j++) {
                cykTable[i][j] = new CYKField();
            }
        }

        if (verbose) {
            cout << "quarks: ";
            for (int i = 0; i < quarkCount; i++) {
                cout << (int) quarks[i] << ", ";
            }
            cout << endl;
        }

        for (int i = 0; i < quarkCount; i++) {
            //            cykTable[i][0]->values[quarks[i]] = 0;
            cykTable[i][0]->nodes[quarks[i]] = new CReactNode(quarks[i], 0);
        }

        if (verbose) {
            cout << "going to fillCYK" << endl;
        }

        for (int i = 1; i < quarkCount; i++) {
            fillCYKRow(i);
        }

        if (verbose) {
            cout << "done." << endl;
        }
    }

    void fillCYKRow(int row) {
        if (verbose) {
            cout << "fillCYKRow(" << row << ")" << endl;
        }

        for (int i = 0; i < quarkCount; i++) {
            fillCYKField(i, row);
        }
    }

    void fillCYKField(const int x, const int y) {
        if (verbose) {
            cout << "fillCYKField(" << x << ", " << y << ")" << endl;
        }

        int leftX = x % quarkCount;
        int leftY = 0;

        int rightX = (x + 1) % quarkCount;
        int rightY = (y - 1);

        for (int step = 0; step < y; step++) {
            leftY = step;
            rightX = (x + step + 1) % quarkCount;
            rightY = y - step - 1;

            if (verbose) {
                cout << "  left: [" << leftX << ", " << leftY << "]" << endl;
                cout << "  right: [" << rightX << ", " << rightY << "]" << endl;
            }


            for (int leftQuark = 0; leftQuark < QUARKS_NR; leftQuark++) {
                CReactNode *leftQuarkNode = cykTable[leftX][leftY]->nodes[leftQuark];
                if (leftQuarkNode == NULL) continue; // This quark is not present at the position

                for (int rightQuark = 0; rightQuark < QUARKS_NR; rightQuark++) {
                    CReactNode *rightQuarkNode = cykTable[rightX][rightY]->nodes[rightQuark];
                    if (rightQuarkNode == NULL) continue; // This quark is not present at the position

                    // leftQuark + rightQuark fusion
                    // 1) see if the reaction is possible and what it gives
                    for (int i = 0; i < QUARKS_NR; i++) {
                        int energy = generator.m_Energy[leftQuark][rightQuark][i];

                        // 2) if the given energy is >0, the fusion is possible - 
                        // add original energies and this together.
                        if (energy != 0) {
                            unsigned int  totalEnergy = energy + leftQuarkNode->m_Energy + rightQuarkNode->m_Energy;

                            if (cykTable[x][y]->nodes[i] == NULL) {
                                cykTable[x][y]->nodes[i] = new CReactNode(i, totalEnergy, leftQuarkNode, rightQuarkNode);
                            } else if (cykTable[x][y]->nodes[i]->m_Energy < totalEnergy) {
                                cykTable[x][y]->nodes[i]->m_Energy = totalEnergy;
                                cykTable[x][y]->nodes[i]->m_Product = i;
                                cykTable[x][y]->nodes[i]->m_L = leftQuarkNode;
                                cykTable[x][y]->nodes[i]->m_R = rightQuarkNode;
                            }

                            if (verbose) {
                                cout << "  [" << leftQuark << "]+[" << rightQuark << "] -> [" <<
                                        i << "] (" << totalEnergy << ")" << endl;
                            }
                        }
                    }
                }
            }
        }
    }

    TSetup* getBest(mint quark) {
        if (verbose) {
            cout << "getBest" << endl;
        }

        CReactNode *bestNode = NULL;
        int startPos = 0;
        for (int i = 0; i < quarkCount; i++) {
            if (cykTable[i][quarkCount - 1]->nodes[quark] == NULL) continue;

            if (bestNode == NULL || cykTable[i][quarkCount - 1]->nodes[quark]->m_Energy > bestNode->m_Energy) {
                bestNode = cykTable[i][quarkCount - 1]->nodes[quark];
                startPos = i;
            }
        }

        if (verbose) {
            cout << " > best found" << endl;
        }

        if (bestNode) {
            TSetup * best = new TSetup();
            best->m_Energy = bestNode->m_Energy;
            best->m_Root = duplicateTree(bestNode);
            best->m_StartPos = startPos;
            return best;
        } else {
            return NULL;
        }
    }
};

TSetup* countCYK(const TGenerator &generator, const TRequest * request) {
//    const int quarksCount = request->m_FuelNr;
//    const uint8_t *quarks = request->m_Fuel;
    const uint8_t finalQuark = request->m_FinalProduct;

    //    cout << "countCYK" << endl;

    CYK cyk(request, generator);

    return cyk.getBest(finalQuark);
}

void optimizeEnergySeq(const TGenerator* generators,
        int generatorsNr,
        const TRequest * request,
        TSetup * setup) {


    TSetup * best = NULL;
    for (int i = 0; i < generatorsNr; i++) {
        TSetup * tmp = countCYK(generators[i], request);
        if (!tmp) continue; // if no possible tree, skip for next generator

        if (best == NULL || tmp->m_Energy > best->m_Energy) {
            if (best){
                delete best->m_Root;
                delete best;
            }
            best = tmp;
            best->m_Generator = i;
        } else {
            delete tmp->m_Root;
            delete tmp;
        }
    }

    if (best) {
        setup->m_Energy = best->m_Energy;
        setup->m_Generator = best->m_Generator;
        setup->m_Root = best->m_Root;
        setup->m_StartPos = best->m_StartPos;
        delete best;
    } else {
        setup->m_Root = NULL;
    }
}

struct TFuelRingJob {
    int generators;
    int submitCount;
    pthread_mutex_t lock;
    TSetup *best;

    TFuelRingJob(int pGenerators) : generators(pGenerators) {
        pthread_mutex_init(&lock, NULL);
        best = NULL;
        submitCount = 0;
    }

    ~TFuelRingJob() {
        pthread_mutex_destroy(&lock);
    }
};

class CJob {
public:
    const TGenerator *generator;
    const TRequest * request;
    TFuelRingJob * parentJob;
    int generatorID;

    CJob(const TRequest * pRequest, const TGenerator *pGenerator, TFuelRingJob * pParentJob, int pGeneratorID) :
    generator(pGenerator), request(pRequest), parentJob(pParentJob), generatorID(pGeneratorID) {

    }

    //    ~CJob() {
    //        delete parentJob;
    //    }
};

struct TWorkerParam {
    int ID;

    TWorkerParam(int pID) : ID(pID) {
    }
};

CJob** jobs;
int jobsCount = 0;
sem_t jobsSemaphoreBottom;
sem_t jobsSemaphoreTop;
pthread_mutex_t jobsMutex;

void (* melkamar_submitResult) (const TRequest*, TSetup *);

bool keepWaitingForJob = true;

void * workerRoutine(void * param) {

    TWorkerParam* info = new TWorkerParam(((TWorkerParam*) param)->ID);
    delete (TWorkerParam*) param;

    while (keepWaitingForJob) {

        /* Picking up a job - critical section */
        sem_wait(&jobsSemaphoreBottom);
        pthread_mutex_lock(&jobsMutex);

        if (jobsCount == 0) {
            pthread_mutex_unlock(&jobsMutex);
            break;
        }


        CJob * todo = new CJob(
                jobs[jobsCount - 1]->request,
                jobs[jobsCount - 1]->generator,
                jobs[jobsCount - 1]->parentJob,
                jobs[jobsCount - 1]->generatorID);
        delete jobs[jobsCount - 1];
        jobsCount--;

        pthread_mutex_unlock(&jobsMutex);
        sem_post(&jobsSemaphoreTop);
        /* end of critical section */


        TSetup * result = countCYK(*(todo->generator), todo->request);



        /* submitting job results - critical section */
        pthread_mutex_lock(&todo->parentJob->lock);

        if (result != NULL) {
            if (todo->parentJob->best == NULL || result->m_Energy > todo->parentJob->best->m_Energy) {
                // Overwriting new best - delete the old one
                if (todo->parentJob->best) {
                    delete todo->parentJob->best->m_Root;
                    delete todo->parentJob->best;
                }
                todo->parentJob->best = result;
                todo->parentJob->best->m_Generator = todo->generatorID;
            } else {
                // result is not the best - delete it
                delete result->m_Root;
                delete result;
            }
        }

        todo->parentJob->submitCount++;

        if (todo->parentJob->submitCount == todo->parentJob->generators) {
            // this was the last thread to finish work - submit it

            if (!todo->parentJob->best) {
                todo->parentJob->best = new TSetup();
                todo->parentJob->best->m_Root = NULL;
            }
            melkamar_submitResult(todo->request, todo->parentJob->best);
            delete todo->parentJob->best;
            delete todo->parentJob;
            delete todo;
        } else {
            pthread_mutex_unlock(&todo->parentJob->lock);
            delete todo;
        }

        /* end of critical section */
    }

    delete info;
    return NULL;
}

void optimizeEnergy(int threads,
        const TGenerator* generators,
        int generatorsNr,
        const TRequest *(* dispatcher)(void),
        void (* engines) (const TRequest *, TSetup *)) {

    melkamar_submitResult = engines;

    jobs = new CJob*[threads];

    /*************** Initializing threads *************************************/
    pthread_t *workerThreads = new pthread_t[threads];
    pthread_attr_t attributes;

    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_JOINABLE);

    pthread_mutex_init(&jobsMutex, NULL);
    sem_init(&jobsSemaphoreBottom, 0, 0);
    sem_init(&jobsSemaphoreTop, 0, threads);

    for (int i = 0; i < threads; i++) {
        TWorkerParam* param = new TWorkerParam(i);
        int res = pthread_create(&workerThreads[i], &attributes, workerRoutine, (void*) param);

        if (res) {
            cout << "Error when creating a thread." << endl;
        }
    }

    pthread_attr_destroy(&attributes);

    const TRequest * newRequest;
    while ((newRequest = dispatcher())) {
        //        requests.push_back(newRequest);

        TFuelRingJob* fullJob = new TFuelRingJob(generatorsNr);
        for (int i = 0; i < generatorsNr; i++) {
            CJob* newJob = new CJob(newRequest, &(generators[i]), fullJob, i);

            sem_wait(&jobsSemaphoreTop);
            pthread_mutex_lock(&jobsMutex);
            jobs[jobsCount++] = newJob;
            pthread_mutex_unlock(&jobsMutex);
            sem_post(&jobsSemaphoreBottom);
        }
    }

    for (int i = 0; i < threads; i++) {
        sem_post(&jobsSemaphoreBottom);
    }

    for (int i = 0; i < threads; i++) {
        pthread_join(workerThreads[i], NULL);
    }

    //    keepWaitingForJob = false;

    pthread_mutex_destroy(&jobsMutex);
    sem_destroy(&jobsSemaphoreBottom);
    sem_destroy(&jobsSemaphoreTop);

    delete [] jobs;
    delete [] workerThreads;
}



