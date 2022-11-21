#ifndef _XTSC_QUEUE_POP_IF_VP_MON_H_
#define _XTSC_QUEUE_POP_IF_VP_MON_H_

// Copyright (c) 2006-2011 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

/**
 * @file 
 */

#include <xtsc_vp/xtsc_vp.h>
#include <xtsc/xtsc_queue_pop_if.h>



namespace xtsc_vp {



/**
 * Parameters for an xtsc_queue_pop_if_vp_mon.
 *
 * Mainly this is to allow overriding at run time using the extract_parms mechanism.
 */
class XTSC_VP_API xtsc_queue_pop_if_vp_mon_parms : public xtsc::xtsc_parms {
public:

  xtsc_queue_pop_if_vp_mon_parms(bool enable_tracing, const char *vcd_name) {
    add("enable_tracing",       enable_tracing);
    add("vcd_name",             vcd_name);
  }

  virtual const char* kind() const { return "xtsc_queue_pop_if_vp_mon_parms"; }

};



/**
 * This module allows hardware watchpoints and tracing on an XTSC queue pop interface.
 *
 * This module allows VPA hardware watchpoints to be place on an XTSC queue pop interface
 * as well as waveform tracing of the interface.  The actual XTSC interface involved is
 * xtsc::xtsc_queue_pop_if.
 *
 * @Note If data width exceeds 256 bits, then the data will not be visible in VPA (as of
 * V2010.1.1).
 */
template <unsigned int DATA_WIDTH = 32> 
class xtsc_queue_pop_if_vp_mon : public sc_module {
public:

  typedef std::ostringstream            ostringstream;
  typedef xtsc::xtsc_queue_pop_if       xtsc_queue_pop_if;
  typedef xtsc::xtsc_exception          xtsc_exception;
  typedef xtsc::u64                     u64;
  typedef xtsc::u32                     u32;

  sc_export<xtsc_queue_pop_if>          m_queue_pop_export;             ///< Receive the pop from downstream consumer
  sc_port<xtsc_queue_pop_if, NSPP>      m_queue_pop_port;               ///< Forward the pop to upstream queue
      
  sc_out<u64>                           m_num_nb_can_pop_true;          ///< Total calls to nb_can_pop that returned true
  sc_out<u64>                           m_num_nb_can_pop_false;         ///< Total calls to nb_can_pop that returned false
  sc_out<u64>                           m_num_nb_pop_true;              ///< Total calls to nb_pop that returned true
  sc_out<u64>                           m_num_nb_pop_false;             ///< Total calls to nb_pop that returned false
  sc_out<u64>                           m_num_nonempty_events;          ///< Total times the nonempty event has been notified

  sc_out<u64>                           m_ticket;                       ///< Last queue ticket
  sc_out<sc_biguint<DATA_WIDTH> >       m_element;                      ///< Last element popped


  SC_HAS_PROCESS(xtsc_queue_pop_if_vp_mon);



#define XTSC_QUEUE_POP_IF_VP_MON_INITIALIZERS                                           \
    sc_module                     (module_name),                                        \
    m_queue_pop_export            ("m_queue_pop_export"),                               \
    m_queue_pop_port              ("m_queue_pop_port"),                                 \
    m_num_nb_can_pop_true         ("m_num_nb_can_pop_true"),                            \
    m_num_nb_can_pop_false        ("m_num_nb_can_pop_false"),                           \
    m_num_nb_pop_true             ("m_num_nb_pop_true"),                                \
    m_num_nb_pop_false            ("m_num_nb_pop_false"),                               \
    m_num_nonempty_events         ("m_num_nonempty_events"),                            \
    m_ticket                      ("m_ticket"),                                         \
    m_element                     ("m_element"),                                        \
    m_queue_pop_impl              ("m_queue_pop_impl", *this),                          \
    m_enable_tracing              ("/Trace/enable_tracing", false),                     \
    m_vcd_name                    ("/Trace/vcd_name",       ""),                        \
    m_p_trace_file                (0),                                                  \
    m_num_nb_can_pop_true_cnt     (0),                                                  \
    m_num_nb_can_pop_false_cnt    (0),                                                  \
    m_num_nb_pop_true_cnt         (0),                                                  \
    m_num_nb_pop_false_cnt        (0),                                                  \
    m_num_nonempty_events_cnt     (0),                                                  \
    m_connected                   (true),                                               \
    m_text                        (log4xtensa::TextLogger::getInstance(name()))         \



  // ctor for "external" monitor (outside of xtsc_core_vp and visible in Platform Creator)
  xtsc_queue_pop_if_vp_mon(const sc_module_name &module_name) :
    XTSC_QUEUE_POP_IF_VP_MON_INITIALIZERS
  {
    xtsc_vp_initialize();

    string vcd_name = m_vcd_name;
    xtsc_queue_pop_if_vp_mon_parms parms(m_enable_tracing, vcd_name.c_str());
    ctor_helper(parms, false);
  }



  // ctor for "internal" monitor (inside of xtsc_core_vp and not visible in Platform Creator)
  xtsc_queue_pop_if_vp_mon(const sc_module_name &module_name, const string& vcd_name, bool connected = true) :
    XTSC_QUEUE_POP_IF_VP_MON_INITIALIZERS
  {
    xtsc_vp_initialize();

    xtsc_queue_pop_if_vp_mon_parms parms((vcd_name != ""), vcd_name.c_str());
    m_connected = connected;
    ctor_helper(parms, true);
  }



  void ctor_helper(xtsc_queue_pop_if_vp_mon_parms& parms, bool internal) {
    if (m_connected) {
      parms.extract_parms(sc_argc(), sc_argv(), name());
    }
    m_enable_tracing = parms.get_bool("enable_tracing");
    const char *vcd_name = parms.get_c_str("vcd_name");
    m_vcd_name = (vcd_name ? vcd_name : "");

    if (internal) {
      // If inside xtsc_core_vp then we have to cap.  If outside (visible in pct) then pct will cap. 
      m_num_nb_can_pop_true_cap   = new sc_signal<u64>                     ("m_num_nb_can_pop_true_cap");
      m_num_nb_can_pop_false_cap  = new sc_signal<u64>                     ("m_num_nb_can_pop_false_cap");
      m_num_nb_pop_true_cap       = new sc_signal<u64>                     ("m_num_nb_pop_true_cap");
      m_num_nb_pop_false_cap      = new sc_signal<u64>                     ("m_num_nb_pop_false_cap");
      m_num_nonempty_events_cap   = new sc_signal<u64>                     ("m_num_nonempty_events_cap");
      m_ticket_cap                = new sc_signal<u64>                     ("m_ticket_cap");
      m_element_cap               = new sc_signal<sc_biguint<DATA_WIDTH> > ("m_element_cap");

      m_num_nb_can_pop_true     (*m_num_nb_can_pop_true_cap);
      m_num_nb_can_pop_false    (*m_num_nb_can_pop_false_cap);
      m_num_nb_pop_true         (*m_num_nb_pop_true_cap);
      m_num_nb_pop_false        (*m_num_nb_pop_false_cap);
      m_num_nonempty_events     (*m_num_nonempty_events_cap);
      m_ticket                  (*m_ticket_cap);
      m_element                 (*m_element_cap);
    }

    if (m_connected) {
      // Add dummy SCML command to make it visible in VPA
      SCML_COMMAND_PROCESSOR(handle_scml_commands);
      SCML_ADD_COMMAND("dummy",   0, 0, "dummy",            "Dummy command that doesn't do anything.");

      if (m_enable_tracing) {
        m_p_trace_file = xtsc_vp_get_trace_file(m_vcd_name);

        sc_trace(m_p_trace_file, m_num_nb_can_pop_true,         m_num_nb_can_pop_true   .name());
        sc_trace(m_p_trace_file, m_num_nb_can_pop_false,        m_num_nb_can_pop_false  .name());
        sc_trace(m_p_trace_file, m_num_nb_pop_true,             m_num_nb_pop_true       .name());
        sc_trace(m_p_trace_file, m_num_nb_pop_false,            m_num_nb_pop_false      .name());
        sc_trace(m_p_trace_file, m_num_nonempty_events,         m_num_nonempty_events   .name());
        sc_trace(m_p_trace_file, m_ticket,                      m_ticket                .name());
        sc_trace(m_p_trace_file, m_element,                     m_element               .name());
      }

      log4xtensa::LogLevel ll = xtsc::xtsc_get_constructor_log_level();
      XTSC_LOG(m_text, ll,        "Constructed xtsc_queue_pop_if_vp_mon '" << name() << "'" << ":");
      XTSC_LOG(m_text, ll,        " internal                = "   << std::boolalpha << internal);
      XTSC_LOG(m_text, ll,        " connected               = "   << std::boolalpha << m_connected);
      XTSC_LOG(m_text, ll,        " enable_tracing          = "   << std::boolalpha << m_enable_tracing);
      XTSC_LOG(m_text, ll,        " vcd_name                = "   << m_vcd_name);

      SC_THREAD(nonempty_event_thread);
    }

    m_queue_pop_export(m_queue_pop_impl);

  }



  virtual ~xtsc_queue_pop_if_vp_mon() {
    xtsc_vp_finalize();
  }



  void end_of_elaboration() {
    m_num_nb_can_pop_true       .write(0);
    m_num_nb_can_pop_false      .write(0);
    m_num_nb_pop_true           .write(0);
    m_num_nb_pop_false          .write(0);
    m_num_nonempty_events       .write(0);
    m_ticket                    .write(0);
    m_element                   .write(0);
  }



  string handle_scml_commands(const vector<string>& cmd) {
    ostringstream oss;
    if (cmd[0] == "dummy") {
      oss << "dummy command executed";
    }
    return oss.str();
  }



protected:


  /// Implementation of xtsc_queue_pop_if.
  class xtsc_queue_pop_if_impl : public xtsc_queue_pop_if, public sc_object {
  public:

    /// Constructor
    xtsc_queue_pop_if_impl(const char *object_name, xtsc_queue_pop_if_vp_mon& monitor) :
      sc_object (object_name),
      m_monitor (monitor),
      m_p_port  (NULL)
    {}

    /// @see xtsc::xtsc_queue_pop_if
    bool nb_can_pop() {
      bool result = m_monitor.m_queue_pop_port->nb_can_pop();
      if (result) {
        m_monitor.m_num_nb_can_pop_true.write(++m_monitor.m_num_nb_can_pop_true_cnt);
      }
      else {
        m_monitor.m_num_nb_can_pop_false.write(++m_monitor.m_num_nb_can_pop_false_cnt);
      }
      return result;
    }

    /// @see xtsc::xtsc_queue_pop_if
    bool nb_pop(sc_dt::sc_unsigned& element, u64& ticket = pop_ticket) {
      bool result = m_monitor.m_queue_pop_port->nb_pop(element, ticket);
      if (result) {
        m_monitor.m_num_nb_pop_true.write(++m_monitor.m_num_nb_pop_true_cnt);
      }
      else {
        m_monitor.m_num_nb_pop_false.write(++m_monitor.m_num_nb_pop_false_cnt);
      }
      m_monitor.m_ticket.write(ticket);
      m_monitor.m_element.write(element);
      return result;
    }

    /// @see xtsc::xtsc_queue_pop_if
    u32 nb_get_bit_width() { return DATA_WIDTH; }

    /// Get the event that will be notified when the queue transitions from empty to not empty.
    virtual const sc_event& default_event() const {
      return (m_monitor.m_queue_pop_port.get_interface()) ?  m_monitor.m_queue_pop_port->default_event() : m_monitor.m_never_notified;
    }


  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename) {
      XTSC_INFO(m_monitor.m_text, "In register_port");
      if (m_p_port) {
        ostringstream oss;
        oss << "Illegal multiple binding detected to xtsc_queue_pop_if_vp_mon '" << m_monitor.name() << "' m_queue_pop_export" << endl;
        oss << "  " << port.name() << endl;
        throw xtsc_exception(oss.str());
      }
      m_p_port = &port;
    }

    xtsc_queue_pop_if_vp_mon&   m_monitor;      ///< Our xtsc_queue_pop_if_vp_mon object
    sc_core::sc_port_base      *m_p_port;       ///< Port that is bound to us
  };


private:


  void nonempty_event_thread() {
    if (!m_queue_pop_port.get_interface()) {
      XTSC_INFO(m_text, "Exiting nonempty_event_thread because monitor is not bound");
      return;
    }
    const sc_event& nonempty_event = m_queue_pop_port->default_event();
    while (true) {
      wait(nonempty_event);
      m_num_nonempty_events.write(++m_num_nonempty_events_cnt);
    }
  }



  xtsc_queue_pop_if_impl                m_queue_pop_impl;               ///< m_queue_pop_export binds to these

  scml_property<bool>                   m_enable_tracing;               ///< Trace to VCD file
  scml_property<string>                 m_vcd_name;                     ///< VCD file name
  sc_trace_file                        *m_p_trace_file;                 ///< The VCD file

  u64                                   m_num_nb_can_pop_true_cnt;      ///< To count m_num_nb_can_pop_true
  u64                                   m_num_nb_can_pop_false_cnt;     ///< To count m_num_nb_can_pop_false
  u64                                   m_num_nb_pop_true_cnt;          ///< To count m_num_nb_pop_true
  u64                                   m_num_nb_pop_false_cnt;         ///< To count m_num_nb_pop_false
  u64                                   m_num_nonempty_events_cnt;      ///< To count m_num_nonempty_events

  sc_signal<u64>                       *m_num_nb_can_pop_true_cap;      ///< To cap m_num_nb_can_pop_true
  sc_signal<u64>                       *m_num_nb_can_pop_false_cap;     ///< To cap m_num_nb_can_pop_false
  sc_signal<u64>                       *m_num_nb_pop_true_cap;          ///< To cap m_num_nb_pop_true
  sc_signal<u64>                       *m_num_nb_pop_false_cap;         ///< To cap m_num_nb_pop_false
  sc_signal<u64>                       *m_num_nonempty_events_cap;      ///< To cap m_num_nonempty_events
  sc_signal<u64>                       *m_ticket_cap;                   ///< To cap m_ticket
  sc_signal<sc_biguint<DATA_WIDTH> >   *m_element_cap;                  ///< To cap m_element

  bool                                  m_connected;                    ///< false if internal to xtsc_core_vp and not monitored

  sc_event                              m_never_notified;               ///< Used if we're not bound
  log4xtensa::TextLogger&               m_text;                         ///< Text logger
};


};  // namespace xtsc_vp 


#endif  // _XTSC_QUEUE_POP_IF_VP_MON_H_

