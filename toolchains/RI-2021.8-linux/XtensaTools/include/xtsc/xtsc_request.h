#ifndef _xtsc_request_h_
#define _xtsc_request_h_

// Copyright (c) 2005-2017 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */


#include <xtsc/xtsc.h>


namespace xtsc {

/**
 * Class representing a PIF, AXI, APB, XLMI, or LX local memory request transfer (beat).
 *
 * The general 2-step procedure to create a request is:
 *  -# Construct the xtsc_request object using the appropriate xtsc_request constructor
 *     for the type of request you want to create (see type_t).  These xtsc_request
 *     constructors all have parameters.
 *  -# Use the set_buffer() or get_buffer() methods to fill in the data payload.  This
 *     step is only needed for RCW, WRITE, BLOCK_WRITE, and BURST_WRITE request types.
 *
 * If you wish to preconstruct an empty xtsc_request object before you know the request
 * type (for example to create a pool of xtsc_request objects to improve performance),
 * use the xtsc_request constructor that takes no parameters.  When you are ready to use
 * the xtsc_request object:
 *  -# Call the the appropriate xtsc_request initialize() method for the type of request
 *     you want to create (see type_t).
 *  -# Use the set_buffer() or get_buffer() methods to fill in the data payload.  This
 *     step is only needed for RCW, WRITE, BLOCK_WRITE, and BURST_WRITE request types.
 *
 * If desired, the above two procedures can be mixed.  For example, create an RCW
 * xtsc_request using the main xtsc_request constructor and then, when the time comes to
 * create the second RCW request, call the special initialize() method meant for the
 * second RCW request.  This same technique can be used for BLOCK_WRITE and BURST_WRITE
 * transactions.
 *
 * For protocol and timing information specific to xtsc_core, see
 * xtsc_core::Information_on_memory_interface_protocols.
 *
 * @see xtsc_request_if
 * @see xtsc_respond_if
 * @see xtsc_response
 * @see xtsc_core::Information_on_memory_interface_protocols.
 * @see xtsc_component::xtsc_arbiter
 * @see xtsc_component::xtsc_dma_engine
 * @see xtsc_component::xtsc_master
 * @see xtsc_component::xtsc_mmio
 * @see xtsc_component::xtsc_memory
 * @see xtsc_component::xtsc_router
 * @see xtsc_component::xtsc_slave
 */
class XTSC_API xtsc_request {
public:


  /**
   * Enumeration used to identify the request type.
   *
   */
  typedef enum type_t {
    READ               = 0x00,  ///<  Single read
    BLOCK_READ         = 0x10,  ///<  Block read (num_transfers = 2|4|8|16)
    BURST_READ         = 0x30,  ///<  Burst read.  If m_burst is NON_AXI, then num_transfers between 2 and 16, inclusive
    RCW                = 0x50,  ///<  Read-conditional-write
    SNOOP              = 0x60,  ///<  Snoop request to an Xtensa config supporting data cache coherence
    WRITE              = 0x80,  ///<  Write
    BLOCK_WRITE        = 0x90,  ///<  Block write (num_transfers = 2|4|8|16)
    BURST_WRITE        = 0xB0,  ///<  Burst write.  If m_burst is NON_AXI, then num_transfers between 2 and 16, inclusive 
  } type_t;



  /**
   * Enumeration used to identify the AXI burst type.
   */
  typedef enum burst_t {
    NON_AXI             = 0x0,  ///<  A non-AXI request (e.g. PIF, APB, or LX local memory)
    FIXED               = 0x1,  ///<  AXI fixed burst
    INCR                = 0x2,  ///<  AXI incrementing burst
    WRAP                = 0x3,  ///<  AXI wrapping burst
  } burst_t;


  static const xtsc::u8 MUEvict = 0x4;   ///<  AxSNOOP: Memory Update transaction: Evict
  static const xtsc::u8 CaMx_CS = 0x8;   ///<  AxSNOOP: Cache Maintenance: CleanShared
  static const xtsc::u8 CaMx_CI = 0x9;   ///<  AxSNOOP: Cache Maintenance: CleanInvalid
  static const xtsc::u8 CaMx_CU = 0xB;   ///<  AxSNOOP: Cache Maintenance: CleanUnique
  static const xtsc::u8 CaMx_MI = 0xD;   ///<  AxSNOOP: Cache Maintenance: MakeInvalid

  /**
   * Enumeration used to identify the AXI snoop transaction type for AxSnoop bits.
   */
  typedef enum snoop_t {
    READ_NO_SNOOP          = 0,  ///<  ARSNOOP: Non-snooping: ReadNoSnoop 
    READ_ONCE              = 0,  ///<  ARSNOOP: Coherent: ReadOnce 
    READ_SHARED            = 1,  ///<  ARSNOOP: Coherent: ReadShared
    READ_CLEAN             = 2,  ///<  ARSNOOP: Coherent: ReadClean
    READ_NOT_SHARED_DIRTY  = 3,  ///<  ARSNOOP: Coherent: ReadNotSharedDirty 
    READ_UNIQUE            = 7,  ///<  ARSNOOP: Coherent: ReadUnique
    CLEAN_UNIQUE           = 11, ///<  ARSNOOP: Coherent: CleanUnique
    MAKE_UNIQUE            = 12, ///<  ARSNOOP: Coherent: MakeUnique
    CLEAN_SHARED           = 8,  ///<  ARSNOOP: Cache Maintenance: CleanShared
    CLEAN_INVALID          = 9,  ///<  ARSNOOP: Cache Maintenance: CleanInvalid
    MAKE_INVALID           = 13, ///<  ARSNOOP: Cache Maintenance: MakeInvalid 
    DVM_COMPLETE           = 14, ///<  ARSNOOP: DVM: DVM Complete 
    DVP_MESSAGE            = 15, ///<  ARSNOOP: DVM: DVM Message 
    WRITE_NO_SNOOP         = 0,  ///<  AWSNOOP: Non-snooping: WriteNoSnoop 
    WRITE_UNIQUE           = 0,  ///<  AWSNOOP: Coherent: WriteUnique 
    WRITE_LINE_UNIQUE      = 1,  ///<  AWSNOOP: Coherent: WriteLineUnique 
    WRITE_CLEAN            = 2,  ///<  AWSNOOP: Memory Update: WriteClean 
    WRITE_BACK             = 3,  ///<  AWSNOOP: Memory Update: WriteBack 
    EVICT                  = 4,  ///<  AWSNOOP: Memory Update: Evict 
    WRITE_EVICT            = 5,  ///<  AWSNOOP: Memory Update: WriteEvict
  } snoop_t;

  /**
   * Constructor for an empty xtsc_request object used to create a pool of pre-allocated
   * xtsc_request objects.  Before using an xtsc_request object that was created with
   * this constructor, either assign another xtsc_request object to it or call one of
   * the initialize() methods on it.
   */
  xtsc_request();


  /**
   * Constructor for most kinds of xtsc_request objects.
   *
   * This constructor is used to create the following kinds of xtsc_request objects:
   *  - READ
   *  - BLOCK_READ
   *  - BURST_READ  (PIF or AXI)
   *  - RCW         (first transfer only)
   *  - WRITE
   *  - BLOCK_WRITE (first transfer only)
   *  - BURST_WRITE (first transfer only) (PIF or AXI)
   *  - SNOOP
   *
   * @param     type            Type of request. See xtsc_request::type_t.
   * @param     address8        Byte address of request.  See set_byte_address().
   * @param     size8           Size in bytes of each transfer.  See set_byte_size().
   * @param     tag             The tag if it has already been assigned by the Xtensa
   *                            ISS.  If not, pass in 0 (the default) and a new non-zero
   *                            tag will be assigned (and can be obtained by calling
   *                            the get_tag() method).
   * @param     num_transfers   See set_num_transfers().
   * @param     byte_enables    See set_byte_enables().  If burst is NON_AXI and type is
   *                            BLOCK_WRITE, BLOCK_READ, or BURST_READ then this
   *                            parameter is ignored and the byte enables are all set
   *                            for a bus width corresponding to size8.  If a different
   *                            set of byte enables is desired for BLOCK_WRITE, then
   *                            call set_byte_enables() after calling this constructor
   *                            or the corresponding initialize() method.
   * @param     last_transfer   See set_last_transfer().
   * @param     route_id        The route ID.  See set_route_id().
   * @param     id              The PIF ID.  See set_id().
   * @param     priority        The transaction priority.  See set_priority().
   * @param     pc              The associated processor program counter. See set_pc().
   * @param     burst           The AXI burst type.
   * @param     apb             See is_apb_protocol().
   */
  xtsc_request( type_t                type,
                xtsc_address          address8,
                u32                   size8,
                u64                   tag             = 0,
                u32                   num_transfers   = 1,
                xtsc_byte_enables     byte_enables    = 0xFFFF,
                bool                  last_transfer   = true,
                u32                   route_id        = 0,
                u8                    id              = 0,
                u8                    priority        = 2,
                xtsc_address          pc              = 0xFFFFFFFF,
                burst_t               burst           = NON_AXI,
                bool                  apb             = false
              );


  /**
   * Constructor for second through last request of a BLOCK_WRITE.
   *
   * @param     tag             The tag from the first request of the BLOCK_WRITE
   *                            sequence.
   * @param     address8        Byte address of request.  This should increment
   *                            by size8 (bus width) for each request of the BLOCK_WRITE
   *                            sequence.
   * @param     size8           Size in bytes of each transfer.  For BLOCK_WRITE, this
   *                            is always equal to the bus width.
   * @param     num_transfers   The total number of BLOCK_WRITE request transfers.
   *                            This should be the same number as in the first 
   *                            request of the BLOCK_WRITE sequence.
   * @param     last_transfer   True if this is the last request of the BLOCK_WRITE
   *                            sequence. 
   * @param     route_id        The route ID.  See set_route_id().
   * @param     id              The PIF ID.  See set_id().
   * @param     priority        The transaction priority.  See set_priority().
   * @param     pc              The associated processor program counter. See set_pc().
   *
   * Note:  This constructor sets the the byte enables for all bytes of the bus (based
   *        on size8).  If you want byte enables other then this, call 
   *        set_byte_enables() after calling this constructor.
   *
   */
  xtsc_request( u64                   tag,
                xtsc_address          address8,
                u32                   size8,
                u32                   num_transfers,
                bool                  last_transfer,
                u32                   route_id        = 0,
                u8                    id              = 0,
                u8                    priority        = 2,
                xtsc_address          pc              = 0xFFFFFFFF
              );


  /**
   * Constructor for the second (last) request transfer of a RCW sequence.
   *
   * @param     tag             The tag from the first request of the RCW
   *                            sequence.
   * @param     address8        Byte address of request.  This should be the same
   *                            as the address of the first RCW request transfer.
   * @param     route_id        The route ID.  See set_route_id().
   * @param     id              The PIF ID.  See set_id().
   * @param     priority        The transaction priority.  See set_priority().
   * @param     pc              The associated processor program counter. See set_pc().
   *
   * Note:  This constructor sets the byte size to 4 and the byte enables to 0xF.  If
   *        you want an RCW request with a byte size other then 4, then call
   *        set_byte_size() after calling this constructor.  If you want byte enables
   *        other then 0xF, call set_byte_enables() after calling this constructor.
   *
   * Note:  If you use literals or non-byte integers for both id and priority, then at
   *        least one of them will need to be cast to a u8 to disambiguate this
   *        constructor from the BLOCK_WRITE constructor.  For example:
   *  \verbatim
               xtsc_request rcw2(tag, address8, 0, 0, (u8) 0, pc);
      \endverbatim
   */
  xtsc_request( u64                     tag,
                xtsc_address            address8,
                u32                     route_id        = 0,
                u8                      id              = 0,
                u8                      priority        = 2,
                xtsc_address            pc              = 0xFFFFFFFF
              );


  /**
   * Constructor for second through last request of a BURST_WRITE (PIF or AXI).
   *
   * @param     hw_address8     For PIF, this should be equal to the lowest byte address
   *                            enabled by a byte enable in the first request of the
   *                            sequence.  For AXI, this should be equal to the address8
   *                            argument passed to the constructor or initialize method
   *                            for the first transfer of the BURST_WRITE sequence.  
   *                            See get_hardware_address().
   * @param     tag             The tag from the first request of the BURST_WRITE
   *                            sequence.
   * @param     address8        Byte address of request.  This starts out bus-width-
   *                            aligned and should increment by the bus width for each
   *                            request of the BURST_WRITE sequence.
   * @param     size8           Size in bytes of each transfer.  Should not exceed bus
   *                            width (for PIF should equal bus width).
   *                            See set_byte_size().
   * @param     num_transfers   The total number of BURST_WRITE request transfers.
   *                            This should be the same number as in the first 
   *                            request of the BURST_WRITE sequence.
   * @param     transfer_num    The sequential number of this transfer (valid values are
   *                            2 through num_transfers).
   * @param     byte_enables    The bytes to be written.  See set_byte_enables().
   * @param     route_id        The route ID.  See set_route_id().
   * @param     id              The PIF ID.  See set_id().
   * @param     priority        The transaction priority.  See set_priority().
   * @param     pc              The associated processor program counter. See set_pc().
   * @param     burst           The AXI burst type.
   */
  xtsc_request( xtsc_address          hw_address8,
                u64                   tag,
                xtsc_address          address8,
                u32                   size8,
                u32                   num_transfers,
                u32                   transfer_num,
                xtsc_byte_enables     byte_enables    = 0xFFFF,
                u32                   route_id        = 0,
                u8                    id              = 0,
                u8                    priority        = 2,
                xtsc_address          pc              = 0xFFFFFFFF,
                burst_t               burst           = NON_AXI
              );


  /// Copy Constructor
  xtsc_request(const xtsc_request& request);


  /// Copy assignment
  xtsc_request& operator=(const xtsc_request& request);


  /// Destructor
  ~xtsc_request();


  /**
   * Initializer for most kinds of xtsc_request objects.
   *
   * This method is used to initialize the following kinds of xtsc_request objects:
   *  - READ
   *  - BLOCK_READ
   *  - BURST_READ  (PIF or AXI)
   *  - RCW         (first transfer only)
   *  - WRITE
   *  - BLOCK_WRITE (first transfer only)
   *  - BURST_WRITE (first transfer only) (PIF or AXI)
   *  - SNOOP
   *
   * See the documentation for the corresponding constructor.
   */
  void initialize(type_t                type,
                  xtsc_address          address8,
                  u32                   size8,
                  u64                   tag             = 0,
                  u32                   num_transfers   = 1,
                  xtsc_byte_enables     byte_enables    = 0xFFFF,
                  bool                  last_transfer   = true,
                  u32                   route_id        = 0,
                  u8                    id              = 0,
                  u8                    priority        = 2,
                  xtsc_address          pc              = 0xFFFFFFFF,
                  burst_t               burst           = NON_AXI,
                  bool                  apb             = false
                 );


  /**
   * Initializer for second through last request of a BLOCK_WRITE.
   *
   * See the documentation for the corresponding constructor.
   */
  void initialize(u64                   tag,
                  xtsc_address          address8,
                  u32                   size8,
                  u32                   num_transfers,
                  bool                  last_transfer,
                  u32                   route_id        = 0,
                  u8                    id              = 0,
                  u8                    priority        = 2,
                  xtsc_address          pc              = 0xFFFFFFFF
                 );


  /**
   * Initializer for the second (last) request transfer of a RCW sequence.
   *
   * See the documentation for the corresponding constructor.
   */
  void initialize(u64                   tag,
                  xtsc_address          address8,
                  u32                   route_id        = 0,
                  u8                    id              = 0,
                  u8                    priority        = 2,
                  xtsc_address          pc              = 0xFFFFFFFF
                 );


  /**
   * Initializer for second through last request of a BURST_WRITE (PIF or AXI).
   *
   * See the documentation for the corresponding constructor.
   */
  void initialize(xtsc_address          hw_address8,
                  u64                   tag,
                  xtsc_address          address8,
                  u32                   size8,
                  u32                   num_transfers,
                  u32                   transfer_num,
                  xtsc_byte_enables     byte_enables    = 0xFFFF,
                  u32                   route_id        = 0,
                  u8                    id              = 0,
                  u8                    priority        = 2,
                  xtsc_address          pc              = 0xFFFFFFFF,
                  burst_t               burst           = NON_AXI
                );


  /**
   * Set the byte address.
   *
   * @param     address8        Byte address of request.  Should be size-aligned (that
   *                            is, address8 modulo m_size8 should be 0).  For
   *                            BLOCK_WRITE requests, address8 starts out aligned to
   *                            m_size8*num_transfers and increases by m_size8 on each
   *                            subsequent request transfer in the sequence.  For
   *                            BURST_WRITE requests, address8 should start out m_size8
   *                            aligned and should increase by m_size8 on each
   *                            subsequent request transfer in the sequence.  For
   *                            BLOCK_READ, BURST_READ, and SNOOP requests, address8
   *                            should be aligned to m_size8; it does not have to be
   *                            aligned to m_size8*num_transfers (for BLOCK_READ, this
   *                            is to allow critical word first access to memory).
   *                            address8 is the same for both request transfers of an
   *                            RCW sequence.
   *
   * Note:  For PIF BLOCK_WRITE, BURST_WRITE, BLOCK_READ, and BURST_WRITE, m_size8
   *        should match the bus width.
   *
   * Note:  For a PIF interface, the address in a BLOCK_WRITE xtsc_request differs
   *        from the address in the PIF hardware specification.  In XTSC, the
   *        address changes with each request of a BLOCK_WRITE sequence to reflect
   *        the target address for that transfer.  In the hardware specification,
   *        the address for each transfer is constant and reflects the starting
   *        address of the block.  
   *
   * Note:  For a PIF interface, the address in a BURST_WRITE xtsc_request differs
   *        from the address in the PIF hardware specification.  In XTSC, the
   *        address starts out aligned with the bus width and increases by the bus width
   *        for each request of a BURST_WRITE sequence.  In the hardware specification,
   *        the address for all transfers in the sequence is constant and equal to the
   *        lowest address enabled by a byte enable in the first transfer.
   *
   * Note:  The address corresponding to the hardware specification is available by
   *        calling the get_hardware_address() method.
   *
   * @See get_byte_address().
   * @See get_hardware_address().
   */
  void set_byte_address(xtsc_address address8) { m_address8 = address8; }


  /**
   * Get the byte address.
   *
   * @see set_byte_address()
   */
  xtsc_address get_byte_address() const { return m_address8; }


  /**
   * Get the byte address corresponding to the hardware address signals.
   *
   * For all request types except BLOCK_WRITE and BURST_WRITE, this method returns the
   * same address as the get_byte_address() method.  For all BLOCK_WRITE requests in a
   * sequence, this method returns the address of the start of the block unless the
   * adjust_block_write() method has been called for the request.  For the first
   * PIF BURST_WRITE request in a sequence, this method returns the hardware address
   * inferred by the starting address and the byte enables; that is, the lowest byte
   * address enabled by a byte enable.  For the first AXI (non-PIF) BURST_WRITE request
   * this method reurns the address set in the contructor unless overridden by a call to
   * adjust_burst_write().  For all other BURST_WRITE requests, this method returns the
   * value set by the constructor or the initialize() method (which for PIF is supposed
   * to be the lowest byte address enabled in the first request of the sequence).
   *
   * Note: For BLOCK_WRITE requests in normal operation, this method computes the return
   *       value based on byte address (from get_byte_address) of the xtsc_request.  If
   *       the adjust_block_write() method has been called for this request, then the
   *       value provide by that call is returned instead.
   *
   * @see adjust_block_write()
   * @see get_byte_address()
   */
  xtsc_address get_hardware_address() const;


  /**
   * Override the computed hardware address and transfer number for a BLOCK_WRITE
   * request.
   *
   * In normal operations for BLOCK_WRITE requests, xtsc_request computes the hardware
   * address returned by the get_hardware_address() method and the transfer number
   * returned by the get_transfer_number() method.  After constructing or initializing
   * a BLOCK_WRITE xtsc_request, this method can be called to override the values
   * normally returned by the two get methods.  This might be done, for example, to test
   * how a model handles a protocol violation.
   *
   * @see get_hardware_address()
   * @see get_transfer_number()
   */
  void adjust_block_write(xtsc_address hw_address8, u32 transfer_num);


  /**
   * Set the hardware address for the first transfer of an AXI BURST_WRITE request.
   *
   * The constructor for the first transfer of an AXI BURST_WRITE request sets the
   * hardware address equal to the Xtensa TLM address.  This method can be called to
   * change the hardware address to a different value.
   *
   * @see get_hardware_address()
   */
  void adjust_burst_write(xtsc_address hw_address8);


  /**
   * Set the size in bytes of each transfer.
   *
   * @param     size8           Size in bytes of each transfer.  For PIF BLOCK_READ,
   *                            BLOCK_WRITE, BURST_READ, BURST_WRITE, and SNOOP, this is
   *                            always equal to the bus width.  For READ, WRITE, and
   *                            RCW, size8 must be a power of 2 less than or equal to
   *                            bus width.  For AXI, this corresponds to AxSIZE.
   */
  void set_byte_size(u32 size8) { m_size8 = size8; }


  /**
   * Get the size in bytes of each transfer.
   *
   * @see set_byte_size()
   */
  u32 get_byte_size() const { return m_size8; }


  /**
   * Set the PIF request attributes of each transfer (not used for AXI).
   *
   * @param     pif_attribute   PIF request attributes.
   */
  void set_pif_attribute(u32 pif_attribute) { m_pif_attribute = (pif_attribute == 0xFFFFFFFF ? 0xFFFFFFFF : (pif_attribute & 0xFFF)); }


  /**
   * Get the PIF request attributes of each transfer (not used for AXI).
   *
   * @see set_pif_attribute()
   */
  u32 get_pif_attribute() const { return m_pif_attribute; }


  /**
   * Set the PIF request domain bits of each transfer.
   *
   * @param     domain   PIF request domain bits.
   */
  void set_pif_req_domain(u8 domain) { m_domain = (domain == 0xFF ? 0xFF : (domain & 0x03)); }


  /**
   * Get the PIF request domain bits of each transfer.
   *
   * @see set_pif_req_domain()
   */
  u8 get_pif_req_domain() const { return m_domain; }


  /**
   * Set the AxCACHE bits of each transfer.
   *
   * @param     cache   AxCACHE bits.
   *
   * @see get_cache()
   */
  void set_cache(u8 cache) { m_cache = (cache & 0x0F); }


  /**
   * Get the AxCACHE bits of each transfer.
   *
   * @see set_cache()
   */
  u8 get_cache() const { return m_cache; }


  /**
   * Set the AxDOMAIN bits of each transfer.
   *
   * @param     domain   AxDOMAIN bits.
   *
   * @see get_domain()
   */
  void set_domain(u8 domain) { m_domain = (domain & 0x03); }


  /**
   * Get the AxDOMAIN bits of each transfer.
   *
   * @see set_domain()
   */
  u8 get_domain() const { return m_domain; }


  /**
   * Set the AxPROT bits of each transfer.
   *
   * @param     prot   AxPROT bits.
   *
   * @see get_prot()
   */
  void set_prot(u8 prot) { m_prot = (prot & 0x07); }


  /**
   * Get the AxPROT bits of each transfer.
   *
   * @see set_prot()
   */
  u8 get_prot() const { return m_prot; }


  /**
   * Set the AxSNOOP bits of each transfer.
   *
   * @param     snoop   AxSNOOP bits.
   *
   * @see get_snoop()
   */
  void set_snoop(u8 snoop) { m_snoop = (snoop & 0x0F); }


  /**
   * Get the AxSNOOP bits of each transfer.
   *
   * @see set_snoop()
   */
  u8 get_snoop() const { return m_snoop; }


  /**
   * Set the DRAM read/write attibutes of this request.
   *
   * @param     dram_attribute          The DRAM read/write attributes.
   *
   * @see get_dram_attribute()
   */
  void set_dram_attribute(const sc_dt::sc_unsigned& dram_attribute);


  /**
   * Get the DRAM read/write attributes of this request.
   *
   * @param     dram_attribute          The DRAM read/write attributes.
   *
   * @throws xtsc_exception if this request has no DRAM attributes.
   *
   * @see has_dram_attribute()
   * @see set_dram_attribute()
   */
  void get_dram_attribute(sc_dt::sc_unsigned& dram_attribute) const;


  /**
   * Return whether or not this request has DRAM read/write attributes.
   *
   * @see get_dram_attribute()
   * @see set_dram_attribute()
   */
  bool has_dram_attribute() const { return (m_dram_attribute != NULL); }


  /**
   * Set the route ID.
   *
   * @param     route_id        Arbiters add bits to this field as needed to be able 
   *                            to route the return response.  Terminal devices must
   *                            echo this field back verbatim in the response.  An
   *                            arbiter should clear the corresponding bit field in the
   *                            response before forwarding the response back upstream.
   *                            The result should be that the route ID in the response
   *                            that the arbiter sends upstream should match the route
   *                            ID in the original request received by the arbiter.
   *
   * @see xtsc_response::set_route_id()
   * @see get_route_id()
   * @see get_transaction_id()
   */
  void set_route_id(u32 route_id) { m_route_id = route_id; }


  /**
   * Get the route ID.
   *
   * @see set_route_id()
   * @see get_transaction_id()
   */
  u32 get_route_id() const { return m_route_id; }


  /**
   * Set the request type. 
   *
   * @see type_t
   */
  void set_type(type_t type);


  /**
   * Get the request type.
   *
   * @see type_t
   */
  type_t get_type() const { return m_type; }


  /**
   * Get a c-string corresponding to this request's type.
   */
  const char *get_type_name() const;


  /**
   * Get a c-string corresponding to this request's extended type (including AXI burst
   * info).
   *
   * If m_burst is NON_AXI and m_apb is false, then this method returns the same thing
   * as get_type_name().
   *
   * When m_burst is other than NON_AXI, it returns one of the following:
   *  \verbatim
     "RD_FIXD"
     "RD_INCR"
     "RD_WRAP"
     "CaMx_CS"      (Cache Maintenance: CleanShared)
     "CaMx_CI"      (Cache Maintenance: CleanInvalid)
     "CaMx_CU"      (Cache Maintenance: CleanUnique)
     "CaMx_MI"      (Cache Maintenance: MakeInvalid)
     "WR_FIXD"
     "WR_INCR"
     "WR_WRAP"
     "MUEvict"
    \endverbatim
   *
   * When m_apb is true, it returns "APB_RD" or "APB_WR".
   */
  const char *get_type_name_ext() const;


  /**
   * Get a c-string corresponding to the specified request type.
   *
   * @param     type    The desired type.
   */
  static const char *get_type_name(type_t type);


  /**
   * Get the xtsc_request::type_t from a string name.
   *
   * @param     name    The name of the desired type.
   */
  static type_t get_type(const std::string& name);


  /**
   * Set the request burst type. Throw an exception if m_burst is changing from
   * a non-AXI to any AXI burst type, or vice-versa.
   *
   * @param     burst   Request burst type.
   *
   * @see get_burst_type()
   */
  void set_burst_type(burst_t burst);


  /**
   * Get the AXI burst type.  Non-AXI requests return NON_AXI.
   *
   * @see burst_t
   */
  burst_t get_burst_type() const { return m_burst; }


  /**
   * Get the AXI burst type given the AxBURST bits.
   *
   * @see burst_t
   */
  static burst_t get_burst_type_from_bits(u32 bits) { return (bits==0) ? FIXED : (bits==1) ? INCR : (bits==2) ? WRAP : NON_AXI; }


  /// Get AxBURST bit encoding of m_burst 
  xtsc::u32 get_burst_bits() { return ((u32)m_burst - 1); }


  /**
   * Return true if this is a READ|BLOCK_READ|BURST_READ|RCW|SNOOP transaction,
   * otherwise return false.
   */
  bool is_read() const { return (m_type <= SNOOP); }


  /**
   * Return true if this is an APB transaction, otherwise return false.
   */
  bool is_apb_protocol() const { return m_apb; }


  /**
   * Return true if this is an AXI transaction, otherwise return false.
   */
  bool is_axi_protocol() const { return (m_burst != NON_AXI); }


  /**
   * Return true if this is an AXI cache maintenance operation, otherwise return false.
   */
  bool is_cache_maintenance() const { return (is_axi_protocol() && ((m_snoop == CaMx_CS) || (m_snoop == CaMx_CI) || (m_snoop == CaMx_CU) || (m_snoop == CaMx_MI))); }


  /**
   * Return true if this is an AXI Evict transaction, otherwise return false.
   */
  bool is_evict() const { return (is_axi_protocol() && (m_snoop == MUEvict)); }


  /**
   * Get a c-string corresponding to this request's burst type.
   */
  const char *get_burst_type_name() const;


  /**
   * Get a c-string corresponding to the specified burst type.
   *
   * @param     burst   The desired burst type.
   */
  static const char *get_burst_type_name(burst_t burst);


  /**
   * Set the number of transfers.  
   *
   * @param     num_transfers   For READ and WRITE, this is 1.
   *                            For BLOCK_READ, this is the number of response 
   *                            transfers expected by this request (2|4|8|16).
   *                            For BURST_READ, this is the number of response 
   *                            transfers expected by this request (2-16 if PIF).
   *                            For BLOCK_WRITE this is the number of request 
   *                            transfers in the BLOCK_WRITE sequence (2|4|8|16).  
   *                            For BURST_WRITE this is the number of request 
   *                            transfers in the BURST_WRITE sequence (2-16 if PIF).  
   *                            For RCW, this is 2.
   *                            For SNOOP, this is the number of response with data
   *                            transfers expected by this request (1|2|4|8|16).
   */
  void set_num_transfers(u32 num_transfers) { m_num_transfers = num_transfers; }


  /**
   * Get the number of transfers.
   *
   * @see set_num_transfers()
   */
  u32 get_num_transfers() const { return m_num_transfers; }


  /**
   * Get the actual number of transfers.
   *
   * This method returns 1 for Evict and cache maintenance AXI requests, for other
   * requests it returns get_num_transfers().
   *
   * @see get_num_transfers()
   */
  u32 get_actual_transfers() const { return ((is_evict() || is_cache_maintenance()) ? 1 : get_num_transfers()); }


  /**
   * Get which request transfer this is in a sequence of request transfers.
   *
   * For READ, BLOCK_READ, BURST_READ, WRITE, and SNOOP, this always returns 1.  For
   * BLOCK_WRITE and BURST_WRITE, this returns which request transfer this is in the
   * sequence of BLOCK_WRITE or BURST_WRITE request transfers (starting with 1 and going
   * up to m_num_transfers).  For RCW, this returns 1 for the first request transfer and
   * 2 for the second request transfer.
   *
   * Note: For BLOCK_WRITE requests in normal operation, this method computes the return
   *       value based on byte address (from get_byte_address) of the xtsc_request.  If
   *       the adjust_block_write() method has been called for this request, then the
   *       value provide by that call is returned instead.
   *
   * @see adjust_block_write()
   * @see get_byte_address()
   */
  u32 get_transfer_number() const;


  /**
   * Set the transfer number explictly  
   *
   * @param     value           New value for the transfer number. Used only for AXI
   *                            burst mode transfers.
   */
  void set_transfer_number(u32 value);


  /**
   * Set the byte enables.
   *
   * @param     byte_enables    Let address8 be the address set by set_byte_address() or
   *                            by an xtsc_request constructor or initialize() method:
   *                            For READ, WRITE, RCW, BLOCK_WRITE and BURST_WRITE, bit 0
   *                            of byte_enables applies to address8, bit 1 of 
   *                            byte_enables applies to address8+1, ..., bit (size8-1)
   *                            of byte_enables applies to address8+size8-1. Byte_enable 
   *                            for these request types can have any arbitrary pattern. 
   *                            For BLOCK_READ, BURST_READ, and SNOOP byte_enables is
   *                            not used. Unaligned transfer in BURST_WRITE can be achieved 
   *                            with the address that is aligned to the starting byte,
   *                            and the byte enables that are consistent with the 
   *                            starting address.
   *                            Note : Arbitrary byte enables in BLOCK_WRITE and 
   *                            BURST_WRITE requests are supported from PIF4.0.
   */
  void set_byte_enables(xtsc_byte_enables byte_enables) { m_byte_enables = byte_enables; }


  /**
   * Get the byte enables.
   *
   * @see set_byte_enables()
   */
  xtsc_byte_enables get_byte_enables() const { return m_byte_enables; }


  /**
   * Set the PIF request ID.  This method is also used to set up to 8 of the low bits of
   * the AXI transaction ID (AxID|xID).
   *
   * @param     id              The PIF request ID or up to 8 of the low bits of the AXI
   *                            transaction ID.  Master devices are free to use this
   *                            field to support multiple outstanding requests. Terminal
   *                            devices must echo the transaction ID back verbatim in
   *                            the response.
   *
   * @see get_id()
   * @see get_transaction_id()
   */
  void set_id(u8 id) { m_id = id; }


  /**
   * Get the PIF request ID.  For AXI, this corresponds to the low 8 bits of AxID|xID
   * [but typically you should use get_transaction_id() for AXI requests].
   *
   * @see set_id()
   * @see get_transaction_id()
   */
  u8 get_id() const { return m_id; }


  /**
   * Get the Transaction ID.  Transaction ID is formed by left-shifting route ID by 8
   * and then putting the request ID in the low-order bit positions.
   *
   * @see set_id()
   * @see set_route_id()
   */
  u32 get_transaction_id() const { return ((m_route_id << 8) | m_id); }


  /**
   * Set the transaction priority.   For AXI, this corresponds to AxQOS.
   *
   * PIF Hardware only supports priority values of 0-3 (i.e. 2 bits).
   *  \verbatim
     Value  Meaning
     -----  -------
      0     Low
      1     Medium low
      2     Medium high
      3     High
      \endverbatim
   * The Xtensa LX processor issues all PIF requests at medium-high (2) priority and
   * ignores the priority bits of PIF responses.  Inbound PIF requests to instruction
   * RAMs always have priority over processor generated instruction fetches regardless
   * of the value set using the set_priority() method.  Inbound PIF requests to data
   * RAMs and XLMI will take priority over processor generated loads and stores if
   * priority is set to 3 (High).  A priority of 0, 1, or 2 will result in the inbound
   * PIF request getting access to the XLMI or data RAM when it is free.  If an inbound
   * PIF request with a priority less then 3 has been blocked for several cycles by
   * processor generated loads or stores, then bubbles will be inserted in the Xtensa
   * processor pipeline to allow the request to make forward progress.
   */
  void set_priority(u8 priority) { m_priority = priority; }


  /**
   * Get the transaction priority.
   * @see set_priority()
   */
  u8 get_priority() const { return m_priority; }


  /**
   * Set whether or not this is the last transfer of the request.
   *
   * @param     last_transfer   True if this is the last transfer of the request.  For
   *                            READ, BLOCK_READ, BURST_READ, WRITE, and SNOOP, this should
   *                            always be true.  For all but the last transfer of a
   *                            BLOCK_WRITE, BURST_WRITE, or RCW sequence, this should
   *                            be false and for the last transfer it should be true.
   */
  void set_last_transfer(bool last_transfer) { m_last_transfer = last_transfer; }


  /**
   * Get whether or not this is the last transfer of the request.
   *
   * @see set_last_transfer()
   */
  bool get_last_transfer() const { return m_last_transfer; }


  /**
   * Set whether or not this is request is for an instruction fetch.
   *
   * @param     instruction_fetch       True indicates this request is for an
   *                                    instruction fetch.
   *
   * Note: Typically instruction_fetch should be false if m_type is not one of
   *       READ|BLOCK_READ|BURST_READ.
   */
  void set_instruction_fetch(bool instruction_fetch) { m_prot = ((m_prot & 0x3) | (instruction_fetch ? 0x4 : 0x0)); }


  /**
   * Get whether or not this is request is for an instruction fetch.
   *
   * @see set_instruction_fetch()
   */
  bool get_instruction_fetch() const { return ((m_prot & 0x4) != 0); }


  /**
   * Set whether or not this is request is from the Xtensa top XFER control block.
   *
   * @param     xfer_en         True indicates this request is a local memory request
   *                            originating from the Xtensa top XFER control block and
   *                            therefore must not be RSP_NACC'd.
   *
   * Note: This interface is only applicable to TX Xtensa cores which have the
   *       bootloader interface and at least one local memory with a busy signal.
   */
  void set_xfer_en(bool xfer_en) { m_xfer_en = xfer_en; }


  /**
   * Get whether or not this is request is from the Xtensa top XFER control block.
   *
   * Note: This interface is only applicable to TX Xtensa cores which have the
   *       bootloader interface and at least one local memory with a busy signal.
   *
   * @see set_xfer_en()
   */
  bool get_xfer_en() const { return m_xfer_en; }


  /**
   * Set the processor program counter (PC) associated with this request.
   *
   * @param     pc      The PC associated with this request.  If no meaningful
   *                    PC can be associated with the request use 0xFFFFFFFF.
   *                    This signal is not in the hardware, but is provided for
   *                    debugging and logging purposes.
   */
  void set_pc(xtsc_address pc) { m_pc = pc; }


  /**
   * Get the processor program counter (PC) associated with this request.
   *
   * @see set_pc()
   */
  xtsc_address get_pc() const { return m_pc; }


  /**
   * Set the request's transfer buffer (data payload).
   *
   * This method should only be used for RCW, WRITE, BLOCK_WRITE, and BURST_WRITE
   * request transaction types.
   *
   * For RCW transactions, the first RCW request transfer contains that data that the
   * target should use to compare with the current memory contents and the second RCW
   * request transfer contains the data that should replace the current contents if the
   * first transfers data matched the current contents.
   *
   * Data is arranged in the buffer as follows:
   * Let address8 be the address set by set_byte_address() or the 
   * xtsc_request constructor or initialize() method.
   * Let size8 be the transfer size set by set_byte_size() or the
   * xtsc_request constructor. 
   *  \verbatim
     The byte corresponding to address8+0       is in buffer[0].
     The byte corresponding to address8+1       is in buffer[1].
        . . .
     The byte corresponding to address8+size8-1 is in buffer[size8-1].
      \endverbatim
   * This format applies regardless of host and target endianess.
   * Note: The above mapping applies regardless of byte enables; however, byte enables
   *       may dictate that certain bytes in the buffer are meaningless and not to be
   *       used.  See set_byte_enables().
   */
  void set_buffer(u32 size8, const u8 *buffer);


  /**
   * Get a pointer to the request's transfer data suitable only for reading 
   * the data.
   *
   * @see set_buffer()
   */
  const u8 *get_buffer() const { return m_buffer; }


  /**
   * Get a pointer to the request's transfer data suitable either for reading 
   * or writing the data.
   *
   * The buffer size is 64 bytes to accommodate the widest possible Xtensa 
   * memory interface; however, you should only use the first N bytes where
   * N is the size of the actual memory interface in use.  
   *
   * Warning:  Writing past the 64th byte results in undefined bad behavior.
   *
   * @see set_buffer()
   */
  u8 *get_buffer() { return m_buffer; }


  /**
   * Set optional user data associated with this request.
   *
   * Note: User data is neither readable nor writable by Xtensa Ld/St/iFetch operations.
   *
   * Note: The initial value of m_user_data is 0 (NULL).
   *
   * @param     user_data       Optional user data.
   */
  void set_user_data(void *user_data) { m_user_data = user_data; }


  /**
   * Get the optional user data associated with this request.
   *
   * @see set_user_data()
   */
  void *get_user_data() const { return m_user_data; }


  /**
   * Return a string suitable for logging containing bytes from the optional user data
   * associated with this request.
   *
   * @param     num_bytes       If 0, an empty string is returned.  If non-zero, the
   *                            returned string will begin with prefix and end with
   *                            suffix.  If positive and m_user_data is not 0 (NULL),
   *                            the bytes are taken from the memory pointed to by the
   *                            user data pointer (user discretion is advised).  If
   *                            positive and m_user_data is 0, then return string
   *                            consists only of the prefix and suffix.  If negative,
   *                            the least significant -num_bytes bytes are obtained from
   *                            the user data pointer itself, as shown here:  
   * \verbatim
              0xFFFFFFFF or -1:  ((u64)p_user_data & 0x00000000000000FF)
              0xFFFFFFFE or -2:  ((u64)p_user_data & 0x000000000000FFFF)
              0xFFFFFFFD or -3:  ((u64)p_user_data & 0x0000000000FFFFFF)
              0xFFFFFFFC or -4:  ((u64)p_user_data & 0x00000000FFFFFFFF)
              0xFFFFFFFB or -5:  ((u64)p_user_data & 0x000000FFFFFFFFFF)
              0xFFFFFFFA or -6:  ((u64)p_user_data & 0x0000FFFFFFFFFFFF)
              0xFFFFFFF9 or -7:  ((u64)p_user_data & 0x00FFFFFFFFFFFFFF)
              0xFFFFFFF8 or -8:  ((u64)p_user_data & 0xFFFFFFFFFFFFFFFF)
     \endverbatim
   *                            For example, if num_bytes is -3 (0xFFFFFFFD) and
   *                            m_user_data is 0x12345678 (and prefix and suffix are
   *                            left at their default values), then this method will
   *                            return "{78 56 34}".
   * 
   * @param     prefix          Prefix to the user data bytes.
   *
   * @param     suffix          Suffix to the user data bytes.
   *
   * @see set_user_data()
   * @see get_user_data()
   */
  std::string get_user_data_for_logging(xtsc::u32 num_bytes, const std::string &prefix = " {", const std::string &suffix = "}") const;


  /**
   * Set whether or not this request if for exclusive access.  For AXI, this corresponds
   * to AxLOCK.
   *
   * @param     exclusive       True if this request is for exclusive access.
   *
   */
  void set_exclusive(bool exclusive) { m_exclusive = exclusive; }


  /**
   * Get whether or not this request if for exclusive access.
   *
   * @see set_exclusive()
   */
  bool get_exclusive() const { return m_exclusive; }


  /**
   * Get this request's tag.  This is an artificial number (not in hardware) 
   * useful for correlating requests and responses in, for example, a log file.
   * For READ and WRITE, the tag is the same for a request and its corresponding
   * response.
   * For BLOCK_WRITE and BURST_WRITE, the tag is the same for all request transfers and
   * the single response transfer of the BLOCK_WRITE or BURST_WRITE sequence.
   * For BLOCK_READ, BURST_READ, and SNOOP, the tag is the same for the single request
   * transfer and all the response transfers in the block.
   * For RCW, the tag is the same for both request transfers and the single response
   * transfer.
   * A request that gets RSP_NACC maintains the same tag when the request is repeated.
   */
  u64 get_tag() const { return m_tag; }


  /**
   * This method dumps this request's info to the specified ostream object, optionally 
   * including data (if applicable).
   *
   * Note: This dump() method in particular and the XTSC logging facility in general are
   *       intended primarily for human debugging and analysis, and there is no
   *       guarantee that the format of this dump() method or the contents of logging
   *       messages will never change.  Although these changes are relatively
   *       infrequent, users who choose to develop scripts to parse log file contents
   *       should understand those scripts may require modification in order to run on
   *       future releases.
   *
   * The format of the output is:
   * \verbatim
       AXI (single line of output shown on two lines due to printing limitations):
        tag=<Tag> pc=<PC> <Type>*[<Address>/<TID>/<Cnt>/<Tot>/<Size>/<ByteEnables>/<QOS>/<CDPS>]
        <E> <Data> <UD>

       APB:
        tag=<Tag> pc=<PC> <Type> *[<Address>/<Size>/<ByteEnables>/<Prot>] <Data> <UD>

       non-AXI/APB (single line of output shown on two lines due to printing limitations):
        tag=<Tag> pc=<PC> <Type>* [<Address>/<Num>/<ByteEnables>/0/<ID>/p<Pri>/<Attr>/<Dom>]
        <F|E> <Data> <UD> <X>

       Where:
        <Tag>           is m_tag in decimal.
        <PC>            is m_pc in hex.
        <Type>          non-AXI/APB: READ|BLOCK_READ|BURST_READ|RCW|WRITE|BLOCK_WRITE|
                                     BURST_WRITE|SNOOP
                        AXI: RD_FIXD|RD_WRAP|RD_INCR|CaMx_CS|CaMx_CI|CaMx_CU|CaMx_MI|
                             WR_FIXD|WR_WRAP|WR_INCR|MUEvict.
                        APB: APB_RD|APB_WR
        *               indicates m_last_transfer is true.
        <Address>       is m_address8 in hex.
        <TID>           is from get_transaction_id() in hex.
        <Cnt>           is get_transfer_number().
        <Tot>           is m_num_transfers.
        <Num>           is m_size8 for READ|WRITE, or m_num_transfers for BLOCK_READ|
                        BURST_READ|SNOOP, or get_transfer_number() for RCW|BLOCK_WRITE|
                        BURST_WRITE.
        <Size>          is m_size8.
        <ByteEnables>   is m_byte_enables in hex.
        <Prot>          is m_prot in decimal.
        <ID>            is m_id in decimal.
        <Pri>           is m_priority in decimal.
        <QOS>           is m_priority in decimal.
        <Attr>          is m_pif_attribute or m_dram_attribute in hex.  <Attr> is not
                        present if neither m_pif_attribute nor m_dram_attribute has been
                        set.
        <Dom>           is m_domain in dec.  <Dom> is not present if m_domain has not
                        been set.
        <CDPS>          Are m_cache, m_domain, m_prot, and m_snoop shown as a 4 nibble
                        hex value with one nibble for each value and without the usual
                        "0x" prefix.
        <F|E>           is the letter "F" if get_instruction_fetch() returns true, else
                        is the letter "E" if m_exclusive is true, else is null (not
                        present).
        <E>             is the letter "E" if m_exclusive is true, else is null (not
                        present).
        <Data>          is the equal sign followed by the contents of m_buffer in
                        hex (without leading '0x').  This field is only present if
                        dump_data is true AND m_type is RCW|WRITE|BLOCK_WRITE|
                        BURST_WRITE.
        <UD>            is m_user_data (in hex with leading '0x') if non-zero.  If
                        m_user_data is 0, then this field is not present.
        <X>             is the letter "X" if m_xfer_en is true.  Otherwise, it is null
                        (not present).
     \endverbatim
   *
   * @param     os              The ostream object to which the info should be dumped.
   *
   * @param     dump_data       If true, and the request type is RCW|WRITE|BLOCK_WRITE|
   *                            BURST_WRITE (excluding MUEvict), then the request's
   *                            buffer contents are dumped.  Otherwise, the buffer is
   *                            not dumped.
   */
  void dump(std::ostream& os = std::cout, bool dump_data = true) const;


  /**
   * Helper class to make it easy to dump xtsc_request to an ostream with or without
   * data values.
   *
   * @see show_data()
   */
  class stream_dumper {
  public:
    void dump(std::ostream & os) const {
      if (m_p_request) m_p_request->dump(os, m_show_data);
    }
    static const xtsc_request  *m_p_request;
    static bool                 m_show_data;
  };

  /**
   * This method makes it easy to dump an xtsc_request to an ostream while
   * programatically determining whether or not to include the payload data (if any).
   *
   * Usage:
   *  \verbatim
             cout << request                  << endl;   // Show data
             cout << request.show_data(true)  << endl;   // Show data
             cout << request.show_data(false) << endl;   // Don't show data
      \endverbatim
   */
  const stream_dumper& show_data(bool show) const {
    stream_dumper::m_p_request = this;
    stream_dumper::m_show_data = show;
    return m_stream_dumper;
  }


#ifndef DOXYGEN_SKIP
  // Deprecated and disabled - To be removed in a future release
  typedef enum coherence_t { NONCOHERENT = 0, SHARED = 1, EXCLUSIVE = 2, INVALIDATE = 3, LAST = INVALIDATE } coherence_t;
  void set_coherence(coherence_t dummy) { (void)dummy; }
  coherence_t get_coherence() const { return NONCOHERENT; }
  void set_snoop_virtual_address(xtsc_address dummy) { (void)dummy; }
  xtsc_address get_snoop_virtual_address() const { return 0; }
#endif // DOXYGEN_SKIP

private:


  // Initialize to all 0's (except for a few cases like m_pif_attribute which get set to their "not set" value)
  void zeroize();


  // Throw an exception if m_type is not valid for the given m_burst
  void check_type_vs_burst() const;


  // Get a new sc_unsigned (from the pool)
  sc_dt::sc_unsigned *new_sc_unsigned();


  // Delete an sc_unsigned (return it to the pool)
  void delete_sc_unsigned(sc_dt::sc_unsigned*& p_sc_unsigned);


  xtsc_address          m_address8;                     ///< Byte address 
  u8                    m_buffer[xtsc_max_bus_width8];  ///< Data for RCW, WRITE, and BLOCK_WRITE
  u32                   m_size8;                        ///< Byte size of each transfer
  u32                   m_pif_attribute;                ///< PIF request attributes (12 bits).  0xFFFFFFFF => not set.
  sc_dt::sc_unsigned   *m_dram_attribute;               ///< DRAM write attributes (160 bits).  NULL => not set.
  u32                   m_route_id;                     ///< AXI: AxID[N:8].  non-AXI: PIF Route ID.
  type_t                m_type;                         ///< Request type (READ, BLOCK_READ, etc)
  burst_t               m_burst;                        ///< Burst type (AxBURST)
  u32                   m_num_transfers;                ///< Number of transfers
  xtsc_byte_enables     m_byte_enables;                 ///< Byte enables
  u8                    m_id;                           ///< AXI: AxID[7:0].  non-AXI: PIF request ID.
  u8                    m_priority;                     ///< AXI: AxQOS.      non-AXI: PIF transaction priority.
  u8                    m_cache;                        ///< AXI: AxCACHE.
  u8                    m_domain;                       ///< AXI: AxDOMAIN.   non-AXI: PIF request domain (0xFF => not set).  2 bits.
  u8                    m_prot;                         ///< AXI: AxPROT [0]=Priveleged access, [1]=Non-secure access, [2]=Instruction access
  u8                    m_snoop;                        ///< AXI: AxSNOOP.
  bool                  m_apb;                          ///< True if APB protocol
  bool                  m_last_transfer;                ///< True if last transfer of request
  bool                  m_xfer_en;                      ///< True if request is from Xtensa top XFER control block
  xtsc_address          m_pc;                           ///< Program counter associated with request (artificial)
  xtsc_address          m_hw_address8;                  ///< Address that would appear in hardware.  BURST_WRITE only.
  u32                   m_transfer_num;                 ///< Number of this transfer.  BURST_WRITE only.  (artificial)
  void                 *m_user_data;                    ///< Arbitrary data supplied by user 
  bool                  m_adjust_block_write;           ///< True if adjust_block_write() has been called
  bool                  m_exclusive;                    ///< True if request is for exclusive access

  u64                   m_tag;                          ///< Unique tag per request-response set (artificial)
  static stream_dumper  m_stream_dumper;                ///< To assist with printing (dumping)

  static std::vector<sc_dt::sc_unsigned*>
                        m_sc_unsigned_pool;             ///< Pool of sc_unsigned (160 bits)
};



/**
 * Dump an xtsc_request object.
 *
 * This operator dumps an xtsc_request object using the
 * xtsc_request::dump() method.
 *
 */
XTSC_API std::ostream& operator<<(std::ostream& os, const xtsc_request& request);


/**
 * Dump an xtsc_request object.
 *
 * This operator dumps an xtsc_request object using the
 * xtsc_request::stream_dumper::dump() method.
 *
 */
XTSC_API std::ostream& operator<<(std::ostream& os, const xtsc_request::stream_dumper& dumper);






} // namespace xtsc



#endif  // _xtsc_request_h_
