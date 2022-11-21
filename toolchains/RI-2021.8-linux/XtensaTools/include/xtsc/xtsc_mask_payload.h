#ifndef _xtsc_mask_payload_H_
#define _xtsc_mask_payload_H_

// Copyright (c) 2005-2020 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */

#include <xtsc/xtsc.h>
#include <xtsc/xtsc_wire_write_if.h>
#include <vector>


namespace xtsc {

class xtsc_wire_write_if;

typedef xtsc::u8* mp_fifo_data_t;

class XTSC_API xtsc_mp_decomp_parms : public xtsc::xtsc_parms {
public:

    /**
   * Constructor for an xtsc_mp_decomp_parms object.
   *
   * @param     in_width8       Width of input data stream in bytes.
   *
   * @param     out_width8      Width of output data stream in bytes.
   *
   * @param     num_total_sym   Length of compressed flit in bytes. 
   *
   */


/**
 * Constructor parameters for a xtsc_mp_decomp object.
 *
 *  \verbatim
   Name                        Type   Description
   ------------------          ----   ---------------------------------------------------

   "in_width8"                 u32    Width of input data stream in bytes. Valid values
                                      are 16, 32, 64 and 128.
                                      Default = 128.

   "out_width8"                u32    Width of output data stream in bytes. Valid values
                                      are 32, 64 and 128.               
                                      Default = 128.

   "num_total_sym"             u32    Length of compressed flit. Valid values
                                      are 64 and 128.        
                                      Default = 128.

   "clock_period"              u32    This is the length of this decompressor's clock 
                                      period expressed in terms of the SystemC time 
                                      resolution (from sc_get_time_resolution()).  A 
                                      value of 0xFFFFFFFF means to use the XTSC system 
                                      clock period (from xtsc_get_system_clock_period()).
                                      A value of 0 means one delta cycle.
                                      Default = 0xFFFFFFFF (use the system clock period).
  
    \endverbatim
 *
 * @see xtsc_mp_decomp_parms
 * @see xtsc::xtsc_parms
 */

  xtsc_mp_decomp_parms(xtsc::u32    in_width8       = 128,
                       xtsc::u32    out_width8      = 128,
                       xtsc::u32    num_total_sym   = 128)
  {
    init(in_width8, out_width8, num_total_sym);
  }

  /**
   * Do initialization common to both constructors.
   */
  void init(xtsc::u32               in_width8       = 128,
            xtsc::u32               out_width8      = 128,
            xtsc::u32               num_total_sym   = 128)
  {
    add("in_width8",                in_width8);
    add("out_width8",               out_width8);
    add("num_total_sym",            num_total_sym);
    add("clock_period",             0xFFFFFFFF);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const                      { return "xtsc_mp_decomp_parms"; }
};



class XTSC_API xtsc_mp_comp_parms : public xtsc::xtsc_parms {
public:

    /**
   * Constructor for an xtsc_mp_comp_parms object.
   *
   * @param     in_width8       Width of input data stream in bytes.
   *
   * @param     out_width8      Width of output data stream in bytes.
   *
   * @param     num_total_sym   Length of compressed flit in bytes. 
   *
   */


/**
 * Constructor parameters for a xtsc_mp_comp object.
 *
 *  \verbatim
   Name                        Type   Description
   ------------------          ----   ---------------------------------------------------

   "in_width8"                 u32    Width of input data stream in bytes. Valid values
                                      are 32.
                                      Default = 32.

   "out_width8"                u32    Width of output data stream in bytes. Valid values
                                      are 32.               
                                      Default = 32.

   "num_total_sym"             u32    Length of uncompressed flit. Valid values
                                      are 64 and 128.        
                                      Default = 128.

   "clock_period"              u32    This is the length of this compressor's clock 
                                      period expressed in terms of the SystemC time 
                                      resolution (from sc_get_time_resolution()).  A 
                                      value of 0xFFFFFFFF means to use the XTSC system 
                                      clock period (from xtsc_get_system_clock_period()).
                                      A value of 0 means one delta cycle.
                                      Default = 0xFFFFFFFF (use the system clock period).
  
    \endverbatim
 *
 * @see xtsc_mp_comp_parms
 * @see xtsc::xtsc_parms
 */

  xtsc_mp_comp_parms(xtsc::u32      in_width8       = 32,
                     xtsc::u32      out_width8      = 32,
                     xtsc::u32      num_total_sym   = 128)
  {
    init(in_width8, out_width8, num_total_sym);
  }

  /**
   * Do initialization common to both constructors.
   */
  void init(xtsc::u32               in_width8       = 32,
            xtsc::u32               out_width8      = 32,
            xtsc::u32               num_total_sym   = 128)
  {
    add("in_width8",                in_width8);
    add("out_width8",               out_width8);
    add("num_total_sym",            num_total_sym);
    add("clock_period",             0xFFFFFFFF);
  }


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const                      { return "xtsc_mp_comp_parms"; }
};



/// Implementation of the internal input/output fifo.
class internal_fifo {
public:
  internal_fifo(xtsc::u32 tot_byte_size, xtsc::u32 entry_byte_size) :
    m_tot_byte_size   (tot_byte_size),
    m_entry_byte_size (entry_byte_size)
  {
    assert(m_tot_byte_size % m_entry_byte_size == 0);
    m_data = new xtsc::u8[tot_byte_size];
    reset();
  }

  ~internal_fifo() {
    delete m_data;
  }

  void reset() {
    m_num_avail_bytes = 0;
    m_num_free_bytes      = m_tot_byte_size;
    m_rd_ptr              = 0;
    m_wr_ptr              = 0;
  }

  xtsc::u32 num_avail_bytes() const                     { return m_num_avail_bytes; }

  xtsc::u32 num_avail_entries() const                   { return m_num_avail_bytes / m_entry_byte_size; }

  xtsc::u32 num_free_bytes() const                      { return m_num_free_bytes; }

  xtsc::u32 num_free_entries() const                    { return m_num_free_bytes  / m_entry_byte_size; }

  bool read(xtsc::u8 *data, xtsc::u32 byte_size);

  bool write(const xtsc::u8 *data, xtsc::u32 byte_size);

  bool peek(xtsc::u8 *data, xtsc::u32 byte_size);

private:
  xtsc::u32           m_tot_byte_size;                              ///< Total byte size of the fifo
  xtsc::u32           m_entry_byte_size;                            ///< Byte size of each entry
  xtsc::u32           m_num_avail_bytes;                            ///< Number of available bytes
  xtsc::u32           m_num_free_bytes;                             ///< Number of free bytes
  xtsc::u32           m_rd_ptr;                                     ///< Write pointer
  xtsc::u32           m_wr_ptr;                                     ///< Read pointer
  xtsc::u8           *m_data;                                       ///< Internal buffer
};



class XTSC_API xtsc_mask_payload_decomp : public sc_core::sc_module, public xtsc::xtsc_module, public xtsc::xtsc_command_handler_interface {
public:

  sc_core::sc_export<sc_core::sc_fifo_out_if<mp_fifo_data_t>>      m_in_stream_export;        ///<  Producer of input  data stream binds to this
  sc_core::sc_export<sc_core::sc_fifo_in_if<mp_fifo_data_t>>       m_out_stream_export;       ///<  Consumer of output data stream binds to this
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_wr_len_export;           ///<  I_wr_len
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_lut_pri0_export;         ///<  I_lut_pri0: 8 LUTs of pri0 symbols
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_lut_pri1_export;         ///<  I_lut_pri1: 8 LUTs of pri1 symbols
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_lut_pri2_export;         ///<  I_lut_pri2: 8 LUTs of pri2 symbols
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_reset_export;            ///<  MyReset
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_init_export;             ///<  I_init
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_init_instage_export;     ///<  I_init_instage
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_filt_uncomp_export;      ///<  I_filt_uncomp
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_lut_en_export;           ///<  I_lut_en
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_layer_id_export;         ///<  I_layer_id
  sc_core::sc_port  <xtsc::xtsc_wire_write_if, NSPP>               m_in_fifo_full_port;       ///<  O_in_fifo_full
  sc_core::sc_port  <xtsc::xtsc_wire_write_if, NSPP>               m_in_fifo_empty_port;      ///<  O_in_fifo_empty
  sc_core::sc_port  <xtsc::xtsc_wire_write_if, NSPP>               m_early_rdata_inc_port;    ///<  O_early_rdata_inc
  sc_core::sc_port  <xtsc::xtsc_wire_write_if, NSPP>               m_early_rdata_len_port;    ///<  O_early_rdata_len
  sc_core::sc_port  <xtsc::xtsc_wire_write_if, NSPP>               m_early_rdata_invalid_len_port;    ///<  O_early_rdata_invalid_len
  sc_core::sc_port  <xtsc::xtsc_wire_write_if, NSPP>               m_rdata_len_port;          ///<  O_rdata_len
  sc_core::sc_port  <xtsc::xtsc_wire_write_if, NSPP>               m_rdata_invalid_len_port;  ///<  O_rdata_invalid_len
  sc_core::sc_port  <xtsc::xtsc_wire_write_if, NSPP>               m_all_hs_empty_port;       ///<  O_all_hs_empty

  // For SystemC
  SC_HAS_PROCESS(xtsc_mask_payload_decomp);

  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const                      { return "xtsc_mask_payload_decomp"; }

  /**
   * Constructor for an xtsc_mask_payload_decomp.
   *
   * @param     module_name        Name of the mask_payload_decompressor sc_module.
   *
   * @param     input_byte_width   Data width of input stream in bytes.
   *
   * @param     output_byte_width  Data width of output stream in bytes.
   *
   */
  xtsc_mask_payload_decomp (sc_core::sc_module_name module_name, const xtsc_mp_decomp_parms& decomp_parms);


  /// Destructor.
  ~xtsc_mask_payload_decomp();


  /// SystemC callback
  void start_of_simulation();


  /// For xtsc_connection_interface
  virtual u32 get_bit_width(const std::string& port_name, u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// For xtsc_connection interface
  virtual std::string get_user_defined_port_type(const std::string& port_name) const;


  // TODO: Implementation of the xtsc::xtsc_command_handler_interface.
  virtual void execute(const std::string&               cmd_line,
                       const std::vector<std::string>&  words,
                       const std::vector<std::string>&  words_lc,
                       std::ostream&                    result);


  /// Get the input width of this decompressor in bytes
  xtsc::u32 get_in_width8()                             { return m_in_width8; }


  /// Get the input width of this decompressor in bits
  xtsc::u32 get_in_width1()                             { return m_in_width1; }


  /// Get the output width of this decompressor in bytes
  xtsc::u32 get_out_width8()                            { return m_out_width8; }


  /// Get the output width of this decompressor in bits
  xtsc::u32 get_out_width1()                            { return m_out_width1; }


  /// Backdoor method to decompress the input packet (used by XNNE)
  void bd_decompress_packet(u8* input_packet, u8* output_packet, u32 packet_len);


  /// Backdoor method to decode the packet length (used by XNNE)
  bool bd_decode_length(u8 length_byte, u32& decoded_length);


  /// Backdoor method to set LUT num (used by XNNE)
  void bd_set_lut_num (u8 lut_num) { m_hs0_lut_num = lut_num; }


  /// Backdoor method to set pri0 LUT specified by "lut_set_num" (used by XNNE)
  void bd_set_pri0(u8 lut_set_num, u8* lut_data);


  /// Backdoor method to set pri1 LUT specified by "lut_set_num" (used by XNNE)
  void bd_set_pri1(u8 lut_set_num, u8* lut_data);


  /// Backdoor method to set pri2 LUT specified by "lut_set_num" (used by XNNE)
  void bd_set_pri2(u8 lut_set_num, u8* lut_data);  


  /// Implementation of sc_fifo_out_if for input data stream of the decompressor.
  class sc_fifo_out_if_impl : public sc_core::sc_fifo_out_if<mp_fifo_data_t>, public sc_core::sc_object {
  public:

    /// Constructor
    sc_fifo_out_if_impl      (const char *object_name, xtsc_mask_payload_decomp& decompressor) :
      sc_object              (object_name),
      m_decompressor         (decompressor),
      m_p_port               (0)
    {}

    /// @see sc_fifo_out_if
    int num_free() const                                { return m_decompressor.m_in_fifo->num_free_bytes(); }

    /// @see sc_fifo_out_if
    bool nb_write(const mp_fifo_data_t&);

    /// @see sc_fifo_out_if (Do not use - Not implemented and will get an exception)
    void write(const mp_fifo_data_t&);

    /// @see sc_fifo_out_if
    const sc_core::sc_event& data_read_event() const    { return m_decompressor.infifo_read_event(); }

    /// Get the width in bits of the data port
    xtsc::u32 nb_get_bit_width() const                  { return m_decompressor.get_in_width1(); }

  private:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_mask_payload_decomp&               m_decompressor;         ///< Our decompressor object
    sc_core::sc_port_base                  *m_p_port;               ///< Port that is bound to us
  };



  /// Implementation of sc_fifo_in_if for output data stream of the decompressor
  class sc_fifo_in_if_impl : public sc_core::sc_fifo_in_if<mp_fifo_data_t>, public sc_core::sc_object {
  public:

    /// Constructor
    sc_fifo_in_if_impl       (const char *object_name, xtsc_mask_payload_decomp& decompressor) :
      sc_object              (object_name),
      m_decompressor         (decompressor),
      m_p_port               (0)
    {}

    /// @see sc_fifo_in_if
    int num_available() const                           { return m_decompressor.m_out_fifo->num_avail_bytes(); }

    /// @see sc_fifo_in_if
    bool nb_read(mp_fifo_data_t&);

    /// @see sc_fifo_in_if (Do not use - Not implemented and will get an exception)
    mp_fifo_data_t read();

    /// @see sc_fifo_in_if (Do not use - Not implemented and will get an exception)
    void read(mp_fifo_data_t&);

    /// @see sc_fifo_out_if
    const sc_core::sc_event& data_written_event() const { return m_decompressor.outfifo_written_event(); }

    /// Get the width in bits of the data port
    xtsc::u32 nb_get_bit_width() const                  { return m_decompressor.get_out_width1(); }

  private:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_mask_payload_decomp&               m_decompressor;            ///< Our decompressor object    
    sc_core::sc_port_base                  *m_p_port;                  ///< Port that is bound to us 
  };



  /// Implementation of xtsc_wire_write_if for wire exports
  class xtsc_wire_write_if_impl : virtual public xtsc::xtsc_wire_write_if, public sc_core::sc_object {
  public:
    xtsc_wire_write_if_impl(const char *object_name, xtsc_mask_payload_decomp& decompressor, const xtsc::u32 width, const xtsc::u32 state_id) :
      sc_object            (object_name),
      m_decompressor       (decompressor),
      m_p_port             (0),
      m_wire_name          (object_name),
      m_width              (width),
      m_state_id           (state_id)
    {}

    /// @see xtsc_wire_write_if
    void nb_write(const sc_dt::sc_unsigned& value);

    /// @see xtsc_wire_write_if
    xtsc::u32  nb_get_bit_width()                       { return m_width; }

  private:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_mask_payload_decomp&               m_decompressor;            ///< Our decompressor
    sc_core::sc_port_base                  *m_p_port;                  ///< Port that is bound to us
    const std::string                       m_wire_name;               ///< Wire name
    const xtsc::u32                         m_width;                   ///< Wire width
    const xtsc::u32                         m_state_id;                ///< State ID: indicating which wire it is
  };


  /// Enumation used to indicate which wire it is since all wires share the same impl class
  typedef enum wire_type : xtsc::u32 {
    LUT_PRI0       = 0,                // I_lut_pri0
    LUT_PRI1       = 1,                // I_lut_pri1
    LUT_PRI2       = 2,                // I_lut_pri2
    RESET          = 3,                // MyReset
    INIT           = 4,                // I_init
    INIT_INSTAGE   = 5,                // I_init_instage
    FILT_UNCOMP    = 6,                // I_filt_uncomp
    LUT_EN         = 7,                // I_lut_en
    LAYER_ID       = 8,                // I_layer_id
    WR_LEN         = 9                 // I_wr_len
  } wire_type_t;

  /// Enumation used to define the state of instage holding stage 0 (hs0)
  typedef enum hs0_state {
    INIT_STATE,
    READ_LEN,
    CHECK_LEN,
    WAIT_FOR_ALL_BYTES,
    DECOMPRESS,
    HAS_ERROR
  } hs0_state_t;

  /// Enumation used to define the payload type of each symbol
  typedef enum sym_type {
    PRI0,
    PRI1,
    PRI2,
    UNCODED,
    UNKNOWN_SYM
  } sym_type_t;

  /// Enumation used to define the type of the compression
  typedef enum compression_type {
    UNCOMPRESSED,
    ALL_PRI0,
    COMPRESSED,
    UNKNOWN_FORMAT
  } compression_type_t;


  void reset();
  void decode_lut_packet();
  void pass_packet();
  void decode_uncomp_packet();
  void decode_comp_packet();
  void decode_pri0_packet();
  void get_sym_type_table();
  void separate_4b_payload();
  void hs0_decompress(bool bd_mode = false);  // backdoor mode is used by xtsc_xnne with no port binding
  void pipeline_propogate();
  void decompress_thread();
  const sc_core::sc_event& infifo_read_event()     const { return m_infifo_read_event;     }
  const sc_core::sc_event& outfifo_written_event() const { return m_outfifo_written_event; }


private:
  sc_fifo_out_if_impl                       m_in_stream_push_impl;     ///<  Input  data stream           binds to this
  sc_fifo_in_if_impl                        m_out_stream_pop_impl;     ///<  Output data stream           binds to this
  xtsc_wire_write_if_impl                   m_wr_len_impl;             ///<  Input port of wr_len         binds to this
  xtsc_wire_write_if_impl                   m_lut_pri0_impl;           ///<  Input port of pri0 LUT       binds to this
  xtsc_wire_write_if_impl                   m_lut_pri1_impl;           ///<  Input port of pri1 LUT       binds to this
  xtsc_wire_write_if_impl                   m_lut_pri2_impl;           ///<  Input port of pri2 LUT       binds to this
  xtsc_wire_write_if_impl                   m_reset_impl;              ///<  Input port of reset          binds to this
  xtsc_wire_write_if_impl                   m_init_impl;               ///<  Input port of init           binds to this
  xtsc_wire_write_if_impl                   m_init_instage_impl;       ///<  Input port of init_instage   binds to this
  xtsc_wire_write_if_impl                   m_filt_uncomp_impl;        ///<  Input port of filt_uncomp    binds to this
  xtsc_wire_write_if_impl                   m_lut_en_impl;             ///<  Input port of lut_en         binds to this
  xtsc_wire_write_if_impl                   m_layer_id_impl;           ///<  Input port of layer_id       binds to this
  sc_core::sc_event                         m_outfifo_written_event;   ///<  Event when decompressor is writing data into out fifo
  sc_core::sc_event                         m_infifo_read_event;       ///<  Event when decompressor is reading data from in  fifo


  const xtsc::u32                           m_in_width8;               ///<  Byte width of input data stream
  const xtsc::u32                           m_in_width1;               ///<  Bit width of input data stream
  const xtsc::u32                           m_out_width8;              ///<  Byte width of output data stream
  const xtsc::u32                           m_out_width1;              ///<  Bit width of output data stream
  const xtsc::u32                           m_num_total_sym;           ///<  See "num_total_sym" parameter
  const xtsc::u32                           m_num_mask0_bytes;         ///<  Number of bytes in mask0
  const xtsc::u32                           m_max_flit_len;            ///<  Upper bound of flit length    
  const xtsc::u32                           m_min_flit_len;            ///<  Lower bound of flit length
  internal_fifo                            *m_in_fifo;                 ///<  Fifo for input  data stream
  internal_fifo                            *m_out_fifo;                ///<  Fifo for output data stream
  xtsc::u32                                 m_in_fifo_valid_bytes;     ///<  Number of valid bytes in m_in_fifo

  compression_type_t                        m_hs0_packet_format;       ///<  The format type of the packet being processed
  xtsc::u32                                 m_hs0_packet_len;          ///<  Length of the current packet in bytes (= m_hs0_byte0 + 1)
  xtsc::u8                                  m_hs0_byte0;               ///<  1st byte (byte 0) of the compressed packet (= actual length - 1)
  xtsc::u8                                  m_hs0_byte1;               ///<  2nd byte (byte 1) of the compressed packet (LUT_num: bits [7:5], payload_start_ptr[4:0])
  xtsc::u32                                 m_hs0_lut_num;             ///<  LUT number that the currect packet is using
  xtsc::u32                                 m_hs0_mask12_bytes;        ///<  Total bytes of Mask1 + Mask2 (they are bundled together) for hs0
  xtsc::u32                                 m_hs0_4b_payload_bytes;    ///<  Total bytes of 4-bit payload (pri1 + pri2 payload are bundled together) for hs0
  xtsc::u32                                 m_hs0_8b_payload_bytes;    ///<  Total bytes of 8-bit payload (uncompressed) for hs0
  xtsc::u32                                 m_hs0_mask1_bits;          ///<  Number of bits in mask1 for hs0
  xtsc::u32                                 m_hs0_mask2_bits;          ///<  Number of bits in mask2 for hs0
  xtsc::u32                                 m_hs0_pri0_sym_cnt;        ///<  Number of pri0 symbols for hs0
  xtsc::u32                                 m_hs0_pri1_sym_cnt;        ///<  Number of pri1 symbols for hs0
  xtsc::u32                                 m_hs0_pri2_sym_cnt;        ///<  Number of pri2 symbols for hs0
  xtsc::u32                                 m_hs0_uncoded_sym_cnt;     ///<  Number of uncoded (uncompressed) symbols for hs0
  bool                                      m_hs0_filt_uncomp_mode;    ///<  "Do not decompress" mode. See "I_flit_uncomp".
  bool                                      m_hs0_4b_to_8b_lut_mode;   ///<  Traditional 4b-to-8b Lookup table decompression (4b used as an index to look up LUT)
  xtsc::u32                                 m_hs0_4b_to_8b_lut_num;    ///<  To choose one pri1 table from the 8 pri1 set for Traditional 4b-to-8b Lookup decompression

  xtsc::u8                                 *m_hs0_mask0;               ///<  The Mask0 data for hs0
  xtsc::u8                                 *m_hs0_mask1;               ///<  The Mask1 data for hs0
  xtsc::u8                                 *m_hs0_mask2;               ///<  The Mask2 data for hs0
  xtsc::u8                                 *m_hs0_mask12;              ///<  The (Mask1 + Mask2) data (they are bundled together in the compressed packet) for hs0
  xtsc::u8                                 *m_hs0_4b_payload;          ///<  4-bit Payload (pri1 + pri2) raw data for hs0
  xtsc::u8                                 *m_hs0_8b_payload;          ///<  8-bit Payload (uncompressed) raw data for hs0
  xtsc::u8                                 *m_hs0_pri1_payload;        ///<  Payload for pri1 symbols (each symbol has a u8 storing 4-bit payload) for hs0
  xtsc::u8                                 *m_hs0_pri2_payload;        ///<  Payload for pri2 symbols (each symbol has a u8 storing 4-bit payload) for hs0

  xtsc::u8                                 *m_lut_pri0;                ///<  8 LUTs (each has 1-entry  and each entry is 8-bit)
  xtsc::u8                                **m_lut_pri1;                ///<  8 LUTs (each has 16-entry and each entry is 8-bit)
  xtsc::u8                                **m_lut_pri2;                ///<  8 LUTs (each has 16-entry and each entry is 8-bit)

  sym_type_t                               *m_hs0_sym_type_table;      ///<  One-dimension array as the table for querying symbol type based on index
  hs0_state_t                               m_hs0_state;               ///<  State-machine state of instage hs0 (holding stage 0) 
  xtsc::u8                                 *m_hs0_decoded_sym;         ///<  Decoded symbol of hs0 (each symbol is a byte)
  xtsc::u8                                 *m_hs1_decoded_sym;         ///<  Decoded symbol of hs1 (each symbol is a byte)
  xtsc::u8                                 *m_hs2_decoded_sym;         ///<  Decoded symbol of hs2 (each symbol is a byte)
  xtsc::u8                                 *m_hs3_decoded_sym;         ///<  Decoded symbol of hs3 (each symbol is a byte)
  xtsc::u8                                 *m_hs4_decoded_sym;         ///<  Decoded symbol of hs4 (each symbol is a byte)
  xtsc::u32                                 m_hs4_bytes_written;       ///<  Bytes of hs4 decoded symbols that have been written to out_fifo
  xtsc::u32                                 m_hs4_bytes_to_write;      ///<  Bytes of hs4 decoded symbols that need to be written to out_fifo

  bool                                      m_pipeline_stalled;        ///<  Whether or not pipeline gets stalled (back-pressure from out fifo)
  bool                                      m_pipeline_propagated;     ///<  Whether or not pipeline has been propagated in this cycle

  bool                                      m_hs0_data_valid;          ///<  Whether or not hs0 has valid data
  bool                                      m_hs1_data_valid;          ///<  Whether or not hs1 has valid data
  bool                                      m_hs2_data_valid;          ///<  Whether or not hs2 has valid data
  bool                                      m_hs3_data_valid;          ///<  Whether or not hs3 has valid data
  bool                                      m_hs4_data_valid;          ///<  Whether or not hs4 has valid data

  sc_core::sc_event                         m_decompress_event;        ///<  Event to signal decompress_thread()
  std::vector<sc_core::sc_process_handle>   m_process_handles;         ///<  For reset

  sc_core::sc_time                          m_clock_period;            ///<  This decompressor's clock period
  xtsc::u64                                 m_clock_period_value;      ///<  Clock period as u64
  sc_core::sc_time                          m_time_resolution;         ///<  SystemC time resolution
  log4xtensa::TextLogger&                   m_text;                    ///<  Text logger
  log4xtensa::BinaryLogger&                 m_binary;                  ///<  Binary logger
};



class XTSC_API xtsc_mask_payload_comp : public sc_core::sc_module, public xtsc::xtsc_module, public xtsc::xtsc_command_handler_interface {
public:

  sc_core::sc_export<sc_core::sc_fifo_out_if<mp_fifo_data_t>>      m_in_stream_export;        ///<  Producer of input  data stream binds to this
  sc_core::sc_export<sc_core::sc_fifo_in_if<mp_fifo_data_t>>       m_out_stream_export;       ///<  Consumer of output data stream binds to this
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_lut_pri0_export;         ///<  I_lut_pri0: 1 LUTs of pri0 symbols
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_lut_pri1_export;         ///<  I_lut_pri1: 1 LUTs of pri1 symbols
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_lut_pri2_export;         ///<  I_lut_pri2: 1 LUTs of pri2 symbols
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_reset_export;            ///<  MyReset
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_wr_last_export;          ///<  I_wr_last
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_init_export;             ///<  I_init
  sc_core::sc_export<xtsc::xtsc_wire_write_if>                     m_lut_num_export;          ///<  I_lut_num
  sc_core::sc_port  <xtsc::xtsc_wire_write_if, NSPP>               m_ready_port;              ///<  O_ready
  sc_core::sc_port  <xtsc::xtsc_wire_write_if, NSPP>               m_sym_enc_last_port;       ///<  O_sym_enc_last
  sc_core::sc_port  <xtsc::xtsc_wire_write_if, NSPP>               m_sym_enc_rdata_len_port;  ///<  O_sym_enc_rdata_len
  sc_core::sc_port  <xtsc::xtsc_wire_write_if, NSPP>               m_all_hs_empty_port;       ///<  O_all_hs_empty


  // For SystemC
  SC_HAS_PROCESS(xtsc_mask_payload_comp);

  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const                      { return "xtsc_mask_payload_comp"; }


  /**
   * Constructor for an xtsc_mask_payload_comp.
   *
   * @param     module_name        Name of the mask_payload_compressor sc_module.
   *
   * @param     input_byte_width   Data width of input stream in bytes.
   *
   * @param     output_byte_width  Data width of output stream in bytes.
   *
   */
  xtsc_mask_payload_comp (sc_core::sc_module_name module_name, const xtsc_mp_comp_parms& comp_parms);


  /// Destructor.
  ~xtsc_mask_payload_comp();


  /// SystemC callback
  void start_of_simulation();


  /// For xtsc_connection_interface
  virtual u32 get_bit_width(const std::string& port_name, u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /// For xtsc_connection interface
  virtual std::string get_user_defined_port_type(const std::string& port_name) const;


  // TODO: Implementation of the xtsc::xtsc_command_handler_interface.
  virtual void execute(const std::string&               cmd_line,
                       const std::vector<std::string>&  words,
                       const std::vector<std::string>&  words_lc,
                       std::ostream&                    result);


  /// Get the input width of this decompressor in bytes
  xtsc::u32 get_in_width8()                             { return m_in_width8; }


  /// Get the input width of this decompressor in bits
  xtsc::u32 get_in_width1()                             { return m_in_width1; }


  /// Get the output width of this decompressor in bytes
  xtsc::u32 get_out_width8()                            { return m_out_width8; }


  /// Get the output width of this decompressor in bits
  xtsc::u32 get_out_width1()                            { return m_out_width1; }


  /// Backdoor method to compress the input packet (used by XNNE)
  void bd_compress_packet(u8* input_packet, u8* output_packet, u32& cmpr_packet_len);


  /// Backdoor method to set LUT num (used by XNNE)
  void bd_set_lut_num (u8 lut_num) { m_lut_num = lut_num; }


  /// Backdoor method to set pri0 LUT (used by XNNE)
  void bd_set_pri0(u8* lut_data);


  /// Backdoor method to set pri1 LUT (used by XNNE)
  void bd_set_pri1(u8* lut_data);


  /// Backdoor method to set pri2 LUT (used by XNNE)
  void bd_set_pri2(u8* lut_data);  

  /// Implementation of sc_fifo_out_if for input data stream of the compressor.
  class sc_fifo_out_if_impl : public sc_core::sc_fifo_out_if<mp_fifo_data_t>, public sc_core::sc_object {
  public:

    /// Constructor
    sc_fifo_out_if_impl      (const char *object_name, xtsc_mask_payload_comp& compressor) :
      sc_object              (object_name),
      m_compressor           (compressor),
      m_p_port               (0)
    {}

    /// @see sc_fifo_out_if
    int num_free() const                                { return m_compressor.m_in_fifo->num_free_bytes(); }

    /// @see sc_fifo_out_if
    bool nb_write(const mp_fifo_data_t&);

    /// @see sc_fifo_out_if (Do not use - Not implemented and will get an exception)
    void write(const mp_fifo_data_t&);

    /// @see sc_fifo_out_if
    const sc_core::sc_event& data_read_event() const    { return m_compressor.infifo_read_event(); }

    /// Get the width in bits of the data port
    xtsc::u32 nb_get_bit_width() const                  { return m_compressor.get_in_width1(); }

  private:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_mask_payload_comp&                 m_compressor;           ///< Our compressor object
    sc_core::sc_port_base                  *m_p_port;               ///< Port that is bound to us
  };



  /// Implementation of sc_fifo_in_if for output data stream of the compressor
  class sc_fifo_in_if_impl : public sc_core::sc_fifo_in_if<mp_fifo_data_t>, public sc_core::sc_object {
  public:

    /// Constructor
    sc_fifo_in_if_impl       (const char *object_name, xtsc_mask_payload_comp& compressor) :
      sc_object              (object_name),
      m_compressor           (compressor),
      m_p_port               (0)
    {}

    /// @see sc_fifo_in_if
    int num_available() const                           { return m_compressor.m_out_fifo->num_avail_bytes(); }

    /// @see sc_fifo_in_if
    bool nb_read(mp_fifo_data_t&);

    /// @see sc_fifo_in_if (Do not use - Not implemented and will get an exception)
    mp_fifo_data_t read();

    /// @see sc_fifo_in_if (Do not use - Not implemented and will get an exception)
    void read(mp_fifo_data_t&);

    /// @see sc_fifo_out_if
    const sc_core::sc_event& data_written_event() const { return m_compressor.outfifo_written_event(); }

    /// Get the width in bits of the data port
    xtsc::u32 nb_get_bit_width() const                  { return m_compressor.get_out_width1(); }

  private:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_mask_payload_comp&                 m_compressor;              ///< Our compressor object    
    sc_core::sc_port_base                  *m_p_port;                  ///< Port that is bound to us
  };



  /// Implementation of xtsc_wire_write_if for wire exports
  class xtsc_wire_write_if_impl : virtual public xtsc::xtsc_wire_write_if, public sc_core::sc_object {
  public:

    /// Constructor
    xtsc_wire_write_if_impl(const char *object_name, xtsc_mask_payload_comp& compressor, const xtsc::u32 width, const xtsc::u32 state_id) :
      sc_object            (object_name),
      m_compressor         (compressor),
      m_p_port             (0),
      m_wire_name          (object_name),
      m_width              (width),
      m_state_id           (state_id)
    {}

    /// @see xtsc_wire_write_if
    void nb_write(const sc_dt::sc_unsigned& value);

    /// @see xtsc_wire_write_if
    xtsc::u32  nb_get_bit_width()                       { return m_width; }

  private:

    /// SystemC callback when something binds to us
    virtual void register_port(sc_core::sc_port_base& port, const char *if_typename);

    xtsc_mask_payload_comp&                 m_compressor;           ///< Our compressor
    sc_core::sc_port_base                  *m_p_port;               ///< Port that is bound to us
    const std::string                       m_wire_name;            ///< Wire name
    const xtsc::u32                         m_width;                ///< Wire width
    const xtsc::u32                         m_state_id;             ///< State ID: indicating which wire it is
  };


  /// Enumation used to indicate which wire it is since all wires share the same impl class
  typedef enum wire_type : xtsc::u32 {
    LUT_PRI0       = 0,            // I_lut_pri0
    LUT_PRI1       = 1,            // I_lut_pri0
    LUT_PRI2       = 2,            // I_lut_pri0
    RESET          = 3,            // MyReset
    WR_LAST        = 4,            // I_wr_last
    INIT           = 5,            // I_init
    LUT_NUM        = 6             // I_lut_num
  } wire_type_t;

  /// Enumation used to define the state of instage holding stage 0 (hs0)
  typedef enum hs0_state {
    INIT_STATE,
    WAIT_FOR_ALL_BYTES,
    COMPRESS,
    HAS_ERROR
  } hs0_state_t;

  /// Enumation used to define the payload type of each symbol
  typedef enum sym_type {
    PRI0,
    PRI1,
    PRI2,
    UNCODED,
    UNKNOWN_SYM
  } sym_type_t;

  /// Enumation used to define the type of the compression
  typedef enum compression_type {
    UNCOMPRESSED,
    ALL_PRI0,
    COMPRESSED,
    UNKNOWN_FORMAT
  } compression_type_t;

  class encoded_data {
  public:
    xtsc::u8*          data;                                          ///< Hold the encoded data of this packet
    xtsc::u32          byte_length;                                   ///< The length in bytes of the encoded data
    xtsc::u32          rounded_byte_length;                           ///< "byte_length" rounded to output fifo width
    xtsc::u32          bytes_written;                                 ///< Bytes that have been written to out_fifo
    xtsc::u32          bytes_to_write;                                ///< Bytes that need to be written to out_fifo
    xtsc::u32          encoded_buf_bytes;                             ///< Size in bytes of encoded buffer
    compression_type_t encode_type;                                   ///< The type of the encoded packet
    bool               last_packet;                                   ///< Indicating if this is the last packet to compress (used for "O_sym_enc_last")

    encoded_data(xtsc::u32 encoded_buf_bytes);
    ~encoded_data();
    void reset();
  };


  void reset();
  void hs0_scan_and_match();
  void hs0_get_enc_length();
  void hs0_compress();
  void pipeline_propogate();
  void copy_encoded_data(const encoded_data* source, encoded_data* dest);
  void compress_thread();
  const sc_core::sc_event& infifo_read_event()     const { return m_infifo_read_event;     }
  const sc_core::sc_event& outfifo_written_event() const { return m_outfifo_written_event; }


private:
  sc_fifo_out_if_impl                       m_in_stream_push_impl;     ///<  Input  data stream      binds to this
  sc_fifo_in_if_impl                        m_out_stream_pop_impl;     ///<  Output data stream      binds to this
  xtsc_wire_write_if_impl                   m_lut_pri0_impl;           ///<  Input port of pri0 LUT  binds to this
  xtsc_wire_write_if_impl                   m_lut_pri1_impl;           ///<  Input port of pri1 LUT  binds to this
  xtsc_wire_write_if_impl                   m_lut_pri2_impl;           ///<  Input port of pri2 LUT  binds to this
  xtsc_wire_write_if_impl                   m_reset_impl;              ///<  Input port of reset     binds to this
  xtsc_wire_write_if_impl                   m_wr_last_impl;            ///<  Input port of wr_last   binds to this
  xtsc_wire_write_if_impl                   m_init_impl;               ///<  Input port of init      binds to this
  xtsc_wire_write_if_impl                   m_lut_num_impl;            ///<  Input port of lut_num   binds to this
  sc_core::sc_event                         m_outfifo_written_event;   ///<  Event when decompressor is writing data into out fifo
  sc_core::sc_event                         m_infifo_read_event;       ///<  Event when decompressor is reading data from in  fifo

  const xtsc::u32                           m_in_width8;               ///<  Byte width of input data stream
  const xtsc::u32                           m_in_width1;               ///<  Bit width of input data stream
  const xtsc::u32                           m_out_width8;              ///<  Byte width of output data stream
  const xtsc::u32                           m_out_width1;              ///<  Bit width of output data stream
  const xtsc::u32                           m_num_total_sym;           ///<  See "num_total_sym" parameter
  const xtsc::u32                           m_num_mask0_bytes;         ///<  Number of bytes in mask0
  const xtsc::u32                           m_max_flit_len;            ///<  Upper bound of flit length    
  const xtsc::u32                           m_min_flit_len;            ///<  Lower bound of flit length
  const xtsc::u32                           m_encoded_buf_bytes;       ///<  Encoded buffer bytes
  internal_fifo                            *m_in_fifo;                 ///<  Fifo for input  data stream
  internal_fifo                            *m_out_fifo;                ///<  Fifo for output data stream

  xtsc::u8                                  m_lut_num;                 ///<  Indicates which one of the 8 LUTs is being used
  xtsc::u8                                  m_lut_pri0;                ///<  1 LUT for pri0 (1-entry  and each entry is 8-bit)
  xtsc::u8                                 *m_lut_pri1;                ///<  1 LUT for pri1 (16-entry and each entry is 8-bit)
  xtsc::u8                                 *m_lut_pri2;                ///<  1 LUT for pri2 (16-entry and each entry is 8-bit)
  bool                                      m_infifo_last_packet;      ///<  The packet in the in_fifo  is the last packet
  bool                                      m_outfifo_last_packet;     ///<  The packet in the out_fifo is the last packet
  xtsc::u32                                 m_outfifo_last_len;        ///<  The useful length of the last beat of the last packet

  sym_type_t                               *m_hs0_sym_type_table;      ///<  One-dimension array as the table for querying symbol type based on index
  hs0_state_t                               m_hs0_state;               ///<  State-machine state of instage hs0 (holding stage 0)
  xtsc::u8                                 *m_hs0_orig_sym_data;       ///<  The original uncoded data for hs0
  xtsc::u32                                 m_hs0_pri0_sym_cnt;        ///<  Number of pri0 symbols for hs0
  xtsc::u32                                 m_hs0_pri1_sym_cnt;        ///<  Number of pri1 symbols for hs0
  xtsc::u32                                 m_hs0_pri2_sym_cnt;        ///<  Number of pri2 symbols for hs0
  xtsc::u32                                 m_hs0_uncoded_sym_cnt;     ///<  Number of uncoded symbols for hs0
  xtsc::u32                                 m_hs0_mask1_bits;          ///<  Number of bits in mask1 for hs0
  xtsc::u32                                 m_hs0_mask2_bits;          ///<  Number of bits in mask2 for hs0
  xtsc::u32                                 m_hs0_mask12_bytes;        ///<  Number of bytes in mask1 and mask2 for hs0
  xtsc::u32                                 m_hs0_4b_payload_bytes;    ///<  Number of bytes in 4-bit payload for hs0
  xtsc::u32                                 m_hs0_8b_payload_bytes;    ///<  Number of bytes in 8-bit payload for hs0
  xtsc::u32                                 m_hs0_enc_length;          ///<  The length of the packet if encoded (to determine whether encoded or not)
  xtsc::u8                                 *m_hs0_pri1_lut_num;        ///<  The pri1 LUT entry number for each symbol (if belonged to pri1) in hs0
  xtsc::u8                                 *m_hs0_pri2_lut_num;        ///<  The pri2 LUT entry number for each symbol (if belonged to pri2) in hs0

  encoded_data                             *m_hs0_encoded_data;        ///<  Encoded symbol of hs0
  encoded_data                             *m_hs1_encoded_data;        ///<  Encoded symbol of hs1
  encoded_data                             *m_hs2_encoded_data;        ///<  Encoded symbol of hs2
  encoded_data                             *m_hs3_encoded_data;        ///<  Encoded symbol of hs3
  encoded_data                             *m_hs4_encoded_data;        ///<  Encoded symbol of hs4

  bool                                      m_pipeline_stalled;        ///<  Whether or not pipeline gets stalled (back-pressure from out fifo)
  bool                                      m_pipeline_propagated;     ///<  Whether or not pipeline has been propagated in this cycle

  bool                                      m_hs0_data_valid;          ///<  Whether or not hs0 has valid data
  bool                                      m_hs1_data_valid;          ///<  Whether or not hs1 has valid data
  bool                                      m_hs2_data_valid;          ///<  Whether or not hs2 has valid data
  bool                                      m_hs3_data_valid;          ///<  Whether or not hs3 has valid data
  bool                                      m_hs4_data_valid;          ///<  Whether or not hs4 has valid data

  sc_core::sc_event                         m_compress_event;          ///<  Event to signal compress_thread()
  std::vector<sc_core::sc_process_handle>   m_process_handles;         ///<  For reset

  sc_core::sc_time                          m_clock_period;            ///<  This compressor's clock period
  xtsc::u64                                 m_clock_period_value;      ///<  Clock period as u64
  sc_core::sc_time                          m_time_resolution;         ///<  SystemC time resolution
  log4xtensa::TextLogger&                   m_text;                    ///<  Text logger
  log4xtensa::BinaryLogger&                 m_binary;                  ///<  Binary logger
};


}  // namespace xtsc

#endif  // _xtsc_mask_payload_H_
