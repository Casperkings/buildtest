// Module:  Log4CPLUS
// File:    objectregistry.h
// Created: 3/2003
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

#ifndef LOG4XTENSA_SPI_OBJECT_REGISTRY_HEADER_
#define LOG4XTENSA_SPI_OBJECT_REGISTRY_HEADER_

#include <log4xtensa/config.h>
#include <log4xtensa/tstring.h>
#include <log4xtensa/helpers/threads.h>
#include <map>
#include <memory>
#include <vector>


namespace log4xtensa {
    namespace spi {

        /**
         * This is the base class used to implement the functionality required
         * by the ObjectRegistry template class.
         */
        class LOG4XTENSA_EXPORT ObjectRegistryBase {
        public:
          // public methods
            /**
             * Tests to see whether or not an object is bound in the
             * registry as <code>name</code>.
             */
            bool exists(const log4xtensa::tstring& name) const;

            /**
             * Returns the names of all registered objects.
             */
            std::vector<log4xtensa::tstring> getAllNames() const;

        protected:
          // Ctor and Dtor
            ObjectRegistryBase();
            virtual ~ObjectRegistryBase();

          // protected methods
            /**
             * Used to enter an object into the registry.  (The registry now
             * owns <code>object</code>.)
             */
            bool putVal(const log4xtensa::tstring& name, void* object);

            /**
             * Used to retrieve an object from the registry.  (The registry
             * owns the returned pointer.)
             */
            void* getVal(const log4xtensa::tstring& name) const;

            /**
             * Deletes <code>object</code>.
             */
            virtual void deleteObject(void *object) const = 0;

            /**
             * Deletes all objects from this registry.
             */
            virtual void clear();

          // Types
            typedef std::map<log4xtensa::tstring, void*> ObjectMap;

          // Data
            LOG4XTENSA_MUTEX_PTR_DECLARE mutex;
            ObjectMap data;
        };

    }
}


#endif // LOG4XTENSA_SPI_OBJECT_REGISTRY_HEADER_

