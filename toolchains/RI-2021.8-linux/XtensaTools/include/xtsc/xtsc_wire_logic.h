#ifndef _XTSC_WIRE_LOGIC_H_
#define _XTSC_WIRE_LOGIC_H_

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
#include <xtsc/xtsc_parms.h>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <vector>





namespace xtsc {
class xtsc_core;
class xtsc_interrupt_distributor;
class xtsc_tx_loader;
}


namespace xtsc_component {


/**
 * Constructor parameters for a xtsc_wire_logic object.
 *
 *  \verbatim
   Name                 Type    Description
   ------------------   ----    --------------------------------------------------------
  
   "definition_file"    char*   The name of a file providing input definitions, output
                                definitions, and the mapping from inputs to outputs.  
                                Lines in this file must be in one of the following 
                                formats (curly braces indicate optional elements):

         input <InputName> <BitWidth> {<InitialValue>}

         output <OutputName> <BitWidth> {<InitialValue> {<WritePolicy> {<Delay> {clock_period}}}}

         iterator <Iterator> <Start> <Stop> {<StepSize>}

         assign <OutputName> = <RpnExpression>

         assign <OutputName>[<Iterator>] = <RpnExpression>

         assign <OutputName>[<Index>] = <RpnExpression>

         lua_function <OutputName> <LuaFunctionName> <InputName> ...


                                1.  A line beginning with the keyword "input" defines an
                                    sc_export<xtsc_wire_write_if> and its bit width.
                                2.  A line beginning with the keyword "output" defines an
                                    sc_port<xtsc_wire_write_if> and its bit width.
                                3.  A line beginning with the keyword "iterator" defines
                                    an iterator to be used in a subsequent assign line.
                                    Its effect is to replicate any subsequent assign
                                    line that uses the iterator one time for each value
                                    in the iteration sequence.  <Iterator> must be a valid
                                    C/C++ identifier.  Both <Start> and <Stop> must be
                                    in the range of 0 to <BitWidth> minus 1 of any
                                    inputs or outputs to which they are applied.
                                    <StepSize> must be a non-zero integer.  An
                                    <Iterator> can be redefined and the new definition
                                    will apply to subsequent assign lines.
                                    The default <StepSize> is +1 or -1 (depending on
                                    whether <Stop> is greater than or less than <Start>).
                                4.  A line beginning with the keyword "assign" defines
                                    the mapping between all the input bits and one
                                    output bit (after iterator expansion) using a
                                    Reverse Polish Notation (RPN) expression
                                    (<RpnExpression>).  The assign line has three
                                    supported formats (as shown above).  The first format
                                    shown has an implied iterator from 0 to <BitWidth>
                                    minus 1.  If <OutputName> appears in an assign line
                                    then it may not appear in any lua_function line.
                                5.  A line beginning with the keyword "lua_function"
                                    declares that <LuaFunctionName> is the name of a Lua
                                    function defined in a #lua_beg/#lua_end block in the
                                    definition file which takes 1 Lua string argument
                                    for each <InputName> entry on the line and returns
                                    one Lua string value to be assigned to <OutputName>.
                                    The Lua strings for all <InputName> entries will
                                    be in hexadecimal format with ((BitWidth + 3) / 4)
                                    hex nibbles after the leading "0x" (where BitWidth
                                    is from the <BitWidth> entry on the input line).
                                    If <OutputName> appears in a lua_function line then
                                    it may not appear in any assign line.
                                6.  An <RpnExpression> is a space-separated sequence of 
                                    operand tokens and operator tokens.  An operand
                                    token must be in one of the following 3 formats (no
                                    internal spaces are allowed):
                                        <InputName>
                                        <InputName>[<Iterator>]
                                        <InputName>[<Index>]
                                    The first <RpnExpression> format shown here has an
                                    implied iterator from 0 to <BitWidth> minus 1 and it
                                    may only be used with the first assign format shown
                                    above (and <InputName> and <OutputName> must have
                                    the same <BitWidth>).  When <RpnExpression> is
                                    evaluated (logically this occurs each time there is
                                    a write to an input), each occurance of an operand
                                    token causes its current value (0 or 1) to be pushed
                                    onto the operand stack.  Four single-character
                                    operator tokens are supported as shown here with
                                    their English language meaning in quotes followed
                                    by an example RPN expression using it:
                                        !   "NOT"               a !
                                        |   "OR"                a b |
                                        &   "AND"               a b &
                                        ^   "exclusive OR"      a b ^
                                    The ! operator pops one operand (a) off the stack,
                                    then pushes "NOT a" onto the stack.  The | operator
                                    pops 2 operands (a,b) off the stack, computes "a OR b",
                                    then pushes the result onto the stack.  The &
                                    operator pops two operands (a,b) off the stack,
                                    computes "a AND b", then pushes the result onto the
                                    stack.  The ^ operator pops two operands (a,b) off
                                    the stack, computes "a exclusive OR b", then pushes
                                    the result onto the stack.  After the <RpnExpression>
                                    has been evaluated there should only be a single
                                    operand remaining on the stack to assign to the
                                    output.
                                7.  <InputName> and <OutputName> must be valid C/C++
                                    identifiers and must be unique.
                                8.  <BitWidth> must be an integer greater then 0.
                                9.  If the <InitialValue> element contains less bits
                                    then the specified <BitWidth> then the missing
                                    high-order bits are assumed to be 0.
                                    Default = 0.
                                10. An input bit may appear in any number of assign or
                                    lua_function lines or none at all.
                                11. An output bit may appear in zero or one assign line.
                                    An output bit that does not appear in an assign line
                                    and whose <OutputName> does not appear in a
                                    lua_function line gets its constant value from the
                                    <InitialValue> element of its output definition line.
                                12. The definitions for inputs and outputs referenced in
                                    an assign or lua_function line must appear prior to
                                    the assign or lua_function line.
                                13. <WritePolicy> can be one of always|change.  If
                                    <WritePolicy> is always then a write to any input
                                    that this output is dependent on will result in a
                                    write to this output.  If <WritePolicy> is change,
                                    then a write to an input will result in a write to
                                    this output only if the output value changes.  At
                                    start of simulation and at reset all output ports
                                    are written with their initial value regardless of
                                    this parameter.
                                    Default = change.
                                14. <Delay> specifies how long to wait after an input is
                                    written before writing any dependent outputs.  By
                                    default, <Delay> must be an integer and is taken to
                                    be expressed in terms of the SystemC time resolution
                                    (from sc_get_time_resolution()).  However, if the
                                    clock_period qualifier appears after <Delay> then
                                    <Delay> may be an integer or floating point number
                                    that will be multiplied by this device's clock
                                    period (from the "clock_period" parameter).  A
                                    <Delay> of 0 means one delta cycle.  A <Delay> of
                                    now means no delay (nb_write calls to dependent
                                    outputs occur during the nb_write call to the input).
                                    Default = now.
                                15. Comments, extra whitespace, and blank lines are
                                    ignored.   See xtsc_script_file.
                                16. Here are some example "definition_file" contents.
                                    - Bit 0 is the OR and Bit 1 is the AND of 2 inputs
                                       input SigIn0 1
                                       input SigIn1 1
                                       output SigOut 2
                                       assign SigOut[0] = SigIn0[0] SigIn1[0] |
                                       assign SigOut[1] = SigIn0[0] SigIn1[0] &
                                    - Swizzle the 3 bits of an input
                                       input SigIn 3
                                       output SigOut 3
                                       iterator i 0 2
                                       iterator j 2 0
                                       assign SigOut[i] = SigIn[j]
                                    - Delay a 12-bit signal 1 SystemC time resolution
                                       input SigIn 12
                                       output SigOut 12 0 always 1
                                       assign SigOut = SigIn
                                    - Delay a 12-bit signal 5 clock cycles
                                       input SigIn 12
                                       output SigOut 12 0 always 5 clock_period
                                       assign SigOut = SigIn
                                    - Invert all bits of a 32-bit input
                                       input SigIn 32
                                       output SigOut 32 0
                                       assign SigOut = SigIn !
                                    - Replicate 1 bit 2 times (fanout)
                                       input SigIn 1
                                       output SigOut0 1
                                       output SigOut1 1
                                       assign SigOut0 = SigIn
                                       assign SigOut1 = SigIn
                                    - Concatenate 6 bits and 3 bits giving 9 bits
                                       input SigIn1 6
                                       input SigIn2 3
                                       output SigOut 9
                                       iterator i 0 5
                                       assign SigOut[i] = SigIn1[i]
                                       iterator i 6 8
                                       iterator j 0 2
                                       assign SigOut[i] = SigIn2[j]
                                    - 4 bit output is bit-wise AND of two 4-bit inputs
                                       input SigIn1 4
                                       input SigIn2 4
                                       output SigOut 4
                                       assign SigOut = SigIn1 SigIn2 &
                                    - 1 bit output is reduction AND of 4 bit input
                                       input In 4
                                       output Out 1
                                       assign Out = In[0] In[1] In[2] In[3] & & &
                                    - 1 bit output is OR of 4 1-bit inputs
                                      Using assign:
                                       input In0 1
                                       input In1 1
                                       input In2 1
                                       input In3 1
                                       output Out 1
                                       assign Out = In0 In1 In2 In3 | | |
                                      Using lua_function:
                                       #lua_beg
                                         function my_or(arg1, arg2, arg3, arg4)
                                           return ((arg1 == "0x1") or
                                                   (arg2 == "0x1") or
                                                   (arg3 == "0x1") or
                                                   (arg4 == "0x1")) and "0x1" or "0x0"
                                         end
                                       #lua_end
                                       input In0 1
                                       input In1 1
                                       input In2 1
                                       input In3 1
                                       output Out 1
                                       lua_function Out my_or In0 In1 In2 In3

   "clock_period"        u32    This is the length of this device's clock period
                                expressed in terms of the SystemC time resolution (from
                                sc_get_time_resolution()).  A value of 0xFFFFFFFF means
                                to use the XTSC system clock period (from
                                xtsc_get_system_clock_period()).  A value of 0 means one
                                delta cycle.
                                Default = 0xFFFFFFFF (i.e. use the system clock period).

    \endverbatim
 *
 * @see xtsc_wire_logic
 * @see xtsc::xtsc_parms
 */
class XTSC_COMP_API xtsc_wire_logic_parms : public xtsc::xtsc_parms {
public:

  /**
   * Constructor for an xtsc_wire_logic_parms object.
   *
   * @param definition_file     See the "definition_file" parameter
   *
   */
  xtsc_wire_logic_parms(const char *definition_file = "") {
    add("definition_file",      definition_file);
    add("clock_period",         0xFFFFFFFF);
  }

  /// Our C++ type (the xtsc_parms base class uses this for error messages)
  virtual const char* kind() const { return "xtsc_wire_logic_parms"; }

};



// Forward references
class xtsc_mmio;
class xtsc_wire_source;



/**
 * A general-purpose glue logic device for the xtsc::xtsc_wire_write_if.
 *
 * This device supports general-purpose glue and delay logic for the
 * xtsc::xtsc_wire_write_if.  It can have an arbitrary number of inputs and an arbitrary
 * number of outputs and have each bit of each output be an arbitrary function of the
 * input bits.  A file (named by the "definition_file" parameter of
 * xtsc_wire_logic_parms) is used to define the input and output ports and the mapping
 * between them.
 *
 * This device has as many I/O ports as are defined in the definition file.  The input
 * ports (technically, sc_export<xtsc_wire_write_if>) can be written by any device
 * having an sc_port<xtsc_wire_write_if>, for example, a TIE export state of an
 * xtsc::xtsc_core, an xtsc_mmio output, or an xtsc_wire_source.  The output ports
 * (technically, sc_port<xtsc_wire_write_if>) can be connected to any device
 * implementing the xtsc::xtsc_wire_write_if, for example, a system-level input wire of
 * an xtsc::xtsc_core such as "BInterrupt" or "BReset", an xtsc_mmio input, or an
 * xtsc_wire.
 *
 * Because the I/O ports are not known until construction time (when the definition file
 * is processed), they are not named members of the class.  To perform port binding, use
 * the xtsc::xtsc_connect() method or do manual SystemC port binding using the
 * get_input() and get_output() methods to obtain references to the desired ports.
 *
 * Here is a block diagram of an xtsc_wire_logic as it is used in the example:
 * @image html  Example_xtsc_wire_logic.jpg
 * @image latex Example_xtsc_wire_logic.eps "xtsc_wire_logic Example" width=13cm
 *
 * And here is the code to connect using the xtsc::xtsc_connect() method:
 * \verbatim
    xtsc_connect(source, "A", "A", logic);
    xtsc_connect(source, "B", "B", logic);

    xtsc_connect(logic, "not_A",   "wire_write", not_A);
    xtsc_connect(logic, "A_and_B", "wire_write", A_and_B);
    xtsc_connect(logic, "A_or_B",  "wire_write", A_or_B);
    xtsc_connect(logic, "A_xor_B", "wire_write", A_xor_B);
    xtsc_connect(logic, "A0",      "wire_write", A0);
    xtsc_connect(logic, "A1",      "wire_write", A1);
    xtsc_connect(logic, "A_dup1",  "wire_write", A_dup1);
    xtsc_connect(logic, "A_dup2",  "wire_write", A_dup2);
    xtsc_connect(logic, "A_B",     "wire_write", A_B);
   \endverbatim
 *
 * And here is the code to connect using manual SystemC port binding:
 * \verbatim
    source.get_tlm_output("A")(logic.get_input("A"));
    source.get_tlm_output("B")(logic.get_input("B"));

    logic.get_output("not_A")  (not_A);
    logic.get_output("A_and_B")(A_and_B);
    logic.get_output("A_or_B") (A_or_B);
    logic.get_output("A_xor_B")(A_xor_B);
    logic.get_output("A0")     (A0);
    logic.get_output("A1")     (A1);
    logic.get_output("A_dup1") (A_dup1);
    logic.get_output("A_dup2") (A_dup2);
    logic.get_output("A_B")    (A_B);
   \endverbatim
 *
 * @see xtsc_wire_logic_parms
 * @see xtsc::xtsc_core
 * @see xtsc_mmio
 * @see xtsc_wire
 * @see xtsc_wire_source
 * @see xtsc::xtsc_connect()
 * @see xtsc::xtsc_core::How_to_do_port_binding
 * @see xtsc::xtsc_core::get_export_state()
 * @see xtsc::xtsc_core::get_system_input_wire()
 * @see xtsc::xtsc_core::get_system_output_wire()
 */
class XTSC_COMP_API xtsc_wire_logic : public sc_core::sc_module, public xtsc::xtsc_module, public xtsc::xtsc_command_handler_interface  {
public:

  // See get_input() and get_output()


  /// This SystemC macro inserts some code required for SC_THREAD's to work
  SC_HAS_PROCESS(xtsc_wire_logic);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_wire_logic"; }


  /**
   * Constructor for a xtsc_wire_logic.
   *
   * @param     module_name             Name of the xtsc_wire_logic sc_module.
   * @param     wire_logic_parms        The remaining parameters for construction.
   *
   * @see xtsc_wire_logic_parms
   */
  xtsc_wire_logic(sc_core::sc_module_name module_name, const xtsc_wire_logic_parms& wire_logic_parms);


  /// Destructor.
  ~xtsc_wire_logic(void);


  /// For xtsc_connection_interface
  virtual xtsc::u32 get_bit_width(const std::string& port_name, xtsc::u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual xtsc::xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// Handle delayed outputs
  void delay_thread();


  /// Return true if the named input exists
  bool has_input(const char *input_name) const;


  /// Return true if the named output exists
  bool has_output(const char *output_name) const;


  /// Return the bit width of the named input/output
  xtsc::u32 get_bit_width(const char *io_name) const;


  /**
   * Return the sc_export of the named input.
   *
   * This method is used for port binding. For example, to bind the TIE export state
   * named "foo" of an xtsc_core named core0 to the sc_export input named "bar" of an
   * xtsc_wire_logic named logic:
   * \verbatim
       core0.get_export_state("foo")(logic.get_input("bar"));
     \endverbatim
   *
   */
  sc_core::sc_export<xtsc::xtsc_wire_write_if>& get_input(const char *input_name) const;


  /**
   * Return the sc_port of the named output.
   *
   * This method is used for port binding. For example, to bind the sc_port output named
   * "vectors" of an xtsc_wire_logic named logic to the "BInterrupt" system-level input of an
   * xtsc_core named core0:
   * \verbatim
       logic.get_output("vectors")(core0.get_input_wire("BInterrupt"));
     \endverbatim
   */
  sc_core::sc_port<xtsc::xtsc_wire_write_if, NSPP>& get_output(const char *output_name) const;


  /// Get the set of input names defined for this xtsc_wire_logic
  std::set<std::string> get_input_set() const;


  /// Get the set of output names defined for this xtsc_wire_logic
  std::set<std::string> get_output_set() const;


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        change_clock_period <ClockPeriodFactor>
          Call xtsc_wire_logic::change_clock_period(<ClockPeriodFactor>).  Return previous
          <ClockPeriodFactor> for this device.

        reset
          Call xtsc_wire_logic::reset().
      \endverbatim
   */
  virtual void execute(const std::string&               cmd_line,
                       const std::vector<std::string>&  words,
                       const std::vector<std::string>&  words_lc,
                       std::ostream&                    result);


  /**
   * Connect an xtsc_wire_logic input or output to an xtsc_core.
   *
   * This method connects an export state or system-level output wire of an xtsc_core to
   * this xtsc_wire_logic or it connects this xtsc_wire_logic to a system-level input wire of an
   * xtsc_core.
   *
   * @param     core            The xtsc_core to connect to.
   *
   * @param     core_intf_name  The export state or system-level output/input wire of
   *                            the xtsc_core that is to be connected to this 
   *                            xtsc_wire_logic.  For an export state, core_intf_name is
   *                            the name as it appears in the user's TIE code (it must
   *                            NOT begin with the "TIE_" prefix).  For a system-level
   *                            output/input wire, core_intf_name is the name as it
   *                            appears in the Xtensa microprocessor data book.
   *
   * @param     io_name         The output or input of this xtsc_wire_logic to be
   *                            connected to the xtsc_core.  If core_intf_name is an
   *                            xtsc_core export state or a system-level output wire
   *                            then io_name must name an input.  If core_intf_name is
   *                            an xtsc_core system-level input wire then io_name must
   *                            name an output.
   *
   * @see xtsc::xtsc_core::get_output_wire().
   * @see xtsc::xtsc_core::get_input_wire().
   */
  void connect(xtsc::xtsc_core& core, const char *core_intf_name, const char *io_name);


  /**
   * Connect the output of an xtsc_tx_loader to an input of this xtsc_wire_logic.
   *
   * This method connects the named output of the specified xtsc_tx_loader to the named
   * input of this xtsc_wire_logic.
   *
   * @param     loader          The xtsc_tx_loader to be connected.
   *
   * @param     output_name     The output of the xtsc_tx_loader to be connected to this
   *                            xtsc_wire_logic.
   *
   * @param     input_name      The input of this xtsc_wire_logic that the
   *                            xtsc_tx_loader is to be connected to.
   */
  void connect(xtsc::xtsc_tx_loader& loader, const char *output_name, const char *input_name);


  /**
   * Connect the output of another xtsc_wire_logic to an input of this xtsc_wire_logic.
   *
   * This method connects the named output of the specified xtsc_wire_logic to the named
   * input of this xtsc_wire_logic.
   *
   * @param     logic           The other xtsc_wire_logic to be connected.
   *
   * @param     output_name     The output of the other xtsc_wire_logic to be connected
   *                            to this xtsc_wire_logic.
   *
   * @param     input_name      The input of this xtsc_wire_logic that the
   *                            xtsc_wire_logic is to be connected to.
   */
  void connect(xtsc_wire_logic& logic, const char *output_name, const char *input_name);


#ifndef DOXYGEN_SKIP
  /**
   * Connect an xtsc_interrupt_distributor to this xtsc_wire_logic.
   *
   * This method connects the specified output/input of the specified
   * xtsc_interrupt_distributor to the specified input/output of this xtsc_wire_logic.
   *
   * @param     distributor             The xtsc_interrupt_distributor to be connected.
   *
   * @param     distributor_io_name     The output/input of the distributor to be
   *                                    connected to this xtsc_wire_logic.  Whether this
   *                                    names an output or an input will be determined
   *                                    by querying distributor.
   *
   * @param     logic_io_name           The input/output of this xtsc_wire_logic to be
   *                                    connected to distributor.
   */
  void connect(xtsc::xtsc_interrupt_distributor& distributor, const char *distributor_io_name, const char *logic_io_name);
#endif // DOXYGEN_SKIP


  /**
   * Connect an xtsc_mmio output to an input of this xtsc_wire_logic.
   *
   * This method connects the named output of the specified xtsc_mmio to the named input
   * of this xtsc_wire_logic.
   *
   * @param     mmio            The xtsc_mmio to be connected.
   *
   * @param     output_name     The output of the xtsc_mmio to be connected to this 
   *                            xtsc_wire_logic.
   *
   * @param     input_name      The input of this xtsc_wire_logic that the xtsc_mmio is
   *                            to be connected to.
   */
  void connect(xtsc_mmio& mmio, const char *output_name, const char *input_name);


  /**
   * Connect an xtsc_wire_source output to an input of this xtsc_wire_logic.
   *
   * This method connects the named output of the specified xtsc_wire_source to the
   * named input of this xtsc_wire_logic.
   *
   * @param     source          The xtsc_wire_source to connect to this xtsc_wire_logic.
   *
   * @param     output_name     The output of the xtsc_wire_source to be connected to
   *                            this xtsc_wire_logic.  If this parameter is NULL or
   *                            empty then the default (first/only) output of source
   *                            will be connected.
   *
   * @param     input_name      The input of this xtsc_wire_logic that the
   *                            xtsc_wire_source is to be connected to.
   */
  void connect(xtsc_wire_source& source, const char *output_name, const char *input_name);


  /// Write the initial values to the output ports
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


  /// Handle input line
  void handle_input(const std::string& input_name, xtsc::u32 bit_width, const std::string& initial_value);


  /// Handle output line
  void handle_output(const std::string& output_name, xtsc::u32 bit_width, const std::string& initial_value);


  /// Handle iterator line
  void handle_iterator();


  /// Handle assign line
  void handle_assign();


  /// Handle lua_function line
  void handle_lua_function();


  /**
   * Parse operand at m_words[index] (this could also be the LHS of the assign).
   *
   * @param     index           Index into m_words.
   *
   * @param     io_name         A reference in which to return the input/output name
   *                            found in the operand
   *
   * @param     is_iterator     A reference in which to return whether operand is an
   *                            iterator.
   *
   * @param     index_value     A reference in which to return the iterator index or the
   *                            input/output index.
   *
   * @return true if operand has an iterator or an input/output index.
   */
  bool parse_operand(xtsc::u32 index, std::string& io_name, bool& is_iterator, xtsc::u32& index_value);


  // Forward declaration
  class output_definition;
  class input_definition;
  class iterator_definition;

  // Shorthand aliases
  typedef std::vector<xtsc::u32>                                rpn_assignment;
  typedef std::vector<rpn_assignment*>                          assignment_table;
  typedef std::vector<output_definition*>                       output_definition_vector;
  typedef std::map<std::string, output_definition*>             output_definition_map;
  typedef std::vector<input_definition*>                        input_definition_vector;
  typedef std::map<std::string, input_definition*>              input_definition_map;
  typedef std::map<std::string, iterator_definition*>           iterator_definition_map;
  typedef std::vector<iterator_definition*>                     iterator_definition_vector;
  typedef std::set<output_definition*>                          output_set;
  typedef std::set<input_definition*>                           input_set;
  typedef sc_core::sc_port<xtsc::xtsc_wire_write_if, NSPP>      wire_write_port;
  typedef sc_core::sc_export<xtsc::xtsc_wire_write_if>          wire_write_export;


  /// SystemC callback
  virtual void end_of_elaboration(void);


  /// Extract a u32 value (named argument_name) from the word at m_words[index]
  xtsc::u32 get_u32(xtsc::u32 index, const std::string& argument_name);


  /// Extract a i32 value (named argument_name) from the word at m_words[index]
  xtsc::i32 get_i32(xtsc::u32 index, const std::string& argument_name);


  /// Extract a double value (named argument_name) from the word at m_words[index]
  double get_double(xtsc::u32 index, const std::string& argument_name);


  /**
   * Throw an exception if m_words[index] (named argument_name) does not exists or is
   * not a valid C/C++ identifier, otherwise return the m_words[index].
   */
  const std::string& validate_identifier(xtsc::u32 index, const std::string& argument_name);


  /// Return whether or not name is a valid identifier
  bool is_identifier(const std::string& name);



  /**
   * Information about a delayed output value
   */
  class output_info {
  public:
    output_info(xtsc::u32 bit_width) : m_value(bit_width) {}
    sc_dt::sc_unsigned          m_value;                ///< The value to be output
    sc_core::sc_time            m_output_time;          ///< The time to output it
    xtsc::u64                   m_delta_cycle;          ///< The delta cycle when output was queued
  };



  /**
   * Output definition and sc_port.
   */
  class output_definition {
  public:

    /// Constructor
    output_definition(xtsc_wire_logic&          logic,
                      const std::string&        name,
                      xtsc::u32                 index,
                      xtsc::u32                 bit_width,
                      const std::string&        initial_value,
                      bool                      always_write,
                      bool                      delay_output,
                      sc_core::sc_time          delay_time);

    /// Reset and drive the output
    void reset() { 
      m_value = m_initial_value.c_str();
      (*m_p_wire_write_port)->nb_write(m_value);

      // Cancel notified events
      m_event.cancel();

      while (!m_output_info_deque.empty()) {
        output_info *p_output_info = m_output_info_deque.front();
        delete_output_info(p_output_info);
        m_output_info_deque.pop_front();
      }

    }

    /// Dump
    void dump(std::ostream& os = std::cout) const;

    /// Get a new output_info object from the pool
    output_info *new_output_info() {
      if (m_output_info_pool.empty()) {
        return new output_info(m_bit_width);
      }
      else {
        output_info *p_output_info = m_output_info_pool.back();
        m_output_info_pool.pop_back();
        return p_output_info;
      }
    }

    void delete_output_info(output_info*& p_output_info) {
      m_output_info_pool.push_back(p_output_info);
      p_output_info = 0;
    }


    xtsc_wire_logic&            m_logic;                ///< Our xtsc_wire_logic object
    std::string                 m_name;                 ///< Port name
    xtsc::u32                   m_index;                ///< Our index in xtsc_wire_logic::m_outputs
    xtsc::u32                   m_bit_width;            ///< Port width in bits
    sc_dt::sc_unsigned          m_value;                ///< Latest computed value of output
    sc_dt::sc_unsigned          m_value_prev;           ///< Previous computed value of output
    sc_dt::sc_unsigned          m_bit_assigned;         ///< bit is 1 if that bit position has an assign statement in definition_file
    std::string                 m_initial_value;        ///< From <InitialValue> in definition_file
    std::string                 m_lua_function_name;    ///< From <LuaFunctionName> of lua_function line in definition_file
    std::vector<xtsc::u32>      m_lua_function_inputs;  ///< From <InputName> entries of lua_function line in definition_file
    bool                        m_lua_function;         ///< True if this output appears in a lua_function statement in definition_file
    bool                        m_assign;               ///< True if this output appears in an assign statement in definition_file
    bool                        m_always_write;         ///< Write port even if value hasn't changed
    bool                        m_delay_output;         ///< True if output is delayed
    sc_core::sc_time            m_delay_time;           ///< Amount of delay
    sc_core::sc_event           m_event;                ///< Event to notify for delayed output
    std::vector<output_info*>   m_output_info_pool;     ///< Maintain a pool of output_info objects to improve performance
    std::deque<output_info*>    m_output_info_deque;    ///< The deque of output_info objects waiting to be output
    wire_write_port            *m_p_wire_write_port;    ///< sc_port for this output
  };



  /**
   * Input definition and sc_export.
   */
  class input_definition {
  public:

    /// Constructor
    input_definition(xtsc_wire_logic&   logic,
                     const std::string& name,
                     xtsc::u32          index,
                     xtsc::u32          bit_width,
                     const std::string& initial_value);

    void reset();

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
        m_p_port                (0),
        m_text                  (definition.m_logic.m_text)
      {}

      /// The kind of sc_object we are
      const char* kind() const { return "xtsc_wire_logic::input_definition::xtsc_wire_write_if_impl"; }

      /**
       *  Receive writes from the master
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

      input_definition&         m_input_definition;     ///< Our input_definition object
      std::string               m_name;                 ///< Our name as a std::string
      xtsc::u32                 m_bit_width;            ///< Port width in bits
      sc_core::sc_port_base    *m_p_port;               ///< Port that is bound to us
      log4xtensa::TextLogger&   m_text;                 ///< Used for logging 
    };


    xtsc_wire_logic&            m_logic;                ///< Our xtsc_wire_logic object
    std::string                 m_name;                 ///< Input port name
    xtsc::u32                   m_index;                ///< Our index in xtsc_wire_logic::m_inputs
    xtsc::u32                   m_bit_width;            ///< Port width in bits
    sc_dt::sc_unsigned          m_value;                ///< Latest received value of input
    std::string                 m_initial_value;        ///< From <InitialValue> in definition_file
    bool                        m_detect_value_change;  ///< True if any dependent output has a <WritePolicy> of change
    assignment_table            m_assignments;          ///< Vector of RPN assignments; 1 for each output bit touched by this input
    output_set                  m_lua_function_outputs; ///< Set of all lua_function outputs that depend on this input
    output_set                  m_outputs;              ///< Set of all outputs that depend on this input
    xtsc_wire_write_if_impl    *m_p_wire_write_impl;    ///< m_p_wire_write_export binds to this
    wire_write_export          *m_p_wire_write_export;  ///< sc_export for this input
  };



  /**
   * Iterator definition 
   */
  class iterator_definition {
  public:

    /// Constructor
    iterator_definition(std::string& name, xtsc::u32 index, xtsc::u32 start, xtsc::u32 stop, xtsc::i32 step) :
      m_name    (name),
      m_index   (index),
      m_start   (start),
      m_stop    (stop),
      m_step    (step)
    {
      init();
    }

    /// Dump
    void dump(std::ostream& os = std::cout) const;

    /// Return the number of values the iterator ranges over
    xtsc::u32 range() { return ((m_step > 0) ? (((m_stop - m_start) / +m_step) + 1) :
                                               (((m_start - m_stop) / -m_step) + 1)); }

    /// Initialize the iterator to its starting value
    void init() { m_value = m_start; }

    /// Return the iterators current value
    xtsc::u32 value() { return m_value; }

    /// Step the iterator to its next value
    void step() { m_value += m_step; }

    std::string         m_name;         ///< Iterator name
    xtsc::u32           m_index;        ///< Index in 
    xtsc::u32           m_start;        ///< First value iterator is to assume
    xtsc::u32           m_stop;         ///< Iterator limit
    xtsc::i32           m_step;         ///< Step size (can be negative)
    xtsc::u32           m_value;        ///< The current value of the iterator
  };



  log4xtensa::TextLogger&       m_text;                         ///< Used for logging 
  sc_core::sc_time              m_clock_period;                 ///< This device's clock period
  xtsc::xtsc_script_file       *m_p_definition_file;            ///< The script file from "definition_file" parameter
  std::string                   m_definition_file;              ///< The name of the script file from the "definition_file" parameter
  std::string                   m_line;                         ///< Current line of script file
  xtsc::u32                     m_line_count;                   ///< Current line number in script file
  std::vector<std::string>      m_words;                        ///< Current line in script file tokenized into words
  output_definition_vector      m_outputs;                      ///< Vector of all output definitions
  output_definition_vector      m_delayed_outputs;              ///< Vector of output definitions for delayed outputs
  output_definition_map         m_output_definition_map;        ///< Map of output definitions
  input_definition_vector       m_inputs;                       ///< Vector of all input definitions
  input_definition_map          m_input_definition_map;         ///< Map of input definitions
  iterator_definition_map       m_iterator_definition_map;      ///< Map of iterator definitions
  iterator_definition_vector    m_iterators;                    ///< Vector of all iterator definitions
  std::set<std::string>         m_input_set;                    ///< Set of names of all inputs
  std::set<std::string>         m_output_set;                   ///< Set of names of all outputs
  std::set<std::string>         m_io_set;                       ///< Set of names of all inputs and outputs
  std::vector<bool>             m_stack;                        ///< Stack of operand values
  xtsc::u32                     m_max_depth;                    ///< Maximum stack depth needed
  assignment_table              m_assignments;                  ///< List of all defined RPN assignments
  xtsc::u32                     m_next_delay_thread_index;      ///< Used by delay_thread upon entry to get its output
  xtsc::xtsc_port_table         m_port_table_all;               ///< All wire inputs and outputs
  xtsc::xtsc_port_table         m_port_table_all_inputs;        ///< All wire inputs
  xtsc::xtsc_port_table         m_port_table_all_outputs;       ///< All wire outputs

  std::vector<sc_core::sc_process_handle>
                                m_process_handles;              ///< For reset 

  friend std::ostream& operator<<(std::ostream& os, const output_definition& output);
  friend std::ostream& operator<<(std::ostream& os, const input_definition& input);
  friend std::ostream& operator<<(std::ostream& os, const iterator_definition& output);

};  // class xtsc_wire_logic



inline std::ostream& operator<<(std::ostream& os, const xtsc_wire_logic::output_definition& output) {
  output.dump(os);
  return os;
}



inline std::ostream& operator<<(std::ostream& os, const xtsc_wire_logic::input_definition& input) {
  input.dump(os);
  return os;
}



inline std::ostream& operator<<(std::ostream& os, const xtsc_wire_logic::iterator_definition& iterator) {
  iterator.dump(os);
  return os;
}



}  // namespace xtsc_component



#endif // _XTSC_WIRE_LOGIC_H_
