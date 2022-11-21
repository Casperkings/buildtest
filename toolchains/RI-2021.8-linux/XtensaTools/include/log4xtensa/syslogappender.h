// Module:  Log4CPLUS
// File:    syslogappender.h
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

// 2005.09.01.  Tensilica.  Global replace of log4cplus/LOG4CPLUS with log4xtensa/LOG4XTENSA
//                          to avoid potential conflicts with customer code independently 
//                          using log4cplus.


/** @file */

#ifndef _LOG4XTENSA_SYSLOG_APPENDER_HEADER_
#define _LOG4XTENSA_SYSLOG_APPENDER_HEADER_

#include <log4xtensa/config.h>

#if defined(HAVE_SYSLOG_H) && !defined(_WIN32)
#include <log4xtensa/appender.h>

namespace log4xtensa {

    /**
     * Appends log events to a file. 
     */
    class SysLogAppender : public Appender {
    public:
      // Ctors
        SysLogAppender(const tstring& ident);
        SysLogAppender(const log4xtensa::helpers::Properties properties);

      // Dtor
        virtual ~SysLogAppender();

      // Methods
        virtual void close();

    protected:
        virtual int getSysLogLevel(const LogLevel& ll) const;
        virtual void append(const spi::InternalLoggingEvent& event);

      // Data
        tstring ident;

    private:
      // Disallow copying of instances of this class
        SysLogAppender(const SysLogAppender&);
        SysLogAppender& operator=(const SysLogAppender&);
    };

} // end namespace log4xtensa

#endif // defined(HAVE_SYSLOG_H)

#endif // _LOG4XTENSA_SYSLOG_APPENDER_HEADER_

