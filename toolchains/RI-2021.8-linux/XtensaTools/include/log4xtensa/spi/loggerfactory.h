// Module:  Log4CPLUS
// File:    loggerfactory.h
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

// 2005.09.01.  Tensilica.  Use reference return type for makeNewLoggerInstance.
// 2005.09.01.  Tensilica.  Global replace of log4cplus/LOG4CPLUS with log4xtensa/LOG4XTENSA
//                          to avoid potential conflicts with customer code independently 
//                          using log4cplus.


/** @file */

#ifndef _LOG4XTENSA_SPI_LOGGER_FACTORY_HEADER
#define _LOG4XTENSA_SPI_LOGGER_FACTORY_HEADER

#include <log4xtensa/config.h>
#include <log4xtensa/tstring.h>
#include <log4xtensa/helpers/pointer.h>


namespace log4xtensa {
    // Forward Declarations
    class Logger;
    class Hierarchy;

    namespace spi {
        /**
         * Implement this interface to create new instances of Logger or
         * a sub-class of Logger.
         */
        class LOG4XTENSA_EXPORT LoggerFactory {
        public:
            /**
             * Creates a new <code>Logger</code> object.
             */
            virtual Logger& makeNewLoggerInstance(const log4xtensa::tstring& name,
                                                 Hierarchy& h) = 0; 
            virtual ~LoggerFactory(){}
        };

    } // end namespace spi
} // end namespace log4xtensa

#endif // _LOG4XTENSA_SPI_LOGGER_FACTORY_HEADER

