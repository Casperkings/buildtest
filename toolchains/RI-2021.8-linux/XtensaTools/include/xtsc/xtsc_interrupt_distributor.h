#ifndef _XTSC_INTERRUPT_DISTRIBUTOR_H_
#define _XTSC_INTERRUPT_DISTRIBUTOR_H_

// Copyright (c) 2007-2012 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Tensilica Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Tensilica Inc.

/**
 * @file 
 */


#include <xtsc/xtsc.h>
#include <xtsc/xtsc_lookup_if.h>
#include <xtsc/xtsc_parms.h>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <vector>







namespace xtsc {

class xtsc_core;


/**
 * Constructor parameters for a xtsc_interrupt_distributor object.
 *
 *  \verbatim
   Name                 Type    Description
   ------------------   ----    --------------------------------------------------------
  

   "syscfgid"           u32     The initial value of the SYSCFGID input.  If desired,
                                the SYSCFGID input can be left unconnected and this
                                value will be used for the whole simulation.
                                Default = 0x00000000.

   "num_interrupts"     u32     The number of external interrupts (bit width of the
                                EXTINT input).

   "num_ports"          u32     The number of connection ports.  Generally, this is the
                                number of xtsc_core instances that will be connected to
                                the xtsc_interrupt_distributor (although it is possible
                                to connect other devices besides xtsc_core instances).

   "allow_bad_address"  bool    If false, bad addresses will result in an exception
                                being thrown.  If true, bad addresses will be logged and
                                ignored.
                                Default = false.

   "clock_period"       u32     This is the length of this device's clock period
                                expressed in terms of the SystemC time resolution (from
                                sc_get_time_resolution()).  A value of 0xFFFFFFFF means
                                to use the XTSC system clock period (from
                                xtsc_get_system_clock_period()).  A value of 0 means one
                                delta cycle.
                                Default = 0xFFFFFFFF (i.e. use the system clock period).

   "posedge_offset"     u32     This specifies the time at which the first posedge of
                                this device's clock conceptually occurs.  It is
                                expressed in units of the SystemC time resolution and
                                the value implied by it must be strictly less than the
                                value implied by the "clock_period" parameter.  A value
                                of 0xFFFFFFFF means to use the same posedge offset as
                                the system clock (from
                                xtsc_get_system_clock_posedge_offset()).
                                Default = 0xFFFFFFFF.

   "sample_phase"       u32     This specifies the phase (i.e. the point) in a clock
                                period at which inputs are "sampled" (i.e. processed).
                                This value is expressed in terms of the SystemC time
                                resolution (from sc_get_time_resolution()) and must be
                                less than the clock period as specified by the
                                "clock_period" parameter.  A value of 0 means inputs are
                                sampled on posedge clock.  A value of 0xFFFFFFFF, the
                                default, means that, if "clock_period" is equal to the
                                XTSC system clock period then inputs will be sampled at
                                phase A, otherwise inputs will be sampled at 1 SystemC
                                time resolution after posedge clock.  See the discussion
                                under xtsc_core::set_clock_phase_delta_factors() of 
                                clock phases.
                                Default = 0xFFFFFFFF.

   "output_delay"       u32     This specifies the delay after "sample_phase" before
                                outputs will be driven.  This value is expressed in
                                terms of the SystemC time resolution (from
                                sc_get_time_resolution()) and must be less than the
                                clock period.  A value of 0 means one delta cycle.
                                Default = 1 (i.e. 1 time resolution).
                                

    \endverbatim
 *
 * @see xtsc_interrupt_distributor
 * @see xtsc_parms
 */
class XTSC_API xtsc_interrupt_distributor_parms : public xtsc_parms {
public:

  /**
   * Constructor for an xtsc_interrupt_distributor_parms object.
   *
   * @param num_interrupts      See the "num_interrupts" parameter
   *
   * @param num_ports           See the "num_ports" parameter
   *
   */
  xtsc_interrupt_distributor_parms(u32 num_interrupts, u32 num_ports) {
    add("syscfgid",             0x00000000);
    add("num_interrupts",       num_interrupts);
    add("num_ports",            num_ports);
    add("allow_bad_address",    false);
    add("clock_period",         0xFFFFFFFF);
    add("posedge_offset",       0xFFFFFFFF);
    add("sample_phase",         0xFFFFFFFF);
    add("output_delay",         1);
  }

  /// Our C++ type (the xtsc_parms base class uses this for error messages)
  virtual const char* kind() const { return "xtsc_interrupt_distributor_parms"; }

};





/**
 * A TLM model of Tensilica's Interrupt Distributor.
 *
 * This device provides a TLM model of Tensilica's Interrupt Distributor.  It can be
 * configured to have one or more external interrupts and to support one or more
 * connected cores.  For testing purposes, other devices besides xtsc_core models can be
 * connected to this device.
 *
 * To perform port binding, use the get_input(), get_output(), and get_lookup() methods
 * to obtain references to the desired port or, if you are connecting one of the XTSC
 * components, simply use the convenience connect() methods.
 *
 * The mapping between port number, C++ name, and SystemC name is shown below for the
 * case of RMPINT.  Ports are numbered between 0 and the value of the "num_ports"
 * parameter minus 1.  This same scheme applies to WMPINT_ADDR, WMPINT_DATA,
 * WMPINT_TOGGLEEN, and PROCINT.  The EXTINT and SYSCFGID ports are not numbered and
 * their SystemC name is the same as their C++ name:
 * \verbatim
        Port #    C++ Name       SystemC Name
        ------    -------------  ------------
        0         *RMPINT[0]     "RMPINT0"
        1         *RMPINT[1]     "RMPINT1"
        2         *RMPINT[2]     "RMPINT2"
        ...
        -          EXTINT        "EXTINT"
        -          SYSCFGID      "SYSCFGID"

   \endverbatim
 *
 * TODO
 * Here is a block diagram of an xtsc_interrupt_distributor as it is used in the example:
 * @image html  Example_xtsc_interrupt_distributor.jpg
 * @image latex Example_xtsc_interrupt_distributor.eps "xtsc_interrupt_distributor Example" width=13cm
 *
 * @see xtsc_interrupt_distributor_parms
 * @see xtsc_core
 * @see xtsc_component::xtsc_mmio
 * @see xtsc_component::xtsc_wire
 * @see xtsc_component::xtsc_wire_source
 *
 */
class XTSC_API xtsc_interrupt_distributor : public sc_core::sc_module, public xtsc_resettable {
public:

  // Shorthand aliases
  typedef sc_core::sc_export<xtsc_lookup_if>              lookup_export;
  typedef sc_core::sc_export<xtsc_wire_write_if>          wire_write_export;
  typedef sc_core::sc_port  <xtsc_wire_write_if, NSPP>    wire_write_port;

  typedef std::vector<lookup_export*>                     lookup_exports;
  typedef std::vector<wire_write_export*>                 wire_write_exports;
  typedef std::vector<wire_write_port*>                   wire_write_ports;

  lookup_exports        RMPINT;                 ///< Connect cores.  To read registers (RER)
  wire_write_export     SYSCFGID;               ///< Connect source of SYSCFGID
  wire_write_export     EXTINT;                 ///< Connect external interrupts
  wire_write_exports    WMPINT_ADDR;            ///< Connect cores.  To write registers (WER)
  wire_write_exports    WMPINT_DATA;            ///< Connect cores.  To write registers (WER)
  wire_write_exports    WMPINT_TOGGLEEN;        ///< Connect cores.  To write registers (WER)
  wire_write_ports      PROCINT;                ///< Connect to core's BInterrupt


  /// This SystemC macro inserts some code required for SC_THREAD's to work
  SC_HAS_PROCESS(xtsc_interrupt_distributor);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_interrupt_distributor"; }


  /**
   * Constructor for a xtsc_interrupt_distributor.
   *
   * @param     module_name                     Name of the xtsc_interrupt_distributor
   *                                            sc_module.
   * @param     interrupt_distributor_parms     The remaining parameters for
   *                                            construction.
   *
   * @see xtsc_interrupt_distributor_parms
   */
  xtsc_interrupt_distributor(sc_core::sc_module_name module_name, const xtsc_interrupt_distributor_parms& interrupt_distributor_parms);


  /// Destructor.
  ~xtsc_interrupt_distributor(void);


  ///
  u32 get_num_ports() { return m_num_ports; }



  /**
   * Connect an xtsc_core to this xtsc_interrupt_distributor.
   *
   * This method connects the "WMPINT_ADDR", "WMPINT_DATA", and "WMPINT_TOGGLEEN" TIE
   * export states and the "RMPINT" TIE lookup of the specified xtsc_core to this
   * xtsc_interrupt_distributor at the specified port index.
   *
   * @param     core            The xtsc_core to connect to.
   *
   * @param     port            The port index of this xtsc_interrupt_distributor to
   *                            connect to.
   *
   * @See xtsc_core::connect() for connecting PROCINT to an xtsc_core BInterrupt input.
   */
  void connect(xtsc_core& core, u32 port);


  /**
   * Connect an xtsc_core to this xtsc_interrupt_distributor.
   *
   * This method connects the specified ouput (TIE export state, system-level output, or
   * TIE lookup) of the specified core to the specified input/lookup of this
   * xtsc_interrupt_distributor.
   *
   * @param     core            The xtsc_core to connect to.
   *
   * @param     output_name     The TIE export state, system-level output, or TIE lookup
   *                            of core to connect to this xtsc_interrupt_distributor.
   *
   * @param     input_name      The input or lookup of this xtsc_interrupt_distributor
   *                            to connect to.
   *
   */
  void connect(xtsc_core& core, const char *output_name, const char *input_name);


  /// Return true if the named input exists
  bool has_input(const char *input_name) const;


  /// Return true if the named lookup exists
  bool has_lookup(const char *lookup_name) const;


  /// Return true if the named output exists
  bool has_output(const char *output_name) const;


  /// Return the bit width of the named input/output
  u32 get_bit_width(const char *io_name) const;


  /// Get address bit width of RMPINT lookups
  u32 get_address_bit_width() { return ADDR_WIDTH; }


  /// Get data bit width of RMPINT lookups
  u32 get_data_bit_width() { return DATA_WIDTH; }


  /**
   * Return the sc_export of the named input.
   *
   * This method can be used for port binding. For example, to bind the TIE export state
   * named "WMPINT_ADDR" of an xtsc_core named core0 to the sc_export input named
   * "WMPINT_ADDR0" (i.e. *WMPINT_ADDR[0]) of an xtsc_interrupt_distributor named
   * distributor:
   * \verbatim
       core0.get_export_state("WMPINT_ADDR")(distributor.get_input("WMPINT_ADDR0"));
     \endverbatim
   *
   * Note:   The connect() methods generally provide the easiest means of performing
   *         port binding.
   */
  sc_core::sc_export<xtsc_wire_write_if>& get_input(const char *input_name) const;


  /**
   * Return the sc_export of the named lookup.
   *
   * This method can be used for port binding. For example, to bind the TIE lookup named
   * "RMPINT" of an xtsc_core named core1 to the sc_export lookup named "RMPINT1" (i.e.
   * *RMPINT[1]) of an xtsc_interrupt_distributor named distributor:
   * \verbatim
       core1.get_lookup("RMPINT")(distributor.get_lookup("RMPINT1"));
     \endverbatim
   *
   * Note:   The connect() methods generally provide the easiest means of performing
   *         port binding.
   */
  sc_core::sc_export<xtsc_lookup_if>& get_lookup(const char *lookup_name) const;


  /**
   * Return the sc_port of the named output.
   *
   * This method can be used for port binding. For example, to bind the sc_port output
   * named "PROCINT2" (i.e. *PROCINT[2]) of an xtsc_interrupt_distributor named
   * distributor to the "BInterrupt" system-level input of an xtsc_core named core2:
   * \verbatim
       distributor.get_output("PROCINT2")(core2.get_input_wire("BInterrupt"));
     \endverbatim
   *
   * Note:   The connect() methods generally provide the easiest means of performing
   *         port binding.
   */
  sc_core::sc_port<xtsc_wire_write_if, NSPP>& get_output(const char *output_name) const;


  /// Get the set of input names defined for this xtsc_interrupt_distributor
  std::set<std::string> get_input_set() const;


  /// Get the set of lookup names defined for this xtsc_interrupt_distributor
  std::set<std::string> get_lookup_set() const;


  /// Get the set of output names defined for this xtsc_interrupt_distributor
  std::set<std::string> get_output_set() const;


  /// Write the initial values to the output ports
  virtual void reset(bool hard_reset = false);


protected:


  /// SystemC callback
  virtual void end_of_elaboration(void);


  /// Thread runs for a few cycles each time there is an input
  void worker_thread();


  /// Method to delay the interrupts going to the processors
  void procint_method();


  /// Method to notify worker_thread that an input has occurred
  void notify(const std::string& export_name, u32 countdown);


  /// Throw exception if address is not a valid register address
  void validate_address(u32 address, const std::string& export_name, const std::string& method_name);


  /// Return the value stored at the given register address
  u32 read_address(u32 address);


  /// POD class to hold information about each sc_export<xtsc_wire_write_if>
  class input_info {
  public:
    input_info(const std::string& name, u32 bit_width, wire_write_export& xport) :
      m_name            (name),
      m_bit_width       (bit_width),
      m_export          (xport)
    {}
    std::string         m_name;
    u32                 m_bit_width;
    wire_write_export&  m_export;
  };


  /// POD class to hold information about each sc_export<xtsc_lookup_if>
  class lookup_info {
  public:
    lookup_info(const std::string& name, u32 address_bit_width, u32 data_bit_width, lookup_export& xport) :
      m_name                    (name),
      m_address_bit_width       (address_bit_width),
      m_data_bit_width          (data_bit_width),
      m_export                  (xport)
    {}
    std::string         m_name;
    u32                 m_address_bit_width;
    u32                 m_data_bit_width;
    lookup_export&      m_export;
  };


  /// POD class to hold information about each sc_port<xtsc_wire_write_if>
  class output_info {
  public:
    output_info(const std::string& name, u32 bit_width, wire_write_port& port) :
      m_name            (name),
      m_bit_width       (bit_width),
      m_port            (port)
    {}
    std::string         m_name;
    u32                 m_bit_width;
    wire_write_port&    m_port;
  };


  /// Base class for handling the sc_export<xtsc_wire_write_if>
  class xtsc_wire_write_if_impl : public xtsc_wire_write_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_wire_write_if_impl(const std::string&          export_name,
                            xtsc_interrupt_distributor& interrupt_distributor,
                            u32                         countdown,
                            u32                         bit_width) :
      sc_object                 ((export_name+"_impl").c_str()),
      m_interrupt_distributor   (interrupt_distributor),
      m_export_name             (export_name),
      m_countdown               (countdown),
      m_bit_width               (bit_width),
      m_value                   (m_bit_width),
      m_p_port                  (0),
      m_text                    (m_interrupt_distributor.m_text)
    {}

    /// The kind of sc_object we are
    virtual const char* kind() const { return "xtsc_interrupt_distributor::xtsc_wire_write_if_impl"; }

    /**
     *  Receive writes from the master
     *  @see xtsc_wire_write_if
     */
    virtual void nb_write(const sc_dt::sc_unsigned& value);

    /**
     *  Get the wire width in bits.
     *  @see xtsc_wire_write_if
     */
    virtual u32 nb_get_bit_width() { return m_bit_width; }


    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_interrupt_distributor& m_interrupt_distributor;        ///< Our xtsc_interrupt_distributor object
    std::string                 m_export_name;                  ///< Our name as a string
    u32                         m_countdown;                    ///< Number of processing cycles a write to this port triggers
    u32                         m_bit_width;                    ///< Port width in bits
    sc_dt::sc_unsigned          m_value;                        ///< Newly written value
    sc_core::sc_port_base      *m_p_port;                       ///< Port that is bound to us
    log4xtensa::TextLogger&     m_text;                         ///< Used for logging 
  };


  /// Implementation for handling WMPINT_ADDR
  class xtsc_wire_write_if_impl_WMPINT_ADDR : public xtsc_wire_write_if_impl {
  public:

    /// Constructor
    xtsc_wire_write_if_impl_WMPINT_ADDR(const std::string&              export_name,
                                        xtsc_interrupt_distributor&     interrupt_distributor,
                                        u32                             countdown,
                                        u32                             bit_width) :
      xtsc_wire_write_if_impl (export_name, interrupt_distributor, countdown, bit_width)
    {}

    /// The kind of sc_object we are
    const char* kind() const { return "xtsc_interrupt_distributor::xtsc_wire_write_if_impl_WMPINT_ADDR"; }

    /**
     *  Receive writes from the master
     *  @see xtsc_wire_write_if
     */
    void nb_write(const sc_dt::sc_unsigned& value);

  };



  /// Implementation of xtsc_lookup_if.
  class xtsc_lookup_if_impl : public xtsc_lookup_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_lookup_if_impl(const std::string& export_name, xtsc_interrupt_distributor& interrupt_distributor, u32 index) :
      sc_object                 ((export_name+"_impl").c_str()),
      m_interrupt_distributor   (interrupt_distributor),
      m_export_name             (export_name),
      m_index                   (index),
      m_value                   (DATA_WIDTH),
      m_p_port                  (0),
      m_text                    (m_interrupt_distributor.m_text)
    {}

    /// The kind of sc_object we are
    const char* kind() const { return "xtsc_interrupt_distributor::xtsc_lookup_if_impl"; }

    /// @see xtsc_lookup_if
    void nb_send_address(const sc_dt::sc_unsigned& address);

    /// @see xtsc_lookup_if
    sc_dt::sc_unsigned nb_get_data();

    /// @see xtsc_lookup_if
    u32 nb_get_address_bit_width() { return ADDR_WIDTH; }

    /// @see xtsc_lookup_if
    u32 nb_get_data_bit_width() { return DATA_WIDTH; }

    /**
     * Get the event that will be notified when the lookup data is available.
     *
     * @see xtsc_lookup_if::default_event()
     */
    virtual const sc_core::sc_event& default_event() const { return m_ready_event; }

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_interrupt_distributor&         m_interrupt_distributor;        ///< Our xtsc_interrupt_distributor object
    std::string                         m_export_name;                  ///< The name of the sc_export that we are an implementation for
    u32                                 m_index;                        ///< Our index
    sc_dt::sc_unsigned                  m_value;                        ///< Current lookup value as sc_unsigned
    std::deque<u32>                     m_value_deque;                  ///< deque of lookup values as u32
    sc_core::sc_port_base              *m_p_port;                       ///< Port that is bound to us
    sc_core::sc_event                   m_ready_event;                  ///< Notified when data will be ready
    log4xtensa::TextLogger&             m_text;                         ///< Used for logging 
  };



  /// Record information about an input
  void record_input(const std::string& name, u32 bit_width, wire_write_export& xport);


  /// Record information about a lookup
  void record_lookup(const std::string& name, u32 address_bit_width, u32 data_bit_width, lookup_export& xport);


  /// Record information about an output
  void record_output(const std::string& name, u32 bit_width, wire_write_port& port);



  typedef std::vector<xtsc_wire_write_if_impl*>                 wire_write_impl_table;
  typedef std::vector<xtsc_wire_write_if_impl_WMPINT_ADDR*>     wire_write_impl_table_WMPINT_ADDR;
  typedef std::vector<xtsc_lookup_if_impl*>                     lookup_impl_table;
  typedef std::map<std::string,input_info*>                     input_map;
  typedef std::map<std::string,lookup_info*>                    lookup_map;
  typedef std::map<std::string,output_info*>                    output_map;


  log4xtensa::TextLogger&               m_text;                         ///< Used for logging 
  xtsc_wire_write_if_impl               m_impl_SYSCFGID;                ///< Implementation  for sc_export SYSCFGID
  xtsc_wire_write_if_impl               m_impl_EXTINT;                  ///< Implementation  for sc_export EXTINT
  wire_write_impl_table_WMPINT_ADDR     m_impl_table_WMPINT_ADDR;       ///< Implementations for sc_export WMPINT_ADDR
  wire_write_impl_table                 m_impl_table_WMPINT_DATA;       ///< Implementations for sc_export WMPINT_DATA
  wire_write_impl_table                 m_impl_table_WMPINT_TOGGLEEN;   ///< Implementations for sc_export WMPINT_TOGGLEEN
  lookup_impl_table                     m_impl_table_RMPINT;            ///< Implementations for sc_export RMPINT
  u32                                   m_num_ports;                    ///< From "num_ports" parameter         = M
  u32                                   m_num_interrupts;               ///< From "num_interrupts" parameter    = N
  u64                                   m_clock_period_value;           ///< Clock period as u64
  bool                                  m_allow_bad_address;            ///< From "allow_bad_address" parameter
  bool                                  m_showed_addresses;             ///< Only print out the list of valid addresses 1 time
  sc_core::sc_time                      m_clock_period;                 ///< This device's clock period
  sc_core::sc_time                      m_sample_phase;                 ///< "sample_phase" as sc_time
  sc_core::sc_time                      m_sample_phase_plus_one;        ///< m_sample_phase + m_clock_period
  sc_core::sc_time                      m_output_delay;                 ///< "output_delay" as sc_time
  sc_core::sc_time                      m_time_resolution;              ///< SystemC time resolution
  bool                                  m_has_posedge_offset;           ///< True if m_posedge_offset is non-zero
  sc_core::sc_time                      m_posedge_offset;               ///< From "posedge_offset" parameter
  xtsc::u64                             m_posedge_offset_value;         ///< m_posedge_offset as u64
  sc_core::sc_event                     m_input_event;                  ///< To notify worker_thread that input has occurred
  sc_core::sc_event                     m_procint_event;                ///< To notify procint_method that PROCINT needs to be driven
  std::set<std::string>                 m_input_set;                    ///< Set of names of all wire_write_export objects
  std::set<std::string>                 m_output_set;                   ///< Set of names of all wire_write_port objects
  std::set<std::string>                 m_lookup_set;                   ///< Set of names of all lookup_export objects
  input_map                             m_input_map;                    ///< Map input  name to its information POD class
  lookup_map                            m_lookup_map;                   ///< Map lookup name to its information POD class
  output_map                            m_output_map;                   ///< Map output name to its information POD class

  bool                                 *m_mieng;                        ///< MIENG registered                   N
  bool                                 *m_miasg;                        ///< MIASG registered                   N
  bool                                **m_mirout;                       ///< MIROUT registered                  NxM
  bool                                **m_mipicause;                    ///< MIPICAUSE registered               Mx16
  u8                                    m_mipipart[4];                  ///< MIPIPART registered                1

  u32                                   m_extint;                       ///< EXTINT value                       1

  bool                                 *m_wmpint_toggleen_last;         ///< WMPINT_TOGGLEEN last cycle         M
  bool                                 *m_wmpint_do;                    ///< WMPINT toggled                     M
  u32                                  *m_rmpint_addr;                  ///< RMPINT address value               M
  bool                                 *m_rmpint_do;                    ///< RMPINT nb_send_address() called    M

  std::vector<sc_dt::sc_unsigned*>      m_procint;                      ///< PROCINT driven value               M
  std::vector<sc_dt::sc_unsigned*>      m_procint_next;                 ///< PROCINT computed value             M
  bool                                 *m_procint_do;                   ///< PROCINT needs to be written        M

  u32                                   m_countdown;                    ///< worker_thread cycle countdown

  static const u32                      ADDR_WIDTH       = 12;
  static const u32                      DATA_WIDTH       = 32;
  static const u32                      MIPI_CAUSE_WIDTH = 16;

  static const u32                      MIROUT_ADDR      = 0x000;
  static const u32                      MIPICAUSE_ADDR   = 0x100;
  static const u32                      MIPISET_ADDR     = 0x140;
  static const u32                      MIENG_ADDR       = 0x180;
  static const u32                      MIENGSET_ADDR    = 0x184;
  static const u32                      MIASG_ADDR       = 0x188;
  static const u32                      MIASGSET_ADDR    = 0x18c;
  static const u32                      MIPIPART_ADDR    = 0x190;
  static const u32                      SYSCFGID_ADDR    = 0x1A0;
  static const u32                      MPSCORE_ADDR     = 0x200;
  static const u32                      CCON_ADDR        = 0x220;

};  // class xtsc_interrupt_distributor




}  // namespace xtsc



#endif // _XTSC_INTERRUPT_DISTRIBUTOR_H_
