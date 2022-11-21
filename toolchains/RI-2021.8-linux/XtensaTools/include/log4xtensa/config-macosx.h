// Module:  Log4CPLUS
// File:    config-macosx.h
// Created: 7/2003
// Author:  Christopher R. Bailey
//
//
// Copyright (C) Tad E. Smith  All rights reserved.
//
// This software is published under the terms of the Apache Software
// License version 1.1, a copy of which has been included with this
// distribution in the LICENSE.APL file.
//

// 2005.09.01.  Tensilica.  Global replace of log4cplus/LOG4CPLUS with log4xtensa/LOG4XTENSA
//                          to avoid potential conflicts with customer code independently 
//                          using log4cplus.


/** @file */

#ifndef LOG4CPLUS_CONFIG_MACOSX_HEADER_
#define LOG4CPLUS_CONFIG_MACOSX_HEADER_

#if (defined(__APPLE__) || (defined(__MWERKS__) && defined(__MACOS__)))

#define HAVE_SSTREAM 1
#define HAVE_GETTIMEOFDAY 1

#endif // MACOSX
#endif // LOG4CPLUS_CONFIG_MACOSX_HEADER_
