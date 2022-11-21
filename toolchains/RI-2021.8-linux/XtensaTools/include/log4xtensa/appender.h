// Module:  Log4CPLUS
// File:    appender.h
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

#ifndef _LOG4XTENSA_APPENDER_HEADER_
#define _LOG4XTENSA_APPENDER_HEADER_

#include <log4xtensa/config.h>
#include <log4xtensa/layout.h>
#include <log4xtensa/loglevel.h>
#include <log4xtensa/tstring.h>
#include <log4xtensa/helpers/logloguser.h>
#include <log4xtensa/helpers/pointer.h>
#include <log4xtensa/helpers/property.h>
#include <log4xtensa/spi/filter.h>

#include <memory>


namespace log4xtensa {

    /**
     * This class is used to "handle" errors encountered in an {@link
     * #Appender}.
     */
    class LOG4XTENSA_EXPORT ErrorHandler {
    public:
        virtual ~ErrorHandler();
        virtual void error(const log4xtensa::tstring& err) = 0;
    };



    class LOG4XTENSA_EXPORT OnlyOnceErrorHandler : public ErrorHandler,
                                                  protected log4xtensa::helpers::LogLogUser
    {
    public:
      // Ctor
        OnlyOnceErrorHandler() : firstTime(true){}

        virtual void error(const log4xtensa::tstring& err);

    private:
        bool firstTime;
    };


    /**
     * Extend this class for implementing your own strategies for printing log
     * statements.
     */
    class LOG4XTENSA_EXPORT Appender : public log4xtensa::helpers::SharedObject,
                                      protected log4xtensa::helpers::LogLogUser

    {
    public:
      // Ctor
        Appender();
        Appender(const log4xtensa::helpers::Properties properties);

      // Dtor
        virtual ~Appender(){};

        void destructorImpl();

      // Methods
        /**
         * Release any resources allocated within the appender such as file
         * handles, network connections, etc.
         * <p>
         * It is a programming error to append to a closed appender.
         */
        virtual void close() = 0;

        /**
         * This method performs threshold checks and invokes filters before
         * delegating actual logging to the subclasses specific {@link
         * AppenderSkeleton#append} method.
         */
        void doAppend(const log4xtensa::spi::InternalLoggingEvent& event);

        /**
         * Get the name of this appender. The name uniquely identifies the
         * appender.
         */
        virtual log4xtensa::tstring getName();

        /**
         * Set the name of this appender. The name is used by other
         * components to identify this appender.
         */
        virtual void setName(const log4xtensa::tstring& name);

        /**
         * Set the {@link ErrorHandler} for this Appender.
         */
        virtual void setErrorHandler(std::unique_ptr<ErrorHandler> eh);

        /**
         * Return the currently set {@link ErrorHandler} for this
         * Appender.
         */
        virtual ErrorHandler* getErrorHandler();

        /**
         * Set the layout for this appender. Note that some appenders have
         * their own (fixed) layouts or do not use one. For example, the
         * SocketAppender ignores the layout set here.
         */
        virtual void setLayout(std::unique_ptr<Layout> layout);

        /**
         * Returns the layout of this appender. The value may be NULL.
         * <p>
         * This class owns the returned pointer.
         */
        virtual Layout* getLayout();

        /**
         * Set the filter chain on this Appender.
         */
        void setFilter(log4xtensa::spi::FilterPtr f) { filter = f; }

        /**
         * Get the filter chain on this Appender.
         */
        log4xtensa::spi::FilterPtr getFilter() const { return filter; }

        /**
         * Returns this appenders threshold LogLevel. See the {@link
         * #setThreshold} method for the meaning of this option.
         */
        LogLevel getThreshold() const { return threshold; }

        /**
         * Set the threshold LogLevel. All log events with lower LogLevel
         * than the threshold LogLevel are ignored by the appender.
         * <p>
         * In configuration files this option is specified by setting the
         * value of the <b>Threshold</b> option to a LogLevel
         * string, such as "DEBUG", "INFO" and so on.
         */
        void setThreshold(LogLevel th) { threshold = th; }

        /**
         * Check whether the message LogLevel is below the appender's
         * threshold. If there is no threshold set, then the return value is
         * always <code>true</code>.
         */
        bool isAsSevereAsThreshold(LogLevel ll) const {
            return ((ll != NOT_SET_LOG_LEVEL) && (ll >= threshold));
        }

    protected:
      // Methods
        /**
         * Subclasses of <code>AppenderSkeleton</code> should implement this
         * method to perform actual logging. See also {@link #doAppend
         * AppenderSkeleton.doAppend} method.
         */
        virtual void append(const log4xtensa::spi::InternalLoggingEvent& event) = 0;

      // Data
        /** The layout variable does not need to be set if the appender
         *  implementation has its own layout. */
        std::unique_ptr<Layout> layout;

        /** Appenders are named. */
        log4xtensa::tstring name;

        /** There is no LogLevel threshold filtering by default.  */
        LogLevel threshold;

        /** The first filter in the filter chain. Set to <code>null</code>
         *  initially. */
        log4xtensa::spi::FilterPtr filter;

        /** It is assumed and enforced that errorHandler is never null. */
        std::unique_ptr<ErrorHandler> errorHandler;

        /** Is this appender closed? */
        bool closed;
    };

    /** @var This is a pointer to an Appender. */
    typedef helpers::SharedObjectPtr<Appender> SharedAppenderPtr;

} // end namespace log4xtensa

#endif // _LOG4XTENSA_APPENDER_HEADER_

