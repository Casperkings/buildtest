// Module:  Log4CPLUS
// File:    threads.h
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

#ifndef _LOG4XTENSA_THREADS_HEADER_
#define _LOG4XTENSA_THREADS_HEADER_

#include <log4xtensa/config.h>
#include <log4xtensa/tstring.h>
#include <log4xtensa/helpers/sleep.h>
#include <log4xtensa/helpers/pointer.h>
#include <memory>


namespace log4xtensa {
    namespace thread {

        /**
         * This is used to lock a mutex.  The dtor unlocks the mutex.
         */
        class LOG4XTENSA_EXPORT Guard {
        public:
            /** "locks" <code>mutex</code>. */
            Guard(LOG4XTENSA_MUTEX_PTR_DECLARE mutex) {
                LOG4XTENSA_MUTEX_ASSIGN( _mutex, mutex );
                LOG4XTENSA_MUTEX_LOCK( _mutex );
            }

            /** "unlocks" <code>mutex</code>. */
            ~Guard() { LOG4XTENSA_MUTEX_UNLOCK( _mutex ); }

        private:
            LOG4XTENSA_MUTEX_PTR_DECLARE _mutex;
 
            // disable copy
            Guard(const Guard&);
            Guard& operator=(const Guard&);
        };

#ifndef LOG4XTENSA_SINGLE_THREADED
#ifdef LOG4XTENSA_USE_PTHREADS
        void* threadStartFunc(void*);
#elif defined(LOG4XTENSA_USE_WIN32_THREADS)
        DWORD WINAPI threadStartFunc(LPVOID arg);
#endif

        LOG4XTENSA_EXPORT void yield();
        LOG4XTENSA_EXPORT log4xtensa::tstring getCurrentThreadName();

        /**
         * There are many cross-platform C++ Threading libraries.  The goal of
         * this class is not to replace (or match in functionality) those
         * libraries.  The goal of this class is to provide a simple Threading
         * class with basic functionality.
         */
        class LOG4XTENSA_EXPORT AbstractThread : public log4xtensa::helpers::SharedObject {
        public:
            AbstractThread();
            bool isRunning() { return running; }
            LOG4XTENSA_THREAD_KEY_TYPE getThreadId() { return threadId; }
            virtual void start();

        protected:
            // Force objects to be constructed on the heap
            virtual ~AbstractThread();
            virtual void run() = 0;

        private:
            bool running;
            LOG4XTENSA_THREAD_KEY_TYPE threadId;

            // Disallow copying of instances of this class
            AbstractThread(const AbstractThread&);
            AbstractThread& operator=(const AbstractThread&);

        // Friends
#ifdef LOG4XTENSA_USE_PTHREADS
        friend void* threadStartFunc(void*);
#elif defined(LOG4XTENSA_USE_WIN32_THREADS)
        friend DWORD WINAPI threadStartFunc(LPVOID arg);
#endif

        };
#endif // LOG4XTENSA_SINGLE_THREADED

    } // end namespace thread 
} // end namespace log4xtensa 


#endif // _LOG4XTENSA_THREADS_HEADER_

