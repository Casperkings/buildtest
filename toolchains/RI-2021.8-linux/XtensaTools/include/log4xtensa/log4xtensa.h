// Copyright (c) 2005 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

#ifndef _LOG4XTENSA_H_
#define _LOG4XTENSA_H_



#include <string>
#include <sstream>
#include <ostream>
#include <istream>
#include <cstdarg>
#include <log4xtensa/logger.h>
#include <log4xtensa/bostringstream.h>




/**
 * Macro for generating a binary logging event.
 * @param       binaryLogger    The BinaryLogger object to log the event with.
 *
 * @param       logLevel        The LogLevel of the event.
 *
 * @param       recordType      The binary record type of the event.
 *
 * @param       payload         The payload of the binary logging event.         
 */
#define LOG4XTENSA_BIN(binaryLogger, logLevel, recordType, payload) { \
  if (binaryLogger.isEnabledFor(logLevel)) { \
    log4xtensa::bostringstream _boss; \
    _boss << payload; \
    binaryLogger.forcedBLog(logLevel, recordType, _boss); \
  } \
}




/**
 * log4xtensa namespace.
 *
 * The log4xtensa namespace includes a modified version of the log4cplus library
 * (http://sourceforge.net/projects/log4cplus) which has been enhanced with a 
 * BinaryLogger class.
 */
namespace log4xtensa {

class BinaryLogger;
class BinaryLoggerFactory;

/// Method to dump a list of all text loggers to an ostream object.
LOG4XTENSA_EXPORT void dump_text_loggers(std::ostream& os = std::cout);

/// Method to dump a list of all binary loggers to an ostream object.
LOG4XTENSA_EXPORT void dump_binary_loggers(std::ostream& os = std::cout);

/** 
 * The sizer token can be used in a bostringstream preceding an integer (of any type).
 * Its effect is to cause the integer to set the size in bytes of data pointed to 
 * by subsequent pointer(s).  The integer value itself is NOT added to the bostringstream
 * object.
 *
 * Example:  To add 8 bytes from the data array to a bostringstream object named boss:
 *  \verbatim
  u32 data[] = { 0x01234567, 0x89abcdef };
  bostringstream boss;
  boss << sizer << 8 << data;
    \endverbatim
 *
 * @see bostringstream
 */
const Sizer sizer = *new Sizer();





/**
 * Class used to generate binary logging events (messages).
 * When using a BinaryLogger, you must first configure binary logging by
 * calling one of the PropertyConfigurator constructors with a Hierarchy
 * that has been set up to use a BinaryLoggerFactory.  The BinaryLogger::
 * getDefaultHierarchy() method provides you such a Hierarchy.  For example:
 *  \verbatim
  PropertyConfigurator binaryConfig("binary.cnf", BinaryLogger::getDefaultHierarchy());
  binaryConfig.configure();
    \endverbatim
 * 
 * All BinaryLogger events (records) have a 12 byte header followed by a payload
 * whose length depends upon the record type.
 *
   *  \verbatim
    Bytes  Description
    -----  ---------------------------------------------------
    4      Total bytes in this record - 4 = 8 + P.
    4      Record type.
    4      Logger ID.
    P      Payload (format determined by record type)
      \endverbatim
 *
 */
class LOG4XTENSA_EXPORT BinaryLogger : public Logger {
public:

  typedef unsigned char           u8;
  typedef   signed char           i8;
  typedef unsigned short          u16;
  typedef   signed short          i16;
  typedef unsigned int            u32;
  typedef   signed int            i32;
#ifdef _MSC_EXTENSIONS
  typedef unsigned __int64        u64;
  typedef   signed __int64        i64;
#else
  typedef unsigned long long      u64;
  typedef   signed long long      i64;
#endif

  /**
   * Method to generate a binary logging event if the passed LogLevel
   * is greater then or equal to this logger's assigned LogLevel.
   *
   * @param     ll              The LogLevel of this logging event.
   *
   * @param     recordType      Specifies the format of the payload.
   *                            This value will be included in
   *                            the logging event.  Value is by
   *                            arrangement with post-processing
   *                            tools.
   *
   * @param     numBytes        The number of bytes in the payload
   *                            field to be copied to the logging
   *                            event.
   *
   * @param     payload         A pointer to the payload to be copied
   *                            to the logging event.
   */
  void blog(LogLevel ll, u32 recordType, size_t numBytes, void *payload);

  /**
   * Method to generate a binary logging event if the passed LogLevel
   * is greater then or equal to this logger's assigned LogLevel.
   *
   * @param     ll              The LogLevel of this logging event.
   *
   * @param     recordType      Specifies the format of the payload.
   *                            This value will be included in
   *                            the logging event.  Value is by
   *                            arrangement with post-processing
   *                            tools.
   *
   * @param     format          A format string specifying the
   *                            contents of the variadic portion
   *                            of the function call (...).  Format
   *                            characters and their meaning are:
   * \verbatim
            Character   Variadic Parameter(s) 
            ---------   ---------------------
            b|B         byte (char)
            h|H         halfword (short)
            w|W         word (int)
            d|D         doubleword (long long)
            x|X         2 variadic parameters specifying a
                        variable length buffer:
                        1st) int => Buffer size = N
                        2nd) u8* => Buffer
     \endverbatim
   *
   * @param     ...             A variable number of parameters as 
   *                            specified in format to be written to 
   *                            the logging event.
   *
   * Note: The "x" (or "X") format character causes N (the first variadic
   *       parameter) bytes from the Buffer (the second variadic parameter)
   *       to be added to the binary logging event.  The value of N itself 
   *       is NOT added.  If the size needs to also be added to the logging 
   *       event then the size needs to appear twice as a variadic parameter
   *       and an additional character would need to be added to the format
   *       parameter to account for it.  For example to add a 2 byte size
   *       field containing the value 8 followed by 8 bytes from an array:
   * \verbatim
           u8 array[8] = { 1, 2, 3, 4, 5, 6, 7, 8};
           myBinaryLogger.log(ll, recordType, "HX",
                              (u16) sizeof(array),    // "H"
                              sizeof(array), array    // "X"
                             );
     \endverbatim
   */
  void blog(LogLevel ll, u32 recordType, const char *format, ...);

  /**
   * Method to generate a binary logging event if the passed LogLevel
   * is greater then or equal to this logger's assigned LogLevel.
   *
   * @param     ll              The LogLevel of this logging event.
   *
   * @param     recordType      Specifies the format of the payload.
   *                            This value will be included in
   *                            the logging event.  Value is by
   *                            arrangement with post-processing
   *                            tools.
   *
   * @param     payload         A bostringstream object containing
   *                            the bytes to be written to the logging
   *                            event.  A bostringstream object can
   *                            be written to like a ostringstream using
   *                            the << operator; however the data is
   *                            written to the object in binary.  
   *                            @see bostringstream
   */
  void blog(LogLevel ll, u32 recordType, const bostringstream& payload);


  /// Same as corresponding log() method except LogLevel of logger is not checked.
  void forcedBLog(LogLevel ll, u32 recordType, size_t numBytes, void *payload);

  /// Same as corresponding log() method except LogLevel of logger is not checked.
  void forcedBLog(LogLevel ll, u32 recordType, const char *format, ...);

  /// Same as corresponding log() method except LogLevel of logger is not checked.
  void forcedBLog(LogLevel ll, u32 recordType, const bostringstream& payload);

  /// Helper method to generate log event from va_list
  void doVarArg(LogLevel ll, u32 recordType, const char *format, va_list& args);



  // Static Methods


  /**
   * Returns true if the named logger exists (in the default 
   * BinaryLogger hierarchy).
   *
   * @param     name            The name of the logger to search for.
   */
  static bool exists(const log4xtensa::tstring& name);

  /*
   * Returns all the currently defined loggers in the default
   * BinaryLogger hierarchy.  The root logger is NOT included 
   * in the returned list.     
   */
  static LoggerList getCurrentLoggers();

  /**
   * Return the default BinaryLogger Hierarchy instance.
   */
  static Hierarchy& getDefaultHierarchy();

  /**
   * Method to get a BinaryLogger object.
   *
   * @param     name    Hierarchical name of the BinaryLogger.
   */
  static BinaryLogger& getInstance(const tstring& name);


  /**
   * Return the root of the default logger hierrachy.
   *
   * The root logger is always instantiated and available. It's
   * name is "root".
   *
   * Nevertheless, calling BinaryLogger.getInstance("root") does 
   * not retrieve the root logger but a logger just under root 
   * named "root" (i.e. root.root).
   */
  static BinaryLogger& getRoot();

  /**
   * Method to get a unique ID to identify an event name.
   */
  static u32 getUniqueEventNameID();

  /**
   * Calling this method will safely close and remove all
   * appenders in all the loggers including root contained in the
   * default BinaryLogger hierarchy.
   * 
   * Some appenders such as SocketAppender need to be closed before the
   * application exits. Otherwise, pending logging events might be
   * lost.
   *
   * The shutdown method is careful to close nested
   * appenders before closing regular appenders. This is allows
   * configurations where a regular appender is attached to a logger
   * and again to a nested appender.  
   */
  static void shutdown();



private:

  // Copy Ctor
  BinaryLogger(const BinaryLogger& rhs);
  BinaryLogger& operator=(const BinaryLogger& rhs);

  BinaryLogger(spi::LoggerImpl *ptr);
  BinaryLogger(const spi::SharedLoggerImplPtr& val);
  friend class BinaryLoggerFactory;
  static u32 nextUniqueEventNameID;

};


/**
 * Class used to create a BinaryLogger
 */
class LOG4XTENSA_EXPORT BinaryLoggerFactory : public spi::LoggerFactory {
public:
  BinaryLogger& makeNewLoggerInstance(const tstring& name, Hierarchy& h);
};


/// TextLogger is a typedef for Logger 
typedef Logger TextLogger;









/// typedef to indicate a dummy variable used to anchor additional doc++/doxygen documentation comments.
typedef int Document;

/**
 * Byte ordering in the file is Little Endian.  For non-byte integer values (u16, u32,
 * and u64), the least significant byte is nearer the beginning of the file then the 
 * most significant byte.  For a u8 data buffer of size N bytes, buffer[0] is nearest 
 * the beginning of the file and buffer[N-1] is farthest from the beginning of the file.
 *
 * "Event name ID" is a unique number that is associated with the actual event name in
 * a binary record of type EVENT_NAME_RECORD.  When using log4xtensa, a simulation-run 
 * unique number can be obtained by calling BinaryLogger::getUniqueEventNameID().
 *
 */
extern Document Binary_format_general_information;


/**
 * Binary file record format for recording a logger's name.
 *  \verbatim
  Bytes  Description
  -----  -------------------------------------------------------
  4      Total bytes in this record - 4 = 8 + N.
  4      Record type = 0xC0010000 (LOGGER_NAME_RECORD).
  4      Logger ID.
  N      Logger's name buffer (ASCII data, non-0-terminated).
    \endverbatim
 */
extern Document Binary_format_logger_name_record;



/**
 * Binary file record format for recording a TIE port's name.
 * This record is used for import wires, export states, input 
 * queues, and output queues.
 *  \verbatim
  Bytes  Description
  -----  -------------------------------------------------------
  4      Total bytes in this record - 4 = 13 + N.
  4      Record type = 0xC0020000 (TIE_PORT_NAME_RECORD).
  4      Logger ID.
  4      TIE port number.
  1      TIE port type:
           0x0 - OUTPUT_QUEUE
           0x1 - INPUT_QUEUE
           0x2 - EXPORT_STATE
           0x3 - IMPORT_WIRE
           0x4 - TIE_LOOKUP
  N      TIE port name buffer (ASCII data, non-0-terminated).
    \endverbatim
 */
extern Document Binary_format_tie_port_name_record;



/**
 * Binary file record format for recording a memory port's name.
 *  \verbatim
  Bytes  Description
  -----  -------------------------------------------------------
  4      Total bytes in this record - 4 = 12 + N.
  4      Record type = 0xC0040000 (MEMORY_PORT_NAME_RECORD).
  4      Logger ID.
  4      Memory port number.
  N      Memory port name buffer (ASCII data, non-0-terminated).
    \endverbatim
 */
extern Document Binary_format_memory_port_name_record;



/**
 * Binary file record format for recording a user-defined event
 * name.
 *  \verbatim
  Bytes  Description
  -----  -------------------------------------------------------
  4      Total bytes in this record - 4 = 12 + N.
  4      Record type = 0xC0080000 (EVENT_NAME_RECORD).
  4      Logger ID.
  4      Event name ID.
  N      Event name buffer (ASCII data, non-0-terminated).
    \endverbatim
 */
extern Document Binary_format_event_name_record;



/**
 * Binary file record format for recording the time resolution.
 *  \verbatim
  Bytes  Description
  -----  -------------------------------------------------------
  4      Total bytes in this record - 4 = 17.
  4      Record type = 0xC0100000 (TIME_RESOLUTION_RECORD).
  4      Logger ID.
  1      Simulation time resolution code = C.  C determines the
         units for the simulation time fields using the formula
           time units = 10^(C-15) seconds
         Valid values for C are 0 through 15 which correspond
         to all power-of-ten time units between 1 femtosecond 
         (10^-15 seconds) and 1 second.  The default value for
         C (i.e. if this record type has not yet appeared in
         the log file) is 6 which corresponds to 1 nanosecond:
           time units = 10^(6-15) sec = 10^(-9) sec = 1 NS.
  8      Nominal system clock period.
    \endverbatim
 */
extern Document Binary_format_time_resolution_record;



/**
 * Binary file record format for a memory request event.
 *  \verbatim
  Bytes  Description
  -----  ---------------------------------------------------
  4      Total bytes in this record - 4 = 43 + N.
  4      Record type = 0x81000000 (REQUEST_EVENT).
  4      Logger ID.
  8      Simulation time.
  8      Transaction tag.
  4      Program counter (PC) or 0xFFFFFFFF if unknown.
  4      Port number an INCOMING request is received on or 
         an OUTGOING request is sent out on.
  1      Request type:
           0x00 - READ
           0x10 - BLOCK_READ
           0x50 - RCW
           0x80 - WRITE
           0x90 - BLOCK_WRITE
  1      1=INCOMING, 0=OUTGOING.
  1      Last data flag (1=last data, 0=not last data).
  2      Number of transfers for BLOCK_READ or BLOCK_WRITE
         (always 1 for READ and WRITE; always 2 for RCW).
  2      Byte enables.
  4      Byte address.
  N      Buffer.
    \endverbatim
 */
extern Document Binary_format_memory_request_event;



/**
 * Binary file record format for a memory response event.
 *  \verbatim
  Bytes  Description
  -----  ---------------------------------------------------
  4      Total bytes in this record - 4 = 35 + N.
  4      Record type = 0x81100000 (RESPONSE_EVENT).
  4      Logger ID.
  8      Simulation time.
  8      Transaction tag.
  4      Program counter (PC) or 0xFFFFFFFF if unknown.
  4      Port number an INCOMING response is received on or 
         an OUTGOING response is sent out on.
  1      Response status:
           0x0  - Okay
           0x1  - Address error
           0x2  - Data error
           0x3  - Address or data error
           0x4  - NACC
  1      1=INCOMING, 0=OUTGOING.
  1      Last data flag (1=last data, 0=not last data).
  N      Buffer.
    \endverbatim
 */
extern Document Binary_format_memory_response_event;



/**
 * Binary file record format for a TIE port (import wire or 
 * export state) event.
 *  \verbatim
  Bytes  Description
  -----  ---------------------------------------------------
  4      Total bytes in this record - 4 = 27 + N.
  4      Record type = 0x82000000 (TIE_PORT_EVENT).
  4      Logger ID.
  8      Simulation time.
  4      TIE port number.
  1      Transaction type 1=READ_WIRE, 0=WRITE_STATE.
  4      Program counter (PC) or 0xFFFFFFFF if unknown.
  2      Number of bits in buffer (can be 0) = B.  N=(B+7)/8.
  N      Buffer.
    \endverbatim
 *
 */
extern Document Binary_format_tie_port_event;



/**
 * Binary file record format for a queue event.
 *  \verbatim
  Bytes  Description
  -----  ---------------------------------------------------
  4      Total bytes in this record - 4 = 43 + N.
  4      Record type = 0x84000000 (QUEUE_EVENT).
  4      Logger ID.
  8      Simulation time.
  8      Transaction ticket.
  4      TIE port number or 0xFFFFFFFF if unknown.
  1      Transaction type:
           0x0  - POP
           0x1  - PUSH
           0x2  - POP_FAILED
           0x3  - PUSH_FAILED
         A POP_FAILED record may be generated due to a call
         to nb_pop that failed or due to a call to nb_can_pop
         that returns false.  A PUSH_FAILED record may be 
         generated due to a call to nb_push that failed or 
         due to a call to nb_can_push that returns false.
  4      Program counter (PC) or 0xFFFFFFFF if unknown.
  4      Number elements in queue AFTER this PUSH/POP or
         0xFFFFFFFF if unknown.
  4      Total queue capacity or 0xFFFFFFFF if unknown.
  2      Number of bits in buffer (can be 0) = B.  N=(B+7)/8.
  N      Buffer.
    \endverbatim
 *
 */
extern Document Binary_format_queue_event;



/**
 * Binary file record format for a TIE lookup event.
 *  \verbatim
  Bytes  Description
  -----  ---------------------------------------------------
  4      Total bytes in this record - 4 = 27 + N.
  4      Record type = 0x88000000 (LOOKUP_EVENT).
  4      Logger ID.
  8      Simulation time.
  4      TIE port number or 0xFFFFFFFF if unknown.
  1      Transaction type:
           0x0  - LOOKUP_KEY (Buffer holds key)
           0x1  - READY (Buffer holds: 0=not ready, 1=ready)
           0x2  - RESPONSE_VALUE (Buffer holds value)
  4      Program counter (PC) or 0xFFFFFFFF if unknown.
  2      Number of bits in buffer (can be 0) = B.  N=(B+7)/8.
  N      Buffer.
    \endverbatim
 *
 */
extern Document Binary_format_lookup_event;



/**
 * Binary file record format for a user-defined data event.
 *  \verbatim
  Bytes  Description
  -----  ---------------------------------------------------
  4      Total bytes in this record - 4 = 22 + N.
  4      Record type = 0x91000000 (DATA_EVENT).
  4      Logger ID.
  8      Simulation time.
  4      Event name ID.
  2      Number of bits in buffer (can be 0) = B.  N=(B+7)/8.
  N      Buffer.
    \endverbatim
 *
 */
extern Document Binary_format_data_event;



/**
 * Binary file record format for a user-defined string event.
 *  \verbatim
  Bytes  Description
  -----  ---------------------------------------------------
  4      Total bytes in this record - 4 = 20 + N.
  4      Record type = 0x92000000 (STRING_EVENT).
  4      Logger ID.
  8      Simulation time.
  4      Event name ID.
  N      Buffer.
    \endverbatim
 *
 */
extern Document Binary_format_string_event;



/**
 * Binary file record format for a user-defined elementary event.
 *  \verbatim
  Bytes  Description
  -----  ---------------------------------------------------
  4      Total bytes in this record - 4 = 20.
  4      Record type = 0x94000000 (ELEMENTARY_EVENT).
  4      Logger ID.
  8      Simulation time.
  4      Event name ID.
    \endverbatim
 *
 */
extern Document Binary_format_elementary_event;



/// enum for identifying the logging event record type.
enum LogEventType {

  /// Request event (0x81000000)
  REQUEST_EVENT               = 0x81000000,
  /// Response event (0x81100000)
  RESPONSE_EVENT              = 0x81100000,

  /// TIE port event (0x82000000)
  TIE_PORT_EVENT              = 0x82000000,

  /// Queue event (0x84000000)
  QUEUE_EVENT                 = 0x84000000,

  /// Lookup event (0x88000000)
  LOOKUP_EVENT                = 0x88000000,

  /// User defined data event (0x91000000)
  DATA_EVENT                  = 0x91000000,

  /// User defined string event (0x92000000)
  STRING_EVENT                = 0x92000000,

  /// User defined elementary event (0x94000000)
  ELEMENTARY_EVENT            = 0x94000000,

  /// Logger's name record (0xC0010000)
  LOGGER_NAME_RECORD          = 0xC0010000,

  /// TIE port name record (0xC0020000)
  TIE_PORT_NAME_RECORD        = 0xC0020000,

  /// Memory port name record (0xC0040000)
  MEMORY_PORT_NAME_RECORD     = 0xC0040000,

  /// Event's name record (0xC0080000)
  EVENT_NAME_RECORD           = 0xC0080000,

  /// Time resolution record (0xC0100000)
  TIME_RESOLUTION_RECORD      = 0xC0100000,

};


/// enum for identifying the memory transaction type.
enum MemoryTransaction {
  /// PIF read transaction (0x00)
  READ          = 0x00,
  /// PIF block read transaction (0x10)
  BLOCK_READ    = 0x10,
  /// PIF RCW transaction (0x50)
  RCW           = 0x50,
  /// PIF write transaction (0x80)
  WRITE         = 0x80,
  /// PIF block write transaction (0x90)
  BLOCK_WRITE   = 0x90
};


/// enum for identifying whether memory transaction is a request or a responsis a request or a response.
enum RequestResponse {
  /// Request transaction (0x0)
  REQUEST       = 0x0,
  /// Response transaction (0x1)
  RESPONSE      = 0x1
};


/// enum to indicate whether or not this is the last transfer of the current transaction
enum LastData {
  /// Indicates this transfer is not the last for the current transaction (0)
  NO            = 0,
  /// Indicates this transfer is the last for the current transaction (1)
  YES           = 1,
};


/// enum to indicate if this transaction is now entering or now leaving the device
enum PortDirection {
  /// Indicates the transfer is being sent out of the logging device (0)
  OUTGOING      = 0,
  /// Indicates the transfer is being received by the logging device (1)
  INCOMING      = 1
};


/// enum to indicate the type of queue transaction and whether it succeeded
enum QueueTransaction {
  /// Indicates a successful queue pop transaction (0x0)
  POP           = 0x0,
  /// Indicates a successful queue push transaction (0x1)
  PUSH          = 0x1,
  /// Indicates a failed queue pop attempt (0x2)
  POP_FAILED    = 0x2,
  /// Indicates a failed queue push attempt (0x3)
  PUSH_FAILED   = 0x3
};


/// enum to indicate the type of lookup transaction
enum LookupTransaction {
  /// Indicates a lookup key (TIE_xxx_out signal) (0x0)
  LOOKUP_KEY    = 0x0,
  /// Indicates a poll of the TIE_xxx_rdy signal (0x1)
  READY         = 0x1,
  /// Indicates a response value (TIE_xxx_in signal) (0x2)
  RESPONSE_VALUE= 0x2
};


/// enum to indicate the type of TIE port
enum TiePort {
  /// Indicates the TIE port is an ouput queue (0x0)
  OUTPUT_QUEUE  = 0x0,
  /// Indicates the TIE port is an input queue (0x1)
  INPUT_QUEUE   = 0x1,
  /// Indicates the TIE port is an export state (0x2)
  EXPORT_STATE  = 0x2,
  /// Indicates the TIE port is an import wire (0x3)
  IMPORT_WIRE   = 0x3,
  /// Indicates the TIE port is a TIE lookup (0x4)
  TIE_LOOKUP    = 0x4,
};


/// enum to indicate the type of TIE port event
enum TiePortTransaction {
  /// Indicates a TIE port write state event (0x0)
  WRITE_STATE   = 0x0,
  /// Indicates a TIE port read wire event (0x1)
  READ_WIRE     = 0x1
};


/// enum to indicate program counter is unknown or not-applicable 
enum {
  /// Indicates device does not know/have a PC (0xFFFFFFFF)
  UNKNOWN_PC    = 0xFFFFFFFF
};


/// enum to indicate value is unknown or not-applicable 
enum {
  /// Unknown value (0xFFFFFFFF)
  UNKNOWN       = 0xFFFFFFFF
};




} // namespace log4xtensa







#endif  // _LOG4XTENSA_H_
