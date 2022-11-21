#ifndef _XTSC_LOGGING_H_
#define _XTSC_LOGGING_H_

// Copyright (c) 2005-2010 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

/**
 * @file 
 */


#include <xtsc/xtsc.h>
#include <xtsc/xtsc_request.h>
#include <xtsc/xtsc_response.h>


#if 0
#define xtsc_logging
#endif

namespace xtsc {



/**
 * This method can be used to log a memory request transaction.
 *
 * @param       logger           The BinaryLogger to use to log the event.
 *
 * @param       log_level        The log level of this event (typically, 
 *                               VERBOSE_LOG_LEVEL or INFO_LOG_LEVEL).
 *
 * @param       incoming         true indicates the xtsc_request is being
 *                               received by the device and false indicates
 *                               the xtsc_request is being sent out of the
 *                               device.
 *
 * @param       port_number      The port number this request was received
 *                               on (when incoming is true) or sent out on
 *                               (when incoming is false).
 *
 * @param       log_data         If true, log data (if any) associated with
 *                               this request.  If false, do not log data from
 *                               request, regardless of whether or not the 
 *                               request has data associated with it.
 *                               Note: Only WRITE, BLOCK_WRITE, and RCW 
 *                               requests have data associated with them.
 *
 * @param       request          A reference to the xtsc_request object.
 *
 */
XTSC_API
void xtsc_log_memory_request_event(log4xtensa::BinaryLogger&    logger,
                                   log4xtensa::LogLevel         log_level,
                                   bool                         incoming,
                                   u32                          port_number,
                                   bool                         log_data,
                                   const xtsc_request&          request);



/**
 * This method can be used to log a memory response transaction.
 *
 * @param       logger           The BinaryLogger to use to log the event.
 *
 * @param       log_level        The log level of this event (typically, 
 *                               VERBOSE_LOG_LEVEL or INFO_LOG_LEVEL).
 *
 * @param       incoming         true indicates the xtsc_response is being
 *                               received by the device and false indicates
 *                               the xtsc_response is being sent out of the
 *                               device.
 *
 * @param       port_number      The port number this response was received
 *                               on (when incoming is true) or sent out on
 *                               (when incoming is false).
 *
 * @param       log_data         If true, log data (if any) associated with
 *                               this response.  If false, do not log data from
 *                               response, regardless of whether or not the 
 *                               response has data associated with it.
 *                               Note: Only READ, BLOCK_READ, and RCW 
 *                               responses have data associated with them.
 *
 * @param       response         A reference to the xtsc_response object.
 *
 */
XTSC_API
void xtsc_log_memory_response_event(log4xtensa::BinaryLogger&   logger,
                                    log4xtensa::LogLevel        log_level,
                                    bool                        incoming,
                                    u32                         port_number,
                                    bool                        log_data,
                                    const xtsc_response&        response);



/**
 * This method can be used to log a read wire or write state event to a 
 * TLM TIE port.
 *
 * @param       logger           The BinaryLogger to use to log the event.
 *
 * @param       log_level        The log level of this event (typically, 
 *                               VERBOSE_LOG_LEVEL or INFO_LOG_LEVEL).
 *
 * @param       port_number      The TIE port number.
 *
 * @param       type             The transaction type (READ_WIRE or WRITE_STATE).
 *
 * @param       program_counter  PC of the core reading the wire or writing state.
 *                               If not know, use log4xtensa::UNKNOWN_PC.
 *
 * @param       log_data         If true, log buffer data.  If false, do not 
 *                               log buffer data.
 *
 * @param       buffer           The sc_unsigned object being read or written.
 *
 */
XTSC_API
void xtsc_log_tie_port_event(log4xtensa::BinaryLogger&       logger,
                             log4xtensa::LogLevel            log_level,
                             u32                             port_number,
                             log4xtensa::TiePortTransaction  type,
                             xtsc_address                    program_counter,
                             bool                            log_data,
                             const sc_dt::sc_unsigned&       buffer); 



/**
 * This method can be used to log a read wire or write state event to a 
 * pin-level TIE port.
 *
 * @param       logger           The BinaryLogger to use to log the event.
 *
 * @param       log_level        The log level of this event (typically, 
 *                               VERBOSE_LOG_LEVEL or INFO_LOG_LEVEL).
 *
 * @param       port_number      The TIE port number.
 *
 * @param       type             The transaction type (READ_WIRE or WRITE_STATE).
 *
 * @param       program_counter  PC of the core reading the wire or writing state.
 *                               If not know, use log4xtensa::UNKNOWN_PC.
 *
 * @param       log_data         If true, log buffer data.  If false, do not 
 *                               log buffer data.
 *
 * @param       buffer           The sc_bv_base object being read or written.
 *
 */
XTSC_API
void xtsc_log_tie_port_event(log4xtensa::BinaryLogger&       logger,
                             log4xtensa::LogLevel            log_level,
                             u32                             port_number,
                             log4xtensa::TiePortTransaction  type,
                             xtsc_address                    program_counter,
                             bool                            log_data,
                             const sc_dt::sc_bv_base&        buffer); 



/**
 * This method can be used to log a TLM push or pop in a queue or to a queue.
 *
 * @param       logger           The BinaryLogger to use to log the event.
 *
 * @param       log_level        The log level of this event (typically, 
 *                               VERBOSE_LOG_LEVEL or INFO_LOG_LEVEL).
 *
 * @param       ticket           The ticket returned by the queue push
 *                               or queue pop call.
 *
 * @param       port_number      The TIE port number.  If not know, use
 *                               log4xtensa::UNKNOWN.
 *
 * @param       type             The transaction type (PUSH or POP).
 *
 * @param       program_counter  PC of the core pushing/popping the queue.
 *                               If not know, use log4xtensa::UNKNOWN_PC.
 *
 * @param       queue_quantity   Number of elements in queue AFTER this 
 *                               PUSH or POP event.  If not known, use
 *                               log4xtensa::UNKNOWN.
 *
 * @param       queue_capacity   The total capacity of the queue (queue_capacity
 *                               minus queue_quantity should equal the available 
 *                               empty space in the queue).  If not known, use
 *                               log4xtensa::UNKNOWN.
 *
 * @param       log_data         If true, log element data.  If false, do not 
 *                               log element data.
 *
 * @param       element          The sc_unsigned object being pushed or popped.
 *
 */
XTSC_API
void xtsc_log_queue_event(log4xtensa::BinaryLogger&     logger,
                          log4xtensa::LogLevel          log_level,
                          u64                           ticket,
                          u32                           port_number,
                          log4xtensa::QueueTransaction  type,
                          xtsc_address                  program_counter,
                          u32                           queue_quantity,
                          u32                           queue_capacity,
                          bool                          log_data,
                          const sc_dt::sc_unsigned&     element);


/**
 * This method can be used to log a pin-level push or pop in a queue or to a queue.
 *
 * @param       logger           The BinaryLogger to use to log the event.
 *
 * @param       log_level        The log level of this event (typically, 
 *                               VERBOSE_LOG_LEVEL or INFO_LOG_LEVEL).
 *
 * @param       ticket           The ticket returned by the queue push
 *                               or queue pop call.
 *
 * @param       port_number      The TIE port number.  If not know, use
 *                               log4xtensa::UNKNOWN.
 *
 * @param       type             The transaction type (PUSH or POP).
 *
 * @param       program_counter  PC of the core pushing/popping the queue.
 *                               If not know, use log4xtensa::UNKNOWN_PC.
 *
 * @param       queue_quantity   Number of elements in queue AFTER this 
 *                               PUSH or POP event.  If not known, use
 *                               log4xtensa::UNKNOWN.
 *
 * @param       queue_capacity   The total capacity of the queue (queue_capacity
 *                               minus queue_quantity should equal the available 
 *                               empty space in the queue).  If not known, use
 *                               log4xtensa::UNKNOWN.
 *
 * @param       log_data         If true, log element data.  If false, do not 
 *                               log element data.
 *
 * @param       element          The sc_bv_base object being pushed or popped.
 *
 */
XTSC_API
void xtsc_log_queue_event(log4xtensa::BinaryLogger&     logger,
                          log4xtensa::LogLevel          log_level,
                          u64                           ticket,
                          u32                           port_number,
                          log4xtensa::QueueTransaction  type,
                          xtsc_address                  program_counter,
                          u32                           queue_quantity,
                          u32                           queue_capacity,
                          bool                          log_data,
                          const sc_dt::sc_bv_base&      element);


/**
 * This method can be used to log a TLM TIE lookup event of type LOOKUP_KEY or
 * RESPONSE_DATA.
 *
 * @param       logger           The BinaryLogger to use to log the event.
 *
 * @param       log_level        The log level of this event (typically, 
 *                               VERBOSE_LOG_LEVEL or INFO_LOG_LEVEL).
 *
 * @param       port_number      The TIE port number.  If not know, use
 *                               log4xtensa::UNKNOWN.
 *
 * @param       type             The transaction type (LOOKUP_KEY or
 *                               RESPONSE_DATA).
 *
 * @param       program_counter  PC of the core generating the lookup event.
 *                               If not know, use log4xtensa::UNKNOWN_PC.
 *
 * @param       log_data         If true, log contents of data.  If false, do not 
 *                               log contents of data.
 *
 * @param       data             The sc_unsigned object being pushed or popped.
 *
 */
XTSC_API
void xtsc_log_lookup_event(log4xtensa::BinaryLogger&     logger,
                           log4xtensa::LogLevel          log_level,
                           u32                           port_number,
                           log4xtensa::LookupTransaction type,
                           xtsc_address                  program_counter,
                           bool                          log_data,
                           const sc_dt::sc_unsigned&     data);


/**
 * This method can be used to log a pin-level TIE lookup event of type LOOKUP_KEY or
 * RESPONSE_DATA.
 *
 * @param       logger           The BinaryLogger to use to log the event.
 *
 * @param       log_level        The log level of this event (typically, 
 *                               VERBOSE_LOG_LEVEL or INFO_LOG_LEVEL).
 *
 * @param       port_number      The TIE port number.  If not know, use
 *                               log4xtensa::UNKNOWN.
 *
 * @param       type             The transaction type (LOOKUP_KEY or
 *                               RESPONSE_DATA).
 *
 * @param       program_counter  PC of the core generating the lookup event.
 *                               If not know, use log4xtensa::UNKNOWN_PC.
 *
 * @param       log_data         If true, log contents of data.  If false, do not 
 *                               log contents of data.
 *
 * @param       data             The sc_bv_base object being pushed or popped.
 *
 */
XTSC_API
void xtsc_log_lookup_event(log4xtensa::BinaryLogger&     logger,
                           log4xtensa::LogLevel          log_level,
                           u32                           port_number,
                           log4xtensa::LookupTransaction type,
                           xtsc_address                  program_counter,
                           bool                          log_data,
                           const sc_dt::sc_bv_base&      data);


/**
 * This method can be used to log a TIE lookup event of type READY.
 *
 * @param       logger           The BinaryLogger to use to log the event.
 *
 * @param       log_level        The log level of this event (typically, 
 *                               VERBOSE_LOG_LEVEL or INFO_LOG_LEVEL).
 *
 * @param       port_number      The TIE port number.  If not know, use
 *                               log4xtensa::UNKNOWN.
 *
 * @param       program_counter  PC of the core generating the lookup event.
 *                               If not know, use log4xtensa::UNKNOWN_PC.
 *
 * @param       log_data         If true, log value of ready.  If false, do not 
 *                               log value of ready.
 *
 * @param       ready            The value of the TIE_xxx_Rdy signal.
 *
 */
XTSC_API
void xtsc_log_lookup_event(log4xtensa::BinaryLogger&     logger,
                           log4xtensa::LogLevel          log_level,
                           u32                           port_number,
                           xtsc_address                  program_counter,
                           bool                          log_data,
                           bool                          ready);


/**
 * Function to record an event name and ID association.
 *
 * @param       logger          The BinaryLogger to use to record the association
 *
 * @param       event_name_id   The ID of the event name.  Typically this was optained 
 *                              from the log4xtensa::BinaryLogger::getUniqueEventNameID()
 *                              method.
 *
 * @param       event_name      The event name.
 *
 */
XTSC_API
void xtsc_record_event_name(log4xtensa::BinaryLogger&   logger,
                            u32                         event_name_id,
                            const char                 *event_name);


/**
 * This method can be used to log a data event.
 *
 * @param       logger          The BinaryLogger to use to log the event.
 *
 * @param       log_level       The log level of this event (typically, 
 *                              VERBOSE_LOG_LEVEL or INFO_LOG_LEVEL).
 *
 * @param       event_name_id   The ID of the event name.  Typically this was optained 
 *                              from the log4xtensa::BinaryLogger::getUniqueEventNameID()
 *                              method.
 *
 * @param       log_data        If true, log data.  If false, do not log data.
 *
 * @param       data            The sc_unsigned object containing the data
 *                              to be logged.
 *
 */
XTSC_API
void xtsc_log_data_event(log4xtensa::BinaryLogger&      logger,
                         log4xtensa::LogLevel           log_level,
                         u32                            event_name_id,
                         bool                           log_data,
                         const sc_dt::sc_unsigned&      data);


/**
 * This method can be used to log a string event.
 *
 * @param       logger          The BinaryLogger to use to log the event.
 *
 * @param       log_level       The log level of this event (typically, 
 *                              VERBOSE_LOG_LEVEL or INFO_LOG_LEVEL).
 *
 * @param       event_name_id   The ID of the event name.  Typically this was optained 
 *                              from the log4xtensa::BinaryLogger::getUniqueEventNameID()
 *                              method.
 *
 * @param       data            The c-string containing the data to be logged.
 *
 */
XTSC_API
void xtsc_log_string_event(log4xtensa::BinaryLogger&      logger,
                           log4xtensa::LogLevel           log_level,
                           u32                            event_name_id,
                           const char                    *data);


/**
 * This method can be used to log an elementary event.
 *
 * @param       logger          The BinaryLogger to use to log the event.
 *
 * @param       log_level       The log level of this event (typically, 
 *                              VERBOSE_LOG_LEVEL or INFO_LOG_LEVEL).
 *
 * @param       event_name_id   The ID of the event name.  Typically this was optained 
 *                              from the log4xtensa::BinaryLogger::getUniqueEventNameID()
 *                              method.
 *
 */
XTSC_API
void xtsc_log_elementary_event(log4xtensa::BinaryLogger&      logger,
                               log4xtensa::LogLevel           log_level,
                               u32                            event_name_id);
                         



} // namespace xtsc



#endif  // _XTSC_LOGGING_H_
