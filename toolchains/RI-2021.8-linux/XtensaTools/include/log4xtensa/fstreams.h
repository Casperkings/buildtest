// Module:  Log4CPLUS
// File:    fstreams.h
// Created: 4/2003
// Author:  Tad E. Smith
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

#ifndef LOG4XTENSA_FSTREAMS_HEADER_
#define LOG4XTENSA_FSTREAMS_HEADER_

#include <log4xtensa/config.h>

#include <fstream>

#if defined(__DECCXX) && !defined(__USE_STD_IOSTREAM)
#  define LOG4XTENSA_FSTREAM_NAMESPACE
#else
#  define LOG4XTENSA_FSTREAM_NAMESPACE std
#endif


#ifdef UNICODE
    namespace log4xtensa {
        typedef LOG4XTENSA_FSTREAM_NAMESPACE::wofstream tofstream;
        typedef LOG4XTENSA_FSTREAM_NAMESPACE::wifstream tifstream;
    }
#else
    namespace log4xtensa {
        typedef LOG4XTENSA_FSTREAM_NAMESPACE::ofstream tofstream;
        typedef LOG4XTENSA_FSTREAM_NAMESPACE::ifstream tifstream;
    }
#endif // UNICODE

#endif // LOG4XTENSA_FSTREAMS_HEADER_

