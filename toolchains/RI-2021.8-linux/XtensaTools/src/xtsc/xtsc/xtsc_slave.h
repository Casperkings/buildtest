#ifndef _XTSC_SLAVE_H_
#define _XTSC_SLAVE_H_

// Copyright (c) 2005-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */


#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_request_if.h>
#include <xtsc/xtsc_respond_if.h>
#include <xtsc/xtsc_response.h>
#include <xtsc/xtsc_fast_access.h>
#include <string>
#include <vector>
#include <deque>
#include <fstream>



namespace xtsc {
class xtsc_core;
}



namespace xtsc_component {

class xtsc_arbiter;
class xtsc_master;
class xtsc_memory_trace;
class xtsc_router;
class xtsc_pin2tlm_memory_transactor;



/**
 * Constructor parameters for a xtsc_slave object.
 *
 *  \verbatim
   Name                 Type    Description
   ------------------   ----    --------------------------------------------------------
  
   "format"             u32     If 1, the default, support line format #1 in the script
                                file (see "script_file" parameter).  If 2, line format
                                #2 is supported.  Generally, line format #2 is identical
                                to format #1 except for the addition of the coh field
                                which is now unused.
                                The line format specified by this parameter can be
                                changed using the "FORMAT 1|2" command in the script
                                file itself.
                                Default 1.

   "script_file"        char*   The file to read the responses from.  Each response
                                takes one line in the file; however, the file can
                                contain a few other lines besides response lines.
                                Each time an nb_request() call is made to xtsc_slave,
                                the script file is processed line by line up to and
                                including the next response line (a response line is a
                                line that starts with a delay) or to an IGNORE line.
                                The supported line formats are:

            // All formats
            REPEAT rcount [rdelay]
            IGNORE icount
            NOTE <message>
            INFO <message>
            FORMAT 1|2

            // Format 1
            delay status size route_id id priority pc last b0 b1 ... bN [CONT]

            // Format 2
            delay status size route_id id priority pc last coh b0 b1 ... bN [CONT]
                                
                                1.  Each field after delay corresponds to a data member
                                    in the xtsc::xtsc_response object.  
                                2.  The coh field (Format 2) is now unused and can be
                                    set to 0.
                                3.  Integers can appear in decimal or hexadecimal (using
                                    '0x' prefix) format.
                                4.  N (in bN) is equal to (size - 1).
                                5.  If CONT was not specified for the previous response,
                                    then delay starts upon receipt of a request.  If
                                    CONT was specified for the previous response, then
                                    delay starts upon completion of the previous
                                    response.  delay can be 0 (to mean 1 delta cycle),
                                    or "now" to mean no delta cycle delay (i.e.
                                    immediate notification), or a positive integer or
                                    floating point number to mean that many clock
                                    periods.
                                6.  status = OKAY|NACC|ADDR|DATA|BOTH.
                                7.  Any of route_id, id, priority, and pc can be an
                                    asterisk (*) to indicate that the value should be
                                    obtained from the xtsc::xtsc_request object used to
                                    create this xtsc::xtsc_response.
                                8.  last can be 1 to mean last transfer or 0 to mean
                                    NOT last transfer.
                                9.  If CONT appears at the end of a response line, then the
                                    next response line (and all intervening non-response
                                    lines) will be processed without waiting for a
                                    request.  This can be used for BLOCK_READ responses
                                    or to generate a response without a request.
                               10.  A line starting with the word REPEAT redefines the
                                    "repeat_count" and "repeat_delay" parameter values.
                                    rcount can be any non-negative integer (including 0)
                                    to indicate that a rejected response should be
                                    repeated rcount times or it can be "forever" or
                                    0xFFFFFFFF to indicate that a response should be
                                    repeated until it is accepted.  rdelay can be an
                                    integer or floating point number.  It will be
                                    multiplied by the clock period (as determined by the
                                    "clock_period" parameter value) to determine the
                                    delay before a response is repeated.
                               11.  An IGNORE icount lines causes the next icount
                                    requests to be ignored; that is, no response is
                                    sent.  icount must be greater then 0.
                               12.  If a line starts with the word NOTE or INFO then the
                                    entire line to be logged at NOTE_LOG_LEVEL or
                                    INFO_LOG_LEVEL, respectively, and the next line of
                                    the file will be read to get the response.
                               13.  Words are case insensitive.
                               14.  Comments, extra whitespace, blank lines, and lines
                                    between "#if 0" and "#endif" are ignored.  
                                    See xtsc_script_file.

                                If "script_file" is null or empty then each 
                                response will be a standard response.  A standard
                                response has the following values:

                            delay status size route_id id priority pc last coh
                            1     okay   0    *        *  *        *  1    0


   "wraparound"         bool    Specifies what should happen when the end of file (EOF)
                                is reached on "script_file".  When EOF is reached and
                                "wraparound" is true, "script_file" will be reset to
                                the beginning of file to get the next response.  When
                                EOF is reached and "wraparound" is false, a standard
                                response will be used for the current request and all
                                subsequent requests.
                                Default = true.

   "repeat_count"       u32     Specifies how many times to repeat a rejected response
                                (this only applies to responses whose delay field is not
                                "now").  A value of 0xFFFFFFFF means to repeat the
                                response until it is accepted.
                                Default = 0 (do not repeat rejected responses).

   "repeat_delay"       u32     This specifies how long to wait before sending a
                                repeated response (this applies only if "repeat_count"
                                is not 0).  "repeat_delay" is expressed in terms of the
                                SystemC time resolution (from sc_get_time_resolution()).
                                A value of 0xFFFFFFFF means to use the value determined
                                by "clock_period".  A value of 0 means one delta cycle.
                                Default = 0xFFFFFFFF (i.e. use the value determined by
                                "clock_period").

   "clock_period"       u32     This is the length of this slave's clock period
                                expressed in terms of the SystemC time resolution
                                (from sc_get_time_resolution()).  A value of 
                                0xFFFFFFFF means to use the XTSC system clock 
                                period (from xtsc_get_system_clock_period()).
                                A value of 0 means one delta cycle.
                                Default = 0xFFFFFFFF (i.e. use the system clock 
                                period).

    \endverbatim
 *
 * @see xtsc_slave
 * @see xtsc::xtsc_parms
 * @see xtsc::xtsc_response
 */
class XTSC_COMP_API xtsc_slave_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_slave_parms object.
   *
   * @param     script_file     The file name to read the responses from.
   *
   * @param     wraparound      Indicates if script_file should wraparound to the
   *                            beginning of the file after the end of file is
   *                            reached.
   */
  xtsc_slave_parms(const char *script_file = NULL, bool wraparound = true) {
    add("script_file",          script_file);
    add("wraparound",           wraparound);
    add("repeat_count",         0);
    add("repeat_delay",         0xFFFFFFFF);
    add("clock_period",         0xFFFFFFFF);
    add("format",               1);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_slave_parms"; }

};





/**
 * A scripted memory interface slave.
 *
 * Example XTSC device implementing a slave on a memory interface that reads an input
 * file to determine what responses to send to a master device.  This provides a simple
 * means to test an xtsc_master, an xtsc_core, or even a system comprised of
 * xtsc_arbiter, xtsc_core, xtsc_master, xtsc_mmio, and xtsc_router objects.
 *
 * Here is a block diagram of an xtsc_slave as it is used in the slave example:
 * @image html  xtsc_slave.jpg
 * @image latex xtsc_slave.eps "xtsc_slave Example" width=10cm
 *
 * @see xtsc_slave_parms
 * @see xtsc::xtsc_request_if
 * @see xtsc::xtsc_respond_if
 */
class XTSC_COMP_API xtsc_slave : public sc_core::sc_module, public xtsc::xtsc_module, virtual public xtsc::xtsc_request_if {
public:

  /// From memory interface master to us
  sc_core::sc_export<xtsc::xtsc_request_if>     m_request_export;

  /// From us to memory interface master
  sc_core::sc_port  <xtsc::xtsc_respond_if>     m_respond_port;

  /// This class helps keep track of a response and when it is due
  class response_info {
  public:
    /// Constructor
    response_info(xtsc::xtsc_response *p_response, bool respond_now, bool cont, const sc_core::sc_time& delay) :
      m_p_response      (p_response),
      m_respond_now     (respond_now),
      m_cont            (cont),
      m_delay           (delay)
    {}
    ~response_info() {
      if (m_p_response) {
        delete m_p_response;
        m_p_response = 0;
      }
    }
    xtsc::xtsc_response        *m_p_response;           ///<  The xtsc_response to respond with
    bool                        m_respond_now;          ///<  The response line had delay of "now"
    bool                        m_cont;                 ///<  The response line had CONT
    sc_core::sc_time            m_delay;                ///<  How long to delay
  };


  // For SystemC
  SC_HAS_PROCESS(xtsc_slave);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_slave"; }


  /**
   * Constructor for an xtsc_slave.
   *
   * @param     module_name     Name of the xtsc_slave sc_module.
   *
   * @param     slave_parms     The remaining parameters for construction.
   *
   * @see xtsc_slave_parms
   */
  xtsc_slave(sc_core::sc_module_name module_name, const xtsc_slave_parms& slave_parms);


  // Destructor.
  ~xtsc_slave(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "slave_port"; }


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /**
   * Reset the xtsc_slave.
   */
  void reset(bool hard_reset = false);


  /**
   * Connect with an xtsc_arbiter.
   *
   * This method connects the master port pair of the specified xtsc_arbiter with the
   * slave port pair of this xtsc_slave.
   *
   * @param     arbiter          The xtsc_arbiter to connect with this xtsc_slave.
   */
  void connect(xtsc_arbiter& arbiter);


  /**
   * Connect with an xtsc_core.
   *
   * This method connects the specified memory interface master port pair of the
   * specified xtsc_core with the slave port pair of this xtsc_slave.
   *
   * @param     core            The xtsc_core to connect with this xtsc_slave.
   *
   * @param     memory_name     The name of the memory interface master port pair of the
   *                            xtsc_core to connect with this xtsc_slave.
   */
  void connect(xtsc::xtsc_core& core, const char *memory_name);


  /**
   * Connect with an xtsc_master.
   *
   * This method connects the master port pair of the specified xtsc_master with the
   * slave port pair of this xtsc_slave.
   *
   * @param     master          The xtsc_master to connect with this xtsc_slave.
   */
  void connect(xtsc_master& master);


  /**
   * Connect with an xtsc_memory_trace.
   *
   * This method connects the specified master port pair of the specified
   * xtsc_memory_trace with the slave port pair of this xtsc_slave.
   *
   * @param     memory_trace    The xtsc_memory_trace to connect with this xtsc_slave.
   *
   * @param     port_num        The xtsc_memory_trace master port pair to connect with
   *                            this xtsc_slave.
   */
  void connect(xtsc_memory_trace& memory_trace, xtsc::u32 port_num);


  /**
   * Connect with an xtsc_pin2tlm_memory_transactor.
   *
   * This method connects the specified TLM master port pair of the specified
   * xtsc_pin2tlm_memory_transactor with the slave port pair of this xtsc_slave.
   *
   * @param     pin2tlm         The xtsc_pin2tlm_memory_transactor to connect with this
   *                            xtsc_slave.
   *
   * @param     port_num        The xtsc_pin2tlm_memory_transactor master port pair to
   *                            connect with this xtsc_slave.
   */
  void connect(xtsc_pin2tlm_memory_transactor& pin2tlm, xtsc::u32 port_num);


  /**
   * Connect with an xtsc_router.
   *
   * This method connects the specified master port pair of the specified xtsc_router
   * with the slave port pair of this xtsc_slave.
   *
   * @param     router          The xtsc_router to connect with this xtsc_slave.
   *
   * @param     port_num        The xtsc_router master port pair to connect with this
   *                            xtsc_slave.
   */
  void connect(xtsc_router& router, xtsc::u32 port_num);


  /// Send delayed (not immediate) responses to the memory interface master
  virtual void delayed_response_thread(void);


  /// Receive peeks from the memory interface master
  virtual void nb_peek(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer);


  /// Receive pokes from the memory interface master
  virtual void nb_poke(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer);


  /// Receive coherent peeks from the memory interface master
  virtual bool nb_peek_coherent(xtsc::xtsc_address      virtual_address8,
                                xtsc::xtsc_address      physical_address8,
                                xtsc::u32               size8,
                                xtsc::u8               *buffer);


  /// Receive coherent pokes from the memory interface master
  virtual bool nb_poke_coherent(xtsc::xtsc_address      virtual_address8,
                                xtsc::xtsc_address      physical_address8,
                                xtsc::u32               size8,
                                const xtsc::u8         *buffer);


  /// Receive fast access requests from the memory interface master
  virtual bool nb_fast_access(xtsc::xtsc_fast_access_request &request);


  /// Receive requests from the memory interface master
  void nb_request(const xtsc::xtsc_request& request);


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


protected:


  std::string                           m_script_file;                  ///<  The name of the response script file
  bool                                  m_wraparound;                   ///<  True if m_script_file should be reread after reaching EOF
  xtsc::xtsc_script_file               *m_response_stream;              ///<  The response script file object
  xtsc::u32                             m_format;                       ///<  The format in effect.  See "format" parameter.
  std::string                           m_line;                         ///<  Line from m_script_file
  xtsc::u32                             m_line_count;                   ///<  Line number in m_script_file of m_line
  std::vector<std::string>              m_words;                        ///<  The vector of words from current line of m_script_file
  std::deque<response_info*>            m_responses;                    ///<  The deque of pending responses
  sc_core::sc_event                     m_delayed_response_event;       ///<  Event to notify delayed_response_thread
  xtsc::u32                             m_repeat_count;                 ///<  How many times to repeat a rejected response
  xtsc::u32                             m_ignore_count;                 ///<  How many times to ignore a request
  sc_core::sc_time                      m_repeat_delay_time;            ///<  How long to wait between repeated responses
  sc_core::sc_time                      m_clock_period;                 ///<  This modules clock period
  xtsc::u32                             m_transfer_num;                 ///<  For xtsc_response::set_transfer_number()
  log4xtensa::TextLogger&               m_text;                         ///<  Text logger

  std::vector<sc_core::sc_process_handle>
                                        m_process_handles;              ///<  For reset 

  /// Send and log a response (set non_blocking true if called from nb_request)
  void send_response(xtsc::xtsc_response &response, bool non_blocking);

  /// Create a new response_info object
  response_info *new_response_info(const xtsc::xtsc_request& request);

  /// Delete a response_info object
  void delete_response_info(response_info*& p_response_info);

  /// Get the next vector of words which define an xtsc::xtsc_response test vector
  int get_words();

  /// Extract a u32 value (named argument_name) from the word at m_words[index]
  xtsc::u32 get_u32(xtsc::u32 index, const std::string& argument_name);

  /// Extract a double value (named argument_name) from the word at m_words[index]
  double get_double(xtsc::u32 index, const std::string& argument_name);

  /// Set the buffer of this response using size8 words starting at m_words[index]
  void set_buffer(xtsc::xtsc_response& response, xtsc::u32 index, xtsc::u32 size8);
};



}  // namespace xtsc_component



#endif  // _XTSC_SLAVE_H_
