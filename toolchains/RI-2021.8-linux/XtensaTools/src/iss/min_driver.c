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

/* ===================================================================== */
//
// This is the very minimal interface to the XTMP library that leaves
// all driver functions unimplemented. It can be used only by stepping
// each core by one cycle:
//
//   while (!done) {
//     XTMP_startOfCycleProcessing();
//     XTMP_stepCore(core, 1); // ...
//     XTMP_endOfCycleProcessing();
//   }
//
// Caveats:
// - No XTMP_stepSystem()
// - No user threads
// - No events
// - Attaching the debugger to a core will be problematic
//
/* ===================================================================== */

static XTMP_driverTable driverTable = {
  XTMP_VERSION,
  NULL,         /* obsolete */
  NULL,         /* obsolete */
  NULL,         /* initialize */
  NULL,         /* threadNew */
  NULL,         /* cpuThreadNew */
  NULL,         /* start */
  NULL,         /* stop */
  NULL,         /* wait */
  NULL,         /* eventNew */
  NULL,         /* eventFree */
  NULL,         /* waitOnEvent */
  NULL,         /* fireEvent */
  NULL,         /* hasEventFired */
  NULL,         /* time */
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

