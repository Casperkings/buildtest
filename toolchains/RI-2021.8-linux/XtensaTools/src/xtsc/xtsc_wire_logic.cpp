// Copyright (c) 2007-2012 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Tensilica Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Tensilica Inc.

#include <iostream>
#include <algorithm>
#include <xtsc/xtsc_wire_logic.h>
#include <xtsc/xtsc_wire_source.h>
#include <xtsc/xtsc_mmio.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_interrupt_distributor.h>
#include <xtsc/xtsc_tx_loader.h>


using namespace std;
#if SYSTEMC_VERSION >= 20050601
using namespace sc_core;
#endif
using namespace sc_dt;
using namespace log4xtensa;
using namespace xtsc;



/*
 * Theory of operation:
 *
 * The user defines the inputs and outputs and mappings between them using the "definition_file".
 * Mappings are defined using reverse polish notation (RPN) which is also sometimes called 
 * postfix notation.  In this notation, the operator comes after the operands (not between them).
 * So, for example, "a OR b" is written:   a b |
 *
 * Information about each input is stored in an input_definition object.  Amongst other things
 * this object has a list of all RPN assignments (m_assignments) and lua_function outputs
 * (m_lua_function_outputs) that must be recalculated when the input is written and a list of
 * all outputs (m_outputs) that may need to be written when the input is written.  When the
 * nb_write() method is called for an input, its RPN assignments are evaluated, then Lua
 * function calls are made, and then the nb_write() method is called for any outputs that
 * were touched (<WritePolicy> of 'always') or that changed (<WritePolicy> of 'change').  The
 * call to an output port nb_write() method will occur in the context of the input port
 * nb_write() method if the output's <Delay> was 'now', otherwise the call will be handed off
 * to delay_thread().  One instance of delay_thread() is spawned for each output that has a
 * <Delay> other then 'now'.
 *
 * Information about each output is stored in an output_definition object.
 *
 * An rpn_assignment is a vector<u32> whose entries encode the RPN expression for each output
 * bit from an assign line.  The first two entries encode the output and output bit and each 
 * subsequent entry is an op code (unless the preceding op code was op_PUSH in which case the
 * 2 entries after op_PUSH are not op codes but instead encode the input and input bit to
 * push onto the stack).  For example:
 *    [0] - output (index into m_outputs)
 *    [1] - output_bit_index
 *    [2] - op code other than op_PUSH
 *    [3] - op code other than op_PUSH
 *    [4] - op code op_PUSH
 *    [5] - input (index into m_inputs)
 *    [6] - input_bit_index
 *    [7] - op code other than op_PUSH
 *    ...
 */


static const u32 op_PUSH = 0;   ///< RPN assignment op-code to push an input bit onto stack
static const u32 op_NOT  = 1;   ///< RPN assignment op-code to NOT the top bit on stack      ("!")
static const u32 op_AND  = 2;   ///< RPN assignment op-code to AND the top two bits on stack ("&")
static const u32 op_OR   = 3;   ///< RPN assignment op-code to OR  the top two bits on stack ("|")
static const u32 op_XOR  = 4;   ///< RPN assignment op-code to XOR the top two bits on stack ("^")





xtsc_component::xtsc_wire_logic::xtsc_wire_logic(sc_module_name module_name, const xtsc_wire_logic_parms& logic_parms) :
  sc_module                     (module_name),
  xtsc_module                   (*(sc_module*)this),
  m_text                        (TextLogger::getInstance(name())),
  m_p_definition_file           (new xtsc_script_file(logic_parms.get_non_empty_c_str("definition_file"), "definition_file",  name(), kind(), false)),
  m_definition_file             (m_p_definition_file->name()),
  m_next_delay_thread_index     (0)
{

  m_max_depth           = 0;

  // Get clock period 
  u32 clock_period = logic_parms.get_u32("clock_period");
  if (clock_period == 0xFFFFFFFF) {
    m_clock_period = xtsc_get_system_clock_period();
  }
  else {
    m_clock_period = sc_get_time_resolution() * clock_period;
  }

  // Process script file to get sc_port and sc_export definitions
  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, "Reading xtsc_wire_logic port definitions from file '" << m_definition_file << "'.");
  while ((m_line_count = m_p_definition_file->get_words(m_words, m_line, false))) {
    transform(m_words[0].begin(), m_words[0].end(), m_words[0].begin(), ::tolower);
    if ((m_words[0] == "input") || (m_words[0] == "output")) {
      bool input = (m_words[0] == "input");
      string io_name  = validate_identifier(1, (input ? "<InputName>" : "<OutputName>"));
      if (m_io_set.find(io_name) != m_io_set.end()) {
        ostringstream oss;
        oss << "Duplicate input/output name ('" << io_name << "') found:" << endl;
        oss << m_line;
        oss << m_p_definition_file->info_for_exception();
        throw xtsc_exception(oss.str());
      }
      u32 bit_width = get_u32(2, "<BitWidth>");
      if (bit_width == 0) {
        ostringstream oss;
        oss << "<BitWidth> cannot be zero in line:" << endl;
        oss << m_line;
        oss << m_p_definition_file->info_for_exception();
        throw xtsc_exception(oss.str());
      }
      string initial_value("0");
      if (m_words.size() >= 4) {
        initial_value = m_words[3];
      }
      // Prevent sign extension (we want 0 fill, not sign extension)
      if ((initial_value.length() > 2) && (initial_value[0] == '0') && ((initial_value[1] == 'x') || (initial_value[1] == 'X'))) {
        initial_value.insert(2, "0");
      }
      if (input) {
        handle_input(io_name, bit_width, initial_value);
      }
      else {
        handle_output(io_name, bit_width, initial_value);
      }
    }
    else if (m_words[0] == "iterator") {
      handle_iterator();
    }
    else if (m_words[0] == "assign") {
      handle_assign();
    }
    else if (m_words[0] == "lua_function") {
      handle_lua_function();
    }
    else {
      ostringstream oss;
      oss << "Syntax error (first word must be input|output|iterator|assign):" << endl;
      oss << m_line;
      oss << m_p_definition_file->info_for_exception();
      throw xtsc_exception(oss.str());
    }
  }

  // Size stack
  m_stack.resize(m_max_depth);

  for (input_definition_vector::const_iterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {
    input_definition& input = **i;
    for (output_set::const_iterator j = input.m_outputs.begin(); j != input.m_outputs.end(); ++j) {
      output_definition& output = **j;
      if (!output.m_always_write) {
        input.m_detect_value_change = true;
        break;
      }
    }
  }

  for (output_definition_vector::iterator i = m_delayed_outputs.begin(); i != m_delayed_outputs.end(); ++i) {
    ostringstream oss;
    oss << "delay_" << ((*i)->m_name) << "_thread";
    declare_thread_process(delay_thread_handle, oss.str().c_str(), SC_CURRENT_USER_MODULE, delay_thread);
    m_process_handles.push_back(sc_get_current_process_handle());
  }

  m_port_table_all.push_back("all_inputs" );
  m_port_table_all.push_back("all_outputs");

  if (m_port_types.find("all"        ) == m_port_types.end()) { m_port_types["all"        ] = PORT_TABLE; }
  if (m_port_types.find("all_inputs" ) == m_port_types.end()) { m_port_types["all_inputs" ] = PORT_TABLE; }
  if (m_port_types.find("all_outputs") == m_port_types.end()) { m_port_types["all_outputs"] = PORT_TABLE; }

  xtsc_register_command(*this, *this, "change_clock_period", 1, 1,
      "change_clock_period <ClockPeriodFactor>", 
      "Call xtsc_wire_logic::change_clock_period(<ClockPeriodFactor>).  Return previous <ClockPeriodFactor> for this device."
  );

  xtsc_register_command(*this, *this, "reset", 0, 1,
      "reset", 
      "Call xtsc_wire_logic::reset()."
  );

  // Log our construction
  XTSC_LOG(m_text, ll, "Constructed " << kind() << " '" << name() << "':");
  XTSC_LOG(m_text, ll, " definition_file         = "                 << m_definition_file);
  if (clock_period == 0xFFFFFFFF) {
  XTSC_LOG(m_text, ll, " clock_period            = 0x" << hex        << clock_period << " (" << m_clock_period << ")");
  } else {
  XTSC_LOG(m_text, ll, " clock_period            = "                 << clock_period << " (" << m_clock_period << ")");
  }
}



xtsc_component::xtsc_wire_logic::~xtsc_wire_logic(void) {
  // Do any required clean-up here
}



xtsc::u32 xtsc_component::xtsc_wire_logic::get_bit_width(const string& port_name, u32 interface_num) const {
  return get_bit_width(port_name.c_str());
}



sc_object *xtsc_component::xtsc_wire_logic::get_port(const string& port_name) {

  if (has_output(port_name.c_str())) {
    return &get_output(port_name.c_str());
  }

  if (has_input(port_name.c_str())) {
    return &get_input(port_name.c_str());
  }

  ostringstream oss;
  oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
  throw xtsc_exception(oss.str());
}



xtsc_port_table xtsc_component::xtsc_wire_logic::get_port_table(const string& port_table_name) const {
  if (port_table_name == "all"        ) return m_port_table_all;
  if (port_table_name == "all_inputs" ) return m_port_table_all_inputs;
  if (port_table_name == "all_outputs") return m_port_table_all_outputs;

  ostringstream oss;
  oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
  throw xtsc_exception(oss.str());
}



void xtsc_component::xtsc_wire_logic::change_clock_period(u32 clock_period_factor) {
  m_clock_period = sc_get_time_resolution() * clock_period_factor;
  XTSC_INFO(m_text, "change_clock_period(" << clock_period_factor << ") => " << m_clock_period);
}



void xtsc_component::xtsc_wire_logic::delay_thread() {
  // Get the m_delay_outputs index for this "instance" of delay_thread
  u32 index = m_next_delay_thread_index++;

  output_definition& output = *m_delayed_outputs[index];

  XTSC_DEBUG(m_text, "Starting delay_thread[" << index << "] for output '" << output << "'");

  try {

    while (true) {
      wait(output.m_event);
      while (!output.m_output_info_deque.empty()) {
        output_info *p_output_info = output.m_output_info_deque.front();
        sc_time now = sc_time_stamp();
        if ((p_output_info->m_output_time > now) || (p_output_info->m_delta_cycle == sc_delta_count())) {
          output.m_event.notify(p_output_info->m_output_time - now);
          break;
        }
        // A variable substr is used to get rid of the pesky extra leading zero bit
        XTSC_INFO(m_text, output.m_name << " => 0x" << p_output_info->m_value.to_string(SC_HEX).substr(p_output_info->m_value.length()%4 ? 2 : 3));
        (*output.m_p_wire_write_port)->nb_write(p_output_info->m_value);
        output.delete_output_info(p_output_info);
        output.m_output_info_deque.pop_front();
      }
    }

  }
  catch (const sc_unwind_exception& error) { throw; }
  catch (const exception& error) {
    ostringstream oss;
    oss << "std::exception caught in output '" << output.m_name << "' for delay_thread[" << index << "] of " << kind() << " '"
        << name() << "'." << endl;
    oss << "what(): " << error.what() << endl;
    xtsc_log_multiline(m_text, log4xtensa::FATAL_LOG_LEVEL, oss.str(), 2);
    throw;
  }
}



void xtsc_component::xtsc_wire_logic::handle_input(const string& input_name, u32 bit_width, const string& initial_value) {
  input_definition *p_input         = new input_definition(*this, input_name, m_inputs.size(), bit_width, initial_value);
  p_input->m_p_wire_write_export    = new wire_write_export(input_name.c_str());
  p_input->m_p_wire_write_impl      = new input_definition::xtsc_wire_write_if_impl(input_name + "__impl", *p_input, bit_width);
  (*p_input->m_p_wire_write_export)(*p_input->m_p_wire_write_impl);  // Bind the sc_export to the implementation
  m_input_definition_map.insert(map<string, input_definition*>::value_type(p_input->m_name, p_input));
  m_input_set.insert(p_input->m_name);
  m_io_set.insert(p_input->m_name);
  m_inputs.push_back(p_input);
  m_port_types[p_input->m_name] = WIRE_WRITE_EXPORT; 
  m_port_table_all_inputs.push_back(p_input->m_name);
  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, " input (sc_export): " << *p_input);
}



//   1          2          3             4               5           6          7     <= size
//   0          1          2             3               4           5          6     <= index
// output <OutputName> <BitWidth> {<InitialValue> {<WritePolicy> {<Delay> {clock_period}}}}
void xtsc_component::xtsc_wire_logic::handle_output(const string& output_name, u32 bit_width, const string& initial_value) {
  bool always_write = false;
  bool delay_output = false;
  sc_time delay_time(SC_ZERO_TIME);
  if (m_words.size() > 4) {
    if ((m_words[4] != "always") && (m_words[4] != "change")) {
      ostringstream oss;
      oss << "<WritePolicy> must be one of always|change at word #5:" << endl;
      oss << m_line;
      oss << m_p_definition_file->info_for_exception();
      throw xtsc_exception(oss.str());
    }
    always_write = (m_words[4] == "always");
    if (m_words.size() > 5) {
      bool use_clock_period = (m_words.size() > 6) ? true : false;
      if (use_clock_period) {
        if (m_words[6] != "clock_period") {
          ostringstream oss;
          oss << "Word #7, if present, must be \"clock_period\":" << endl;
          oss << m_line;
          oss << m_p_definition_file->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        if (m_words.size() > 7) {
          ostringstream oss;
          oss << "Extra words found in output line:" << endl;
          oss << m_line;
          oss << m_p_definition_file->info_for_exception();
          throw xtsc_exception(oss.str());
        }
      }
      if (m_words[5] != "now") {
        delay_output = true;
        if (use_clock_period) {
          double delay = get_double(5, "<Delay>");
          delay_time = delay * m_clock_period;
        }
        else {
          u32 delay = get_u32(5, "<Delay>");
          delay_time = delay * sc_get_time_resolution();
        }
      }
    }
  }
  output_definition *p_output = new output_definition(*this, output_name, m_outputs.size(), bit_width, initial_value,
                                                      always_write, delay_output, delay_time);
  m_output_definition_map.insert(map<string, output_definition*>::value_type(p_output->m_name, p_output));
  m_output_set.insert(p_output->m_name);
  m_io_set.insert(p_output->m_name);
  m_outputs.push_back(p_output);
  m_port_types[p_output->m_name] = WIRE_WRITE_PORT; 
  m_port_table_all_outputs.push_back(p_output->m_name);
  if (delay_output) {
    m_delayed_outputs.push_back(p_output);
  }
  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, " output (sc_port):  " << *p_output);
}



void xtsc_component::xtsc_wire_logic::handle_iterator() {
  string iterator   = validate_identifier(1, "<Iterator>");
  u32    start      = get_u32(2, "<Start>");
  u32    stop       = get_u32(3, "<Stop>");
  i32    step       = ((stop > start) ? +1 : -1);
  if (m_words.size() > 4) {
    step = get_i32(4, "<StepSize>");
  }
  if (!step) {
    ostringstream oss;
    oss << "<StepSize> cannot be 0:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  if (((start < stop) && (step < 0)) || ((start > stop) && (step > 0))) {
    ostringstream oss;
    oss << "Invalid <Start>, <Stop>, and <StepSize> combination (infinite loop):" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  iterator_definition *p_iterator   = new iterator_definition(iterator, m_iterators.size(), start, stop, step);
  iterator_definition_map::iterator i = m_iterator_definition_map.find(iterator);
  if (i != m_iterator_definition_map.end()) {
    m_iterator_definition_map.erase(i);
  }
  m_iterator_definition_map.insert(map<string, iterator_definition*>::value_type(p_iterator->m_name, p_iterator));
  m_iterators.push_back(p_iterator);
  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll, " iterator:  " << *p_iterator);
}



void xtsc_component::xtsc_wire_logic::handle_assign() {
  if (m_words.size() < 4) {
    ostringstream oss;
    oss << "assign statement has too few words:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  if (m_words[2] != "=") {
    ostringstream oss;
    oss << "Expected equal sign as third word in assign statement:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  string io_name;
  string iname("");
  bool   is_iterator;
  u32    index_value;
  bool   is_indexed = parse_operand(1, io_name, is_iterator, index_value);
  output_definition& output = *m_output_definition_map[io_name];
  if (output.m_lua_function) {
    ostringstream oss;
    oss << "<OutputName> '" << io_name << "' appears in a assign line after already appearing in an lua_function line:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  output.m_assign = true;
  u32    start  = index_value;
  u32    stop   = index_value;
  i32    step   = 1;
  u32    range  = 1;
  if (!is_indexed || is_iterator) {
    // Initialize all iterators to their start value
    for (iterator_definition_vector::iterator ii = m_iterators.begin(); ii != m_iterators.end(); ++ii) {
      (*ii)->init();
    }
    if (!is_indexed) {
      start     = 0;
      stop      = output.m_bit_width - 1;
      step      = 1;
      range     = output.m_bit_width;
    }
    else {
      iterator_definition& iterator = *m_iterators[index_value];
      start     = iterator.m_start;
      stop      = iterator.m_stop;
      step      = iterator.m_step;
      range     = iterator.range();
      iname     = iterator.m_name;
    }
  }
  u32 output_bit_index = start;
  bool more_iterations = true;
  u32 num_words = m_words.size();
  while (more_iterations) {
    if (output_bit_index >= output.m_bit_width) {
      ostringstream oss;
      if (is_iterator) {
        oss << "Output bit iterator (" << iname << "=";
      }
      else {
        oss << "Output bit index (";
      }
      oss << output_bit_index << ") is out of range [0:" << (output.m_bit_width-1) << "] at word #2:" << endl;
      oss << m_line;
      oss << m_p_definition_file->info_for_exception();
      throw xtsc_exception(oss.str());
    }
    if (output.m_bit_assigned[output_bit_index]) {
      ostringstream oss;
      oss << "Output bit " << output.m_name << "[" << output_bit_index << "] has previously been assigned:" << endl;
      oss << m_line;
      oss << m_p_definition_file->info_for_exception();
      throw xtsc_exception(oss.str());
    }
    output.m_bit_assigned[output_bit_index] = 1;
    set<input_definition*> inputs;      // All inputs that this output bit is a function of
    rpn_assignment *assignment = new rpn_assignment();
    assignment->push_back(output.m_index);
    assignment->push_back(output_bit_index);
    u32 stack_depth = 0;
    for (u32 i = 3; i < num_words; ++i) {
      string word = m_words[i];
      bool is_operator = true;
      u32 op_code = 0;
      u32 num_pops = 2;
      if (word == "!") {
        op_code = op_NOT;
        num_pops = 1;
      }
      else if (word == "&") {
        op_code = op_AND;
      }
      else if (word == "|") {
        op_code = op_OR;
      }
      else if (word == "^") {
        op_code = op_XOR;
      }
      else {
        is_operator = false;
        num_pops = 0;
        string  input_name;
        bool    is_iterator;
        u32     index_value;
        bool    is_indexed = parse_operand(i, input_name, is_iterator, index_value);
        input_definition& input = *m_input_definition_map[input_name];
        input.m_outputs.insert(&output);
        inputs.insert(&input);
        if (is_iterator && (m_iterators[index_value]->range() != range)) {
          ostringstream oss;
          oss << "Unequal explicit iterator range (" << m_iterators[index_value]->range() << " != "
              << range << ") at word #" << (i+1) << ":" << endl;
          oss << m_line;
          oss << m_p_definition_file->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        if (!is_indexed && (input.m_bit_width != range)) {
          ostringstream oss;
          oss << "Unequal implied iterator range (" << input.m_bit_width << " != " << range
              << ") at word #" << (i+1) << ":" << endl;
          oss << m_line;
          oss << m_p_definition_file->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        u32 input_bit_index = (is_indexed ? (is_iterator ? m_iterators[index_value]->value() : index_value) : output_bit_index);
        if (input_bit_index >= input.m_bit_width) {
          ostringstream oss;
          oss << "Input bit index (" << input_bit_index << ") is out of range [0:" << (input.m_bit_width-1) << "] at word #"
              << (i+1) << ":" << endl;
          oss << m_line;
          oss << m_p_definition_file->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        assignment->push_back(op_PUSH);
        assignment->push_back(input.m_index);
        assignment->push_back(input_bit_index);
      }
      if (is_operator) {
        if (stack_depth < num_pops) {
          ostringstream oss;
          oss << "Invalid RPN expression - empty stack at word #" << (i+1) << ":" << endl;
          oss << m_line;
          oss << m_p_definition_file->info_for_exception();
          throw xtsc_exception(oss.str());
        }
        stack_depth -= num_pops;  // Operands popped off
        assignment->push_back(op_code);
      }
      stack_depth += 1; // result/operand pushed on
      if (stack_depth > m_max_depth) {
        m_max_depth = stack_depth;
      }
    }
    if (stack_depth != 1) {
      ostringstream oss;
      oss << "Invalid RPN expression - stack should contain exactly 1 item (but has " << stack_depth << "):" << endl;
      oss << m_line;
      oss << m_p_definition_file->info_for_exception();
      throw xtsc_exception(oss.str());
    }
    m_assignments.push_back(assignment);
    for (set<input_definition*>::const_iterator i = inputs.begin(); i != inputs.end(); ++i) {
      input_definition& input = **i;
      input.m_assignments.push_back(assignment);
    }
    // Increment all iterators 
    for (iterator_definition_vector::iterator ii = m_iterators.begin(); ii != m_iterators.end(); ++ii) {
      (*ii)->step();
    }
    output_bit_index += step;
    more_iterations = ((step > 0) ? ((output_bit_index >= start) && (output_bit_index <= stop)) :
                                    ((output_bit_index <= start) && (output_bit_index >= stop)));
  }
}



bool xtsc_component::xtsc_wire_logic::parse_operand(u32         index,
                                                    string&     io_name,
                                                    bool&       is_iterator,
                                                    u32&        index_value)
{
  bool invalid = true;
  bool is_indexed = false;
  is_iterator = false;
  index_value = 0;
  io_name = "";
  string::size_type pos1 = m_words[index].find_first_of('[');
  string::size_type pos2 = m_words[index].find_first_of(']');
  if (pos1 == string::npos) {
    io_name = m_words[index];
    invalid = false;
  }
  else if ((pos1 != 0) && (pos2 != string::npos) && (pos2 > pos1 + 1)) {
    is_indexed = true;
    io_name = m_words[index].substr(0, pos1);
    string operand_index = m_words[index].substr(pos1+1, pos2 - pos1 - 1);
    if ((operand_index[0] >= '0') && (operand_index[0] <= '9')) {
      try {
        index_value = xtsc_strtou32(operand_index);
        invalid = false;
      }
      catch (...) {}
    }
    else {
      iterator_definition_map::const_iterator ii = m_iterator_definition_map.find(operand_index);
      if (ii != m_iterator_definition_map.end()) {
        invalid = false;
        is_iterator = true;
        index_value = ii->second->m_index;
      }
    }
  }
  if (invalid || !is_identifier(io_name)) {
    ostringstream oss;
    oss << "'" << m_words[index] << "' is not a valid operand:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  bool input = (index != 1);
  if (( input && (m_input_definition_map .find(io_name) == m_input_definition_map .end())) ||
      (!input && (m_output_definition_map.find(io_name) == m_output_definition_map.end())))
  {
    ostringstream oss;
    oss << "'" << io_name << "' is not a defined " << (input ? "input" : "output") << " at word #" << (index+1) << ":" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return is_indexed;
}



//      1            2               3              4       <= size
//      0            1               2              3       <= index
// lua_function <OutputName> <LuaFunctionName> <InputName> ...
void xtsc_component::xtsc_wire_logic::handle_lua_function() {
  u32 num_words = m_words.size();
  if (num_words < 4) {
    ostringstream oss;
    oss << "lua_function statement has too few words:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  output_definition_map::const_iterator io = m_output_definition_map.find(m_words[1]);
  if (io == m_output_definition_map.end()) {
    ostringstream oss;
    oss << "<OutputName> '" << m_words[1] << "' has not yet appeared in an output line:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  output_definition& output = *io->second;
  if (output.m_assign) {
    ostringstream oss;
    oss << "<OutputName> '" << m_words[1] << "' appears in a lua_function line after already appearing in an assign line:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  if (output.m_lua_function) {
    ostringstream oss;
    oss << "<OutputName> '" << m_words[1] << "' appears in multiple lua_function lines:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  output.m_lua_function = true;
  if (!is_identifier(m_words[2])) {
    ostringstream oss;
    oss << "<LuaFunctionName> '" << m_words[2] << "' is not a valid Lua function name:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  output.m_lua_function_name = m_words[2];
  for (u32 i = 3; i < num_words; ++i) {
    input_definition_map::const_iterator ii = m_input_definition_map.find(m_words[i]);
    if (ii == m_input_definition_map.end()) {
      ostringstream oss;
      oss << "<InputName> '" << m_words[i] << "' has not yet appeared in an input line:" << endl;
      oss << m_line;
      oss << m_p_definition_file->info_for_exception();
      throw xtsc_exception(oss.str());
    }
    input_definition& input = *ii->second;
    input.m_lua_function_outputs.insert(&output);
    input.m_outputs.insert(&output);
    output.m_lua_function_inputs.push_back(input.m_index);
  }
}



bool xtsc_component::xtsc_wire_logic::has_input(const char *input_name) const {
  input_definition_map::const_iterator ii = m_input_definition_map.find(input_name);
  return (ii != m_input_definition_map.end());
}



bool xtsc_component::xtsc_wire_logic::has_output(const char *output_name) const {
  output_definition_map::const_iterator io = m_output_definition_map.find(output_name);
  return (io != m_output_definition_map.end());
}



xtsc::u32 xtsc_component::xtsc_wire_logic::get_bit_width(const char *io_name) const {
  input_definition_map::const_iterator ii = m_input_definition_map.find(io_name);
  if (ii != m_input_definition_map.end()) {
    return ii->second->m_bit_width;
  }
  output_definition_map::const_iterator io = m_output_definition_map.find(io_name);
  if (io != m_output_definition_map.end()) {
    return io->second->m_bit_width;
  }
  ostringstream oss;
  oss << "xtsc_wire_logic::get_bit_width(): '" << name() << "' has no input/output named '" << io_name << "'";
  throw xtsc_exception(oss.str());
}



sc_export<xtsc_wire_write_if>& xtsc_component::xtsc_wire_logic::get_input(const char *input_name) const {
  input_definition_map::const_iterator ii = m_input_definition_map.find(input_name);
  if (ii == m_input_definition_map.end()) {
    ostringstream oss;
    oss << "xtsc_wire_logic::get_input(): '" << name() << "' has no sc_export input named '" << input_name << "'";
    throw xtsc_exception(oss.str());
  }
  return *ii->second->m_p_wire_write_export;
}



sc_port<xtsc_wire_write_if, NSPP>& xtsc_component::xtsc_wire_logic::get_output(const char *output_name) const {
  output_definition_map::const_iterator io = m_output_definition_map.find(output_name);
  if (io == m_output_definition_map.end()) {
    ostringstream oss;
    oss << "xtsc_wire_logic::get_output(): '" << name() << "' has no sc_port output named '" << output_name << "'";
    throw xtsc_exception(oss.str());
  }
  return *io->second->m_p_wire_write_port;
}



set<string> xtsc_component::xtsc_wire_logic::get_input_set() const {
  return m_input_set;
}



set<string> xtsc_component::xtsc_wire_logic::get_output_set() const {
  return m_output_set;
}



void xtsc_component::xtsc_wire_logic::execute(const string&             cmd_line, 
                                              const vector<string>&     words,
                                              const vector<string>&     words_lc,
                                              ostream&                  result)
{
  ostringstream res;

  if (false) {
  }
  else if (words[0] == "change_clock_period") {
    u32 clock_period_factor = xtsc_command_argtou32(cmd_line, words, 1);
    res << m_clock_period.value();
    change_clock_period(clock_period_factor);
  }
  else if (words[0] == "reset") {
    reset();
  }
  else {
    ostringstream oss;
    oss << name() << "::" << __FUNCTION__ << "() called for unknown command '" << cmd_line << "'.";
    throw xtsc_exception(oss.str());
  }

  result << res.str();
}



void xtsc_component::xtsc_wire_logic::connect(xtsc_core& core, const char *core_intf_name, const char *io_name) {
  u32 wo = 0;
  bool output = true;
  if (core.has_export_state(core_intf_name)) {
    core.get_export_state(core_intf_name)(get_input(io_name));
    wo = core.get_tie_bit_width(core_intf_name);
  }
  else if (core.has_system_output_wire(core_intf_name)) {
    core.get_system_output_wire(core_intf_name)(get_input(io_name));
    wo = core.get_sysio_bit_width(core_intf_name);
  }
  else if (core.has_system_input_wire(core_intf_name)) {
    output = false;
    get_output(io_name)(core.get_system_input_wire(core_intf_name));
    wo = core.get_sysio_bit_width(core_intf_name);
  }
  else {
    ostringstream oss;
    oss << "xtsc_wire_logic::connect: core '" << core.name() << "' has no export state or system-level output/input wire named '"
        << core_intf_name << "'.";
    throw xtsc_exception(oss.str());
  }
  u32 wi = get_bit_width(io_name);
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit " << (output ? "output" : "input") << " '" << core_intf_name
        << "' of xtsc_core '" << core.name() << "' to " << wi << "-bit " << (output ? "input" : "output") << " '" << io_name
        << "' of xtsc_wire_logic '" << name() << "'";
    throw xtsc_exception(oss.str());
  }
}



void xtsc_component::xtsc_wire_logic::connect(xtsc_tx_loader& loader, const char *output_name, const char *input_name) {
  u32 wo = 0;
  u32 wi = get_bit_width(input_name);
  string output(output_name ? output_name : "");
  if (output == "Done") {
    wo = 1;
    loader.m_done(get_input(input_name));
  }
  else if (output == "Mode") {
    wo = 3;
    loader.m_mode(get_input(input_name));
  }
  else {
    ostringstream oss;
    oss << "xtsc_tx_loader '" << loader.name() << "' has no output named \"" << output
        << "\" (valid output names are \"Done\" and \"Mode\")";
    throw xtsc_exception(oss.str());
  }
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit output '" << output << "' of xtsc_tx_loader '" << loader.name()
        << "' to " << wi << "-bit input '" << input_name << "' of xtsc_wire_logic '" << name() << "'";
    throw xtsc_exception(oss.str());
  }
}



void xtsc_component::xtsc_wire_logic::connect(xtsc_wire_logic& logic, const char *output_name, const char *input_name) {
  u32 wo = logic.get_bit_width(output_name);
  u32 wi = get_bit_width(input_name);
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit output '" << output_name << "' of xtsc_wire_logic '" << logic.name()
        << "' to " << wi << "-bit input '" << input_name << "' of xtsc_wire_logic '" << name() << "'";
    throw xtsc_exception(oss.str());
  }
  logic.get_output(output_name)(get_input(input_name));
}



void xtsc_component::xtsc_wire_logic::connect(xtsc_interrupt_distributor&       distributor,
                                              const char                       *distributor_io_name,
                                              const char                       *logic_io_name)
{
  bool distributor_is_output = distributor.has_output(distributor_io_name);
  u32 wo = distributor.get_bit_width(distributor_io_name);
  u32 wi = get_bit_width(logic_io_name);
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit I/O '" << distributor_io_name << "' of xtsc_interrupt_distributor '"
        << distributor.name() << "' to " << wi << "-bit I/O '" << logic_io_name << "' of xtsc_wire_logic '" << name() << "'";
    throw xtsc_exception(oss.str());
  }
  if (distributor_is_output) {
    distributor.get_output(distributor_io_name)(get_input(logic_io_name));
  }
  else {
    get_output(logic_io_name)(distributor.get_input(distributor_io_name));
  }
}



void xtsc_component::xtsc_wire_logic::connect(xtsc_mmio& mmio, const char *output_name, const char *input_name) {
  u32 wo = mmio.get_bit_width(output_name);
  u32 wi = get_bit_width(input_name);
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit output '" << output_name << "' of xtsc_mmio '" << mmio.name()
        << "' to " << wi << "-bit input '" << input_name << "' of xtsc_wire_logic '" << name() << "'";
    throw xtsc_exception(oss.str());
  }
  mmio.get_output(output_name)(get_input(input_name));
}



void xtsc_component::xtsc_wire_logic::connect(xtsc_wire_source& source, const char *output_name, const char *input_name) {
  u32 wo = source.get_bit_width(output_name);
  u32 wi = get_bit_width(input_name);
  if (wo != wi) {
    ostringstream oss;
    oss << "Width mismatch:  cannot connect " << wo << "-bit output '" << output_name << "' of xtsc_wire_source '" << source.name()
        << "' to " << wi << "-bit input '" << input_name << "' of xtsc_wire_logic '" << name() << "'";
    throw xtsc_exception(oss.str());
  }
  source.get_tlm_output(output_name)(get_input(input_name));
}



void xtsc_component::xtsc_wire_logic::end_of_elaboration(void) {
  reset();
} 



void xtsc_component::xtsc_wire_logic::reset(bool /* hard_reset */) {
  XTSC_INFO(m_text, kind() << "::reset()");

  m_next_delay_thread_index = 0;
  for (input_definition_vector::iterator i = m_inputs.begin(); i != m_inputs.end(); ++i) {
    (*i)->reset();
  }
  for (output_definition_vector::iterator i = m_outputs.begin(); i != m_outputs.end(); ++i) {
    (*i)->reset();
  }

  xtsc_reset_processes(m_process_handles);
}



u32 xtsc_component::xtsc_wire_logic::get_u32(u32 index, const string& argument_name) {
  u32 value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtou32(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert " << argument_name << " argument (#" << index+1 << ") '" << m_words[index] << "' to number:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



i32 xtsc_component::xtsc_wire_logic::get_i32(u32 index, const string& argument_name) {
  i32 value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtoi32(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert " << argument_name << " argument (#" << index+1 << ") '" << m_words[index] << "' to number:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



double xtsc_component::xtsc_wire_logic::get_double(u32 index, const string& argument_name) {
  double value = 0;
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << argument_name << " argument (#" << index+1 << ") missing:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  try {
    value = xtsc_strtod(m_words[index]);
  }
  catch (const xtsc_exception&) {
    ostringstream oss;
    oss << "Cannot convert " << argument_name << " argument (#" << index+1 << ") '" << m_words[index] << "' to number:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return value;
}



const string& xtsc_component::xtsc_wire_logic::validate_identifier(u32 index, const string& argument_name) {
  if (index >= m_words.size()) {
    ostringstream oss;
    oss << "The " << argument_name << " argument (#" << index+1 << ") is missing:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  if (!is_identifier(m_words[index])) {
    ostringstream oss;
    oss << "The " << argument_name << " argument ('" << m_words[index] << "') contains invalid characters:" << endl;
    oss << m_line;
    oss << m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  return m_words[index];
}



bool xtsc_component::xtsc_wire_logic::is_identifier(const string& name) {
  const char *name_c_str = name.c_str();
  for (const char *pc = name_c_str; *pc; ++pc) {
    char c = *pc;
    if (c>='a' && c<='z') continue;
    if (c>='A' && c<='Z') continue;
    if (c=='_') continue;
    // If not 1st character . . .
    if (pc != name_c_str) {
      // Allow digit
      if (c>='0' && c<='9') continue;
    }
    return false;
  }
  return (name != "");
}



xtsc_component::xtsc_wire_logic::output_definition::output_definition(xtsc_wire_logic&  logic,
                                                                      const string&     name,
                                                                      u32               index,
                                                                      u32               bit_width,
                                                                      const string&     initial_value,
                                                                      bool              always_write,
                                                                      bool              delay_output,
                                                                      sc_time           delay_time) :
  m_logic            (logic),
  m_name             (name),
  m_index            (index),
  m_bit_width        (bit_width),
  m_value            (bit_width),
  m_value_prev       (bit_width),
  m_bit_assigned     (bit_width),
  m_initial_value    (initial_value),
  m_lua_function_name(""),
  m_lua_function     (false),
  m_assign           (false),
  m_always_write     (always_write),
  m_delay_output     (delay_output),
  m_delay_time       (delay_time)
{
  m_p_wire_write_port = new wire_write_port(m_name.c_str());
  m_bit_assigned = 0;
  try {
    m_value = m_initial_value.c_str();
  }
  catch (...) {
    ostringstream oss;
    oss << "Cannot convert <InitialValue> (word #4 = \"" << m_initial_value << "\") to number:" << endl;
    oss << m_logic.m_line;
    oss << m_logic.m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
}



void xtsc_component::xtsc_wire_logic::output_definition::dump(ostream& os) const {
  // Save state of stream
  char c = os.fill('0');
  ios::fmtflags old_flags = os.flags();
  streamsize w = os.width();
  os << right;

  os << m_name << " " << m_bit_width << " " << m_initial_value << " " << (m_always_write ? "always" : "change");
  if (m_delay_output) {
    os << " " << m_delay_time;
  }

  // Restore state of stream
  os.fill(c);
  os.flags(old_flags);
  os.width(w);
}



xtsc_component::xtsc_wire_logic::input_definition::input_definition(xtsc_wire_logic&   logic,
                                                                    const std::string& name,
                                                                    xtsc::u32          index,
                                                                    xtsc::u32          bit_width,
                                                                    const std::string& initial_value) :
  m_logic                   (logic),
  m_name                    (name),
  m_index                   (index),
  m_bit_width               (bit_width),
  m_value                   (bit_width),
  m_initial_value           (initial_value),
  m_detect_value_change     (false)
{
  reset();
}



void xtsc_component::xtsc_wire_logic::input_definition::reset() {
  try {
    m_value = m_initial_value.c_str();
  }
  catch (...) {
    ostringstream oss;
    oss << "Cannot convert <InitialValue> (word #4 = \"" << m_initial_value << "\") to number:" << endl;
    oss << m_logic.m_line;
    oss << m_logic.m_p_definition_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
}




void xtsc_component::xtsc_wire_logic::input_definition::dump(ostream& os) const {
  // Save state of stream
  char c = os.fill('0');
  ios::fmtflags old_flags = os.flags();
  streamsize w = os.width();
  os << right;

  os << m_name << " " << m_bit_width << " " << m_initial_value;

  // Restore state of stream
  os.fill(c);
  os.flags(old_flags);
  os.width(w);
}



void xtsc_component::xtsc_wire_logic::iterator_definition::dump(ostream& os) const {
  // Save state of stream
  char c = os.fill('0');
  ios::fmtflags old_flags = os.flags();
  streamsize w = os.width();
  os << right;

  os << m_name << "=" << m_value << " (" << m_start << ":" << m_stop << ":" << m_step << ")";

  // Restore state of stream
  os.fill(c);
  os.flags(old_flags);
  os.width(w);
}



void xtsc_component::xtsc_wire_logic::input_definition::xtsc_wire_write_if_impl::nb_write(const sc_unsigned& value) {
  if (static_cast<u32>(value.length()) != m_bit_width) {
    ostringstream oss;
    oss << "ERROR: Value of width=" << value.length() << " bits written to sc_export \"" << m_input_definition.m_name << "\" of width="
        << m_bit_width << " in xtsc_wire_logic '" << m_input_definition.m_logic.name() << "'";
    throw xtsc_exception(oss.str());
  }
  // A variable substr is used to get rid of the pesky extra leading zero bit
  XTSC_INFO(m_text, m_input_definition.m_name << " <= 0x" << value.to_string(SC_HEX).substr(m_bit_width%4 ? 2 : 3));
  m_input_definition.m_value = value;

  // Make a copy of each dependent output that has a WritePolicy of change
  if (m_input_definition.m_detect_value_change) {
    for (output_set::iterator i = m_input_definition.m_outputs.begin(); i != m_input_definition.m_outputs.end(); ++i) {
      output_definition& output = **i;
      if (!output.m_always_write) {
        output.m_value_prev = output.m_value;
      }
    }
  }

  // Crunch all RPN assignments dependent on this input
  for (assignment_table::iterator i = m_input_definition.m_assignments.begin(); i != m_input_definition.m_assignments.end(); ++i) {
    rpn_assignment& assignment = **i;
    output_definition& output = *m_input_definition.m_logic.m_outputs[assignment[0]];
    u32 output_bit_index = assignment[1];
    u32 sp = 0;
    vector<bool>& stack = m_input_definition.m_logic.m_stack;
    for (u32 j = 2; j < assignment.size(); ++j) {
      switch (assignment[j]) {
        case op_PUSH: {
          input_definition& input = *m_input_definition.m_logic.m_inputs[assignment[j+1]];
          u32 bit_index = assignment[j+2];
          bool value = input.m_value[bit_index].to_bool();
          stack[sp] = value;
          XTSC_DEBUG(m_text, "stack[" << sp << "] <= " << input.m_name << "[" << bit_index << "] = " << value);
          sp += 1;
          j += 2;
          break;
        }
        case op_NOT: {
          XTSC_DEBUG(m_text, "stack[" << (sp-1) << "] <= ! " << stack[sp-1]);
          stack[sp-1] = !stack[sp-1];
          break;
        }
        case op_AND: {
          XTSC_DEBUG(m_text, "stack[" << (sp-2) << "] <= " << stack[sp-2] << " & " << stack[sp-1]);
          stack[sp-2] = stack[sp-2] && stack[sp-1];
          sp -= 1;
          break;
        }
        case op_OR:  {
          XTSC_DEBUG(m_text, "stack[" << (sp-2) << "] <= " << stack[sp-2] << " | " << stack[sp-1]);
          stack[sp-2] = stack[sp-2] || stack[sp-1];
          sp -= 1;
          break;
        }
        case op_XOR: {
          XTSC_DEBUG(m_text, "stack[" << (sp-2) << "] <= " << stack[sp-2] << " ^ " << stack[sp-1]);
          stack[sp-2] = (stack[sp-2] != stack[sp-1]);
          sp -= 1;
          break;
        }
        default: {
          ostringstream oss;
          oss << "Program Bug:  Unrecognized op-code in RPN assignment in xtsc_wire_logic '" << m_input_definition.m_logic.name()
              << "'";
          throw xtsc_exception(oss.str());
        }
      }
    }
    if (sp != 1) {
      ostringstream oss;
      oss << "Program Bug:  Stack pointer not 1 after RPN evalutaion in xtsc_wire_logic '" << m_input_definition.m_logic.name() << "'";
      throw xtsc_exception(oss.str());
    }
    XTSC_DEBUG(m_text, output.m_name << "[" << output_bit_index << "] <= " << stack[0]);
    output.m_value[output_bit_index] = stack[0];
  }

  // Call the Lua function of each output dependent on this input
  for (output_set::iterator i = m_input_definition.m_lua_function_outputs.begin(); i != m_input_definition.m_lua_function_outputs.end(); ++i) {
    output_definition& output = **i;
    ostringstream exp;
    exp << output.m_lua_function_name << "(" << hex;
    for (u32 j = 0; j < output.m_lua_function_inputs.size(); ++j) {
      input_definition& input = *m_input_definition.m_logic.m_inputs[output.m_lua_function_inputs[j]];
      if (j > 0) {
        exp << ", ";
      }
      // A variable substr is used to get rid of the pesky extra leading zero bit
      exp << "\"0x" << input.m_value.to_string(SC_HEX).substr(input.m_bit_width%4 ? 2 : 3) << "\"";
    }
    exp << ")";
    string result = m_input_definition.m_logic.m_p_definition_file->evaluate_lua_expression(exp.str());
    XTSC_INFO(m_text, "lua_function: " << exp.str() << " => " << result);
    output.m_value = result.c_str();
  }

  // Write or schedule any required outputs
  for (output_set::iterator i = m_input_definition.m_outputs.begin(); i != m_input_definition.m_outputs.end(); ++i) {
    output_definition& output = **i;
    if (output.m_always_write || (output.m_value != output.m_value_prev)) {
      if (output.m_delay_output) {
        output_info *p_output_info = output.new_output_info();
        p_output_info->m_value = output.m_value;
        p_output_info->m_output_time = sc_time_stamp() + output.m_delay_time;
        p_output_info->m_delta_cycle = sc_delta_count();
        output.m_output_info_deque.push_back(p_output_info);
        output.m_event.notify(output.m_delay_time);
        XTSC_DEBUG(m_text, "Scheduled 0x" << output.m_value.to_string(SC_HEX).substr(output.m_bit_width%4 ? 2 : 3) << " out \"" << output.m_name <<
                           "\" at " << p_output_info->m_output_time);
      }
      else {
        // A variable substr is used to get rid of the pesky extra leading zero bit
        XTSC_INFO(m_text, output.m_name << " => 0x" << output.m_value.to_string(SC_HEX).substr(output.m_bit_width%4 ? 2 : 3));
        (*output.m_p_wire_write_port)->nb_write(output.m_value);
      }
    }
  }
}



u32 xtsc_component::xtsc_wire_logic::input_definition::xtsc_wire_write_if_impl::nb_get_bit_width() {
  return m_bit_width;
}



void xtsc_component::xtsc_wire_logic::input_definition::xtsc_wire_write_if_impl::register_port(sc_port_base&    port,
                                                                                               const char      *if_typename)
{
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to sc_export<xtsc_wire_write_if> \"" << m_input_definition.m_name
        << "\" of xtsc_wire_logic '" << m_input_definition.m_logic.name() << "'" << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_text, "Binding '" << port.name() << "' to sc_export<xtsc_wire_write_if> \"" << m_input_definition.m_name <<
                    "\" of xtsc_wire_logic '" << m_input_definition.m_logic.name() << "'");
  m_p_port = &port;
}



