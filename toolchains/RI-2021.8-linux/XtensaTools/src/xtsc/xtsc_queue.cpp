// Copyright (c) 2005-2017 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.


#include <sstream>
#include <xtsc/xtsc_queue.h>
#include <xtsc/xtsc_core.h>
#include <xtsc/xtsc_queue_producer.h>
#include <xtsc/xtsc_queue_consumer.h>
#include <xtsc/xtsc_logging.h>


using namespace std;
#if SYSTEMC_VERSION >= 20050601
using namespace sc_core;
#endif
using namespace sc_dt;
using namespace xtsc;
using log4xtensa::INFO_LOG_LEVEL;
using log4xtensa::PUSH;
using log4xtensa::POP;
using log4xtensa::PUSH_FAILED;
using log4xtensa::POP_FAILED;
using log4xtensa::UNKNOWN_PC;
using log4xtensa::UNKNOWN;



xtsc_component::xtsc_queue_parms::xtsc_queue_parms(const xtsc_core&     core,
                                                   const char          *queue_name,
                                                   u32                  depth,
                                                   const char          *push_file,
                                                   const char          *pop_file,
                                                   bool                 wraparound)
{
  if (!core.has_input_queue(queue_name) && !core.has_output_queue(queue_name)) {
    ostringstream oss;
    oss << "xtsc_queue_parms: core '" << core.name() << "' has no input/output queue interface named '" << queue_name << "'.";
    throw xtsc_exception(oss.str());
  }
  u32 width1 = core.get_tie_bit_width(queue_name);
  init(width1, depth, push_file, pop_file, wraparound);
}



xtsc_component::xtsc_queue::xtsc_queue(sc_module_name module_name, const xtsc_queue_parms& queue_parms) :
  sc_module             (module_name),
  xtsc_module           (*(sc_module*)this),
  m_producer            (queue_parms.get_u32("num_producers") > 1 ? "m_producer[0]" : "m_producer"),
  m_consumer            (queue_parms.get_u32("num_consumers") > 1 ? "m_consumer[0]" : "m_consumer"),
  m_producers           (NULL),
  m_consumers           (NULL),
  m_push_impl           (NULL),
  m_pop_impl            (NULL),
  m_push_multi_impl     (NULL),
  m_pop_multi_impl      (NULL),
  m_num_producers       (queue_parms.get_u32("num_producers")),
  m_num_consumers       (queue_parms.get_u32("num_consumers")),
  m_multi_client        ((m_num_producers > 1) || (m_num_consumers > 1)),
  m_depth               (queue_parms.get_u32("depth")),
  m_fifo                ("m_fifo", (m_depth ? m_depth : 1)),
  m_use_fifo            (true),
  m_element_ptrs        (NULL),
  m_tickets             (NULL),
  m_skid_index          (0),
  m_skid_fifos          (NULL),
  m_skid_buffers        (NULL),
  m_skid_tickets        (NULL),
  m_jerk_index          (0),
  m_jerk_fifos          (NULL),
  m_jerk_buffers        (NULL),
  m_jerk_tickets        (NULL),
  m_pop_ticket          ((u64)-1),
  m_push_ticket         ((u64)-1),
  m_dummy               (1),    // Length of 1 bit
  m_width1              (queue_parms.get_non_zero_u32("bit_width")),
  m_width8              ((m_width1+7)/8),
  m_value               (m_width1),
  m_text                (log4xtensa::TextLogger::getInstance(name())),
  m_binary              (log4xtensa::BinaryLogger::getInstance(name())),
  m_log_data_binary     (true),
  m_push_file           (NULL),
  m_timestamp           (false),
  m_pop_file            (NULL),
  m_pop_file_element    (m_width1),
#if IEEE_1666_SYSTEMC >= 201101L
  m_drain_fifo_event    ("m_drain_fifo_event"),
  m_push_pop_event      ("m_push_pop_event"),
  m_nonempty_event      ("m_nonempty_event"),
  m_nonfull_event       ("m_nonfull_event"),
#endif
  m_nonempty_events     (NULL),
  m_nonfull_events      (NULL),
  m_host_shared_memory  (queue_parms.get_bool("host_shared_memory")),
  m_shmem_name          (""),
  m_p_shmem             (NULL),
  m_shmem_bytes_per_row (0),
  m_shmem_num_rows      (0),
  m_shmem_array_size    (0),
  m_shmem_total_size    (0),
  m_p_shmem_ridx        (NULL),
  m_p_shmem_widx        (NULL)
{

  m_dummy = 0;                  // Value of 0

  // Handle push_file
  const char *push_file = queue_parms.get_c_str("push_file");
  if (push_file && push_file[0]) {
    m_push_file_name = push_file;
    m_use_fifo = false;
    m_push_file = new ofstream(m_push_file_name.c_str(), ios::out);
    if (!m_push_file->is_open()) {
      ostringstream oss;
      oss << "xtsc_queue '" << name() << "' cannot open push_file '" << m_push_file_name << "'.";
      throw xtsc_exception(oss.str());
    }
    m_timestamp = queue_parms.get_bool("timestamp");
  }

  // Handle pop_file
  m_wraparound  = queue_parms.get_bool("wraparound");
  const char *pop_file = queue_parms.get_c_str("pop_file");
  if (pop_file && pop_file[0]) {
    m_use_fifo = false;
    m_pop_file = new xtsc_script_file(pop_file, "pop_file",  name(), kind(), m_wraparound);
    m_pop_file_name = m_pop_file->name();
    get_next_pop_file_element();
  }

  const char *shared_memory_name = queue_parms.get_c_str("shared_memory_name");
  if (m_host_shared_memory) {
    if (m_push_file) {
      ostringstream oss;
      oss << "Error in " << kind() << " '" << name()
          << "': It is not legal for \"push_file\" to be specified if \"host_shared_memory\" is true.";
      throw xtsc_exception(oss.str());
    }
    if (m_pop_file) {
      ostringstream oss;
      oss << "Error in " << kind() << " '" << name()
          << "': It is not legal for \"pop_file\" to be specified if \"host_shared_memory\" is true.";
      throw xtsc_exception(oss.str());
    }
    if (m_multi_client) {
      ostringstream oss;
      oss << "Error in " << kind() << " '" << name()
          << "': It is not legal for \"num_producers\" or \"num_consumers\" to be greater than 1 if \"host_shared_memory\" is true.";
      throw xtsc_exception(oss.str());
    }
    if (shared_memory_name && shared_memory_name[0]) {
      m_shmem_name = shared_memory_name;
    }
    else {
      ostringstream oss;
      oss << xtsc_get_user_name(name(), kind());
      oss << "." << name();
      m_shmem_name = oss.str();
    }
    m_use_fifo = false;
    // Each row contains 64-bit ticket followed by data extended to 64-bit boundary
    m_shmem_bytes_per_row = ((64 + m_width1 + 63) / 64) * 8;
    m_shmem_num_rows      = m_depth + 1;
    m_shmem_array_size    = m_shmem_bytes_per_row*m_shmem_num_rows;
    m_shmem_total_size    = m_shmem_array_size+8;
    m_p_shmem             = xtsc_get_shared_memory(m_shmem_name, m_shmem_total_size, name(), kind(), 0x0);
    m_p_shmem_ridx        = (u32*)&m_p_shmem[m_shmem_array_size+0];
    m_p_shmem_widx        = (u32*)&m_p_shmem[m_shmem_array_size+4];
  }

  if (m_use_fifo || m_p_shmem) {
    if (m_depth == 0) {
      // This will generate an exception in a standard format 
      queue_parms.get_non_zero_u32("depth");
    }
  }

  if (m_use_fifo) {
    // Create a pool of sc_unsigned objects with pool size equal to the fifo depth
    m_element_ptrs = new sc_unsigned*[m_depth];
    for (u32 i=0; i<m_depth; i++) {
      m_element_ptrs[i] = new sc_unsigned(m_width1);
    }
    m_tickets = new u64[m_depth];
    SC_METHOD(drain_fifo_method);
    sensitive << m_drain_fifo_event;
  }

  m_producers = new sc_export<xtsc_queue_push_if>*[m_num_producers];
  m_consumers = new sc_export<xtsc_queue_pop_if>*[m_num_consumers];
  m_producers[0] = &m_producer;
  m_consumers[0] = &m_consumer;

  if (m_multi_client) {
    m_push_multi_impl   = new xtsc_queue_push_if_multi_impl*[m_num_producers];
    if (m_use_fifo) {
      m_skid_fifos      = new sc_fifo<int>                 *[m_num_producers];
      m_skid_buffers    = new sc_unsigned                  *[m_num_producers];
      m_skid_tickets    = new u64                           [m_num_producers];
      m_nonfull_events  = new sc_event                     *[m_num_producers];
      m_jerk_fifos      = new sc_fifo<int>                 *[m_num_consumers];
      m_jerk_buffers    = new sc_unsigned                  *[m_num_consumers];
      m_jerk_tickets    = new u64                           [m_num_consumers];
      m_nonempty_events = new sc_event                     *[m_num_consumers];
      SC_METHOD(delta_cycle_method);
      sensitive << m_push_pop_event;
    }
    for (u32 i=0; i < m_num_producers; ++i) {
      if (i > 0) {
        ostringstream oss1;
        oss1 << "m_producer[" << i << "]";
        m_producers[i] = new sc_export<xtsc_queue_push_if>(oss1.str().c_str());
      }
      ostringstream oss2;
      oss2 << "m_push_multi_impl[" << i << "]";
      m_push_multi_impl[i] = new xtsc_queue_push_if_multi_impl(oss2.str().c_str(), *this, i);
      (*m_producers[i])(*m_push_multi_impl[i]);
      if (m_use_fifo) {
        ostringstream oss3;
        oss3 << "m_skid_fifos[" << i << "]";
        m_skid_fifos    [i] = new sc_fifo<int>(oss3.str().c_str(), 1);
        m_skid_buffers  [i] = new sc_unsigned(m_width1);
        m_skid_tickets  [i] = 0;
        ostringstream oss4;
        oss4 << "m_nonfull_events_" << i;
#if IEEE_1666_SYSTEMC >= 201101L
        m_nonfull_events[i] = new sc_event(oss4.str().c_str());
#else
        m_nonfull_events[i] = new sc_event();
        xtsc_event_register(*m_nonfull_events[i], oss4.str().c_str(), this);
#endif
      }
    }
    m_pop_multi_impl    = new xtsc_queue_pop_if_multi_impl*[m_num_consumers];
    for (u32 i=0; i < m_num_consumers; ++i) {
      if (i > 0) {
        ostringstream oss1;
        oss1 << "m_consumer[" << i << "]";
        m_consumers[i] = new sc_export<xtsc_queue_pop_if>(oss1.str().c_str());
      }
      ostringstream oss2;
      oss2 << "m_pop_multi_impl[" << i << "]";
      m_pop_multi_impl[i] = new xtsc_queue_pop_if_multi_impl(oss2.str().c_str(), *this, i);
      (*m_consumers[i])(*m_pop_multi_impl[i]);
      if (m_use_fifo) {
        ostringstream oss3;
        oss3 << "m_jerk_fifos[" << i << "]";
        m_jerk_fifos     [i] = new sc_fifo<int>(oss3.str().c_str(), 1);
        m_jerk_buffers   [i] = new sc_unsigned(m_width1);
        m_jerk_tickets   [i] = 0;
        ostringstream oss4;
        oss4 << "m_nonempty_events_" << i;
#if IEEE_1666_SYSTEMC >= 201101L
        m_nonempty_events[i] = new sc_event(oss4.str().c_str());
#else
        m_nonempty_events[i] = new sc_event();
        xtsc_event_register(*m_nonempty_events[i], oss4.str().c_str(), this);
#endif
      }
    }
  }
  else {
    m_push_impl = new xtsc_queue_push_if_impl("m_push_impl", *this),
    m_pop_impl  = new xtsc_queue_pop_if_impl("m_pop_impl", *this),
    m_producer(*m_push_impl);
    m_consumer(*m_pop_impl);
  }

  for (u32 i=0; i<m_num_producers; ++i) {
    m_port_types[m_producers[i]->basename()] = QUEUE_PUSH_EXPORT;
    if (m_num_producers == 1) {
      m_port_types["queue_push"] = PORT_TABLE;
    }
    else {
      ostringstream oss;
      oss << "queue_push[" << i << "]";
      m_port_types[oss.str()] = PORT_TABLE;
    }
  }

  for (u32 i=0; i<m_num_consumers; ++i) {
    m_port_types[m_consumers[i]->basename()] = QUEUE_POP_EXPORT;
    if (m_num_consumers == 1) {
      m_port_types["queue_pop"] = PORT_TABLE;
    }
    else {
      ostringstream oss;
      oss << "queue_pop[" << i << "]";
      m_port_types[oss.str()] = PORT_TABLE;
    }
  }

  xtsc_register_command(*this, *this, "can_pop", 0, 1,
      "can_pop [<PopPort>]", 
      "Return nb_can_pop() for the specified <PopPort> (default 0)."
  );

  xtsc_register_command(*this, *this, "can_push", 0, 1,
      "can_push [<PushPort>]", 
      "Return nb_can_push() for the specified <PushPort> (default 0)."
  );

  xtsc_register_command(*this, *this, "dump", 0, 0,
      "dump", 
      "Return the os buffer from calling xtsc_queue::dump(os)."
  );

  xtsc_register_command(*this, *this, "get_bit_width", 0, 0,
      "get_bit_width", 
      "Return xtsc_queue::get_bit_width()."
  );

  xtsc_register_command(*this, *this, "get_num_consumers", 0, 0,
      "get_num_consumers", 
      "Return xtsc_queue::get_num_consumers()."
  );

  xtsc_register_command(*this, *this, "get_num_producers", 0, 0,
      "get_num_producers", 
      "Return xtsc_queue::get_num_producers()."
  );

  xtsc_register_command(*this, *this, "get_pop_ticket", 0, 0,
      "get_pop_ticket", 
      "Return the ticket of most recent previous element popped."
  );

  xtsc_register_command(*this, *this, "get_push_ticket", 0, 0,
      "get_push_ticket", 
      "Return the ticket of most recent previous element pushed."
  );

  xtsc_register_command(*this, *this, "num_available", 0, 0,
      "num_available", 
      "Return xtsc_queue::num_available()."
  );

  xtsc_register_command(*this, *this, "num_free", 0, 0,
      "num_free", 
      "Return xtsc_queue::num_free()."
  );

  xtsc_register_command(*this, *this, "peek", 1, 1,
      "peek <nth>", 
      "Return value from calling xtsc_queue::peek(<nth>, value)."
  );

  xtsc_register_command(*this, *this, "poke", 2, 2,
      "poke <nth> <NewValue>", 
      "Call xtsc_queue::poke(<nth>, <NewValue>).  Return old value."
  );

  xtsc_register_command(*this, *this, "pop", 0, 1,
      "pop [<PopPort>]", 
      "Return the value popped by calling nb_pop() for the specified <PopPort> (default 0)."
  );

  xtsc_register_command(*this, *this, "push", 1, 2,
      "push <Value> [<PushPort>]", 
      "Return nb_push(<Value>) for the specified <PushPort> (default 0)."
  );

  xtsc_register_command(*this, *this, "reset", 0, 0,
      "reset", 
      "Call xtsc_queue::reset()."
  );

#if IEEE_1666_SYSTEMC < 201101L
  xtsc_event_register(m_drain_fifo_event, "m_drain_fifo_event", this);
  xtsc_event_register(m_push_pop_event,   "m_push_pop_event",   this);
  xtsc_event_register(m_nonempty_event,   "m_nonempty_event",   this);
  xtsc_event_register(m_nonfull_event,    "m_nonfull_event",    this);
#endif

  log4xtensa::LogLevel ll = xtsc_get_constructor_log_level();
  XTSC_LOG(m_text, ll,        "Constructed xtsc_queue '" << name() << "'" << (m_multi_client ? " (Multi-client)" : "") << ":");
  XTSC_LOG(m_text, ll,        " num_producers           = "   << m_num_producers);
  XTSC_LOG(m_text, ll,        " num_consumers           = "   << m_num_consumers);
  XTSC_LOG(m_text, ll,        " depth                   = "   << m_depth);
  XTSC_LOG(m_text, ll,        " bit_width               = "   << m_width1);
  XTSC_LOG(m_text, ll,        " push_file               = "   << m_push_file_name);
  if (m_push_file_name != "") {
  XTSC_LOG(m_text, ll,        " timestamp               = "   << boolalpha << m_timestamp);
  }
  XTSC_LOG(m_text, ll,        " pop_file                = "   << m_pop_file_name);
  XTSC_LOG(m_text, ll,        " wraparound              = "   << (m_wraparound ? "true" : "false"));
  XTSC_LOG(m_text, ll,        " host_shared_memory      = "   << boolalpha << m_host_shared_memory);
  if (m_p_shmem) {
  if (shared_memory_name && shared_memory_name[0]) {
  XTSC_LOG(m_text, ll,        " shared_memory_name      = "   << m_shmem_name);
  } else {
  XTSC_LOG(m_text, ll,        " shared_memory_name      = \"\" => " << m_shmem_name);
  }
  XTSC_LOG(m_text, ll,        "   Bytes per row         = " << m_shmem_bytes_per_row);
  XTSC_LOG(m_text, ll,        "   Number of rows        = " << m_shmem_num_rows);
  XTSC_LOG(m_text, ll,        "   Byte offset of ridx   = " << m_shmem_array_size+0);
  XTSC_LOG(m_text, ll,        "   Byte offset of widx   = " << m_shmem_array_size+4);
  XTSC_LOG(m_text, ll,        "   Total bytes           = " << m_shmem_total_size);
  }

  reset();

}



xtsc_component::xtsc_queue::~xtsc_queue() {
  if (m_push_file) delete m_push_file;
  if (m_pop_file)  delete m_pop_file;
  if (m_element_ptrs) {
    for (u32 i=0; i<m_depth; i++) {
      delete m_element_ptrs[i];
    }
    delete [] m_element_ptrs;
  }
  if (m_tickets) delete [] m_tickets;
}



u32 xtsc_component::xtsc_queue::get_bit_width(const string& port_name, u32 interface_num) const {
  return m_width1;
}



sc_object *xtsc_component::xtsc_queue::get_port(const string& port_name) {
  string name_portion;
  u32    index;
  bool   indexed = xtsc_parse_port_name(port_name, name_portion, index);

  if ((name_portion == "m_producer") &&
      (index < m_num_producers) &&
      ((!indexed && (m_num_producers == 1)) || (indexed && (m_num_producers > 1))))
  {
    return m_producers[index];
  }

  if ((name_portion == "m_consumer") &&
      (index < m_num_consumers) &&
      ((!indexed && (m_num_consumers == 1)) || (indexed && (m_num_consumers > 1))))
  {
    return m_consumers[index];
  }
  
  ostringstream oss;
  oss << kind() << " \"" << name() << "\" has no port named \"" << port_name << "\"" << endl;
  throw xtsc_exception(oss.str());
}



xtsc_port_table xtsc_component::xtsc_queue::get_port_table(const string& port_table_name) const {
  xtsc_port_table table;
  string          name_portion;
  u32             index;
  bool            indexed = xtsc_parse_port_name(port_table_name, name_portion, index);

  if ((name_portion == "queue_push") &&
      (index < m_num_producers) &&
      ((!indexed && (m_num_producers == 1)) || (indexed && (m_num_producers > 1))))
  {
    table.push_back(m_producers[index]->basename());
  }
  else if ((name_portion == "queue_pop") &&
      (index < m_num_consumers) &&
      ((!indexed && (m_num_consumers == 1)) || (indexed && (m_num_consumers > 1))))
  {
    table.push_back(m_consumers[index]->basename());
  }
  else {
    ostringstream oss;
    oss << kind() << " \"" << name() << "\" has no port table named \"" << port_table_name << "\"" << endl;
    throw xtsc_exception(oss.str());
  }

  return table;
}



u32 xtsc_component::xtsc_queue::get_num_producers() {
  return m_num_producers;
}



u32 xtsc_component::xtsc_queue::get_num_consumers() {
  return m_num_consumers;
}



void xtsc_component::xtsc_queue::reset(bool /* hard_reset */) {
  XTSC_INFO(m_text, "xtsc_queue::reset()");
  m_pop_file_element            = 0;
  m_has_pop_file_element        = true;
  m_pop_file_line_number        = 0;
  m_next_word_index             = 0;

  m_words.clear();

  m_next       = 0;
  m_skid_index = 0;
  m_jerk_index = 0;

  if (m_push_file) {
    m_push_file->close();
    m_push_file->clear();
    m_push_file->open(m_push_file_name.c_str(), ios::out);
    if (!m_push_file->is_open()) {
      ostringstream oss;
      oss << "xtsc_queue '" << name() << "' reset() method cannot open push_file '" << m_push_file_name << "'.";
      throw xtsc_exception(oss.str());
    }
  }

  if (m_pop_file) {
    m_pop_file->reset();
    get_next_pop_file_element();
  }

  if (m_use_fifo) {
    m_drain_fifo_event.notify(SC_ZERO_TIME);
  }
  else if (m_p_shmem) {
    if (m_host_shared_memory && m_num_producers && m_num_consumers) {
      memset(m_p_shmem, 0, m_shmem_total_size);
    }
  }
}



// Runs when m_drain_fifo_event fires
void xtsc_component::xtsc_queue::drain_fifo_method() {
  int dummy = 0;
  while (m_fifo.nb_read(dummy));                 // Drain m_fifo dry
  if (m_multi_client) {
    for (u32 i=0; i < m_num_producers; ++i) {
      while (m_skid_fifos[i]->nb_read(dummy));   // Drain m_skid_fifos[i] dry
    }
    for (u32 i=0; i < m_num_consumers; ++i) {
      while (m_jerk_fifos[i]->nb_read(dummy));   // Drain m_jerk_fifos[i] dry
    }
  }
}



xtsc_queue_push_if& xtsc_component::xtsc_queue::get_queue_push_interface(u32 port) {
  if (port < m_num_producers) {
    if (m_multi_client) {
      return *m_push_multi_impl[port];
    }
    else {
      return *m_push_impl;
    }
  }
  ostringstream oss;
  oss << "xtsc_queue '" << name() << "' has no xtsc_queue_push_if port #" << port;
  throw xtsc_exception(oss.str());
}



xtsc_queue_pop_if& xtsc_component::xtsc_queue::get_queue_pop_interface(u32 port) {
  if (port < m_num_consumers) {
    if (m_multi_client) {
      return *m_pop_multi_impl[port];
    }
    else {
      return *m_pop_impl;
    }
  }
  ostringstream oss;
  oss << "xtsc_queue '" << name() << "' has no xtsc_queue_pop_if port #" << port;
  throw xtsc_exception(oss.str());
}



void xtsc_component::xtsc_queue::confirm_has_fifo_or_shmem(const char *function) {
  if (!m_use_fifo && !m_p_shmem) {
    ostringstream oss;
    oss << kind() << "::" << function << " called for '" << name() << "' which has neither sc_fifo nor host OS shared memory";
    throw xtsc_exception(oss.str());
  }
}



u32 xtsc_component::xtsc_queue::get_index_of_nth_from_front(u32 nth, const char *function) {
  confirm_has_fifo_or_shmem(function);
  if ((nth == 0) || (num_available() < nth)) {
    ostringstream oss;
    oss << kind() << "::" << function << " called for '" << name() << "' ";
    if (nth == 0) {
      oss << "with nth=0 which is not allowed (numbering starts at 1)";
    }
    if (num_available() < nth) {
      oss << "with nth (" << nth << ") larger than num_available (" << num_available() << ") which is not allowed";
    }
    throw xtsc_exception(oss.str());
  }
  if (m_p_shmem) {
    u32 ridx = *m_p_shmem_ridx;
    return ((ridx + nth - 1) % m_shmem_num_rows);
  }
  else {
    return ((m_next + m_depth - num_available() + nth - 1) % m_depth);
  }
}



void xtsc_component::xtsc_queue::dump(ostream &os) {
  confirm_has_fifo_or_shmem(__FUNCTION__);
  if (m_p_shmem) {
    for (u32 i = 0; i <= m_depth; ++i) {
      u32 row_offset = i * m_shmem_bytes_per_row;
      u64 ticket = *(u64*)&m_p_shmem[row_offset];
      xtsc_byte_array_to_sc_unsigned(&m_p_shmem[row_offset+8], m_value);
      os << "Row " << setw(3) << i << ":" << setw(6) << ticket << " 0x" << m_value.to_string(SC_HEX).substr(m_width1%4 ? 2 : 3) << endl;
    }
    os << "ridx=" << *m_p_shmem_ridx << " widx=" << *m_p_shmem_widx << endl;
  }
  else {
    for (u32 i = m_next + m_depth - m_fifo.num_available(); i < m_next + m_depth; ++i) {
      os << "0x" << m_element_ptrs[i%m_depth]->to_string(SC_HEX).substr(m_width1%4 ? 2 : 3) << "  " << m_tickets[i%m_depth] << endl;
    }
  }
}



u32 xtsc_component::xtsc_queue::num_available() {
  confirm_has_fifo_or_shmem(__FUNCTION__);
  if (m_p_shmem) {
    u32 widx = *m_p_shmem_widx;
    u32 ridx = *m_p_shmem_ridx;
    return ((widx >= ridx) ? (widx - ridx) : (widx + m_shmem_num_rows - ridx));
  }
  return m_fifo.num_available();
}



u32 xtsc_component::xtsc_queue::num_free() {
  confirm_has_fifo_or_shmem(__FUNCTION__);
  if (m_p_shmem) {
    return (m_shmem_num_rows - num_available() - 1);
  }
  return m_fifo.num_free();
}



void xtsc_component::xtsc_queue::peek(u32 nth, sc_unsigned& value) {
  u32 index = get_index_of_nth_from_front(nth, __FUNCTION__);
  if (m_p_shmem) {
    u32 row_offset = index * m_shmem_bytes_per_row;
    xtsc_byte_array_to_sc_unsigned(&m_p_shmem[row_offset+8], value);
  }
  else {
    value = *m_element_ptrs[index];
  }
}



void xtsc_component::xtsc_queue::poke(u32 nth, const sc_unsigned& value) {
  u32 index = get_index_of_nth_from_front(nth, __FUNCTION__);
  if (m_p_shmem) {
    u32 row_offset = index * m_shmem_bytes_per_row;
    xtsc_sc_unsigned_to_byte_array(value, &m_p_shmem[row_offset+8]);
  }
  else {
    *m_element_ptrs[index] = value;
  }
}



void xtsc_component::xtsc_queue::man(ostream& os) {
  os << " Unless it was configured with \"push_file\" or \"pop_file\" specified, xtsc_queue uses an internal FIFO or host OS" << endl;
  os << " shared memory for storage and many of the following commands can be used to query and/or manipulate it." << endl;
  os << " Tip: When entering a hex value for the poke/push commands, use a leading zero if desired to suppress sign extension:" << endl;
  os << "      e.g:   " << name() << " push 0x0FFF" << endl;
  os << endl;
}



void xtsc_component::xtsc_queue::execute(const string&          cmd_line, 
                                         const vector<string>&  words,
                                         const vector<string>&  words_lc,
                                         ostream&               result)
{
  ostringstream res;

  if (false) {
  }
  else if (words[0] == "can_pop") {
    u32 port = (words.size() <= 1) ?  0 : xtsc_command_argtou32(cmd_line, words, 1);
    res << get_queue_pop_interface(port).nb_can_pop();
  }
  else if (words[0] == "can_push") {
    u32 port = (words.size() <= 1) ?  0 : xtsc_command_argtou32(cmd_line, words, 1);
    res << get_queue_push_interface(port).nb_can_push();
  }
  else if (words[0] == "dump") {
    dump(res);
  }
  else if (words[0] == "get_bit_width") {
    res << get_bit_width();
  }
  else if (words[0] == "get_num_consumers") {
    res << get_num_consumers();
  }
  else if (words[0] == "get_num_producers") {
    res << get_num_producers();
  }
  else if (words[0] == "get_pop_ticket") {
    res << m_pop_ticket;
  }
  else if (words[0] == "get_push_ticket") {
    res << m_push_ticket;
  }
  else if (words[0] == "num_available") {
    res << num_available();
  }
  else if (words[0] == "num_free") {
    res << num_free();
  }
  else if (words[0] == "peek") {
    u32 nth = xtsc_command_argtou32(cmd_line, words, 1);
    peek(nth, m_value);
    res << m_value.to_string(SC_HEX).substr(m_width1%4 ? 2 : 3);
  }
  else if (words[0] == "poke") {
    u32 nth = xtsc_command_argtou32(cmd_line, words, 1);
    peek(nth, m_value);
    res << "0x" << m_value.to_string(SC_HEX).substr(m_width1%4 ? 2 : 3);
    m_value = words[2].c_str();
    poke(nth, m_value);
  }
  else if (words[0] == "pop") {
    u32 port = (words.size() <= 1) ?  0 : xtsc_command_argtou32(cmd_line, words, 1);
    if (!get_queue_pop_interface(port).nb_pop(m_value)) {
      ostringstream oss;
      oss << "xtsc_queue '" << name() << "': nb_pop() returned false on port #" << port;
      throw xtsc_exception(oss.str());
    }
    res << "0x" << m_value.to_string(SC_HEX).substr(m_width1%4 ? 2 : 3);
  }
  else if (words[0] == "push") {
    m_value = words[1].c_str();
    u32 port = (words.size() <= 2) ?  0 : xtsc_command_argtou32(cmd_line, words, 2);
    res << get_queue_push_interface(port).nb_push(m_value);
  }
  else if (words[0] == "reset") {
    reset();
  }
  else {
    ostringstream oss;
    oss << "xtsc_queue::execute called for unknown command '" << cmd_line << "'.";
    throw xtsc_exception(oss.str());
  }

  result << res.str();
}



void xtsc_component::xtsc_queue::connect(xtsc::xtsc_core& core, const char *queue_name, u32 port_num) {
  if (core.has_input_queue(queue_name)) {
    if (port_num >= m_num_consumers) {
      ostringstream oss;
      oss << "xtsc_queue::connect called with port_num=" << port_num << " which is not strictly less than " << m_num_consumers
          << " (from \"m_num_consumers\")"; 
      throw xtsc_exception(oss.str());
    }
    core.get_input_queue(queue_name)(m_multi_client ? *m_consumers[port_num] : m_consumer);
  }
  else if (core.has_output_queue(queue_name)) {
    if (port_num >= m_num_producers) {
      ostringstream oss;
      oss << "xtsc_queue::connect called with port_num=" << port_num << " which is not strictly less than " << m_num_producers
          << " (from \"m_num_producers\")"; 
      throw xtsc_exception(oss.str());
    }
    core.get_output_queue(queue_name)(m_multi_client ? *m_producers[port_num] : m_producer);
  }
  else {
    ostringstream oss;
    oss << "xtsc_queue::connect: core '" << core.name() << "' has no input/output queue interface named '" << queue_name << "'.";
    throw xtsc_exception(oss.str());
  }
}



void xtsc_component::xtsc_queue::connect(xtsc_queue_producer& producer, u32 port_num) {
  if (port_num >= m_num_producers) {
    ostringstream oss;
    oss << "xtsc_queue::connect called with port_num=" << port_num << " which is not strictly less than " << m_num_producers
        << " (from \"m_num_producers\")"; 
    throw xtsc_exception(oss.str());
  }
  producer.m_queue(m_multi_client ? *m_producers[port_num] : m_producer);
}



void xtsc_component::xtsc_queue::connect(xtsc_queue_consumer& consumer, u32 port_num) {
  if (port_num >= m_num_consumers) {
    ostringstream oss;
    oss << "xtsc_queue::connect called with port_num=" << port_num << " which is not strictly less than " << m_num_consumers
        << " (from \"m_num_consumers\")"; 
    throw xtsc_exception(oss.str());
  }
  consumer.m_queue(m_multi_client ? *m_consumers[port_num] : m_consumer);
}



void xtsc_component::xtsc_queue::delta_cycle_method() {
  XTSC_DEBUG(m_text, "delta_cycle_method()");

  for (u32 limit = m_skid_index + m_num_producers; ((m_skid_index < limit) && (m_fifo.num_free() != 0)); ++m_skid_index) {
    u32 port = m_skid_index % m_num_producers;
    if (m_skid_fifos[port]->num_available() != 0) {
      int dummy = 0;
      m_skid_fifos[port]->nb_read(dummy);
      m_fifo.nb_write(m_next);
      m_tickets[m_next] = m_skid_tickets[port];
      *m_element_ptrs[m_next] = *m_skid_buffers[port];
      m_next = (m_next + 1) % m_depth;
      m_nonfull_events[port]->notify(SC_ZERO_TIME);
      m_push_pop_event.notify(SC_ZERO_TIME);
      XTSC_DEBUG(m_text, "delta_cycle_method() moved from skid buffer #" << port << " to fifo: 0x" << hex << *m_skid_buffers[port]);
    }
  }
  m_skid_index = m_skid_index % m_num_producers;

  for (u32 limit = m_jerk_index + m_num_consumers; ((m_jerk_index < limit) && (m_fifo.num_available() != 0)); ++m_jerk_index) {
    u32 port = m_jerk_index % m_num_consumers;
    if (m_jerk_fifos[port]->num_free() != 0) {
      int dummy = 0;
      m_jerk_fifos[port]->nb_write(dummy);
      int index = 0;
      m_fifo.nb_read(index);
      m_jerk_tickets[port] = m_tickets[index];
      *m_jerk_buffers[port] = *m_element_ptrs[index];
      m_nonempty_events[port]->notify(SC_ZERO_TIME);
      m_push_pop_event.notify(SC_ZERO_TIME);
      XTSC_DEBUG(m_text, "delta_cycle_method() moved from fifo to jerk buffer #" << port << ": 0x" << hex << *m_jerk_buffers[port]);
    }
  }
  m_jerk_index = m_jerk_index % m_num_consumers;

}




void xtsc_component::xtsc_queue::get_next_pop_file_element() {
  if (!m_has_pop_file_element) {
    return;
  }
  if (m_next_word_index >= m_words.size()) {
    m_pop_file_line_number = m_pop_file->get_words(m_words, m_line);
    if (!m_pop_file_line_number) {
      m_has_pop_file_element = false;
      return;
    }
    m_next_word_index = 0;
  }
  try {
    m_pop_file_element = m_words[m_next_word_index].c_str();
  }
  catch (...) {
    ostringstream oss;
    oss << "Cannot convert word #" << (m_next_word_index+1) << " (\"" << m_words[m_next_word_index] << "\") to number:" << endl;
    oss << m_line;
    oss << m_pop_file->info_for_exception();
    throw xtsc_exception(oss.str());
  }
  m_next_word_index += 1;
}



xtsc::u32 xtsc_component::xtsc_queue::xtsc_queue_push_if_impl::nb_num_free_spaces() {
  u32 num_free = 0;
  if (m_queue.m_use_fifo) {
    num_free = m_queue.m_fifo.num_free();
  }
  else if (m_queue.m_p_shmem) {
    // num_free = (((*m_queue.m_p_shmem_widx + 1) % m_queue.m_shmem_num_rows) != *m_queue.m_p_shmem_ridx);   // not full
    num_free = (((*m_queue.m_p_shmem_widx + 1) % m_queue.m_shmem_num_rows) != *m_queue.m_p_shmem_ridx) ? 1 : 0; // TODO
  }
  else {
    if (m_queue.m_push_file) {
      num_free = 1;  // TODO:  Set by parameter and/or XTSC cmd?
    }
    else {
      ostringstream oss;
      oss << "nb_num_free_spaces() called for xtsc_queue '" << m_queue.name() << "', but no push_file was provided at construction time.";
      throw xtsc_exception(oss.str());
    }
  }
  XTSC_DEBUG(m_queue.m_text, "nb_num_free_spaces()=" << num_free);
  return num_free;
}



bool xtsc_component::xtsc_queue::xtsc_queue_push_if_impl::nb_can_push() {
  bool can_push = false;
  if (m_queue.m_use_fifo) {
    if (m_queue.m_fifo.num_free() != 0) {
      can_push = true;
    }
    else {
      xtsc_log_queue_event(m_queue.m_binary, INFO_LOG_LEVEL, 0, UNKNOWN, PUSH_FAILED, UNKNOWN_PC, m_queue.m_fifo.num_available(),
                           m_queue.m_depth, false, m_queue.m_dummy);
      can_push = false;
    }
  }
  else if (m_queue.m_p_shmem) {
    can_push = (((*m_queue.m_p_shmem_widx + 1) % m_queue.m_shmem_num_rows) != *m_queue.m_p_shmem_ridx);   // not full
  }
  else {
    if (m_queue.m_push_file) {
      can_push = true;
    }
    else {
      ostringstream oss;
      oss << "nb_can_push() called for xtsc_queue '" << m_queue.name() << "', but no push_file was provided at construction time.";
      throw xtsc_exception(oss.str());
    }
  }
  XTSC_DEBUG(m_queue.m_text, "nb_can_push()=" << boolalpha << can_push);
  return can_push;
}



xtsc::u32 xtsc_component::xtsc_queue::xtsc_queue_pop_if_impl::nb_num_available_entries() {
  u32 num_available = 0;
  if (m_queue.m_use_fifo) {
    num_available = m_queue.m_fifo.num_available();
  }
  else if (m_queue.m_p_shmem) {
    // num_available = (*m_queue.m_p_shmem_widx != *m_queue.m_p_shmem_ridx);   // not empty
    num_available = ((*m_queue.m_p_shmem_widx != *m_queue.m_p_shmem_ridx) ? 1 : 0);       // TODO
  }
  else {
    if (m_queue.m_pop_file) {
      num_available = (m_queue.m_has_pop_file_element ? 1 : 0); // TODO:  read ahead and count?
    }
    else {
      ostringstream oss;
      oss << "nb_num_available_entries() called for xtsc_queue '" << m_queue.name() << "', but no pop_file was provided at construction time.";
      throw xtsc_exception(oss.str());
    }
  }
  XTSC_DEBUG(m_queue.m_text, "nb_num_available_entries()=" << num_available);
  return num_available;
}



bool xtsc_component::xtsc_queue::xtsc_queue_pop_if_impl::nb_can_pop() {
  bool can_pop = false;
  if (m_queue.m_use_fifo) {
    if (m_queue.m_fifo.num_available() != 0) {
      can_pop = true;
    }
    else {
      xtsc_log_queue_event(m_queue.m_binary, INFO_LOG_LEVEL, 0, UNKNOWN, POP_FAILED, UNKNOWN_PC, m_queue.m_fifo.num_available(),
                           m_queue.m_depth, false, m_queue.m_dummy);
      can_pop = false;
    }
  }
  else if (m_queue.m_p_shmem) {
    can_pop = (*m_queue.m_p_shmem_widx != *m_queue.m_p_shmem_ridx);   // not empty
  }
  else {
    if (m_queue.m_pop_file) {
      can_pop = m_queue.m_has_pop_file_element;
    }
    else {
      ostringstream oss;
      oss << "nb_can_pop() called for xtsc_queue '" << m_queue.name() << "', but no pop_file was provided at construction time.";
      throw xtsc_exception(oss.str());
    }
  }
  XTSC_DEBUG(m_queue.m_text, "nb_can_pop()=" << boolalpha << can_pop);
  return can_pop;
}



bool xtsc_component::xtsc_queue::xtsc_queue_push_if_impl::nb_push(const sc_unsigned& element, u64& ticket) {
  if (static_cast<u32>(element.length()) != m_queue.m_width1) {
    ostringstream oss;
    oss << "ERROR: Element of width=" << element.length() << " bits added to queue '" << m_queue.name() << "' of width="
        << m_queue.m_width1;
    throw xtsc_exception(oss.str());
  }
  if (m_queue.m_use_fifo) {
    if (!m_queue.m_fifo.nb_write(m_queue.m_next)) {
      xtsc_log_queue_event(m_queue.m_binary, INFO_LOG_LEVEL, 0, UNKNOWN, PUSH_FAILED, UNKNOWN_PC, m_queue.m_fifo.num_available(),
                           m_queue.m_depth, m_queue.m_log_data_binary, element);
      return false;
    }
    ticket = xtsc_create_queue_ticket();
    m_queue.m_push_ticket = ticket;
    m_queue.m_tickets[m_queue.m_next] = ticket;
    *m_queue.m_element_ptrs[m_queue.m_next] = element;
    XTSC_INFO(m_queue.m_text, "Pushed (ticket=" << ticket << " cnt=" << m_queue.m_fifo.num_available()+1 <<
                              "): 0x" << element.to_string(SC_HEX).substr(m_queue.m_width1%4 ? 2 : 3));
    xtsc_log_queue_event(m_queue.m_binary, INFO_LOG_LEVEL, ticket, UNKNOWN, PUSH, UNKNOWN_PC, m_queue.m_fifo.num_available()+1,
                         m_queue.m_depth, m_queue.m_log_data_binary, element);
    m_queue.m_next = (m_queue.m_next + 1) % m_queue.m_depth;
    m_queue.m_push_pop_event.notify(SC_ZERO_TIME);
    // whenever the number available is 0 (i.e. it was empty), then notify one delta cycle later that it is no longer empty
    if (m_queue.m_fifo.num_available() == 0) {
      m_queue.m_nonempty_event.notify(SC_ZERO_TIME);
    }
    return true;
  }
  else if (m_queue.m_p_shmem) {
    u32  widx = *m_queue.m_p_shmem_widx;
    u32  ridx = *m_queue.m_p_shmem_ridx;
    bool full = (((widx + 1) % m_queue.m_shmem_num_rows) == ridx);
    if (full) { return false; }
    bool empty = (ridx == widx);
    u32 row_offset = widx * m_queue.m_shmem_bytes_per_row;
    ticket = xtsc_create_queue_ticket();
    m_queue.m_push_ticket = ticket;
    *(u64*)&m_queue.m_p_shmem[row_offset] = ticket;
    xtsc_sc_unsigned_to_byte_array(element, &m_queue.m_p_shmem[row_offset+8]);
    XTSC_INFO(m_queue.m_text, "Pushed (ticket=" << ticket << " ridx=" << ridx << " widx=" << widx <<
                              "): 0x" << element.to_string(SC_HEX).substr(m_queue.m_width1%4 ? 2 : 3));
    *m_queue.m_p_shmem_widx = (widx + 1) % m_queue.m_shmem_num_rows;
    m_queue.m_push_pop_event.notify(SC_ZERO_TIME);
    if (empty) {
      m_queue.m_nonempty_event.notify(SC_ZERO_TIME);
    }
    return true;
  }
  else {
    ticket = 0ULL;
    m_queue.m_push_ticket = ticket;
    if (m_queue.m_push_file) {
      ostringstream oss;
      oss << "0x" << element.to_string(SC_HEX).substr(m_queue.m_width1%4 ? 2 : 3);
      if (m_queue.m_timestamp) {
        string buf;
        oss << " // " << setprecision(xtsc_get_text_logging_time_precision()) << fixed << setw(xtsc_get_text_logging_time_width())
                      << (sc_core::sc_time_stamp() / xtsc_get_system_clock_period()) << xtsc_log_delta_cycle(buf);
      }
      *m_queue.m_push_file << oss.str() << endl;
      XTSC_INFO(m_queue.m_text, "Pushed to file " << oss.str());
      return true;
    }
    else {
      ostringstream oss;
      oss << "nb_push() called for xtsc_queue '" << m_queue.name() << "', but no push_file was provided at construction time.";
      throw xtsc_exception(oss.str());
    }
  }
}



bool xtsc_component::xtsc_queue::xtsc_queue_pop_if_impl::nb_pop(sc_unsigned& element, u64& ticket) {
  if (m_queue.m_use_fifo) {
    int index = 0;
    if (!m_queue.m_fifo.nb_read(index)) {
      xtsc_log_queue_event(m_queue.m_binary, INFO_LOG_LEVEL, 0, UNKNOWN, POP_FAILED, UNKNOWN_PC, m_queue.m_fifo.num_available(),
                           m_queue.m_depth, false, m_queue.m_dummy);
      return false;
    }
    element = *m_queue.m_element_ptrs[index];
    ticket = m_queue.m_tickets[index];
    m_queue.m_pop_ticket = ticket;
    XTSC_INFO(m_queue.m_text, "Popped (ticket=" << ticket << " cnt=" << m_queue.m_fifo.num_available() <<
                               "): 0x" << element.to_string(SC_HEX).substr(m_queue.m_width1%4 ? 2 : 3));
    xtsc_log_queue_event(m_queue.m_binary, INFO_LOG_LEVEL, ticket, UNKNOWN, POP, UNKNOWN_PC, m_queue.m_fifo.num_available(),
                         m_queue.m_depth, m_queue.m_log_data_binary, element);
    m_queue.m_push_pop_event.notify(SC_ZERO_TIME);
    // whenever the number free is 0 (i.e. it was full), then notify one delta cycle later that it is no longer full
    if (m_queue.m_fifo.num_free() == 0) {
      m_queue.m_nonfull_event.notify(SC_ZERO_TIME);
    }
    return true;
  }
  else if (m_queue.m_p_shmem) {
    u32  widx  = *m_queue.m_p_shmem_widx;
    u32  ridx  = *m_queue.m_p_shmem_ridx;
    bool empty = (ridx == widx);
    if (empty) { return false; }
    bool full = (((widx + 1) % m_queue.m_shmem_num_rows) == ridx);
    u32 row_offset = ridx * m_queue.m_shmem_bytes_per_row;
    ticket = *(u64*)&m_queue.m_p_shmem[row_offset];
    m_queue.m_pop_ticket = ticket;
    xtsc_byte_array_to_sc_unsigned(&m_queue.m_p_shmem[row_offset+8], element);
    XTSC_INFO(m_queue.m_text, "Popped (ticket=" << ticket << " ridx=" << ridx << " widx=" << widx <<
                              "): 0x" << element.to_string(SC_HEX).substr(m_queue.m_width1%4 ? 2 : 3));
    *m_queue.m_p_shmem_ridx = (ridx + 1) % m_queue.m_shmem_num_rows;
    m_queue.m_push_pop_event.notify(SC_ZERO_TIME);
    if (full) {
      m_queue.m_nonfull_event.notify(SC_ZERO_TIME);
    }
    return true;
  }
  else {
    ticket = 0ULL;
    m_queue.m_pop_ticket = ticket;
    if (m_queue.m_pop_file) {
      if (m_queue.m_has_pop_file_element) {
        element = m_queue.m_pop_file_element;
        XTSC_INFO(m_queue.m_text, "Popped from file 0x" << element.to_string(SC_HEX).substr(m_queue.m_width1%4 ? 2 : 3));
        m_queue.get_next_pop_file_element();
        return true;
      }
      else {
        return false;
      }
    }
    else {
      ostringstream oss;
      oss << "nb_pop() called for xtsc_queue '" << m_queue.name() << "', but no pop_file was provided at construction time.";
      throw xtsc_exception(oss.str());
    }
  }
}



void xtsc_component::xtsc_queue::xtsc_queue_pop_if_impl::nb_peek(sc_unsigned& element, u64& ticket) {
  if (m_queue.m_use_fifo) {
    if (!m_queue.m_fifo.num_available()) {
      return;
    }
    u32 index = m_queue.get_index_of_nth_from_front(1, __FUNCTION__);
    element = *m_queue.m_element_ptrs[index];
    ticket = m_queue.m_tickets[index];
    m_queue.m_pop_ticket = ticket;
    XTSC_INFO(m_queue.m_text, "Peeked (ticket=" << ticket << " cnt=" << m_queue.m_fifo.num_available() <<
                               "): 0x" << element.to_string(SC_HEX).substr(m_queue.m_width1%4 ? 2 : 3));
    return;
  }
  else if (m_queue.m_p_shmem) {
    u32  widx  = *m_queue.m_p_shmem_widx;
    u32  ridx  = *m_queue.m_p_shmem_ridx;
    bool empty = (ridx == widx);
    if (empty) { return; }
    u32 row_offset = ridx * m_queue.m_shmem_bytes_per_row;
    ticket = *(u64*)&m_queue.m_p_shmem[row_offset];
    m_queue.m_pop_ticket = ticket;
    xtsc_byte_array_to_sc_unsigned(&m_queue.m_p_shmem[row_offset+8], element);
    XTSC_INFO(m_queue.m_text, "Peeked (ticket=" << ticket << " ridx=" << ridx << " widx=" << widx <<
                              "): 0x" << element.to_string(SC_HEX).substr(m_queue.m_width1%4 ? 2 : 3));
    return;
  }
  else {
    ticket = 0ULL;
    m_queue.m_pop_ticket = ticket;
    if (m_queue.m_pop_file) {
      if (m_queue.m_has_pop_file_element) {
        element = m_queue.m_pop_file_element;
        XTSC_INFO(m_queue.m_text, "Peeked from file 0x" << element.to_string(SC_HEX).substr(m_queue.m_width1%4 ? 2 : 3));
        return;
      }
      else {
        return;
      }
    }
    else {
      ostringstream oss;
      oss << "nb_peek() called for xtsc_queue '" << m_queue.name() << "', but no pop_file was provided at construction time.";
      throw xtsc_exception(oss.str());
    }
  }
}



void xtsc_component::xtsc_queue::xtsc_queue_push_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_queue '" << m_queue.name() << "' m_producer export: " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_queue.m_text, "Binding '" << port.name() << "' to xtsc_queue::m_producer");
  m_p_port = &port;
}



void xtsc_component::xtsc_queue::xtsc_queue_pop_if_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to xtsc_queue '" << m_queue.name() << "' m_consumer export: " << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_queue.m_text, "Binding '" << port.name() << "' to xtsc_queue::m_consumer");
  m_p_port = &port;
}



xtsc::u32 xtsc_component::xtsc_queue::xtsc_queue_push_if_multi_impl::nb_num_free_spaces() {
  u32 num_free = 0;
  if (m_queue.m_use_fifo) {
    num_free = m_queue.m_skid_fifos[m_port_num]->num_free();
  }
  else {
    if (m_queue.m_push_file) {
      num_free = 1; // TODO:  parameter and/or XTSC cmd
    }
    else {
      ostringstream oss;
      oss << "nb_num_free_spaces() called for xtsc_queue '" << m_queue.name() << "', but no push_file was provided at construction time.";
      throw xtsc_exception(oss.str());
    }
  }
  XTSC_DEBUG(m_queue.m_text, "nb_num_free_spaces()=" << num_free << " (m_port_num=" << m_port_num << ")");
  return num_free;
}



bool xtsc_component::xtsc_queue::xtsc_queue_push_if_multi_impl::nb_can_push() {
  bool can_push = false;
  if (m_queue.m_use_fifo) {
    if (m_queue.m_skid_fifos[m_port_num]->num_free() != 0) {
      can_push = true;
    }
    else {
      xtsc_log_queue_event(m_queue.m_binary, INFO_LOG_LEVEL, 0, UNKNOWN, PUSH_FAILED, UNKNOWN_PC, m_queue.m_fifo.num_available(),
                           m_queue.m_depth, false, m_queue.m_dummy);
      can_push = false;
    }
  }
  else {
    if (m_queue.m_push_file) {
      can_push = true;
    }
    else {
      ostringstream oss;
      oss << "nb_can_push() called for xtsc_queue '" << m_queue.name() << "', but no push_file was provided at construction time.";
      throw xtsc_exception(oss.str());
    }
  }
  XTSC_DEBUG(m_queue.m_text, "nb_can_push()=" << boolalpha << can_push << " (m_port_num=" << m_port_num << ")");
  return can_push;
}



bool xtsc_component::xtsc_queue::xtsc_queue_push_if_multi_impl::nb_push(const sc_unsigned& element, u64& ticket) {
  if (static_cast<u32>(element.length()) != m_queue.m_width1) {
    ostringstream oss;
    oss << "ERROR: Element of width=" << element.length() << " bits added to queue '" << m_queue.name() << "' of width="
        << m_queue.m_width1;
    throw xtsc_exception(oss.str());
  }
  if (m_queue.m_use_fifo) {
    if (!nb_can_push()) {
      xtsc_log_queue_event(m_queue.m_binary, INFO_LOG_LEVEL, 0, UNKNOWN, PUSH_FAILED, UNKNOWN_PC, m_queue.m_fifo.num_available(),
                           m_queue.m_depth, m_queue.m_log_data_binary, element);
      return false;
    }
    ticket = xtsc_create_queue_ticket();
    m_queue.m_skid_tickets[m_port_num] = ticket;
    m_queue.m_skid_fifos[m_port_num]->nb_write(0);
    *m_queue.m_skid_buffers[m_port_num] = element;
    XTSC_INFO(m_queue.m_text, "Pushed (ticket=" << ticket << " cnt=" << m_queue.m_fifo.num_available()+1 << "): 0x" <<
                              element.to_string(SC_HEX).substr(m_queue.m_width1%4 ? 2 : 3) << " (Port #" << m_port_num << ")");
    xtsc_log_queue_event(m_queue.m_binary, INFO_LOG_LEVEL, ticket, UNKNOWN, PUSH, UNKNOWN_PC, m_queue.m_fifo.num_available()+1,
                         m_queue.m_depth, m_queue.m_log_data_binary, element);
    m_queue.m_push_pop_event.notify(SC_ZERO_TIME);
    return true;
  }
  else {
    ticket = 0ULL;
    if (m_queue.m_push_file) {
      ostringstream oss;
      oss << "0x" << element.to_string(SC_HEX).substr(m_queue.m_width1%4 ? 2 : 3);
      if (m_queue.m_timestamp) {
        string buf;
        oss << " // " << setprecision(xtsc_get_text_logging_time_precision()) << fixed << setw(xtsc_get_text_logging_time_width())
                      << (sc_core::sc_time_stamp() / xtsc_get_system_clock_period()) << xtsc_log_delta_cycle(buf);
      }
      *m_queue.m_push_file << oss.str() << endl;
      XTSC_INFO(m_queue.m_text, "Pushed to file " << oss.str() << " (Port #" << m_port_num << ")");
      return true;
    }
    else {
      ostringstream oss;
      oss << "nb_push() called for xtsc_queue '" << m_queue.name() << "', but no push_file was provided at construction time.";
      throw xtsc_exception(oss.str());
    }
  }
}



void xtsc_component::xtsc_queue::xtsc_queue_push_if_multi_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to '" << m_queue.name() << "' xtsc_queue::m_producers[" << m_port_num << "] export: "
        << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_queue.m_text, "Binding '" << port.name() << "' to xtsc_queue::m_producers[" << m_port_num << "]");
  m_p_port = &port;
}



xtsc::u32 xtsc_component::xtsc_queue::xtsc_queue_pop_if_multi_impl::nb_num_available_entries() {
  u32 num_available = 0;
  if (m_queue.m_use_fifo) {
    num_available = m_queue.m_jerk_fifos[m_port_num]->num_available();
  }
  else {
    if (m_queue.m_pop_file) {
      num_available = (m_queue.m_has_pop_file_element ? 1 : 0); // TODO: read ahead and count?
    }
    else {
      ostringstream oss;
      oss << "nb_num_available_entries() called for xtsc_queue '" << m_queue.name() << "', but no pop_file was provided at construction time.";
      throw xtsc_exception(oss.str());
    }
  }
  XTSC_DEBUG(m_queue.m_text, "nb_num_available_entries()=" << num_available << " (m_port_num=" << m_port_num << ")");
  return num_available;
}



bool xtsc_component::xtsc_queue::xtsc_queue_pop_if_multi_impl::nb_can_pop() {
  bool can_pop = false;
  if (m_queue.m_use_fifo) {
    if (m_queue.m_jerk_fifos[m_port_num]->num_available() != 0) {
      can_pop = true;
    }
    else {
      xtsc_log_queue_event(m_queue.m_binary, INFO_LOG_LEVEL, 0, UNKNOWN, POP_FAILED, UNKNOWN_PC, m_queue.m_fifo.num_available(),
                           m_queue.m_depth, false, m_queue.m_dummy);
    }
  }
  else {
    if (m_queue.m_pop_file) {
      can_pop = m_queue.m_has_pop_file_element;
    }
    else {
      ostringstream oss;
      oss << "nb_can_pop() called for xtsc_queue '" << m_queue.name() << "', but no pop_file was provided at construction time.";
      throw xtsc_exception(oss.str());
    }
  }
  XTSC_DEBUG(m_queue.m_text, "nb_can_pop()=" << boolalpha << can_pop << " (m_port_num=" << m_port_num << ")");
  return can_pop;
}



bool xtsc_component::xtsc_queue::xtsc_queue_pop_if_multi_impl::nb_pop(sc_unsigned& element, u64& ticket) {
  if (m_queue.m_use_fifo) {
    if (!nb_can_pop()) {
      xtsc_log_queue_event(m_queue.m_binary, INFO_LOG_LEVEL, 0, UNKNOWN, POP_FAILED, UNKNOWN_PC, m_queue.m_fifo.num_available(),
                           m_queue.m_depth, false, m_queue.m_dummy);
      return false;
    }
    int dummy = 0;
    m_queue.m_jerk_fifos[m_port_num]->nb_read(dummy);
    element = *m_queue.m_jerk_buffers[m_port_num];
    ticket  =  m_queue.m_jerk_tickets[m_port_num];
    XTSC_INFO(m_queue.m_text, "Popped (ticket=" << ticket << "): 0x" << element.to_string(SC_HEX).substr(m_queue.m_width1%4 ? 2 : 3) <<
                              " (Port #" << m_port_num << ")");
    xtsc_log_queue_event(m_queue.m_binary, INFO_LOG_LEVEL, ticket, UNKNOWN, POP, UNKNOWN_PC, m_queue.m_fifo.num_available(),
                         m_queue.m_depth, m_queue.m_log_data_binary, element);
    m_queue.m_push_pop_event.notify(SC_ZERO_TIME);
    return true;
  }
  else {
    ticket = 0ULL;
    if (m_queue.m_pop_file) {
      if (m_queue.m_has_pop_file_element) {
        element = m_queue.m_pop_file_element;
        XTSC_INFO(m_queue.m_text, "Popped from file 0x" << element.to_string(SC_HEX).substr(m_queue.m_width1%4 ? 2 : 3) <<
                                  " (Port #" << m_port_num << ")");
        m_queue.get_next_pop_file_element();
        return true;
      }
      else {
        return false;
      }
    }
    else {
      ostringstream oss;
      oss << "nb_pop() called for xtsc_queue '" << m_queue.name() << "', but no pop_file was provided at construction time.";
      throw xtsc_exception(oss.str());
    }
  }
}



void xtsc_component::xtsc_queue::xtsc_queue_pop_if_multi_impl::register_port(sc_port_base& port, const char *if_typename) {
  if (m_p_port) {
    ostringstream oss;
    oss << "Illegal multiple binding detected to '" << m_queue.name() << "' xtsc_queue::m_consumers[" << m_port_num << "] export: "
        << endl;
    oss << "  " << port.name() << endl;
    oss << "  " << m_p_port->name();
    throw xtsc_exception(oss.str());
  }
  XTSC_INFO(m_queue.m_text, "Binding '" << port.name() << "' to xtsc_queue::m_consumers[" << m_port_num << "]");
  m_p_port = &port;
}



