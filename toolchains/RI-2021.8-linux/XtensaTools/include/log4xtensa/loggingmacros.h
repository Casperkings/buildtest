// Module:  Log4CPLUS
// File:    loggingmacros.h
// Created: 8/2003
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


/** @file 
 * This header defines the logging macros. */

#ifndef _LOG4XTENSA_LOGGING_MACROS_HEADER_
#define _LOG4XTENSA_LOGGING_MACROS_HEADER_

#if defined(LOG4XTENSA_DISABLE_FATAL) && !defined(LOG4XTENSA_DISABLE_ERROR)
#define LOG4XTENSA_DISABLE_ERROR
#endif
#if defined(LOG4XTENSA_DISABLE_ERROR) && !defined(LOG4XTENSA_DISABLE_WARN)
#define LOG4XTENSA_DISABLE_WARN
#endif
#if defined(LOG4XTENSA_DISABLE_WARN) && !defined(LOG4XTENSA_DISABLE_NOTE)
#define LOG4XTENSA_DISABLE_NOTE
#endif
#if defined(LOG4XTENSA_DISABLE_NOTE) && !defined(LOG4XTENSA_DISABLE_INFO)
#define LOG4XTENSA_DISABLE_INFO
#endif
#if defined(LOG4XTENSA_DISABLE_INFO) && !defined(LOG4XTENSA_DISABLE_VERBOSE)
#define LOG4XTENSA_DISABLE_VERBOSE
#endif
#if defined(LOG4XTENSA_DISABLE_VERBOSE) && !defined(LOG4XTENSA_DISABLE_DEBUG)
#define LOG4XTENSA_DISABLE_DEBUG
#endif
#if defined(LOG4XTENSA_DISABLE_DEBUG) && !defined(LOG4XTENSA_DISABLE_TRACE)
#define LOG4XTENSA_DISABLE_TRACE
#endif


/**
 * @def LOG4XTENSA_TRACE(logger, logEvent)  This macro creates a TraceLogger 
 * to log a TRACE_LOG_LEVEL message to <code>logger</code> upon entry and
 * exiting of a method.  
 * <code>logEvent</code> will be streamed into an <code>ostream</code>.
 */
#if !defined(LOG4XTENSA_DISABLE_TRACE)
#define LOG4XTENSA_TRACE_METHOD(logger, logEvent) \
    do { log4xtensa::TraceLogger _log4xtensa_trace_logger(logger, logEvent, __FILE__, __LINE__); } while(0)
#define LOG4XTENSA_TRACE(logger, logEvent) \
    do { \
        if(logger.isEnabledFor(log4xtensa::TRACE_LOG_LEVEL)) { \
            log4xtensa::tostringstream _log4xtensa_buf; \
            _log4xtensa_buf << logEvent; \
            logger.forcedLog(log4xtensa::TRACE_LOG_LEVEL, _log4xtensa_buf.str(), __FILE__, __LINE__); \
        } \
    } while(0)
#define LOG4XTENSA_TRACE_STR(logger, logEvent) \
    do { \
        if(logger.isEnabledFor(log4xtensa::TRACE_LOG_LEVEL)) { \
            logger.forcedLog(log4xtensa::TRACE_LOG_LEVEL, logEvent, __FILE__, __LINE__); \
        } \
    } while(0)
#else
#define LOG4XTENSA_TRACE_METHOD(logger, logEvent)       
#define LOG4XTENSA_TRACE(logger, logEvent)              
#define LOG4XTENSA_TRACE_STR(logger, logEvent)          
#endif

/**
 * @def LOG4XTENSA_DEBUG(logger, logEvent)  This macro is used to log a
 * DEBUG_LOG_LEVEL message to <code>logger</code>.  
 * <code>logEvent</code> will be streamed into an <code>ostream</code>.
 */
#if !defined(LOG4XTENSA_DISABLE_DEBUG)
#define LOG4XTENSA_DEBUG(logger, logEvent) \
    do { \
        if(logger.isEnabledFor(log4xtensa::DEBUG_LOG_LEVEL)) { \
            log4xtensa::tostringstream _log4xtensa_buf; \
            _log4xtensa_buf << logEvent; \
            logger.forcedLog(log4xtensa::DEBUG_LOG_LEVEL, _log4xtensa_buf.str(), __FILE__, __LINE__); \
        } \
    } while(0)
#define LOG4XTENSA_DEBUG_STR(logger, logEvent) \
    do { \
        if(logger.isEnabledFor(log4xtensa::DEBUG_LOG_LEVEL)) { \
            logger.forcedLog(log4xtensa::DEBUG_LOG_LEVEL, logEvent, __FILE__, __LINE__); \
        } \
    } while(0)
#else
#define LOG4XTENSA_DEBUG(logger, logEvent)      
#define LOG4XTENSA_DEBUG_STR(logger, logEvent)  
#endif

/**
 * @def LOG4XTENSA_VERBOSE(logger, logEvent)  This macro is used to log a
 * VERBOSE_LOG_LEVEL message to <code>logger</code>.  
 * <code>logEvent</code> will be streamed into an <code>ostream</code>.
 */
#if !defined(LOG4XTENSA_DISABLE_VERBOSE)
#define LOG4XTENSA_VERBOSE(logger, logEvent) \
    do { \
        if(logger.isEnabledFor(log4xtensa::VERBOSE_LOG_LEVEL)) { \
            log4xtensa::tostringstream _log4xtensa_buf; \
            _log4xtensa_buf << logEvent; \
            logger.forcedLog(log4xtensa::VERBOSE_LOG_LEVEL, _log4xtensa_buf.str(), __FILE__, __LINE__); \
        } \
    } while(0)
#define LOG4XTENSA_VERBOSE_STR(logger, logEvent) \
    do { \
        if(logger.isEnabledFor(log4xtensa::VERBOSE_LOG_LEVEL)) { \
            logger.forcedLog(log4xtensa::VERBOSE_LOG_LEVEL, logEvent, __FILE__, __LINE__); \
        } \
    } while(0)
#else
#define LOG4XTENSA_VERBOSE(logger, logEvent)            
#define LOG4XTENSA_VERBOSE_STR(logger, logEvent)        
#endif

/**
 * @def LOG4XTENSA_INFO(logger, logEvent)  This macro is used to log a
 * INFO_LOG_LEVEL message to <code>logger</code>.  
 * <code>logEvent</code> will be streamed into an <code>ostream</code>.
 */
#if !defined(LOG4XTENSA_DISABLE_INFO)
#define LOG4XTENSA_INFO(logger, logEvent) \
    do { \
        if(logger.isEnabledFor(log4xtensa::INFO_LOG_LEVEL)) { \
            log4xtensa::tostringstream _log4xtensa_buf; \
            _log4xtensa_buf << logEvent; \
            logger.forcedLog(log4xtensa::INFO_LOG_LEVEL, _log4xtensa_buf.str(), __FILE__, __LINE__); \
        } \
    } while(0)
#define LOG4XTENSA_INFO_STR(logger, logEvent) \
    do { \
        if(logger.isEnabledFor(log4xtensa::INFO_LOG_LEVEL)) { \
            logger.forcedLog(log4xtensa::INFO_LOG_LEVEL, logEvent, __FILE__, __LINE__); \
        } \
    } while(0)
#else
#define LOG4XTENSA_INFO(logger, logEvent)       
#define LOG4XTENSA_INFO_STR(logger, logEvent)   
#endif

/**
 * @def LOG4XTENSA_NOTE(logger, logEvent)  This macro is used to log a
 * NOTE_LOG_LEVEL message to <code>logger</code>.  
 * <code>logEvent</code> will be streamed into an <code>ostream</code>.
 */
#if !defined(LOG4XTENSA_DISABLE_NOTE)
#define LOG4XTENSA_NOTE(logger, logEvent) \
    do { \
        if(logger.isEnabledFor(log4xtensa::NOTE_LOG_LEVEL)) { \
            log4xtensa::tostringstream _log4xtensa_buf; \
            _log4xtensa_buf << logEvent; \
            logger.forcedLog(log4xtensa::NOTE_LOG_LEVEL, _log4xtensa_buf.str(), __FILE__, __LINE__); \
        } \
    } while(0)
#define LOG4XTENSA_NOTE_STR(logger, logEvent) \
    do { \
        if(logger.isEnabledFor(log4xtensa::NOTE_LOG_LEVEL)) { \
            logger.forcedLog(log4xtensa::NOTE_LOG_LEVEL, logEvent, __FILE__, __LINE__); \
        } \
    } while(0)
#else
#define LOG4XTENSA_NOTE(logger, logEvent)       
#define LOG4XTENSA_NOTE_STR(logger, logEvent)   
#endif

/**
 * @def LOG4XTENSA_WARN(logger, logEvent)  This macro is used to log a
 * WARN_LOG_LEVEL message to <code>logger</code>.  
 * <code>logEvent</code> will be streamed into an <code>ostream</code>.
 */
#if !defined(LOG4XTENSA_DISABLE_WARN)
#define LOG4XTENSA_WARN(logger, logEvent) \
    do { \
        if(logger.isEnabledFor(log4xtensa::WARN_LOG_LEVEL)) { \
            log4xtensa::tostringstream _log4xtensa_buf; \
            _log4xtensa_buf << logEvent; \
            logger.forcedLog(log4xtensa::WARN_LOG_LEVEL, _log4xtensa_buf.str(), __FILE__, __LINE__); \
        } \
    } while(0)
#define LOG4XTENSA_WARN_STR(logger, logEvent) \
    do { \
        if(logger.isEnabledFor(log4xtensa::WARN_LOG_LEVEL)) { \
            logger.forcedLog(log4xtensa::WARN_LOG_LEVEL, logEvent, __FILE__, __LINE__); \
        } \
    } while(0)
#else
#define LOG4XTENSA_WARN(logger, logEvent)       
#define LOG4XTENSA_WARN_STR(logger, logEvent)   
#endif

/**
 * @def LOG4XTENSA_ERROR(logger, logEvent)  This macro is used to log a
 * ERROR_LOG_LEVEL message to <code>logger</code>.  
 * <code>logEvent</code> will be streamed into an <code>ostream</code>.
 */
#if !defined(LOG4XTENSA_DISABLE_ERROR)
#define LOG4XTENSA_ERROR(logger, logEvent) \
    do { \
        if(logger.isEnabledFor(log4xtensa::ERROR_LOG_LEVEL)) { \
            log4xtensa::tostringstream _log4xtensa_buf; \
            _log4xtensa_buf << logEvent; \
            logger.forcedLog(log4xtensa::ERROR_LOG_LEVEL, _log4xtensa_buf.str(), __FILE__, __LINE__); \
        } \
    } while(0)
#define LOG4XTENSA_ERROR_STR(logger, logEvent) \
    do { \
        if(logger.isEnabledFor(log4xtensa::ERROR_LOG_LEVEL)) { \
            logger.forcedLog(log4xtensa::ERROR_LOG_LEVEL, logEvent, __FILE__, __LINE__); \
        } \
    } while(0)
#else
#define LOG4XTENSA_ERROR(logger, logEvent)      
#define LOG4XTENSA_ERROR_STR(logger, logEvent)  
#endif

/**
 * @def LOG4XTENSA_FATAL(logger, logEvent)  This macro is used to log a
 * FATAL_LOG_LEVEL message to <code>logger</code>.  
 * <code>logEvent</code> will be streamed into an <code>ostream</code>.
 */
#if !defined(LOG4XTENSA_DISABLE_FATAL)
#define LOG4XTENSA_FATAL(logger, logEvent) \
    do { \
        if(logger.isEnabledFor(log4xtensa::FATAL_LOG_LEVEL)) { \
            log4xtensa::tostringstream _log4xtensa_buf; \
            _log4xtensa_buf << logEvent; \
            logger.forcedLog(log4xtensa::FATAL_LOG_LEVEL, _log4xtensa_buf.str(), __FILE__, __LINE__); \
        } \
    } while(0)
#define LOG4XTENSA_FATAL_STR(logger, logEvent) \
    do { \
        if(logger.isEnabledFor(log4xtensa::FATAL_LOG_LEVEL)) { \
            logger.forcedLog(log4xtensa::FATAL_LOG_LEVEL, logEvent, __FILE__, __LINE__); \
        } \
    } while(0)
#else
#define LOG4XTENSA_FATAL(logger, logEvent)      
#define LOG4XTENSA_FATAL_STR(logger, logEvent)  
#endif

#endif /* _LOG4XTENSA_LOGGING_MACROS_HEADER_ */

