// Module:  Log4CPLUS
// File:    nullappender.h
// Created: 6/2003
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

#ifndef _LOG4XTENSA_NULL_APPENDER_HEADER_
#define _LOG4XTENSA_NULL_APPENDER_HEADER_

#include <log4xtensa/config.h>
#include <log4xtensa/appender.h>
#include <log4xtensa/helpers/property.h>


namespace log4xtensa {

    /**
     * Appends log events to a file. 
     */
    class LOG4XTENSA_EXPORT NullAppender : public Appender {
    public:
      // Ctors
        NullAppender();
        NullAppender(const log4xtensa::helpers::Properties& properties);

      // Dtor
        virtual ~NullAppender();

      // Methods
        virtual void close();

    protected:
        virtual void append(const log4xtensa::spi::InternalLoggingEvent& event);

    private:
      // Disallow copying of instances of this class
        NullAppender(const NullAppender&);
        NullAppender& operator=(const NullAppender&);
    };

} // end namespace log4xtensa

#endif // _LOG4XTENSA_NULL_APPENDER_HEADER_

