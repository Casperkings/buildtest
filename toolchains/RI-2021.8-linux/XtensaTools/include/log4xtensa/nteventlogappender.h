// Module:  Log4CPLUS
// File:    nteventlogappender.h
// Created: 4/2003
// Author:  Michael CATANZARITI
//
// Copyright (C) Michael CATANZARITI  All rights reserved.
//
// This software is published under the terms of the Apache Software
// License version 1.1, a copy of which has been included with this
// distribution in the LICENSE.APL file.
//

// 2005.09.01.  Tensilica.  Global replace of log4cplus/LOG4CPLUS with log4xtensa/LOG4XTENSA
//                          to avoid potential conflicts with customer code independently 
//                          using log4cplus.


/** @file */

#ifndef _LOG4XTENSA_NT_EVENT_LOG_APPENDER_HEADER_
#define _LOG4XTENSA_NT_EVENT_LOG_APPENDER_HEADER_

#include <log4xtensa/config.h>
#if defined(_WIN32)

#include <log4xtensa/appender.h>


namespace log4xtensa {

    /**
     * Appends log events to NT EventLog. 
     */
    class LOG4XTENSA_EXPORT NTEventLogAppender : public Appender {
    public:
      // ctors
        NTEventLogAppender(const log4xtensa::tstring& server, 
                           const log4xtensa::tstring& log, 
                           const log4xtensa::tstring& source);
        NTEventLogAppender(const log4xtensa::helpers::Properties properties);

      // dtor
        virtual ~NTEventLogAppender();

      // public Methods
        virtual void close();

    protected:
        virtual void append(const spi::InternalLoggingEvent& event);
        virtual WORD getEventType(const spi::InternalLoggingEvent& event);
        virtual WORD getEventCategory(const spi::InternalLoggingEvent& event);
        void init();

        /*
         * Add this source with appropriate configuration keys to the registry.
         */
        void addRegistryInfo();

      // Data
        log4xtensa::tstring server;
        log4xtensa::tstring log;
        log4xtensa::tstring source;
        HANDLE hEventLog;
        SID* pCurrentUserSID;

    private:
      // Disallow copying of instances of this class
        NTEventLogAppender(const NTEventLogAppender&);
        NTEventLogAppender& operator=(const NTEventLogAppender&);
    };

} // end namespace log4xtensa

#endif // _WIN32
#endif //_LOG4XTENSA_NT_EVENT_LOG_APPENDER_HEADER_

