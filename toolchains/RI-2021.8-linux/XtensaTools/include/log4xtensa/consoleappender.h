// Module:  Log4CPLUS
// File:    consoleappender.h
// Created: 6/2001
// Author:  Tad E. Smith
//
//
// Copyright (C) Tad E. Smith  All rights reserved.
//
// This software is published under the terms of the Apache Software
// License version 1.1, a copy of which has been included with this
// distribution in the LICENSE.APL file.
//

// 2006.01.20.  Tensilica.  Change default value for immediateFlush to true to match log4j.
// 2005.09.01.  Tensilica.  Global replace of log4cplus/LOG4CPLUS with log4xtensa/LOG4XTENSA
//                          to avoid potential conflicts with customer code independently 
//                          using log4cplus.


/** @file */

#ifndef _LOG4XTENSA_CONSOLE_APPENDER_HEADER_
#define _LOG4XTENSA_CONSOLE_APPENDER_HEADER_

#include <log4xtensa/config.h>
#include <log4xtensa/appender.h>

namespace log4xtensa {
    /**
     * ConsoleAppender appends log events to <code>System.out</code> or
     * <code>System.err</code> using a layout specified by the
     * user. The default target is <code>System.out</code>.
     */
    class LOG4XTENSA_EXPORT ConsoleAppender : public Appender {
    public:
      // Ctors
        ConsoleAppender(bool logToStdErr = false, bool immediateFlush = true);
        ConsoleAppender(const log4xtensa::helpers::Properties properties);

      // Dtor
        ~ConsoleAppender();

      // Methods
        virtual void close();

    protected:
        virtual void append(const spi::InternalLoggingEvent& event);

      // Data
        bool logToStdErr;
        /**
         * Immediate flush means that the underlying output stream
         * will be flushed at the end of each append operation.
         */
        bool immediateFlush;
    };

} // end namespace log4xtensa

#endif // _LOG4XTENSA_CONSOLE_APPENDER_HEADER_

