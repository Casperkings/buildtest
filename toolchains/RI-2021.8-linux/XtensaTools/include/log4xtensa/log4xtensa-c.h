// Copyright (c) 2005 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

#ifndef _LOG4XTENSA_C_H_
#define _LOG4XTENSA_C_H_

#include <stdarg.h>

#ifdef _WIN32
#  ifdef LOG4XTENSA_STATIC
#    define LOG4XTENSA_EXPORT
#  else
#    ifdef LOG4XTENSA_BUILD_DLL
#      define LOG4XTENSA_EXPORT __declspec(dllexport)
#    else
#      define LOG4XTENSA_EXPORT __declspec(dllimport)
#    endif
#  endif
#else
#  define LOG4XTENSA_EXPORT
#endif



#if defined __cplusplus
#  define EXTERN extern "C"
#else
#  define EXTERN extern
#endif


typedef unsigned int            u32;

#ifdef _MSC_EXTENSIONS
typedef unsigned __int64        u64;
#else
typedef unsigned long long      u64;
#endif

/// Text logger handle
typedef struct log4xtensa_TextLogger_struct *log4xtensa_TextLogger;

/// Binary logger handle
typedef struct log4xtensa_BinaryLogger_struct *log4xtensa_BinaryLogger;

/// Generic (either text or binary) logger handle
typedef void *log4xtensa_Logger;


typedef enum {
  log4xtensa_OFF_LOG_LEVEL         = 60000,
  log4xtensa_FATAL_LOG_LEVEL       = 50000,
  log4xtensa_ERROR_LOG_LEVEL       = 40000,
  log4xtensa_WARN_LOG_LEVEL        = 30000,
  log4xtensa_NOTE_LOG_LEVEL        = 25000,
  log4xtensa_INFO_LOG_LEVEL        = 20000,
  log4xtensa_VERBOSE_LOG_LEVEL     = 15000,
  log4xtensa_DEBUG_LOG_LEVEL       = 10000,
  log4xtensa_TRACE_LOG_LEVEL       = 0,
  log4xtensa_ALL_LOG_LEVEL         = log4xtensa_TRACE_LOG_LEVEL
} log4xtensa_LogLevel;



typedef enum {
  log4xtensa_REQUEST_EVENT               = 0x81000000,
  log4xtensa_RESPONSE_EVENT              = 0x81100000,
  log4xtensa_TIE_PORT_EVENT              = 0x82000000,
  log4xtensa_QUEUE_EVENT                 = 0x84000000,
  log4xtensa_DATA_EVENT                  = 0x91000000,
  log4xtensa_STRING_EVENT                = 0x92000000,
  log4xtensa_ELEMENTARY_EVENT            = 0x94000000,
  log4xtensa_LOGGER_NAME_RECORD          = 0xC0010000,
  log4xtensa_TIE_PORT_NAME_RECORD        = 0xC0020000,
  log4xtensa_MEMORY_PORT_NAME_RECORD     = 0xC0040000,
  log4xtensa_EVENT_NAME_RECORD           = 0xC0080000,
  log4xtensa_TIME_RESOLUTION_RECORD      = 0xC0100000,
} log4xtensa_LogEventType;


typedef enum {
  log4xtensa_READ        = 0x00,
  log4xtensa_BLOCK_READ  = 0x10,
  log4xtensa_RCW         = 0x50,
  log4xtensa_WRITE       = 0x80,
  log4xtensa_BLOCK_WRITE = 0x90
} log4xtensa_MemoryTransaction;


typedef enum {
  log4xtensa_REQUEST  = 0x0,
  log4xtensa_RESPONSE = 0x1
} log4xtensa_RequestResponse;


typedef enum {
  log4xtensa_NO        = 0,
  log4xtensa_YES       = 1,
} log4xtensa_LastData;


typedef enum {
  log4xtensa_OUTGOING    = 0,
  log4xtensa_INCOMING    = 1
} log4xtensa_PortDirection;


typedef enum {
  log4xtensa_POP  = 0x0,
  log4xtensa_PUSH = 0x1,
  log4xtensa_POP_FAILED  = 0x2,
  log4xtensa_PUSH_FAILED  = 0x3
} log4xtensa_QueueTransaction;


typedef enum {
  log4xtensa_OUTPUT_QUEUE  = 0x0,
  log4xtensa_INPUT_QUEUE   = 0x1,
  log4xtensa_EXPORT_STATE  = 0x2,
  log4xtensa_IMPORT_WIRE   = 0x3,
  log4xtensa_TIE_LOOKUP    = 0x4
} log4xtensa_TiePort;


typedef enum {
  log4xtensa_WRITE_STATE  = 0x0,
  log4xtensa_READ_WIRE = 0x1
} log4xtensa_TiePortTransaction;


enum {
  log4xtensa_UNKNOWN_PC  = 0xFFFFFFFF
};

enum {
  log4xtensa_UNKNOWN = 0xFFFFFFFF
};




/**
 * C-function to configure (or re-configure) the text and binary logger hierarchies.
 *
 * @param textFileName          Name of configuration file to use to configure
 *                              the text logger hierarachy.  If null or empty,
 *                              the text logger hierarchy is configured to set
 *                              logging events at LOG_LEVEL_NOTE or higher to
 *                              the console and to discard other events.
 *
 * @param binaryFileName        Name of configuration file to use to configure
 *                              the binary logger hierarachy.  If null or empty,
 *                              the binary logger hierarchy is configured to discard
 *                              all logging events.
 *
 * @return true if function succeeded, false if function failed.
 */
EXTERN LOG4XTENSA_EXPORT bool log4xtensa_Configure(const char *textFileName, const char *binaryFileName);



/**
 * C-function to get a handle to the named text logger.
 *
 * @param       name            The name of the text logger whose handle is desired.
 */
EXTERN LOG4XTENSA_EXPORT log4xtensa_TextLogger log4xtensa_GetTextLogger(const char *name);



/**
 * C-function to get a handle to the named binary logger.
 *
 * @param       name            The name of the binary logger whose handle is desired.
 */
EXTERN LOG4XTENSA_EXPORT log4xtensa_BinaryLogger log4xtensa_GetBinaryLogger(const char *name);



/**
 * C-function to set the log level of the named logger.
 *
 * @param       logger          A handle to the text or binary logger whose
 *                              log level is to be set.
 *
 * @param       logLevel        The new log level.
 *
 * @return true if function succeeded, false if function failed.
 */
EXTERN LOG4XTENSA_EXPORT bool log4xtensa_SetLogLevel(log4xtensa_Logger logger, log4xtensa_LogLevel logLevel);



/**
 * C-function to get the log level of the named logger.
 *
 * @param       logger          A handle to the text or binary logger whose
 *                              log level is desired.
 *
 * @return the log level of the specified text or binary logger.
 */
EXTERN LOG4XTENSA_EXPORT int log4xtensa_GetLogLevel(log4xtensa_Logger logger);



/**
 * C-function to determined if the specified text or binary logger is enabled for the
 * specified log level.
 *
 * @param       logger          A handle to a text or binary logger.
 *
 * @param       logLevel        The log level to compare against the logger's log level.
 *
 * @return true if the logger is enabled for logging at logLevel, otherwise false.
 */
EXTERN LOG4XTENSA_EXPORT bool log4xtensa_IsEnabledFor(log4xtensa_Logger logger, log4xtensa_LogLevel logLevel);



/**
 * C-function to conditionally generate a logging event for the specified text logger if the
 * logger's log level is less then or equal to the specified log level.
 *
 * @param       logger          A handle to a text logger.
 *
 * @param       logLevel        The log level to compare against the logger's log level.
 *
 * @param       format          A c-string specifying the format of the text logging event. The
 *                              format of format is the same as printf() in the standard C 
 *                              library.
 *
 * @param       ...             A variable number of parameters as specified in format to be
 *                              written to the logging event.
 *
 * @return true if function succeeded, false if function failed.
 */
EXTERN LOG4XTENSA_EXPORT bool log4xtensa_Text(log4xtensa_TextLogger     logger,
                                              log4xtensa_LogLevel       logLevel,
                                              const char               *format,
                                              ...);



/**
 * C-function to conditionally generate a logging event for the specified binary logger if the
 * logger's log level is less then or equal to the specified log level.
 *
 * @param       logger          A handle to a binary logger.
 *
 * @param       logLevel        The log level to compare against the logger's log level.
 *
 * @param       format          A c-string specifying the format of the binary logging event. The
 *                              format is as specified for the corresponding BinaryLogger::log
 *                              method.  @see BinaryLogger::log
 *                              Note:  This format is completely different then the printf() format.
 *
 * @param       ...             A variable number of parameters as specified in format to be
 *                              written to the logging event.
 *
 * @return true if function succeeded, false if function failed.
 */
EXTERN LOG4XTENSA_EXPORT bool log4xtensa_Binary(log4xtensa_BinaryLogger logger,
                                                log4xtensa_LogLevel     logLevel,
                                                int                     recordType,
                                                const char             *format,
                                                ...);



/**
 * C-function that works like log4xtensa_Binary for clients that already have a va_list.
 *
 * @param       logger          A handle to a binary logger.
 *
 * @param       logLevel        The log level to compare against the logger's log level.
 *
 * @param       format          A c-string specifying the format of the binary logging event. The
 *                              format is as specified for the corresponding BinaryLogger::log
 *                              method.  @see BinaryLogger::log
 *                              Note:  This format is completely different then the printf() format.
 *
 * @param       p_args          A va_list.
 *
 * @return true if function succeeded, false if function failed.
 */
EXTERN LOG4XTENSA_EXPORT bool log4xtensa_BinaryVarArg(log4xtensa_BinaryLogger logger,
                                                      log4xtensa_LogLevel     logLevel,
                                                      int                     recordType,
                                                      const char             *format,
                                                      va_list                *p_args);



/**
 * C-function to get a unique ID to identify an event name.
 *
 * @return a unique 64-bit ID.
 */
EXTERN LOG4XTENSA_EXPORT u64 log4xtensa_GetUniqueEventNameID();


/**
 * C-function to record the name of all binary loggers.
 *
 * @return true if function succeeded, false if function failed.
 */
EXTERN LOG4XTENSA_EXPORT bool log4xtensa_RecordBinaryLoggers();



#endif  // _LOG4XTENSA_C_H_
