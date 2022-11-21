#ifndef _XTSC_MEMORY_IF_VP_MON_H_
#define _XTSC_MEMORY_IF_VP_MON_H_

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
#include <xtsc/xtsc_request_if.h>
#include <xtsc/xtsc_request.h>
#include <xtsc/xtsc_respond_if.h>
#include <xtsc/xtsc_response.h>



namespace xtsc_vp {


typedef void (*request_callback)(void *arg, const xtsc::xtsc_request& req);
typedef void (*respond_callback)(void *arg, const xtsc::xtsc_response& req);



/**
 * Parameters for an xtsc_memory_if_vp_mon.
 *
 * Mainly this is to allow overriding at run time using the extract_parms mechanism.
 */
class XTSC_VP_API xtsc_memory_if_vp_mon_parms : public xtsc::xtsc_parms {
public:

  xtsc_memory_if_vp_mon_parms(bool big_endian, bool enable_tracing, const char *vcd_name) {
    add("big_endian",           big_endian);
    add("enable_tracing",       enable_tracing);
    add("vcd_name",             vcd_name);
  }

  virtual const char* kind() const { return "xtsc_memory_if_vp_mon_parms"; }

};



/**
 * This module allows hardware watchpoints and tracing on an XTSC memory interface.
 *
 * This module allows VPA hardware watchpoints to be place on an XTSC memory interface
 * as well as waveform tracing of TLM payload values passing over the interface.  The
 * actual XTSC interfaces involved are xtsc::xtsc_request_if and xtsc::xtsc_respond_if.
 *
 * This monitor must be instantiated once for each port of a multi-ported module that
 * you desire to monitor (e.g. a dual-ported xtsc_memory_vp).
 *
 * @Note If bus width exceeds 256 bits (64 bytes), then bus data will not be visible in
 *       VPA (as of V2010.1.1).
 */
template <unsigned int DATA_WIDTH = 32> 
class xtsc_memory_if_vp_mon : public sc_module {
public:

  typedef std::ostringstream                    ostringstream;
  typedef xtsc::xtsc_byte_enables               xtsc_byte_enables;
  typedef xtsc::xtsc_address                    xtsc_address;
  typedef xtsc::xtsc_request_if                 xtsc_request_if;
  typedef xtsc::xtsc_respond_if                 xtsc_respond_if;
  typedef xtsc::xtsc_request                    xtsc_request;
  typedef xtsc::xtsc_response                   xtsc_response;
  typedef xtsc::xtsc_fast_access_request        xtsc_fast_access_request;
  typedef xtsc::xtsc_exception                  xtsc_exception;
  typedef xtsc::u64                             u64;
  typedef xtsc::u32                             u32;
  typedef xtsc::u16                             u16;
  typedef xtsc::u8                              u8;

  sc_export<xtsc_request_if>            m_request_export;               ///< Receive request from master
  sc_port<xtsc_request_if, NSPP>        m_request_port;                 ///< Forward request to   slave
      
  sc_export<xtsc_respond_if>            m_respond_export;               ///< Receive response from slave
  sc_port<xtsc_respond_if, NSPP>        m_respond_port;                 ///< Forward response to   master

  sc_out<u64>                           m_req_count;                    ///< Total requests
  sc_out<u64>                           m_req_tag;                      ///< Unique tag per request-response set (artificial)
  sc_out<xtsc_address>                  m_req_address8;                 ///< Byte address 
  sc_out<sc_biguint<DATA_WIDTH> >       m_req_data;                     ///< Write data 
  sc_out<u32>                           m_req_size8;                    ///< Byte size of each transfer
  sc_out<u32>                           m_req_route_id;                 ///< Route ID for arbiters
  sc_out<u32>                           m_req_type;                     ///< Request type (READ, BLOCK_READ, etc)
  sc_out<u32>                           m_req_num_transfers;            ///< Number of transfers
  sc_out<xtsc_byte_enables>             m_req_byte_enables;             ///< Byte enables
  sc_out<u8>                            m_req_id;                       ///< Transaction ID
  sc_out<u8>                            m_req_priority;                 ///< Transaction priority
  sc_out<bool>                          m_req_last_transfer;            ///< True if last transfer of request
  sc_out<xtsc_address>                  m_req_pc;                       ///< Program counter associated with request (artificial)
      
  sc_out<u64>                           m_rsp_count;                    ///< Count the nb_respond calls 
  sc_out<u64>                           m_rsp_ok_count;                 ///< Count the nb_respond calls that are RSP_OK
  sc_out<u64>                           m_rsp_nacc_count;               ///< Count the nb_respond calls that are RSP_NACC
  sc_out<u64>                           m_rsp_tag;                      ///< Unique tag per request-response set
  sc_out<u32>                           m_rsp_address8;                 ///< Starting byte address
  sc_out<sc_biguint<DATA_WIDTH> >       m_rsp_data;                     ///< Read data 
  sc_out<u32>                           m_rsp_size8;                    ///< Byte size of each transfer
  sc_out<u32>                           m_rsp_route_id;                 ///< Route ID for arbiters
  sc_out<u8>                            m_rsp_status;                   ///< Response status
  sc_out<u8>                            m_rsp_id;                       ///< Transaction ID
  sc_out<u8>                            m_rsp_priority;                 ///< Transaction priority
  sc_out<bool>                          m_rsp_last_transfer;            ///< True if last transfer of response
  sc_out<u32>                           m_rsp_pc;                       ///< Program counter associated with request


  SC_HAS_PROCESS(xtsc_memory_if_vp_mon);

#define XTSC_MEMORY_IF_VP_MON_INITIALIZERS                                              \
    sc_module                     (module_name),                                        \
    m_request_export              ("m_request_export"),                                 \
    m_request_port                ("m_request_port"),                                   \
    m_respond_export              ("m_respond_export"),                                 \
    m_respond_port                ("m_respond_port"),                                   \
    m_req_count                   ("m_req_count"),                                      \
    m_req_tag                     ("m_req_tag"),                                        \
    m_req_address8                ("m_req_address8"),                                   \
    m_req_data                    ("m_req_data"),                                       \
    m_req_size8                   ("m_req_size8"),                                      \
    m_req_route_id                ("m_req_route_id"),                                   \
    m_req_type                    ("m_req_type"),                                       \
    m_req_num_transfers           ("m_req_num_transfers"),                              \
    m_req_byte_enables            ("m_req_byte_enables"),                               \
    m_req_id                      ("m_req_id"),                                         \
    m_req_priority                ("m_req_priority"),                                   \
    m_req_last_transfer           ("m_req_last_transfer"),                              \
    m_req_pc                      ("m_req_pc"),                                         \
    m_rsp_count                   ("m_rsp_count"),                                      \
    m_rsp_ok_count                ("m_rsp_ok_count"),                                   \
    m_rsp_nacc_count              ("m_rsp_nacc_count"),                                 \
    m_rsp_tag                     ("m_rsp_tag"),                                        \
    m_rsp_address8                ("m_rsp_address8"),                                   \
    m_rsp_data                    ("m_rsp_data"),                                       \
    m_rsp_size8                   ("m_rsp_size8"),                                      \
    m_rsp_route_id                ("m_rsp_route_id"),                                   \
    m_rsp_status                  ("m_rsp_status"),                                     \
    m_rsp_id                      ("m_rsp_id"),                                         \
    m_rsp_priority                ("m_rsp_priority"),                                   \
    m_rsp_last_transfer           ("m_rsp_last_transfer"),                              \
    m_rsp_pc                      ("m_rsp_pc"),                                         \
    m_request_impl                ("m_request_impl",       *this),                      \
    m_respond_impl                ("m_respond_impl",       *this),                      \
    m_big_endian                  ("/Misc/big_endian",      false),                     \
    m_enable_tracing              ("/Trace/enable_tracing", false),                     \
    m_vcd_name                    ("/Trace/vcd_name",       ""),                        \
    m_sc_unsigned                 (DATA_WIDTH),                                         \
    m_connected                   (true),                                               \
    m_text                        (log4xtensa::TextLogger::getInstance(name()))         \



  // ctor for "external" monitor (outside of xtsc_core_vp and visible in Platform Creator)
  xtsc_memory_if_vp_mon(const sc_module_name &module_name) :
    XTSC_MEMORY_IF_VP_MON_INITIALIZERS
  {
    xtsc_vp_initialize();

    string vcd_name = m_vcd_name;
    xtsc_memory_if_vp_mon_parms parms(m_big_endian, m_enable_tracing, vcd_name.c_str());
    ctor_helper(parms, false);
  }



  // ctor for "internal" monitor (inside of xtsc_core_vp and not visible in Platform Creator)
  xtsc_memory_if_vp_mon(const sc_module_name &module_name, bool big_endian, const string& vcd_name, bool connected = true) :
    XTSC_MEMORY_IF_VP_MON_INITIALIZERS
  {
    xtsc_vp_initialize();

    m_big_endian = big_endian;
    xtsc_memory_if_vp_mon_parms parms(big_endian, (vcd_name != ""), vcd_name.c_str());
    m_connected = connected;
    ctor_helper(parms, true);
  }



  void ctor_helper(xtsc_memory_if_vp_mon_parms& parms, bool internal) {
    m_p_trace_file         = 0;
    m_request_callback     = 0;
    m_request_callback_arg = 0;
    m_respond_callback     = 0;
    m_respond_callback_arg = 0;
    m_num_requests         = 0;
    m_num_responses        = 0;
    m_num_rsp_ok           = 0;
    m_num_rsp_nacc         = 0;

    if (m_connected) {
      parms.extract_parms(sc_argc(), sc_argv(), name());
    }
    m_enable_tracing = parms.get_bool("enable_tracing");
    const char *vcd_name = parms.get_c_str("vcd_name");
    m_vcd_name = (vcd_name ? vcd_name : "");

    if (internal) {
      m_req_count_cap           = new sc_signal<u64>                       ("m_req_count_cap");
      m_req_tag_cap             = new sc_signal<u64>                       ("m_req_tag_cap");
      m_req_address8_cap        = new sc_signal<xtsc_address>              ("m_req_address8_cap");
      m_req_data_cap            = new sc_signal<sc_biguint<DATA_WIDTH> >   ("m_req_data_cap");
      m_req_size8_cap           = new sc_signal<u32>                       ("m_req_size8_cap");
      m_req_route_id_cap        = new sc_signal<u32>                       ("m_req_route_id_cap");
      m_req_type_cap            = new sc_signal<u32>                       ("m_req_type_cap");
      m_req_num_transfers_cap   = new sc_signal<u32>                       ("m_req_num_transfers_cap");
      m_req_byte_enables_cap    = new sc_signal<xtsc_byte_enables>         ("m_req_byte_enables_cap");
      m_req_id_cap              = new sc_signal<u8>                        ("m_req_id_cap");
      m_req_priority_cap        = new sc_signal<u8>                        ("m_req_priority_cap");
      m_req_last_transfer_cap   = new sc_signal<bool>                      ("m_req_last_transfer_cap");
      m_req_pc_cap              = new sc_signal<xtsc_address>              ("m_req_pc_cap");
      m_rsp_count_cap           = new sc_signal<u64>                       ("m_rsp_count_cap");
      m_rsp_ok_count_cap        = new sc_signal<u64>                       ("m_rsp_ok_count_cap");
      m_rsp_nacc_count_cap      = new sc_signal<u64>                       ("m_rsp_nacc_count_cap");
      m_rsp_tag_cap             = new sc_signal<u64>                       ("m_rsp_tag_cap");
      m_rsp_address8_cap        = new sc_signal<u32>                       ("m_rsp_address8_cap");
      m_rsp_data_cap            = new sc_signal<sc_biguint<DATA_WIDTH> >   ("m_rsp_data_cap");
      m_rsp_size8_cap           = new sc_signal<u32>                       ("m_rsp_size8_cap");
      m_rsp_route_id_cap        = new sc_signal<u32>                       ("m_rsp_route_id_cap");
      m_rsp_status_cap          = new sc_signal<u8>                        ("m_rsp_status_cap");
      m_rsp_id_cap              = new sc_signal<u8>                        ("m_rsp_id_cap");
      m_rsp_priority_cap        = new sc_signal<u8>                        ("m_rsp_priority_cap");
      m_rsp_last_transfer_cap   = new sc_signal<bool>                      ("m_rsp_last_transfer_cap");
      m_rsp_pc_cap              = new sc_signal<u32>                       ("m_rsp_pc_cap");

      m_req_count               (*m_req_count_cap);
      m_req_tag                 (*m_req_tag_cap);
      m_req_address8            (*m_req_address8_cap);
      m_req_data                (*m_req_data_cap);
      m_req_size8               (*m_req_size8_cap);
      m_req_route_id            (*m_req_route_id_cap);
      m_req_type                (*m_req_type_cap);
      m_req_num_transfers       (*m_req_num_transfers_cap);
      m_req_byte_enables        (*m_req_byte_enables_cap);
      m_req_id                  (*m_req_id_cap);
      m_req_priority            (*m_req_priority_cap);
      m_req_last_transfer       (*m_req_last_transfer_cap);
      m_req_pc                  (*m_req_pc_cap);
      m_rsp_count               (*m_rsp_count_cap);
      m_rsp_ok_count            (*m_rsp_ok_count_cap);
      m_rsp_nacc_count          (*m_rsp_nacc_count_cap);
      m_rsp_tag                 (*m_rsp_tag_cap);
      m_rsp_address8            (*m_rsp_address8_cap);
      m_rsp_data                (*m_rsp_data_cap);
      m_rsp_size8               (*m_rsp_size8_cap);
      m_rsp_route_id            (*m_rsp_route_id_cap);
      m_rsp_status              (*m_rsp_status_cap);
      m_rsp_id                  (*m_rsp_id_cap);
      m_rsp_priority            (*m_rsp_priority_cap);
      m_rsp_last_transfer       (*m_rsp_last_transfer_cap);
      m_rsp_pc                  (*m_rsp_pc_cap);
    }
    else {
      m_big_endian = parms.get_bool("big_endian");
    }

    if (m_connected) {
      // Add dummy SCML command to make it visible in VPA
      SCML_COMMAND_PROCESSOR(handle_scml_commands);
      SCML_ADD_COMMAND("dummy",   0, 0, "dummy",            "Dummy command that doesn't do anything.");

      if (m_enable_tracing) {
        m_p_trace_file = xtsc_vp_get_trace_file(m_vcd_name);

        sc_trace(m_p_trace_file, m_req_count,               m_req_count             .name());
        sc_trace(m_p_trace_file, m_req_tag,                 m_req_tag               .name());
        sc_trace(m_p_trace_file, m_req_address8,            m_req_address8          .name());
        sc_trace(m_p_trace_file, m_req_data,                m_req_data              .name());
        sc_trace(m_p_trace_file, m_req_size8,               m_req_size8             .name());
        sc_trace(m_p_trace_file, m_req_route_id,            m_req_route_id          .name());
        sc_trace(m_p_trace_file, m_req_type,                m_req_type              .name());
        sc_trace(m_p_trace_file, m_req_num_transfers,       m_req_num_transfers     .name());
        sc_trace(m_p_trace_file, m_req_byte_enables,        m_req_byte_enables      .name());
        sc_trace(m_p_trace_file, m_req_id,                  m_req_id                .name());
        sc_trace(m_p_trace_file, m_req_priority,            m_req_priority          .name());
        sc_trace(m_p_trace_file, m_req_last_transfer,       m_req_last_transfer     .name());
        sc_trace(m_p_trace_file, m_req_pc,                  m_req_pc                .name());

        sc_trace(m_p_trace_file, m_rsp_count,               m_rsp_count             .name());
        sc_trace(m_p_trace_file, m_rsp_ok_count,            m_rsp_ok_count          .name());
        sc_trace(m_p_trace_file, m_rsp_nacc_count,          m_rsp_nacc_count        .name());
        sc_trace(m_p_trace_file, m_rsp_tag,                 m_rsp_tag               .name());
        sc_trace(m_p_trace_file, m_rsp_address8,            m_rsp_address8          .name());
        sc_trace(m_p_trace_file, m_rsp_data,                m_rsp_data              .name());
        sc_trace(m_p_trace_file, m_rsp_size8,               m_rsp_size8             .name());
        sc_trace(m_p_trace_file, m_rsp_route_id,            m_rsp_route_id          .name());
        sc_trace(m_p_trace_file, m_rsp_status,              m_rsp_status            .name());
        sc_trace(m_p_trace_file, m_rsp_id,                  m_rsp_id                .name());
        sc_trace(m_p_trace_file, m_rsp_priority,            m_rsp_priority          .name());
        sc_trace(m_p_trace_file, m_rsp_last_transfer,       m_rsp_last_transfer     .name());
        sc_trace(m_p_trace_file, m_rsp_pc,                  m_rsp_pc                .name());
      }

      log4xtensa::LogLevel ll = xtsc::xtsc_get_constructor_log_level();
      XTSC_LOG(m_text, ll,        "Constructed xtsc_memory_if_vp_mon '" << name() << "'" << ":");
      XTSC_LOG(m_text, ll,        " internal                = "   << std::boolalpha << internal);
      XTSC_LOG(m_text, ll,        " connected               = "   << std::boolalpha << m_connected);
      XTSC_LOG(m_text, ll,        " big_endian              = "   << std::boolalpha << m_big_endian);
      XTSC_LOG(m_text, ll,        " enable_tracing          = "   << std::boolalpha << m_enable_tracing);
      XTSC_LOG(m_text, ll,        " vcd_name                = "   << m_vcd_name);
    }

    m_request_export(m_request_impl);
    m_respond_export(m_respond_impl);

  }




  virtual ~xtsc_memory_if_vp_mon() {
    xtsc_vp_finalize();
  }




  void end_of_elaboration() {
    m_req_count         .write(m_num_requests);
    m_req_tag           .write(0);
    m_req_address8      .write(0);
    m_req_size8         .write(0);
    m_req_route_id      .write(0);
    m_req_type          .write(0);
    m_req_num_transfers .write(0);
    m_req_byte_enables  .write(0);
    m_req_id            .write(0);
    m_req_priority      .write(0);
    m_req_last_transfer .write(0);
    m_req_pc            .write(0);

    m_rsp_count         .write(m_num_responses);
    m_rsp_ok_count      .write(m_num_rsp_ok);
    m_rsp_nacc_count    .write(m_num_rsp_nacc);
    m_rsp_tag           .write(0);
    m_rsp_address8      .write(0);
    m_rsp_size8         .write(0);
    m_rsp_route_id      .write(0);
    m_rsp_status        .write(0);
    m_rsp_id            .write(0);
    m_rsp_priority      .write(0);
    m_rsp_last_transfer .write(0);
    m_rsp_pc            .write(0);
  }




  string handle_scml_commands(const vector<string>& cmd) {
    ostringstream oss;
    if (cmd[0] == "dummy") {
      oss << "dummy command executed";
    }
    return oss.str();
  }




  void register_request_callback(request_callback req_cb, void *arg) {
    if (m_connected) {
      XTSC_INFO(m_text, "register_request_callback(" << (void*) req_cb << ", " << arg << ")");
      m_request_callback_arg = arg;
      m_request_callback     = req_cb;
    }
  }



  void register_respond_callback(respond_callback rsp_cb, void *arg) {
    if (m_connected) {
      XTSC_INFO(m_text, "register_respond_callback(" << (void*) rsp_cb << ", " << arg << ")");
      m_respond_callback_arg = arg;
      m_respond_callback     = rsp_cb;
    }
  }




protected:

  /// Implementation of xtsc_request_if.
  class xtsc_request_if_impl : virtual public xtsc_request_if, public sc_object {
  public:

    /**
     * Constructor.
     * @param   mon     A reference to the owning xtsc_memory_if_vp_mon object.
     */
    xtsc_request_if_impl(const char *object_name, xtsc_memory_if_vp_mon& mon) :
      sc_object         (object_name),
      m_monitor         (mon),
      m_p_port          (NULL)
    {}

    /// @see xtsc::xtsc_request_if
    void nb_request(const xtsc_request& request) {
      u32 size8 = request.get_byte_size();
      const u8 *buf = request.get_buffer();
      m_monitor.m_sc_unsigned = 0;
      for (u32 i=0; i<size8; ++i) {
        u32 index = i;
        if (m_monitor.m_big_endian) {
          index = size8 - 1 - i;
        }
        m_monitor.m_sc_unsigned.range(i*8+7, i*8) = buf[index];
      }

      m_monitor.m_num_requests += 1;
      m_monitor.m_req_count         .write(m_monitor.m_num_requests);
      m_monitor.m_req_tag           .write(request.get_tag());
      m_monitor.m_req_address8      .write(request.get_byte_address());
      m_monitor.m_req_data          .write(m_monitor.m_sc_unsigned);
      m_monitor.m_req_size8         .write(size8);
      m_monitor.m_req_route_id      .write(request.get_route_id());
      m_monitor.m_req_type          .write(request.get_type());
      m_monitor.m_req_num_transfers .write(request.get_num_transfers());
      m_monitor.m_req_byte_enables  .write(request.get_byte_enables());
      m_monitor.m_req_id            .write(request.get_id());
      m_monitor.m_req_priority      .write(request.get_priority());
      m_monitor.m_req_last_transfer .write(request.get_last_transfer());
      m_monitor.m_req_pc            .write(request.get_pc());


      if (m_monitor.m_request_callback) {
        m_monitor.m_request_callback(m_monitor.m_request_callback_arg, request);
      }

      m_monitor.m_request_port->nb_request(request);
    }


    /// @see xtsc::xtsc_request_if
    void nb_peek(xtsc_address address8, u32 size8, u8 *buffer) {
      m_monitor.m_request_port->nb_peek(address8, size8, buffer);
    }


    /// @see xtsc::xtsc_request_if
    void nb_poke(xtsc_address address8, u32 size8, const u8 *buffer) {
      m_monitor.m_request_port->nb_poke(address8, size8, buffer);
    }


    /// @see xtsc::xtsc_request_if
    bool nb_fast_access(xtsc_fast_access_request &request) {
      return m_monitor.m_request_port->nb_fast_access(request);
    }


  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char * /*if_typename*/) {
      if (m_p_port) {
        ostringstream oss;
        oss << "Illegal multiple binding detected to xtsc_memory_if_vp_mon '" << m_monitor.name() << "' m_request_export" << endl;
        oss << "  " << port.name() << endl;
        throw xtsc_exception(oss.str());
      }
      m_p_port = &port;
    }


    xtsc_memory_if_vp_mon&      m_monitor;      ///< Our xtsc_memory_if_vp_mon object
    sc_core::sc_port_base      *m_p_port;       ///< Port that is bound to us
  };


  /// Implementation of xtsc_respond_if.
  class xtsc_respond_if_impl : public xtsc_respond_if, public sc_object {
  public:

    /// Constructor
    xtsc_respond_if_impl(const char *object_name, xtsc_memory_if_vp_mon& mon) :
      sc_object (object_name),
      m_monitor (mon),
      m_p_port  (NULL)
    {}

    /// @see xtsc::xtsc_respond_if
    bool nb_respond(const xtsc_response& response) {
      u32 size8 = response.get_byte_size();
      const u8 *buf   = response.get_buffer();
      m_monitor.m_sc_unsigned = 0;
      for (u32 i=0; i<size8; ++i) {
        u32 index = i;
        if (m_monitor.m_big_endian) {
          index = size8 - 1 - i;
        }
        m_monitor.m_sc_unsigned.range(i*8+7, i*8) = buf[index];
      }

      m_monitor.m_num_responses += 1;
      if (response.get_status() == xtsc_response::RSP_OK)   m_monitor.m_num_rsp_ok   += 1;
      if (response.get_status() == xtsc_response::RSP_NACC) m_monitor.m_num_rsp_nacc += 1;

      m_monitor.m_rsp_count         .write(m_monitor.m_num_responses);
      m_monitor.m_rsp_ok_count      .write(m_monitor.m_num_rsp_ok);
      m_monitor.m_rsp_nacc_count    .write(m_monitor.m_num_rsp_nacc);

      m_monitor.m_rsp_tag           .write(response.get_tag());
      m_monitor.m_rsp_address8      .write(response.get_byte_address());
      m_monitor.m_rsp_data          .write(m_monitor.m_sc_unsigned);
      m_monitor.m_rsp_size8         .write(size8);
      m_monitor.m_rsp_route_id      .write(response.get_route_id());
      m_monitor.m_rsp_status        .write((u8) response.get_status());
      m_monitor.m_rsp_id            .write(response.get_id());
      m_monitor.m_rsp_priority      .write(response.get_priority());
      m_monitor.m_rsp_last_transfer .write(response.get_last_transfer());
      m_monitor.m_rsp_pc            .write(response.get_pc());

      bool result = m_monitor.m_respond_port->nb_respond(response);

      if (m_monitor.m_respond_callback) {
        m_monitor.m_respond_callback(m_monitor.m_respond_callback_arg, response);
      }

      return result;
    }


  protected:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char * /*if_typename*/) {
      if (m_p_port) {
        ostringstream oss;
        oss << "Illegal multiple binding detected to xtsc_memory_if_vp_mon '" << m_monitor.name() << "' m_respond_export" << endl;
        oss << "  " << port.name() << endl;
        throw xtsc_exception(oss.str());
      }
      m_p_port = &port;
    }


    xtsc_memory_if_vp_mon&      m_monitor;      ///< Our xtsc_memory_if_vp_mon object
    sc_core::sc_port_base      *m_p_port;       ///< Port that is bound to us
  };




private:

  xtsc_request_if_impl                  m_request_impl;                 ///< m_request_export binds to these
  xtsc_respond_if_impl                  m_respond_impl;                 ///< m_respond_export binds to these

  scml_property<bool>                   m_big_endian;                   ///< Swizzle the bytes in the request/response data bufer
  scml_property<bool>                   m_enable_tracing;               ///< Trace to VCD file
  scml_property<string>                 m_vcd_name;                     ///< VCD file name
  sc_trace_file                        *m_p_trace_file;                 ///< The VCD file

  sc_signal<u64>                       *m_req_count_cap;                ///< To cap m_req_count
  sc_signal<u64>                       *m_req_tag_cap;                  ///< To cap m_req_tag
  sc_signal<xtsc_address>              *m_req_address8_cap;             ///< To cap m_req_address8
  sc_signal<sc_biguint<DATA_WIDTH> >   *m_req_data_cap;                 ///< To cap m_req_data
  sc_signal<u32>                       *m_req_size8_cap;                ///< To cap m_req_size8
  sc_signal<u32>                       *m_req_route_id_cap;             ///< To cap m_req_route_id
  sc_signal<u32>                       *m_req_type_cap;                 ///< To cap m_req_type
  sc_signal<u32>                       *m_req_num_transfers_cap;        ///< To cap m_req_num_transfers
  sc_signal<xtsc_byte_enables>         *m_req_byte_enables_cap;         ///< To cap m_req_byte_enables
  sc_signal<u8>                        *m_req_id_cap;                   ///< To cap m_req_id
  sc_signal<u8>                        *m_req_priority_cap;             ///< To cap m_req_priority
  sc_signal<bool>                      *m_req_last_transfer_cap;        ///< To cap m_req_last_transfer
  sc_signal<xtsc_address>              *m_req_pc_cap;                   ///< To cap m_req_pc

  sc_signal<u64>                       *m_rsp_count_cap;                ///< To cap m_rsp_count
  sc_signal<u64>                       *m_rsp_ok_count_cap;             ///< To cap m_rsp_ok_count
  sc_signal<u64>                       *m_rsp_nacc_count_cap;           ///< To cap m_rsp_nacc_count
  sc_signal<u64>                       *m_rsp_tag_cap;                  ///< To cap m_rsp_tag
  sc_signal<u32>                       *m_rsp_address8_cap;             ///< To cap m_rsp_address8
  sc_signal<sc_biguint<DATA_WIDTH> >   *m_rsp_data_cap;                 ///< To cap m_rsp_data
  sc_signal<u32>                       *m_rsp_size8_cap;                ///< To cap m_rsp_size8
  sc_signal<u32>                       *m_rsp_route_id_cap;             ///< To cap m_rsp_route_id
  sc_signal<u8>                        *m_rsp_status_cap;               ///< To cap m_rsp_status
  sc_signal<u8>                        *m_rsp_id_cap;                   ///< To cap m_rsp_id
  sc_signal<u8>                        *m_rsp_priority_cap;             ///< To cap m_rsp_priority
  sc_signal<bool>                      *m_rsp_last_transfer_cap;        ///< To cap m_rsp_last_transfer
  sc_signal<u32>                       *m_rsp_pc_cap;                   ///< To cap m_rsp_pc

  sc_unsigned                           m_sc_unsigned;                  ///< Temporarily hold m_req_data/m_rsp_data
  request_callback                      m_request_callback;             ///< Optional function called before forwarding nb_request
  void                                 *m_request_callback_arg;         ///< Argument to be passed to request callback
  respond_callback                      m_respond_callback;             ///< Optional function called after  forwarding nb_respond
  void                                 *m_respond_callback_arg;         ///< Argument to be passed to respond callback

  u64                                   m_num_requests;                 ///< Total requests
  u64                                   m_num_responses;                ///< Total responses
  u64                                   m_num_rsp_ok;                   ///< Total RSP_OK responses
  u64                                   m_num_rsp_nacc;                 ///< Total RSP_NACC responses

  bool                                  m_connected;                    ///< false if internal to xtsc_core_vp and not monitored

  log4xtensa::TextLogger&               m_text;                         ///< Text logger
};


};  // namespace xtsc_vp 


#endif  // _XTSC_MEMORY_IF_VP_MON_H_

