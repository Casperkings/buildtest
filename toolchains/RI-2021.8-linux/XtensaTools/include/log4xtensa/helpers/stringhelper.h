// Module:  Log4CPLUS
// File:    stringhelper.h
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

#ifndef LOG4XTENSA_HELPERS_STRINGHELPER_HEADER_
#define LOG4XTENSA_HELPERS_STRINGHELPER_HEADER_

#include <log4xtensa/config.h>
#include <log4xtensa/tstring.h>

#include <algorithm>


namespace log4xtensa {
    namespace helpers {

        /**
         * Returns <code>s</code> in upper case.
         */
        LOG4XTENSA_EXPORT log4xtensa::tstring toUpper(const log4xtensa::tstring& s);


        /**
         * Returns <code>s</code> in lower case.
         */
        LOG4XTENSA_EXPORT log4xtensa::tstring toLower(const log4xtensa::tstring& s);


        /**
         * Tokenize <code>s</code> using <code>c</code> as the delimiter and
         * put the resulting tokens in <code>_result</code>.  If 
         * <code>collapseTokens</code> is false, multiple adjacent delimiters
         * will result in zero length tokens.
         * <p>
         * <b>Example:</b>
         * <pre>
         *   string s = // Set string with '.' as delimiters
         *   list<log4xtensa::tstring> tokens;
         *   tokenize(s, '.', back_insert_iterator<list<string> >(tokens));
         * </pre>
         */
        template <class _StringType, class _OutputIter>
        void tokenize(const _StringType& s, typename _StringType::value_type c, 
                      _OutputIter _result, bool collapseTokens = true) 
        {
            _StringType tmp;
            for(typename _StringType::size_type i=0; i<s.length(); ++i) {
                if(s[i] == c) {
                    *_result = tmp;
                    ++_result;
                    tmp.erase(tmp.begin(), tmp.end());
                    if(collapseTokens)
                        while(s[i+1] == c) ++i;
                }
                else
                    tmp += s[i];
            }
            if(tmp.length() > 0) *_result = tmp;
        }
        
        
         
        template<class intType>
        inline tstring convertIntegerToString(intType value) 
        {
            if(value == 0) {
                return LOG4XTENSA_TEXT("0");
            }
            
            char buffer[21];
            char ret[21];
            unsigned int bufferPos = 0;
            unsigned int retPos = 0;

            if(value < 0) {
                ret[retPos++] = '-';
            }
            
            // convert to string in reverse order
            while(value != 0) {
                intType mod = value % 10;
                value = value / 10;
                buffer[bufferPos++] = '0' + static_cast<char>(mod);
            }
            
            // now reverse the string to get it in proper order
            while(bufferPos > 0) {
                ret[retPos++] = buffer[--bufferPos];
            }
            ret[retPos] = 0;
            
            return LOG4XTENSA_C_STR_TO_TSTRING(ret);
        }
    } 
}

#endif // LOG4XTENSA_HELPERS_STRINGHELPER_HEADER_

