#ifndef _XTSC_EXT_REGFILE_H_
#define _XTSC_EXT_REGFILE_H_

// Copyright (c) 2006-2012 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

/**
 * @file 
 */


#include <xtsc/xtsc.h>
#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_ext_regfile_if.h>
#include <string>
#include <vector>
#include <map>
#include <deque>




namespace xtsc {
class xtsc_core;
}


namespace xtsc_component {


/**
 * Constructor parameters for a xtsc_ext_regfile object.
 *
 * This class contains the constructor parameters for a xtsc_ext_regfile object.
 *  \verbatim
   Name                 Type    Description
   ------------------   ----    -------------------------------------------------------
  
   "address_bit_width"  u32     The width of the TIE address signal in bits.

   "data_bit_width"     u32     The width of the data in bits.

   "clock_period"       u32     This is the length of this module's clock period
                                expressed in terms of the SystemC time resolution
                                (from sc_get_time_resolution()).  A value of 
                                0xFFFFFFFF means to use the XTSC system clock 
                                period (from xtsc_get_system_clock_period()).
                                Default = 0xFFFFFFFF (i.e. use the system clock 
                                period).

   "posedge_offset"     u32     This specifies the time at which the first posedge of
                                this device's clock conceptually occurs.  It is
                                expressed in units of the SystemC time resolution and
                                the value implied by it must be strictly less than the
                                value implied by the "clock_period" parameter.  A value
                                of 0xFFFFFFFF means to use the same posedge offset as
                                the system clock (from
                                xtsc_get_system_clock_posedge_offset()).
                                Default = 0xFFFFFFFF.

   "vcd_handle"         void*   Pointer to SystemC VCD object (sc_trace_file *) or 0 if
                                tracing is not desired.
                                Default = 0 (no tracing).

    \endverbatim
 *
 * @see xtsc_ext_regfile
 * @see xtsc::xtsc_parms
 */
class XTSC_COMP_API xtsc_ext_regfile_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_ext_regfile_parms object.
   *
   * @param     address_bit_width       The width of the request address in bits.
   *
   * @param     data_bit_width          The width of the response data in bits.
   *
   */
  xtsc_ext_regfile_parms(xtsc::u32   address_bit_width       = 0,
                         xtsc::u32   data_bit_width          = 0)
  {
    init(address_bit_width, data_bit_width);
  }


  /**
   * Constructor for an xtsc_ext_regfile_parms object based upon an xtsc_core object and a
   * named TIE ext_regfile. 
   *
   * This constructor will determine address_bit_width, data_bit_width, and clock_period by 
   * querying the core object.  If desired, after the xtsc_ext_regfile_parms object is constructed,
   * its data members can be changed using the appropriate xtsc::xtsc_parms::set() method before 
   * passing it to the xtsc_ext_regfile constructor.
   *
   * @param     core                    A reference to the xtsc_core object upon which
   *                                    to base the xtsc_ext_regfile_parms.
   *
   * @param     ext_regfile_name        The name of the TIE ext_regfile as it appears in the
   *                                    user's TIE code after the ext_regfile keyword.
   *
   */
  xtsc_ext_regfile_parms(const xtsc::xtsc_core&      core, 
                         const char                 *ext_regfile_name);


  /**
   * Do initialization common to both constructors.
   */
  void init(xtsc::u32   address_bit_width,
            xtsc::u32   data_bit_width)
  {
    add("address_bit_width",    address_bit_width);
    add("data_bit_width",       data_bit_width);
    add("clock_period",         0xFFFFFFFF);
    add("posedge_offset",       0xFFFFFFFF);
    add("vcd_handle",           (void*) NULL); 
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_ext_regfile_parms"; }

};





/**
 * An TIE ext_regfile implementation that connects using TLM-level ports.
 *
 * Example XTSC module implementing the xtsc::xtsc_ext_regfile_if interface.
 *
 * Here is a block diagram of the system used in the xtsc_ext_regfile example (memory not
 * shown):
 * @image html  Example_xtsc_ext_regfile.jpg
 * @image latex Example_xtsc_ext_regfile.eps "xtsc_ext_regfile Example" width=10cm
 *
 * Here is the code to connect the system using the xtsc::xtsc_connect() method:
 * \verbatim
    xtsc_connect(core0, "pif", "", core0_pif);
    xtsc_connect(core0, "erf", "", erf);
   \endverbatim
 *
 * And here is the code to connect the system using manual SystemC port binding:
 * \verbatim
    core0.get_request_port("pif")(*core0_pif.m_request_exports[0]);
    (*core0_pif.m_respond_ports[0])(core0.get_respond_export("pif"));
    core0.get_ext_regfile("erf")(erf.m_ext_regfile);
   \endverbatim
 *
 * @see xtsc_ext_regfile_parms
 * @see xtsc::xtsc_ext_regfile_if
 * @see xtsc::xtsc_core::How_to_do_port_binding
 */
class XTSC_COMP_API xtsc_ext_regfile : public sc_core::sc_module, public xtsc::xtsc_module, public xtsc::xtsc_command_handler_interface {
public:

  sc_core::sc_export<xtsc::xtsc_ext_regfile_if>      m_ext_regfile;       ///<  Driver binds to this


  // SystemC needs this.
  SC_HAS_PROCESS(xtsc_ext_regfile);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_ext_regfile"; }


  /**
   * Constructor for an xtsc_ext_regfile.
   * @param     module_name     Name of the xtsc_ext_regfile sc_module.
   * @param     ext_regfile_parms    The remaining parameters for construction.
   * @see xtsc_ext_regfile_parms
   */
  xtsc_ext_regfile(sc_core::sc_module_name module_name, const xtsc_ext_regfile_parms& ext_regfile_parms);


  // Destructor.
  ~xtsc_ext_regfile(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "ext_regfile"; }


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /**
   * Reset the xtsc_ext_regfile.
   */
  void reset(bool hard_reset = false);


  /**
   * Method to change the clock period.
   *
   * @param     clock_period_factor     Specifies the new length of this device's clock
   *                                    period expressed in terms of the SystemC time
   *                                    resolution (from sc_get_time_resolution()).
   */
  void change_clock_period(xtsc::u32 clock_period_factor);


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        change_clock_period <ClockPeriodFactor>
          Call xtsc_ext_regfile::change_clock_period(<ClockPeriodFactor>).  Return previous
          <ClockPeriodFactor> for this ext_regfile.

        dump
          Return the os buffer from calling xtsc_ext_regfile::dump(os).

        peek <Address>
          Call xtsc_ext_regfile::peek(<Address>).

        poke <Address> <Value>
          Return xtsc_ext_regfile::poke(<Address>, <Value>).

        reset [<Hard>]
          Call xtsc_ext_regfile::reset(<Hard>).  Where <Hard> is 0|1 (default 0).
      \endverbatim
   */
  void execute(const std::string&               cmd_line,
               const std::vector<std::string>&  words,
               const std::vector<std::string>&  words_lc,
               std::ostream&                    result);


  /**
   * Connect to a xtsc_core.
   *
   * This method connects this xtsc_ext_regfile to the named ext_regfile interface of the specified xtsc_core.
   *
   * @param     core              The xtsc_core to connect to.
   *
   * @param     ext_regfile_name  External regfile name as it appears in the user's TIE code after
   *                              the ext_regfile keyword.  This name must NOT begin with the
   *                              "TIE_" prefix.
   *
   */
  void connect(xtsc::xtsc_core& core, const char *ext_regfile_name);


  /// Dump the connects of the xtsc_ext_regfile to the specified ostream object
  void dump(std::ostream &os);


  /**
   * Return the value stored at the specified address.  If no value is stored at
   * address, then an empty string is returned.
   *
   * @param     address         The address to poke.  It must be in hex with a leading 
   *                            '0x' and contain the number of nibbles implied by the
   *                            "address_bit_width" parameters.
   */
  std::string peek(const std::string& address);


  /**
   * Store the specified value at the specified address.
   *
   * @param     address         The address to poke.  It must be in hex with a leading 
   *                            '0x' and contain the number of nibbles implied by the
   *                            "address_bit_width" parameter.
   *
   * @param     value           The value to store (must be convertible to an
   *                            sc_unsigned).
   */
  void poke(const std::string& address, const std::string& data);


  // SystemC calls this method at the end of simulation
  void end_of_simulation();


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


  /// Get the BinaryLogger for this component (e.g. to adjust its log level)
  log4xtensa::BinaryLogger& get_binary_logger() { return m_binary; }


protected:


  /// Helper method to validate a peek/poke address or throw an exception
  void validate_address(const std::string& address);


  /// Common method to compute/re-compute time delays
  void compute_delays();


  /// Convert m_words[index] to sc_unsigned value
  void get_sc_unsigned(xtsc::u32 index, sc_dt::sc_unsigned& value);


  /// Write the external register file and start the operation.
  void do_write();


  /// Get an sc_unsigned from the pool
  sc_dt::sc_unsigned *new_sc_unsigned(const std::string& value);


  /// Get an sc_unsigned from the pool
  sc_dt::sc_unsigned *new_sc_unsigned(const sc_dt::sc_unsigned& value);


  /// Return an sc_unsigned to the pool
  void delete_sc_unsigned(sc_dt::sc_unsigned*& p_sc_unsigned);


  /// Implementation of xtsc_ext_regfile_if.
  class xtsc_ext_regfile_if_impl : public xtsc::xtsc_ext_regfile_if, public sc_core::sc_object  {
  public:

    /// Constructor
    xtsc_ext_regfile_if_impl(const char *object_name, xtsc_ext_regfile& ext_regfile) :
      sc_object     (object_name),
      m_ext_regfile (ext_regfile),
      m_p_port      (0)
    {}

    /// @see xtsc::xtsc_ext_regfile_if
    void nb_send_write_address_and_data(xtsc::u32 address,
                                        const sc_dt::sc_unsigned& data);

    /// @see xtsc::xtsc_ext_regfile_if
    void nb_send_read_check_address(xtsc::u32 address);

    /// @see xtsc::xtsc_ext_regfile_if
    bool nb_is_read_data_ready();

    /// @see xtsc::xtsc_ext_regfile_if
    void nb_send_read_address(xtsc::u32 address);

    /// @see xtsc::xtsc_ext_regfile_if
    sc_dt::sc_unsigned nb_get_read_data();

    /// @see xtsc::xtsc_ext_regfile_if
    xtsc::u32 nb_get_address_bit_width() { return m_ext_regfile.m_address_bit_width; }

    /// @see xtsc::xtsc_ext_regfile_if
    xtsc::u32 nb_get_data_bit_width() {return m_ext_regfile.m_data_bit_width; }

    /**
     * Get the event that will be notified when the ext_regfile data is available.
     */
    virtual const sc_core::sc_event& default_event() const { return m_ext_regfile.m_ext_regfile_ready_event; }

  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_ext_regfile&           m_ext_regfile;    ///< Our xtsc_ext_regfile object
    sc_core::sc_port_base      *m_p_port;         ///< Port that is bound to us
  };



  xtsc_ext_regfile_if_impl              m_ext_regfile_impl;     ///<  m_ext_regfile binds to this

  xtsc_ext_regfile_parms                m_ext_regfile_parms;    ///< Copy of xtsc_ext_regfile_parms

  xtsc::u32                             m_address_bit_width;    ///<  The ext_regfile address bit width
  xtsc::u32                             m_data_bit_width;       ///<  The ext_regfile data bit width

  //XXXSS: TODO: Are these lua functions needed?
  std::string                           m_ext_regfile_table;    ///<  Name of "ext_regfile_table" file
  std::string                           m_lua_data_function;    ///<  From <LuaDataFunction>  in lua_function line of "ext_regfile_table" file
  std::string                           m_lua_delay_function;   ///<  From <LuaDelayFunction> in lua_function line of "ext_regfile_table" file
  bool                                  m_lua_function;         ///<  True if there was a lua_function line in "ext_regfile_table" file

  bool                                  m_file_logged;          ///<  True if contents of m_file have been logged
  xtsc::xtsc_script_file               *m_file;                 ///<  The ext_regfile_file
  std::string                           m_line;                 ///<  Current line from m_file
  xtsc::u32                             m_line_count;           ///<  Current line number from m_file
  std::vector<std::string>              m_words;                ///<  Current line tokenized into words

  xtsc::u32                             m_wr_addr;              ///<  Current write request address
  xtsc::u32                             m_rd_chk_addr;          ///<  Current read-check request address
  xtsc::u32                             m_rd_addr;              ///<  Current read  request address
  sc_dt::sc_unsigned                    m_wr_data;              ///<  Current write data
  std::map<xtsc::u32, sc_dt::sc_unsigned>
                                        m_data_map;             ///<  The ext_regfile table
  std::map<xtsc::u32, sc_core::sc_time> m_op_fini_time;         ///<  Time at which operation finishes

  //XXXSS: TODO: These data members are most probably not needed.
  std::map<xtsc::u32, xtsc::u32>        m_delay_map;            ///<  Delay associated with each address
  sc_dt::sc_unsigned                    m_data;                 ///<  Current request data
  sc_dt::sc_unsigned                    m_data_temp;            ///<  To hold data temporarily
  sc_dt::sc_unsigned                    m_old_data;             ///<  Data previously at current request address

  sc_dt::sc_unsigned                    m_poke_data;            ///<  Buffer for poke()
  bool                                  m_write;                ///<  True if ext_regfile is a write, false if ext_regfile is a read
  sc_core::sc_event                     m_ext_regfile_ready_event;   ///<  Notified when ext_regfile might be ready (i.e. "ask me again")

  // Note: We use a deque instead of a sc_fifo so that this component can be used in turbo mode
  std::deque<sc_dt::sc_unsigned*>       m_data_fifo;            ///<  FIFO of ext_regfile data values
  std::deque<sc_dt::sc_unsigned*>       m_recycle_fifo;         ///<  FIFO of ext_regfile data values from LuaDataFunction that need to be recycled
  std::deque<xtsc::u64>                 m_cycle_fifo;           ///<  FIFO of clock cycle to get ext_regfile data for latency checking
  sc_dt::sc_unsigned                    m_zero;                 ///<  Constant 0
  sc_dt::sc_unsigned                    m_one;                  ///<  Constant 1
  sc_core::sc_time                      m_time_resolution;      ///<  SystemC time resolution
  sc_core::sc_time                      m_clock_period;         ///<  This module's clock period
  xtsc::u64                             m_clock_period_value;   ///<  Clock period as u64
  bool                                  m_has_posedge_offset;   ///<  True if m_posedge_offset is non-zero
  sc_core::sc_time                      m_posedge_offset;       ///<  From "posedge_offset" parameter
  xtsc::u64                             m_posedge_offset_value; ///<  m_posedge_offset as u64
  std::vector<sc_dt::sc_unsigned*>      m_sc_unsigned_pool;     ///<  Maintain a pool of sc_unsigned to improve performance

  sc_core::sc_trace_file               *m_p_trace_file;         ///<  From "vcd_handle" parameter
  xtsc::u32                             m_nb_send_address_cnt;  ///<  Count nb_send_address() calls for vcd tracing
  xtsc::u32                             m_nb_is_ready_cnt;      ///<  Count nb_is_ready()     calls for vcd tracing
  xtsc::u32                             m_nb_get_data_cnt;      ///<  Count nb_get_data()     calls for vcd tracing
  sc_dt::sc_unsigned                    m_address_trace;        ///<  For vcd tracing
  sc_dt::sc_unsigned                    m_data_trace;           ///<  For vcd tracing

  log4xtensa::TextLogger&               m_text;                 ///<  The logger for text messages
  log4xtensa::BinaryLogger&             m_binary;               ///<  The logger for binary logging

};



}  // namespace xtsc_component




#endif  // _XTSC_EXT_REGFILE_H_
