#ifndef _XTSC_WIRE_SOURCE_H_
#define _XTSC_WIRE_SOURCE_H_

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
#include <xtsc/xtsc_wire_write_if.h>
#include <string>
#include <vector>
#include <fstream>




namespace xtsc {
class xtsc_core;
class xtsc_interrupt_distributor;
}



namespace xtsc_component {

class xtsc_wire_logic;
class xtsc_mmio;

/**
 * Constructor parameters for a xtsc_wire_source object.
 *
 * This class contains the constructor parameters for a xtsc_wire_source object.
 *  \verbatim
   Name                 Type    Description
   ------------------   ----    --------------------------------------------------------
  
   "definition_file"    char*   The name of an optional file providing output port
                                definitions.  If this file is not provide, then there
                                will be a single output whose width is given by the
                                "bit_width" parameter and whose name is "m_write" (if 
                                "pin_level" is false) or "m_pin" (if "pin_level" is
                                true).  Lines in this file must be in the following
                                format (curly braces indicate an optional element):

                                output <OutputName> <BitWidth> {<InitialValue>}

                                1.  A line must begin with the keyword "output" and it
                                    defines an sc_port<xtsc_wire_write_if> and its bit
                                    width.
                                2.  <OutputName> must be valid C/C++ identifier and must
                                    be unique.
                                3.  <BitWidth> must be an integer greater then 0.
                                4.  If the <InitialValue> element contains less bits
                                    then the specified <BitWidth> then the missing
                                    high-order bits are assumed to be 0.
                                    Default = 0.
                                5.  Comments, extra whitespace, and blank lines are
                                    ignored.   See xtsc_script_file.

   "bit_width"          u32     Width of single output port in bits.  This parameter
                                is not used if a "definition_file" is provided.

   "control"            bool    If true, then a 1-bit TLM control input will be created
                                and the "WAIT CONTROL" commands will be enabled in the
                                script file (see "script_file").  The control input can
                                be used to control the xtsc_wire_source device with
                                another device (for example, an xtsc_core when modeling
                                level-sensitive interrupts).
                                Default = false.
                                Note: The control input is a TLM interface regardless of
                                      the "pin_level" setting.

   "pin_level"          bool    If true, then pin-level ports will be used.  If false,
                                then TLM ports will be used.
                                Default = false.

   "vcd_handle"         void*   Pointer to SystemC VCD object (sc_trace_file *) or 0 if
                                tracing is not desired.  
                                Default = 0 (NULL).

   "script_file"        char*   The file to read the test vector commands from (if a
                                "definition_file" is provided and all outputs have an
                                <InitialValue> specified then this parameter is
                                optional).  Each command occupies one line in the file.
                                Valid command formats are shown below (the first two
                                formats show wire write transaction commands; curly
                                braces indicate an optional element and ellipses
                                indicate that an element may be repeated):

                                  <delay> <value> { <value> } ...
                                  <delay> <OutputName>=<value> { <OutputName>=<value> } ...
                                  <delay> STOP
                                  WAIT  <duration>
                                  WAIT  CONTROL WRITE|CHANGE|<value> { <count> }
                                  SYNC  <time>
                                  NOTE  message
                                  INFO  message
                                
                                1.  Integers can appear in decimal or hexadecimal (using
                                    '0x' prefix) format.
                                2.  <delay> specifies how long to wait before writing
                                    <value> to the wire or stopping the simulation.
                                    <delay> can be 0 (to mean 1 delta cycle), or "now"
                                    to mean no delta cycle delay, or a positive integer
                                    or floating point number to mean that many clock
                                    periods (see "clock_period").
                                3.  Each <value> on a line specifies the value to write
                                    to an output port.  In the first format shown above,
                                    the first value on the line applies to the first
                                    output defined in the "definition_file" (or to the
                                    single output port if no "definition_file" is
                                    provided), the second value on the line applies to
                                    the second output defined in the "definition_file",
                                    etc.  If <value> is - (a single hyphen) then that
                                    value is not written.  Trailing missing values are
                                    assumed to be - (and are not written).  In the
                                    second format shown above, each non-hyphen value is
                                    written to the named output port.  To more easily
                                    support toggle signals, if <value> is ~ (a tilde)
                                    then the value written is the bit-wise complement of
                                    the previous value written.
                                4.  The "<delay> STOP" command causes simulation to stop
                                    via a call to the sc_stop() method after the
                                    specified delay.
                                5.  The "WAIT <duration>" command can be used to cause a 
                                    wait of the specified duration.  <duration> can be 0
                                    (to mean 1 delta cycle) or a positive integer or
                                    floating point number to mean that many clock
                                    periods.
                                6.  If the "control" parameter was set to true then the
                                    "WAIT CONTROL" command can be used to cause a wait
                                    until the specified activity occurs on the control
                                    input.  WRITE means any write even if its the same
                                    value, CHANGE means a write of a new value, and
                                    <value> (which can only be 0 or 1) means a write of
                                    the specified value.  An optional <count> can be
                                    specified to mean the event has to occur <count>
                                    times.  The default <count> is 1.  The default event
                                    is WRITE (so "wait control" is the same thing as
                                    "wait control write 1").
                                7.  The "SYNC <time>" command with <time> less than 1.0
                                    can be used to cause a wait to the clock phase
                                    (relative to posedge clock) specified by <time>.
                                    Posedge clock is as specified by "posedge_offset".
                                    For example, "sync 0.5" will cause the minimum wait
                                    necessary to sync to negedge clock.
                                    The "SYNC <time>" command with <time> greater than
                                    or equal to 1.0 can be used to cause a wait until
                                    the specified absolute simulation time.
                                8.  The NOTE and INFO commands can be used to cause
                                    the entire line to be logged at NOTE_LOG_LEVEL
                                    or INFO_LOG_LEVEL, respectively.
                                9.  Words are case insensitive.
                               10.  Comments, extra whitespace, blank lines, and lines
                                    between "#if 0" and "#endif" are ignored.  
                                    See xtsc_script_file for a complete list of
                                    pseudo-preprocessor commands.

   "clock_period"       u32     This is the length of this device's clock period
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

   "wraparound"         bool    If false (the default), "script_file" is only processed
                                one time.  If true, the file pointer will be reset to
                                the beginning of the file each time the end of file is
                                reached.
                                Default = false.

    \endverbatim
 *
 * @see xtsc_wire_source
 * @see xtsc::xtsc_parms
 * @see xtsc::xtsc_script_file
 */
class XTSC_COMP_API xtsc_wire_source_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_wire_source_parms object.
   *
   * @param     bit_width               The width of the output port in bits (only used
   *                                    if definition_file is NULL).
   *
   * @param     script_file             The file name to read test vectors from.
   *
   * @param     wraparound              If false (the default), the file is only
   *                                    processed one time.  If true, the file pointer
   *                                    will be reset to the beginning of the file each
   *                                    time the end of file is reached.
   *
   * @param     definition_file         See the "definition_file" parameter
   */
  xtsc_wire_source_parms(xtsc::u32 bit_width = 1, const char *script_file = "", bool wraparound = false, const char *definition_file = NULL) {
    add("bit_width",            bit_width);
    add("control",              false);
    add("script_file",          script_file);
    add("clock_period",         0xFFFFFFFF);
    add("posedge_offset",       0xFFFFFFFF);
    add("wraparound",           wraparound);
    add("pin_level",            false);
    add("vcd_handle",           (void*) 0); 
    add("definition_file",      definition_file);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_wire_source_parms"; }

};





/**
 * A scripted xtsc::xtsc_wire_write_if or pin-level source.
 *
 * This XTSC device implements a source that reads an input file ("script_file") to
 * determine when and what values to drive out TLM wire write ports or pin-level ports.
 * Multiple output ports can be defined and driven by one xtsc_wire_source device;
 * however, the ports driven by a particular xtsc_wire_source object must be all TLM or
 * all pin-level.  The TLM wire write ports can be connected to any
 * xtsc::xtsc_wire_write_if implementation (such as an input of an xtsc_mmio or
 * xtsc_wire_logic device, an xtsc_wire, or a system-level input wire of
 * xtsc::xtsc_core).  The pin-level ports can be connected to a sc_signal<sc_bv_base>
 * (such as would be connected to a system-level input pin of xtsc::xtsc_core or to a
 * Verilog module).  To use pin-level ports, you must set the "pin_level" parameter to
 * true.
 *
 * To provide a degree of feedback or control of the script, the "control" option can be
 * set to true and a wire writer such as xtsc_core, xtsc_mmio, or xtsc_wire_logic can be
 * connected to the control input.  This allows the xtsc_wire_source device to better
 * model certain SoC components.  For example, it can better model a source of a
 * level-sensitive interrupt to an xtsc_core because the control input provides a means
 * for the xtsc_core to reset the interrupt source by writing to the control input (this
 * can be done using a TIE export state of by writing to a memory location that maps to
 * an xtsc_mmio device which converts the memory-mapped write into a wire write to the
 * control input of the xtsc_wire_source).
 *
 * To perform port binding, use the xtsc::xtsc_connect() method or do manual SystemC
 * port binding using the get_control_input(), get_tlm_output(), or get_output_pin()
 * methods to obtain references to the desired sc_port/sc_out.  If no "definition_file"
 * is used, then manual SystemC port binding can also be done using the
 * xtsc_wire_source::m_write port ("pin_level" false) or xtsc_wire_source::m_pin port
 * ("pin_level" true).
 *
 * Here is a block diagram of an xtsc_wire_source as it is used in the xtsc_wire_source
 * example:
 * @image html  xtsc_wire_source.jpg
 * @image latex xtsc_wire_source.eps "xtsc_wire_source Example" width=10cm
 *
 * Here is the code to connect the xtsc_wire_source to core0 using raw SystemC port
 * binding and the xtsc_wire_source::m_write data member:
 * \verbatim
    BReset.m_write(core0.get_input_wire("BReset"));
   \endverbatim
 *
 * Alternately, raw SystemC port binding can be done using the get_tlm_output() method
 * which will also work when a "definition_file" is used:
 * \verbatim
    BReset.get_tlm_output("m_write")(core0.get_input_wire("BReset"));
   \endverbatim
 *
 * And here is the code to connect using the xtsc::xtsc_connect() method:
 * \verbatim
    xtsc_connect(BReset, "m_write", "BReset", core0);
   \endverbatim
 *
 * @see xtsc_wire_source_parms
 * @see xtsc_wire
 * @see xtsc_wire_logic
 * @see xtsc::xtsc_wire_write_if
 * @see xtsc::xtsc_connect()
 * @see xtsc::xtsc_core
 * @see xtsc::xtsc_core::How_to_do_port_binding
 * @see xtsc::xtsc_core::get_system_input_wire()
 */
class XTSC_COMP_API xtsc_wire_source : public sc_core::sc_module, public xtsc::xtsc_module, virtual public xtsc::xtsc_wire_write_if {
public:

  // See get_control_input() if the "control" parameter was set to true
  // See get_tlm_output() or get_output_pin() if the "definition_file" parameter is used to define multiple outputs

  sc_core::sc_out<sc_dt::sc_bv_base>                    m_pin;          ///<  Pin-level port
  sc_core::sc_port<xtsc::xtsc_wire_write_if, NSPP>      m_write;        ///<  TLM port


  // For SystemC
  SC_HAS_PROCESS(xtsc_wire_source);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_wire_source"; }


  /**
   * Constructor for an xtsc_wire_source.
   *
   * @param     module_name     Name of the xtsc_wire_source sc_module.
   *
   * @param     source_parms    The remaining parameters for construction.
   *
   * @see xtsc_wire_source_parms
   */
  xtsc_wire_source(sc_core::sc_module_name module_name, const xtsc_wire_source_parms& source_parms);


  // Destructor.
  ~xtsc_wire_source(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const;


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// SystemC callback
  virtual void end_of_elaboration(void);


  /**
   * Reset the xtsc_wire_source.
   */
  void reset(bool hard_reset = false);

 
  /// Return the name of the first (or only) output
  std::string get_default_output_name() const;


  /// Return true if the named output exists
  bool has_output(const char *output_name) const;


  /// Return the bit width of the named output
  xtsc::u32 get_bit_width(const char *output_name) const;


  /**
   * Return the sc_export of the optional control input.
   *
   * This method may be used for port binding of the optional control input.
   *
   * For example, to bind the TIE export state named "onebit" of an xtsc_core named core0
   * to the control input of an xtsc_wire_source named source:
   * \verbatim
       core0.get_export_state("onebit")(source.get_control_input());
     \endverbatim
   */
  sc_core::sc_export<xtsc::xtsc_wire_write_if>& get_control_input() const;


  /**
   * Return the sc_port of the named TLM output.  
   *
   * This method can be used for port binding when "pin_level" is false.
   *
   * For example, to bind the sc_port output named "vectors" of an xtsc_wire_source
   * named source to the "BInterrupt" system-level input of an xtsc_core named core0:
   * \verbatim
       source.get_tlm_output("vectors")(core0.get_input_wire("BInterrupt"));
     \endverbatim
   */
  sc_core::sc_port<xtsc::xtsc_wire_write_if, NSPP>& get_tlm_output(const char *output_name) const;


  /**
   * Return the sc_out<sc_bv_base> of the named pin-level output.
   *
   * This method can be used for port binding when "pin_level" is true.
   *
   * For example, to bind the sc_out<sc_bv_base> output named "vectors" of an
   * xtsc_wire_source named source to the "BInterrupt" system-level input of an
   * xtsc_core named core0 (the "SimPinLevelInterfaces" parameter of the xtsc_core_parms
   * object used to create core0 must have contained "BInterrupt"):
   * \verbatim
       source.get_output_pin("vectors")(core0.get_input_pin("BInterrupt"));
     \endverbatim
   */
  sc_core::sc_out<sc_dt::sc_bv_base>& get_output_pin(const char *output_name) const;


  /// Get the set of output names defined for this xtsc_wire_source
  std::set<std::string> get_output_set() const;


  /// Return whether output ports are pin-level (sc_out) or TLM (sc_port)
  bool is_pin_level() const { return m_pin_level; }


  /**
   * Connect an xtsc_wire_logic output to the control input of this xtsc_wire_source.
   *
   * This method connects the specified output of the specified xtsc_wire_logic to the
   * optional control input of this xtsc_wire_source.  This method should not be used
   * unless the "control" parameter was set to true.
   *
   * @param     logic           The xtsc_wire_logic to connect to the control input of
   *                            this xtsc_wire_source.
   *
   * @param     output_name     The output of the xtsc_wire_logic. 
   */
  void connect(xtsc_wire_logic& logic, const char *output_name);


  /**
   * Connect an xtsc_mmio output to the control input of this xtsc_wire_source.
   *
   * This method connects the specified output of the specified xtsc_mmio to the
   * optional control input of this xtsc_wire_source.  This method should not be used
   * unless the "control" parameter was set to true.
   *
   * @param     mmio            The xtsc_mmio to connect to the control input of
   *                            this xtsc_wire_source.
   *
   * @param     output_name     The output of the xtsc_mmio. 
   */
  void connect(xtsc_mmio& mmio, const char *output_name);


  /**
   * Connect to an xtsc_core.
   *
   * This method connects the specified output of this xtsc_wire_source to a
   * system-level TLM input of an xtsc_core.  This method should not be used when
   * "pin_level" is true.
   *
   * @param     core            The xtsc_core to connect to.
   *
   * @param     input_name      The system-level input of the xtsc_core that this
   *                            xtsc_wire_source is to be connected to.  input_name is
   *                            the name as it appears in the Xtensa microprocessor data
   *                            book.  
   *
   * @param     output_name     The output of this xtsc_wire_source.  If output_name is
   *                            NULL, then the default (first/only) output of this
   *                            xtsc_wire_source is used.
   */
  void connect(xtsc::xtsc_core& core, const char *input_name, const char *output_name = NULL);


#ifndef DOXYGEN_SKIP
  /**
   * Connect to an xtsc_interrupt_distributor.
   *
   * This method connects the specified output of this xtsc_wire_source to the specified
   * input of the specified xtsc_interrupt_distributor.  This method should not be used
   * when "pin_level" is true.
   *
   * @param     distributor     The xtsc_interrupt_distributor to connect to.
   *
   * @param     input_name      The input of the xtsc_interrupt_distributor that this
   *                            xtsc_wire_source is to be connected to.
   *
   * @param     output_name     The output of this xtsc_wire_source.  If output_name is
   *                            NULL, then the default (first/only) output of this
   *                            xtsc_wire_source is used.
   */
  void connect(xtsc::xtsc_interrupt_distributor& distributor, const char *input_name, const char *output_name = NULL);
#endif // DOXYGEN_SKIP


  /// Thread to send the writes (from m_script_file) out the output ports
  void write_thread(void);


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


protected:

  // Forward declaration
  class output_definition;

  // Shorthand aliases
  typedef std::vector<output_definition*>                       output_definition_vector;
  typedef std::map<std::string, output_definition*>             output_definition_map;
  typedef std::set<output_definition*>                          output_set;
  typedef sc_core::sc_port<xtsc::xtsc_wire_write_if, NSPP>      tlm_port;
  typedef sc_core::sc_out<sc_dt::sc_bv_base>                    pin_port;



  /**
   * Output definition and sc_port.
   */
  class output_definition {
  public:

    /// Constructor
    output_definition(xtsc_wire_source&         source,
                      const std::string&        name,
                      xtsc::u32                 bit_width,
                      const std::string&        initial_value,
                      tlm_port                 *p_tlm_port,
                      pin_port                 *p_pin_port
                      );

    /// Reset and drive the output
    void reset() { 
      drive(m_initial_value);
    }

    /// Drive the output
    void drive(const std::string& value) { 
      m_value = value.c_str();
      // a variable substr is used to get rid of the pesky extra leading bit
      XTSC_INFO(m_source.m_text, m_name << " => 0x" << m_value.to_string(sc_dt::SC_HEX).substr(m_bit_width%4 ? 2 : 3));
      if (m_pin_level) {
        m_value_bv = m_value;
        m_p_pin_port->write(m_value_bv);
      }
      else {
        (*m_p_tlm_port)->nb_write(m_value);
      }
    }

    /// Drive bit-wise complement of the previous value driven
    void complement_output() { 
      m_value = ~m_value;
      // a variable substr is used to get rid of the pesky extra leading bit
      XTSC_INFO(m_source.m_text, m_name << " => 0x" << m_value.to_string(sc_dt::SC_HEX).substr(m_bit_width%4 ? 2 : 3));
      if (m_pin_level) {
        m_value_bv = m_value;
        m_p_pin_port->write(m_value_bv);
      }
      else {
        (*m_p_tlm_port)->nb_write(m_value);
      }
    }



    xtsc_wire_source&           m_source;               ///<  Our xtsc_wire_source object
    std::string                 m_name;                 ///<  Port name
    xtsc::u32                   m_index;                ///<  Our output index
    xtsc::u32                   m_bit_width;            ///<  Port width in bits
    bool                        m_pin_level;            ///<  True output port is pin-level
    sc_dt::sc_unsigned          m_value;                ///<  Latest value of output
    sc_dt::sc_bv_base           m_value_bv;             ///<  Current value from "script_file" (for pin-level)
    std::string                 m_initial_value;        ///<  From <InitialValue> in definition_file
    tlm_port                   *m_p_tlm_port;           ///<  output port for TLM
    pin_port                   *m_p_pin_port;           ///<  output port for pin-level
  };



  class xtsc_wire_write_if_impl : public xtsc::xtsc_wire_write_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_wire_write_if_impl(const std::string& port_name, xtsc_wire_source& source) :
      sc_object         (port_name.c_str()),
      m_source          (source),
      m_name            (port_name),
      m_bit_width       (1),
      m_p_port          (0)
    {}

    /// The kind of sc_object we are
    const char* kind() const { return "xtsc_wire_source::xtsc_wire_write_if_impl"; }

    /**
     *  Receive writes from the wire source
     *  @see xtsc::xtsc_wire_write_if
     */
    virtual void nb_write(const sc_dt::sc_unsigned& value);

    /**
     *  Get the wire width in bits.
     *  @see xtsc::xtsc_wire_write_if
     */
    virtual xtsc::u32 nb_get_bit_width() { return m_bit_width; }


  protected:

    /// SystemC callback when something binds to us
    void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_wire_source&         m_source;         ///< Our xtsc_wire_source object
    std::string               m_name;           ///< Our name as a std::string
    xtsc::u32                 m_bit_width;      ///< Width in bits
    sc_core::sc_port_base    *m_p_port;         ///< Port that is bound to us
  };




  typedef sc_core::sc_export<xtsc::xtsc_wire_write_if>          wire_write_export;


  log4xtensa::TextLogger&       m_text;                         ///<  TextLogger
  xtsc::u32                     m_width1;                       ///<  Bit width of 1st output port
  bool                          m_control;                      ///<  From "control" parameter
  bool                          m_control_bound;                ///<  Something is connected to the control input
  wire_write_export            *m_p_control;                    ///<  Optional control input
  xtsc_wire_write_if_impl      *m_p_write_impl;                 ///<  Implementaion for optional control input
  sc_dt::sc_unsigned            m_control_value;                ///<  Current value of the control input
  xtsc::u32                     m_control_write_count;          ///<  Number of times control input is written
  xtsc::u32                     m_control_change_count;         ///<  Number of times control input is written with a new value
  sc_core::sc_event             m_control_write_event;          ///<  Notified when control input is written
  bool                          m_wraparound;                   ///<  Should "script_file" wraparound
  bool                          m_pin_level;                    ///<  True if pin-level, false if TLM
  sc_core::sc_trace_file       *m_p_trace_file;                 ///<  Pointer to sc_trace_file or NULL if not tracing
  std::string                   m_definition_file;              ///<  Name of file to get outputs from (from "definition_file")
  std::string                   m_script_file;                  ///<  Name of file to read values from (from "script_file")
  xtsc::xtsc_script_file       *m_p_test_vector_stream;         ///<  File to read values from
  std::string                   m_line;                         ///<  Current line of "definition_file"/"script_file"
  xtsc::u32                     m_line_count;                   ///<  Current line number in "definition_file"/"script_file"
  std::vector<std::string>      m_words;                        ///<  Tokenized words from "definition_file"/"script_file"
  std::vector<std::string>      m_words_lc;                     ///<  Lower-case version of m_words
  xtsc::u64                     m_clock_period_value;           ///<  This device's clock period as u64
  sc_core::sc_time              m_clock_period;                 ///<  From "clock_period" parameter
  sc_core::sc_time              m_time_resolution;              ///<  The SystemC time resolution
  bool                          m_has_posedge_offset;           ///<  True if m_posedge_offset is non-zero
  sc_core::sc_time              m_posedge_offset;               ///<  From "posedge_offset" parameter
  xtsc::u64                     m_posedge_offset_value;         ///<  m_posedge_offset as u64
  output_definition_vector      m_outputs;                      ///<  Vector of all output definitions
  output_definition_map         m_output_definition_map;        ///<  Map of output definitions
  std::set<std::string>         m_output_set;                   ///<  Set of names of all outputs
  xtsc::xtsc_port_table         m_port_table_all;               ///<  All wire outputs and control input (if present)
  xtsc::xtsc_port_table         m_port_table_all_outputs;       ///<  All wire outputs

  std::vector<sc_core::sc_process_handle>
                                m_process_handles;              ///<  For reset 


  /// Extract a u32 value (named argument_name) from the word at m_words[index]
  xtsc::u32 get_u32(xtsc::u32 index, const std::string& argument_name, xtsc::xtsc_script_file *p_script_file);


  /// Extract a double value (named argument_name) from the word at m_words[index]
  double get_double(xtsc::u32 index, const std::string& argument_name, xtsc::xtsc_script_file *p_script_file);


  /**
   * Throw an exception if m_words[index] (named argument_name) does not exists or is
   * not a valid C/C++ identifier, otherwise return the m_words[index].
   */
  const std::string& validate_identifier(xtsc::u32 index, const std::string& argument_name, xtsc::xtsc_script_file *p_script_file);


  /// Return whether or not name is a valid identifier
  bool is_identifier(const std::string& name);


  // These methods are to cap off the single TLM port (m_write) when this source is 
  // connected at the pin-level.  These methods will never be called.
  void nb_write(const sc_dt::sc_unsigned& value) { return; }
  xtsc::u32 nb_get_bit_width() { return m_width1; }

  // This signal is to cap off the pin-level port when this driver is 
  // connected at the TLM level
  xtsc::xtsc_signal_sc_bv_base_floating m_pin_floating;
};



}  // namespace xtsc_component



#endif  // _XTSC_WIRE_SOURCE_H_
