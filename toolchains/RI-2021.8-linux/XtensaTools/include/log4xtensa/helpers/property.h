// Module:  Log4CPLUS
// File:    property.h
// Created: 2/2002
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

#ifndef LOG4XTENSA_HELPERS_PROPERTY_HEADER_
#define LOG4XTENSA_HELPERS_PROPERTY_HEADER_

#include <log4xtensa/config.h>
#include <log4xtensa/streams.h>
#include <log4xtensa/tstring.h>
#include <map>
#include <vector>

#if (defined(__MWERKS__) && defined(__MACOS__))
using std::size_t;
#endif


namespace log4xtensa {
    namespace helpers {

        class LOG4XTENSA_EXPORT Properties {
        public:
            Properties();
            explicit Properties(log4xtensa::tistream& input);
            explicit Properties(const log4xtensa::tstring& inputFile);
            virtual ~Properties();

          // constants
            static const tchar PROPERTIES_COMMENT_CHAR;

          // methods
            /**
             * Tests to see if <code>key</code> can be found in this map.
             */
            bool exists(const log4xtensa::tstring& key) const {
                return data.find(key) != data.end();
            }


            /**
             * Returns the number of entries in this map.
             */
            size_t size() const {
                return data.size();
            }

            /**
             * Searches for the property with the specified key in this property
             * list. If the key is not found in this property list, the default
             * property list, and its defaults, recursively, are then checked. 
             * The method returns <code>null</code> if the property is not found.
             */
            log4xtensa::tstring getProperty(const log4xtensa::tstring& key) const;

            /**
             * Searches for the property with the specified key in this property
             * list. If the key is not found in this property list, the default
             * property list, and its defaults, recursively, are then checked. 
             * The method returns the default value argument if the property is 
             * not found.
             */
            log4xtensa::tstring getProperty(const log4xtensa::tstring& key,
                                           const log4xtensa::tstring& defaultVal) const;

            /**
             * Returns all the keys in this property list.
             */
            std::vector<log4xtensa::tstring> propertyNames() const;

            /**
             * Inserts <code>value</code> into this map indexed by <code>key</code>.
             */
            void setProperty(const log4xtensa::tstring& key, const log4xtensa::tstring& value);

            /**
             * Removed the property index by <code>key</code> from this map.
             */
            bool removeProperty(const log4xtensa::tstring& key);

            /**
             * Returns a subset of the "properties" whose keys start with
             * "prefix".  The returned "properties" have "prefix" trimmed from
             * their keys.
             */
            Properties getPropertySubset(const log4xtensa::tstring& prefix) const;

        protected:
          // Types
//            LOG4XTENSA_EXPIMP_TEMPLATE template class LOG4XTENSA_EXPORT std::map<log4xtensa::tstring, log4xtensa::tstring>;
            typedef std::map<log4xtensa::tstring, log4xtensa::tstring> StringMap;

          // Methods
            void init(log4xtensa::tistream& input);

          // Data
            StringMap data;
        };
    } // end namespace helpers

}


#endif // LOG4XTENSA_HELPERS_PROPERTY_HEADER_

