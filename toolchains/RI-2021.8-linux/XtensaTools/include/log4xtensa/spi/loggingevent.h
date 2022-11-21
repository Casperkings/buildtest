// Module:  Log4CPLUS
// File:    loggingevent.h
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

#ifndef _LOG4XTENSA_SPI_INTERNAL_LOGGING_EVENT_HEADER_
#define _LOG4XTENSA_SPI_INTERNAL_LOGGING_EVENT_HEADER_

#include <log4xtensa/config.h>
#include <log4xtensa/loglevel.h>
#include <log4xtensa/ndc.h>
#include <log4xtensa/tstring.h>
#include <log4xtensa/helpers/timehelper.h>
#include <log4xtensa/helpers/threads.h>

namespace log4xtensa {
    namespace spi {
        /**
         * The internal representation of logging events. When an affirmative
         * decision is made to log then a <code>InternalLoggingEvent</code> 
         * instance is created. This instance is passed around to the 
         * different log4xtensa components.
         *
         * <p>This class is of concern to those wishing to extend log4xtensa. 
         */
        class LOG4XTENSA_EXPORT InternalLoggingEvent {
        public:
          // Ctors
             /**
              * Instantiate a LoggingEvent from the supplied parameters.
              * <p>
              * @param logger The logger of this event.
              * @param ll       The LogLevel of this event.
              * @param message  The message of this event.
              */
             InternalLoggingEvent(const log4xtensa::tstring& logger,
                                  LogLevel ll,
                                  const log4xtensa::tstring& message,
                                  const char* filename,
                                  int line)
              : message(message),
                loggerName(logger),
                ll(ll),
                ndc(),
                thread(),
                timestamp(log4xtensa::helpers::Time::gettimeofday()),
                file( (  filename
                       ? LOG4XTENSA_C_STR_TO_TSTRING(filename) 
                       : log4xtensa::tstring()) ),
                line(line),
                threadCached(false),
                ndcCached(false)
             {
             }

             InternalLoggingEvent(const log4xtensa::tstring& logger,
                                  LogLevel ll,
                                  const log4xtensa::tstring& ndc,
                                  const log4xtensa::tstring& message,
                                  const log4xtensa::tstring& thread,
                                  log4xtensa::helpers::Time time,
                                  const log4xtensa::tstring& file,
                                  int line)
              : message(message),
                loggerName(logger),
                ll(ll),
                ndc(ndc),
                thread(thread),
                timestamp(time),
                file(file),
                line(line),
                threadCached(true),
                ndcCached(true)
             {
             }

             InternalLoggingEvent(const log4xtensa::spi::InternalLoggingEvent& rhs)
              : message(rhs.getMessage()),
                loggerName(rhs.getLoggerName()),
                ll(rhs.getLogLevel()),
                ndc(rhs.getNDC()),
                thread(rhs.getThread()),
                timestamp(rhs.getTimestamp()),
                file(rhs.getFile()),
                line(rhs.getLine()),
                threadCached(true),
                ndcCached(true)
             {
             }

            virtual ~InternalLoggingEvent();


          // public virtual methods
            /** The application supplied message of logging event. */
            virtual const log4xtensa::tstring& getMessage() const;

            /** Returns the 'type' of InternalLoggingEvent.  Derived classes
             *  should override this method.  (NOTE: Values <= 1000 are
             *  reserved for log4xtensa and should not be used.)
             */
            virtual unsigned int getType() const;

           /** Returns a copy of this object.  Derived classes
             *  should override this method.
	     */
            virtual std::unique_ptr<InternalLoggingEvent> clone() const;



          // public methods
            /** The logger of the logging event. It is set by 
             *  the LoggingEvent constructor. 
	     */
            const log4xtensa::tstring& getLoggerName() const { return loggerName; }

            /** LogLevel of logging event. */
            LogLevel getLogLevel() const { return ll; }

            /** The nested diagnostic context (NDC) of logging event. */
            const log4xtensa::tstring& getNDC() const { 
                if(!ndcCached) {
                    ndc = log4xtensa::getNDC().get();
                    ndcCached = true;
                }
                return ndc; 
            }

            /** The name of thread in which this logging event was generated. */
            const log4xtensa::tstring& getThread() const {
                if(!threadCached) {
                    thread = LOG4XTENSA_GET_CURRENT_THREAD_NAME;
                    threadCached = true;
                }
                return thread; 
            }

            /** The number of milliseconds elapsed from 1/1/1970 until logging event
             *  was created. */
            const log4xtensa::helpers::Time& getTimestamp() const { return timestamp; }

            /** The is the file where this log statement was written */
            const log4xtensa::tstring& getFile() const { return file; }

            /** The is the line where this log statement was written */
            int getLine() const { return line; }
 
          // public operators
            log4xtensa::spi::InternalLoggingEvent&
            operator=(const log4xtensa::spi::InternalLoggingEvent& rhs);

          // static methods
            static unsigned int getDefaultType();

        protected:
          // Data
            log4xtensa::tstring message;

        private:
            log4xtensa::tstring loggerName;
            LogLevel ll;
            mutable log4xtensa::tstring ndc;
            mutable log4xtensa::tstring thread;
            log4xtensa::helpers::Time timestamp;
            log4xtensa::tstring file;
            int line;
            /** Indicates whether or not the Threadname has been retrieved. */
            mutable bool threadCached;
            /** Indicates whether or not the NDC has been retrieved. */
            mutable bool ndcCached;
        };

    } // end namespace spi
} // end namespace log4xtensa

#endif // _LOG4XTENSA_SPI_INTERNAL_LOGGING_EVENT_HEADER_

