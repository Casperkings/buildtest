// Module:  Log4CPLUS
// File:    sleep.h
// Created: 5/2003
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

#ifndef _LOG4XTENSA_HELPERS_SLEEP_HEADER_
#define _LOG4XTENSA_HELPERS_SLEEP_HEADER_

#include <log4xtensa/config.h>


namespace log4xtensa {
    namespace helpers {
        LOG4XTENSA_EXPORT void sleep(unsigned long secs, 
                                    unsigned long nanosecs = 0);
        LOG4XTENSA_EXPORT void sleepmillis(unsigned long millis);
    } // end namespace helpers
} // end namespace log4xtensa

#endif // _LOG4XTENSA_HELPERS_SLEEP_HEADER_

