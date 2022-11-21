// Module:  Log4CPLUS
// File:    loglog.h
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

#ifndef _LOG4XTENSA_HELPERS_LOGLOG
#define _LOG4XTENSA_HELPERS_LOGLOG

#include <log4xtensa/config.h>
#include <log4xtensa/tstring.h>
#include <log4xtensa/helpers/pointer.h>
#include <log4xtensa/helpers/threads.h>


namespace log4xtensa {
    namespace helpers {

        /**
         * This class used to output log statements from within the log4xtensa package.
         *
         * <p>Log4cplus components cannot make log4xtensa logging calls. However, it is
         * sometimes useful for the user to learn about what log4xtensa is
         * doing. You can enable log4xtensa internal logging by defining the
         * <b>log4xtensa.configDebug</b> variable.
         *
         * <p>All log4xtensa internal debug calls go to <code>cout</code>
         * where as internal error messages are sent to
         * <code>cerr</code>. All internal messages are prepended with
         * the string "log4clus: ".
         */
        class LOG4XTENSA_EXPORT LogLog : public log4xtensa::helpers::SharedObject {
        public:
          // Static methods
            /**
             * Returns a reference to the <code>LogLog</code> singleton.
             */
            static log4xtensa::helpers::SharedObjectPtr<LogLog> getLogLog();


            /**
             * Allows to enable/disable log4xtensa internal logging.
             */
            void setInternalDebugging(bool enabled);

            /**
             * In quite mode no LogLog generates strictly no output, not even
             * for errors. 
             *
             * @param quietMode A true for not
             */
            void setQuietMode(bool quietMode);

            /**
             * This method is used to output log4xtensa internal debug
             * statements. Output goes to <code>std::cout</code>.
             */
            void debug(const log4xtensa::tstring& msg);

            /**
             * This method is used to output log4xtensa internal error
             * statements. There is no way to disable error statements.
             * Output goes to <code>std::cerr</code>.
             */
            void error(const log4xtensa::tstring& msg);

            /**
             * This method is used to output log4xtensa internal warning
             * statements. There is no way to disable warning statements.
             * Output goes to <code>std::cerr</code>.
             */
            void warn(const log4xtensa::tstring& msg);

          // Dtor
            virtual ~LogLog();

          // Data
            LOG4XTENSA_MUTEX_PTR_DECLARE mutex;

        private:
          // Data
            bool debugEnabled;
            bool quietMode;
            const log4xtensa::tstring PREFIX;
            const log4xtensa::tstring WARN_PREFIX;
            const log4xtensa::tstring ERR_PREFIX;

          // Ctors
            LogLog();
            LogLog(const LogLog&);
        };

    } // end namespace helpers
} // end namespace log4xtensa


#endif // _LOG4XTENSA_HELPERS_LOGLOG

