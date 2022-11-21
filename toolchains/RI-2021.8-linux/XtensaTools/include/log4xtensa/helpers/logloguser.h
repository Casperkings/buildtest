// Module:  Log4CPLUS
// File:    logloguser.h
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

#ifndef _LOG4XTENSA_HELPERS_LOGLOG_USER
#define _LOG4XTENSA_HELPERS_LOGLOG_USER

#include <log4xtensa/config.h>


namespace log4xtensa {
    namespace helpers {
        // forward declarations
        class LogLog;

        /**
         * This class used to simplify the use of the LogLog class.  Any class
         * that uses the LogLog class should extend this class and retrieve
         * their reference to LogLog using the method provided.
         */
        class LOG4XTENSA_EXPORT LogLogUser {
        public:
          // ctor and dtor
            LogLogUser();
            LogLogUser(const LogLogUser&);
            virtual ~LogLogUser();

          // public methods
            LogLog& getLogLog() const;
            
          // operators
            LogLogUser& operator=(const LogLogUser& rhs);

        private:
          // Data
            void* loglogRef;
        };

    } // end namespace helpers
} // end namespace log4xtensa


#endif // _LOG4XTENSA_HELPERS_LOGLOG_USER

