// Module:  Log4CPLUS
// File:    loggerimpl.h
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

// 2005.09.02.  Tensilica.  Add friend BinaryLoggerFactory to LoggerImpl.
// 2005.09.01.  Tensilica.  Global replace of log4cplus/LOG4CPLUS with log4xtensa/LOG4XTENSA
//                          to avoid potential conflicts with customer code independently 
//                          using log4cplus.


/** @file */

#ifndef _LOG4XTENSA_SPI_LOGGER_HEADER_
#define _LOG4XTENSA_SPI_LOGGER_HEADER_

#include <log4xtensa/config.h>
#include <log4xtensa/logger.h>
#include <log4xtensa/tstring.h>
#include <log4xtensa/helpers/appenderattachableimpl.h>
#include <log4xtensa/helpers/pointer.h>
#include <log4xtensa/spi/loggerfactory.h>
#include <memory>
#include <vector>


namespace log4xtensa {
    class BinaryLoggerFactory;
    namespace spi {

        /**
         * This is the central class in the log4xtensa package. One of the
         * distintive features of log4xtensa are hierarchical loggers and their
         * evaluation.
         *
         * <p>See the <a href="../../../../manual.html">user manual</a> for an
         * introduction on this class. 
         */
        class LOG4XTENSA_EXPORT LoggerImpl : public log4xtensa::helpers::SharedObject,
                                            public log4xtensa::helpers::AppenderAttachableImpl
        {
        public:
          // Methods

            /**
             * Call the appenders in the hierrachy starting at
             * <code>this</code>.  If no appenders could be found, emit a
             * warning.
             * <p>
             * This method calls all the appenders inherited from the
             * hierarchy circumventing any evaluation of whether to log or not
             * to log the particular log request.
             *                                   
             * @param spi::InternalLoggingEvent the event to log. 
             */
            virtual void callAppenders(const InternalLoggingEvent& event);

            /**
             * Close all attached appenders implementing the AppenderAttachable
             * interface.  
             */
            virtual void closeNestedAppenders();

            /**
             * Check whether this logger is enabled for a given LogLevel passed 
             * as parameter.
             *
             * @return boolean True if this logger is enabled for <code>ll</code>.
             */
            virtual bool isEnabledFor(LogLevel ll) const;

            /**
             * This generic form is intended to be used by wrappers. 
             */
            virtual void log(LogLevel ll, const log4xtensa::tstring& message,
                             const char* file=NULL, int line=-1);

            /**
             * Starting from this logger, search the logger hierarchy for a
             * "set" LogLevel and return it. Otherwise, return the LogLevel of the
             * root logger.
             *                     
             * <p>The Logger class is designed so that this method executes as
             * quickly as possible.
             */
            virtual LogLevel getChainedLogLevel() const;

            /**
             * Returns the assigned LogLevel, if any, for this Logger.  
             *           
             * @return LogLevel - the assigned LogLevel.
             */
            LogLevel getLogLevel() const { return this->ll; }

            /**
             * Set the LogLevel of this Logger.
             */
            void setLogLevel(LogLevel _ll) { this->ll = _ll; }

            /**
             * Return the the {@link Hierarchy} where this <code>Logger</code>
             * instance is attached.
             */
            virtual Hierarchy& getHierarchy() const;

            /**
             * Return the logger name.  
             */
            log4xtensa::tstring getName() const { return name; }

            /**
             * Get the additivity flag for this Logger instance.
             */
            bool getAdditivity() const;

            /**
             * Set the additivity flag for this Logger instance.
             */
            void setAdditivity(bool additive);

            virtual ~LoggerImpl();

        protected:
          // Ctors
            /**
             * This constructor created a new <code>Logger</code> instance and
             * sets its name.
             *
             * <p>It is intended to be used by sub-classes only. You should not
             * create loggers directly.
             *
             * @param name The name of the logger.  
             */
            LoggerImpl(const log4xtensa::tstring& name, Hierarchy& h);


          // Methods
            /**
             * This method creates a new logging event and logs the event
             * without further checks.  
             */
            virtual void forcedLog(LogLevel ll,
                                   const log4xtensa::tstring& message,
                                   const char* file=NULL, 
                                   int line=-1);


          // Data
            /** The name of this logger */
            log4xtensa::tstring name;

            /**
             * The assigned LogLevel of this logger.
             */
            LogLevel ll;

            /**
             * The parent of this logger. All loggers have at least one
             * ancestor which is the root logger. 
             */ 
            SharedLoggerImplPtr parent;

            /** 
             * Additivity is set to true by default, that is children inherit
             * the appenders of their ancestors by default. If this variable is
             * set to <code>false</code> then the appenders found in the
             * ancestors of this logger are not used. However, the children
             * of this logger will inherit its appenders, unless the children
             * have their additivity flag set to <code>false</code> too. See
             * the user manual for more details. 
             */
            bool additive;

        private:
          // Data
            /** Loggers need to know what Hierarchy they are in. */
            Hierarchy& hierarchy;

          // Disallow copying of instances of this class
            LoggerImpl(const LoggerImpl&);
            LoggerImpl& operator=(const LoggerImpl&);

          // Friends
            friend class log4xtensa::Logger;
            friend class log4xtensa::DefaultLoggerFactory;
            friend class log4xtensa::BinaryLoggerFactory;
            friend class log4xtensa::Hierarchy;
        };

    } // end namespace spi
} // end namespace log4xtensa

#endif // _LOG4XTENSA_SPI_LOGGER_HEADER_


