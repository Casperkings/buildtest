/*
 * Copyright (c) 2007 Tensilica Inc.  ALL RIGHTS RESERVED.
 * 
 * These coded instructions, statements, and computer programs are
 * the copyrighted works and confidential proprietary information of
 * Tensilica Inc.  They may be adapted and modified by bona fide
 * purchasers for internal use, but no adapted or modified version
 * may be disclosed or distributed to third parties in any manner,
 * medium, or form, in whole or in part, without the prior  written
 * consent of Tensilica Inc.
 */

#if defined(_BUILD_INTERNAL_)
#include "mp.h"
#else
#include "iss/mp.h"
#endif

#include <stdlib.h>
#include <assert.h>

/* ===================================================================== */
//
// This is a very simple non-thread XTMP driver that implements only
// 3 functions: creating a core, stepping all the cores, reporting time.
// With this limited functionality, you can drive the simulation like this:
// 
//   while (!done) {
//     XTMP_startOfCycleProcessing();
//     XTMP_stepSystem(1);
//     XTMP_endOfCycleProcessing();
//   }
//
// Caveats:
// - No user threads
// - No events
// - Attaching the debugger to a core will be problematic
// 
/* ===================================================================== */

typedef struct _XtCore XtCore;

struct _XtCore {
  XtCore *next;
  XTMP_time (*stepCore)(XTMP_threadInfo *);
  XTMP_threadInfo *threadInfo;
};

static XTMP_time currentTime;
static XtCore *allCores;


/*
// XTMP_driverTable::cpuThreadNew(name, stepCore, syncCore, ti)
*/
static void *
createXtCore(const char *name,
             XTMP_time (*stepCore)(XTMP_threadInfo *),
             void (*syncCore)(XTMP_threadInfo *, XTMP_timeDelta),
             XTMP_threadInfo *ti)
{
  XtCore *core = (XtCore *) malloc(sizeof(XtCore));
  assert(core);

  core->next = allCores;
  core->stepCore = stepCore;
  core->threadInfo = ti;

  allCores = core;
  return core;
}


/*
// XTMP_driverTable::start(ncycles) 
*/
static void
stepAllCores(int ncycles)
{
  XtCore *core;

  assert(ncycles == 1);

  for (core = allCores; core; core = core->next) {
    if (!core->threadInfo->exitFlag) {
      core->stepCore(core->threadInfo);
    }
  }

  currentTime++;
}


/*
// XTMP_driverTable::time()
*/
static XTMP_time
systemTime(void)
{
  return currentTime;
}


static XTMP_driverTable driverTable = {
  XTMP_VERSION,
  NULL,         /* obsolete */
  NULL,         /* obsolete */
  NULL,         /* initialize */
  NULL,         /* threadNew */
  createXtCore, /* cpuThreadNew */
  stepAllCores, /* start */
  NULL,         /* stop */
  NULL,         /* wait */
  NULL,         /* eventNew */
  NULL,         /* eventFree */
  NULL,         /* waitOnEvent */
  NULL,         /* fireEvent */
  NULL,         /* hasEventFired */
  systemTime,   /* time */
  NULL          /* numberOfThreads */
};


/*
// If you don't want to use XTMP_main, you can simply put the code
// in XTMP_main directly into this main program.
*/
int
main(int argc, char **argv)
{
  int status;
  XTMP_initialize(&driverTable);
  status = XTMP_main(argc, argv);
  XTMP_cleanup();
  return status;
}

