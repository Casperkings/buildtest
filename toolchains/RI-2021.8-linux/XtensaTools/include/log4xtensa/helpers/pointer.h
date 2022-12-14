// Module:  Log4CPLUS
// File:    pointer.h
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

// 2006.06.29.  Tensilica.  Get rid of the "using log4xtensa::helpers::safe_auto_ptr;" line to avoid 
//                          a conflict with someone using log4cplus.
// 2005.09.01.  Tensilica.  Global replace of log4cplus/LOG4CPLUS with log4xtensa/LOG4XTENSA
//                          to avoid potential conflicts with customer code independently 
//                          using log4cplus.


//
// Note: Some of this code uses ideas from "More Effective C++" by Scott
// Myers, Addison Wesley Longmain, Inc., (c) 1996, Chapter 29, pp. 183-213
//

/** @file */

#ifndef _LOG4XTENSA_HELPERS_POINTERS_HEADER_
#define _LOG4XTENSA_HELPERS_POINTERS_HEADER_

#include <log4xtensa/config.h>
#include <memory>
#include <stdexcept>
#include <string>


namespace log4xtensa {
    namespace helpers {

#if defined(_MSC_VER) && (_MSC_VER >= 1300)
        // Added to remove the following warning from MSVC++ 7:
        // warning C4275: non dll-interface class 'std::runtime_error' used as 
        //                base for dll-interface class 
        //                'log4xtensa::helpers::NullPointerException'
        class LOG4XTENSA_EXPORT std::runtime_error;
#endif

        class LOG4XTENSA_EXPORT NullPointerException : public std::runtime_error {
        public:
            NullPointerException(const std::string& what_arg) : std::runtime_error(what_arg) {}
        };

        void throwNullPointerException(const char* file, int line);


        /******************************************************************************
         *                       Class SharedObject (from pp. 204-205)                *
         ******************************************************************************/

        class LOG4XTENSA_EXPORT SharedObject {
        public:
            void addReference();
            void removeReference();

        protected:
          // Ctor
            SharedObject() 
             : access_mutex(LOG4XTENSA_MUTEX_CREATE), count(0), destroyed(false) {}
            SharedObject(const SharedObject&) 
             : access_mutex(LOG4XTENSA_MUTEX_CREATE), count(0), destroyed(false) {}

          // Dtor
            virtual ~SharedObject();

          // Operators
            SharedObject& operator=(const SharedObject&) { return *this; }

        public:
            LOG4XTENSA_MUTEX_PTR_DECLARE access_mutex;

        private:
            int count;
            bool destroyed;
        };


        /******************************************************************************
         *           Template Class SharedObjectPtr (from pp. 203, 206)               *
         ******************************************************************************/
        template<class T>
        class LOG4XTENSA_EXPORT SharedObjectPtr {
        public:
          // Ctor
            SharedObjectPtr(T* realPtr = 0) : pointee(realPtr) { init(); };
            SharedObjectPtr(const SharedObjectPtr& rhs) : pointee(rhs.pointee) { init(); };

          // Dtor
            ~SharedObjectPtr() {if (pointee != 0) ((SharedObject *)pointee)->removeReference(); }

          // Operators
            bool operator==(const SharedObjectPtr& rhs) const { return (pointee == rhs.pointee); }
            bool operator!=(const SharedObjectPtr& rhs) const { return (pointee != rhs.pointee); }
            bool operator==(const T* rhs) const { return (pointee == rhs); }
            bool operator!=(const T* rhs) const { return (pointee != rhs); }
            T* operator->() const {validate(); return pointee; }
            T& operator*() const {validate(); return *pointee; }
            SharedObjectPtr& operator=(const SharedObjectPtr& rhs) {
                if (pointee != rhs.pointee) {
                    T* oldPointee = pointee;
                    pointee = rhs.pointee;
                    init();
                    if(oldPointee != 0) oldPointee->removeReference();
                }
                return *this;
            }
            SharedObjectPtr& operator=(T* rhs) {
                if (pointee != rhs) {
                    T* oldPointee = pointee;
                    pointee = rhs;
                    init();
                    if(oldPointee != 0) oldPointee->removeReference();
                }
                return *this;
            }

          // Methods
            T* get() const { return pointee; }

        private:
          // Methods
            void init() {
                if(pointee == 0) return;
                ((SharedObject*)pointee)->addReference();
            }
            void validate() const {
                if(pointee == 0) throw std::runtime_error("NullPointer");
            }

          // Data
            T* pointee;
        };

    } // end namespace helpers
} // end namespace log4xtensa

#endif // _LOG4XTENSA_HELPERS_POINTERS_HEADER_

