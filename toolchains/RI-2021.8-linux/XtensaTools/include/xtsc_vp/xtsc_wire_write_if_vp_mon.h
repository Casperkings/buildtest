#ifndef _XTSC_WIRE_WRITE_IF_VP_MON_H_
#define _XTSC_WIRE_WRITE_IF_VP_MON_H_

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
#include <xtsc/xtsc_wire_write_if.h>



namespace xtsc_vp {



/**
 * Parameters for an xtsc_wire_write_if_vp_mon.
 *
 * Mainly this is to allow overriding at run time using the extract_parms mechanism.
 */
class XTSC_VP_API xtsc_wire_write_if_vp_mon_parms : public xtsc::xtsc_parms {
public:

  xtsc_wire_write_if_vp_mon_parms(bool enable_tracing, const char *vcd_name) {
    add("enable_tracing",       enable_tracing);
    add("vcd_name",             vcd_name);
  }

  virtual const char* kind() const { return "xtsc_wire_write_if_vp_mon_parms"; }

};



/**
 * This module allows hardware watchpoints and tracing on an XTSC wire write interface.
 *
 * This module allows VPA hardware watchpoints to be place on an XTSC wire write interface
 * as well as waveform tracing of the interface.  The actual XTSC interface involved is
 * xtsc::xtsc_wire_write_if.
 *
 * @Note If data width exceeds 256 bits, then the data will not be visible in VPA (as of
 * V2010.1.1).
 */
template <unsigned int DATA_WIDTH = 1> 
class xtsc_wire_write_if_vp_mon : public sc_module {
public:

  typedef std::ostringstream            ostringstream;
  typedef xtsc::xtsc_wire_write_if      xtsc_wire_write_if;
  typedef xtsc::xtsc_exception          xtsc_exception;
  typedef xtsc::u64                     u64;
  typedef xtsc::u32                     u32;

  sc_export<xtsc_wire_write_if>         m_wire_write_export;            ///< Receive the write from upstream source
  sc_port<xtsc_wire_write_if, NSPP>     m_wire_write_port;              ///< Forward the write to downstream sink
      
  sc_out<u64>                           m_num_nb_write;                 ///< Total calls to nb_write

  sc_out<sc_biguint<DATA_WIDTH> >       m_value;                        ///< Last value written


  SC_HAS_PROCESS(xtsc_wire_write_if_vp_mon);



#define XTSC_WIRE_WRITE_IF_VP_MON_INITIALIZERS                                          \
    sc_module                     (module_name),                                        \
    m_wire_write_export           ("m_wire_write_export"),                              \
    m_wire_write_port             ("m_wire_write_port"),                                \
    m_num_nb_write                ("m_num_nb_write"),                                   \
    m_value                       ("m_value"),                                          \
    m_wire_write_impl             ("m_wire_write_impl", *this),                         \
    m_enable_tracing              ("/Trace/enable_tracing", false),                     \
    m_vcd_name                    ("/Trace/vcd_name",       ""),                        \
    m_p_trace_file                (0),                                                  \
    m_num_nb_write_cnt            (0),                                                  \
    m_connected                   (true),                                               \
    m_bound                       (false),                                              \
    m_text                        (log4xtensa::TextLogger::getInstance(name()))         \



  // ctor for "external" monitor (outside of xtsc_core_vp and visible in Platform Creator)
  xtsc_wire_write_if_vp_mon(const sc_module_name &module_name) :
    XTSC_WIRE_WRITE_IF_VP_MON_INITIALIZERS
  {
    xtsc_vp_initialize();

    string vcd_name = m_vcd_name;
    xtsc_wire_write_if_vp_mon_parms parms(m_enable_tracing, vcd_name.c_str());
    ctor_helper(parms, false);
  }



  // ctor for "internal" monitor (inside of xtsc_core_vp and not visible in Platform Creator)
  xtsc_wire_write_if_vp_mon(const sc_module_name &module_name, const string& vcd_name, bool connected = true) :
    XTSC_WIRE_WRITE_IF_VP_MON_INITIALIZERS
  {
    xtsc_vp_initialize();

    xtsc_wire_write_if_vp_mon_parms parms((vcd_name != ""), vcd_name.c_str());
    m_connected = connected;
    ctor_helper(parms, true);
  }



  void ctor_helper(xtsc_wire_write_if_vp_mon_parms& parms, bool internal) {
    if (m_connected) {
      parms.extract_parms(sc_argc(), sc_argv(), name());
    }
    m_enable_tracing = parms.get_bool("enable_tracing");
    const char *vcd_name = parms.get_c_str("vcd_name");
    m_vcd_name = (vcd_name ? vcd_name : "");

    if (internal) {
      // If inside xtsc_core_vp then we have to cap.  If outside (visible in pct) then pct will cap. 
      m_num_nb_write_cap        = new sc_signal<u64>                     ("m_num_nb_write_cap");
      m_value_cap               = new sc_signal<sc_biguint<DATA_WIDTH> > ("m_value_cap");

      m_num_nb_write            (*m_num_nb_write_cap);
      m_value                   (*m_value_cap);
    }

    if (m_connected) {
      // Add dummy SCML command to make it visible in VPA
      SCML_COMMAND_PROCESSOR(handle_scml_commands);
      SCML_ADD_COMMAND("dummy",   0, 0, "dummy",            "Dummy command that doesn't do anything.");

      if (m_enable_tracing) {
        m_p_trace_file = xtsc_vp_get_trace_file(m_vcd_name);

        sc_trace(m_p_trace_file, m_num_nb_write,        m_num_nb_write  .name());
        sc_trace(m_p_trace_file, m_value,               m_value         .name());
      }

      log4xtensa::LogLevel ll = xtsc::xtsc_get_constructor_log_level();
      XTSC_LOG(m_text, ll,        "Constructed xtsc_wire_write_if_vp_mon '" << name() << "'" << ":");
      XTSC_LOG(m_text, ll,        " internal                = "   << std::boolalpha << internal);
      XTSC_LOG(m_text, ll,        " connected               = "   << std::boolalpha << m_connected);
      XTSC_LOG(m_text, ll,        " enable_tracing          = "   << std::boolalpha << m_enable_tracing);
      XTSC_LOG(m_text, ll,        " vcd_name                = "   << m_vcd_name);

    }

    m_wire_write_export(m_wire_write_impl);
  }



  virtual ~xtsc_wire_write_if_vp_mon() {
    xtsc_vp_finalize();
  }



  void end_of_elaboration() {
    m_bound = (m_wire_write_port.get_interface() != 0);
    m_num_nb_write      .write(0);
    m_value             .write(0);
  }



  string handle_scml_commands(const vector<string>& cmd) {
    ostringstream oss;
    if (cmd[0] == "dummy") {
      oss << "dummy command executed";
    }
    return oss.str();
  }



protected:


  /// Implementation of xtsc_wire_write_if.
  class xtsc_wire_write_if_impl : public xtsc_wire_write_if, public sc_object {
  public:

    /// Constructor
    xtsc_wire_write_if_impl(const char *object_name, xtsc_wire_write_if_vp_mon& monitor) :
      sc_object (object_name),
      m_monitor (monitor),
      m_p_port  (NULL)
    {}

    /// @see xtsc::xtsc_wire_write_if
    void nb_write(const sc_dt::sc_unsigned& value) {
      m_monitor.m_value = value;
      m_monitor.m_num_nb_write.write(++m_monitor.m_num_nb_write_cnt);
      if (m_monitor.m_bound) {
        m_monitor.m_wire_write_port->nb_write(value);
      }
    }

    /// @see xtsc::xtsc_wire_write_if
    u32 nb_get_bit_width() { return DATA_WIDTH; }


  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename) {
      XTSC_INFO(m_monitor.m_text, "In register_port");
      if (m_p_port) {
        ostringstream oss;
        oss << "Illegal multiple binding detected to xtsc_wire_write_if_vp_mon '" << m_monitor.name() << "' m_wire_write_export"
            << endl;
        oss << "  " << port.name() << endl;
        throw xtsc_exception(oss.str());
      }
      m_p_port = &port;
    }

    xtsc_wire_write_if_vp_mon&  m_monitor;      ///< Our xtsc_wire_write_if_vp_mon object
    sc_core::sc_port_base      *m_p_port;       ///< Port that is bound to us
  };


private:


  xtsc_wire_write_if_impl               m_wire_write_impl;              ///< m_wire_write_export binds to these

  scml_property<bool>                   m_enable_tracing;               ///< Trace to VCD file
  scml_property<string>                 m_vcd_name;                     ///< VCD file name
  sc_trace_file                        *m_p_trace_file;                 ///< The VCD file

  u64                                   m_num_nb_write_cnt;             ///< To count m_num_nb_write

  sc_signal<u64>                       *m_num_nb_write_cap;             ///< To cap m_num_nb_write
  sc_signal<sc_biguint<DATA_WIDTH> >   *m_value_cap;                    ///< To cap m_value

  bool                                  m_connected;                    ///< false if internal to xtsc_core_vp and not monitored
  bool                                  m_bound;                        ///< true if m_wire_write_port is bound.

  log4xtensa::TextLogger&               m_text;                         ///< Text logger
};


};  // namespace xtsc_vp 


#endif  // _XTSC_WIRE_WRITE_IF_VP_MON_H_

