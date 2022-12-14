/*****************************************************************************

  The following code is derived, directly or indirectly, from the SystemC
  source code Copyright (c) 1996-2014 by all Contributors.
  All Rights reserved.

  The contents of this file are subject to the restrictions and limitations
  set forth in the SystemC Open Source License (the "License");
  You may not use this file except in compliance with such restrictions and
  limitations. You may obtain instructions on how to receive a copy of the
  License at http://www.accellera.org/. Software distributed by Contributors
  under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
  ANY KIND, either express or implied. See the License for the specific
  language governing rights and limitations under the License.

 *****************************************************************************/

/*****************************************************************************

  sc_externs.h -- Declaration of `sc_main' and other global variables.

  Original Author: Stan Y. Liao, Synopsys, Inc.

  CHANGE LOG AT THE END OF THE FILE
 *****************************************************************************/


#ifndef SC_EXTERNS_H
#define SC_EXTERNS_H

#include "sc_windows_dll.h"


#if 1
extern "C" int sc_main( int argc, char* argv[] );

namespace sc_core {
	extern "C" int sc_elab_and_sim( int argc, char* argv[] );
	extern "C" int sc_argc();
	extern "C" const char* const* sc_argv();

} // namespace sc_core
#else
// TODO
//extern "C" int sc_main( int argc, char* argv[] );

namespace sc_core {
	extern "C" SYSTEMC_API int sc_elab_and_sim( int argc, char* argv[], int (sc_main)(int, char *[]) );
	extern "C" SYSTEMC_API int sc_argc();
	extern "C" SYSTEMC_API const char* const* sc_argv();

} // namespace sc_core
#endif

// $Log: sc_externs.h,v $
// Revision 1.5  2011/08/26 20:46:09  acg
//  Andy Goodrich: moved the modification log to the end of the file to
//  eliminate source line number skew when check-ins are done.
//
// Revision 1.4  2011/02/18 20:27:14  acg
//  Andy Goodrich: Updated Copyrights.
//
// Revision 1.3  2011/02/13 21:47:37  acg
//  Andy Goodrich: update copyright notice.
//
// Revision 1.2  2008/05/22 17:06:25  acg
//  Andy Goodrich: updated copyright notice to include 2008.
//
// Revision 1.1.1.1  2006/12/15 20:20:05  acg
// SystemC 2.3
//
// Revision 1.3  2006/01/13 18:44:29  acg
// Added $Log to record CVS changes into the source.
//

#endif
