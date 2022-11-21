#ifndef _XTSC_MMIO_H_
#define _XTSC_MMIO_H_

// Copyright (c) 2007-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems, Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */


#include <map>
#include <set>
#include <vector>
#include <string>
#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_request.h>
#include <xtsc/xtsc_request_if.h>
#include <xtsc/xtsc_respond_if.h>
#include <xtsc/xtsc_wire_write_if.h>





namespace xtsc {
class xtsc_core;
class xtsc_interrupt_distributor;
}




namespace xtsc_component {


/**
 * Constructor parameters for a xtsc_mmio object.
 *
 *  \verbatim
   Name                 Type    Description
   ------------------   ----    --------------------------------------------------------
  
   "byte_width"         u32     Memory data interface width in bytes.  Valid values are
                                4, 8, 16, 32, and 64.

   "definition_file"    char*   The name of a file providing memory-mapped I/O register
                                definitions, output sc_port definitions, and input
                                sc_export definitions.  Lines in this file must be in
                                one of the following three formats (curely braces
                                indicate optional elements):

         register <RegisterName> <BitWidth> <Address> {read_only} {<InitialValue>}

         output <PortName> <RegisterName> {<HighBit> {<LowBit>}}

         input <ExportName> <RegisterName> {<HighBit> {<LowBit>}}

                                1.  A line beginning with the keyword "register" defines
                                    a register that can be read and written using the
                                    memory interface (that is, m_request_export and
                                    m_respond_port).  Memory-mapped writes to this
                                    register result in calls to the nb_write() method of
                                    the xtsc_wire_write_if interface of the sc_port
                                    objects defined by output definition lines that map
                                    to this register (see #2 below).  Memory-mapped
                                    reads of this register return the last value written
                                    to this register.  Values can be written to a
                                    register by one of two means.  First, by memory-
                                    mapped writes just discussed.  Second, by calls to
                                    the nb_write() method of the xtsc_wire_write_if
                                    interface bound to the sc_export objects defined by
                                    any input definition lines (see #3 below) that map
                                    to the register.
                                2.  A line beginning with the keyword "output" defines an
                                    sc_port<xtsc_wire_write_if> and specifies which
                                    bits of which register map to that sc_port.
                                3.  A line beginning with the keyword "input" defines an
                                    sc_export<xtsc_wire_write_if> and specifies which
                                    bits of which register that sc_export maps to.
                                    If the corresponding register is 'read_only', "input"
                                    is disallowed.
                                4.  <RegisterName>, <PortName>, and <ExportName> must be
                                    valid C/C++ identifiers.
                                5.  <PortName> and <ExportName> must be unique (they
                                    have the same name space).
                                6.  Every <RegisterName> that appears in the definition
                                    file must appear in exactly one register definition
                                    line.
                                7.  <BitWidth> must be between 1 and B*8, where
                                        B = "byte_width"
                                8.  A register occupies N bytes of the address space
                                    from <Address> to <Address> + N - 1.  N is defined
                                    as:
                                        N = ceil(W/8)
                                    Where,
                                        W = <BitWidth>
                                9.  Writes to memory-mapped addresses not covered by a
                                    register definition line are logged and discarded. 
                                    Reads to memory-mapped addresses not covered by a
                                    register definition line are logged and return all
                                    zeroes.
                                10. If the optional <read_only> is specified, writes 
                                    to the register are discarded. 
                                11. If the optional <InitialValue> element is not
                                    preset, it defaults to 0.
                                12. A <RegisterName> may appear in any combination of
                                    input and output definition lines or none at all;
                                    however, a specific bit of a register may be used in
                                    at most one input or output definition line.
                                13. The size of an input or output is defined as follows:
                                    If both <LowBit> and <HighBit> are present:
                                        Size = <HighBit> - <LowBit> + 1
                                    If <HighBit> is present and <LowBit> is missing:
                                        Size = 1
                                    If both <LowBit> and <HighBit> are missing:
                                        Size = <BitWidth> of corresponding register
                                14. Comments, extra whitespace, and blank lines are
                                    ignored.   See xtsc_script_file.

   "swizzle_bytes"      bool    If true, the bytes in the data payload of memory-mapped
                                writes will be swizzled prior to being stored in the
                                register.  For memory-mapped reads, the bytes will be
                                taken from the register and then swizzled prior to being
                                loaded into the response data payload.  This may be
                                useful when the memory interface of the xtsc_mmio object
                                is connected to a big-endian xtsc_core object.  
                                Default = false (do not swizzle bytes).

   "always_write"       bool    If true, a write to a register will result in a write to
                                all output ports mapped to that register regardless of
                                whether or not the port values changed.  If false, a
                                write to a mapped port will occur only if the port value
                                has changed.  At start of simulation and at reset all
                                ports are written with their initial value regardless of
                                this parameter.
                                Default = true.

   "use_fast_access"     bool   If true, this device will support fast access for the
                                TurboXim simulation engine using peek/poke.  If false,
                                TurboXim fast access will not be allowed.
                                Default = true.

   "clock_period"        u32    This is the length of this device's clock period
                                expressed in terms of the SystemC time resolution
                                (from sc_get_time_resolution()).  A value of 
                                0xFFFFFFFF means to use the XTSC system clock 
                                period (from xtsc_get_system_clock_period()).  A value
                                of 0 means one delta cycle.
                                Default = 0xFFFFFFFF (i.e. use the system clock 
                                period).

   "response_time"      u32     This is the number of clock periods that this device
                                takes to respond to memory-interface requests.  A value
                                of 0 means one delta cycle.
                                Default = 1.

    \endverbatim
 *
 * @see xtsc_mmio
 * @see xtsc::xtsc_parms
 */
class XTSC_COMP_API xtsc_mmio_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_mmio_parms object.
   *
   * @param definition_file     See the "definition_file" parameter
   *
   * @param width8              Memory data interface width in bytes.
   *
   * @param swizzle_bytes       See the "swizzle_bytes" parameter
   *
   */
  xtsc_mmio_parms(const char *definition_file = "", xtsc::u32 width8 = 4, bool swizzle_bytes = false) {
    add("byte_width",           width8);
    add("definition_file",      definition_file);
    add("swizzle_bytes",        swizzle_bytes);
    add("use_fast_access",      true);
    add("clock_period",         0xFFFFFFFF);
    add("response_time",        1);
    add("always_write",         true);
  }

  /// Our C++ type (the xtsc_parms base class uses this for error messages)
  virtual const char* kind() const { return "xtsc_mmio_parms"; }

};


// Forward references
class xtsc_arbiter;
class xtsc_master;
class xtsc_memory_trace;
class xtsc_router;
class xtsc_wire_logic;
class xtsc_wire_source;
class xtsc_pin2tlm_memory_transactor;



/**
 * A general-purpose memory-mapped input/output (MMIO) register device.
 *
 * This device models a set of memory-mapped registers that connect to general-purpose
 * I/O ports.  A file (named by the "definition_file" parameter of xtsc_mmio_parms) is
 * used to define the registers that this device models as well as the input and
 * output ports connected to those registers.
 *
 * The device is a memory interface slave so that the registers can be written by a
 * memory interface master such as xtsc::xtsc_core or xtsc_master.  The memory interface
 * would typically be connected to the system memory interface of an xtsc_core by way of
 * an xtsc_router so that only the addresses that map to this device get routed to it and
 * other addresses can go to some other device, for example, an xtsc_memory connected to
 * another port of the xtsc_router.  This device only supports single read and single
 * write transactions (PIF: READ/WRITE; AXI: single-transfer BURST_READ/BURST_WRITE).
 *
 * The device has as many wire I/O ports as are defined in the definition file.  The
 * input ports (technically, sc_export<xtsc_wire_write_if>) can be written by any device
 * having an sc_port<xtsc_wire_write_if>, for example, a TIE export state of an
 * xtsc::xtsc_core, an xtsc_wire_logic output, or an xtsc_wire_source.  The output ports
 * (technically, sc_port<xtsc_wire_write_if>) can write to any device implementing the
 * xtsc::xtsc_wire_write_if, for example, a system-level input wire of an xtsc_core such
 * as "BInterrupt" or "BReset", an xtsc_wire_logic input, or an xtsc_wire.
 *
 * Because the I/O ports are not known until construction time (when the definition file
 * is processed), they are not named members of the class.  To perform port binding, use
 * the xtsc::xtsc_connect() method or do manual SystemC port binding using the
 * get_input() and get_output() methods to obtain references to the desired port.
 *
 * Here is a block diagram of the system used in the xtsc_mmio example:
 * @image html  Example_xtsc_mmio.jpg
 * @image latex Example_xtsc_mmio.eps "xtsc_mmio Example" width=13cm
 *
 * Here is the code to connect the xtsc_mmio example system using the
 * xtsc::xtsc_connect() method:
 * \verbatim
    xtsc_connect(core0, "pif", "slave_port", router);
    xtsc_connect(router, "master_port[0]", "", core0_pif);
    xtsc_connect(router, "master_port[1]", "slave_port", mmio);
    xtsc_connect(core1, "BInterrupt10", "BInterrupt10", mmio);
    xtsc_connect(core1, "EXPSTATE", "EXPSTATE", mmio);
    xtsc_connect(core1, "pif", "", core1_pif);
   \endverbatim
 *
 * And here is the code to connect the system using manual SystemC port binding:
 * \verbatim
    core0.get_request_port("pif")(router.m_request_export);
    router.m_respond_port(core0.get_respond_export("pif"));
    (*router.m_request_ports[0])(*core0_pif.m_request_exports[0]);
    (*core0_pif.m_respond_ports[0])(*router.m_respond_exports[0]);
    (*router.m_request_ports[1])(mmio.m_request_export);
    mmio.m_respond_port(*router.m_respond_exports[1]);
    mmio.get_output("BInterrupt10")(core1.get_system_input_wire("BInterrupt10"));
    core1.get_export_state("EXPSTATE")(mmio.get_input("EXPSTATE"));
    core1.get_request_port("pif")(*core1_pif.m_request_exports[0]);
    (*core1_pif.m_respond_ports[0])(core1.get_respond_export("pif"));
   \endverbatim
 *
 * @see xtsc_mmio_parms
 * @see xtsc::xtsc_core
 * @see xtsc_router
 * @see xtsc_memory
 * @see xtsc_wire
 * @see xtsc::xtsc_connect()
 * @see xtsc::xtsc_core::How_to_do_port_binding
 */
class XTSC_COMP_API xtsc_mmio : public sc_core::sc_module, public xtsc::xtsc_module, public xtsc::xtsc_command_handler_interface  {
public:

  /// From the memory interface master to us
  sc_core::sc_export<xtsc::xtsc_request_if>     m_request_export;

  /// From us to the memory interface master
  sc_core::sc_port  <xtsc::xtsc_respond_if>     m_respond_port;

  // See get_input() and get_output()


  /// This SystemC macro inserts some code required for SC_THREAD's to work
  SC_HAS_PROCESS(xtsc_mmio);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_mmio"; }


  /**
   * Constructor for a xtsc_mmio.
   *
   * @param     module_name     Name of the xtsc_mmio sc_module.
   * @param     mmio_parms      The remaining parameters for construction.
   *
   * @see xtsc_mmio_parms
   */
  xtsc_mmio(sc_core::sc_module_name module_name, const xtsc_mmio_parms& mmio_parms);


  /// Destructor.
  ~xtsc_mmio(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const { return "slave_port"; }


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// Return true if the named input exists
  bool has_input(const char *input_name) const;


  /// Return true if the named output exists
  bool has_output(const char *output_name) const;


  /// Return the bit width of the name input/output
  xtsc::u32 get_bit_width(const char *io_name) const;


  /**
   * Return the sc_export of the named input.
   *
   * This method is used for port binding. For example, to bind the TIE export state
   * named "foo" of an xtsc_core named core0 to the sc_export input named "bar" of an
   * xtsc_mmio named mmio:
   * \verbatim
       core0.get_export_state("foo")(mmio.get_input("bar"));
     \endverbatim
   *
   */
  sc_core::sc_export<xtsc::xtsc_wire_write_if>& get_input(const char *input_name) const;


  /**
   * Return the sc_port of the named output.
   *
   * This method is used for port binding. For example, to bind the sc_port output named
   * "vectors" of an xtsc_mmio named mmio to the "BInterrupt" system-level input of an
   * xtsc_core named core0:
   * \verbatim
       mmio.get_output("vectors")(core0.get_input_wire("BInterrupt"));
     \endverbatim
   */
  sc_core::sc_port<xtsc::xtsc_wire_write_if, NSPP>& get_output(const char *output_name) const;


  /// Get the set of input names defined for this xtsc_mmio
  std::set<std::string> get_input_set() const;


  /// Get the set of output names defined for this xtsc_mmio
  std::set<std::string> get_output_set() const;


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        change_clock_period <ClockPeriodFactor>
          Call xtsc_mmio::change_clock_period(<ClockPeriodFactor>).  Return previous
          <ClockPeriodFactor> for this device.

        peek <StartAddress> <NumBytes>
          Peek <NumBytes> starting at <StartAddress>.

        poke <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>
          Poke <NumBytes> (=N) starting at <StartAddress>.

        reset
          Call xtsc_mmio::reset().
      \endverbatim
   */
  virtual void execute(const std::string&               cmd_line,
                       const std::vector<std::string>&  words,
                       const std::vector<std::string>&  words_lc,
                       std::ostream&                    result);


  /**
   * Connect an xtsc_arbiter with this xtsc_mmio.
   *
   * This method connects the master port pair of the xtsc_arbiter with the memory
   * interface slave port pair of this xtsc_mmio.
   *
   * @param     arbiter          The xtsc_arbiter to connect with this xtsc_mmio.
   *
   */
  void connect(xtsc_arbiter& arbiter);


  /**
   * Connect an xtsc_core with the memory interface slave port pair of this xtsc_mmio.
   *
   * This method connects the memory interface master port pair specified by
   * memory_port_name of the xtsc_core specified by core with the memory interface slave
   * port pair of this xtsc_mmio.
   *
   * @param     core                    The xtsc_core to connect with this xtsc_mmio.
   *
   * @param     memory_port_name        The name of the memory interface master port
   *                                    pair of the xtsc_core to connect with this
   *                                    xtsc_mmio.  Case-insensitive.
   *
   * @see xtsc::xtsc_core::How_to_do_memory_port_binding for a list of valid
   *      memory_port_name values.
   */
  void connect(xtsc::xtsc_core& core, const char *memory_port_name);


  /**
   * Connect an xtsc_mmio wire input or output to an xtsc_core.
   *
   * This method connects an export state or system-level output wire of an xtsc_core to
   * this xtsc_mmio or it connects this xtsc_mmio to a system-level input wire of an
   * xtsc_core.
   *
   * @param     core            The xtsc_core to connect with.
   *
   * @param     core_intf_name  The export state or system-level output/input wire of
   *                            the xtsc_core that is to be connected to this xtsc_mmio.
   *                            For an export state, core_intf_name is the name as it
   *                            appears in the user's TIE code (it must NOT begin with
   *                            the "TIE_" prefix).  For a system-level output/input
   *                            wire, core_intf_name is the name as it appears in the
   *                            Xtensa microprocessor data book.
   *
   * @param     io_name         The output or input of this xtsc_mmio to be connected to
   *                            the xtsc_core.  If core_intf_name is an xtsc_core export
   *                            state or a system-level output wire then io_name must
   *                            name an input.  If core_intf_name is an xtsc_core
   *                            system-level input wire then io_name must name an output.
   *
   * @see xtsc::xtsc_core::get_output_wire().
   * @see xtsc::xtsc_core::get_input_wire().
   */
  void connect(xtsc::xtsc_core& core, const char *core_intf_name, const char *io_name);


#ifndef DOXYGEN_SKIP
  /**
   * Connect an xtsc_interrupt_distributor with this xtsc_mmio.
   *
   * This method connects the specified output/input of the specified
   * xtsc_interrupt_distributor with the specified input/output of this xtsc_mmio.
   *
   * @param     distributor             The xtsc_interrupt_distributor to be connected.
   *
   * @param     distributor_io_name     The output/input of the distributor to be
   *                                    connected with this xtsc_mmio.  Whether this
   *                                    names an output or an input will be determined
   *                                    by querying distributor.
   *
   * @param     mmio_io_name            The input/output of this xtsc_mmio to be
   *                                    connected with distributor.
   */
  void connect(xtsc::xtsc_interrupt_distributor& distributor, const char *distributor_io_name, const char *mmio_io_name);
#endif // DOXYGEN_SKIP


  /**
   * Connect an xtsc_master with this xtsc_mmio.
   *
   * This method connects the memory interface master port pair of xtsc_master with the
   * memory interface slave port pair of this xtsc_mmio.
   *
   * @param     master          The xtsc_master to connect with this xtsc_mmio.
   */
  void connect(xtsc_master& master);


  /**
   * Connect an xtsc_memory_trace with this xtsc_mmio.
   *
   * This method connects the specified memory interface master port pair of the
   * specified xtsc_memory_trace with the memory interface slave port pair of this
   * xtsc_mmio.
   *
   * @param     memory_trace    The xtsc_memory_trace to connect with this xtsc_mmio.
   *
   * @param     port_num        The xtsc_memory_trace master port pair to connect with
   *                            this xtsc_mmio.
   */
  void connect(xtsc_memory_trace& memory_trace, xtsc::u32 port_num);


  /**
   * Connect an xtsc_pin2tlm_memory_transactor with this xtsc_mmio.
   *
   * This method connects the specified TLM master port pair of the specified
   * xtsc_pin2tlm_memory_transactor with the memory interface slave port pair of this
   * xtsc_mmio.
   *
   * @param     pin2tlm         The xtsc_pin2tlm_memory_transactor to connect with this
   *                            xtsc_mmio.
   *
   * @param     port_num        The TLM master port pair of the
   *                            xtsc_pin2tlm_memory_transactor to connect with this
   *                            xtsc_mmio.  port_num must be in the range of 0 to the
   *                            xtsc_pin2tlm_memory_transactor's "num_ports" parameter
   *                            minus 1.
   */
  void connect(xtsc_pin2tlm_memory_transactor& pin2tlm, xtsc::u32 port_num);


  /**
   * Connect an xtsc_router with this xtsc_mmio.
   *
   * This method connects the specified master port pair of the specified xtsc_router
   * with the memory interface slave port pair of this xtsc_mmio.
   *
   * @param     router          The xtsc_router to connect with this xtsc_mmio.
   *
   * @param     port_num        The master port pair of the xtsc_router to connect with
   *                            this xtsc_mmio.  port_num must be in the range of 0 to
   *                            the xtsc_router's "num_slaves" parameter minus 1.
   */
  void connect(xtsc_router& router, xtsc::u32 port_num);


  /**
   * Connect a wire output of another xtsc_mmio to a wire input of this xtsc_mmio.
   *
   * This method connects the specified wire output of the specified xtsc_mmio to the
   * specified wire input of this xtsc_mmio.
   *
   * @param     mmio            The other xtsc_mmio to be connected to this xtsc_mmio.
   *
   * @param     output_name     The output of the other xtsc_mmio to be connected to an
   *                            input of this xtsc_mmio.
   *
   * @param     input_name      The input of this xtsc_mmio that the other xtsc_mmio is 
   *                            to be connected to.
   */
 void connect(xtsc_mmio& mmio, const char *output_name, const char *input_name);


  /**
   * Connect an xtsc_wire_logic output to an input of this xtsc_mmio.
   *
   * This method connects the specified output of the specified xtsc_wire_logic to the
   * specified wire input of this xtsc_mmio.
   *
   * @param     logic           The xtsc_wire_logic to be connected to this xtsc_mmio.
   *
   * @param     output_name     The output of the xtsc_wire_logic to be connected to
   *                            this xtsc_mmio.
   *
   * @param     input_name      The input of this xtsc_mmio that the xtsc_wire_logic is
   *                            to be connected to.
   */
  void connect(xtsc_wire_logic& logic, const char *output_name, const char *input_name);


  /**
   * Connect an xtsc_wire_source output to an input of this xtsc_mmio.
   *
   * This method connects the specified output of the specified xtsc_wire_source to the
   * specified wire input of this xtsc_mmio.
   *
   * @param     source          The xtsc_wire_source to connect to this xtsc_mmio.
   *
   * @param     output_name     The output of the xtsc_wire_source to be connected to
   *                            this xtsc_mmio.  If this parameter is NULL or empty then
   *                            the default (first/only) output of source will be
   *                            connected.
   *
   * @param     input_name      The input of this xtsc_mmio that the xtsc_wire_source is
   *                            to be connected to.  For backwards compatibility, if
   *                            this parameter is NULL, then the output_name and
   *                            input_name values will be swapped so that the default
   *                            (first/only) output of source will be connected to this
   *                            xtsc_mmio input named output_name (which must not be
   *                            NULL or empty).
   */
  void connect(xtsc_wire_source& source, const char *output_name, const char *input_name = NULL);


  /// Write the initial values to the registers and drive any outputs
  virtual void reset(bool hard_reset = false);


  /**
   * Method to change the clock period.
   *
   * @param     clock_period_factor     Specifies the new length of this device's clock
   *                                    period expressed in terms of the SystemC time
   *                                    resolution (from sc_get_time_resolution()).
   */
  void change_clock_period(xtsc::u32 clock_period_factor);


protected:

  // Forward declaration
  class register_definition;
  class output_definition;
  class input_definition;

  // Shorthand aliases
  typedef std::map<xtsc::xtsc_address, register_definition*>    address_register_map;
  typedef std::map<std::string, register_definition*>           register_definition_map;
  typedef std::map<std::string, output_definition*>             output_definition_map;
  typedef std::map<std::string, input_definition*>              input_definition_map;
  typedef std::set<output_definition*>                          output_set;
  typedef std::set<input_definition*>                           input_set;
  typedef sc_core::sc_port<xtsc::xtsc_wire_write_if, NSPP>      wire_write_port;
  typedef sc_core::sc_export<xtsc::xtsc_wire_write_if>          wire_write_export;


  /// SystemC callback
  virtual void end_of_elaboration(void);


  /// Common method to compute/re-compute time delays
  virtual void compute_delays();


  /// Handle memory-interface requests
  void request_thread(void);


  /**
   * Determine which bits of which register (if any) address maps to.
   * 
   * @param     address         in      The address of interest.
   *
   * @param     high_bit        out     The high bit of the register address maps to.
   *
   * @param     low_bit         out     The low bit of the register address maps to.
   *
   * @returns A pointer to the register_definition object that address maps to, or NULL
   *          if address does not map to a register.
   */
  register_definition *get_register(xtsc::xtsc_address address, xtsc::u32& high_bit, xtsc::u32& low_bit);


  /// Extract a u32 value (named argument_name) from the word at m_words[index]
  xtsc::u32 get_u32(xtsc::u32 index, const std::string& argument_name);


  /// Extract a u64 value (named argument_name) from the word at m_words[index]
  xtsc::u64 get_u64(xtsc::u32 index, const std::string& argument_name);


  /**
   * Throw an exception if m_words[index] (named argument_name) does not exists or is
   * not a valid C/C++ identifier, otherwise return the m_words[index].
   */
  const std::string& validate_identifier(xtsc::u32 index, const std::string& argument_name);


  /**
   * Swizzle the contents of buffer based on starting address (address8) and size
   * (size8) and the bus width defined by "byte_width" in xtsc_mmio_parms.  
   *
   * This method is meant to support peeks and pokes with arbitrary size and alignment
   * from a big-endian core.
   *
   * @param     address8        Starting address.
   *
   * @param     size8           The number of bytes in buffer.
   *
   * @param     buffer          The data to be swizzled.
   *
   * As an example, if "byte_width" is 8 and address8 is 0x10000001 and size8 is 22 then
   * the following 5 swizzles will take place (the bytes in buffer[0] and buffer[21]
   * will be left in place):
   * verbatim
     1.  buffer[1]  through buffer[2]  (2 bytes at nominal addr 0x10000002-0x10000003)
     2.  buffer[3]  through buffer[6]  (4 bytes at nominal addr 0x10000004-0x10000007)
     3.  buffer[7]  through buffer[14] (8 bytes at nominal addr 0x10000008-0x1000000F)
     4.  buffer[15] through buffer[18] (4 bytes at nominal addr 0x10000010-0x10000013)
     5.  buffer[19] through buffer[20] (2 bytes at nominal addr 0x10000014-0x10000015)
     \endverbatim
   */
  void swizzle_buffer(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer);


  /**
   * Get bytes from the register(s) mapped to by the addresses from address8
   * to address8+size8-1 and copy them to buffer.
   */
  void read_bytes(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer);


  /**
   * Write bytes from buffer to the register(s) mapped to by the addresses from address8
   * to address8+size8-1 and then write all sc_port objects that the register(s) map to.
   */
  void write_bytes(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer);


  /// Send and log a response
  void send_response(xtsc::xtsc_response &response);



  /// Implementation of xtsc_request_if.
  class xtsc_request_if_impl : public xtsc::xtsc_request_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_request_if_impl(const char *object_name, xtsc_mmio& mmio) :
      sc_object (object_name),
      m_mmio    (mmio),
      m_p_port  (0)
    {}

    /// The kind of sc_object we are
    const char* kind() const { return "xtsc_mmio::xtsc_request_if_impl"; }

    /**
     *  Receive peeks from the memory interface master
     *  @see xtsc::xtsc_request_if
     */
    void nb_peek(xtsc::xtsc_address address8, xtsc::u32 size8, xtsc::u8 *buffer);

    /**
     *  Receive pokes from the memory interface master
     *  @see xtsc::xtsc_request_if
     */
    void nb_poke(xtsc::xtsc_address address8, xtsc::u32 size8, const xtsc::u8 *buffer);

    /**
     *  Receive fast access requests from the memory interface master
     *  @see xtsc::xtsc_request_if
     */
    bool nb_fast_access(xtsc::xtsc_fast_access_request &request);

    /**
     *  Receive requests from the memory interface master
     *  @see xtsc::xtsc_request_if
     */
    void nb_request(const xtsc::xtsc_request& request);


  protected:

    /// SystemC callback when something binds to us
    void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_mmio&                  m_mmio;         ///< Our xtsc_mmio object
    sc_core::sc_port_base      *m_p_port;       ///< Port that is bound to us
  };



  /**
   * Register definition and value.
   */
  class register_definition {
  public:

    /// Constructor
    register_definition(xtsc_mmio& mmio) : m_mmio(mmio) {}

    /// Dump
    void dump(std::ostream& os = std::cout) const;

    /// Reset the register
    void reset();

    /**
     * Write the outputs of this register.
     *
     * @param   always_write    If true, write all outputs.  If false, only write
     *                          outputs that have changed since the last call to
     *                          write_outputs().
     */
    void write_outputs(bool always_write = true);


    xtsc_mmio&                  m_mmio;                 ///< Our xtsc_mmio object
    std::string                 m_name;                 ///< Register name
    xtsc::xtsc_address          m_address;              ///< Lowest address of register
    xtsc::u32                   m_bit_width;            ///< Number of bits in register
    sc_dt::sc_unsigned         *m_p_initial_value;      ///< Initial value of register
    sc_dt::sc_unsigned         *m_p_previous_value;     ///< Previous value of register
    sc_dt::sc_unsigned         *m_p_current_value;      ///< Current value of register
    output_set                  m_output_set;           ///< Set of output_definition's
    input_set                   m_input_set;            ///< Set of input_definition's
    bool                        m_read_only;            ///< Is the register read only
  };



  /**
   * Output definition and sc_port.
   */
  class output_definition {
  public:

    /// Constructor
    output_definition(xtsc_mmio& mmio) : m_mmio(mmio) {}

    /// Dump
    void dump(std::ostream& os = std::cout) const;


    xtsc_mmio&                  m_mmio;                 ///< Our xtsc_mmio object
    std::string                 m_name;                 ///< Port name
    std::string                 m_reg_name;             ///< Name or our associated register
    xtsc::u32                   m_high_bit;             ///< High bit of register
    xtsc::u32                   m_low_bit;              ///< Low bit of register
    wire_write_port            *m_p_wire_write_port;    ///< sc_port for this output
  };



  /**
   * Input definition and sc_export.
   */
  class input_definition {
  public:

    /// Constructor
    input_definition(xtsc_mmio& mmio) : m_mmio(mmio) {}

    /// Dump
    void dump(std::ostream& os = std::cout) const;

    /// Implementation of xtsc_wire_write_if.
    class xtsc_wire_write_if_impl : public xtsc::xtsc_wire_write_if, public sc_core::sc_object {
    public:

      /// Constructor
      xtsc_wire_write_if_impl(const std::string& port_name, input_definition& definition, xtsc::u32 bit_width) :
        sc_object               (port_name.c_str()),
        m_input_definition      (definition),
        m_name                  (port_name),
        m_bit_width             (bit_width),
        m_p_port                (0)
      {}

      /// The kind of sc_object we are
      const char* kind() const { return "xtsc_mmio::input_definition::xtsc_wire_write_if_impl"; }

      /**
       *  Receive writes from the wire source
       *  @see xtsc::xtsc_wire_write_if
       */
      virtual void nb_write(const sc_dt::sc_unsigned& value);

      /**
       *  Get the wire width in bits.
       *  @see xtsc::xtsc_wire_write_if
       */
      virtual xtsc::u32 nb_get_bit_width();


    protected:

      /// SystemC callback when something binds to us
      void register_port(sc_core::sc_port_base& port, const char *if_typename);

      input_definition&         m_input_definition;             ///< Our input_definition object
      std::string               m_name;                         ///< Our name as a std::string
      xtsc::u32                 m_bit_width;                    ///< Port width in bits
      sc_core::sc_port_base    *m_p_port;                       ///< Port that is bound to us
    };




    xtsc_mmio&                  m_mmio;                         ///< Our xtsc_mmio object
    std::string                 m_name;                         ///< Input port name
    register_definition        *m_p_register_definition;        ///< Our associated register_definition
    xtsc::u32                   m_high_bit;                     ///< High bit of register
    xtsc::u32                   m_low_bit;                      ///< Low bit of register
    xtsc_wire_write_if_impl    *m_p_wire_write_impl;            ///< m_p_wire_write_export binds to this
    wire_write_export          *m_p_wire_write_export;          ///< sc_export for this input
  };



  xtsc_mmio_parms               m_mmio_parms;                   ///< Copy of xtsc_mmio_parms
  xtsc_request_if_impl          m_request_impl;                 ///< m_request_export binds to this
  log4xtensa::TextLogger&       m_text;                         ///< Used for logging 
  sc_core::sc_time              m_clock_period;                 ///< This device's clock period
  bool                          m_use_fast_access;              ///< From the "use_fast_access" parameter
  sc_core::sc_event             m_request_event;                ///< Event used to notify request_thread
  sc_core::sc_event             m_write_event;                  ///< Event notified whenever an nb_request WRITE or nb_write occurs
  sc_core::sc_event             m_poke_event;                   ///< Event notified whenever an nb_poke occurs
  xtsc::xtsc_script_file       *m_p_definition_file;            ///< The script file from the "definition_file" parameter
  std::string                   m_definition_file;              ///< The name of the script file
  std::string                   m_line;                         ///< Current line of script file
  xtsc::u32                     m_line_count;                   ///< Current line number in script file
  std::vector<std::string>      m_words;                        ///< Current line in script file tokenized into words
  address_register_map          m_address_register_map;         ///< Map from address to register definition
  register_definition_map       m_register_definition_map;      ///< Map of register definitions
  output_definition_map         m_output_definition_map;        ///< Map of output definitions
  input_definition_map          m_input_definition_map;         ///< Map of input definitions
  std::set<std::string>         m_input_set;                    ///< Set of names of all inputs
  std::set<std::string>         m_output_set;                   ///< Set of names of all outputs
  std::set<std::string>         m_io_set;                       ///< Set of names of all inputs and outputs
  xtsc::u32                     m_byte_width;                   ///< The byte width of this device's data interface
  xtsc::xtsc_request            m_active_request;               ///< Our copy of the active (current) request
  bool                          m_busy;                         ///< We can only accept one request at a time
  sc_core::sc_time              m_response_time;                ///< How long to take to respond
  bool                          m_always_write;                 ///< Write port even if value hasn't changed
  bool                          m_swizzle_bytes;                ///< Swizzle bytes before writing and after reading
  xtsc::xtsc_port_table         m_port_table_all;               ///< All wire inputs and outputs in "definition_file" order
  xtsc::xtsc_port_table         m_port_table_all_inputs;        ///< All wire inputs             in "definition_file" order
  xtsc::xtsc_port_table         m_port_table_all_outputs;       ///< All wire outputs            in "definition_file" order

  std::vector<sc_core::sc_process_handle>
                                m_process_handles;              ///< For reset 

  friend std::ostream& operator<<(std::ostream& os, const register_definition& reg);
  friend std::ostream& operator<<(std::ostream& os, const output_definition& output);
  friend std::ostream& operator<<(std::ostream& os, const input_definition& input);

};  // class xtsc_mmio 



inline std::ostream& operator<<(std::ostream& os, const xtsc_mmio::register_definition& reg) {
  reg.dump(os);
  return os;
}



inline std::ostream& operator<<(std::ostream& os, const xtsc_mmio::output_definition& output) {
  output.dump(os);
  return os;
}



inline std::ostream& operator<<(std::ostream& os, const xtsc_mmio::input_definition& input) {
  input.dump(os);
  return os;
}



}  // namespace xtsc_component



#endif // _XTSC_MMIO_H_
