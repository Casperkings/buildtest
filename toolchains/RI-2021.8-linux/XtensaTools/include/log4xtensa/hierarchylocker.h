// Module:  Log4CPLUS
// File:    hierarchylocker.h
// Created: 8/2003
// Author:  Tad E. Smith
//
//
// Copyright (C) Tad E. Smith  All rights reserved.
//
// This software is published under the terms of the Apache Software
// License version 1.1, a copy of which has been included with this
// distribution in the LICENSE.APL file.
//

// 2017.07.25.  Cadence.    Fix ~HierarchyLocker for GCC >= 6
// 2005.09.03.  Tensilica.  Use reference when returning Logger
// 2005.09.01.  Tensilica.  Global replace of log4cplus/LOG4CPLUS with log4xtensa/LOG4XTENSA
//                          to avoid potential conflicts with customer code independently 
//                          using log4cplus.


/** @file */

#ifndef _LOG4XTENSA_HIERARCHY_LOCKER_HEADER_
#define _LOG4XTENSA_HIERARCHY_LOCKER_HEADER_

#include <log4xtensa/hierarchy.h>


namespace log4xtensa {

    /**
     * This is used to lock a Hierarchy.  The dtor unlocks the Hierarchy.
     */
    class LOG4XTENSA_EXPORT HierarchyLocker {
    public:
      // ctor & dtor
        HierarchyLocker(Hierarchy& h);
#if (__GNUC__ >= 6)
        ~HierarchyLocker() throw();
#else
        ~HierarchyLocker();
#endif
        
        /**
         * Calls the <code>resetConfiguration()</code> method on the locked Hierarchy.
         */
        void resetConfiguration(); 
        
        /**
         * Calls the <code>getInstance()</code> method on the locked Hierarchy.
         */
        Logger& getInstance(const log4xtensa::tstring& name);
        
        /**
         * Calls the <code>getInstance()</code> method on the locked Hierarchy.
         */
        Logger& getInstance(const log4xtensa::tstring& name, spi::LoggerFactory& factory);
        
        void addAppender(Logger &logger, log4xtensa::SharedAppenderPtr& appender);
        
    private:
      // Data
        Hierarchy& h;
        log4xtensa::thread::Guard hierarchyLocker;
        LoggerList loggerList;
    };

} // end namespace log4xtensa

#endif // _LOG4XTENSA_HIERARCHY_LOCKER_HEADER_

