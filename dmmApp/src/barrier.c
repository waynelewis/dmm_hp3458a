/* async. device support which work like a pthread barrier
 *
 * Wait for all records of a barrier to begin processing.
 * Then complete each FIFO.
 */

#include <string.h>

#ifndef USE_TYPED_RSET
#  define USE_TYPED_RSET
#endif
#ifndef USE_TYPED_DSET
#  define USE_TYPED_DSET
#endif

#include <epicsAssert.h>
#include <epicsStdio.h>
#include <epicsString.h>
#include <ellLib.h>
#include <dbDefs.h>
#include <epicsMutex.h>
#include <callback.h>
#include <cantProceed.h>

#include <dbLock.h>
#include <link.h>
#include <dbCommon.h>
#include <devSup.h>
#include <recSup.h>

#include <epicsExport.h>

typedef struct Barrier Barrier;

typedef struct {
    ELLNODE node;
    dbCommon *prec;
    Barrier *barrier;
} WaitNode;

struct Barrier {
    ELLNODE node;

    const char *name;

    epicsMutexId mutex;

    CALLBACK done;

    ELLLIST waiters;
    /* number of workers at barrier.
     * Also serves as re-queue flag during barrier release process.
     * Must be <= two times the number of waiters
     */
    unsigned ready;
};

static ELLLIST allBarriers;

static long init_record(dbCommon* prec)
{
    ELLNODE *cur;
    WaitNode *node = callocMustSucceed(1, sizeof(*node), "barrier init_record");
    DBLINK *plink = dbGetDevLink(prec);
    const char *name = plink->value.instio.string;

    assert(plink && plink->type==INST_IO);

    node->prec = prec;

    for(cur = ellFirst(&allBarriers); cur; cur = ellNext(cur))
    {
        Barrier *barrier = CONTAINER(cur, Barrier, node);
        if(strcmp(barrier->name, name)==0) {
            node->barrier = barrier;
            break;
        }
    }

    if(!node->barrier) {
        node->barrier = callocMustSucceed(1, sizeof(*node->barrier), "barrier create");
        node->barrier->name = epicsStrDup(name);
        node->barrier->mutex = epicsMutexMustCreate();
        ellAdd(&allBarriers, &node->barrier->node);
    }

    ellAdd(&node->barrier->waiters, &node->node);

    prec->dpvt = node;
    return 0;
}

static void barrier_done(CALLBACK *cb);

static int barrier_check(Barrier *barrier)
{
    if(barrier->ready == ellCount(&barrier->waiters)) {
        /* last hit */
        callbackSetCallback(barrier_done, &barrier->done);
        callbackSetUser(barrier, &barrier->done);
        callbackSetPriority(priorityLow, &barrier->done);
        /* trigger calllback after unlock */
        return 1;
    }
    return 0;
}

static void barrier_done(CALLBACK *cb)
{
    Barrier *barrier;
    callbackGetUser(barrier, cb);
    ELLNODE *cur;
    int last = 0;
    int debug = 0;

    /* Don't decrement barrier->ready until after processing.
     * Some/all records may re-hit the barrier before we return!
     */

    for(cur = ellFirst(&barrier->waiters); cur; cur = ellNext(cur))
    {
        WaitNode *node = CONTAINER(cur, WaitNode, node);
        dbCommon *prec = node->prec;
        rset *sup = prec->rset;

        dbScanLock(prec);
        debug |= prec->tpro>1;
        (*sup->process)(prec);
        dbScanUnlock(prec);
    }

    epicsMutexMustLock(barrier->mutex);
    assert(barrier->ready <= ellCount(&barrier->waiters)*2);
    /* complete all records.  Some may have re-hit */
    barrier->ready -= ellCount(&barrier->waiters);
    last = barrier_check(barrier);
    epicsMutexUnlock(barrier->mutex);

    if(debug)
        printf(" barrier %s %spassed\n",
               barrier->name,
               last ? "re-" : "");

    /* all our records have already hit the barrier again */
    if(last)
        callbackRequest(&barrier->done);
}

static long hit_barrier(dbCommon* prec)
{
    int last = 0;
    WaitNode *node = prec->dpvt;
    Barrier *barrier;
    if(!node) return 0; /* should not happen */

    barrier = node->barrier;

    epicsMutexMustLock(barrier->mutex);

    if(!prec->pact) {
        /* hit barrier */

        /* always add to waiters list */
        barrier->ready++;
        assert(barrier->ready <= ellCount(&barrier->waiters)*2);

        prec->pact = 1;

        last = barrier_check(barrier);

        if(prec->tpro>1)
            printf("%s : hits barrier %s%s\n",
                   prec->name, barrier->name,
                   last ? " last" : "");

    } else {
        /* completion */
        prec->pact = 0;
        if(prec->tpro>1)
            printf("%s : passes barrier %s\n",
                   prec->name, barrier->name);
    }

    epicsMutexUnlock(barrier->mutex);

    if(last)
        callbackRequest(&barrier->done);

    return 0;
}

static struct {
    dset common;
    long (*hit)(dbCommon*);
} devLoBarrier = {
    {5, NULL, NULL, &init_record, NULL},
    &hit_barrier
};

epicsExportAddress(dset, devLoBarrier);
