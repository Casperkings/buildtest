// Module:  Log4CPLUS
// File:    tstring.h
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

#ifndef LOG4XTENSA_TSTRING_HEADER_
#define LOG4XTENSA_TSTRING_HEADER_

#include <log4xtensa/config.h>
#include <string>

#ifdef UNICODE
#  define LOG4XTENSA_TEXT(STRING) L##STRING
#else
#  define LOG4XTENSA_TEXT(STRING) STRING
#endif // UNICODE


#ifdef UNICODE
namespace log4xtensa {
    typedef wchar_t tchar;
    typedef std::wstring tstring;

    namespace helpers {
        LOG4XTENSA_EXPORT std::string tostring(const std::wstring&);
        LOG4XTENSA_EXPORT std::wstring towstring(const std::string&);
    }

}

#define LOG4XTENSA_C_STR_TO_TSTRING(STRING) log4xtensa::helpers::towstring(STRING)
#define LOG4XTENSA_STRING_TO_TSTRING(STRING) log4xtensa::helpers::towstring(STRING)
#define LOG4XTENSA_TSTRING_TO_STRING(STRING) log4xtensa::helpers::tostring(STRING)

#else
namespace log4xtensa {
    typedef char tchar;
    typedef std::string tstring;
}

#define LOG4XTENSA_C_STR_TO_TSTRING(STRING) std::string(STRING)
#define LOG4XTENSA_STRING_TO_TSTRING(STRING) STRING
#define LOG4XTENSA_TSTRING_TO_STRING(STRING) STRING

#endif // UNICODE

#endif // LOG4XTENSA_TSTRING_HEADER_

