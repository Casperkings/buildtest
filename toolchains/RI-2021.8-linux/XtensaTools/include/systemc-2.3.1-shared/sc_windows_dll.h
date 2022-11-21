#if !defined(_SC_WINDOWS_DLL_H_)
#define _SC_WINDOWS_DLL_H_

// Copyright (c) 2010-2012 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.


// This file defines two macros, SYSTEMC_API and SC_PRAGMA_WARNING, that have
// been added by Tensilica, Inc to source code files that came from the OSCI
// SystemC distribution to allow building the SystemC library as a MS Windows
// Dynamic Link Library (DLL).

#ifdef _MSC_VER

#define SC_PRAGMA_WARNING(arg) __pragma(warning(arg))

#if defined(SYSTEMC_DLL)
#if defined(EXPORT_SYSTEMC_INTERFACE)
#define SYSTEMC_API __declspec(dllexport)
#else
#define SYSTEMC_API __declspec(dllimport)
#endif
#else
#define SYSTEMC_API
#endif

SC_PRAGMA_WARNING(push)
SC_PRAGMA_WARNING(disable:4251)
SC_PRAGMA_WARNING(disable:4275)
#include <vector>
SC_PRAGMA_WARNING(pop)

#else

#define SC_PRAGMA_WARNING(arg)
#define SYSTEMC_API

#endif

#endif  // _SC_WINDOWS_DLL_H_
