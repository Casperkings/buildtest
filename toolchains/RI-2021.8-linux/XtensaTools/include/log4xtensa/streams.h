// Module:  Log4CPLUS
// File:    streams.h
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

#ifndef LOG4XTENSA_STREAMS_HEADER_
#define LOG4XTENSA_STREAMS_HEADER_

#include <log4xtensa/config.h>
#include <log4xtensa/tstring.h>

#include <iostream>
#ifdef HAVE_SSTREAM
#  include <sstream>
#  define LOG4XTENSA_STREAM_NAMESPACE std
#elif defined(HAVE_STRSTREAM)
#  include <strstream>
#  if defined(__DECCXX) && !defined(__USE_STD_IOSTREAM)
#    define LOG4XTENSA_STREAM_NAMESPACE
#  else
#    define LOG4XTENSA_STREAM_NAMESPACE std
#  endif
#elif defined(HAVE_STRSTREAM_H)
#  include <strstream.h>
#  define LOG4XTENSA_STREAM_NAMESPACE
#else
#  error "There doesn't appear to be any s*stream headers!!"
#endif

#ifdef UNICODE
    namespace log4xtensa {
        typedef LOG4XTENSA_STREAM_NAMESPACE::wostream tostream;
        typedef LOG4XTENSA_STREAM_NAMESPACE::wistream tistream;
        typedef LOG4XTENSA_STREAM_NAMESPACE::wostringstream tostringstream;
        static tostream &tcout = LOG4XTENSA_STREAM_NAMESPACE::wcout;
        static tostream &tcerr = LOG4XTENSA_STREAM_NAMESPACE::wcerr;
    }

LOG4XTENSA_EXPORT log4xtensa::tostream& operator <<(log4xtensa::tostream&, const char* psz );

#else
    namespace log4xtensa {
        typedef LOG4XTENSA_STREAM_NAMESPACE::ostream tostream;
        typedef LOG4XTENSA_STREAM_NAMESPACE::istream tistream;
        static tostream &tcout = LOG4XTENSA_STREAM_NAMESPACE::cout;
        static tostream &tcerr = LOG4XTENSA_STREAM_NAMESPACE::cerr;
#ifdef HAVE_SSTREAM
        typedef LOG4XTENSA_STREAM_NAMESPACE::ostringstream tostringstream;
#else
        class tostringstream : public LOG4XTENSA_STREAM_NAMESPACE::ostrstream {
        public:
            tstring str() { 
                char *ptr = LOG4XTENSA_STREAM_NAMESPACE::ostrstream::str(); 
                if(ptr) {
                    return tstring(ptr, pcount());
                }
                else {
                    return tstring();
                }
            }
        };

#endif // HAVE_SSTREAM
    }
#endif // UNICODE

#endif // LOG4XTENSA_STREAMS_HEADER_

