#ifndef _XTSC_XNNE_H_
#define _XTSC_XNNE_H_

// Copyright (c) 2018-2019 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */


#include <xtsc/xtsc.h>
#include <xtsc/xtsc_parms.h>
#include <string>
#include <vector>


namespace xtsc {


class xtsc_xnne_parts;





/**
 * Constructor parameters for a xtsc_xnne object.
 *
 * This class contains the parameters needed to construct an xtsc_xnne object.
 * 
 *  \verbatim
   Name                         Type    Description
   -------------------------    ----    -----------------------------------------------

   "XNNECoreID"                 u32     The XNNE Core ID.  This defines the intial
                                        value of the XNNECoreID register.  After 
                                        elaboration, the XNNECoreID register can be
                                        changed using the "XNNECoreID" system-level 
                                        input.

   "axim_byte_width"            u32     Byte width of the AXI master data buses.  Valid
                                        values are 16 (128-bit) and 32 (256-bit).
                                        Default = 16.

   "axim_read_delay"            u32     For cycle estimation when peek is being used
                                        instead of real AXI RD_INCR transactions. How
                                        many cycles it would take the system to respond
                                        to a read with the first beat of data.
                                        Default = 100.

   "axim_read_repeat"           u32     For cycle estimation when peek is being used
                                        instead of real AXI RD_INCR transactions. How
                                        many cycles it would take the system to deliver
                                        each remaining (non-first) beat of read data.
                                        Default = 1.

   "axim_write_delay"           u32     For cycle estimation when poke is being used
                                        instead of real AXI WR_INCR transactions. How
                                        many cycles it would take the system to respond
                                        to a single beat write.
                                        Default = 100.

   "axim_write_repeat"          u32     For cycle estimation when poke is being used
                                        instead of real AXI WR_INCR transactions. How
                                        many more cycles it would take the system to
                                        respond for each additional (non-first) beat of
                                        write data.
                                        Default = 1.

   "clock_period"               u32    This is the length of XNNE's clock period
                                       expressed in terms of the SystemC time resolution
                                       (from sc_get_time_resolution()).  A value of 
                                       0xFFFFFFFF means to use the XTSC system clock 
                                       period (from xtsc_get_system_clock_period()).
                                       A value of 0 means one delta cycle.
                                       Default = 0xFFFFFFFF (i.e. use the system clock 
                                       period).

   "base_address"               u64     The base address for this XNNE.
                                        Default 0x40000000.

   "cbuf_byte_size"             char*   Number of bytes in each coefficient memory
                                        (CBUF).
                                        Default = 393216=0x60000 (384KB).

   "cbuf_fill_byte"             u32     The low byte specifies the value used to 
                                        initialize CBUF memory contents at address
                                        locations not initialize from "load_file".
                                        Default = 0.

   "continue_on_error"          bool    Under normal operations any error that would set
                                        an ErrorStatus bit, an Address Error on inbound
                                        AXI, or a Slave Error will cause simulation to
                                        terminate with an explanatory exception message.
                                        Set this parameter to true to enable simulation
                                        to continue.  This would normally only be done
                                        to test error interrupt handling code on the VP6.
                                        Default = false.

   "ibuf_byte_size"             char*   Number of bytes in each input activation memory
                                        (IBUF).
                                        Default = 262144=0x40000 (256KB).

   "ibuf_fill_byte"             u32     The low byte specifies the value used to 
                                        initialize IBUF memory contents at address
                                        locations not initialize from "load_file".
                                        Default = 0.

   "load_file"                  char*   A file in xtsc_memory_parms "initial_value_file"
                                        format to load to the xtsc_xnne accessible space.
                                        May also contain a comma-separated list of files
                                        to load.

   "log_level_fields"           u32     Specifies the log level to be used for dumping
                                        message fields.
                                        Default = 25000 (NOTE_LOG_LEVEL)

   "log_level_tensors"          u32     Specifies the log level to be used for dumping
                                        TF_Go tensors.
                                        Default = 15000 (VERBOSE_LOG_LEVEL)

   "log_level_trace"            u32     Specifies the log level to be used for dumping
                                        OFMAP traces.
                                        Default = 25000 (NOTE_LOG_LEVEL)

   "num_mblks"                  char*   Number of multiplier blocks (MBLKs) in each SBLK.
                                        For DNA100, legal values are 1|2|4.
                                        For DNA150, legal values are 1|2|4|8.
                                        Default = 1.

   "num_sblks"                  u32     Number of super blocks (SBLKs) in the xtsc_xnne.
                                        Legal values are 1|2|4.
                                        Default = 1.

   "num_decompress"             u32     Number of DMAin PTile decompressor units in each
                                        SBLK. Legal values are 1|2|4.
                                        Default = 2.

   "obuf_byte_size"             char*   Number of bytes in each output memory (OBUF).
                                        Default = 131072=0x20000 (128KB).

   "obuf_fill_byte"             u32     The low byte specifies the value used to 
                                        initialize OBUF memory contents at address
                                        locations not initialize from "load_file".
                                        Default = 0.

   "sblk_msg_fifo_depth"        char*   XNNEMQ FIFO depth in each SBLK.
                                        Default = 4.

   "trace_ofmap"                char*   Index of OFMAP to trace.  Note: The mapping
                                        between OFMAP index and OBUF address can be
                                        seen by setting "log_level_tensors" to 25000.
                                        Default = "" (do not trace any OFMAP entry)

   "vpu_config"                 char*   The Xtensa config to use inside the VPU to drive
                                        the VPU ISA datapath.
                                        Default = "XRC_7C0"

   "vpu_core_parms"             char*   xtsc_core_parms parameters to pass to the
                                        vpu_core (XRC_7C0) of each SBLK.
                                        - Parameter set separator is vertical bar, "|".
                                        - NAME=VALUE pair separator is plus sign, "+"

   "vpu_msg_fifo_depth"         char*   XNNEMQ FIFO depth in each VPU.
                                        Default = 4.

   "xnne_msg_fifo_depth"        u32     XNNEMQ FIFO depth in XNNE.
                                        Default = 2.

   "zero_fill_byte"             u32     The low byte of this parameter specifies the
                                        value used to do zero-filling when doing DMA in
                                        to IBUF.  This parameter should normally be left
                                        at the value suggested by its name; however, it
                                        can be set to some other value to aid debug or
                                        visualization of DMA zero-filling.
                                        Note: This parameter is only used in DNA100.
                                        In DNA150, DMAin always use PadValue for filling.
                                        Default = 0.

   "hw_version"                 u32     HW version of the XNNE.
                                        Legal values are 281010 (RI.1), 281020 (RI.2), 
                                        281030 (RI.3) or later.
                                        Default = 281010

   "ubuf_byte_size"             char*   Number of bytes in each unified buffer (UBUF).
                                        Legal values are: 256KB, 512KB, 768KB, 1024KB,
                                        1536KB, 2048KB.
                                        Default = 786432 = 0xC0000 (768KB).

   "ubuf_fill_byte"             u32     The low byte specifies the value used to 
                                        initialize UBUF memory contents at address
                                        locations not initialize from "load_file".
                                        Default = 0.

   "enable_buf_contention"      bool    Specifies whether or not to enable contention
                                        modeling for the internal buffers (IBUF/CBUF/OBUF
                                        for DNA100 or UBUF for DNA150).
                                        Note: it can't be used in turbo mode.
                                        Default = false.

   "sblk_compare"               bool    Specifies if the config has SBLK compare feature.
                                        It's used for DNA150 only. Ignored for DNA100.
                                        Default = false.

   "profile_axi_traffic"        bool    If true, the xtsc_xnne class keeps the track of 
                                        AXI requests and responses. At the end of the 
                                        simulation, AXI traffic info are printed in the 
                                        output log file at NOTE level. 
                                        Default = false.

    \endverbatim
 *
 *
 * @see xtsc_xnne
 * @see xtsc_parms
 */
class XTSC_API xtsc_xnne_parms : public xtsc_parms {
public:


  /**
   * Constructor for an xtsc_xnne_parms object.
   */
  xtsc_xnne_parms();


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_xnne_parms"; }

};



/**
 * An XTSC model of the Cadence-Tensilica Xtensa Neural Network Engine (XNNE).
 *
 * This class models TODO
 *
 */
class XTSC_API xtsc_xnne : public sc_core::sc_module, public xtsc_module, public xtsc_command_handler_interface {
public:

  // SystemC needs this.
  SC_HAS_PROCESS(xtsc_xnne);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_xnne"; }


  /**
   * Constructor for an xtsc_xnne.
   *
   * @param module_name   Name of the xtsc_xnne sc_module.
   *
   * @param xnne_parms    The configuration parameters.
   *
   * @see xtsc_xnne_parms
   */
  xtsc_xnne(sc_core::sc_module_name module_name, const xtsc_xnne_parms &xnne_parms);


  ~xtsc_xnne();


  /// SystemC callback
  void start_of_simulation();


  /// For xtsc_connection_interface
  virtual u32 get_bit_width(const std::string& port_name, u32 interface_num = 0) const;


  /**
   * For xtsc_connection_interface
   * TODO: doxygen for non-xtsc_connection_interface users to get each port for manually binding
   *       For example:
   *       To get a pointer to the AXI slave read request sc_export
   *         sc_export<xtsc_request_if> *p_rd_request_export =
   *            dynamic_cast<sc_export<xtsc_request_if>*>(get_port("m_rd_request_export"))
   */
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * Note: The xtsc_xnne model internally contains xnne_sblk and xnne_vp_dpu instances
   *       which support their own sets of commands.  These commands are documented at
   *       the end of the following list of commands that the xtsc_xnne model itself
   *       directly supports.
   *
   * The xtsc_xnne model supports the following commands:
   *  \verbatim
        copy <SrcAddress> <DstAddress> <NumBytes>
          Copy <NumBytes> of memory from <SrcAddress> to <DstAddress>.  Addresses can be
          internal or external.

        copy_mmo_to_tmo <TH> <TW> <TD> <MH> <MW> <MD> <MMOStartAddress> <TMOStartAddress> <ElementSize>
          Copy MMO tensor with dimensions of (<TH>,<TW>,<TD>) elements of <ElementSize>
          bytes arranged in MEMTILES with dimensions of (<MH>,<MW>,<MD>) elements from
          <MMOStartAddress> to <TMOStartAddress> and convert to TMO.

        copy_pmo_to_tmo <TH> <TW> <TD> <PMOStartAddress> <TMOStartAddress> <ElementSize>
          Copy PMO tensor with dimensions of (<TH>,<TW>,<TD>) elements of <ElementSize>
          bytes at <PMOStartAddress> to <TMOStartAddress> and convert to TMO.

        copy_tmo_to_mmo <TH> <TW> <TD> <MH> <MW> <MD> <TMOStartAddress> <MMOStartAddress> <ElementSize>
          Copy TMO tensor with dimensions of (<TH>,<TW>,<TD>) elements of <ElementSize>
          bytes at <TMOStartAddress> to <MMOStartAddress> and convert to MMO using
          MEMTILES with dimensions of (<MH>,<MW>,<MD>) elements.

        copy_tmo_to_pmo <TH> <TW> <TD> <TMOStartAddress> <PMOStartAddress> <ElementSize>
          Copy TMO tensor with dimensions of (<TH>,<TW>,<TD>) elements of <ElementSize>
          bytes at <TMOStartAddress> to <PMOStartAddress> and convert to PMO.

        dump <StartAddress> <NumBytes> [dec [<WordSize> [<Unsigned>]]]
          If dec is not specified, dump <NumBytes> of memory in hex starting at
          <StartAddress> (dump includes header and printable ASCII column).  If dec is
          specified, dump <NumBytes> bytes as <WordSize>-byte words of memory in decimal
          starting at <StartAddress> (if <Unsigned> is true, words are treated as
          unsigned values, otherwise they are treated as signed values).  

        dump_all [1|0]
          Control whether or not column headings, addresses, and ASCII values (hex dump
          only) are included in dump.

        dump_event_channel [<Pattern> [<state>]]
          Dump event channel information on all global event channels of this xtsc_xnne
          matching <Pattern> (default *) and optionally matching <state> (clr|set|0|1|
          wait).

        dump_internal_state [<Pattern>]
          Dump all internal state (or optionally only those names matching <Pattern>),
          showing address, name, and value.

        dump_map [SBLKIndex]
          Dump the address map for the specified SBLK index.  Default is all SBLK's.

        fill <Address> <NumBytes> <Value>
          Fill <NumBytes> of memory starting at <Address> with byte <Value>.  <Address>
          can be internal or external.

        get_event_channel  <EventChannelName>
          Return the state (0=clr, 1=set) of <EventChannelName>.

        get_log_level_fields
          Return the log level that is used for dumping message fields.

        get_log_level_tensors
          Return the log level that is used for dumping tensors during TF_Go.

        get_log_level_trace
          Return the log level that is used for tracing accumulators and OFMAP entries.

        load_file <FileName>
          Load <FileName> to any internal/external memory-mapped address.  <FileName>
          can be a comma-separated list of file names.

        peek <MemoryPortName> <StartAddress> <NumBytes>
          Call peek(<StartAddress>, <NumBytes>, buffer) on the port specfied by
          <MemoryPortName> and display buffer.

        poke <MemoryPortName> <StartAddress> <NumBytes> <Byte1> <Byte2> ... <ByteN>
          Call poke(<StartAddress>, <NumBytes>, buffer) on the port specfied by
          <MemoryPortName> after putting <Byte1> ... <ByteN> in buffer.

        quantize8 <acc> <bias> <shift> <mpyScale> <ApplyMpyScale> <RoundMode> <ODT> <ApplyRelu> <trace>
          Return the result of calling quantize8() with the specified arguments.

        read_internal_state <Address>
          Return the value of internal state variable at <Address>.

        reset
          Call xtsc_xnne::reset(<Hard>).  Where <Hard> is 0|1 (default 0).

        set_event_channel  <EventChannelName> 0|1
          Set the state of <EventChannelName> (0=clr, 1=set).

        set_log_level_fields <LogLevel>
          Set the log level (from "xtsc dump_log_levels") that is used for dumping
          message fields.

        set_log_level_tensors <LogLevel>
          Set the log level (from "xtsc dump_log_levels") that is used for dumping
          tensors during TF_Go.

        set_log_level_trace <LogLevel>
          Set the log level (from "xtsc dump_log_levels") that is used for tracing
          accumulators and OFMAP entries.

        show_cbuf_sequence <C> <M> <NumFilters> <STH> <STW> <RH> <RW> <PADH_T> <PADW_L>
          Show the CBUF sequence of coefficients for the specified arguments.

        status
          Show the development status of XNNE features in xtsc_xnne

        write_internal_state <Address> <Value>
          Write <Value> to the internal state variable at <Address>.

      \endverbatim
   *
   * The internal xnne_sblk model instances support the following commands using a
   * hierarchichal command handler name based on the xtsc_xnne instance name and the
   * xnne_sblk instance name (for example: "xnne0.sblk0"):
   *  \verbatim
        dump_event_channel [<Pattern> [<state>]]
          Dump event channel information on all local event channels of this xnne_sblk
          matching <Pattern> (default *) and optionally matching <state> (clr|set|0|1|
          wait).

        dump_lut [<LAYERID>|*]
          Dump filter LUT for layer <LAYERID>.  Default is to dump filter LUT for each
          TensorFetch layer.

        dump_register_layer <Block> <LayerID>
          Dump all register names and values in <LayerID> of <Block> (IDMA|TF|VPU).

        dump_trf [<ARID>|*]
          Dump Tracking Register File entry <ARID>, or all TRF entries if * is specified,
          else all active TRF entries.

        estimate_cycles <NumMultiplies> <ZeroActivations> <ZeroCoefficients> <ZeroBoth>
                        <SS> <SM> <NumMBLKS>
          Estimate the number of cycles for a TF_Go (does not include bytes_written to
          OBUF).

        get_event_channel  <EventChannelName>
          Return the state (0=clr, 1=set) of <EventChannelName>.

        get_layer_register <Block> <LayerID> <RegisterName>
          Return the value of <RegisterName> (case-sensitive) in <LayerID> of <Block>
          (IDMA|TF|VPU).

        peek <StartAddress> <NumBytes>
          Peek <NumBytes> of memory starting at <StartAddress>.

        poke <StartAddress> <NumBytes> <Byte1> <Byte2> . . . <ByteN>
          Poke <NumBytes> (=N) of memory starting at <StartAddress>.

        reset [<Hard>]
          Call xnne_sblk::reset(<Hard>).  Where <Hard> is 0|1 (default 0).

        set_event_channel  <EventChannelName> 0|1
          Set the state of <EventChannelName> (0=clr, 1=set).

        set_layer_register <Block> <LayerID> <RegisterName> <RegisterValue>
          Set <RegisterName> (case-sensitive) in <LayerID> of <Block> (IDMA|TF|VPU) to
          <RegisterValue>.

        trace_ofmap <Index>
          Trace accumalation of OFMAP[<Index>]. Where <Index> = OW*M*oh+M*ow+m
          corresponding to OFMAP[oh][ow][m].  Use "log_level_tensors" to see mapping of
          <Index> to OBUF address.

      \endverbatim
   *
   * The internal xnne_vpu_dp model instances support the following commands using a
   * hierarchichal command handler name based on the xtsc_xnne instance name and the
   * xnne_sblk instance name (for example: "xnne0.sblk0.m_vpu_dp"):
   *  \verbatim
        dump_cfg_registers
          Dump the known config registers.

        dump_pipeline_reg
          Dump the 64 pipeline registers, 16 per line.

        printf_pipeline_reg
          Dump the output from calling printf_pipeline_reg() in the vpu_datapath C-model.

        reset [<Hard>]
          Call xnne_vpu_dp::reset(<Hard>).  Where <Hard> is 0|1 (default 0).

      \endverbatim
   */
  virtual void execute(const std::string&               cmd_line,
                       const std::vector<std::string>&  words,
                       const std::vector<std::string>&  words_lc,
                       std::ostream&                    result);


  /**
   * Reset the xtsc_xnne.
   */
  void reset(bool hard_reset = false);


  /// Dump the address map for the SBLK specified by index (default is all of them)
  void dump_map(std::ostream& os, xtsc::u32 index = 0xFFFFFFFF);


  /// Fill memory starting at address with num_bytes value
  void fill(xtsc_address address, u32 num_bytes, i8 value);


  /// Fill memory starting at address with 32-bit value for a total of "num_bytes" bytes
  void fill_u32(xtsc_address address, u32 num_bytes, u32 value);


  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


private:


  /// Handle incoming read requests 
  void rd_request_thread(void);


  /// Handle incoming read responses
  void rd_response_thread(void);


  /// Handle incoming write requests 
  void wr_request_thread(void);


  /// Handle incoming messages from XNNEMQ
  void xnne_msg_fifo_thread(void);


  /// Move message responses from each SBK to XNNERQ
  void xnne_rsp_fifo_thread(void);


  /// Handle incoming writes to internal state space on XNNEWR
  void xnne_wr_state_fifo_thread(void);


  /// Drive interrrupts to the Xtensa controller
  void xtensa_interrupts_thread(void);


  log4xtensa::TextLogger&       m_text;                 ///< For logging text messages

  // Note: Keep m_p last in class
  xtsc_xnne_parts&              m_p;                    ///< Internal parts

  friend class xtsc_xnne_parts;
};


} // namespace xtsc


#endif  // _XTSC_XNNE_H_
