#ifndef _XTSC_H_
#define _XTSC_H_

// Copyright (c) 2005-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file xtsc.h
 *
 * @see xtsc_text_logging_macros
 */


#include <xtsc/xtsc_types.h>
#include <ostream>
#include <string>
#include <set>
#include <vector>
#include <iomanip>
#include "xtensa-versions.h"
#include <xtsc/xtsc_exception.h>
#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_wire_write_if.h>
#include <xtsc/xtsc_mode_switch_if.h>
#include <log4xtensa/log4xtensa.h>



// If not defined, then assume SystemC 2.2
#if !defined(IEEE_1666_SYSTEMC)
#define IEEE_1666_SYSTEMC 200703L
#endif


// NSPP = N=number of ports, SPP = sc_port_policy (N.A. prior to SystemC 2.2)
#if SYSTEMC_VERSION < 20060505
#define NSPP 1
#else
#define NSPP 1, sc_core::SC_ZERO_OR_MORE_BOUND
#endif

#if IEEE_1666_SYSTEMC < 201101L
namespace sc_core { class sc_unwind_exception{}; };
#endif


/**
 * All xtsc library objects (non-member functions, non-member data, and classes including
 * xtsc_core) and their associated typedef and enum types are in the xtsc namespace.
 *
 * The xtsc_comp library objects (xtsc_arbiter, xtsc_dma_engine, xtsc_ext_regfile, 
 * xtsc_ext_regfile_pin, xtsc_lookup, xtsc_lookup_driver, xtsc_lookup_pin, 
 * xtsc_master, xtsc_memory, xtsc_memory_pin, xtsc_memory_tlm2, 
 * xtsc_memory_trace, xtsc_mmio, xtsc_pin2tlm_memory_transactor, xtsc_queue, 
 * xtsc_queue_consumer, xtsc_queue_pin, xtsc_queue_producer, xtsc_router,
 * xtsc_slave, xtsc_tlm2pin_memory_transactor, xtsc_tlm22xttlm_transactor, xtsc_wire,
 * xtsc_wire_logic, xtsc_wire_source, and xtsc_xttlm2tlm2_transactor) are in the
 * xtsc_component namespace.
 */
namespace xtsc {

class xtsc_core;
class xtsc_request;
class xtsc_response;
class xtsc_host_mutex;
struct lua_State;


/// typedef to indicate a dummy variable used to anchor doxygen documentation comments.
typedef int Readme;



/**
 * Summary of macros to disable or to do text logging.
 *
 * The following macros may be passed to the compiler to disable text logging at compile
 * time.
 *
 * \verbatim
   XTSC_DISABLE_LOGGING             Disable all text and binary logging (FATAL, ERROR
                                    and WARN text messages will be sent to stderr).
   XTSC_DISABLE_FATAL_LOGGING       Disable all text logging (FATAL, ERROR, and WARN
                                    messages will be sent to stderr).
   XTSC_DISABLE_ERROR_LOGGING       Disable all text logging at ERROR level and below
                                    (ERROR and WARN messages will be sent to stderr).
   XTSC_DISABLE_WARN_LOGGING        Disable all text logging at WARN level and below
                                    (WARN messages will be sent to stderr).
   XTSC_DISABLE_NOTE_LOGGING        Disable all text logging at NOTE level and below.
   XTSC_DISABLE_INFO_LOGGING        Disable all text logging at INFO level and below.
   XTSC_DISABLE_VERBOSE_LOGGING     Disable all text logging at VERBOSE level and below.
   XTSC_DISABLE_DEBUG_LOGGING       Disable all text logging at DEBUG level and below.
   XTSC_DISABLE_TRACE_LOGGING       Disable all text logging at TRACE level and below.
   \endverbatim
 *
 *
 * Unless compiled out using the above macros, the following macros may be used in
 * source code to do text logging at the specified level.
 *
 * \verbatim
   XTSC_FATAL(logger, msg)
   XTSC_ERROR(logger, msg)
   XTSC_WARN(logger, msg)
   XTSC_NOTE(logger, msg)
   XTSC_INFO(logger, msg)
   XTSC_VERBOSE(logger, msg)
   XTSC_DEBUG(logger, msg)
   XTSC_TRACE(logger, msg)
   \endverbatim
 *
 * @see XTSC_FATAL
 * @see XTSC_ERROR
 * @see XTSC_WARN
 * @see XTSC_NOTE
 * @see XTSC_INFO
 * @see XTSC_VERBOSE
 * @see XTSC_DEBUG
 * @see XTSC_TRACE
 *
 */
extern Readme xtsc_text_logging_macros;



#ifndef DOXYGEN_SKIP
/*
 * Summary of macros to disable or to do binary logging.
 *
 * The following macros may be passed to the compiler to disable binary logging at
 * compile time.
 *
 * \verbatim
   XTSC_DISABLE_LOGGING             Disable all text and binary logging (FATAL, ERROR
                                    and WARN text messages will be sent to stderr).
   XTSC_DISABLE_BIN_LOGGING         Disable all binary logging.
   XTSC_DISABLE_NOTE_BIN_LOGGING    Disable all binary logging at NOTE level and below.
   XTSC_DISABLE_INFO_BIN_LOGGING    Disable all binary logging at INFO level and below.
   XTSC_DISABLE_VERBOSE_BIN_LOGGING Disable all binary logging at VERBOSE level and
                                    below.
   \endverbatim
 *
 *
 * Unless compiled out using the above macros, the following macros may be used in
 * source code to do binary logging at the specified level.
 *
 * \verbatim
   XTSC_NOTE_BIN(binaryLogger, type, logEvent)
   XTSC_INFO_BIN(binaryLogger, type, logEvent)
   XTSC_VERBOSE_BIN(binaryLogger, type, logEvent)
   \endverbatim
 *
 * @see XTSC_NOTE_BIN
 * @see XTSC_INFO_BIN
 * @see XTSC_VERBOSE_BIN
 */
//extern Readme xtsc_binary_logging_macros;
#endif // DOXYGEN_SKIP



#if defined(XTSC_DISABLE_LOGGING) && !defined(XTSC_DISABLE_FATAL_LOGGING)
#define XTSC_DISABLE_FATAL_LOGGING
#endif

#if defined(XTSC_DISABLE_FATAL_LOGGING) && !defined(XTSC_DISABLE_ERROR_LOGGING)
#define XTSC_DISABLE_ERROR_LOGGING
#endif

#if defined(XTSC_DISABLE_ERROR_LOGGING) && !defined(XTSC_DISABLE_WARN_LOGGING)
#define XTSC_DISABLE_WARN_LOGGING
#endif

#if defined(XTSC_DISABLE_WARN_LOGGING) && !defined(XTSC_DISABLE_NOTE_LOGGING)
#define XTSC_DISABLE_NOTE_LOGGING
#endif

#if defined(XTSC_DISABLE_NOTE_LOGGING) && !defined(XTSC_DISABLE_INFO_LOGGING)
#define XTSC_DISABLE_INFO_LOGGING
#endif

#if defined(XTSC_DISABLE_INFO_LOGGING) && !defined(XTSC_DISABLE_VERBOSE_LOGGING)
#define XTSC_DISABLE_VERBOSE_LOGGING
#endif

#if defined(XTSC_DISABLE_VERBOSE_LOGGING) && !defined(XTSC_DISABLE_DEBUG_LOGGING)
#define XTSC_DISABLE_DEBUG_LOGGING
#endif

#if defined(XTSC_DISABLE_DEBUG_LOGGING) && !defined(XTSC_DISABLE_TRACE_LOGGING)
#define XTSC_DISABLE_TRACE_LOGGING
#endif


#if defined(XTSC_DISABLE_LOGGING) && !defined(XTSC_DISABLE_BIN_LOGGING)
#define XTSC_DISABLE_BIN_LOGGING
#endif

#if defined(XTSC_DISABLE_NOTE_BIN_LOGGING) && !defined(XTSC_DISABLE_INFO_BIN_LOGGING)
#define XTSC_DISABLE_INFO_BIN_LOGGING
#endif

#if defined(XTSC_DISABLE_INFO_BIN_LOGGING) && !defined(XTSC_DISABLE_VERBOSE_BIN_LOGGING)
#define XTSC_DISABLE_VERBOSE_BIN_LOGGING
#endif

#if defined(XTSC_DISABLE_VERBOSE_BIN_LOGGING) && !defined(XTSC_DISABLE_DEBUG_BIN_LOGGING)
#define XTSC_DISABLE_DEBUG_BIN_LOGGING
#endif



/**
 * Macro to ensure a standard prefix to all XTSC log messages.
 *
 *  Note: This macro was changed with the RD-2011.1 release to require the user
 *        to provide a non-const string buffer.  See xtsc_log_delta_cycle().
 *
 * @see xtsc_set_text_logging_time_precision()
 * @see xtsc_set_text_logging_time_width()
 * @see xtsc_set_system_clock_factor()
 * @see xtsc_set_text_logging_delta_cycle_digits()
 */
#define XTSC_LOG_PREFIX(buf) std::setprecision(xtsc::xtsc_get_text_logging_time_precision()) \
                          << std::fixed \
                          << std::setw(xtsc::xtsc_get_text_logging_time_width()) \
                          << (sc_core::sc_time_stamp() / xtsc::xtsc_get_system_clock_period()) \
                          << xtsc::xtsc_log_delta_cycle(buf) \
                          << ": "



/**
 * Macro for logging at the FATAL_LOG_LEVEL.
 *
 * Note: calling this macro does not cause your program to terminate.
 *
 * @param logger        A reference to a TextLogger object.
 *
 * @param msg           The message to log.  msg can be any text acceptable
 *                      to the ostringstream left-shift operator.
 */
#if !defined(XTSC_DISABLE_FATAL_LOGGING)
#define   XTSC_FATAL(logger, msg)       do { \
                                          std::string buf; \
                                          if (xtsc::xtsc_is_logging_configured()) { \
                                            LOG4XTENSA_FATAL  (logger, XTSC_LOG_PREFIX(buf) << msg); \
                                          } \
                                          else { \
                                            std::cerr << "FATAL: " << XTSC_LOG_PREFIX(buf) << msg << std::endl; \
                                          } \
                                        } while (false)

#else
#define   XTSC_FATAL(logger, msg)       do { \
                                          std::string buf; \
                                          std::cerr << "FATAL: " << XTSC_LOG_PREFIX(buf) << msg << std::endl; \
                                        } while (false)
#endif


/**
 * Macro for logging at the ERROR_LOG_LEVEL.
 *
 * @param logger        A reference to a TextLogger object.
 *
 * @param msg           The message to log.  msg can be any text acceptable
 *                      to the ostringstream left-shift operator.
 */
#if !defined(XTSC_DISABLE_ERROR_LOGGING)
#define   XTSC_ERROR(logger, msg)       do { \
                                          std::string buf; \
                                          if (xtsc::xtsc_is_logging_configured()) { \
                                            LOG4XTENSA_ERROR  (logger, XTSC_LOG_PREFIX(buf) << msg); \
                                          } \
                                          else { \
                                            std::cerr << "ERROR: " << XTSC_LOG_PREFIX(buf) << msg << std::endl; \
                                          } \
                                        } while (false)

#else
#define   XTSC_ERROR(logger, msg)       do { \
                                          std::string buf; \
                                          std::cerr << "ERROR: " << XTSC_LOG_PREFIX(buf) << msg << std::endl; \
                                        } while (false)
#endif


/**
 * Macro for logging at the WARN_LOG_LEVEL.
 *
 * @param logger        A reference to a TextLogger object.
 *
 * @param msg           The message to log.  msg can be any text acceptable
 *                      to the ostringstream left-shift operator.
 */
#if !defined(XTSC_DISABLE_WARN_LOGGING)
#define    XTSC_WARN(logger, msg)       do { \
                                          std::string buf; \
                                          if (xtsc::xtsc_is_logging_configured()) { \
                                            LOG4XTENSA_WARN   (logger, XTSC_LOG_PREFIX(buf) << msg); \
                                          } \
                                          else { \
                                            std::cerr << "WARN: " << XTSC_LOG_PREFIX(buf) << msg << std::endl; \
                                          } \
                                        } while (false)

#else
#define    XTSC_WARN(logger, msg)       do { \
                                          std::string buf; \
                                          std::cerr << "WARN: " << XTSC_LOG_PREFIX(buf) << msg << std::endl; \
                                        } while (false)
#endif


/**
 * Macro for logging at the NOTE_LOG_LEVEL.
 *
 * @param logger        A reference to a TextLogger object.
 *
 * @param msg           The message to log.  msg can be any text acceptable
 *                      to the ostringstream left-shift operator.
 */
#if !defined(XTSC_DISABLE_NOTE_LOGGING)
#define    XTSC_NOTE(logger, msg)       do { \
                                          std::string buf; \
                                          if (xtsc::xtsc_is_logging_configured()) { \
                                            LOG4XTENSA_NOTE   (logger, XTSC_LOG_PREFIX(buf) << msg); \
                                          } \
                                          else { \
                                            std::cout << "NOTE: " << XTSC_LOG_PREFIX(buf) << msg << std::endl; \
                                          } \
                                        } while (false)

#else
#define    XTSC_NOTE(logger, msg)       do { \
                                          std::string buf; \
                                          std::cout << "NOTE: " << XTSC_LOG_PREFIX(buf) << msg << std::endl; \
                                        } while (false)
#endif


/**
 * Macro for logging at the INFO_LOG_LEVEL.
 *
 * @param logger        A reference to a TextLogger object.
 *
 * @param msg           The message to log.  msg can be any text acceptable
 *                      to the ostringstream left-shift operator.
 */
#if !defined(XTSC_DISABLE_INFO_LOGGING)
#define    XTSC_INFO(logger, msg) do { if (xtsc::xtsc_is_text_logging_enabled()) \
                                        { std::string buf; LOG4XTENSA_INFO   (logger, XTSC_LOG_PREFIX(buf) << msg); } \
                                     } while (false)
#else
#define    XTSC_INFO(logger, msg)
#endif


/**
 * Macro for logging at the VERBOSE_LOG_LEVEL.
 *
 * @param logger        A reference to a TextLogger object.
 *
 * @param msg           The message to log.  msg can be any text acceptable
 *                      to the ostringstream left-shift operator.
 */
#if !defined(XTSC_DISABLE_VERBOSE_LOGGING)
#define XTSC_VERBOSE(logger, msg) do { if (xtsc::xtsc_is_text_logging_enabled()) \
                                        { std::string buf; LOG4XTENSA_VERBOSE(logger, XTSC_LOG_PREFIX(buf) << msg); } \
                                     } while (false)
#else
#define XTSC_VERBOSE(logger, msg)
#endif


/**
 * Macro for logging at the DEBUG_LOG_LEVEL.
 *
 * @param logger        A reference to a TextLogger object.
 *
 * @param msg           The message to log.  msg can be any text acceptable
 *                      to the ostringstream left-shift operator.
 */
#if !defined(XTSC_DISABLE_DEBUG_LOGGING)
#define   XTSC_DEBUG(logger, msg) do { if (xtsc::xtsc_is_text_logging_enabled()) \
                                        { std::string buf; LOG4XTENSA_DEBUG  (logger, XTSC_LOG_PREFIX(buf) << msg); } \
                                     } while (false)
#else
#define   XTSC_DEBUG(logger, msg)
#endif


/**
 * Macro for logging at the TRACE_LOG_LEVEL.
 *
 * @param logger        A reference to a TextLogger object.
 *
 * @param msg           The message to log.  msg can be any text acceptable
 *                      to the ostringstream left-shift operator.
 */
#if !defined(XTSC_DISABLE_TRACE_LOGGING)
#define   XTSC_TRACE(logger, msg) do { if (xtsc::xtsc_is_text_logging_enabled()) \
                                        { std::string buf; LOG4XTENSA_TRACE  (logger, XTSC_LOG_PREFIX(buf) << msg); } \
                                     } while (false)
#else
#define   XTSC_TRACE(logger, msg)
#endif


/**
 * This macro is used to log at a programmatic log level.
 */
#define XTSC_LOG(logger, level, msg) do { \
  if ((xtsc::xtsc_is_text_logging_enabled() || (level >= log4xtensa::NOTE_LOG_LEVEL)) && logger.isEnabledFor(level)) { \
    std::string buf; \
    log4xtensa::tostringstream _xtsc_buf; \
    _xtsc_buf << XTSC_LOG_PREFIX(buf) << msg; \
    logger.forcedLog(level, _xtsc_buf.str(), __FILE__, __LINE__); \
  } \
  else if (level == log4xtensa::NOTE_LOG_LEVEL) { \
    std::string buf; \
    std::cout << "NOTE: " << XTSC_LOG_PREFIX(buf) << msg << std::endl; \
  } \
  else if (level > log4xtensa::NOTE_LOG_LEVEL) { \
    std::string buf; \
    std::cerr << ((level >= log4xtensa::FATAL_LOG_LEVEL) ? "FATAL: " : (level >= log4xtensa::ERROR_LOG_LEVEL) ? "ERROR: " : "WARN: ");\
    std::cerr << XTSC_LOG_PREFIX(buf) << msg << std::endl; \
  } \
} while (false)





#ifndef DOXYGEN_SKIP
#if !defined(XTSC_DISABLE_NOTE_BIN_LOGGING)
#define XTSC_NOTE_BIN(binaryLogger, type, logEvent) do { if (xtsc::xtsc_is_binary_logging_enabled()) \
                                        { LOG4XTENSA_BIN(binaryLogger, log4xtensa::NOTE_LOG_LEVEL, type, logEvent); } } while (false)
#else
#define XTSC_NOTE_BIN(binaryLogger, type, logEvent)
#endif

#if !defined(XTSC_DISABLE_INFO_BIN_LOGGING)
#define XTSC_INFO_BIN(binaryLogger, type, logEvent) do { if (xtsc::xtsc_is_binary_logging_enabled()) \
                                        { LOG4XTENSA_BIN(binaryLogger, log4xtensa::INFO_LOG_LEVEL, type, logEvent); } } while (false)
#else
#define XTSC_INFO_BIN(binaryLogger, type, logEvent)
#endif

#if !defined(XTSC_DISABLE_VERBOSE_BIN_LOGGING)
#define XTSC_VERBOSE_BIN(binaryLogger, type, logEvent) do { if (xtsc::xtsc_is_binary_logging_enabled()) \
                                        { LOG4XTENSA_BIN(binaryLogger, log4xtensa::VERBOSE_LOG_LEVEL, type, logEvent); } } \
                                        while (false)
#else
#define XTSC_VERBOSE_BIN(binaryLogger, type, logEvent)
#endif
#endif // DOXYGEN_SKIP



/// Return true if the logging facility (log4xtensa) has been configured
XTSC_API bool xtsc_is_logging_configured();



/**
 * Conveniece method to determine if a string matches a pattern.  This method returns
 * true if str matches pattern, otherwise it returns false.
 *
 * @param       pattern         Pattern to match str against.  Any asterisk (*) found in
 *                              the pattern is treated as a wildcard which matches any
 *                              number of any characters in any combination.
 *
 * @param       str             The string being checked for a match.
 */
XTSC_API bool xtsc_pattern_match(const std::string& pattern, const std::string& str);



/**
 * Return a string showing version information for the XTSC core library
 */
XTSC_API std::string xtsc_version();





#if !defined(SCP)
#define SCP
#undef SCP
#endif

/**
 * Method to set the XTSC system clock period (SCP).
 *
 * Although XTSC does not instantiate any sc_clock objects, it still has the concept of
 * a system clock period.  This method is used to set the XTSC system clock period as a
 * multiple of the SystemC time resolution (which can be accessed using the SystemC
 * methods sc_set_time_resolution() and sc_get_time_resolution()).  By default (i.e. if
 * this method is never called), XTSC uses a factor of 1000.  So, if neither the 
 * sc_set_time_resolution() method nor the xtsc_set_system_clock_factor() method are
 * called then the XTSC system clock period will be 1 nanosecond (which is 1000 times
 * the default SystemC time resolution of 1 picosecond).
 *
 * XTSC also has the concept of clock phase.  By default, the first posedge clock
 * conceptually occurs at time 0, the second one occurs one system clock period later,
 * and so on.  If desired, the posedge_factor argument can be set to a value other than
 * 0 (but strictly less than the clock_factor argument) to delay the first conceptual
 * posedge clock (and all subsequent ones, too).  Many of the XTSC components have
 * parameters that make reference to such things as "clock phase", "posedge clock", and
 * "negedge clock".  All these parameters are in reference to the clock phase concept
 * explained here and their effect is adjusted in accordance with any change to the
 * posedge_factor argument.
 *
 * This method is called to change the default value of system_clock_factor. When this 
 * method is called the clock phase delta factors are also recalibrated. If you don't 
 * want the recalibrated values then you may subsequently change them using the 
 * xtsc_core::set_clock_phase_delta_factors() method. If the new value of 
 * system_clock_factor is very small, say less than 10, the recalibrated clock phases
 * may overlap, therby impacting the simulation of iss. Hence, it is recommended to 
 * adjust the sc_time_resolution and reduce it accordingly. For instance, for a 
 * system_clock_factor value of 1, the sc_time_resolution can be made SC_FS and 
 * system_clock_factor be made 1000 to get the same clock period.
 *
 * This method must not be called after xtsc_get_system_clock_period() has been called.
 * The xtsc_initialize() method internally calls xtsc_get_system_clock_period, so
 * xtsc_set_system_clock_factor() must not be called after xtsc_initialize() is called.
 *
 * Technique #1: Using the logging configuration file form of the xtsc_initialize()
 * function.  The general sequence (most likely in sc_main) is:
 * \verbatim
       sc_set_time_resolution(...);                     // Optional
       xtsc_set_system_clock_factor(...);               // Optional
       xtsc_initialize(logging_config_file);
       // Now construct any desired XTSC modules (xtsc_core, xtsc_memory, etc)
   \endverbatim
 *
 * Technique #2: Using the xtsc_initialize_parms form of the xtsc_initialize() function.
 * The general sequence (most likely in sc_main) is:
 * \verbatim
       sc_set_time_resolution(...);                     // Optional
       xtsc_initialize_parms init_parms(...);
       init_parms.set("system_clock_factor", ...);      // Optional
       init_parms.set("posedge_offset_factor", ...);    // Optional
       init_parms.extract_parms(argc, argv, "xtsc");    // Optional
       xtsc_initialize(init_parms);
       // Now construct any desired XTSC modules (xtsc_core, xtsc_memory, etc)
   \endverbatim
 *
 * @see xtsc_initialize_parms
 * @see xtsc_get_system_clock_period.
 * @see xtsc_wait.
 * @see xtsc_core::set_clock_phase_delta_factors
 */
XTSC_API void xtsc_set_system_clock_factor(u32 clock_factor, u32 posedge_factor = 0);




/**
 * This method returns the factor by which the SystemC time resolution is multiplied to
 * determine the system clock period.
 * @see xtsc_set_system_clock_factor.
 */
XTSC_API u32 xtsc_get_system_clock_factor();



/**
 * Configuration parameters for the call to xtsc_initialize().
 *
 *  \verbatim
   Name                            Type         Description
   -----------------------------   -----------  ----------------------------------------
  
   "systemc_time_resolution"       char*        This read-only parameter shows the value
                                                returned from sc_get_time_resolution().

   "system_clock_factor"           u32          If this value is changed, then the
                                                xtsc_set_system_clock_factor() method
                                                is called during xtsc_initialize() with
                                                the new value supplied by this parameter.
                                                Default = as set at xtsc_initialize_parms
                                                construction time (which, by default, is
                                                1000).

   "posedge_offset_factor"         u32          This specifies the time at which the
                                                first posedge of the system clock
                                                conceptually occurs.  It is expressed in
                                                units of the SystemC time resolution and
                                                must be strictly less than
                                                "system_clock_factor".
                                                Default = 0.  The default can be
                                                overridden by setting the environment
                                                variable XTSC_POSEDGE_OFFSET_FACTOR
                                                which, in turn, can be overridden by
                                                calling xtsc_set_system_clock_factor().

   "clock_phase_delta_factors"     vector<u32>  If any of these values are changed, then
                                                xtsc_core::set_clock_phase_delta_factors()
                                                is called during xtsc_initialize() with
                                                the 3 values supplied by this parameter.
                                                Default = as set at xtsc_initialize_parms
                                                construction time (which, by default, is
                                                [200, 100, 600]).

   "call_sc_stop_on_finalize"      bool         The xtsc_set_call_sc_stop() method is
                                                called during xtsc_initialize() with the
                                                value supplied by this parameter.  If 
                                                this parameter is set to true, then the
                                                sc_stop() method will be called by the
                                                xtsc_finalize() method.
                                                Default = as set at xtsc_initialize_parms
                                                construction time (which, by default, is
                                                true).

   "stop_after_all_cores_exit"     bool         xtsc_core::set_stop_after_all_cores_exit()
                                                is called during xtsc_initialize() with
                                                the value supplied by this parameter.  If 
                                                this parameter is set to true, then the
                                                sc_stop() method will be called when the
                                                last running core exits.
                                                Default = as set at xtsc_initialize_parms
                                                construction time (which, by default, is
                                                true).

   "address_bits"                  u32          The nominal number of bits in the global
                                                address space.  This parameter does not
                                                actually limit the number of address
                                                bits, rather it is used by models to
                                                determine how many nibbles to use when
                                                logging an address or how many addresss
                                                bits to trace to a VCD file.
                                                See xtsc_address_bits().
                                                See xtsc_address_nibbles().
                                                Default = 32 (minimum=32, maximum=64)

   "constructor_log_level"         char*        The xtsc_set_constructor_log_level()
                                                method is called during xtsc_initialize()
                                                with the value supplied by this parameter.
                                                Case-insensitive valid values are:
                                           FATAL|ERROR|WARN|NOTE|INFO|VERBOSE|DEBUG|TRACE
                                                Default = as set at xtsc_initialize_parms
                                                construction time (which, by default, is
                                                INFO).

   "cycle_limit"                   u32          The number of system clock periods that
                                                the simulation should be limited to.
                                                Default = 0 (no limit).

   "enable_dynamic_clock_period"   bool         Several models in XTSC include a method
                                                to change their clock period after
                                                simulation has started.  In general,
                                                changing the clock period at arbitrary
                                                times and to arbitrary values cannot be
                                                guaranteed to work.  See the
                                                documentation associated with the
                                                xtsc_core::change_clock_period method
                                                for some of the possible problems. By
                                                default, changing the clock period 
                                                of xtsc_core is disabled.  Set this
                                                parameter to true to enable using
                                                the xtsc_core::change_clock_period
                                                method.
                                                Default = false.

   "enable_sc_report"              bool         If set, sc_report logs will not be 
                                                directed to the XTSC text logger file. 
                                                After setting this parameter, the user 
                                                has full control to specify attributes 
                                                of sc_report like actions, verbosity, 
                                                log file name, file-handler etc in the 
                                                systemC code using sc_report_handler and
                                                sc_report APIs.
                                                Default = false.

   "full_hierarchical_names"       bool         During SystemC-Verilog co-simulation
                                                with Verilog-on-top, this parameter can
                                                be set to true to cause the
                                                extract_parms() methods to require the
                                                full hierarchical module instance name.
                                                In addition, if doing waveform tracing
                                                to a file in the current directory, the
                                                hierarchical name (using a period as the
                                                hierarchy separator) will be prepended
                                                to the vcd file name.  These changes are
                                                useful when you want to instantiated the
                                                SystemC wrapper multiple times from a
                                                Verilog module or testbench.
                                                Default = false.

   "hex_dump_left_to_right"        bool         The xtsc_set_hex_dump_left_to_right()
                                                method is called during xtsc_initialize()
                                                with the value supplied by this parameter.
                                                Default = as set at xtsc_initialize_parms
                                                construction time (which, by default, is
                                                true).

   "lua_command_prompt"            bool         If true, then a lua command prompt will
                                                be available at the start of simulation.
                                                This prompt works like the lua command
                                                processor provided with the Lua
                                                distribution and includes an additional
                                                Lua module called xtsc which includes
                                                a Lua function called cmd which gives
                                                access to the commands registered with
                                                the XTSC command facility.
                                                For example using hello_world:
              ./hello_world -lua_command_prompt=true
              Lua 5.2.3  Copyright (C) 1994-2013 Lua.org, PUC-Rio
              > help
              ...
              > ?
              sc_simulation:    sc
              xtsc_core:        core0
              xtsc_memory:      core0_pif
              xtsc_simulation:  xtsc
              > =xtsc.cmd("sc wait 100")
                   100.0/320: 
              > =xtsc.cmd("core0 get_pc")
              0x4000005f
              > =xtsc.cmd("core0 dasm pc")
              0x4000005f: movi.n a3,21
              > =xtsc.cmd("core0 dasm next")
              0x40000061: wsr.atomctl a3
              > c
                                                For example using xtsc-run:
              xtsc-run -include=hello_world.inc -set_xtsc_parm=lua_command_prompt=true
              ...
                                                You can get the lua command prompt in an
                                                "empty" simulation which has only the
                                                global xtsc and sc command handlers
                                                using xtsc-run like this:
              xtsc-run -set_xtsc_parm=lua_command_prompt=true
              ...
                                                For more information about the Lua
                                                language, please see www.Lua.org.
                                                Note: If this parameter is set to true
                                                then "xtsc_command_prompt" should be
                                                left false.
                                                Default = false

   "lua_script_files"              char**       An optional comma-separated list of Lua
                                                script files to be processed during
                                                simulation.  Each script will be
                                                processed in its own SystemC thread.
                                                See "lua_command_prompt" above for more
                                                information about Lua.
                                                Default = NULL
                                                
   "lua_script_file_beoe"          char*        Optional name of a Lua script file to be
                                                processed during the SystemC 
                                                before_end_of_elaboration() callback.
                                                See "lua_command_prompt" above for more
                                                information about Lua.
                                                Default = NULL
                                                
   "lua_script_file_eoe"           char*        Optional name of a Lua script file to be
                                                processed during the SystemC 
                                                end_of_elaboration() callback.
                                                See "lua_command_prompt" above for more
                                                information about Lua.
                                                Default = NULL
                                                
   "lua_script_file_sos"           char*        Optional name of a Lua script file to be
                                                processed during the SystemC 
                                                start_of_simulation() callback.
                                                See "lua_command_prompt" above for more
                                                information about Lua.
                                                Default = NULL
                                                
   "lua_script_file_eos"           char*        Optional name of a Lua script file to be
                                                processed during the SystemC 
                                                end_of_simulation() callback.
                                                See "lua_command_prompt" above for more
                                                information about Lua.
                                                Default = NULL
                                                
   "breakpoint_csv_file"           char*        Optional name of a comma-separated value
                                                file in which to record breakpoint hits.
                                                If desired, you can specify a single
                                                hyphen for "breakpoint_csv_file" value
                                                and the output will be sent to STDOUT
                                                instead of to a disk file.
                                                The xtsc_core::set_breakpoint_callback()
                                                method provides a means for user code
                                                to get a callback each time a breakpoint
                                                is hit.  If desired, you can use this
                                                mechanism to record the breakpoint
                                                information to a CSV file using a
                                                built-in callback function instead of
                                                having to write your own.  To do this
                                                simply set this parameter to the desired
                                                name ofthe CSV file to generated.  At
                                                the start of simulation, this file will
                                                be created and for each user breakpoint
                                                hit in target code, a line with 9 CSV
                                                values will be added to this file in the
                                                following format:
                                        SimulationTime,"CoreName",PC,CycleCount,CCOUNT
                                                Default = NULL

   "simcall_csv_file"              char*        Optional name of a comma-separated value
                                                file in which to record user simcalls.
                                                If desired, you can specify a single
                                                hyphen for "simcall_csv_file" value and
                                                the output will be sent to STDOUT
                                                instead of to a disk file.  In either
                                                case, the values will also be logged
                                                at INFO_LOG_LEVEL to the xtsc.log file.
                                                Xtensa ISS provides a means for target
                                                code to call a user-provided function in
                                                the simulator and pass it up to 6 values
                                                (see xtsc_core::set_simcall_callback).
                                                If desired, you can use this mechanism
                                                to record the 6 simcall arguments to a
                                                CSV file using a built-in callback
                                                function instead of having to write your
                                                own.  To do this simply set this
                                                parameter to the desired name ofthe CSV
                                                file to generated.  At the start of
                                                simulation, this file will be created
                                                and for each user simcall executed in
                                                target code, a line with 10 CSV values
                                                will be added to this file in the
                                                following format:
         SimulationTime,"CoreName",CycleCount,CCOUNT,arg1,arg2,arg3,arg4,arg5,arg6
                                                By default, each argN value is printed
                                                as a decimal number but this can be
                                                changed using the "simcall_csv_format" 
                                                parameter.
                                                Instructions:
      1. Include the following file in your target (Xtensa) program:
            #include <xtensa/sim.h>
      2. Call the xt_iss_simcall() function and pass it 6 arguments.  If you
         need fewer arguments, you can pass dummy values (e.g. 0) for the extra
         arguments to make a total of 6.  For example:
            xt_iss_simcall(arg1, arg2, 0, 0, 0, 0);
                                                Default = NULL

   "simcall_csv_format"            vector<u32>  Optional vector of from 0 to 6 values
                                                used to specify the output format of the
                                                6 simcall arguments.  The first value
                                                specifies the format for arg1, the
                                                second for arg2, and so on (see
                                                "simcall_csv_file").  A value of 0 means
                                                to print the corresponding argument as a
                                                decimal number in the CSV file.  A value
                                                of 1 means to print it as hexadecimal with
                                                "0x" prefix.  A value of 2 means to
                                                interpret the value as a pointer to a
                                                null-terminated C-string (char*) in the
                                                core's address space and to print the
                                                string.  A value of 3 means to not print
                                                the corresponding argument.  If less
                                                than 6 values are specified the missing
                                                values default to 0.
                                                Default (unset).

   "simcall_logging"               bool         Enable log4xtensa on/off control via an
                                                xt_iss_simcall() from target (Xtensa)
                                                code.  Instructions:
      1. Set up logging for the simulation run (see the Logging chapter in xtsc_ug.pdf).
      2. Set this parameter to true.
      3. In your Xtensa code:
            #include <xtensa/sim.h>
         To turn logging on:
            xt_iss_simcall(1, 0xFACEF00D, 0, 0, 0, 0);
         To turn logging off:
            xt_iss_simcall(0, 0xFACEF00D, 0, 0, 0, 0);
                                                Note: The arg2 magic value of 0xFACEF00D
                                                is to allow compatibility with other
                                                simcall options like "simcall_csv_file".
                                                Default = false.

   "target_memory_limit"           u32          The total limit in megabytes of target
                                                memory space that the ISS will allocate
                                                for local memories modelled internally.
                                                This limit does not apply to XTSC
                                                memory components such as xtsc_memory.
                                                Default = 512 megabytes (0x20000000).

   "text_logging_config_file"      char*        The configuration file for text logging.
                                                If NULL (the default) or empty, and the
                                                XTSC_TEXTLOGGER_CONFIG_FILE environment
                                                variable is defined and NOT equal to off
                                                (case-insensitive) then the contents of
                                                that environment variable will be taken
                                                as the optional path and name of the
                                                configuration file to be used.  If the
                                                configuration file named by the
                                                XTSC_TEXTLOGGER_CONFIG_FILE environment
                                                variable does not exists, it will be
                                                created and initialized to contain
                                                configuration information to send
                                                messages at INFO_LOG_LEVEL and higher to
                                                a file in the current working directory
                                                called xtsc.log (or whatever is
                                                specified by the XTSC_LOG_FILE_NAME
                                                environment variable) and to also send
                                                messages at NOTE_LOG_LEVEL and higher to
                                                the console.  If
                                                "text_logging_config_file" is NULL or
                                                empty and the
                                                XTSC_TEXTLOGGER_CONFIG_FILE environment
                                                variable is not defined, or is defined
                                                and equal to off, then text logging will
                                                be configured so that messages at
                                                NOTE_LOG_LEVEL and higher will be sent
                                                to the console and other messages will
                                                be discarded.  In this case if the
                                                XTSC_TEXTLOGGER_CONFIG_FILE is equal to
                                                off, even though the log4xtensa library
                                                will be configured as just specified,
                                                the xtsc_enable_text_logging() method
                                                will be called with an argument of false
                                                to disable all text logging at
                                                INFO_LOG_LEVEL and below.

   "text_logging_delta_cycle_digits" u32        xtsc_set_text_logging_delta_cycle_digits()
                                                is called during xtsc_initialize() with the
                                                value supplied by this parameter.
                                                Default = as set at xtsc_initialize_parms
                                                construction time (which, by default, is
                                                1).

   "text_logging_disable"          bool         The xtsc_enable_text_logging() method is
                                                called during xtsc_initialize() with the
                                                value implied by this parameter unless
                                                "text_logging_config_file" is NULL/empty
                                                and the XTSC_TEXTLOGGER_CONFIG_FILE
                                                environment is defined and equal to off.
                                                Default = as set at xtsc_initialize_parms
                                                construction time (which, by default, is
                                                false).

   "text_logging_axi_id_width"     u32          The number of nibbles that should be
                                                used to log the AXI Transaction ID.
                                                Default = 2.
                                                Minimum = 1.

   "text_logging_axi_cnt_width"    u32          The number of digits that should be
                                                used to log the AXI transfer count.
                                                Default = 2.
                                                Minimum = 1.

   "text_logging_time_precision"   u32          The xtsc_set_text_logging_time_precision()
                                                method is called during xtsc_initialize()
                                                with the value supplied by this parameter.
                                                Default = as set at xtsc_initialize_parms
                                                construction time (which, by default, is
                                                1).

   "text_logging_time_width"       u32          The xtsc_set_text_logging_time_width()
                                                method is called during xtsc_initialize()
                                                with the value supplied by this parameter.
                                                Default = as set at xtsc_initialize_parms
                                                construction time (which, by default, is
                                                10).

   "toggle_logging"                vector<u32>  Optional vector of delay values after
                                                which the state of logging will be
                                                toggled.  The delays are expressed in
                                                terms of the XTSC system clock period.
                                                For example, using the hello_world XTSC
                                                example the following can be used to
                                                turn logging off after elaboration and
                                                then back on again after 20000 clock
                                                periods for 1000 clock periods:
           xtsc-run -set_xtsc_parm=toggle_logging=0,20000,1000 -include=hello_world.inc
           ./hello_world -toggle_logging=0,20000,1000 
                                                Default (unset).

   "turbo"                         bool         This parameter controls the default
                                                setting of the xtsc_core_parms parameter
                                                "SimTurbo".  
                                                Default = false.

   "turbo_max_relaxed_cycles"      u32          This specifies the maximum total amount
                                                that a device may run ahead of actual
                                                simulation time without yielding to the
                                                SystemC kernel when operating in the
                                                functional mode of TurboXim.  This
                                                amount is expressed in terms of 
                                                system clock periods.
                                                See xtsc_get_system_clock_period().
                                                Default = 10000000.

   "turbo_min_sync"                bool         By default, after TurboXim has run ahead
                                                and returns control back to the SystemC
                                                kernel, it synchronizes the SystemC time
                                                by waiting for a number of clock periods
                                                approximately corresponding to the
                                                number of instructions executed.  If
                                                desired, you can limit this wait to a
                                                single cycle by setting this parameter
                                                to true.
                                                Note: Setting this parameter to true
                                                when using xtsc_dma_engine can cause
                                                problems.  See the "turbo" parameter in
                                                xtsc_component::xtsc_dma_engine_parms.
                                                Default = false.

   "xtsc_command_prompt"           bool         If true, then a XTSC command prompt will 
                                                be available at the start of simulation
                                                that gives access to all the XTSC commands
                                                registered via xtsc_register_command().
                                                For example using hello_world:
              ./hello_world -xtsc_command_prompt=true
              cmd: help
              ...
              cmd: ?
              sc_simulation:    sc
              xtsc_core:        core0
              xtsc_memory:      core0_pif
              xtsc_simulation:  xtsc
              cmd: sc wait 100
                   100.0/320: 
              cmd: core0 get_pc
              0x4000005f
              cmd: core0 dasm pc
              0x4000005f: movi.n a3,21
              cmd: core0 dasm next
              0x40000061: wsr.atomctl a3
              cmd: c
                                                For example using xtsc-run:
              xtsc-run -include=hello_world.inc -set_xtsc_parm=xtsc_command_prompt=true
              ...
                                                You can get the XTSC command prompt in
                                                an "empty" simulation which has only the
                                                global xtsc and sc command handlers
                                                using xtsc-run like this:
              xtsc-run -set_xtsc_parm=xtsc_command_prompt=true
              ...
                                                Note: If this parameter is set to true
                                                then "lua_command_prompt" should be
                                                left false.
                                                Note: When debugging, although it is
                                                possible to switch between the xt-gdb
                                                prompt and the XTSC command prompt with
                                                some effort, usually a better technique
                                                is to issue XTSC commands directly from
                                                the xt-gdb prompt using the ISS cmdloop
                                                (so all XTSC commands are prefixed by
                                                "iss xtsc").  For example:
              (xt-gdb) iss help
              (xt-gdb) iss help xtsc
              (xt-gdb) iss xtsc help 
              (xt-gdb) iss xtsc help sc_time_stamp
              (xt-gdb) iss xtsc sc sc_time_stamp
              (xt-gdb) iss xtsc xtsc xtsc_version
                                                Default = false

   "xtsc_finalize_unwind"          bool         If this parameter is set to false, then
                                                the first call to xtsc_finalize() will
                                                finalize simulation and subsequent calls
                                                will be ignored.  If this parameter is
                                                true, then xtsc_finalize() must be called
                                                as many times as xtsc_initialize() has
                                                been called before simulation will
                                                actually be finalized.
                                                Note: Simulation is always initialized
                                                upon the first call to xtsc_initialize()
                                                and subsequent calls are tallied but
                                                otherwise ignored.
                                                Default = false.

   "xtsc_port_type_check_bypass"   bool         If this parameter is set to true, then
                                                xtsc_port_type_check() will log an INFO
                                                level message instead of failing if the
                                                a port's type does not match the
                                                expected xtsc_port_type in the
                                                xtsc_port_type_check() method.
                                                Default = false.

   "unlink_host_shared_memory"     bool         If this parameter is set to true, then
                                                xtsc will unlink the shared memories that
                                                were created during the simulation by 
                                                calling shm_unlink on created shared memories.
                                                The default value of 'false' makes the created
                                                shared memory segments persistent even after
                                                the end of simulation.
                                                Default = false.

    \endverbatim
 *
 * @see xtsc_initialize
 * @see xtsc_parms
 * @see xtsc_core_parms
 */
class XTSC_API xtsc_initialize_parms : public xtsc_parms {
public:

  /**
   * Constructor for an xtsc_initialize_parms object.
   *
   * @param text_logging_config_file    The value for the "text_logging_config_file"
   *                                    parameter.
   */
  xtsc_initialize_parms(const char *text_logging_config_file = NULL);

  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_initialize_parms"; }
};



/**
 * Initialize XTSC simulation.
 *
 * This function should be called before constructing any XTSC modules
 * and before generating any logging messages.  Generally this function
 * should be called from sc_main before constructing any XTSC modules.
 * If you have any global objects then those objects should not generate
 * logging messages in their constructor or they should call this method
 * prior to generating any logging messages.
 *
 * @param init_parms    The parameters used to configure XTSC.
 *
 * @see xtsc_initialize_parms
 */
XTSC_API void xtsc_initialize(const xtsc_initialize_parms& init_parms);



/**
 * Initialize XTSC simulation.
 *
 * This function should be called before constructing any XTSC modules and before
 * generating any logging messages.  This function constructs a default
 * xtsc_initialize_parms object.  Sets the "text_logging_config_file" parameter
 * according to the text_logging_config_file value passed in to this function, and then
 * calls xtsc_initialize with the xtsc_initialize_parms object.
 *
 * @param text_logging_config_file      Value to set the "text_logging_config_file" parameter
 *                                      of the xtsc_initialize_parms object to.
 *
 * @param binary_logging_config_file    The configuration file for binary
 *                                      logging.  If NULL (the default), then
 *                                      binary loggers are disabled for this
 *                                      simulation run.
 *
 * NOTE:  Binary logging is not supported in Xtensa Tools 7.0.
 *
 * @see xtsc_initialize_parms
 */
XTSC_API void xtsc_initialize(const char *text_logging_config_file = NULL, const char *binary_logging_config_file = NULL);


/// Return a copy of the xtsc_initialize_parms used to initialize XTSC
XTSC_API xtsc_initialize_parms xtsc_get_xtsc_initialize_parms();



/// Return true if xtsc_initialize has been called, else return false.
XTSC_API bool xtsc_is_initialized();



/**
 * This function should be called when simulation is over to
 * ensure all resources are properly released and all clients
 * are properly finalized.  Not calling this method can result
 * in, for example, no profile client output being generated.
 */
XTSC_API void xtsc_finalize();



/**
 * This function can be called to reset each sc_process_handle in a vector.  In SystemC
 * versions prior to 2.3.0 this function does nothing.  If simulation is not running
 * (that is, if sc_is_running() returns false) then this function just returns.
 * Typically, a module creates a vector of process handes in its constructor and calls
 * this function with that vector in its reset() method.  For an example, see 
 * xtsc_component::xtsc_memory::m_process_handles, xtsc_component::xtsc_memory::reset(),
 * and the usage of m_process_handles in xtsc_memory.cpp.
 *
 * @see xtsc_reset()
 * @see xtsc_resettable
 */
XTSC_API void xtsc_reset_processes(std::vector<sc_core::sc_process_handle>& process_handles);



/**
 * Set the flag that determines whether sc_stop() will be called when xtsc_finalize() is
 * called.  The default value of the flag is true.  That is, if this method is never
 * called then sc_stop() will be called when xtsc_finalize() is called.
 *
 * Note: Calling sc_stop causes failure during the elaboration phase for some Cadence 
 *       versions (for example, IUS 10.2 s010/s012; but not s017).  Message is:
 *       ncverilog: *E,ELBERR: Error during elaboration (status 250), exiting
 *       
 * @param       call_sc_stop    If true, sc_stop() will be called from xtsc_finalize().
 *
 * @return the previous value of the flag
 */
XTSC_API bool xtsc_set_call_sc_stop(bool call_sc_stop);



/**
 * This method returns the period of the conceptual system clock.
 * @see xtsc_set_system_clock_factor.
 */
XTSC_API sc_core::sc_time xtsc_get_system_clock_period();



/**
 * Get the posedge offset of the conceptual system clock.
 *
 * This methods returns the amount of time by which the first posedge of the conceptual
 * system clock is offset from time 0.  It is computed by multiplying the value of the
 * "posedge_offset_factor" parameter of xtsc_initialize_parms by the SystemC time
 * resolution.  This method must not be called before xtsc_initialize() is called.
 *
 * @see xtsc_initialize_parms.
 */
XTSC_API sc_core::sc_time xtsc_get_system_clock_posedge_offset();



/**
 * This method waits the specified number of system clock periods.
 *
 * This is just a convenience method that calls sc_core::wait() with an sc_time object
 * equal to num_periods times the system clock period.
 *
 * @param       num_periods     The number of system clock periods to wait.
 *
 * @see xtsc_set_system_clock_factor.
 * @see xtsc_get_system_clock_period.
 */
XTSC_API void xtsc_wait(u32 num_periods = 1);



/**
 * Return the number of bits in a system address as set by xtsc_initialize_parms
 * "address_bits"
 */
XTSC_API u32 xtsc_address_bits();



/**
 * Return the number of nibbles in a system address (determined by xtsc_initialize_parms
 * "address_bits").
 */
XTSC_API u32 xtsc_address_nibbles();



/// This method returns true if host processor is big endian, otherwise returns false
XTSC_API bool xtsc_is_big_endian_host();



/**
 * This function returns a unique 64-bit number that can be associate with a new element
 * when it is added to a queue.  This function is meant for use by queue implementations.
 * It is the queue implementation's job to maintain the ticket-to-element association and
 * to return and/or log the ticket with its associated element is popped from the queue.
 *
 * @return a unique 64-bit number.
 *
 * @see xtsc_queue_push_if
 * @see xtsc_queue_pop_if
 */
XTSC_API u64 xtsc_create_queue_ticket();



/**
 * Fire (that is, signal or notify) the specified TurboXim event id.  This function has no
 * effect if called when operating in cycle-accurate (non-TurboXim) mode.
 *
 * When you use the TurboXim simulation engine, you can significantly improve
 * performance by allowing each core to run a large number of instructions at a time.
 * In this relaxed simulation mode, you may want to programmatically control when a core
 * should yield control to allow other cores and other SystemC processes to execute.  To
 * force a core to wait for an event when running in the fast functional simulation
 * mode (TurboXim), you should call the following function from your Xtensa target
 * program:
 *
 *    void xt_iss_event_wait(unsigned eventId);
 *
 * You can fire the event on which a core is waiting by calling
 * xtsc_fire_turboxim_event_id() from the host program and passing the same event ID as
 * was passed in the xt_iss_event_wait() call.  This might be done, for example, from
 * sc_main() or, more typically, from a thread or method process of a SystemC module.
 *
 * Alternatively, the event on which a core is waiting can be fired by calling
 *
 *    void xt_iss_event_fire(unsigned eventId);
 *
 * from the target program running on another core and passing the same eventId number.
 *
 * Prototypes for both target functions are provided in the <xtensa/sim.h> header file.
 *
 */
XTSC_API void xtsc_fire_turboxim_event_id(u32 turboxim_event_id);



/**
 * This method is used to set an absolute time barrier for use by modules operating in
 * fast functional mode.  When the current SystemC simulation time equals or exceeds
 * this barrier, then modules should not run ahead of the current SystemC simulation
 * time.
 *
 * If this method is not called then there is effectively no absolute time barrier.
 *
 * This function is commonly called before calling sc_start() when sampled simulation is
 * to be used.
 *
 * @param delta         This time is added to the current SystemC time to compute the
 *                      absolute time barrier.
 */
XTSC_API void xtsc_set_relaxed_simulation_barrier(const sc_core::sc_time& delta);



/**
 * This method returns the absolute simulation time barrier beyond which modules should
 * cease running ahead of the current SystemC simulation time when in fast functional
 * mode.  This is the absolute simulation time when relaxed simulation (i.e. "running
 * ahead") should cease.
 *
 * If the xtsc_set_relaxed_simulation_barrier() method is never called, then this method
 * will return the maximum possible SystemC simulation time less one SystemC time
 * resolution, that is ((2^64) - 1) times the SystemC time resolution.
 */
XTSC_API sc_core::sc_time xtsc_get_relaxed_simulation_barrier();



/**
 * Set the amount of "equivalent time" that a module is allowed to run ahead of the
 * actual SystemC simulation time when operating in fast functional mode.
 *
 * This method should be called before simulation begins or should be left at its
 * default value of SC_ZERO_TIME.
 *
 * When this interval is SC_ZERO_TIME, then cores and other modules should not run ahead
 * of the current SystemC simulation time.
 *
 * @param       interval        Maximum amount of "equivalent time" that a module may
 *                              run ahead of the current SystemC simulation time.
 */
XTSC_API void xtsc_set_relaxed_simulation_interval(const sc_core::sc_time &interval);



/**
 * Get the amount of "equivalent time" that a module is allowed to run ahead of the
 * actual SystemC simulation time when operating in fast functional mode as set by
 * the xtsc_set_relaxed_simulation_interval() method.
 *
 * This method does not take the absolute simulation time barrier into account.
 * Use xtsc_get_remaining_relaxed_simulation_time() to get the max
 * duration that a module should use.
 */
XTSC_API sc_core::sc_time xtsc_get_relaxed_simulation_interval(void);



/**
 * This method returns the maximum amount of time that a module may run ahead of the
 * current SystemC simulation time.
 *
 * When a system is executing in fast functional mode, cores and other modules are
 * allowed to run ahead of the current simulation time.  The amount of "equivalent time"
 * that a module is allowed to run ahead of the actual SystemC simulation time is
 * limited by two things:
 *
 * First, a module must not run ahead of the current simulation time by an amount
 * greater then the interval specified by the xtsc_set_relaxed_simulation_interval()
 * method.
 *
 * Second, a module should not run ahead when the current SystemC simulation time
 * equals or exceeds the absolute simulation time barrier as set by the
 * xtsc_set_relaxed_simulation_barrier() method.
 *
 * If the xtsc_set_relaxed_simulation_interval() method has not been called (or if it
 * was called with an argument of SC_ZERO_TIME) or if the absolute simulation time
 * barrier as set by the xtsc_set_relaxed_simulation_barrier() method has already
 * arrived or passed, then this method returns SC_ZERO_TIME.  Otherwise, this method
 * returns the smaller of the interval set by the xtsc_set_relaxed_simulation_interval()
 * method and the difference between the current SystemC simulation time and the
 * absolute simulation time barrier as set by the xtsc_set_relaxed_simulation_barrier()
 * method.
 *
 */
XTSC_API sc_core::sc_time xtsc_get_remaining_relaxed_simulation_time(void);



#ifndef DOXYGEN_SKIP
// Deprecated function names being maintained for backward compatibility

inline void xtsc_set_max_relaxed_duration(const sc_core::sc_time &interval) { xtsc_set_relaxed_simulation_interval(interval); }

inline sc_core::sc_time xtsc_get_max_relaxed_duration(void) { return xtsc_get_relaxed_simulation_interval(); }

inline void xtsc_set_relaxed_simulation_duration(const sc_core::sc_time& delta) { xtsc_set_relaxed_simulation_barrier(delta); }

inline sc_core::sc_time xtsc_get_relaxed_simulation_time_limit() { return xtsc_get_relaxed_simulation_barrier(); }

inline sc_core::sc_time xtsc_get_remaining_relaxed_duration(void) { return xtsc_get_remaining_relaxed_simulation_time(); }
#endif



/**
 * Dump the name-value pairs (optionally limited to pattern) in the user-defined state
 * map to the specified ostream object.
 *
 * @param       os              The ostream object to which the user-defined state map
 *                              should be dumped.
 *
 * @param       pattern         Optional pattern of names to match.  Any asterisk (*)
 *                              found in the pattern is treated as a wildcard which
 *                              matches any number of any characters in any combination.
 *                              If pattern is the empty string then all name-value pairs
 *                              are dumped.
 *
 * @see xtsc_user_state_set
 * @see xtsc_user_state_get
 * @see xtsc_pattern_match
 */
XTSC_API void xtsc_user_state_dump(std::ostream& os, const std::string& pattern = "");



/**
 * Get the value associated with the specified name in the user-defined state map.
 *
 * @param       name            The state name. May only contain non-space, printable
 *                              characters.  If this name is not found in the
 *                              user-defined state table then the empty string will be
 *                              returned.
 *
 * @see xtsc_user_state_set
 * @see xtsc_user_state_dump
 */
XTSC_API std::string xtsc_user_state_get(const std::string& name);



/**
 * Add or replace a name-value pair in the user-defined state map.
 *
 * XTSC maintains a map of name-value pairs in which users may store arbitrary state.
 * This can be used, for example, to allow different parts of the simulator (e.g. Lua
 * scripts, the XTSC or lua command prompts, an xtsc-run include script, and target
 * programs using simcalls) to communicate with each other.
 *
 * @param       name            The state name. May only contain non-space, printable
 *                              characters.
 *
 * @param       value           The desired state value.
 *
 * Note: The xtsc-run program supports the following command to allow setting
 * user-defined state:
 * \verbatim
     --user_state=<Name>=<Value>
   \endverbatim
 *
 * Note: Lua code on a #lua line or inside a #lua_beg/#lua_end block of an
 * xtsc_script_file can access user-defined state using Lua functions in the xtsc table
 * (this is the only way that Lua code in an xtsc-run script file can read user-defined
 * state).  For example:
 * \verbatim
     #lua_beg
       MyList = xtsc.user_state_get("MyList")
       xtsc.user_state_set("MyList", MyList .. "AnotherItem,")
     #lua_end
   \endverbatim
 *
 * @see xtsc_user_state_get
 * @see xtsc_user_state_dump
 */
XTSC_API void xtsc_user_state_set(const std::string& name, const std::string& value);



/**
 * This is needed for when XTSC is being used with some commercial SystemC simulators.
 */
XTSC_API bool xtsc_trap_port_binding_failures(bool trap);



/**
 * This method sets the log level for constructor logging.  Normally, the XTSC module
 * constructors (xtsc_core, xtsc_memory, xtsc_queue, etc) will log certain construction
 * information and parameters at INFO_LOG_LEVEL.  This method can be called to change
 * that to a different log level.
 *
 * @param       log_level       The new log level for XTSC module constructor logging.
 *
 * @return previous value
 */
XTSC_API log4xtensa::LogLevel xtsc_set_constructor_log_level(log4xtensa::LogLevel log_level);



/// Get the current log level for constructor logging
XTSC_API log4xtensa::LogLevel xtsc_get_constructor_log_level();



/**
 * Return value set by xtsc_enable_text_logging().
 */
XTSC_API bool xtsc_is_text_logging_enabled();



/**
 * Turn text logging on or off for XTSC_INFO and lower.
 *
 * This method enables or disables text logging for XTSC_INFO and lower.  If text
 * logging is disabled, the XTSC_INFO and lower macros do not generate logging messages,
 * but XTSC_NOTE and higher macros still work as usual.  Calling this method with
 * enable_logging set to false results in near zero logging facility overhead within the
 * XTSC libraries.  Initial setting is true.
 *
 * @param       enable_logging  If true, the default, text logging is enabled as normal.
 *                              Otherwise, text logging is disabled for XTSC_INFO,
 *                              XTSC_VERBOSE, XTSC_DEBUG, and XTSC_TRACE macros.
 *
 * Note: If logging is not configured for INFO|VERBOSE|DEBUG|TRACE then calling this
 *       method with enable_logging=true is largely pointless and will result in a
 *       one-time warning being issued.
 *
 * @return previous value
 */
XTSC_API bool xtsc_enable_text_logging(bool enable_logging = true);



/**
 * Get the number of nibbles that should be used to log AXI Transaction ID.
 * From xtsc_initialize_parms parameter "text_logging_axi_id_width".
 */
XTSC_API u32 xtsc_get_text_logging_axi_id_width();



/**
 * Get the number of digits that should be used to log AXI transfer counts.
 * From xtsc_initialize_parms parameter "text_logging_axi_cnt_width".
 */
XTSC_API u32 xtsc_get_text_logging_axi_cnt_width();



/// Set number of digits after decimal point used when logging simulation time
XTSC_API void xtsc_set_text_logging_time_precision(u32 num_decimal_digits);



/// Get number of digits after decimal point used when logging simulation time
XTSC_API u32 xtsc_get_text_logging_time_precision();



/// Set total number of characters used when logging simulation time
XTSC_API void xtsc_set_text_logging_time_width(u32 num_characters);



/// Get total number of characters used when logging simulation time
XTSC_API u32 xtsc_get_text_logging_time_width();



/**
 * Set the number of least-significant decimal digits of the delta cycle
 * count to be displayed when logging.
 */
XTSC_API void xtsc_set_text_logging_delta_cycle_digits(u32 num_digits);



/**
 * Get the number of least-significant decimal digits of the delta cycle
 * count to be displayed when logging.
 */
XTSC_API u32 xtsc_get_text_logging_delta_cycle_digits();



/**
 *  Put the formatted delta cycle count into string reference buf and return it.
 *
 *  Note: This function was changed with the RD-2011.1 release to require the user
 *        to provide a non-const string buffer.  The caller should create a string
 *        on the stack and pass it to this function.  This is needed for multi-thread
 *        safety.  Although SystemC is single-thread, XTSC will create other OS-level
 *        threads if an xtsc_core has debugging enabled for its target program.  For 
 *        an example usage, see the XTSC_INFO macro.
 */
XTSC_API std::string& xtsc_log_delta_cycle(std::string &buf);



#ifndef DOXYGEN_SKIP
/// Determine if binary logging is enabled
XTSC_API bool xtsc_is_binary_logging_enabled();



/**
 * Turn binary logging on or off.
 *
 * This method enables or disables binary logging.
 *
 * @param       enable_logging  If true, the default, binary logging is enabled.
 *                              Otherwise, binary logging is disabled.
 *
 * @return previous value
 */
XTSC_API bool xtsc_enable_binary_logging(bool enable_logging = true);



/// Dump currently registered binary loggers
XTSC_API void xtsc_dump_binary_loggers(std::ostream& os = std::cout);
#endif // DOXYGEN_SKIP



/// Dump currently registered text loggers
XTSC_API void xtsc_dump_text_loggers(std::ostream& os = std::cout);



/**
 * This method sets the flag that determines the order in which data is
 * dumped by the xtsc_hex_dump(u32, const u8 *, ostream&) method.  The
 * initial (default) value of this flag is true.
 *
 * @param       left_to_right   If true, data is dumped in the order:
 *                              buffer[0], buffer[1], ..., buffer[size8-1].
 *                              If false, data is dumped in the order:
 *                              buffer[size8-1], buffer[size8-2], ..., buffer[0].
 *
 * @return the old (previous) value of the flag.
 */
XTSC_API bool xtsc_set_hex_dump_left_to_right(bool left_to_right);



/**
 * This method returns the flag that determines the order in which data is
 * dumped by the xtsc_hex_dump(u32, const u8 *, ostream&) method.
 *
 * @return If true, data is dumped in the order:
 *            buffer[0], buffer[1], ..., buffer[size8-1].
 *         If false, data is dumped in the order:
 *            buffer[size8-1], buffer[size8-2], ..., buffer[0].
 */
XTSC_API bool xtsc_get_hex_dump_left_to_right();



/**
 * This method dumps the specified number of bytes from the data buffer in hex
 * format (two hex nibbles and a space for each byte in the buffer).  The data
 * is dumped in the order specified by the xtsc_get_hex_dump_left_to_right()
 * method.
 *
 * @param       size8           The number of bytes of data to dump.
 *
 * @param       buffer          The buffer of data.
 *
 * @param       os              The ostream object to which the data is to be
 *                              dumped.
 */
XTSC_API void xtsc_hex_dump(u32 size8, const u8 *buffer, std::ostream& os = std::cout);



/**
 * This method dumps the specified number of bytes from the data buffer.  Each
 * line of output is divided into three columnar sections, each of which is
 * optional.  The first section contains an address.  The second section contains
 * a hex dump of some (possibly all) of the data (two hex nibbles and a space for
 * each byte from the buffer).  The third section contains an ASCII dump of the
 * same data.
 *
 * @param       left_to_right           If true, the data is dumped in the order:
 *                                      buffer[0], buffer[1], ..., buffer[bytes_per_line-1].
 *                                      If false, the data is dumped in the order:
 *                                      buffer[bytes_per_line-1], buffer[bytes_per_line-2],
 *                                      ..., buffer[0].
 *
 * @param       size8                   The number of bytes of data to dump.
 *
 * @param       buffer                  The buffer of data.
 *
 * @param       os                      The ostream object to which the data is to be
 *                                      dumped.
 *
 * @param       bytes_per_line          The number of bytes to dump on each line of output.
 *                                      If bytes_per_line is 0 (the default) then all size8
 *                                      bytes are dumped on a single line with no newline at
 *                                      the end.  If bytes_per_line is non-zero, then all
 *                                      lines of output end in newline.
 *
 * @param       show_address            If true, the first columnar section contains an
 *                                      address printed as an 8-hex-digit number with a 0x
 *                                      prefix.  If false, the first columnar section is null
 *                                      and takes no space in the output.
 *
 * @param       start_byte_address      If show_address is true, the first line of output
 *                                      starts with start_byte_address, the second line of
 *                                      output starts with start_byte_address+bytes_per_line,
 *                                      and so on.  If show_address is false, this parameter
 *                                      is ignored.
 *
 * @param       show_hex_values         If true, the second (middle) columnar section of
 *                                      hex data values is printed.  If false, the second
 *                                      columnar section is null and takes no space in the
 *                                      output.
 *
 * @param       do_column_heading       If true, print byte position column headings over
 *                                      the hex values section.  If false, no column headings
 *                                      are printed.  If show_hex_values is false, then the
 *                                      do_column_heading value is ignored and no column
 *                                      headings are printed.
 *
 * @param       show_ascii_values       If true, the third (last) columnar section of ASCII
 *                                      data values is printed (if an ASCII value is a
 *                                      non-printable character a period is printed).  If
 *                                      show_ascii_values is false, the third columnar
 *                                      section is null and takes no space in the output.
 *
 * @param       initial_skipped_bytes   Skip initial_skipped_bytes bytes on the first line
 *                                      in the second (hex) and third (ASCII) columnar
 *                                      sections.
 */
XTSC_API
void xtsc_hex_dump(bool                 left_to_right,
                   u32                  size8,
                   const u8            *buffer,
                   std::ostream&        os                      = std::cout,
                   u32                  bytes_per_line          = 0,
                   bool                 show_address            = false,
                   xtsc_address         start_byte_address      = 0x0,
                   bool                 show_hex_values         = true,
                   bool                 do_column_heading       = false,
                   bool                 show_ascii_values       = false,
                   u32                  initial_skipped_bytes   = 0);



/**
 * This method dumps the specified number of bytes from the data buffer formatted as
 * decimal words of the specified word_size.  Each line of output optionally starts with
 * the address of that line.
 *
 * @param       size8                   The number of bytes of data to dump.
 *
 * @param       buffer                  The buffer of data.
 *
 * @param       os                      The ostream object to which the data is to be
 *                                      dumped.
 *
 * @param       word_size               The number of bytes in a word (1|2|4|8).
 *
 * @param       positive                If true, each word is treated is an unsigned
 *                                      value (u8, u16, u32, or u64, depending on
 *                                      word_size).  If false, each word is treated as a
 *                                      signed value (i8, i16, i32, or i64).
 *
 * @param       words_per_line          The number of words to dump on each line of
 *                                      output.  If words_per_line is 0 then all size8
 *                                      bytes are dumped on a single line with no
 *                                      newline at the end.  If words_per_line is
 *                                      non-zero, then all lines of output end in
 *                                      newline.
 *
 * @param       show_address            If true, each line starts with the address
 *                                      printed as an 8-hex-digit number with a 0x
 *                                      prefix.  If false, the address is omitted.
 *
 * @param       start_byte_address      If show_address is true, then each line of
 *                                      output starts at the address given by:
 *                                       start_byte_address + words_per_line*word_size*L
 *                                      where L is the 0-based line number.  If
 *                                      show_address is false, this parameter is ignored.
 *
 * @param       do_column_heading       If true, print byte position column headings over
 *                                      the hex values section.  If false, no column
 *                                      headings are printed.  If show_hex_values is
 *                                      false, then the do_column_heading value is
 *                                      ignored and no column headings are printed.
 *
 * @param       initial_skipped_bytes   Skip initial_skipped_bytes bytes on the first line
 *                                      in the second (hex) and third (ASCII) columnar
 *                                      sections.
 */
XTSC_API
void xtsc_dec_dump(u32            size8,
                   const u8      *buffer,
                   std::ostream&  os                    = std::cout,
                   u32            word_size             = 1,
                   bool           positive              = false,
                   u32            words_per_line        = 0,
                   bool           show_address          = false,
                   xtsc_address   start_byte_address    = 0x0,
                   bool           do_column_heading     = false,
                   u32            initial_skipped_words = 0);



/// Utility method to convert a string of comma-separated values to a vector<string> with leading/trailing whitespace removed
XTSC_API void xtsc_strtostrvector(const std::string& str, std::vector<std::string>& vec, char sep = ',', const std::string& whitespace = " \t");



/// Utility method to convert a string of comma-separated values to a vector<u32> (throws xtsc_exception if non-numeric)
XTSC_API void xtsc_strtou32vector(const std::string& str, std::vector<u32>& vec, char sep = ',');



/// Utility method to convert a string to a u32 (or throw xtsc_exception)
XTSC_API u32 xtsc_strtou32(const std::string& str);



/// Utility method to convert a string to a u64 (or throw xtsc_exception)
XTSC_API u64 xtsc_strtou64(const std::string& str);



/// Utility method to convert a string to a i32 (or throw xtsc_exception)
XTSC_API i32 xtsc_strtoi32(const std::string& str);



/// Utility method to convert a string to a i64 (or throw xtsc_exception)
XTSC_API i64 xtsc_strtoi64(const std::string& str);



/// Utility method to convert a string to a double (or throw xtsc_exception)
XTSC_API double xtsc_strtod(const std::string& str);



/// Utility method to find log2 of a value rounded up
XTSC_API u32 xtsc_ceiling_log2(u32 value);



/**
 * Utility method to return a string which matches the input string except that
 * bracketed array indices containing less then 10 decimal digits, if any, will be
 * zero-extended to 10 digits so as to be suitable for sorting in a situation where it
 * is desired that, for example, "foo[10]" comes after "foo[9]" instead of after
 * "foo[1]".
 *
 * For example:
 * \verbatim
      Input (str)       Returns
      ---------------   -----------------------------
      "foo"             "foo"
      "foo[42]bar"      "foo[0000000042]bar"
      "[42]foo"         "[0000000042]foo"
      "foo[7][103]"     "foo[0000000007][0000000103]"
      "foo[0x42]"       "foo[0x42]"
   \endverbatim
 */
XTSC_API std::string xtsc_zero_extend_array_indices(const std::string& str);



/**
 * Run the specified Lua script file in its own SystemC thread process.  This function
 * may only be called prior to the SystemC before_end_of_elaboration() callback.
 *
 * @param lua_script_file       The name of the Lua script file to run in its own
 *                              SystemC thread.
 */
XTSC_API void xtsc_add_lua_script_file(const std::string &lua_script_file);




/**
 * Utility method to get the current user name.
 *
 * @param name          The instance name of the sc_module requesting the current user
 *                      name (used for error reporting).
 *
 * @param kind          The kind of sc_module requesting the current user name (used for
 *                      error reporting).
 *
 * @param unknown       On Linux, the user name to be returned if the OS is unable to
 *                      provide a user name.  This parameter is ignored on MS Windows.
 *
 * @return the current user name.
 */
XTSC_API std::string xtsc_get_user_name(const std::string& name, const std::string& kind, const std::string& unknown = "UNKNOWN_COWBOY");



/**
 * Utility method to get a pointer to host OS shared memory.
 *
 * @param sm_name       The name of the shared memory.
 *
 * @param num_bytes     The number of bytes in the shared memory.
 *
 * @param name          The instance name of the sc_module requesting the shared memory
 *                      (used for error reporting).
 *
 * @param kind          The kind of sc_module requesting the shared memory (used for
 *                      error reporting).
 *
 * @param base_address  The optional target physical starting byte address of the memory
 *                      modelled by this shared memory.
 *
 * @return a pointer to the shared memory.
 */
XTSC_API u8 *xtsc_get_shared_memory(const std::string&  sm_name,
                                    u64                 num_bytes,
                                    const std::string&  name,
                                    const std::string&  kind,
                                    xtsc_address        base_address = 0x00000000);



/**
 * Utility method to convert a raw byte array to an existing sc_unsigned.
 *
 * @param buf           Pointer to a byte array whose size is at least (L+7)/8 bytes
 *                      (where L = value.length()).
 *
 * @param value         A reference to an existing sc_unsigned value.
 */
XTSC_API void xtsc_byte_array_to_sc_unsigned(const xtsc::u8 *buf, sc_dt::sc_unsigned& value);



/**
 * Utility method to convert an sc_unsigned to an existing raw byte array.
 *
 * @param value         A reference to an existing sc_unsigned value.
 *
 * @param buf           Pointer to a byte array whose size is at least (L+7)/8 bytes
 *                      (where L = value.length()).
 */
XTSC_API void xtsc_sc_unsigned_to_byte_array(const sc_dt::sc_unsigned& value, xtsc::u8 *buf);



/// Maximum size of an Xtensa memory bus in bytes
static const u32 xtsc_max_bus_width8 = 512/8;

/// Maximum size of a register in bytes
static const u32 xtsc_max_register_width8 = 2048/8;




/**
 * Class for registering TurboXim simulation mode switching interfaces.
 */
class XTSC_API xtsc_switch_registration {
 public:
  sc_core::sc_object        *m_object;
  xtsc::xtsc_mode_switch_if *m_switch_if;

  /**
   * constructor for a module_name, switching interface, group triplet
   *
   * @param module_name     The module that is registering the switch
   * @param switch_in       The interface class that implements mode switching
   */
  xtsc_switch_registration(sc_core::sc_object &obj,
                           xtsc::xtsc_mode_switch_if &switch_if,
                           std::string /* unused */);
};



/**
 *  Registration function for registering TurboXim simulation mode switching interfaces.
 *
 *  @param registration    a module name, interface, switching group name
 *                         triplet to register.
 */
XTSC_API void xtsc_register_mode_switch_interface(const xtsc::xtsc_switch_registration &registration);



/**
 *  Fill a vector will all of the registered switch groups.
 *
 */
XTSC_API void xtsc_get_registered_mode_switch_interfaces(std::vector<xtsc::xtsc_switch_registration> &ifs);



/**
 *  Switch all modules in all switch groups that have registered a
 *  simulation mode switching interface.  This function will throw an
 *  exception if one or more of the modules was unable to switch.  It
 *  should always be valid to switch mode before simulation starts.
 *  After simulation starts, use the mode-switching protocol to relax
 *  the transactions in the system before invoking this function.
 *  Throws an exception if any of the registered modules cannot switch.
 *
 * @param mode      If mode is XTSC_CYCLE_ACCURATE, then switch to
 *                  cycle-accurate (non-TurboXim for cores) mode.  If mode is
 *                  XTSC_FUNCTIONAL, then switch to functional mode
 *                  (TurboXim for cores).
 */
XTSC_API void xtsc_switch_sim_mode(xtsc::xtsc_sim_mode mode);



/**
 *  Polling-based dynamic simulation switching preparation.
 *
 *  This function should be used prior to switching the simulation
 *  mode as part of the mode-switching protocol to remove or drain
 *  transactions from the system.
 *
 *  Prepare to switch all modules in all switch groups that have
 *  registered a simulation mode switching interface.  This function
 *  will return true if all modules are ready to switch.  Otherwise,
 *  it will return false.  If it returns false, the switcher should
 *  wait at least a cycle before trying again.  Once all modules are
 *  ready, there may still be transactions in passive modules.  The
 *  user should wait enough cycles for all passive modules to
 *  propagate transactions through the system, try one more time, and
 *  switch if all modules are still ready.
 *
 * @param mode      If mode is XTSC_CYCLE_ACCURATE, then switch to
 *                  cycle-accurate (non-TurboXim for cores) mode.  If mode is
 *                  XTSC_FUNCTIONAL, then switch to functional mode
 *                  (TurboXim for cores).
 */
XTSC_API bool xtsc_prepare_to_switch_sim_mode(xtsc::xtsc_sim_mode mode);



/**
 * Compute the swizzle value from memory storage.
 *
 * @param buf  the pointer to memory storage
 *
 * @return the swizzle value to use in xtsc_fast_access_request::allow_raw_access()
 *         or 0xffffffff if no swizzle matches.
 */
XTSC_API u32 xtsc_compute_fast_access_swizzle(const u32 *buf);



/**
 * Get the portion of the path before the last path separator.  On Linux this method
 * just returns dirname(path).
 */
XTSC_API char *xtsc_dirname(char *path);



/**
 * Get the portion of the path after the last path separator.  On Linux this method 
 * just returns basename(path).
 */
XTSC_API char *xtsc_basename(char *path);



/**
 * Utility function to get the absolute path of an existing directory.
 *
 * @param directory  The directory whose absolute path is desired.  directory may be an
 *                   absolute or relative path.
 *
 * @return the absolute path of the directory containing directory
 */
XTSC_API std::string xtsc_get_absolute_directory(const std::string& directory);



/**
 * Utility function to get the absolute path of the directory containing an existing file.
 *
 * @param file_name  The file whose absolute path is desired.  file_name may include an
 *                   absolute or relative path.
 *
 * @return the absolute path of the directory containing file_name
 */
XTSC_API std::string xtsc_get_absolute_path(const std::string& file_name);



/**
 * Utility function to get the absolute path and file name of an existing file.
 *
 * @param file_name  The file whose absolute path is desired.  file_name may include an
 *                   absolute or relative path.
 *
 * @return the absolute path to file_name
 */
XTSC_API std::string xtsc_get_absolute_path_and_name(const std::string& file_name);



/**
 * Utility function to load a shared library (Linux shared object, MS Windows DLL) and
 * then get and return the address of a symbol in the library.  An xtsc_exception is
 * thrown if the library cannot be loaded or if the symbol is not found in the library.
 *
 * @param library_name          The library to be loaded.
 *
 * @param add_library_extension If true, up to two attempts will be made to find the
 *                              library.  For the first attempt an extension of ".so",
 *                              ".dll", or "d.dll" will be added to library_name
 *                              depending upon whether the environment is Linux, MS
 *                              Windows Release build, or MS Windows Debug build
 *                              (respectively).  If no such library is found, then a
 *                              second attempt will be made after adding an extension of
 *                              "_sh.so", "_sh.dll", or "_shd.dll" to library_name
 *                              depending on the environment (as above).  In addition,
 *                              if the environement is Linux and if library_name
 *                              contains no directory delimiter (no forward slash), then
 *                              a prefix of "lib" will be added to library_name.  This
 *                              second attempt matches the name format used by the
 *                              standard XTSC plugins installed in XtensaTools.  As an
 *                              example, if this parameter is true, and library_name is
 *                              "xtsc_widget", then the call to dlopen (Linux) or
 *                              LoadLibrary (MS Windows) will use:
 * \verbatim
                            Environment          First Attempt      Second Attempt
                            ------------------   ----------------   --------------------
                            Linux                xtsc_widget.so     libxtsc_widget_sh.so
                            MS Windows Release   xtsc_widget.dll    xtsc_widget_sh.dll
                            MS Windows Debug     xtsc_widgetd.dll   xtsc_widget_shd.dll
   \endverbatim
 *
 * @param symbol_name           The symbol to be found in the library.
 *
 * @return the address of symbol_name.
 */
XTSC_API void *xtsc_load_library(const std::string& library_name, bool add_library_extension, const std::string& symbol_name);



/**
 * This function splits up a multi-line message into multiple calls
 * to the TextLogger::log() method (one call per line).
 *
 * @param       logger          The TextLogger object.
 *
 * @param       log_level       The log level of this message.
 *
 * @param       msg             The message to log.
 *
 * @param       indent          The number of spaces to indent all
 *                              lines except the first one.
 */
XTSC_API 
void xtsc_log_multiline(log4xtensa::TextLogger& logger,
                        log4xtensa::LogLevel    log_level,
                        const std::string&      msg,
                        u32                     indent = 0);



#ifndef DOXYGEN_SKIP
// This class should not be used except by the friend classes
class XTSC_API xtsc_length_context {
private:
  xtsc_length_context(u32 length);
  void end();
  sc_dt::sc_length_context length_context;
  friend class xtsc_signal_sc_bv_base;
  friend class xtsc_signal_sc_uint_base;
};
#endif // DOXYGEN_SKIP



/**
 * Pin-level signal for connecting to a TIE export state, TIE import wire, or
 * system-level I/O of xtsc_core.
 *
 * This convenience class implements a pin-level signal for connecting to a TIE export
 * state, TIE import wire, or system-level I/O of an xtsc_core.  The advantage of using
 * this class instead of sc_signal<sc_bv_base> (from which it inherits) is that you can
 * directly specify the bit-width of the underlying sc_bv_base using the width1
 * constructor parameter instead of having to indirectly specify it through a separate
 * sc_length_context object.
 *
 * This is a convenience class.  If desired you may use sc_signal<sc_bv_base> in lieu of
 * xtsc_signal_sc_bv_base for connecting to a pin-level input or output of xtsc_core.
 * In either case, however, the TIE export state, TIE import wire, or system-level I/O
 * interfaces must be named in the xtsc_core object's "SimPinLevelInterfaces" parameter
 * in order for it to be available as a pin-level input or output.
 *
 * @see xtsc_core::How_to_do_output_pin_binding;
 * @see xtsc_core::How_to_do_input_pin_binding;
 */
class XTSC_API xtsc_signal_sc_bv_base : public xtsc_length_context, public sc_core::sc_signal<sc_dt::sc_bv_base> {
public:
  xtsc_signal_sc_bv_base(u32 width1);
  xtsc_signal_sc_bv_base(const char *name, u32 width1);

  /// Get the width in bits of this signal.
  u32 get_bit_width() const { return m_width1; }

  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_signal_sc_bv_base"; }

private:
  u32   m_width1;
};



/**
 * Floating signal for a capped (unused) TIE export state or import wire.
 *
 * This class implements the signal used to cap an unconnected TIE export state or TIE
 * import wire.
 */
class XTSC_API xtsc_signal_sc_bv_base_floating: public xtsc_signal_sc_bv_base {
public:
  xtsc_signal_sc_bv_base_floating(u32 width1, log4xtensa::TextLogger& text);
  xtsc_signal_sc_bv_base_floating(const char *name, u32 width1, log4xtensa::TextLogger& text);

  const sc_dt::sc_bv_base& read() const;
  void write(const sc_dt::sc_bv_base& value);

  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_signal_sc_bv_base_floating"; }

private:
  log4xtensa::TextLogger&       m_text;
};



/**
 * Pin-level signal for connecting certain pin-level memory ports.
 *
 * This convenience class implements a pin-level signal for connecting pin-level memory
 * ports of template type sc_unit_base.  The advantage of using this class instead of
 * sc_signal<sc_uint_base> (from which it inherits) is that you can directly specify the
 * bit-width of the underlying sc_uint_base using the width1 constructor parameter
 * instead of having to indirectly specify it through a separate sc_length_context
 * object.
 *
 */
class XTSC_API xtsc_signal_sc_uint_base : public xtsc_length_context, public sc_core::sc_signal<sc_dt::sc_uint_base> {
public:
  xtsc_signal_sc_uint_base(u32 width1);
  xtsc_signal_sc_uint_base(const char *name, u32 width1);

  /// Get the width in bits of this signal.
  u32 get_bit_width() const { return m_width1; }

  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_signal_sc_uint_base"; }

private:
  u32   m_width1;
};



#ifndef DOXYGEN_SKIP
struct XTSC_API is_true  { static const bool value = true;  };
struct XTSC_API is_false { static const bool value = false; };

template <typename T> struct is_integral                        : is_false {};
template <>           struct is_integral  <              bool > : is_true  {};
template <>           struct is_integral  <              char > : is_true  {};
template <>           struct is_integral  <  signed      char > : is_true  {};
template <>           struct is_integral  <  signed      short> : is_true  {};
template <>           struct is_integral  <  signed      int  > : is_true  {};
template <>           struct is_integral  <  signed      long > : is_true  {};
template <>           struct is_integral  <  signed long long > : is_true  {};
template <>           struct is_integral  <unsigned      char > : is_true  {};
template <>           struct is_integral  <unsigned      short> : is_true  {};
template <>           struct is_integral  <unsigned      int  > : is_true  {};
template <>           struct is_integral  <unsigned      long > : is_true  {};
template <>           struct is_integral  <unsigned long long > : is_true  {};

template <typename T> struct is_integral64                      : is_false {};
template <>           struct is_integral64<  signed long long > : is_true  {};
template <>           struct is_integral64<unsigned long long > : is_true  {};
#endif // DOXYGEN_SKIP



/**
 * Base class for converting an sc_out<sc_bv_base> to sc_out<T>.
 *
 * Note: This class is not used directly, instead use
 *       xtsc_sc_out_sc_bv_base_adapter.
 */
template <int W, typename T>
class xtsc_sc_out_sc_bv_base_adapter_base : public sc_core::sc_module, public sc_core::sc_signal_inout_if<sc_dt::sc_bv_base> {
public:

  /// Bind the sc_out<sc_bv_base> to here
  sc_core::sc_export<sc_core::sc_signal_inout_if<sc_dt::sc_bv_base> >   m_sc_export;

  /// Bind this to an sc_signal<T> or to a higher-level (outer) sc_out<T>
  sc_core::sc_out<T>                                                    m_sc_out;

  xtsc_sc_out_sc_bv_base_adapter_base(sc_core::sc_module_name module_name) :
    sc_module   (module_name),
    m_sc_export ("m_sc_export"),
    m_sc_out    ("m_sc_out"),
    m_value     (W)
  {
    m_value = 0;
    m_sc_export(*this);
  }

  // get the default event
  virtual const sc_core::sc_event& default_event() const {
    return m_sc_out.default_event();
  }

  // get the value changed event
  virtual const sc_core::sc_event& value_changed_event() const {
    return m_sc_out.value_changed_event();
  }

  // was there an event?
  virtual bool event() const {
    return m_sc_out.event();
  }

  // get a reference to the current value (for tracing)
  virtual const sc_dt::sc_bv_base& get_data_ref() const {
    return read();
  }

  // read the current value
  virtual const sc_dt::sc_bv_base& read() const  = 0;

  // write the new value
  virtual void write(const sc_dt::sc_bv_base& value)  = 0;


protected:

  mutable sc_dt::sc_bv_base     m_value;

};



#ifndef DOXYGEN_SKIP
// Derived template for converting vector to/from integral types
template <int W, typename T, bool integral = true>
class xtsc_sc_out_sc_bv_base_adapter_derived : public xtsc_sc_out_sc_bv_base_adapter_base<W, T> {
public:
  xtsc_sc_out_sc_bv_base_adapter_derived(sc_core::sc_module_name module_name) : xtsc_sc_out_sc_bv_base_adapter_base<W, T>
                                                                                   (module_name) { }
  virtual const sc_dt::sc_bv_base& read() const { this->m_value = this->m_sc_out.read(); return this->m_value; }
  virtual void write(const sc_dt::sc_bv_base& value) { this->m_sc_out.write((T) value.to_uint64()); }
};



// Specialization for converting vector to/from non-integral types:  T must have a conversion to/from sc_bv_base
template <int W, typename T>
class xtsc_sc_out_sc_bv_base_adapter_derived<W, T, false> : public xtsc_sc_out_sc_bv_base_adapter_base<W, T> {
public:
  xtsc_sc_out_sc_bv_base_adapter_derived(sc_core::sc_module_name module_name) : xtsc_sc_out_sc_bv_base_adapter_base<W, T>
                                                                                   (module_name) { }
  virtual const sc_dt::sc_bv_base& read() const { this->m_value = this->m_sc_out.read(); return this->m_value; }
  virtual void write(const sc_dt::sc_bv_base& value) { this->m_sc_out.write(value); }
};



// Specialization for converting scalar to/from integral types
template <typename T>
class xtsc_sc_out_sc_bv_base_adapter_derived<1, T, true> : public xtsc_sc_out_sc_bv_base_adapter_base<1, T> {
public:
  xtsc_sc_out_sc_bv_base_adapter_derived(sc_core::sc_module_name module_name) : xtsc_sc_out_sc_bv_base_adapter_base<1, T>
                                                                                   (module_name) { }
  virtual const sc_dt::sc_bv_base& read() const { this->m_value = this->m_sc_out.read(); return this->m_value; }
  virtual void write(const sc_dt::sc_bv_base& value) { this->m_sc_out.write((T) value.to_uint()); }
};



// Specialization for converting scalar to/from non-integral types:  T must have bit_select ([0]) which has member function to_bool().
template <typename T>
class xtsc_sc_out_sc_bv_base_adapter_derived<1, T, false> : public xtsc_sc_out_sc_bv_base_adapter_base<1, T> {
public:
  xtsc_sc_out_sc_bv_base_adapter_derived(sc_core::sc_module_name module_name) : xtsc_sc_out_sc_bv_base_adapter_base<1, T>
                                                                                   (module_name) { }
  virtual const sc_dt::sc_bv_base& read() const { this->m_value = this->m_sc_out.read()[0].to_bool(); return this->m_value; }
  virtual void write(const sc_dt::sc_bv_base& value) { T v(value[0].to_bool()); this->m_sc_out.write(v); }
};



// Explicit specialization for converting scalar to/from non-integral sc_logic.
template <>
class xtsc_sc_out_sc_bv_base_adapter_derived<1, sc_dt::sc_logic, false> :
      public xtsc_sc_out_sc_bv_base_adapter_base<1, sc_dt::sc_logic>
{
public:
  xtsc_sc_out_sc_bv_base_adapter_derived(sc_core::sc_module_name module_name) : xtsc_sc_out_sc_bv_base_adapter_base<1, sc_dt::sc_logic>
                                                                                   (module_name) { }
  virtual const sc_dt::sc_bv_base& read() const { this->m_value = this->m_sc_out.read().to_bool(); return this->m_value; }
  virtual void write(const sc_dt::sc_bv_base& value) { sc_dt::sc_logic v(value[0].to_bool()); this->m_sc_out.write(v); }
};

#endif // DOXYGEN_SKIP



/**
 * User interface class for converting an sc_out<sc_bv_base> to sc_out<T>.
 *
 * An instance of this class can be used to convert an sc_out<sc_bv_base> to an
 * sc_out<T> where T is some common integral or SystemC type.  For example, the
 * xtsc_core class uses sc_out<sc_bv_base> for pin-level TIE output ports.  If you need
 * output ports of a different type then sc_bv_base, then this adapter template can
 * be used to create an adapter of the appropriate type.  A typical use-case for this
 * adapter is when cosimulating XTSC with Verilog using a commercial simulator.
 *
 * Some possible types for T are:
 * \verbatim
      bool
      sc_logic
      sc_lv<W>
      sc_bv<W>
      sc_uint<W>
      sc_biguint<W>
      unsigned long long
      unsigned long
      unsigned int
      unsigned short
      unsigned char
   \endverbatim
 *
 * As an example, assume you have two xtsc_core objects.  The first is called core0 and
 * it has a 50-bit TIE export state called "TIE_status" which has been enabled for
 * pin-level access.  The second is called core1 and it has a 50-bit TIE import wire
 * called "TIE_control" which has been enabled for pin-level access.  The following code
 * snippet can be use to connect these two TIE ports together using two adapters,
 * called status and control, and an sc_signal<sc_uint<50>>, called core0_to_core1:
 *
 * Note:  These core interfaces can (and typically should) be connected directly without
 * the use of adapters and an sc_signal.  This example is contrived just to
 * illustrate constructing and connecting these adapters.  Please consult the
 * xtsc-run documentation and the cosim sub-directories in the XTSC examples directory
 * for realistic uses of these adapters to cosimulate XTSC with Verilog.
 *
 * \verbatim
    xtsc_sc_out_sc_bv_base_adapter<50, sc_uint<50> > status("status");
    xtsc_sc_in_sc_bv_base_adapter<50, sc_uint<50> > control("control");
    sc_signal<sc_uint<50> >  core0_to_core1;

    core0.get_output_pin("TIE_status")(status.m_sc_export);
    status.m_sc_out(core0_to_core1);

    core1.get_input_pin("TIE_control")(control.m_sc_export);
    control.m_sc_in(core0_to_core1);
   \endverbatim
 * @see xtsc_sc_out_sc_bv_base_adapter_base::m_sc_export
 * @see xtsc_sc_out_sc_bv_base_adapter_base::m_sc_out
 * @see xtsc_sc_in_sc_bv_base_adapter
 * @see xtsc_core::get_output_pin()
 */
template <int W, typename T>
class xtsc_sc_out_sc_bv_base_adapter : public xtsc_sc_out_sc_bv_base_adapter_derived<W, T, is_integral<T>::value > {
public:

  xtsc_sc_out_sc_bv_base_adapter(sc_core::sc_module_name module_name) :
    xtsc_sc_out_sc_bv_base_adapter_derived<W, T, is_integral<T>::value> (module_name)
  {
  }


  virtual const char* kind() const { return "xtsc_sc_out_sc_bv_base_adapter"; }

};






/**
 * Base class for converting an sc_in<T> to an sc_in<sc_bv_base>.
 *
 * Note: This class is not used directly, instead use
 *       xtsc_sc_in_sc_bv_base_adapter.
 */
template <int W, typename T>
class xtsc_sc_in_sc_bv_base_adapter_base : public sc_core::sc_module, public sc_core::sc_signal_in_if<sc_dt::sc_bv_base> {
public:

  /// Bind the sc_in<sc_bv_base> to here
  sc_core::sc_export<sc_core::sc_signal_in_if<sc_dt::sc_bv_base> >   m_sc_export;

  /// Bind this to an sc_signal<T> or to a higher-level (outer) sc_in<T>
  sc_core::sc_in<T>                                                  m_sc_in;

  xtsc_sc_in_sc_bv_base_adapter_base(sc_core::sc_module_name module_name) :
    sc_module   (module_name),
    m_sc_export ("m_sc_export"),
    m_sc_in     ("m_sc_in"),
    m_value     (W)
  {
    m_value = 0;
    m_sc_export(*this);
  }

  // get the default event
  virtual const sc_core::sc_event& default_event() const {
    return m_sc_in.default_event();
  }

  // get the value changed event
  virtual const sc_core::sc_event& value_changed_event() const {
    return m_sc_in.value_changed_event();
  }

  // was there an event?
  virtual bool event() const {
    return m_sc_in.event();
  }

  // get a reference to the current value (for tracing)
  virtual const sc_dt::sc_bv_base& get_data_ref() const {
    return read();
  }

  // read the current value
  virtual const sc_dt::sc_bv_base& read() const  = 0;


protected:

  mutable sc_dt::sc_bv_base    m_value;

};



#ifndef DOXYGEN_SKIP
// Derived template for converting vector from integral types
template <int W, typename T, bool integral = true>
class xtsc_sc_in_sc_bv_base_adapter_derived : public xtsc_sc_in_sc_bv_base_adapter_base<W, T> {
public:
  xtsc_sc_in_sc_bv_base_adapter_derived(sc_core::sc_module_name module_name) : xtsc_sc_in_sc_bv_base_adapter_base<W, T>
                                                                                  (module_name) { }
  virtual const sc_dt::sc_bv_base& read() const { this->m_value = this->m_sc_in.read(); return this->m_value; }
};



// Specialization for converting vector from non-integral types:  T must have a conversion to sc_bv_base.
template <int W, typename T>
class xtsc_sc_in_sc_bv_base_adapter_derived<W, T, false> : public xtsc_sc_in_sc_bv_base_adapter_base<W, T> {
public:
  xtsc_sc_in_sc_bv_base_adapter_derived(sc_core::sc_module_name module_name) : xtsc_sc_in_sc_bv_base_adapter_base<W, T>
                                                                                  (module_name) { }
  virtual const sc_dt::sc_bv_base& read() const { this->m_value = this->m_sc_in.read(); return this->m_value; }
};



// Specialization for converting scalar from integral types
template <typename T>
class xtsc_sc_in_sc_bv_base_adapter_derived<1, T, true> : public xtsc_sc_in_sc_bv_base_adapter_base<1, T> {
public:
  xtsc_sc_in_sc_bv_base_adapter_derived(sc_core::sc_module_name module_name) : xtsc_sc_in_sc_bv_base_adapter_base<1, T>
                                                                                  (module_name) { }
  virtual const sc_dt::sc_bv_base& read() const { this->m_value = this->m_sc_in.read(); return this->m_value; }
};



// Specialization for converting scalar to/from non-integral types:  T must have bit_select ([0]) which has member function to_bool().
template <typename T>
class xtsc_sc_in_sc_bv_base_adapter_derived<1, T, false> : public xtsc_sc_in_sc_bv_base_adapter_base<1, T> {
public:
  xtsc_sc_in_sc_bv_base_adapter_derived(sc_core::sc_module_name module_name) : xtsc_sc_in_sc_bv_base_adapter_base<1, T>
                                                                                  (module_name) { }
  virtual const sc_dt::sc_bv_base& read() const { this->m_value = this->m_sc_in.read()[0].to_bool(); return this->m_value; }
};



// Explicit specialization for converting scalar to/from non-integral sc_logic.
template <>
class xtsc_sc_in_sc_bv_base_adapter_derived<1, sc_dt::sc_logic, false> :
      public xtsc_sc_in_sc_bv_base_adapter_base<1, sc_dt::sc_logic>
{
public:
  xtsc_sc_in_sc_bv_base_adapter_derived(sc_core::sc_module_name module_name) : xtsc_sc_in_sc_bv_base_adapter_base<1, sc_dt::sc_logic>
                                                                                  (module_name) { }
  virtual const sc_dt::sc_bv_base& read() const { this->m_value = this->m_sc_in.read().to_bool(); return this->m_value; }
};

#endif // DOXYGEN_SKIP






/**
 * User interface class for converting an sc_in<T> to an sc_in<sc_bv_base>.
 *
 * An instance of this class can be used to convert an sc_in<T> to an sc_in<sc_bv_base>
 * where T is some common integral or SystemC type.  For example, the xtsc_core class
 * uses sc_in<sc_bv_base> for pin-level TIE input ports.  If you need input ports of a
 * different type then sc_bv_base, then this adapter template can be used to create
 * an adapter of the appropriate type.  A typical use-case for this adapter is
 * when cosimulating XTSC with Verilog using a commercial simulator.
 *
 * For more information about when this adapter might be used and how to use it,
 * see xtsc_sc_out_sc_bv_base_adapter.
 *
 * @see xtsc_sc_out_sc_bv_base_adapter
 * @see xtsc_sc_in_sc_bv_base_adapter_base::m_sc_export
 * @see xtsc_sc_in_sc_bv_base_adapter_base::m_sc_in
 * @see xtsc_core::get_input_pin()
 */
template <int W, typename T>
class xtsc_sc_in_sc_bv_base_adapter : public xtsc_sc_in_sc_bv_base_adapter_derived<W, T, is_integral<T>::value > {
public:

  xtsc_sc_in_sc_bv_base_adapter(sc_core::sc_module_name module_name) :
    xtsc_sc_in_sc_bv_base_adapter_derived<W, T, is_integral<T>::value> (module_name)
  {
  }

  virtual const char* kind() const { return "xtsc_sc_in_sc_bv_base_adapter"; }


};



/**
 * Base class for converting an sc_out<sc_uint_base> to sc_out<T>.
 *
 * Note: This class is not used directly, instead use
 *       xtsc_sc_out_sc_uint_base_adapter.
 */
template <int W, typename T>
class xtsc_sc_out_sc_uint_base_adapter_base : public sc_core::sc_module, public sc_core::sc_signal_inout_if<sc_dt::sc_uint_base> {
public:

  /// Bind the sc_out<sc_uint_base> to here
  sc_core::sc_export<sc_core::sc_signal_inout_if<sc_dt::sc_uint_base> > m_sc_export;

  /// Bind this to sc_signal<T> or an outer sc_out<T>
  sc_core::sc_out<T>                                                    m_sc_out;

  xtsc_sc_out_sc_uint_base_adapter_base(sc_core::sc_module_name module_name) :
    sc_module   (module_name),
    m_sc_export ("m_sc_export"),
    m_sc_out    ("m_sc_out"),
    m_value     (W)
  {
    m_value = 0;
    m_sc_export(*this);
  }

  // get the default event
  virtual const sc_core::sc_event& default_event() const {
    return m_sc_out.default_event();
  }

  // get the value changed event
  virtual const sc_core::sc_event& value_changed_event() const {
    return m_sc_out.value_changed_event();
  }

  // was there an event?
  virtual bool event() const {
    return m_sc_out.event();
  }

  // get a reference to the current value (for tracing)
  virtual const sc_dt::sc_uint_base& get_data_ref() const {
    return read();
  }

  // read the current value
  virtual const sc_dt::sc_uint_base& read() const  = 0;

  // write the new value
  virtual void write(const sc_dt::sc_uint_base& value)  = 0;


protected:

  mutable sc_dt::sc_uint_base    m_value;

};



#ifndef DOXYGEN_SKIP
// Derived template for converting vector to/from integral types
template <int W, typename T, bool integral = true>
class xtsc_sc_out_sc_uint_base_adapter_derived : public xtsc_sc_out_sc_uint_base_adapter_base<W, T> {
public:
  xtsc_sc_out_sc_uint_base_adapter_derived(sc_core::sc_module_name module_name) :
    xtsc_sc_out_sc_uint_base_adapter_base<W, T>(module_name) { }
  virtual const sc_dt::sc_uint_base& read() const { this->m_value = this->m_sc_out.read(); return this->m_value; }
  virtual void write(const sc_dt::sc_uint_base& value) { this->m_sc_out.write((T) value.to_uint64()); }
};



// Specialization for converting vector to/from non-integral types:  T must have a conversion to/from sc_uint_base
template <int W, typename T>
class xtsc_sc_out_sc_uint_base_adapter_derived<W, T, false> : public xtsc_sc_out_sc_uint_base_adapter_base<W, T> {
public:
  xtsc_sc_out_sc_uint_base_adapter_derived(sc_core::sc_module_name module_name) :
    xtsc_sc_out_sc_uint_base_adapter_base<W, T>(module_name) { }
  virtual const sc_dt::sc_uint_base& read() const { this->m_value = this->m_sc_out.read(); return this->m_value; }
  virtual void write(const sc_dt::sc_uint_base& value) { this->m_sc_out.write(value); }
};



// Specialization for converting scalar to/from integral types
template <typename T>
class xtsc_sc_out_sc_uint_base_adapter_derived<1, T, true> : public xtsc_sc_out_sc_uint_base_adapter_base<1, T> {
public:
  xtsc_sc_out_sc_uint_base_adapter_derived(sc_core::sc_module_name module_name) :
    xtsc_sc_out_sc_uint_base_adapter_base<1, T>(module_name) { }
  virtual const sc_dt::sc_uint_base& read() const { this->m_value = this->m_sc_out.read(); return this->m_value; }
  virtual void write(const sc_dt::sc_uint_base& value) { this->m_sc_out.write((T) value.to_uint()); }
};



// Specialization for converting scalar to/from non-integral types:  T must have bit_select ([0]) which has member function to_bool().
template <typename T>
class xtsc_sc_out_sc_uint_base_adapter_derived<1, T, false> : public xtsc_sc_out_sc_uint_base_adapter_base<1, T> {
public:
  xtsc_sc_out_sc_uint_base_adapter_derived(sc_core::sc_module_name module_name) :
    xtsc_sc_out_sc_uint_base_adapter_base<1, T>(module_name) { }
  virtual const sc_dt::sc_uint_base& read() const { this->m_value = this->m_sc_out.read()[0].to_bool(); return this->m_value; }
  virtual void write(const sc_dt::sc_uint_base& value) { T v(value[0].to_bool()); this->m_sc_out.write(v); }
};



// Explicit specialization for converting scalar to/from non-integral sc_logic.
template <>
class xtsc_sc_out_sc_uint_base_adapter_derived<1, sc_dt::sc_logic, false> :
      public xtsc_sc_out_sc_uint_base_adapter_base<1, sc_dt::sc_logic>
{
public:
  xtsc_sc_out_sc_uint_base_adapter_derived(sc_core::sc_module_name module_name) :
    xtsc_sc_out_sc_uint_base_adapter_base<1, sc_dt::sc_logic>(module_name) { }
  virtual const sc_dt::sc_uint_base& read() const { this->m_value = this->m_sc_out.read().to_bool(); return this->m_value; }
  virtual void write(const sc_dt::sc_uint_base& value) { sc_dt::sc_logic v(value[0].to_bool()); this->m_sc_out.write(v); }
};

#endif // DOXYGEN_SKIP



/**
 * User interface class for converting an sc_out<sc_uint_base> to sc_out<T>.
 *
 * An instance of this class can be used to convert an sc_out<sc_uint_base> to an
 * sc_out<T> where T is some common integral or SystemC type.  For example, the
 * xtsc_tlm2pin_memory_transactor class uses sc_out<sc_uint_base> for output port
 * vectors of 32 bits or less (except for a data bus).  If you need output ports of a
 * different type then sc_uint_base, then this adapter template can be used to create
 * an adapter of the appropriate type.  A typical use-case for this adapter is
 * when cosimulating XTSC with Verilog using a commercial simulator.
 *
 * Some possible types for T are:
 * \verbatim
      bool
      sc_logic
      sc_lv<W>
      sc_bv<W>
      sc_uint<W>
      sc_biguint<W>
      unsigned long long
      unsigned long
      unsigned int
      unsigned short
      unsigned char
   \endverbatim
 *
 * Note:  Please consult the xtsc-run documentation and the cosim sub-directories in the
 *        XTSC examples directory for uses of this adapter to cosimulate XTSC with
 *        Verilog.
 *
 * @see xtsc_sc_out_sc_uint_base_adapter_base::m_sc_export
 * @see xtsc_sc_out_sc_uint_base_adapter_base::m_sc_out
 * @see xtsc_sc_in_sc_uint_base_adapter
 */
template <int W, typename T>
class xtsc_sc_out_sc_uint_base_adapter : public xtsc_sc_out_sc_uint_base_adapter_derived<W, T, is_integral<T>::value > {
public:

  xtsc_sc_out_sc_uint_base_adapter(sc_core::sc_module_name module_name) :
    xtsc_sc_out_sc_uint_base_adapter_derived<W, T, is_integral<T>::value> (module_name)
  {
  }


  virtual const char* kind() const { return "xtsc_sc_out_sc_uint_base_adapter"; }

};






/**
 * Base class for converting an sc_in<T> to an sc_in<sc_uint_base>.
 *
 * Note: This class is not used directly, instead use
 *       xtsc_sc_in_sc_uint_base_adapter.
 */
template <int W, typename T>
class xtsc_sc_in_sc_uint_base_adapter_base : public sc_core::sc_module, public sc_core::sc_signal_in_if<sc_dt::sc_uint_base> {
public:

  /// Bind the sc_in<sc_uint_base> to here
  sc_core::sc_export<sc_core::sc_signal_in_if<sc_dt::sc_uint_base> >  m_sc_export;

  /// Bind this to sc_signal<T> or an outer sc_in<T>
  sc_core::sc_in<T>                                                   m_sc_in;

  xtsc_sc_in_sc_uint_base_adapter_base(sc_core::sc_module_name module_name) :
    sc_module   (module_name),
    m_sc_export ("m_sc_export"),
    m_sc_in     ("m_sc_in"),
    m_value     (W)
  {
    m_value = 0;
    m_sc_export(*this);
  }

  // get the default event
  virtual const sc_core::sc_event& default_event() const {
    return m_sc_in.default_event();
  }

  // get the value changed event
  virtual const sc_core::sc_event& value_changed_event() const {
    return m_sc_in.value_changed_event();
  }

  // was there an event?
  virtual bool event() const {
    return m_sc_in.event();
  }

  // get a reference to the current value (for tracing)
  virtual const sc_dt::sc_uint_base& get_data_ref() const {
    return read();
  }

  // read the current value
  virtual const sc_dt::sc_uint_base& read() const  = 0;


protected:

  mutable sc_dt::sc_uint_base    m_value;

};



#ifndef DOXYGEN_SKIP
// Derived template for converting vector from integral types
template <int W, typename T, bool integral = true>
class xtsc_sc_in_sc_uint_base_adapter_derived : public xtsc_sc_in_sc_uint_base_adapter_base<W, T> {
public:
  xtsc_sc_in_sc_uint_base_adapter_derived(sc_core::sc_module_name module_name) :
    xtsc_sc_in_sc_uint_base_adapter_base<W, T>(module_name) { }
  virtual const sc_dt::sc_uint_base& read() const { this->m_value = this->m_sc_in.read(); return this->m_value; }
};



// Specialization for converting vector from non-integral types:  T must have a conversion to sc_uint_base.
template <int W, typename T>
class xtsc_sc_in_sc_uint_base_adapter_derived<W, T, false> : public xtsc_sc_in_sc_uint_base_adapter_base<W, T> {
public:
  xtsc_sc_in_sc_uint_base_adapter_derived(sc_core::sc_module_name module_name) :
    xtsc_sc_in_sc_uint_base_adapter_base<W, T>(module_name) { }
  virtual const sc_dt::sc_uint_base& read() const { this->m_value = this->m_sc_in.read(); return this->m_value; }
};



// Specialization for converting scalar from integral types
template <typename T>
class xtsc_sc_in_sc_uint_base_adapter_derived<1, T, true> : public xtsc_sc_in_sc_uint_base_adapter_base<1, T> {
public:
  xtsc_sc_in_sc_uint_base_adapter_derived(sc_core::sc_module_name module_name) :
    xtsc_sc_in_sc_uint_base_adapter_base<1, T>(module_name) { }
  virtual const sc_dt::sc_uint_base& read() const { this->m_value = this->m_sc_in.read(); return this->m_value; }
};



// Specialization for converting scalar to/from non-integral types:  T must have bit_select ([0]) which has member function to_bool().
template <typename T>
class xtsc_sc_in_sc_uint_base_adapter_derived<1, T, false> : public xtsc_sc_in_sc_uint_base_adapter_base<1, T> {
public:
  xtsc_sc_in_sc_uint_base_adapter_derived(sc_core::sc_module_name module_name) :
    xtsc_sc_in_sc_uint_base_adapter_base<1, T>(module_name) { }
  virtual const sc_dt::sc_uint_base& read() const { this->m_value = this->m_sc_in.read()[0].to_bool(); return this->m_value; }
};



// Explicit specialization for converting scalar to/from non-integral sc_logic.
template <>
class xtsc_sc_in_sc_uint_base_adapter_derived<1, sc_dt::sc_logic, false> :
      public xtsc_sc_in_sc_uint_base_adapter_base<1, sc_dt::sc_logic>
{
public:
  xtsc_sc_in_sc_uint_base_adapter_derived(sc_core::sc_module_name module_name) :
    xtsc_sc_in_sc_uint_base_adapter_base<1, sc_dt::sc_logic>(module_name) { }
  virtual const sc_dt::sc_uint_base& read() const { this->m_value = this->m_sc_in.read().to_bool(); return this->m_value; }
};

#endif // DOXYGEN_SKIP






/**
 * User interface class for converting an sc_in<T> to an sc_in<sc_uint_base>.
 *
 * An instance of this class can be used to convert an sc_in<T> to an
 * sc_in<sc_uint_base> where T is some common integral or SystemC type.  For example,
 * the xtsc_tlm2pin_memory_transactor class uses sc_in<sc_uint_base> for input port
 * vectors of 32 bits or less (except for a data bus).  If you need input ports of a
 * different type then sc_uint_base, then this adapter template can be used to create
 * an adapter of the appropriate type.  A typical use-case for this adapter is
 * when cosimulating XTSC with Verilog using a commercial simulator.
 *
 * For more information about when this adapter might be used and how to use it,
 * see xtsc_sc_out_sc_uint_base_adapter.
 *
 * @see xtsc_sc_out_sc_uint_base_adapter
 * @see xtsc_sc_in_sc_uint_base_adapter_base::m_sc_export
 * @see xtsc_sc_in_sc_uint_base_adapter_base::m_sc_in
 */
template <int W, typename T>
class xtsc_sc_in_sc_uint_base_adapter : public xtsc_sc_in_sc_uint_base_adapter_derived<W, T, is_integral<T>::value > {
public:

  xtsc_sc_in_sc_uint_base_adapter(sc_core::sc_module_name module_name) :
    xtsc_sc_in_sc_uint_base_adapter_derived<W, T, is_integral<T>::value> (module_name)
  {
  }

  virtual const char* kind() const { return "xtsc_sc_in_sc_uint_base_adapter"; }


};



/**
 * Base class for converting an sc_out<xtsc_wire_write_if> to sc_out<T>.
 *
 * Note: This class is not used directly, instead use
 *       xtsc_tlm2pin_wire_transactor.
 */
template <int W, typename T>
class xtsc_tlm2pin_wire_transactor_base : public sc_core::sc_module, public xtsc_wire_write_if {
public:

  /// Bind the sc_port<xtsc_wire_write_if> to here
  sc_core::sc_export<xtsc_wire_write_if>        m_sc_export;

  /// Bind this to an sc_signal<T> or to a higher-level (outer) sc_out<T>
  sc_core::sc_out<T>                            m_sc_out;

  xtsc_tlm2pin_wire_transactor_base(sc_core::sc_module_name module_name) :
    sc_module   (module_name),
    m_sc_out    ("m_sc_out"),
    m_bit_width (W)
  {
    m_sc_export(*this);
  }

  // get the default event
  virtual const sc_core::sc_event& default_event() const {
    return m_sc_out.default_event();
  }

  // get the value changed event
  virtual const sc_core::sc_event& value_changed_event() const {
    return m_sc_out.value_changed_event();
  }

  // was there an event?
  virtual bool event() const {
    return m_sc_out.event();
  }

  virtual void nb_write(const sc_dt::sc_unsigned& value) = 0;

  virtual u32 nb_get_bit_width() { return m_bit_width; }

private:
  u32                           m_bit_width;
};



#ifndef DOXYGEN_SKIP
// Derived template for converting to integral types of 32 bits or less
template <int W, typename T, bool integral = true, bool integral64 = false>
class xtsc_tlm2pin_wire_transactor_derived : public xtsc_tlm2pin_wire_transactor_base<W, T> {
public:
  xtsc_tlm2pin_wire_transactor_derived(sc_core::sc_module_name module_name) : xtsc_tlm2pin_wire_transactor_base<W, T> (module_name) { }
  virtual void nb_write(const sc_dt::sc_unsigned& value) { this->m_sc_out.write((T) value.to_uint()); }
};



// Specialization for converting to integral64 types
template <int W, typename T>
class xtsc_tlm2pin_wire_transactor_derived<W, T, true, true>: public xtsc_tlm2pin_wire_transactor_base<W, T> {
public:
  xtsc_tlm2pin_wire_transactor_derived(sc_core::sc_module_name module_name) : xtsc_tlm2pin_wire_transactor_base<W, T> (module_name) { }
  virtual void nb_write(const sc_dt::sc_unsigned& value) { this->m_sc_out.write((T) value.to_uint64()); }
};



// Specialization for converting to non-integral types:  T must have a conversion from sc_unsigned
template <int W, typename T>
class xtsc_tlm2pin_wire_transactor_derived<W, T, false> : public xtsc_tlm2pin_wire_transactor_base<W, T> {
public:
  xtsc_tlm2pin_wire_transactor_derived(sc_core::sc_module_name module_name) : xtsc_tlm2pin_wire_transactor_base<W, T> (module_name) { }
  virtual void nb_write(const sc_dt::sc_unsigned& value) { this->m_sc_out.write(value); }
};
#endif // DOXYGEN_SKIP







/**
 * User interface class for connecting an sc_port<xtsc_wire_write_if> to an sc_out<T> or
 * an sc_signal<T>.
 *
 * An instance of this class can be used to connect an sc_port<xtsc_wire_write_if> to an
 * sc_out<T> or an sc_signal<T> where T is some common integral or SystemC type.  For
 * example, the xtsc_core class uses sc_port<xtsc_wire_write_if> for TLM-level TIE
 * export states and system-level output ports.  If you need output ports of a different
 * type then xtsc_wire_write_if, then this transactor template can be used to create an
 * transactor of the appropriate type.  A typical use-case for this transactor is when
 * cosimulating XTSC with Verilog using a commercial simulator.
 *
 * Note: see xtsc_sc_out_sc_bv_base_adapter for a list of some possible types for T.
 *
 * As an example, assume you have two xtsc_core objects.  The first is called core0 and
 * it has a 1-bit TIE export state called "onebit".  The second is called core1 and it
 * has a 1-bit system-level input called "BReset".  The following code snippet can be
 * use to connect these two ports together using two transactors, called onebit and
 * BReset, and an sc_signal<bool>, called onebit_to_BReset:
 *
 * Note:  These core interfaces can (and typically should) be connected directly without
 * the use of transactors and an sc_signal.  This example is contrived just to
 * illustrate constructing and connecting these transactors.  Please consult the
 * xtsc-run documentation and the cosim sub-directories in the XTSC examples directory
 * for realistic uses of these transactors to cosimulate XTSC with Verilog.
 *
 * \verbatim
    xtsc_tlm2pin_wire_transactor<1, bool>  onebit("onebit");
    xtsc_pin2tlm_wire_transactor<1, bool>  BReset("BReset");
    sc_signal<bool>                        onebit_to_BReset;

    core0.get_export_state("onebit")(onebit.m_sc_export);
    onebit.m_sc_out(onebit_to_BReset);

    BReset.m_sc_port(core1.get_input_wire("BReset"));
    BReset.m_sc_in(onebit_to_BReset);
   \endverbatim
 *
 *
 *
 * @see xtsc_tlm2pin_wire_transactor_base::m_sc_export
 * @see xtsc_tlm2pin_wire_transactor_base::m_sc_out
 * @see xtsc_core::get_export_state()
 * @see xtsc_core::get_output_wire()
 * @see xtsc_core::get_input_wire()
 */
template <int W, typename T>
class xtsc_tlm2pin_wire_transactor :
  public xtsc_tlm2pin_wire_transactor_derived<W, T, is_integral<T>::value, is_integral64<T>::value> {
public:

  xtsc_tlm2pin_wire_transactor(sc_core::sc_module_name module_name) :
    xtsc_tlm2pin_wire_transactor_derived<W, T, is_integral<T>::value, is_integral64<T>::value> (module_name)
  {
  }


  virtual const char* kind() const { return "xtsc_tlm2pin_wire_transactor"; }

};



/**
 * User interface class for connecting an sc_in<T> or an sc_signal<T> to an
 * sc_export<xtsc_wire_write_if>.
 *
 * An instance of this class can be used to connect an sc_in<T> or an sc_signal<T> to an
 * sc_export<xtsc_wire_write_if> where T is some common integral or SystemC type.  For
 * example, the xtsc_core class uses sc_export<xtsc_wire_write_if> for system-level
 * input ports.  If you need input ports of a different type then xtsc_wire_write_if,
 * then this transactor template can be used to create an transactor of the appropriate
 * type.  A typical use-case for this transactor is when cosimulating XTSC with Verilog
 * using a commercial simulator.
 *
 * For more information about when this transactor might be used and how to use it,
 * see xtsc_tlm2pin_wire_transactor.
 *
 *
 * @see xtsc_tlm2pin_wire_transactor
 * @see xtsc_core::get_input_wire()
 */
template <int W, typename T>
class xtsc_pin2tlm_wire_transactor : public sc_core::sc_module {
public:

  /// Bind this to the sc_export<xtsc_wire_write_if>
  sc_core::sc_port<xtsc_wire_write_if>          m_sc_port;

  /// Bind this to the sc_signal<T> or to a higher-level (outer) sc_in<T>
  sc_core::sc_in<T>                             m_sc_in;

  SC_HAS_PROCESS(xtsc_pin2tlm_wire_transactor);

  xtsc_pin2tlm_wire_transactor(sc_core::sc_module_name module_name) :
    sc_module   (module_name),
    m_sc_port   ("m_sc_port"),
    m_sc_in     ("m_sc_in"),
    m_value     (W)
  {
    SC_METHOD(relay_method);
    sensitive << m_sc_in;
    dont_initialize();
  }

  virtual const char* kind() const { return "xtsc_pin2tlm_wire_transactor"; }

  // get the default event
  virtual const sc_core::sc_event& default_event() const {
    return m_sc_in.default_event();
  }

  // get the value changed event
  virtual const sc_core::sc_event& value_changed_event() const {
    return m_sc_in.value_changed_event();
  }

  // was there an event?
  virtual bool event() const {
    return m_sc_in.event();
  }

  void relay_method() {
    this->m_value = this->m_sc_in.read();
    m_sc_port->nb_write(m_value);
  }

private:
  sc_dt::sc_unsigned            m_value;
};






/**
 * Utility class for handling a script-style file.
 *
 * This utility class provides a wrapper for handling a script-style file.
 *
 * The get_words() method can be used to get the next non-blank, non-comment line from
 * the file tokenized into words (as a vector of strings).
 *
 * The getline() method can be used to get the next non-blank, non-comment line from the
 * file as a string.
 *
 * The handling of an xtsc_script_file includes a pseudo-preprocessor:
 *
 * - C and C++ style comments are never returned by the get_words() and getline()
 *   methods.
 *
 * - A "#define <Identifier>" line defines a simple macro (a simple macro is a macro
 *   that is defined but that doesn't contain a value and should be contrasted with an
 *   alias macro, see below, that does contain a value).  <Identifier> must be a legal
 *   C/C++ identifier.  The line itself is never returned by the get_words() and
 *   getline() methods.
 *
 * - A "#define <Identifier>=<String>" line defines an alias macro.  <String> can be any
 *   string (including the empty string).  Before a line from the script file is
 *   processed in any other way (other then being skipped because it is a comment or is
 *   between a false "#if/#endif" pair), it is first scanned for occurrences of any
 *   defined simple or alias macro preceeded by a dollar sign and opening parenthesis
 *   and followed by a closing parenthesis.  Each occurrence is replaced verbatim by the
 *   corresponding <String> if it is an alias macro or by an empty string if it is a
 *   simple macro.  For example, a script file consisting only of the following two
 *   lines will result in the single line "Hello World" being returned by the getline()
 *   method:
 *   \verbatim
        #define entity=World
        Hello $(entity)
     \endverbatim
 *
 *   Note: On preprocessor lines (lines beginning with #), a sub-string in the format
 *         "$(<Identifier>)" where <Identifier> is not a defined macro will be removed
 *         (i.e. treated like <Identifier> is a macro aliased to the empty string).
 *         On non-preprocessor lines, no substitution takes place if the <Identifier> is
 *         not a defined macro.  If substitution is desired, then place code similiar to
 *         the following earlier in the script file:
 *   \verbatim
              #ifndef MY_MACRO
              #define MY_MACRO
              #endif
     \endverbatim
 *
 * - A "#define <Identifier> <Text>" line also defines an alias macro.  <Text> can be
 *   any non-blank text that does not contain an equal sign (=).  Leading and trailing
 *   spaces are stripped from <Text>, but embedded spaces are left in place.  For
 *   example, a script file consisting only of the following two lines will result in
 *   the single line "Hello World" being returned by the getline() method:
 *   \verbatim
        #define message     Hello World
        $(message)
     \endverbatim
 *
 *   Note:  The alias macro XTSC_SCRIPT_FILE_PATH is always defined and is equal to the
 *          path to the current script file being processed.  It can be used to specify
 *          locations in the file system relative to the xtsc_script_file.
 *
 *   Note:  The alias macro XTSC_SCRIPT_FILE_PATH_ESC is always defined and is the same
 *          as XTSC_SCRIPT_FILE_PATH except that any backslashes in the path will be
 *          escaped with a 2nd backslash.
 *
 *   Note:  The alias macro XTSC_SCRIPT_FILE_COMPONENT is always defined and is equal to the
 *          value of the name parameter passed into the xtsc_script_file constructor
 *          (which may be the empty string).
 *
 *   Note:  The alias macro XTSC_SCRIPT_FILE_PLATFORM is always defined and is equal to
 *          "LINUX" or "WINDOWS" depending on the platform being used.
 *
 *   Note:  The alias macro XTSC_SCRIPT_FILE_COMPILER is always defined according to
 *          the compiler version used to compile the XTSC library.  On Linux it will be
 *          of the form "GCC-4.1", "GCC-4.4", etc.  On MS Windows it will be of the form
 *          "MSVC2010", "MSVC2012", etc.
 *
 * - A "#environment" line causes all the environment variables that are valid C/C++
 *   identifiers to be read in and treated as macros.  All environment variables that
 *   contain a value are defined as alias macros.  All environment variables that don't
 *   contain a value are defined as simple macros.  The "#environment" line itself is
 *   never returned by the get_words() and getline() methods.
 *
 * - A "#undef <Identifier>" line removes <Identifier> from the list of defined macros
 *   if such a macro exists (no error is generated if the macro is not currently
 *   defined).  The "#undef" line itself is never returned by the get_words() and
 *   getline() methods.
 *
 * - A "#dump" line causes all macros and their value to be logged by logger
 *   xtsc_script_file at NOTE_LOG_LEVEL if logging has been configured, otherwise they
 *   are logged to STDOUT.  This may be useful for trouble-shooting.  The "#dump" line
 *   itself is never returned by the get_words() and getline() methods.
 *
 * - All lines between and including a line starting with "#if 0" and a line starting with
 *   "#elif"/"#else"/"#endif" are ignored and are never returned by the get_words() and
 *   getline() methods.
 *
 * - Lines starting with "#if 1" and "#elif"/"#else"/"#endif" are never returned by the
 *   get_words() and getline() methods; however, non-comment, non-blank, 
 *   non-pseudo-preprocessor lines between them are.
 *
 * - Lines starting with "#ifdef <Identifier>" are handled according to one of the
 *   previous two rules depending upon whether or not <Identifier> is a defined macro
 *   (either a simple macro or an alias macro).  "#if defined <Identifier>", 
 *   "#if !defined <Identifier>", and "#ifndef <Identifier>" are also supported.
 *
 * - A "#ifeq (<TextA>,<TextB>)" line is handled like a "#if 1" line if <TextA>
 *   is equal to <TextB>, otherwise, the line is handled like a "#if 0" line.  Although
 *   it is not a requirement, it is common for at least one of <TextA> and <TextB> to be
 *   a macro.  For example, "#ifeq ($(GCC_VERSION),3.4.1)".  The "#ifneq" directive is
 *   also supported.
 *
 * - The "#else" construct is supported between a "#if/#ifdef/#ifndef/#ifeq/#ifneq" line
 *   and its matching "#endif" line.  The "#else" line itself is never returned by the
 *   get_words() and getline() methods.
 *
 * - Zero, one, or more of the following "#elif" directives are allowed in any combination
 *   between a "#if"-style directive and its corresponding "#else/#endif":
 *   \verbatim
       #elif 0
       #elif 1
       #elif defined <Identifier>
       #elif !defined <Identifier>
       #elifdef <Identifier>
       #elifndef <Identifier>
       #elifeq (<TextA>,<TextB>)
       #elifneq (<TextA>,<TextB>)
     \endverbatim
 *
 * - Nesting of "#if/#elif/#else/#endif" blocks is NOT supported.
 *
 * - A "#error" line causes the line to be thrown as an exception.
 *
 * - A "#warn" line causes the line to be logged by logger xtsc_script_file at
 *   WARN_LOG_LEVEL if logging has been configured, otherwise the line is sent to
 *   STDERR.  The "#warn" line itself is never returned by the get_words() and getline()
 *   methods.
 *
 * - A "#note" line causes the line to be logged by logger xtsc_script_file at
 *   NOTE_LOG_LEVEL if logging has been configured, otherwise the line is sent to
 *   STDOUT.  The "#note" line itself is never returned by the get_words() and getline()
 *   methods.
 *
 * - A "#info" line causes the line to be logged by logger xtsc_script_file at
 *   INFO_LOG_LEVEL if logging has been configured (otherwise it is ignored).  The
 *   "#info" line itself is never returned by the get_words() and getline()
 *
 * Note: "#warn", "#note", and "#info" lines in an xtsc-run script file do not appear in
 *       the XTSC log file because xtsc-run processes the script file before configuring
 *       logging.
 *
 * - A "#include <FileName>" line causes processing of the current file to be
 *   temporarily suspended while file FileName is opened and processed.  The "#include"
 *   line itself is never returned by the get_words() and getline() methods.  FileName
 *   must be enclosed in quotation marks or angle brackets.  The XTSC_SCRIPT_FILE_PATH
 *   macro can be used as part of FileName to specify a path relative to the current
 *   script file being processed.  For example:
 *   \verbatim
       #include "$(XTSC_SCRIPT_FILE_PATH)/../common.inc"
     \endverbatim
 *
 * - The psuedo-preprocessor will pass the code snippet contained on a "#lua" line or
 *   between a "#lua_beg" and "#lua_end" pair to the Lua library for processing in a
 *   separate Lua coroutine.  The "#lua" prefix and the lines between and including the
 *   "#lua_beg" and "#lua_end" lines are never directly returned by the get_words() and
 *   getline() methods; however, the Lua code on the "#lua" line or in the
 *   "#lua_beg/#lua_end" block may pass lines back to the psuedo-preprocessor for return
 *   by the get_words() or getline() methods by calling the Lua function xtsc.write()
 *   which takes a single string argument containing the line to pass back to the 
 *   psuedo-preprocessor.  A "#lua_beg/#lua_end" block may NOT contain "#include" lines
 *   nor any of the "#if/#else/#endif"-style constructs; however, other psuedo-processor
 *   constructs such as "#define", "#error", "#warn", "#note", "#info", and
 *   $(<Identifier>) macro expansion are allowed.  Also, a "#lua" line or a
 *   "#lua_beg/#lua_end" block may appear inside a "#if/#endif"-style block.  
 *
 * Note: All Lua code in a given xtsc_script_file is processed by the same Lua state.  
 *       This allows variables to be set on a "#lua" line or in one "#lua_beg/#lua_end"
 *       block and then to be accessed by subsequent "#lua" lines or "#lua_beg/#lua_end"
 *       blocks and backticked Lua expressions (described next).
 *
 * - After the first "#lua" line (it may be empty) or "#lua_beg/#lua_end" block, any
 *   lines not on a "#lua" line or in a "#lua_beg/#lua_end" block will undergo backtick
 *   expansion such that the string between each pair of backticks will be passed to Lua
 *   for evaluation as a Lua expression.  The Lua expression string and enclosing
 *   backticks will be replaced by the result of the expression's evaluation.
 *
 * - In the formats described above, the pound sign ("#") must always be in the first
 *   column.
 *
 * - Other then the formats defined above, script files should not contain any lines
 *   that have a pound sign ("#") in the first column.
 *
 * - By default, the pseudo-preprocessor uses the backward slash (\) as the line
 *   continuation character.  The pseudo-preprocessor does not support comments or
 *   # constructs inside continued lines; however, macro expansion is supported.  The
 *   line continuation character can be changed using a "#continuation <CHAR>" line,
 *   where <CHAR> is a single non-alphanumeric printable character.  Line continuation
 *   support can be turned off using a "#continuation off" line or by defining the
 *   environment variable XTSC_SCRIPT_FILE_LINE_CONTINUATION_OFF.  If a line ends with a
 *   single line continuation character then that line and the following line are
 *   concatenated after first stripping the line continuation character and all then
 *   trailing whitespace from the first line and all leading whitespace from the second
 *   line.  If a line ends with two consecutive line continuation characters, then that
 *   line and the following line are concatenated after stripping the two line
 *   continuation characters; however, trailing and leading whitespace are preserved.
 *
 *   Note: Line continuation can be used in an xtsc-run script to allow the HERE file
 *         content to appear on separate lines.  For example:
 * \verbatim
              -set_router_parm=routing_table=<;> \
                      0x0 0x00000000 0x0FFFFFFF; \
                      0x1 0x10000000 0x1FFFFFFF; \
                      0x2 0x20000000 0x2FFFFFFF; \
                      0x3 0x30000000 0xFFFFFFFF
   \endverbatim
 *
 *   Note: Error messages regarding xtsc_script_file content contained in continued
 *         lines, use the line number of the last line in the sequence (in the
 *         preceeding example, this would be the line number of the line starting with
 *         0x3).
 *
 * Here is an example script to illustrate most of the supported preprocessor constructs:
 * \verbatim
    // You can include other files
    #include "common.inc"

    // You can define macros
    #define OPTION FAST

    // You can test macros
    #ifndef OPTION
    // You can cause a fatal error
    #error macro OPTION must be defined
    #endif

    // You can compare macros with text strings
    #ifeq ($(OPTION),SLOW)
    #define DELAY=1000
    #elifeq ($(OPTION),MEDIUM)
    #define DELAY=100
    #elifeq ($(OPTION),FAST)
    #define DELAY=10
    #else
    #define DELAY=1
    #endif

    // You can print notes and warnings
    #note Here comes a warning
    #warn DELAY is $(DELAY)

    // You can run a Lua code snippet
    #lua_beg
       for i=1,10 do
         xtsc.write("wait " .. i)
       end
    #lua_end

    // Other then what might be in "common.inc", the 10 "wait N"
    // lines generated by the above Lua snippet, the following
    // 3 lines, and the note below are all that the module reading
    // this script file will see
    $(DELAY) 123
    $(DELAY) 456
    $(DELAY) 789

    // If you have at least one preceeding #lua line or #lua_beg/#lua_end
    // block (the line or the block can be empty), then a pair of backticks
    // can be used to evaluate a Lua expression
    #note Timestamp: `os.date("%x %X")`

    // You can disable/enable blocks of "code"
    #if 0
    #warn Not doing this
    #elif 0
    #warn Not doing this either
    #elif 1
    #warn Doing this 
    #elif 1
    #warn Not doing this because I did one of the above
    #else
    #warn Also, not doing this because I did one of the above
    #endif

    // Pull in environment variables (for example, you could
    // have all the scripts used in a system simulaton conditioned
    // on a single environment variable)
    #environment
    #warn CWD is $(PWD)

    // This predefined macro is were the script file is located
    // even when it is not in the current directory
    #warn This script file is located in $(XTSC_SCRIPT_FILE_PATH)

    // This predefined macro is the optional name of the component
    // using the xtsc_script_file
    #warn This script file is being used by $(XTSC_SCRIPT_FILE_COMPONENT)

    // Line continuation
    #note This note is spread \\
    over 2 lines
   \endverbatim
 *
 * Note: The xtsc-run program can be used to dump the contents of an xtsc_script_file
 *       with preprocessing applied.  For example:
 * \verbatim
            xtsc-run -dump_script_file=my_script_file.vec
   \endverbatim
 */
class XTSC_API xtsc_script_file {
public:

  typedef std::vector<bool>                     vector_bool;
  typedef std::vector<u32>                      vector_u32;
  typedef std::vector<std::string>              vector_string;
  typedef std::vector<vector_string*>           vector_vector_string;
  typedef std::vector<std::ifstream*>           vector_ifstream;
  typedef std::set<std::string>                 set_string;
  typedef std::map<std::string, std::string>    map_string_string;


  /**
   * Constructor for an xtsc_script_file.
   *
   * @param     file_name       The name of the script file or a HERE file indicator and
   *                            contents.  If file_name does not begin with the HERE
   *                            file indicator then it must name an existing file or an
   *                            exception is thrown.  A HERE file is indicated by
   *                            file_name starting with the three character pattern 
   *                            "<CHAR>", where CHAR represents any single non-space,
   *                            printable character, all occurences of which in the
   *                            rest of file_name (if any) will be translated into a new
   *                            line in the HERE file content.  The HERE file name for
   *                            logging and error messages will be "HERE_FILE#N" where N
   *                            is the instantation number of the HERE file in program
   *                            execution order.  As an example, if file_name is
   *                            "<;>one;2;three amigos", then the HERE file indicator is
   *                            "<;>", the end-of-line substitution character is ";",
   *                            and the HERE file content is the following three lines:
   *  \verbatim
                                one
                                2
                                three amigos
   \endverbatim
   *
   * Note: The HERE file content in file_name is NOT pre-processed by the
   *       pseudo-preprocessor of this instance of xtsc_script_file; however, if the
   *       value of file_name itself came from another xtsc_script_file (a typical
   *       use-case in an xtsc-run script) then it would have been pre-processed by that
   *       xtsc_script_file before being used as file_name in this constructor call.
   *
   * @param     m_parm_name     Optional parameter name that file_name came from.  This
   *                            is used for exception messages.
   *
   * @param     name            Optional name of the component using the script file
   *                            (for example, "ColorLUT").  This is used for exception
   *                            messages and for supplying the value of the alias macro
   *                            XTSC_SCRIPT_FILE_COMPONENT.
   *
   * @param     type            Optional type of component using the script file (for
   *                            example, "xtsc_lookup").  This is used for exception
   *                            messages.
   *
   * @param     wraparound      If false (the default), the file is only processed one
   *                            time.  If true, the file pointer will be reset to the
   *                            beginning of the file each time the end of file is
   *                            reached.  If wraparound is true, the file must contain
   *                            at least one non-blank, non-comment line or an exception
   *                            will be thrown.
   *
   * @param     p_macros        Optional pointer to a map of name/value pair strings to
   *                            be used as predefined alias macros.  This mechanism is 
   *                            used by xtsc-run --define command to allow passing the
   *                            value of Xtensa Xplorer variables into an xtsc-run
   *                            script as xtsc_script_file macros.
   *
   * @param     log_here_file   If true (the default), the HERE file name and contents,
   *                            if applicable, will be logged by the constructor.
   */
  xtsc_script_file(const char                  *file_name,
                   const char                  *m_parm_name     = "",
                   const char                  *name            = "",
                   const char                  *type            = "",
                   bool                         wraparound      = false,
                   const map_string_string     *p_macros        = NULL,
                   bool                         log_here_file   = true);

  /// Destructor.
  ~xtsc_script_file();

  /// Return name (for HERE file this returns "HERE_FILE#N")
  std::string name();

  /// Reset script file to the beginning and start over
  void reset();

  /**
   * Get the next line from the file.
   *
   * This method returns the next non-blank, non-comment line from the file.
   *
   * @param     line            A reference to the string object in which to return the
   *                            next non-blank, non-comment line of the file.
   *
   * @param     p_file          An optional pointer to a string in which to return the 
   *                            name of the file from which this line came from (or NULL
   *                            if the name of the file is not desired).
   *
   * Note: By default, the getline() method trims trailing whitespace (tabs and spaces)
   *       from the end of the line before returning it.  If this is not desired, define
   *       the environment variable XTSC_SCRIPT_FILE_DO_NOT_TRIM_TRAILING_WHITESPACE.
   *
   * @returns the line number.  Returns 0 if wraparound is false and the end of file is
   *          reached.
   */
  u32 getline(std::string& line, std::string *p_file = NULL);

  /**
   * Get the next line from the file broken into words.
   *
   * This method returns the next non-blank, non-comment line from the file broken into
   * words.  Word tokenization is based on whitespace (tab and space characters).
   *
   * @param     words           A reference to a vector<string> object in which to
   *                            return the words.
   *
   * @param     downshift       If true, all the uppercase characters in each string in
   *                            the words vector will be shifted to lowercase.
   *
   * @param     p_file          An optional pointer to a string in which to return the 
   *                            name of the file from which this line came from (or NULL
   *                            if the name of the file is not desired).
   *
   * @returns the line number.  Returns 0 if wraparound is false and the end of file is
   *          reached.
   */
  u32 get_words(std::vector<std::string>& words, bool downshift = false, std::string *p_file = NULL);

  /**
   * Get the next line from the file broken into words.
   *
   * This method returns the next non-blank, non-comment line from the file broken into
   * words.  Word tokenization is based on whitespace (tab and space characters).  In
   * addition, the line read from the file is returned.
   *
   * @param     words           A reference to a vector<string> object in which to
   *                            return the words.
   *
   * @param     line            A reference to the string object in which to return the
   *                            line read from the file and used to form the words
   *                            vector.  Any C-style comment characters will not appear
   *                            in line, but the other characters will be in the same
   *                            case as they came from the file (i.e. they will not be
   *                            downshifted).
   *
   * @param     downshift       If true, all the uppercase characters in each string in
   *                            the words vector will be shifted to lowercase.
   *
   * @param     p_file          An optional pointer to a string in which to return the 
   *                            name of the file from which this line came from (or NULL
   *                            if the name of the file is not desired).
   *
   * @returns the line number.  Returns 0 if wraparound is false and the end of file is
   *          reached.
   */
  u32 get_words(std::vector<std::string>& words, std::string& line, bool downshift = false, std::string *p_file = NULL);

  /**
   * Evaluate exp as a Lua expression and return the result.
   *
   * @param     exp             A Lua expression to evaluate.  The expression must not
   *                            call SystemC wait().
   *
   * @returns the result.
   *
   * @see "LUA_FUNCTION" in "script_file" of xtsc_component::xtsc_memory_parms for an
   *       example of how this method can be used.
   */
  std::string evaluate_lua_expression(const std::string& exp);

  /**
   * Dump file name and line number info, including #include backtrace.
   *
   * @param     os              The ostream object to which the backtrace is to be
   *                            dumped.
   *
   * @param     single_line     If true, print backtrace on a signle line, otherwise
   *                            use one line per file.
   */
  void dump_last_line_info(std::ostream& os = std::cout, bool single_line = true);

  // Return common information about current situation for use when throwing an exception
  std::string info_for_exception(bool show_line_number = true, bool new_line = true, u32 beg = 0);


private:

  // Fill in info from m_parm_name, m_name, and m_type
  void info_for_exception(std::ostringstream& oss);

  // Do macro expansion
  void do_macro_expansion(std::string& line);

  // Do inline Lua expression replacement
  void do_backtick_expansion_of_lua_expression(std::string& line);

  // Initialize lua_State m_lua
  void init_lua_state(const std::string& err_msg);

  void throw_slurping_lua_snippet();

  // Add name/value to m_alias_macros and add name to m_macros
  void add_alias(const std::string& name, const std::string& value);

  // Add name to m_macros
  void add_macro(const std::string& name);

  // Deletes name from m_macros and m_alias_macros (if they have name as an entry)
  void delete_macro(const std::string& name);

  // Open the file and add it to the stack
  void open(const std::string& file_name);

  // Close all files in stack
  void close_all();

  // Close top file in stack
  void close();

  // Optionally trim leading and trailing whitespace
  void trim_whitespace(std::string& str, bool trim_leading = true, bool trim_trailing = true);

  // Throw exception due to missing identifer in line type line_type
  void throw_missing_identifier(const std::string& line_type);


  u32                   m_active_line_count;    // Count of active lines (non-comment, non-blank) in all files
  std::string           m_script_file_name;     // The name of the script file
  std::string           m_parm_name;            // Optional parameter name of m_file_name; used for exception messages
  std::string           m_name;                 // Optional name of component using this file; used for exception messages
  std::string           m_type;                 // Optional type of component using this file; used for exception messages
  bool                  m_wraparound;           // True means to wrap around to start of file when end of top level file is reached
  bool                  m_skipping_comment;     // Skipping lines because of multiline comment
  bool                  m_skipping_pound_cond;  // Skipping lines because of #if/#ifdef/#ifndef/#ifeq/#ifneq/#else
  char                  m_line_cont_char;       // The line continuation character (0 = none).
  bool                  m_here_file;            // True if HERE file
  u32                   m_here_file_line_num;   // Line number in HERE file
  vector_string         m_here_file_content;    // HERE file content
  set_string            m_macros;               // The set of simple and alias macros
  map_string_string     m_alias_macros;         // The map of alias macros and their string value
  map_string_string    *m_p_macros;             // Pre-defined alias macros from p_macros passed in to ctor

  u32                   m_file_depth;           // Index into the vectors that keep track of the #include file stack
  vector_bool           m_in_pound_cond;        // File stack of flags: true means we're inside a #if/#else/#endif block or any variant
  vector_bool           m_found_true_cond;      // File stack of flags: true means we're encountered a true arm (skip all remaining arms)
  vector_bool           m_found_pound_else;     // File stack of flags: true means we've encounter a #else
  vector_u32            m_line_number;          // The stack of file line numbers
  vector_string         m_file_name;            // The stack of file names
  vector_string         m_file_path;            // The stack of file paths
  vector_ifstream       m_file;                 // The stack of file stream object pointers

  lua_State            *m_lua;                  // Lua thread for main thread
  lua_State            *m_lua_cor;              // Lua thread for coroutine running a Lua snippet from a #lua line or from a #lua_beg/#lua_end block
  bool                  m_use_lua_snippet;      // true means we're getting script file lines from xtsc.write() calls in Lua code
  u32                   m_lua_line_number;      // File line number to associate with the line received from xtsc.write()
  u32                   m_lua_beg_line;         // File line number of the #lua line or the #lua_beg line

  std::string           m_whitespace;           // whitespace characters
};



/**
 * This utility function parses a optionally-indexed port name into a name portion and
 * and index.  For example if full_name is foobar[7] then port_name will be set to
 * foobar, port_index will be set to 7, and the function will return true.  An exception
 * will be thrown if full_name is not of the form PortName or PortName[N].
 *
 * @param       full_name       The full name including the optional index inclosed in
 *                              brackets.
 *
 * @param       port_name       A reference to a string in which to return the name
 *                              portion (without index)
 *
 * @param       port_index      A reference to a u32 in which to return the index.  If
 *                              full_name has no index the port_index will be set to 0.
 *
 * @return true if the full_name included an index.
 */
XTSC_API bool xtsc_parse_port_name(const std::string& full_name, std::string& port_name, u32& port_index);



/**
 * This utility function can be used to determine if a string is a valid C/C++ identifier.
 *
 * @param       name    The string to be tested.
 *
 * @return true if name is a valid C/C++ identifier, otherwise return false.
 */
XTSC_API bool xtsc_is_valid_identifier(const std::string& name);



/**
 * This utility function safely copies a c-string (char *).  It throws an exception if
 * memory for the new c-string cannot be allocated.
 *
 * @param       str     The c-string to be copied.  It can be NULL.
 *
 * @return a new copy of the str (or NULL if str is NULL).
 */
XTSC_API char *xtsc_copy_c_str(const char *str);



/**
 * This utility function safely copies an array of c-strings (char **).  It throws an
 * exception if memory for the new c-string array or the contained c-strings cannot be
 * allocated.
 *
 * @param       str_array       The c-string array to be copied.  The last entry in the
 *                              array must be NULL.  str_array itself can be NULL.
 *
 * @return a new copy of the str_array (or NULL if str_array is NULL) with each array
 *         entry itself a new copy .
 */
XTSC_API char **xtsc_copy_c_str_array(const char * const *str_array);



/**
 * This utility function deletes a c-string and then sets the pointer to NULL.
 *
 * @param       str     The c-string to be deleted.  It can be NULL.
 */
XTSC_API void xtsc_delete_c_str(char *&str);



/**
 * This utility function deletes an array of c-strings (char **).
 *
 * @param       str_array       The c-string array to be deleted.  The last entry in the
 *                              array must be NULL.  str_array itself can be NULL.
 */
XTSC_API void xtsc_delete_c_str_array(char **&str_array);



/**
 * Dump a list of SystemC objects.
 *
 * This utility method dumps a list of all existing SystemC objects (sc_object) to the
 * specified ostream object.  Optional name and kind patterns may be specified.  If both
 * patterns are specified then both must match for the object to be listed.
 *
 * @param       os              The ostream object to dump the object list to.
 *
 * @param       name_pattern    Optional pattern to match object names against.  The
 *                              pattern may contain 1 or more asterisks (*) as wildcards.
 *                              Each * matches 0 or more characters in any combination.
 *
 * @param       kind_pattern    Optional pattern to match object kind against.  The
 *                              pattern may contain 1 or more asterisks (*) as wildcards.
 *                              Each * matches 0 or more characters in any combination.
 *
 */
XTSC_API void xtsc_dump_systemc_objects(std::ostream& os = std::cout, const std::string& name_pattern = "*", const std::string& kind_pattern = "*");



/**
 * Interface to be called by xtsc_dispatch_command().
 *
 * This interface must be implemented by any class wishing to provide commands to the
 * XTSC command facility.  The class must implement this interface and then call
 * xtsc_register_command() for each command that it wishes to support.  Typically, a
 * class implementing this interface would be an sc_module but all that is actually
 * required is that an sc_object instance be provided in the xtsc_register_command()
 * call.  The name of the command handler is as returned by the sc_object::name()
 * method.
 *
 * The XTSC core library includes several methods that the execute() method can use to
 * make parsing the command line easier.  See the xtsc_command_* methods below.
 *
 * For working examples which illustrate implementing this interface please see the
 * xtsc_component::xtsc_memory source code in:
 *  - $XTENSA_SW_TOOLS/src/xtsc/xtsc_memory.cpp
 *  - $XTENSA_SW_TOOLS/src/xtsc/xtsc/xtsc_memory.h
 *
 * For user examples of invoking commands, see "lua_command_prompt" and
 * "xtsc_command_prompt" in xtsc_initialize_parms.
 *
 * @see xtsc_dispatch_command
 * @see xtsc_register_command
 * @see xtsc_command_argtobool
 * @see xtsc_command_argtoi32
 * @see xtsc_command_argtou32
 * @see xtsc_command_argtoi64
 * @see xtsc_command_argtou64
 * @see xtsc_command_argtod
 * @see xtsc_command_throw
 * @see xtsc_initialize_parms "lua_command_prompt"
 * @see xtsc_initialize_parms "xtsc_command_prompt"
 * @see sc_command_handler_commands
 * @see xtsc_command_handler_commands
 * @see xtsc::xtsc_core::execute()
 * @see xtsc_component::xtsc_arbiter::execute()
 * @see xtsc_component::xtsc_lookup::execute()
 * @see xtsc_component::xtsc_memory::execute()
 * @see xtsc_component::xtsc_memory_tlm2::execute()
 * @see xtsc_component::xtsc_mmio::execute()
 * @see xtsc_component::xtsc_queue::execute()
 * @see xtsc_component::xtsc_router::execute()
 * @see xtsc_component::xtsc_tlm22xttlm_transactor::execute()
 * @see xtsc_component::xtsc_wire_logic::execute()
 * @see xtsc_component::xtsc_xttlm2tlm2_transactor::execute()
 */
class XTSC_API xtsc_command_handler_interface {
public:

  /**
   * The method to be called when a command is to be executed.
   *
   * The xtsc_dispatch_command() method will call this method for handling the command
   * which is guaranteed to be a command that was registered via xtsc_register_command()
   * and guaranteed to have a number of arguments within the range [min_args, max_args].
   *
   *
   * @param     line            The command line as entered by the user (including the
   *                            handler name).
   *
   * @param     words           The tokenized words of the command line excluding the
   *                            handler name.  So the command is in words[0]; the first
   *                            argument, if any, is in words[1]; the second argument,
   *                            if any, is in words[2], and so on.
   *
   * @param     words_lc        Same as words except all characters are shifted to
   *                            lower-case to make parsing easier.
   *
   * @param     result          The ostream object to output the result to.  This
   *                            result must not be an error.  If an error is detected,
   *                            the execute() method should throw a std::exception (e.g.
   *                            xtsc_exception) with a meaningful message.
   */
  virtual void execute(const std::string&               line, 
                       const std::vector<std::string>&  words,
                       const std::vector<std::string>&  words_lc,
                       std::ostream&                    result) = 0;

  /**
   * If the user enters a "man" command, this optional method will be called just before
   * the man lines for the individual commands of this handler are output.  This can be
   * used, for example, to display help information pertaining to the handler as a whole
   * or to all commands rather than to one particular command.
   *
   * @param       os            The ostream object to output overall information to.
   */
  virtual void man(std::ostream& os) {}


  virtual ~xtsc_command_handler_interface() {}
};



/**
 * Method to register a command with the XTSC command facility.
 *
 * The xtsc_dispatch_command() method will call the handler's execute() method for
 * handling its commands.  That call is guaranteed to be for a command that was
 * registered via this method and the command is guaranteed to have a number of
 * arguments within the range [min_args, max_args].
 *
 * @param       object          The sc_object supporting the command.  Its name() method
 *                              will be used to determine the hierarchical name of the
 *                              command handler.
 *
 * @param       handler         The xtsc_command_handler_interface instance supporting
 *                              the command.
 *
 * @param       cmd             The command name.
 *
 * @param       min_args        The minimum number of arguments the command requires.
 *                              Should be 0 if the command accepts no arguments.
 *
 * @param       max_args        The maximum number of arguments the command will accept.
 *                              Should be 0 if the command accepts no arguments.  Use
 *                              -1 (0xFFFFFFFF) to specify no upper limit.
 *
 * @param       format          The command format.  The beginning of the format string
 *                              must be identical to the cmd argument.  If any arguments
 *                              are supported by the command then they should also be
 *                              shown.  This string will be presented to the user in
 *                              response to the "help" command.
 *
 * @param       man             A one line description of what the command does.  This
 *                              line will be presented to the user in response to the
 *                              "man" command.
 *
 * @see xtsc_command_handler_interface
 * @see xtsc_dispatch_command
 */
XTSC_API void xtsc_register_command(sc_core::sc_object&                 object,
                                    xtsc_command_handler_interface&     handler,
                                    const std::string&                  cmd,
                                    xtsc::u32                           min_args,
                                    xtsc::u32                           max_args,
                                    const std::string&                  format,
                                    const std::string&                  man);

/**
 * Method to unregister a command with the XTSC command facility.
 *
 * This method allows a subclass to modify or to disable a command entirely.
 * To do so, the derived class should call xtsc_unregister_command() on the
 * old version of the command prior to calling xtsc_register_command() for
 * the new version.
 *
 * @param       object          The sc_object supporting the command.  Its name() method
 *                              will be used to determine the hierarchical name of the
 *                              command handler.
 *
 * @param       cmd             The command name.
 *
 * @see xtsc_register_command
  */
XTSC_API void xtsc_unregister_command(sc_core::sc_object& object, const std::string& cmd);


/**
 * Method to dispatch an XTSC command line.
 *
 * This method is called by the lua and XTSC command prompts to dispatch XTSC commands.
 * It may also be called from a SystemC thread executing a script file specified by the
 * "lua_script_files" parameter or the Lua code snippet specified on a #lua line or
 * inside a "#lua_beg"/"#lua_end" block in an xtsc_script_file.  In addition, it may be
 * called from a script file running during a SystemC callback and specified by one of
 * the "lua_script_file_XXX" parameters.
 *
 * @param       cmd_line        The command line to be dispatched.  It should include
 *                              the handler name followed by the command itself followed
 *                              by any arguments.  A cmd_line starting with "#" is 
 *                              treated as a comment which is displayed on the console
 *                              but otherwise ignored.
 *
 * @param       result          An ostream object in which any non-error result will be
 *                              stored.  Note: If an error occurs an exception will be
 *                              thrown.
 *
 * @param       echo_comment    True if comment (#) lines should be echoed to result.
 *
 * @param       format_msg      General command format message to be output if handler
 *                              cannot be found or if there is no command.  The default
 *                              message (when format_msg is "") is appropriate for the
 *                              XTSC command prompt.
 *
 * @param       post_help       Additional information to be the final output if a user
 *                              enters a "help" command with no arguments.
 *
 * @param       post_man        Additional information to be the final output if a user
 *                              enters a "man" command with no arguments.
 *
 * @return false for a cmd_line of "c" (continue), otherwise returns true.
 *
 * @see xtsc_command_handler_interface
 * @see xtsc_register_command
 * @see xtsc_initialize_parms "lua_command_prompt"
 * @see xtsc_initialize_parms "lua_script_files"
 * @see xtsc_initialize_parms "lua_script_file_beoe"
 * @see xtsc_initialize_parms "lua_script_file_eoe"
 * @see xtsc_initialize_parms "lua_script_file_sos"
 * @see xtsc_initialize_parms "lua_script_file_eos"
 * @see xtsc_initialize_parms "xtsc_command_prompt"
 */
XTSC_API bool xtsc_dispatch_command(const std::string&  cmd_line,
                                    std::ostream&       result       = std::cout,
                                    bool                echo_comment = true,
                                    const std::string&  format_msg   = "",
                                    const std::string&  post_help    = "",
                                    const std::string&  post_man     = "");



/**
 * This convenience method is for use by the xtsc_command_handler_interface::execute()
 * method to make it easy to convert an argument to a bool value or to throw a
 * meaningful exception if this cannot be done.
 *
 * @param       line            Same as passed to the execute() method.
 *
 * @param       words           Same as passed to the execute() method.
 *
 * @param       argument        Which argument to convert to bool.  The arguments are
 *                              numbered starting at 1 (so words[0] is the command,
 *                              words[1] is the first argument, words[2] is the second
 *                              argument, and so on).
 *
 * @see xtsc_command_handler_interface::execute
 */
XTSC_API bool xtsc_command_argtobool(const std::string& cmd_line, const std::vector<std::string>& words, xtsc::u32 argument);



/**
 * This convenience method is for use by the xtsc_command_handler_interface::execute()
 * method to make it easy to convert an argument to a i32 value or to throw a
 * meaningful exception if this cannot be done.
 *
 * @param       line            Same as passed to the execute() method.
 *
 * @param       words           Same as passed to the execute() method.
 *
 * @param       argument        Which argument to convert to i32.  The arguments are
 *                              numbered starting at 1 (so words[0] is the command,
 *                              words[1] is the first argument, words[2] is the second
 *                              argument, and so on).
 *
 * @see xtsc_command_handler_interface::execute
 */
XTSC_API xtsc::i32 xtsc_command_argtoi32(const std::string& cmd_line, const std::vector<std::string>& words, xtsc::u32 argument);



/**
 * This convenience method is for use by the xtsc_command_handler_interface::execute()
 * method to make it easy to convert an argument to a u32 value or to throw a
 * meaningful exception if this cannot be done.
 *
 * @param       line            Same as passed to the execute() method.
 *
 * @param       words           Same as passed to the execute() method.
 *
 * @param       argument        Which argument to convert to u32.  The arguments are
 *                              numbered starting at 1 (so words[0] is the command,
 *                              words[1] is the first argument, words[2] is the second
 *                              argument, and so on).
 *
 * @see xtsc_command_handler_interface::execute
 */
XTSC_API xtsc::u32 xtsc_command_argtou32(const std::string& cmd_line, const std::vector<std::string>& words, xtsc::u32 argument);



/**
 * This convenience method is for use by the xtsc_command_handler_interface::execute()
 * method to make it easy to convert an argument to a i64 value or to throw a
 * meaningful exception if this cannot be done.
 *
 * @param       line            Same as passed to the execute() method.
 *
 * @param       words           Same as passed to the execute() method.
 *
 * @param       argument        Which argument to convert to i64.  The arguments are
 *                              numbered starting at 1 (so words[0] is the command,
 *                              words[1] is the first argument, words[2] is the second
 *                              argument, and so on).
 *
 * @see xtsc_command_handler_interface::execute
 */
XTSC_API xtsc::i64 xtsc_command_argtoi64(const std::string& cmd_line, const std::vector<std::string>& words, xtsc::u32 argument);



/**
 * This convenience method is for use by the xtsc_command_handler_interface::execute()
 * method to make it easy to convert an argument to a u64 value or to throw a
 * meaningful exception if this cannot be done.
 *
 * @param       line            Same as passed to the execute() method.
 *
 * @param       words           Same as passed to the execute() method.
 *
 * @param       argument        Which argument to convert to u64.  The arguments are
 *                              numbered starting at 1 (so words[0] is the command,
 *                              words[1] is the first argument, words[2] is the second
 *                              argument, and so on).
 *
 * @see xtsc_command_handler_interface::execute
 */
XTSC_API xtsc::u64 xtsc_command_argtou64(const std::string& cmd_line, const std::vector<std::string>& words, xtsc::u32 argument);



/**
 * This convenience method is for use by the xtsc_command_handler_interface::execute()
 * method to make it easy to convert an argument to a double value or to throw a
 * meaningful exception if this cannot be done.
 *
 * @param       line            Same as passed to the execute() method.
 *
 * @param       words           Same as passed to the execute() method.
 *
 * @param       argument        Which argument to convert to double.  The arguments are
 *                              numbered starting at 1 (so words[0] is the command,
 *                              words[1] is the first argument, words[2] is the second
 *                              argument, and so on).
 *
 * @see xtsc_command_handler_interface::execute
 */
XTSC_API double xtsc_command_argtod(const std::string& cmd_line, const std::vector<std::string>& words, xtsc::u32 argument);



/**
 * This convenience method is for use by the xtsc_command_handler_interface::execute()
 * method to make it easy to get the remainder of the command line after and including
 * the specified word.
 *
 * @param       line            Same as passed to the execute() method.
 *
 * @param       word            The first word to include in the remainder.  The words
 *                              are numbered starting at 0 (so words[0] is the command,
 *                              words[1] is the first argument, words[2] is the second
 *                              argument, and so on).  The <HandlerName> portion of
 *                              cmd_line is never returned by this method.
 *
 * @see xtsc_command_handler_interface::execute
 */
XTSC_API std::string xtsc_command_remainder(const std::string& cmd_line, xtsc::u32 word);



/**
 * This convenience method is for use by the xtsc_command_handler_interface::execute()
 * method to make it slightly easier to throw a meaningful exception.
 *
 * @param       line            Same as passed to the execute() method.
 *
 * @param       error_msg       The basic error message.  The command line and the error
 *                              message will be concatenated to form the full exception
 *                              message.
 *
 * @see xtsc_command_handler_interface::execute
 */
XTSC_API void xtsc_command_throw(const std::string& cmd_line, const std::string& error_msg);



/**
 * The following commands are supported by the global XTSC command handler called sc.
 *  \verbatim
      sc_delta_count
        Return sc_core::sc_delta_count() (the total delta cycles in the simulation so
        far).

      sc_get_time_resolution
        Return sc_core::sc_get_time_resolution().to_string().

      sc_stop
        Call sc_core::sc_stop().

      sc_time_stamp
        Return sc_core::sc_time_stamp().value() (the current simulation time in units of
        the SystemC time resolution).

      sc_version
        Return sc_core::sc_version().

      wait [<NumCycles> | <EventName> ...]
        Call sc_module::wait(<NumCycles> * xtsc::xtsc_get_system_clock_period()) or
        sc_module::wait(<EventOrList>).  <NumCycles> can be a non-integer.  An
        <EventName> can be - to mean last event from xtsc_event_create.  <EventOrList>
        is formed from all <EventName> arguments. Returns the logging system timestamp.

      waitall <EventName> ...
        Call sc_module::wait(<EventAndList>).  <EventAndList> is formed from all
        <EventName> arguments. Returns the logging system timestamp.

    \endverbatim
 * 
 * @see xtsc_command_handler_interface
 */
extern Readme sc_command_handler_commands;



/**
 * The following commands are supported by the global XTSC command handler called xtsc.
 *  \verbatim
      dump_status
        Call xtsc_core::dump_status() for all cores in the simulation.

      dump_log_levels
        Dump a list of the named log levels and their numeric value.

      dump_loggers [<Pattern>]
        Dump a list of all logger names (or optionally only those matching <Pattern>).

      get_log_level <Logger>
        Return the numeric log level of <Logger>.

      have_all_cores_exited
        Return xtsc_core::have_all_cores_exited() for all cores in the simulation.

      info <Message>
        Log entire command line (xtsc info <Message>) at INFO_LOG_LEVEL

      is_debugging_synchronized
        Return xtsc_core::is_debugging_synchronized() (applies to all debug-enabled
        cores in the simulation).

      note <Message>
        Log entire command line (xtsc note <Message>) at NOTE_LOG_LEVEL

      set_log_level <Logger> <LogLevel>
        Set log level of <Logger> to <LogLevel> (numeric).  Return previous numeric log
        level.

      shmem_list
        List all shared memories from calls to xtsc_get_shared_memory().

      shmem_get <Name> <BaseAddress> <Size>
        Call xtsc_get_shared_memory(<Name>, <Size>, "shmem_get", "XTSC command", <BaseAddress>).

      shmem_dump <Name> <Address> <NumBytes>
        Dump <NumBytes> bytes from <Address> of shared memory <Name> using xtsc_hex_dump().

      shmem_peek <Name> <StartAddress> <NumBytes>
        Peek <NumBytes> bytes from <StartAddress> of shared memory <Name>.

      shmem_poke <Name> <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>
        Poke <NumBytes> (=N) starting at <StartAddress> to shared memory <Name>.

      warn <Message>
        Log entire command line (xtsc warn <Message>) at WARN_LOG_LEVEL

      whoami
        Return the instance name of the sc_module running this thread.

      xtsc_dump_systemc_objects [<Pattern>]
        Return os buffer from calling xtsc_dump_systemc_objects(os, <Pattern>).

      xtsc_enable_text_logging <Enable>
        Call xtsc_enable_text_logging(<Enable>).  <Enable> may be 0|1.

      xtsc_event_create <EventName>
        Create a new sc_event and return its hierarchical name (which may differ between
        SystemC 2.2 and 2.3).

      xtsc_event_dump [<Pattern>] ...
        Return os buffer from calling xtsc_event_dump(os, <Pattern>) repeatedly for each
        <Pattern>.

      xtsc_event_exists <EventName>
        Return xtsc_event_exists(<EventName>).

      xtsc_event_notify <EventName> [<NumCycles>]
        Call xtsc_event_get(<EventName>).(<NumCycles> *
             xtsc::xtsc_get_system_clock_period()) or xtsc_event_get(<EventName>).() if
             <NumCycles> is unspecified.

      xtsc_filter_create <FilterKind> <FilterName> [<Key>=<Value>] ...
        Call xtsc_filter_create(<FilterKind>, <FilterName>, Pairs) where Pairs is a
        table containing all the <Key>=<Value> arguments.

      xtsc_filter_dump [<FilterKindOrName>]
        Call xtsc_filter_dump(<FilterKindOrName>, <FilterKindOrName>).

      xtsc_filter_exists <FilterName>
        Return xtsc_filter_exists(<FilterName>).

      xtsc_filter_kind_dump [<FilterKind>]
        Call xtsc_filter_kind_dump(<FilterKind>).

      xtsc_get_relaxed_simulation_interval
        Return xtsc::xtsc_get_relaxed_simulation_interval() /
               xtsc::xtsc_get_system_clock_period().

      xtsc_get_system_clock_factor
        Return xtsc::xtsc_get_system_clock_factor().

      xtsc_is_text_logging_enabled
        Return xtsc::xtsc_is_text_logging_enabled().

      xtsc_host_milliseconds
        Return xtsc::xtsc_host_milliseconds().

      xtsc_host_mutex_close <MutexName>
        Call xtsc_host_mutex_close(Mutex(<MutexName>)).

      xtsc_host_mutex_dump [<Pattern>]
        Dump list of xtsc_host_mutex names, lock counts, and current state for names matching <Pattern> (default is *).

      xtsc_host_mutex_lock <MutexName>
        Call xtsc_host_mutex_lock(Mutex(<MutexName>)).

      xtsc_host_mutex_open <MutexName>
        Call xtsc_host_mutex_open(<MutexName>).

      xtsc_host_mutex_try_lock <MutexName> [<Milliseconds>]
        Call xtsc_host_mutex_try_lock(Mutex(<MutexName>), <Milliseconds>).

      xtsc_host_mutex_unlock <MutexName>
        Call xtsc_host_mutex_unlock(Mutex(<MutexName>)).

      xtsc_host_sleep [<Milliseconds>]
        Call xtsc_host_sleep(<Milliseconds>).

      xtsc_pattern_match <Pattern> <Str>
        Return xtsc::xtsc_pattern_match(<Pattern>, <Str>).

      xtsc_prepare_to_switch_sim_mode <Turbo>
        Call xtsc::xtsc_prepare_to_switch_sim_mode(<Turbo>).  Returns 1 if ready, else
        returns 0.  <Turbo> may be 0|1 (0=>Cycle-Accurate  1=>TurboXim)

      xtsc_reset <HardReset>
        Return xtsc::xtsc_reset(<HardReset>).

      xtsc_set_relaxed_simulation_interval <NumCycles>
        Call xtsc::xtsc_set_relaxed_simulation_interval(<NumCycles> *
             xtsc::xtsc_get_system_clock_period()).

      xtsc_switch_sim_mode <Turbo>
        Call xtsc::xtsc_switch_sim_mode(<Turbo>).  See xtsc_prepare_to_switch_sim_mode
        which must return 1 before xtsc_switch_sim_mode is called.

      xtsc_user_state_dump [<Pattern>]
        Return the buffer from calling xtsc_user_state_dump(<Pattern>)

      xtsc_user_state_get <Name>
        Return xtsc_user_state_get(<Name>)

      xtsc_user_state_set <Name> [<Value>]
        Call xtsc_user_state_set(<Name>, <Value>)

      xtsc_version
        Return xtsc::xtsc_version().
    \endverbatim
 * 
 * @see xtsc_command_handler_interface
 */
extern Readme xtsc_command_handler_commands;



/**
 * Register an sc_event with XTSC and give it a name in SystemC 2.2.  In SystemC 2.3,
 * this method just returns the full-hierarchical name from sc_event::name().
 *
 * The xtsc_event_XXX API's allow the XTSC command facility to work with SystemC events.
 *
 * @param       event           The sc_event to be registered with XTSC.
 *
 * @param       base_name       The base (non-hierarchical) name of the sc_event.  The
 *                              full hierarchical name will be formed like this:
 *                                module.name() + "." + base_name
 *
 * @param       object          A pointer to the sc_object associated with the event or
 *                              NULL to indicate a top-level event.
 * 
 * @return the full hierarchical name of the event
 */
XTSC_API std::string xtsc_event_register(sc_core::sc_event& event, const std::string& base_name, sc_core::sc_object *p_object = NULL);



/**
 * This method returns true if the named sc_event was registered with XTSC (SystemC 2.2)
 * or if it exists as a hierarchically-named event (SystemC 2.3).
 *
 * The xtsc_event_XXX API's allow the XTSC command facility to work with SystemC events.
 *
 * @param       event_name      The hierarchical name of the sc_event.
 *
 * @see xtsc_event_register
 */
XTSC_API bool xtsc_event_exists(const std::string& event_name);



/**
 * This method returns the named sc_event.  In SystemC 2.2, the event must have been
 * registered with XTSC via xtsc_event_register().
 *
 * The xtsc_event_XXX API's allow the XTSC command facility to work with SystemC events.
 *
 * @param       event_name      The full hierarchical name of the sc_event.
 *
 * @see xtsc_event_register
 */
XTSC_API sc_core::sc_event& xtsc_event_get(const std::string& event_name);



/**
 * This method dumps a list of all events registered with XTSC (SystemC 2.2) or that are
 * hierarchically-named events (SystemC 2.3).
 *
 * The xtsc_event_XXX API's allow the XTSC command facility to work with SystemC events.
 *
 * @param       os              The ostream object to dump the event list to.
 *
 * @param       pattern         Optional pattern to match events against.  The pattern
 *                              may contain 1 or more asterisks (*) as wildcards.  Each
 *                              * matches 0 or more characters in any combination.
 *
 * @see xtsc_event_register
 */
XTSC_API void xtsc_event_dump(std::ostream& os = std::cout, const std::string& pattern = "*");



/**
 * This method opens a named, recursive mutex and returns a handle to it.  If the mutex
 * already exists, its handle is returned.
 *
 * @param       name            The name of the mutex.
 *
 * Note:  Host mutex support in XTSC is an experimental feature.
 *
 * @see xtsc_host_mutex_lock
 * @see xtsc_host_mutex_try_lock
 * @see xtsc_host_mutex_unlock
 * @see xtsc_host_mutex_close
 */
XTSC_API xtsc_host_mutex *xtsc_host_mutex_open(const std::string& name);



/**
 * This method does a recursive lock on the specified mutex.
 *
 * @param       p_mutex         The mutex handle as returned by the xtsc_host_mutex_open
 *                              method.
 *
 * Note:  Host mutex support in XTSC is an experimental feature.
 *
 * @see xtsc_host_mutex_open
 * @see xtsc_host_mutex_try_lock
 * @see xtsc_host_mutex_unlock
 * @see xtsc_host_mutex_close
 */
XTSC_API void xtsc_host_mutex_lock(xtsc_host_mutex *p_mutex);



/**
 * This method attempts a recursive lock with a timeout as specified in milliseconds on
 * the specified mutex.
 *
 * @param       p_mutex         The mutex handle as returned by the xtsc_host_mutex_open
 *                              method.
 *
 * @param       milliseconds    The maximum number of milliseconds to wait for the lock
 *                              to succeeded.
 *
 * @return true if the lock attempt succeeded, false if the lock attempt timeout out
 * (implying the lock is currently held by some other host process).
 *
 * Note:  Host mutex support in XTSC is an experimental feature.
 *
 * @see xtsc_host_mutex_open
 * @see xtsc_host_mutex_lock
 * @see xtsc_host_mutex_unlock
 * @see xtsc_host_mutex_close
 */
XTSC_API bool xtsc_host_mutex_try_lock(xtsc_host_mutex *p_mutex, u64 milliseconds = 0);



/**
 * This method does a recursive unlock on the specified mutex.  The unlock is recursive
 * in that the mutex is not actually unlocked until this method is called a number of
 * times equal to the total of the number of times that xtsc_host_mutex_lock() was
 * called plus the number of times that xtsc_host_mutex_try_lock() was called and
 * returned true.
 *
 * @param       p_mutex         The mutex handle as returned by the xtsc_host_mutex_open
 *                              method.
 *
 * Note:  Host mutex support in XTSC is an experimental feature.
 *
 * @see xtsc_host_mutex_open
 * @see xtsc_host_mutex_lock
 * @see xtsc_host_mutex_try_lock
 * @see xtsc_host_mutex_close
 */
XTSC_API void xtsc_host_mutex_unlock(xtsc_host_mutex *p_mutex);



/**
 * This method closes the specified mutex.  If the mutex is currently locked by this OS
 * process, then xtsc_host_mutex_unlock() is called as many times as necessary to unlock
 * it.
 *
 * @param       p_mutex         The mutex handle as returned by the xtsc_host_mutex_open
 *                              method.
 *
 * Note:  Host mutex support in XTSC is an experimental feature.
 *
 * @see xtsc_host_mutex_open
 * @see xtsc_host_mutex_lock
 * @see xtsc_host_mutex_try_lock
 * @see xtsc_host_mutex_unlock
 */
XTSC_API void xtsc_host_mutex_close(xtsc_host_mutex *p_mutex);



/**
 * This method returns the number of milliseconds of wall time since some unspecified
 * beginning time.  This method uses clock_gettime() on Linux and GetTickCount64() on
 * MS Windows.
 *
 */
XTSC_API u64 xtsc_host_milliseconds();



/**
 * This method causes a host OS sleep for the specified number of milliseconds.  This
 * method uses nanosleep() on Linux and Sleep() on MS Windows.
 *
 * @param       milliseconds    The number of milliseconds to sleep.
 *
 */
XTSC_API void xtsc_host_sleep(u64 milliseconds = 0);



/**
 * Return a unique 32-bit number suitable for use as watchfilter number.
 *
 * @see xtsc_filter
 */
XTSC_API u32 xtsc_get_next_watchfilter_number();



///  Vector of key,value pairs constituting the main content of an xtsc_filter
typedef std::vector<std::pair<std::string, std::string> > xtsc_filter_table;



/**
 * The xtsc_filter class, in conjunction with the xtsc_filter_XXX() and xtsc_event_XXX()
 * methods and the XTSC command facility, help support the system control and debug
 * framework in XTSC.
 *
 * Typically an xtsc_filter is used to define a pattern that is applied to the payloads
 * being passed by one of the Xtensa TLM interfaces method calls (for example,
 * nb_request(), nb_respond(), nb_peek, and nb_poke()).  If a payload matches the filter
 * then an sc_event associated with that application of the xtsc_filter is notified.
 *
 * Note: The filter does NOT determine whether of not the payload is allowed through the
 *       interface (the payload always goes through).  The filter only determines
 *       whether or not the associated sc_event is notified.
 *
 * The basic idea is that something in the simulation will be waiting on the associated
 * sc_event.  Usually, this would be either the XTSC command prompt, the lua command
 * prompt, or one of the lua script file threads, but it could also be any other SystemC
 * thread or method process in the simulation.  If either of the command prompts is
 * waiting on the sc_event, then when it is notified, the user is presented with the
 * command prompt and can then use any of the XTSC commands to probe the system or issue
 * a breakpoint_interrupt to the ISS to allow probing the system from xt-gdb or Xtensa
 * Xplorer.
 *
 * The xtsc_filter class is a container for a table of key-value pairs
 * (xtsc_filter_table).  An xtsc_filter instance must be of a registered filter kind.
 * The kind of filter defines what keys are allowed.  Filter kinds must be registered
 * with XTSC using xtsc_filter_kind_register().  
 *
 * The xtsc_filter kinds currently supported by the XTSC libraries and their allowed
 * keys are:
 * \verbatim
   xtsc_request        xtsc_response       xtsc_peek           xtsc_poke
   -----------------   -------------       ----------          ----------
   address*            address*            address*            address*
   buffer              buffer              buffer              buffer
   buffer_not          buffer_not          buffer_not          buffer_not
   burst               burst
   byte_enables*
   cache*
   domain*
   exclusive           exclusive
   id*                 id*
   instruction_fetch
   last_transfer       last_transfer
   num_transfers*
   pc*                 pc*
   pif_attribute*
   port*               port*               port*               port*
   prot*
   priority*           priority*
   route_id*           route_id*
   size*               size*               size*               size*
                       status
   snoop*
   tag*                tag*
   type                 
   \endverbatim
 *
 * Note: Filter kinds starting with "xtsc" are reserved to the XTSC libraries.
 *
 * In the above list an asterisk (*) after a key indicates the key supports values with
 * ranges.  This also indicates that the value (or value range) will be converted to a
 * number (or a pair of numbers) and a numeric comparision will be done with the payload
 * item identified by the key.
 *
 * Key values are strings and the xtsc_filter kinds listed above all support the key
 * value being a comma-separated list of values.  The comma is taken as a logical OR for
 * all the above keys except buffer_not where it is taken as a logical AND.
 *
 * For keys listed above without an asterisk, the following special considerations or
 * allowed values apply:
 *
 * buffer:  A string comparison is done with the buffer contents as they would be shown
 * in the xtsc.log file.  The period is a wildcard that means one don't-care character.  
 * Each byte of the payload buffer is specified using 3 characters (a triplet) in the
 * buffer value.  The first character of the triplet specifies the high nibble of the
 * payload byte, the second character of the triplet specifies the low nibble of the
 * payload byte, and the third character of the triplet must always be the period
 * wildcard.  If desired either or both of the first two characters of a triplet can
 * also be periods.  The length of the buffer value must be exactly 3 times the size of
 * the buffer in the payload.  Multiple comma-separated buffer values are allowed and
 * each comma is interpreted as a logical OR.  For example, to match the 1st, 2nd, and
 * 4th byte (and ignore the 3rd byte) of a 4 byte payload that shows in the xtsc.log
 * file as:
 * \verbatim
   00 01 02 03 
   \endverbatim
 * The buffer value should be specified using 12 characters like this:
 * \verbatim
   00.01....03.
   \endverbatim
 * In the context of the XTSC command prompt, it would look something like:
 * \verbatim
   cmd: xtsc xtsc_filter_create xtsc_request req1_filter buffer=00.01....03.
   \endverbatim
 *
 * buffer_not: Same rules as shown for buffer above except that multiple comma-separated
 * values imply a logical AND between them instead of a logical OR.
 *
 * burst: NON_AXI | FIXED | INCR | WRAP
 *
 * exclusive: 0 | 1
 *
 * instruction_fetch: 0 | 1
 *
 * type: READ | BLOCK_READ | BURST_READ | RCW | WRITE | BLOCK_WRITE | BURST_WRITE |
 *       SNOOP
 *
 * last_transfer: 0 | 1
 *
 * status:  RSP_NACC | RSP_OK | RSP_ADDRESS_ERROR | RSP_DATA_ERROR |
 *          RSP_ADDRESS_DATA_ERROR | AXI_OK | EXOKAY | DECERR | SLVERR | NOTRDY
 *          
 * Except for buffer_not, the above keys do not have a built-in NOT EQUAL concept;
 * however, you can get the same effect by specifying all but the undesired value.  For
 * example:
 * \verbatim
     For this               Do This
     --------------------   ------------------------------------------------------------
     address!=0xf0000000    address=0-0xefffffff,0xf0000001-0xffffffff
     size!=0                size=1-64
     status!=RSP_NACC       status=RSP_OK,RSP_ADDRESS_ERROR,RSP_DATA_ERROR,RSP_ADDRESS_DATA_ERROR 
   \endverbatim
 *
 * The watchfilter concept:
 *
 * A watchfilter is analogous to a gdb breakpoint or watchpoint.  The idea is that an
 * xtsc_filter and an sc_event are given to a model which supports watchfilters.  The
 * model returns a watchfilter number which can be used to later remove the watchfilter.
 * While the watchfilter is active, the model is responsible for checking each payload
 * against the filter and notifying the event if there is a match.
 *
 * XTSC provides the following convenience API's to actually test a specific payload
 * against a specific xtsc_filter (currently xtsc_core, xtsc_component::xtsc_router,
 * and xtsc_component::xtsc_memory support watchfilters using these APIs).
 *
 *   xtsc_filter_apply_xtsc_request()
 *
 *   xtsc_filter_apply_xtsc_response()
 *
 *   xtsc_filter_apply_xtsc_peek()
 *
 *   xtsc_filter_apply_xtsc_poke()
 *
 * An Example:
 *
 * The following transcript shows using the xtsc_router example to:
 *
 * 1) Create an xtsc_filter named filt1 of kind xtsc_request which contains key-value
 *    pairs to detect any WRITE or BLOCK_WRITE to addresses in the range of
 *    0x60000000-0x6FFFFFFF.  Note: The command is shown on two lines for printing
 *    considerations, in fact the entire command must be entered on one line.
 *
 * 2) Create a watchfilter in router (an xtsc_router) using filt1 and
 *    xtsc_command_prompt_event.  This watchfilter is assigned watchfilter number 1.
 *
 * 3) Continue the simulation by suspending the XTSC command prompt until its event is
 *    notified.
 *
 * 4) After the XTSC command prompt returns, dump the last filtered request to see what
 *    it was.
 *
 * 5) Disassemble the current instruction of core0.
 *
 * 6) Enable debugging on core0.  Not shown:  In another window connect xt-gdb to the
 *    simulation, investigate program state, set breakpoints as desired, and continue
 *    the simulation (from xt-gdb's point-of-view).
 *
 * 7) Continue the simulation by again suspending the XTSC command prompt.
 *
 * 8) Tell core0 to send a breakpoint interrupt to the ISS to cause xt-gdb to return to
 *    its prompt.
 *
 * 9) Tell the SystemC kernel to run the simulation for 1 cycle to give the ISS a chance
 *    to act on the breakpoint interrupt.
 *
 * 10) Remove all watchfilters from router.
 *
 * 11) Continue the simulation by again suspending the XTSC command prompt.
 *
 * \verbatim
        ./xtsc_router -xtsc_command_prompt=true
     1) cmd: xtsc xtsc_filter_create xtsc_request filt1 address=0x60000000-0x6FFFFFFF 
                                                        type=WRITE,BLOCK_WRITE
     2) cmd: router watchfilter_add filt1 xtsc_command_prompt_event
        1
     3) cmd: c
           44708.3/623: 
     4) cmd: router dump_filtered_request
        tag=4162 pc=0x40000101 WRITE* [0x6000a47c/4/0x000f/0/01/0x4b0]= 9c 81 00 60 
     5) cmd: core0 dasm pc
        0x40000103: l32r a4,0x4000002c
     6) cmd: core0 enable_debug 1 0
        core0: SOCKET:20000
        NOTE core0 - 44708.3/623: Debug info: port=20000 wait=true (target/router_test.out)
     7) cmd: c
           44732.3/706: 
     8) cmd: core0 breakpoint_interrupt
     9) cmd: sc wait 1
           44733.3/709: 
    10) cmd: router watchfilter_remove *
        1
    11) cmd: c
   \endverbatim
 *
 * @see xtsc_filter_table
 * @see xtsc_filter_kind_register
 * @see xtsc_filter_kind_dump
 * @see xtsc_filter_exists
 * @see xtsc_filter_create
 * @see xtsc_filter_dump
 * @see xtsc_filter_get
 * @see xtsc_filter_apply_xtsc_peek
 * @see xtsc_filter_apply_xtsc_poke
 * @see xtsc_filter_apply_xtsc_request
 * @see xtsc_filter_apply_xtsc_response
 * @see xtsc_get_next_watchfilter_number()
 * @see xtsc_event_register
 * @see xtsc_event_exists
 * @see xtsc_event_get
 * @see xtsc_event_dump
 * @see xtsc_component::xtsc_router::watchfilter_add
 * @see xtsc_component::xtsc_router::watchfilter_dump
 * @see xtsc_component::xtsc_router::watchfilter_remove
 * @see xtsc_component::xtsc_router::execute command dump_filtered_request
 * @see xtsc_component::xtsc_router::execute command dump_filtered_response
 * @see xtsc_initialize_parms parameter "xtsc_command_prompt" 
 * @see xtsc_initialize_parms parameter "lua_command_prompt" 
 * @see xtsc_initialize_parms parameter "lua_script_files" 
 */
class XTSC_API xtsc_filter {
public:

  /// Return the kind of this xtsc_filter
  const std::string& get_kind() const { return m_kind; }

  /// Return the unique name of this xtsc_filter
  const std::string& get_name() const { return m_name; }

  /// Return a reference to the key-value pairs specified by this xtsc_filter
  const xtsc_filter_table& get_key_value_pairs() const { return m_key_value_pairs; }

private:

  friend XTSC_API const xtsc_filter& xtsc_filter_create(const std::string& kind,
                                                        const std::string& name,
                                                        const xtsc_filter_table& key_value_pairs);

  /// Private constructor.  Use xtsc_filter_create() to create an xtsc_filter instance.
  xtsc_filter(const std::string& kind, const std::string& name, const xtsc_filter_table& key_value_pairs);

  std::string           m_kind;                 ///< The kind of xtsc_filter
  std::string           m_name;                 ///< The unique name of this xtsc_filter
  xtsc_filter_table     m_key_value_pairs;      ///< The key-value pairs specified by this filter.  Both keys and values are strings.
};



/**
 * Method to register a new xtsc_filter kind.
 *
 * The XTSC libraries currently define xtsc_filter kinds of "xtsc_request",
 * "xtsc_response", "xtsc_peek", and "xtsc_poke".  The xtsc_filter_kind_register()
 * method will throw an exception if kind has already been registered.
 *
 * Note: Filter kinds starting with "xtsc" are reserved to the XTSC libraries.
 *
 * @param       kind            The filter kind which must not already be registered.
 *
 * @param       keys            The names of the allowed keys in an xtsc_filter of this
 *                              kind.
 *
 * @param       range_keys      The names of the keys which may have ranges specified
 *                              for their value.  All entries in range_keys set must
 *                              also be in the keys set.
 *
 * @see xtsc_filter
 * @see xtsc_filter_kind_dump
 */
XTSC_API void xtsc_filter_kind_register(const std::string&              kind,
                                        const std::set<std::string>&    keys,
                                        const std::set<std::string>&    range_keys);


/**
 * Method to dump the list of keys of a specific xtsc_filter kind or to dump a list of
 * all registered xtsc_filter kinds.
 *
 * The xtsc_filter_kind_dump() method dumps a list of the allowed keys of the specified
 * xtsc_filter kind to the specified ostream.  The key name is followed by an asterisk
 * if the key value is allowed to have a range of values specified.  If kind is not
 * specified, then this method dumps a list of all xtsc_filter kinds.
 *
 * @param       kind            The desired filter kind or empty to get a list of all
 *                              registerd filter kinds.
 *
 * @param       os              The ostream object to which the list is to be dumped.
 *
 * @see xtsc_filter
 * @see xtsc_filter_kind_register
 */
XTSC_API void xtsc_filter_kind_dump(const std::string& kind = "", std::ostream& os = std::cout);



/**
 * Method to determine if an xtsc_filter of a given name exists.
 *
 * This method returns true if an xtsc_filter of the specified name exists.  Otherwise
 * it returns false.
 *
 * @param       name            The xtsc_filter name of interest.
 *
 * @see xtsc_filter
 * @see xtsc_filter_create
 * @see xtsc_filter_dump
 * @see xtsc_filter_get
 */
XTSC_API bool xtsc_filter_exists(const std::string& name);



/**
 * Method to create an xtsc_filter of the specified kind with the specified name.
 *
 * This method creates and returns an xtsc_filter of the specified kind and with the
 * specified name and key-value pairs.  An exception is thrown if kind is not
 * registered, if name already exists, or if key_value_pair contains a key which was not
 * specified when the xtsc_filter kind was registered.
 *
 * @param       kind            The kind of the xtsc_filter object.  This kind must have
 *                              already been registered with XTSC.
 *
 * @param       name            The name of the xtsc_filter object.  No other
 *                              xtsc_filter (even of a different kind) may have this
 *                              same name.  Filter names may be formed using the same
 *                              character requirements as C/C++ identifiers.
 *
 * @param       key_value_pairs The table (vector) of key-value pairs. Both keys and
 *                              values are strings.
 *
 * @see xtsc_filter_kind_register
 * @see xtsc_filter
 * @see xtsc_filter_dump
 * @see xtsc_filter_exists
 * @see xtsc_filter_get
 */
XTSC_API const xtsc_filter& xtsc_filter_create(const std::string&       kind,
                                               const std::string&       name,
                                               const xtsc_filter_table& key_value_pairs);



/**
 * Method to dump a list of filters of the specified kind or to dump the key-value pairs
 * of a specific named xtsc_filter.
 *
 * This method dumps a list of xtsc_filter names of the specified kind or it dumps a
 * list of the key-value pairs of the named xtsc_filter.  It both kind and name are
 * specified, name is ignored.  If neither kind nor name are specified, a list of the
 * names of all xtsc_filter objects and their associated filter kind is dumped.
 *
 * @param       kind            The kind of xtsc_filter objects to list.
 *
 * @param       name            The name of a specific xtsc_filter whose key-value pairs
 *                              are to be dumped.
 *
 * @param       os              The ostream object to which the list is to be dumped.
 *
 * @see xtsc_filter_kind_register
 * @see xtsc_filter
 * @see xtsc_filter_create
 * @see xtsc_filter_exists
 * @see xtsc_filter_get
 */
XTSC_API void xtsc_filter_dump(const std::string& kind = "", const std::string& name = "", std::ostream& os = std::cout);



/**
 * Method to get a reference to the xtsc_filter object specified by name.
 *
 * This method throws an exception if the named xtsc_filter does not exist.
 *
 * @param       name            The name of a the desired xtsc_filter.
 *
 * @see xtsc_filter
 * @see xtsc_filter_create
 * @see xtsc_filter_exists
 * @see xtsc_filter_dump
 */
XTSC_API const xtsc_filter& xtsc_filter_get(const std::string& name);



/**
 * Convenience method to a apply an xtsc_filter of kind xtsc_peek to an nb_peek payload.
 *
 * This method returns true if the payload passes the filter, otherwise it returns
 * false.  This method should only be called after the buffer is populated.
 *
 * @param       name            The name of the desired xtsc_filter.
 *
 * @param       port            The port the nb_peek call came in on.  Ports are
 *                              numbered starting at 0.
 *
 * @param       address         The address specified in the nb_peek call.
 *
 * @param       size            The size specified in the nb_peek call.
 *
 * @param       buffer          The buffer specified in the nb_peek call.
 *
 * @see xtsc_filter
 */
XTSC_API bool xtsc_filter_apply_xtsc_peek(const std::string&    name,
                                          xtsc::u32             port,
                                          xtsc::xtsc_address    address,
                                          xtsc::u32             size,
                                          const xtsc::u8       *buffer);



/**
 * Convenience method to a apply an xtsc_filter of kind xtsc_poke to an nb_poke payload.
 *
 * This method returns true if the payload passes the filter, otherwise it returns
 * false.
 *
 * @param       name            The name of the desired xtsc_filter.
 *
 * @param       port            The port the nb_poke call came in on.  Ports are
 *                              numbered starting at 0.
 *
 * @param       address         The address specified in the nb_poke call.
 *
 * @param       size            The size specified in the nb_poke call.
 *
 * @param       buffer          The buffer specified in the nb_poke call.
 *
 * @see xtsc_filter
 */
XTSC_API bool xtsc_filter_apply_xtsc_poke(const std::string&    name,
                                          xtsc::u32             port,
                                          xtsc::xtsc_address    address,
                                          xtsc::u32             size,
                                          const xtsc::u8       *buffer);



/**
 * Convenience method to apply an xtsc_filter of kind xtsc_request to an nb_request
 * payload.
 *
 * This method returns true if the payload passes the filter, otherwise it returns
 * false.
 *
 * @param       name            The name of the desired xtsc_filter.
 *
 * @param       port            The port the nb_request call came in on.  Ports are
 *                              numbered starting at 0.
 *
 * @param       request         The xtsc_request specified in the nb_request call.
 *
 * @see xtsc_filter
 */
XTSC_API bool xtsc_filter_apply_xtsc_request(const std::string& name, xtsc::u32 port, const xtsc_request& request);



/**
 * Convenience method to a apply an xtsc_filter of kind xtsc_response to an nb_respond
 * payload.
 *
 * This method returns true if the payload passes the filter, otherwise it returns
 * false.
 *
 * @param       name            The name of the desired xtsc_filter.
 *
 * @param       port            The port the nb_respond call came in on.  Ports are
 *                              numbered starting at 0.
 *
 * @param       response        The xtsc_response specified in the nb_respond call.
 *
 * @see xtsc_filter
 */
XTSC_API bool xtsc_filter_apply_xtsc_response(const std::string& name, xtsc::u32 port, const xtsc_response& response);



/**
 * Interface for objects which can be reset.
 *
 * Note: If a component has any SystenC thread processes, then those processes may need
 *       to be restarted in order to get a clean reset.  SystemC 2.3 provides mechanisms
 *       for doing this that were previously not available.
 *
 * Here is a 6-step idiom for adding thread process reset to an xtsc_module (for an
 * example, see m_process_handles, sc_unwind_exception, reset_fifos(), and cancel() in
 * xtsc_component::xtsc_arbiter):
 *
 * 1) Add member:
 * \verbatim
        std::vector<sc_core::sc_thread_process>  m_process_handles;
   \endverbatim
 *
 * 2) Add the newly created process to m_process_handles table immediately after each
 *    desired SC_THREAD() macro or declare_thread_process() invocation.  For example,
 *    change this:
 * \verbatim
        SC_THREAD(worker_thread);
   \endverbatim
 *    To this:
 * \verbatim
        SC_THREAD(worker_thread); m_process_handles.push_back(sc_get_current_process_handle());
   \endverbatim
 *
 * 3) In the modules xtsc_resettable::reset() implementation, add a call to 
 *    xtsc_reset_processes():
 * \verbatim
        xtsc_reset_processes(m_process_handles);
   \endverbatim
 *
 * 4) In the modules xtsc_resettable::reset() implementation, call cancel() on any
 *    applicable sc_event objects and call cancel_all() on any applicable sc_event_queue
 *    objects.  For example:
 * \verbatim
        m_arbiter_thread_event.cancel();
   \endverbatim
 *
 * 5) Catch and rethrow sc_unwind_exception in each applicable SystemC thread process.
 *    In the XTSC components, this means changing this:
 * \verbatim
        catch (const exception& error) { ... }
   \endverbatim
 *    To this:
 * \verbatim
        catch (const sc_unwind_exception& error) { throw; }
        catch (const exception& error) { ... }
   \endverbatim
 *    Typically, the catch clause should either not log or should only log at verbose or
 *    debug level because xtsc_reset_processes() will log each process that is reset.
 *
 * 6) Reset any applicable thread state.  This may involve emptying standard library
 *    container and/or sc_fifo instances.  Note that an element cannot be removed from
 *    an sc_fifo in the same delta cycle in which it was added, so the general case may
 *    require waiting a delta cycle which should NOT be done from the reset() method.
 *    One technique is to do a wait(SC_ZERO_TIME) when first entering an SC_THREAD and
 *    then emptying any desired sc_fifo objects.  For an example, see
 *    xtsc_component::xtsc_arbiter::reset_fifos().
 *
 * @see xtsc_reset_processes()
 * @see xtsc_reset
 */
class XTSC_API xtsc_resettable {
public:
  xtsc_resettable();
  /// Reset the object
  virtual void reset(bool hard_reset) = 0;
  virtual ~xtsc_resettable() {};
private:
  xtsc_resettable(const xtsc_resettable&);
  xtsc_resettable& operator=(const xtsc_resettable&);
};



/**
 * This method calls xtsc_resettable::reset(hard_reset) on all objects of type
 * xtsc_resettable.
 *
 * Note: Most XTSC models ignore the hard_reset argument.  Those that do use it (for
 *       example, xtsc_component::xtsc_memory and xtsc_component::xtsc_lookup), tend to
 *       use hard_reset of true to indicate that memory contents should be restored to
 *       its initial value.
 *
 * Note: After calling this method, you will typically need to reload the target program
 *       of any xtsc_core instances in your system.
 *
 * @return the number of xtsc_resettable objects.
 */
XTSC_API u32 xtsc_reset(bool hard_reset);





/**
 * This enum specifies the conjugate port pairs that are supported by the generic
 * connection API (xtsc_connection_interface) and the generic connection method,
 * xtsc_connect().
 *
 * The conjugate port pairs are separated by half of the enum range (128 = 256/2) which
 * has the special enum name of conjugate_delta.
 *
 * Loosely speaking, two port types are "conjugate pairs" if they can be bound to each
 * other.
 *
 * For example, a LOOKUP_PORT (sc_port<xtsc_lookup_if>) has enum value 4 and it can be
 * bound to a LOOKUP_EXPORT (sc_export<xtsc_lookup_if) which has enum 132 (= 4 + 128),
 * thus LOOKUP_PORT and LOOKUP_EXPORT are a conjugate pair.
 *
 * As another example, an INITIATOR_SOCKET_4 (tlm_initiator_socket<32>) has enum value
 * 33 and it can be bound to a TARGET_SOCKET_4 (tlm_target_socket<32>) which has enum
 * 161 (= 33 + 128), thus INITIATOR_SOCKET_4 and TARGET_SOCKET_4 are a conjugate pair.
 *
 * The pin-level ports work nearly the same way.  A BOOL_OUTPUT (sc_out<bool>) has enum
 * value 17 and its conjugate is BOOL_INPUT (sc_in<bool>) which has enum value 145
 * (= 17 + 128).  Although BOOL_OUTPUT cannot be bound directly to BOOL_INPUT, the two
 * can be connected together using an sc_signal<bool> and thus, for our purposes, are
 * considered a conjugate pair.  Note: The xtsc_connect() method will take care of
 * creating the required sc_signal<T> when connecting the pin-level types.
 * 
 * A special exception to the conjugate pair rule is PORT_TABLE which has enum value 255
 * (and which has no conjugate enum).  This port type is used to designate a named and
 * ordered list (vector<string>) of ports.  If two module instances have PORT_TABLE
 * entries of the same size and each port in one list of ports is the conjugate of the
 * corresponding port in the other list, then the two port tables are said to form a
 * conjugate pair and they can be connected together.  That is, their constituent ports
 * can be connected together by just specifying the two tables to the xtsc_connect()
 * method or to the generic connect command in xtsc-run (i.e. --connect=...).
 *
 * A PORT_TABLE entry can also be used to create an alias for a given port.
 *
 * Note: Using a PORT_TABLE is the recommended mechanism for creating aliases rather
 *       then having multiple non-PORT_TABLE entries for the same port.  Using a
 *       PORT_TABLE to define aliases makes it clearer to the user what ports are
 *       available because each non-PORT_TABLE entry identifies a separate real port.
 *
 * The sc_port<> entries are shown here without template parameters values for int N and
 * sc_port_policy P which might seem to imply they are only good for the default values
 * of N=1 and P=SC_ONE_OR_MORE_BOUND (see the declaration of the sc_port template in the
 * SystemC header file sc_port.h).  In fact, the entry is good for any value of N or P;
 * however, if N is greater then 4 then you should set the "xtsc_port_type_check_bypass"
 * parameter in xtsc_initialize_parms to true.  This applies to xtsc_port_type values
 * from DEBUG_PORT through TX_XFER_PORT.
 *
 * For xtsc_port_type values from DEBUG_EXPORT through TX_XFER_EXPORT, the sc_object
 * returned by the xtsc_connection_interface::get_port() method may be an sc_export<T>
 * or simply a T.
 *
 * @see xtsc_connection_interface
 * @see xtsc_connect
 */
enum xtsc_port_type {

  USER_DEFINED_PORT             =   0,  ///< sc_port  <T>                         where T is a user-defined sc_interface
  DEBUG_PORT                    =   1,  ///< sc_port  <xtsc_debug_if      >
  REQUEST_PORT                  =   2,  ///< sc_port  <xtsc_request_if    >
  RESPOND_PORT                  =   3,  ///< sc_port  <xtsc_respond_if    >
  LOOKUP_PORT                   =   4,  ///< sc_port  <xtsc_lookup_if     >
  QUEUE_PUSH_PORT               =   5,  ///< sc_port  <xtsc_queue_push_if >
  QUEUE_POP_PORT                =   6,  ///< sc_port  <xtsc_queue_pop_if  >
  WIRE_WRITE_PORT               =   7,  ///< sc_port  <xtsc_wire_write_if >
  WIRE_READ_PORT                =   8,  ///< sc_port  <xtsc_wire_read_if  >
  TX_XFER_PORT                  =   9,  ///< sc_port  <xtsc_tx_xfer_if    >
  EXT_REGFILE_PORT              =  10,  ///< sc_port  <xtsc_ext_regfile_if>

  USER_DEFINED_OUTPUT           =  16,  ///< sc_out<T           >                 where T is a user-defined type
  BOOL_OUTPUT                   =  17,  ///< sc_out<bool        >
  UINT_OUTPUT                   =  18,  ///< sc_out<sc_uint_base>
  WIDE_OUTPUT                   =  19,  ///< sc_out<sc_bv_base  >

  USER_DEFINED_INITIATOR        =  32,  ///< tlm_initiator_socket< BW>            where BW is a user-defined bit width
  INITIATOR_SOCKET_4            =  33,  ///< tlm_initiator_socket< 32>
  INITIATOR_SOCKET_8            =  34,  ///< tlm_initiator_socket< 64>
  INITIATOR_SOCKET_16           =  35,  ///< tlm_initiator_socket<128>
  INITIATOR_SOCKET_32           =  36,  ///< tlm_initiator_socket<256>
  INITIATOR_SOCKET_64           =  37,  ///< tlm_initiator_socket<512>

  conjugate_delta               = 128,  ///< Begin conjugate pair

  USER_DEFINED_EXPORT           = 128,  ///< sc_export<T                  >        where T is a user-defined sc_interface
  DEBUG_EXPORT                  = 129,  ///< sc_export<xtsc_debug_if      >
  REQUEST_EXPORT                = 130,  ///< sc_export<xtsc_request_if    >
  RESPOND_EXPORT                = 131,  ///< sc_export<xtsc_respond_if    >
  LOOKUP_EXPORT                 = 132,  ///< sc_export<xtsc_lookup_if     >
  QUEUE_PUSH_EXPORT             = 133,  ///< sc_export<xtsc_queue_push_if >
  QUEUE_POP_EXPORT              = 134,  ///< sc_export<xtsc_queue_pop_if  >
  WIRE_WRITE_EXPORT             = 135,  ///< sc_export<xtsc_wire_write_if >
  WIRE_READ_EXPORT              = 136,  ///< sc_export<xtsc_wire_read_if  >
  TX_XFER_EXPORT                = 137,  ///< sc_export<xtsc_tx_xfer_if    >
  EXT_REGFILE_EXPORT            = 138,  ///< sc_export<xtsc_ext_regfile_if>

  USER_DEFINED_INPUT            = 144,  ///< sc_in <T           >                 where T is a user-defined type
  BOOL_INPUT                    = 145,  ///< sc_in <bool        >
  UINT_INPUT                    = 146,  ///< sc_in <sc_uint_base>
  WIDE_INPUT                    = 147,  ///< sc_in <sc_bv_base  >

  USER_DEFINED_TARGET           = 160,  ///< tlm_target_socket   < BW>            where BW is a user-defined bit width
  TARGET_SOCKET_4               = 161,  ///< tlm_target_socket   < 32>
  TARGET_SOCKET_8               = 162,  ///< tlm_target_socket   < 64>
  TARGET_SOCKET_16              = 163,  ///< tlm_target_socket   <128>
  TARGET_SOCKET_32              = 164,  ///< tlm_target_socket   <256>
  TARGET_SOCKET_64              = 165,  ///< tlm_target_socket   <512>

  PORT_TABLE                    = 255,  ///< A named and ordered table of ports
};



/// Map port names to their xtsc_port_type
typedef std::map<std::string,xtsc_port_type>  xtsc_port_type_map;



/// A table of port names
typedef std::vector<std::string>              xtsc_port_table;



/**
 * Return a string showing the C++ type of the port.
 *
 * @param  bare         If true, return the Xtensa TLM interface type as a string
 *                      (port_type must refer to an sc_export of one of the Xtensa TLM
 *                      interfaces; that is DEBUG_EXPORT <= port_type <= TX_XFER_EXPORT).
 *                      For example, if port_type is WIRE_WRITE_EXPORT, then this method
 *                      will return "sc_export<xtsc_wire_write_if>" when bare is false
 *                      and "xtsc_wire_write_if" when bare is true.
 *
 * @see xtsc_port_type_check
 */
XTSC_API std::string xtsc_get_port_type_name(xtsc_port_type port_type, bool bare = false);



/**
 * Returns true if the two xtsc_port_type values correspond to conjugate port pairs.
 */
XTSC_API bool xtsc_are_conjugate_port_types(xtsc_port_type type1, xtsc_port_type type2);



/**
 * Returns true if the specified port type is a user-defined type.
 *
 * The user-defined port types are:
 *   USER_DEFINED_PORT
 *   USER_DEFINED_EXPORT
 *   USER_DEFINED_INITIATOR
 *   USER_DEFINED_TARGET
 *   USER_DEFINED_OUTPUT
 *   USER_DEFINED_INPUT
 *
 * @see xtsc_connection_interface::get_user_defined_port_type
 */
XTSC_API bool xtsc_is_user_defined_port_type(xtsc_port_type port_type);



/**
 * Returns true if the interface associated with the specified port type is an Xtensa
 * TLM interface.  
 *
 * Note: Returns false for USER_DEFINED_PORT and USER_DEFINED_EXPORT.
 */
XTSC_API bool xtsc_is_xttlm_port_type(xtsc_port_type port_type);



/**
 * Returns true if the interface associated with the specified port type is an OSCI TLM2
 * interface.
 *
 * Note: Returns true for USER_DEFINED_INITIATOR and USER_DEFINED_TARGET.
 */
XTSC_API bool xtsc_is_tlm2_port_type(xtsc_port_type port_type);



/**
 * Returns true if the specified port type is a pin-level port type (sc_out<> or
 * sc_in<>).
 *
 * Note: Returns true for USER_DEFINED_OUTPUT and USER_DEFINED_INPUT.
 */
XTSC_API bool xtsc_is_pin_level_port_type(xtsc_port_type port_type);





/**
 * This is the generic connection interface used to support plugin modules and the
 * --connect command in xtsc-run as well as the xtsc_connect() method in the
 * XTSC core library.
 *
 * Note: Derived classes must:
 *
 *  -  Populate m_port_types in their constructor (taking into consideration any
 *     construction parameters which affect what ports the module instance has).
 *  -  Implement the get_bit_width() method.
 *  -  Implement the get_port() method.
 *  -  Implement the get_default_port_name() method if desired to have a default port.
 *  -  Implement the get_port_table() method if m_port_types contains any port of
 *     xtsc_port_type PORT_TABLE.
 *  -  Implement the get_user_defined_port_type() method if m_port_types contains
 *     any port whose xtsc_port_type is one of the user-defined port types (that
 *     is, any port for which xtsc_is_user_defined_port_type() returns true).
 *  -  Implement the connect_user_defined_port_type() method if m_port_types
 *     contains any port of type USER_DEFINED_PORT, USER_DEFINED_OUTPUT, or
 *     USER_DEFINED_INITIATOR.
 *
 * For working examples which illustrate implementing this interface please see the
 * simple_memory.plugin and pif2sb_bridge.plugin sub-directories of the XTSC examples
 * directory installed with each Tensilica core config in:
 *
 * -  <xtsc_examples_root>/simple_memory.plugin
 * -  <xtsc_examples_root>/pif2sb_bridge.plugin
 *
 * Note: Port names in m_port_types are case-sensitive.
 *
 * @see xtsc_port_type
 * @see xtsc_connect
 * @see xtsc_plugin_interface
 */
class XTSC_API xtsc_connection_interface {
public:


  /**
   * Constructor for an xtsc_connection_interface.
   *
   * @param     module          A reference to the associated sc_module or sc_prim_channel subclass.
   */
  xtsc_connection_interface(sc_core::sc_object& module);


  /// Destructor
  virtual ~xtsc_connection_interface();


  /**
   * Get a reference to the sc_object (sc_module or sc_prim_channel) that this
   * xtsc_connection_interface is associated with.
   *
   * Note: Derived classes should NOT implement this method.
   */
  sc_core::sc_object& get_object() { return m_object; }


  /**
   * Get a copy of m_port_types, the xtsc_port_type_map which enumerates all module
   * ports and their associated xtsc_port_type.
   *
   * Note: Derived classes must populate m_port_types in their constructor.
   *
   * Note: Derived classes should NOT implement this method.
   */
  xtsc::xtsc_port_type_map get_port_type_map() { return m_port_types; }


  /**
   *  Return true if the module instance has the specified port.
   *
   *  @param    port_name       The port name of interest.
   *
   * Note: Derived classes should NOT implement this method.
   */
  bool has_port(const std::string& port_name) const;


  /**
   * Return the xtsc_port_type of the specified port.  An exception is thrown if 
   * m_port_types has no such port. 
   *
   * @param     port_name       The port name of interest from m_port_types.
   *
   * Note: Derived classes should NOT implement this method.
   */
  xtsc_port_type get_port_type(const std::string& port_name) const;


  /**
   * Get the bit-width of the specified port.  The non-pin-level interfaces may return
   * 0 to indicate that bit-width is a don't care (for example, an xtsc_router that
   * does not consider the data width of the xtsc_request/xtsc_respond objects).
   *
   * @param     port_name       A port name from m_port_types whose xtsc_port_type is
   *                            not PORT_TABLE.
   * 
   * @param     interface_num   Typically 0.  Can be set to 1 if port_name names a port
   *                            of xtsc_port_type LOOKUP_PORT or LOOKUP_EXPORT or 
   *                            EXT_REGFILE_PORT or EXT_REGFILE_EXPORT to obtain
   *                            the bit width of the lookup address (for these four port
   *                            types, interface_num of 0 refers to the lookup data
   *                            interface bit width).
   *
   * Note: Derived classes must implement this method.
   */
  virtual u32 get_bit_width(const std::string& port_name, u32 interface_num = 0) const = 0;


  /**
   * Get a pointer to the specified port.
   *
   * @param     port_name       A port name from m_port_types whose xtsc_port_type is
   *                            not PORT_TABLE.
   * 
   * Note: Derived classes must implement this method.
   */
  virtual sc_core::sc_object *get_port(const std::string& port_name) = 0;


  /**
   * Return a string specifying the name of the default port.
   *
   * The name returned by this method must either be the empty string ("") or it must be
   * a named port from the m_port_types table.
   *
   * Note: This is an optional method.  Derived classes need implement this method only
   *       if they wish to define a default port.
   */
  virtual std::string get_default_port_name() const { return ""; }


  /**
   * Return an ordered list (vector<string>) of port names comprising the named
   * PORT_TABLE specified by port_table_name.  
   *
   * @param     port_table_name         A named port of xtsc_port_type PORT_TABLE from
   *                                    m_port_types.
   *
   * Note: Derived classes must implement this method only if m_port_types contains a
   *       port of xtsc_port_type PORT_TABLE.
   *
   * Note: Port tables can be nested.  That is, one or more of the port names in the
   *       xtsc_port_table returned by this method may themselves be of xtsc_port_type
   *       PORT_TABLE.
   *
   * @see get_resolved_port_table()
   */
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /**
   * Return a string, T, identifying the user-defined type.  Two ports will be considered
   * to be a conjugate pair if their T, as returned by this method, is the same and one
   * of the following three two-part statements is true:
   *
   *  - one port is a USER_DEFINED_PORT      and the other port is a USER_DEFINED_EXPORT
   *  - one port is a USER_DEFINED_OUTPUT    and the other port is a USER_DEFINED_INPUT
   *  - one port is a USER_DEFINED_INITIATOR and the other port is a USER_DEFINED_TARGET
   *
   * The string contents returned by this method are recommended to be the user-defined
   * T from the following table:
   * \verbatim
     xtsc_port_type           How T is construed
     ----------------------   ----------------------------------------------------------

     USER_DEFINED_PORT        sc_port  <T>        where T is a user-defined sc_interface
     USER_DEFINED_EXPORT      sc_export<T>        where T is a user-defined sc_interface

     USER_DEFINED_OUTPUT      sc_out<T>           where T is a user-defined type
     USER_DEFINED_INPUT       sc_in <T>           where T is a user-defined type

     USER_DEFINED_INITIATOR   T="tlm_socket<BW>"  where BW is a user-defined bit width
     USER_DEFINED_TARGET      T="tlm_socket<BW>"  where BW is a user-defined bit width
     \endverbatim
   *
   * @param     port_name       A named port from m_port_types for which the
   *                            xtsc_is_user_defined_port_type() returns true.
   *
   * Note: Derived classes must implement this method only if their m_port_types
   *       contains a port for which xtsc_is_user_defined_port_type() returns true.
   *
   * @see xtsc_is_user_defined_port_type()
   */
  virtual std::string get_user_defined_port_type(const std::string& port_name) const;


  /**
   * Perform port-binding between the locally-known port specified by port_name and the
   * foreign port pointed to by p_object.  This method is responsible for confirming
   * that p_object is of the correct type.  If the locally-known port is an sc_out<T>
   * then this method is also responsible for creating the sc_signal<T> object necessary
   * to connect an sc_out<T> to an sc_in<T>.
   *
   * @param     port_name       A named port from m_port_types for which the
   *                            xtsc_is_user_defined_port_type() returns true.
   *
   * @param     p_object        A pointer to an sc_object whose type is the conjugate of
   *                            port_name.  
   *
   * @param     signal_name     The name to give to the signal that this method must
   *                            create if the locally-known port is an sc_out<T>.
   *
   * Note: Derived classes must implement this method only if m_port_types contains a
   *       port of type USER_DEFINED_PORT, USER_DEFINED_OUTPUT, or
   *       USER_DEFINED_INITIATOR.
   *
   * @see xtsc_is_user_defined_port_type()
   */
  virtual void connect_user_defined_port_type(const std::string&        port_name,
                                              sc_core::sc_object       *p_object,
                                              const std::string&        signal_name);


  /**
   * This is a convenience method that returns an ordered list (vector<string>) of port
   * names comprising the port or port table specified by port_name and with all
   * port table entries resolved to their constituent ports.  If port_name is not of
   * xtsc_port_type PORT_TABLE, then the list will contain a single entry equal to
   * port_name.  If port_name is of xtsc_port_type PORT_TABLE, then the list will be the
   * same as that returned by the get_port_table() method except that any names in that
   * table which are of xtsc_port_type PORT_TABLE will also be replaced by their
   * constituent ports and so on recursively.
   *
   * Note: Derived classes should NOT implement this method.
   *
   * @param     port_name       A named port from m_port_types.
   */
  xtsc::xtsc_port_table get_resolved_port_table(const std::string& port_name) const;


  /**
   * This convenience method dumps a list of the elementary ports (Part 1), port table
   * names (Part 2), and port table definitions (Part 3) that this connection interface
   * has.
   *
   * Note: Derived classes should NOT implement this method.
   *
   * Example usage:
   * \verbatim
        xtsc_core core0("core0", core_parms);
        core0.dump_ports_and_tables();
     \endverbatim
   *
   * @param     os              The ostream object to dump the port list to.
   *
   * @param     ns              The number of spaces to allow for the first column of output.
   *
   * @param     verbose         If true, include bit width of interface in Parts 1 and 3.
   */
  void dump_ports_and_tables(std::ostream& os = std::cout, xtsc::u32 ns = 35, bool verbose = false) const;


protected:
  sc_core::sc_object&           m_object;       ///< Must be an sc_module or sc_prim_channel subclass.
  xtsc::xtsc_port_type_map      m_port_types;   ///< Derived classes must populate this table in their constructor 
                                                ///< See xtsc_connect()

private:

  mutable std::set<std::string> m_port_table_resolution_set;    // Detect infinite recursion in port table definition

  xtsc_connection_interface(const xtsc_connection_interface&);
  xtsc_connection_interface& operator=(const xtsc_connection_interface&);

};



/**
 * This method will throw an exception if the two specified ports are not conjugate
 * user-defined port types.
 *
 * @see xtsc_connection_interface::get_user_defined_port_type
 */
XTSC_API void xtsc_confirm_conjugate_user_defined_port_types(xtsc_connection_interface& instance_a,
                                                             const std::string&         port_a,
                                                             const std::string&         port_b,
                                                             xtsc_connection_interface& instance_b);



/**
 * Determine if the specified sc_object is of the specified xtsc_port_type.
 *
 * If the sc_object pointed to by p_port is not of the xtsc_port_type specified by
 * port_type, then this method will either throw an exception (if the
 * "xtsc_port_type_check_bypass" xtsc_initialize_parms parameter is false) or log an
 * INFO_LOG_LEVEL message and return (if the "xtsc_port_type_check_bypass" parameter is
 * true).
 *
 * If this method does not throw an exception then it normally returns false; however,
 * if port_type refers to an sc_export of one of the Xtensa TLM interfaces
 * (DEBUG_EXPORT <= port_type <= TX_XFER_EXPORT) and p_port points directly to an
 * implementation of that interface instead of pointing to an sc_export of that
 * interface, then this method will return true.
 *
 * @see xtsc_initialize_parms
 * @see xtsc_port_type
 * @see xtsc_get_port_type_name
 */
XTSC_API bool xtsc_port_type_check(xtsc_port_type port_type, sc_core::sc_object *p_port);



/**
 * This method can be used to connect two modules using named ports (or port tables)
 * from each module.  Each named port must appear as an entry in its respective
 * xtsc_connection_interface::m_port_types map.  In addition, the two named ports must be
 * conjugates of each other.
 *
 * Note: Port names are case-sensitive.
 *
 * @return The number of elementary port pairs connected.  Note: For an N-ported Xtensa
 * TLM memory interface, this method will return 2*N (1 for the request channel and 1
 * for the response channel).
 *
 * @see xtsc_port_type
 * @see xtsc_connection_interface
 * @see xtsc_connection_interface::m_port_types
 * @see xtsc_core::How_to_do_port_binding;
 */
XTSC_API xtsc::u32 xtsc_connect(xtsc_connection_interface&      instance_a,
                                const std::string&              port_a,
                                const std::string&              port_b,
                                xtsc_connection_interface&      instance_b);



/**
 * This composite interface combines the xtsc_connection_interface and xtsc_resettable
 * interfaces.  The XTSC core and component modules all use this composite interface.
 */
class XTSC_API xtsc_module : public xtsc_connection_interface, public xtsc_resettable {
public:


  /**
   * Constructor for an xtsc_module.
   *
   * @param     module          A reference to the associated sc_module.
   */
  xtsc_module(sc_core::sc_module& module) : xtsc_connection_interface(module) {}


  /// Destructor
  virtual ~xtsc_module() {}


  /**
   * The implementation of reset() in xtsc_module logs a warning and does nothing else.
   * Subclasses should provide their own implementation if they are able to support
   * reset.
   */
  void reset(bool hard_reset = false);


private:

  xtsc_module(const xtsc_module&);
  xtsc_module& operator=(const xtsc_module&);

};







/**
 * This interface is used to add plugin modules to an XTSC simulation.
 *
 * Plugin support for a custom module can be added to an XTSC simulation by first
 * creating (writing, compiling, and linking) the plugin and then by using it in
 * xtsc-run or sc_main.
 *
 * To create a plugin:
 *
 * - Implement the xtsc_connection_interface for the custom module.
 * - Implement this interface (xtsc_plugin_interface) for the custom module.
 * - Implement and export a method with the signature defined by
 *   xtsc::xtsc_get_plugin_interface_t.  Typically this method is called 
 *   xtsc_get_plugin_interface() but you may wish to use a unique name if you also
 *   intend to make a static version of the library available (for example, for
 *   SystemC-Verilog co-simulation).
 * - Compile and link the above 3 items as a shared library (Linux shared object or MS
 *    Windows DLL).  
 *    IMPORTANT NOTE: The plugin used in xtsc-run must be compiled and linked against
 *    the shared-library versions of SystemC and XTSC, not the static versions.  The
 *    shared library version of the SystemC header files are installed under
 *    XtensaTools/include/systemc-X.Y.Z-shared.  The shared library version of the XTSC
 *    header files are obtained by compiling with the preprocessor macro XTSC_IS_DSO
 *    defined.  The shared library names end in "_sh" (or "_shd" for MS Windows Debug),
 *    so you should link against systemc_sh, xtsc_sh, and xtsc_comp_sh instead of
 *    systemc, xtsc, and xtsc_comp.
 * - Provide the resulting shared library to the end-user along with the actual name
 *   of the exported method of type xtsc::xtsc_get_plugin_interface_t.
 *
 * To use a plugin from xtsc-run, pass the optional path and name of the shared library
 * (without the extension) to xtsc-run using the --load_library command of xtsc-run.
 *
 * To see one way to use a plugin from sc_main, add the --sc_main command to an xtsc-run
 * script in which you do a --load_library and then configure, create, and connect the
 * plugin module.  You can then inspect the sc_main file that xtsc-run will generate
 * when it processes the script.
 *
 * For working examples which illustrate implementing this interface please see the
 * simple_memory.plugin and pif2sb_bridge.plugin sub-directories of the XTSC examples
 * directory installed with each Tensilica core config in:
 *
 * -  <xtsc_examples_root>/simple_memory.plugin
 * -  <xtsc_examples_root>/pif2sb_bridge.plugin
 *
 * For a step-by-step guide to take an XTSC module from the XTSC component library and
 * then rename, modify, and re-build it as an XTSC plugin please see the README.txt file 
 * in the xtsc.component.plugin sub-directory of the XTSC examples directory installed
 * with each Tensilica core config in:
 *
 * -  <xtsc_examples_root>/xtsc.component.plugin
 *
 * @see xtsc_connection_interface
 * @see xtsc_get_plugin_interface_t
 */
class XTSC_API xtsc_plugin_interface {
public:


  /// Destructor
  virtual ~xtsc_plugin_interface() {}


  /**
   * The implementation of this API must return the set of plugin names that this shared
   * library supports.
   *
   * The xtsc-run program requires that plugin names be valid C/C++ identifiers.  In
   * addition it is recommended that they contain no uppercase letters and that they
   * neither begin nor end with the underscore (_) character.
   *
   * Note: To use a plugin whose name has uppercase letters or begins or ends with the
   *       underscore character, requires adding a plugin name redefinition to the
   *       --load_library command passed to xtsc-run.
   */
  virtual std::set<std::string> get_plugin_names() = 0;


  /**
   * The implementation of this API must generate help information to the specified
   * ostream object for the specified plugin name.
   *
   * @param plugin_name         The plugin name for which help information should be
   *                            generated.  This name is guarranteed to be from the set
   *                            returned by the get_plugin_names() method.
   *
   * @param verbose             If true, verbose help should be generated.
   *
   * @param os                  The ostream object to which the help information should
   *                            be generated.
   */
  virtual void help(const std::string& plugin_name, bool verbose, std::ostream& os) = 0;


  /**
   * The implementation of this API must return a pointer to a valid, newly-constructed
   * xtsc_parms object.  Each call to this API must return a pointer to a different
   * (unique) xtsc_parms object.  The parameter names, types, and initial values
   * contained in the xtsc_parms object are up to the implementation but should
   * correspond to the plugin type specified by plugin_name.  The xtsc-run
   * progam will allow the user to change the parameter values as desired prior to
   * passing the xtsc_parms object back in the create_module() method when the user
   * requests that an instance of the plugin be created.
   *
   * @param plugin_name         The plugin name for which an xtsc_parms object should be
   *                            returned.  This name is guarranteed to be from the set
   *                            returned by the get_plugin_names() method.
   */
  virtual xtsc::xtsc_parms *create_parms(const std::string& plugin_name) = 0;


  /**
   * The implementation of this API must return a reference to a valid, newly-constructed
   * sc_module of the type specified by plugin_name.  Each call to this API must
   * return a reference to a different (unique) newly-constructed sc_module object.
   *
   * @param plugin_name         The plugin name for which an sc_module object should be
   *                            returned.  This name is guarranteed to be from the set
   *                            returned by the get_plugin_names() method.
   *
   * @param instance_name       The SystemC base instance name to be given to the
   *                            sc_module.  This name should be passed to the sc_module
   *                            constructor as an sc_module_name.
   *
   * @param p_parms             A pointer to an xtsc_parms object previously obtained
   *                            by a call to the create_parms() method.  The parameter
   *                            values will have been modified as desired by the user.
   */
  virtual sc_core::sc_module& create_module(const std::string&        plugin_name,
                                            const std::string&        instance_name,
                                            const xtsc::xtsc_parms   *p_parms) = 0;


  /**
   * The implementation of this API must return a pointer to the
   * xtsc_connection_interface associated with the plugin module instance identified by
   * hierarchical_name.  
   *
   * @param hierarchical_name   The full hierarchical name of a sc_module instance
   *                            previously created by a call to create_module().  
   *
   * Note: Under certain circumstances the hierarchical name can differ from the
   *       instance_name passed in to the create_module method.  The implementation can
   *       call the sc_module::name() method from within the create_module() method to
   *       determine the hierarchical name after creating the sc_module.
   */
  virtual xtsc::xtsc_connection_interface *get_connection_interface(const std::string& hierarchical_name) = 0;

};



/**
 * The type of the method used by XTSC to get the plugin interface from the shared
 * library.
 *
 * The shared library (Linux shared object or MS Windows DLL) must implement and export
 * a method with the type defined by xtsc_get_plugin_interface_t.  Typically, this
 * method is called xtsc_get_plugin_interface(); however, if you also intend to make a
 * static version of the library available (for example, for SystemC-Verilog
 * co-simulation), then you may wish to use a unique name to avoid a name clash with 
 * other linkedin models.
 *
 * The following code snippet shows how to declare this method in a manner that works
 * on both Linux and MS Windows:
 * \verbatim
        extern "C" 
        #if defined(_WIN32)
        __declspec(dllexport)
        #endif
        xtsc_plugin_interface& xtsc_get_plugin_interface() { ... }
   \endverbatim
 *
 * @see xtsc_plugin_interface
 */
typedef xtsc_plugin_interface& (*xtsc_get_plugin_interface_t)();



// {
//  The main purpose of the following is to ensure there is a string in any XTSC binary that Xtensa Xplorer can
//  find to help set up the environment for the user.


#define XTSC_REALQUOTE(a1)  #a1
#define XTSC_QUOTE(a1)      XTSC_REALQUOTE(a1)


#define XTSC_TOKEN_PASTER3_(a1, a2, a3)                     a1 ## a2 ## a3
#define XTSC_TOKEN_PASTER_3(a1, a2, a3)                     XTSC_TOKEN_PASTER3_(a1, a2, a3)

#define XTSC_TOKEN_PASTER5_(a1, a2, a3, a4, a5)             a1 ## a2 ## a3 ## a4 ## a5
#define XTSC_TOKEN_PASTER_5(a1, a2, a3, a4, a5)             XTSC_TOKEN_PASTER5_(a1, a2, a3, a4, a5)

#define XTSC_TOKEN_PASTER7_(a1, a2, a3, a4, a5, a6, a7)     a1 ## a2 ## a3 ## a4 ## a5 ## a6 ## a7
#define XTSC_TOKEN_PASTER_7(a1, a2, a3, a4, a5, a6, a7)     XTSC_TOKEN_PASTER7_(a1, a2, a3, a4, a5, a6, a7)

#define XTSC_TOKEN_PASTER8_(a1, a2, a3, a4, a5, a6, a7, a8) a1 ## a2 ## a3 ## a4 ## a5 ## a6 ## a7 ## a8
#define XTSC_TOKEN_PASTER_8(a1, a2, a3, a4, a5, a6, a7, a8) XTSC_TOKEN_PASTER8_(a1, a2, a3, a4, a5, a6, a7, a8)


// Get XTENSA_SWVERSION_NAME_IDENT from xtensa-versions.h
#define XTSC_VERSION_INFO_XTENSATOOLS XTSC_TOKEN_PASTER_3(XtensaTools_, XTENSA_SWVERSION_NAME_IDENT, __)


#if defined(__linux__)
//#define XTSC_VERSION_INFO_COMPILER XTSC_TOKEN_PASTER_5(GCC_, __GNUC__, _, __GNUC_MINOR__, __)
#define XTSC_VERSION_INFO_COMPILER GCC__
#define XTSC_VERSION_INFO_TARGET Linux__
#elif defined(__APPLE__)
//#define XTSC_VERSION_INFO_COMPILER XTSC_TOKEN_PASTER_7(Clang_, __clang_major__, _, __clang_minor__, _, __clang_patchlevel__, __)
#define XTSC_VERSION_INFO_COMPILER Clang__
#define XTSC_VERSION_INFO_TARGET Darwin__
#elif defined(_WIN32)
#if   (_MSC_VER < 1600)
#error "Unsupported MSVC compiler version in xtsc.h"
#elif (_MSC_VER < 1700)
#define XTSC_VERSION_INFO_COMPILER MSVC2010__
#elif (_MSC_VER < 1800)
#define XTSC_VERSION_INFO_COMPILER MSVC2012__
#elif (_MSC_VER < 1900)
#define XTSC_VERSION_INFO_COMPILER MSVC2013__
#elif (_MSC_VER < 1910)
#define XTSC_VERSION_INFO_COMPILER MSVC2015__
#elif (_MSC_VER < 1920)
#define XTSC_VERSION_INFO_COMPILER MSVC2017__
#elif (_MSC_VER < 1930)
#define XTSC_VERSION_INFO_COMPILER MSVC2019__
#else
#error "Unsupported MSVC compiler version in xtsc.h: "
#endif
#if defined(_DEBUG)
#define XTSC_VERSION_INFO_TARGET Debug__
#else
#define XTSC_VERSION_INFO_TARGET Release__
#endif
#endif


#if   defined(__x86_64__) || defined(_M_X64)
#define XTSC_VERSION_INFO_ARCH 64Bit__
#else
#define XTSC_VERSION_INFO_ARCH 32Bit__
#endif


// These need to be update each time we add a new vendor: SystemC, Cadence, Synopsys vcs, Synopsys CoWare, SocDesigner, Mentor, ASTC, etc 
#if   defined(NCSC_REG_H)
#define XTSC_VERSION_INFO_VENDOR Cadence__
#elif defined(XTSC_SD)
#define XTSC_VERSION_INFO_VENDOR Carbon__
#elif defined(CWR_SYSTEMC)
#define XTSC_VERSION_INFO_VENDOR CoWare__
#elif defined(MTI_SYSTEMC)
#define XTSC_VERSION_INFO_VENDOR Mentor__
#elif defined(VCS_SYSTEMC)
#define XTSC_VERSION_INFO_VENDOR Synopsys__
#elif defined(USK_MAJOR_VERSION) && defined(USK_MINOR_VERSION) 
#define XTSC_VERSION_INFO_VENDOR ASTC_USK__
#elif defined(OSCAR_MAJOR_VERSION) && defined(OSCAR_MINOR_VERSION)  && defined(OSCAR_MICRO_VERSION) 
#define XTSC_VERSION_INFO_VENDOR XTSC_TOKEN_PASTER_7(oscar_, OSCAR_MAJOR_VERSION, _, OSCAR_MINOR_VERSION, _, OSCAR_MICRO_VERSION, __)
#elif defined(SC_API_VERSION_STRING) && (SC_API_VERSION_STRING == sc_api_version_2_2_0) && (SYSTEMC_VERSION == 20070314)
#define XTSC_VERSION_INFO_VENDOR OSCI__
#elif defined(SC_API_VERSION_STRING)
#define XTSC_VERSION_INFO_VENDOR Accellera__
#else
#define XTSC_VERSION_INFO_VENDOR Unknown__
#endif

#if defined(SC_API_VERSION_STRING)
#define XTSC_VERSION_INFO_SYSTEMC_VERSION SC_API_VERSION_STRING 
#else
#define XTSC_VERSION_INFO_SYSTEMC_VERSION Unknown
#endif
// Some known SC_API_VERSION_STRING 
//  sc_api_version_2_2_0
//  sc_api_version_2_3_0
//  sc_api_version_2_3_1
//  ncsc_api_version_ius_13_2
//  ncsc_api_version_ius_14_1
//  ncsc_api_version_ius_14_2
//  ncsc_api_version_ius_15_1
//  ncsc_api_version_ius_15_2
//  snps_vcs_sc_api_version_h_2013_06
//  snps_vcs_sc_api_version_i_2014_03
//  snps_vcs_sc_api_version_j_2014_12
//  snps_vcs_sc_api_version_j_2015_09       (this is actually vcs K-2015.09)
//  snps_vcs_sc_api_version_l_2016_06
//  snps_sls_vp_sc_api_version_h_2013_06
//  snps_sls_vp_sc_api_version_j_2014_06
//  snps_sls_vp_sc_api_version_j_2014_06_3
//  snps_sls_vp_sc_api_version_j_2014_12
//  snps_sls_vp_sc_api_version_k_2015_06
//  snps_sls_vp_sc_api_version_k_2015_12
//  snps_sls_vp_sc_api_version_l_2016_06
//  snps_sls_vp_sc_api_version_m_2016_12


#define XTSC_VERSION_INFO_STRING XTSC_TOKEN_PASTER_8(xtsc_version_info__,               \
                                                     XTSC_VERSION_INFO_XTENSATOOLS,     \
                                                     XTSC_VERSION_INFO_COMPILER,        \
                                                     XTSC_VERSION_INFO_TARGET,          \
                                                     XTSC_VERSION_INFO_ARCH,            \
                                                     XTSC_VERSION_INFO_VENDOR,          \
                                                     XTSC_VERSION_INFO_SYSTEMC_VERSION, \
                                                     __end)



//  The following is based off the SystemC SC_API_VERSION_STRING technique in their sc_ver.h/sc_ver.cpp files. 
class XTSC_API XTSC_VERSION_INFO_STRING {
  public:
    XTSC_VERSION_INFO_STRING();
};


static XTSC_VERSION_INFO_STRING xtsc_version_info_check;

// }




#ifndef DOXYGEN_SKIP
// For backward compatibility
#define xtsc_register_mode_switch_if        xtsc_register_mode_switch_interface
#define xtsc_get_registered_mode_switch_ifs xtsc_get_registered_mode_switch_interfaces
#endif // DOXYGEN_SKIP


} // namespace xtsc



#endif  // _XTSC_H_
