#ifndef _xtsc_response_h_
#define _xtsc_response_h_

// Copyright (c) 2005-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */


#include <xtsc/xtsc.h>
#include <xtsc/xtsc_request.h>
#include <cstring>


namespace xtsc {


/**
 * Class representing a PIF, AXI, APB, XLMI, LX local memory, or inbound PIF or inbound
 * AXI (DataSlave or InstSlave) response transfer.
 *
 * The general 2-step procedure to create a response is:
 *  -# Construct the xtsc_response object using an xtsc_request object.
 *  -# Use the set_buffer() or get_buffer() methods to fill in the data payload.  This
 *     step is only needed for responses to READ, BLOCK_READ, BURST_READ, RCW, and SNOOP
 *     request types.
 *
 * For protocol and timing information specific to xtsc_core, see
 * xtsc_core::Information_on_memory_interface_protocols.
 *
 * @see xtsc_respond_if
 * @see xtsc_request_if
 * @see xtsc_request
 * @see xtsc_core::Information_on_memory_interface_protocols.
 * @see xtsc_component::xtsc_arbiter
 * @see xtsc_component::xtsc_dma_engine
 * @see xtsc_component::xtsc_master
 * @see xtsc_component::xtsc_mmio
 * @see xtsc_component::xtsc_memory
 * @see xtsc_component::xtsc_router
 * @see xtsc_component::xtsc_slave
 */
class XTSC_API xtsc_response {
public:

  /**
   *  Enumeration used to identify bus errors and busy information.
   *
   *  Note: For BLOCK_READ and BURST_READ requests, PIF expects RSP_DATA_ERROR for each
   *        response beat in which there is a data error.
   *
   *  Note: For BURST_READ requests, AXI always expects the requested number of
   *        responses (whether AXI_OK, DECERR, or SLVERR).
   */
  typedef enum status_t {
    RSP_OK                  = 0,                        ///< 0 => Okay
    RSP_ADDRESS_ERROR       = 1,                        ///< 1 => Address error
    RSP_DATA_ERROR          = 2,                        ///< 2 => Data error
    RSP_ADDRESS_DATA_ERROR  = 3,                        ///< 3 => Address and data error
    RSP_NACC                = 4,                        ///< 4 => Transaction not accepted (busy)
    AXI_OK                  = RSP_OK,                   ///< AXI: OKAY
    DECERR                  = RSP_ADDRESS_ERROR,        ///< AXI: DECERR
    SLVERR                  = RSP_DATA_ERROR,           ///< AXI: SLVERR,  APB: PSLVERR high
    NOTRDY                  = RSP_NACC,                 ///< AXI: AxREADY not asserted on request channel
    APB_OK                  = RSP_OK,                   ///< APB: PSLVERR low
  } status_t;



  /**
   * Constructor.  A response is always constructed starting with a request.
   *
   * @param     request         The request that this response is in response to.
   * @param     status          The response status.  See status_t.
   * @param     last_transfer   True if this is the last transfer of the response
   *                            sequence, otherwise false.  For READ, WRITE, 
   *                            BLOCK_WRITE, BURST_WRITE, and RCW, this is always true.
   *                            For BLOCK_READ, BURST_READ, and SNOOP, this is false
   *                            except for last response transfer in the sequence of 
   *                            BLOCK_READ, BURST_READ, or SNOOP response transfers.
   */
  xtsc_response(const xtsc_request& request, status_t status = RSP_OK, bool last_transfer = true);


  /**
   * Get the starting byte address.  The starting byte address is determined from the
   * xtsc_request object used to create this response and it cannot be changed.
   *
   * Note: This address does not change for multiple BLOCK_READ, BURST_READ, or SNOOP
   *       responses.
   *
   * Note: For BLOCK_WRITE responses this address is the beginning address of the block.
   */
  xtsc_address get_byte_address() const { return m_address8; }


  /**
   * Get the number of bytes of data in this response's buffer.  This is an 
   * artificial field (i.e. not in real hardware) used for logging/debugging.
   */
  u32 get_byte_size() const { return m_size8; }


  /**
   * Set the response status.
   *
   * @see status_t
   */
  void set_status(status_t status);


  /**
   * Get the response status.
   *
   * @see status_t
   */
  status_t get_status() const { return m_status; }


  /**
   * Get a c-string corresponding to this response's status.
   *
   * If m_status is AXI_OK and m_exclusive_ok is true, then "EXOKAY" will be returned.
   */
  const char *get_status_name() const;


  /**
   * Get a c-string corresponding to the specified response status.
   *
   * @param     status          The status whose c-string is desired.
   * @param     axi             If true, use AXI names.
   * @param     exclusive_ok    If true and axi is true, use "EXOKAY" instead of "AXI_OK".
   * @param     apb             If true, use APB names ("APB_OK" and "SLVERR")
   */
  static const char *get_status_name(status_t status, bool axi = false, bool exclusive_ok = false, bool apb = false);


  /**
   * Get the AXI burst type.
   *
   * @see xtsc_request::burst_t
   */
  xtsc_request::burst_t get_burst_type() const { return m_burst; }


  /**
   * Get the AXI CRESP|RRESP|BRESP bit encoding corresponding to m_status, m_exclusive_ok,
   * m_pass_dirty, and m_is_shared.
   *
   * This method should only be called for AXI responses of other than NOTRDY|RSP_NACC.
   */
  xtsc::u8 get_resp_bits() const;


  /**
   * Return true if this is an AXI transaction, otherwise return false.
   */
  bool is_axi_protocol() const { return (m_burst != xtsc_request::NON_AXI); }


  /**
   * Return true if this is an APB transaction, otherwise return false.
   */
  bool is_apb_protocol() const { return m_apb; }


  /**
   * Return true if this is an AXI cache maintenance operation, otherwise return false.
   */
  bool is_cache_maintenance() const { return m_cache_maintenance; }


  /**
   * Set whether or not this is the last transfer of the response.
   *
   * @param     last_transfer   True if this is the last transfer of the response
   *                            sequence, otherwise false.  For READ, WRITE, 
   *                            BLOCK_WRITE, BURST_WRITE, and RCW, this is always true.
   *                            For BLOCK_READ, BURST_READ, and SNOOP, this is false
   *                            except for last response transfer in the sequence of 
   *                            BLOCK_READ, BURST_READ, or SNOOP response transfers.
   */
  void set_last_transfer(bool last_transfer) { m_last_transfer = last_transfer; }


  /**
   * Get whether or not this is the last transfer of the response.
   *
   * @see set_last_transfer()
   */
  bool get_last_transfer() const { return m_last_transfer; }


  /**
   * Return true if this is a snoop response, else return false;
   */
  bool is_snoop() const { return m_snoop; }


  /**
   * Set whether or not this is a snoop response with data.
   *
   * @param     snoop_data      True if this is a snoop response AND it has data.
   */
  void set_snoop_data(bool snoop_data) { m_snoop_data = snoop_data; }


  /**
   * Return true if this is a snoop response with data, else return false.
   *
   * @see set_snoop_data()
   */
  bool has_snoop_data() const { return m_snoop_data; }


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
   * Set the route ID.  
   *
   * @param     route_id        Arbiters add bits to the route ID of the corresponding
   *                            xtsc_request to be able to route the return response.
   *                            Terminal devices must echo this field back verbatim in
   *                            the response (this is taken care of automatically in the
   *                            constructor, so terminal devices do not need to call
   *                            this method).  On the return, an arbiter should clear
   *                            its bit field in the route ID of the response before
   *                            forwarding the response back upstream.
   *                            The result should be that the route ID in the response
   *                            that the arbiter sends upstream should match the route
   *                            ID in the original request received by the arbiter.
   *
   * @see xtsc_request::set_route_id()
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
   * Set the PIF response ID.  This method is also used to set up to 8 of the low bits of
   * the AXI transaction ID (AxID|xID).
   *
   * @param     id              The PIF response ID.  Terminal devices must echo the
   *                            transaction ID (combination of request ID and route ID)
   *                            back verbatim in the response.  This is taken care of
   *                            automatically in the constructor, so terminal devices
   *                            typically do not need to call this method.
   *
   * @see get_id()
   * @see get_transaction_id()
   */
  void set_id(u8 id) { m_id = id; }


  /**
   * Get the PIF response ID.  For AXI, this corresponds to the low 8 bits of AxID|xID
   * [but typically you should use get_transaction_id() for AXI responses].
   *
   * @see set_id()
   * @see get_transaction_id()
   */
  u8 get_id() const { return m_id; }


  /**
   * Get the Transaction ID.  Transaction ID is formed by left-shifting route ID by 8
   * and then putting the response ID in the low-order bit positions.
   *
   * @see set_id()
   * @see set_route_id()
   */
  u32 get_transaction_id() const { return ((m_route_id << 8) | m_id); }


  /**
   * Set the priority.
   *
   * Hardware only supports priority values of 0-3 (i.e. 2 bits).
   *  \verbatim
     Value  Meaning
     -----  -------
      0     Low
      1     Medium low
      2     Medium high
      3     High
      \endverbatim
   * The Xtensa LX processor issues all PIF requests at medium-high (2) priority and
   * ignores the priority bits of PIF responses.  For responses to inbound PIF requests,
   * the Xtenasa LX processor sets the response priority equal to the request priority.
   * Default = The priority in the request used to create this response.
   */
  void set_priority(u8 priority) { m_priority = priority; }


  /**
   * Get the priority.
   *
   * @see set_priority()
   */
  u8 get_priority() const { return m_priority; }


  /**
   * Set the response's transfer buffer (payload).  Used only for READ,
   * BLOCK_READ, BURST_READ, RCW, and SNOOP.
   * Data is arranged in the buffer as follows:
   * Let address8 be the address returned by xtsc_request::get_byte_address().
   * Let size8 be the transfer size returned by xtsc_response::get_byte_size().
   *  \verbatim
     The byte corresponding to address8+0       is in buffer[0].
     The byte corresponding to address8+1       is in buffer[1].
        . . .
     The byte corresponding to address8+size8-1 is in buffer[size8-1].
      \endverbatim
   * This format applies regardless of host and target endianess.
   */
  void set_buffer(const u8 *buffer) { memcpy(m_buffer, buffer, m_size8); }


  /**
   * Get a pointer to the response's transfer data suitable for reading (but not
   * writing) the data.  Used only for READ, BLOCK_READ, BURST_READ, RCW, and SNOOP.
   *
   * @see set_buffer()
   */
  const u8 *get_buffer() const { return m_buffer; }


  /**
   * Get a pointer to the response's transfer data suitable either for reading 
   * or writing the data.  Used only for READ, BLOCK_READ, BURST_READ, RCW, and SNOOP.
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
   * Set optional user data associated with this response.
   *
   * Note: User data is neither readable nor writable by Xtensa Ld/St/iFetch operations.
   *
   * Note: The default user data in the response is obtained from the user data of the
   *       request from which the response was formed.
   *
   * @param     user_data       Optional user data.
   */
  void set_user_data(void *user_data) { m_user_data = user_data; }


  /**
   * Get the optional user data associated with this response.
   *
   * @see set_user_data()
   */
  void *get_user_data() const { return m_user_data; }


  /**
   * Return a string suitable for logging containing bytes from the optional user data
   * associated with this response.
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
   * Get whether or not this response is for an exclusive access request.
   *
   * @see set_exclusive_ok()
   * @see get_exclusive_ok()
   */
  bool get_exclusive_req() const { return m_exclusive_req; }


  /**
   * Set whether or not the exclusive access request succeeded.
   *
   * @param     ok              True if the exclusive access request succeeded.
   *
   */
  void set_exclusive_ok(bool ok) { m_exclusive_ok = ok; }


  /**
   * Get whether or not the exclusive access request succeeded.
   *
   * @see set_exclusive_ok()
   */
  bool get_exclusive_ok() const { return m_exclusive_ok; }


  /**
   * Set the ACE PassDirty bit, RRESP[2]/CRESP[2].
   *
   * @param     pass_dirty      True if ACE read/snoop response PassDirty bit is set.
   *
   * @see get_pass_dirty()
   */
  void set_pass_dirty(bool pass_dirty) { m_pass_dirty = pass_dirty; }


  /**
   * Get the ACE PassDirty bit, RRESP[2]/CRESP[2].
   *
   * @see set_pass_dirty()
   */
  bool get_pass_dirty() const { return m_pass_dirty; }


  /**
   * Set the ACE IsShared bit, RRESP[3]/CRESP[3].
   *
   * @param     is_shared       True if ACE read/snoop response IsShared  bit is set.
   *
   * @see get_is_shared()
   */
  void set_is_shared(bool is_shared) { m_is_shared = is_shared; }


  /**
   * Get the ACE IsShared bit, RRESP[3]/CRESP[3].
   *
   * @see set_is_shared()
   */
  bool get_is_shared() const { return m_is_shared; }


  /**
   * Set the ACE WasUnique bit, CRESP[4].
   *
   * @param     was_unique       True if ACE snoop response WasUnique bit is set.
   *
   * @see get_was_unique()
   */
  void set_was_unique(bool was_unique) { m_was_unique = was_unique; }


  /**
   * Get the ACE WasUnique bit, CRESP[4].
   *
   * @see set_was_unique()
   */
  bool get_was_unique() const { return m_was_unique; }


  /// Get the number of response transfers.
  u32 get_num_transfers() const { return m_num_transfers; }


  /**
   * Set which transfer this is in a sequence of response transfers.
   *
   * This method should be called to set which transfer this is for responses to
   * BLOCK_READ and BURST_READ requests.  The transfer number defaults to 1, so there is
   * no need to call this method for responses to other kinds of requests (READ, WRITE,
   * BLOCK_WRITE, BURST_WRITE, and RCW).
   *
   */
  void set_transfer_number(u32 transfer_num) { m_transfer_num = transfer_num; }


  /**
   * Get transfer number.
   *
   * @see set_transfer_number()
   */
  u32 get_transfer_number() const { return m_transfer_num; }


  /**
   * Get this response's tag.  This is an artificial number (not in hardware) 
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
   * This method dumps this response's info to the specified ostream object, optionally 
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
       AXI:
       tag=<Tag> pc=<PC> <Status> *[<Address>/<TID>/<Num>/<R>]: <Data> <UD>

       APB:
       tag=<Tag> pc=<PC> <Status> *[<Address>]: <Data> <UD>

       non-AXI/APB:
       tag=<Tag> pc=<PC> <Status>*[<Address>/0/<ID>/<Num>]<E>: <Data> <UD>

       Where:
        <Tag>           is m_tag in decimal.
        <PC>            is m_pc in hex.
        <Status>        non-AXI/APB: RSP_OK|RSP_ADDRESS_ERROR|RSP_DATA_ERROR|
                                     RSP_ADDRESS_DATA_ERROR|RSP_NACC.
                        AXI: AXI_OK|EXOKAY|SLVERR|DECERR|NOTRDY
                        APB: APB_OK|SLVERR
        *               indicates m_last_transfer is true.
        <Address>       is m_address8 in hex.
        <TID>           is from get_transaction_id().
        <Num>           is m_transfer_num in decimal.
        <R>             is the value from get_resp_bits() as a single hex nibble without
                        a leading '0x', or a hyphen ('-') for NOTRDY responses.
        <ID>            is m_id in decimal.
        <E>             is the letter "E" if m_exclusive_ok is true, else is null (not
                        present).
        <Data>          is a colon followed by the contents of m_buffer in
                        hex (without a leading '0x').  This field is only present
                        if dump_data is true AND m_status is RSP_OK AND m_size8 is non-
                        zero (indicating this is a response to a READ, BLOCK_READ,
                        BURST_READ, or RCW request or that m_snoop_data is true).
        <UD>            is m_user_data (in hex with a leading '0x') if non-zero.  If
                        m_user_data is 0, then this field is not present.
     \endverbatim
   *
   * @param     os              The ostream object to which the info should be dumped.
   *
   * @param     dump_data       If true and the response has data, then the response's buffer
   *                            contents are dumped.  Otherwise, the buffer is not dumped.
   */
  void dump(std::ostream& os = std::cout, bool dump_data = true) const;


  /**
   * Helper class to make it easy to dump xtsc_response to an ostream with or without data values
   *
   * @see show_data()
   */
  class stream_dumper {
  public:
    void dump(std::ostream & os) const {
      if (m_p_response) m_p_response->dump(os, m_show_data);
    }
    static const xtsc_response *m_p_response;
    static bool                 m_show_data;
  };


  /**
   * This method makes it easy to dump an xtsc_response to an ostream
   * while programatically determining whether or not to include
   * the payload data (if any).
   *
   * Usage:
   *  \verbatim
             cout << response                  << endl;  // Show data
             cout << response.show_data(true)  << endl;  // Show data
             cout << response.show_data(false) << endl;  // Don't show data
      \endverbatim
   */
  const stream_dumper& show_data(bool show) const {
    stream_dumper::m_p_response = this;
    stream_dumper::m_show_data  = show;
    return m_stream_dumper;
  }


#ifndef DOXYGEN_SKIP
  // Deprecated and disabled - To be removed in a future release
  typedef enum coherence_t { EXCLUSIVE = 0, MODIFIED = 1, SHARED = 2, INVALID = EXCLUSIVE, NONCOHERENT = EXCLUSIVE, LAST = SHARED } coherence_t;
  void set_coherence(coherence_t dummy) { (void)dummy; }
  coherence_t get_coherence() const { return EXCLUSIVE; }
#endif // DOXYGEN_SKIP

private:

  /// Throw an exception if m_status is not valid for the protocol type
  void check_status();

  // Default construction is not allowed
  xtsc_response();

  xtsc_address          m_address8;                     ///< Starting byte address (artificial)
  u8                    m_buffer[xtsc_max_bus_width8];  ///< Data for READ, BLOCK_READ, BURST_READ, RCW, and SNOOP
  u32                   m_size8;                        ///< Byte size of each transfer (artificial)
  u32                   m_route_id;                     ///< Route ID for arbiters
  status_t              m_status;                       ///< Response status
  u8                    m_id;                           ///< PIF response ID
  u8                    m_priority;                     ///< Transaction priority
  bool                  m_apb;                          ///< True if APB protocol
  bool                  m_last_transfer;                ///< True if last transfer of response
  bool                  m_snoop;                        ///< True if this is a snoop response, otherwise false
  bool                  m_snoop_data;                   ///< True if this is a snoop response with data, otherwise false
  bool                  m_exclusive_req;                ///< True if this response is for an exclusive access request
  bool                  m_exclusive_ok;                 ///< True if the exclusive access request was successful
  bool                  m_pass_dirty;                   ///< True if ACE read/snoop response PassDirty bit (RRESP[2]/CRESP[2]) is set, otherwise false
  bool                  m_is_shared;                    ///< True if ACE read/snoop response IsShared  bit (RRESP[3]/CRESP[3]) is set, otherwise false
  bool                  m_was_unique;                   ///< True if ACE snoop response WasUnique bit (CRESP[4]) is set, otherwise false
  bool                  m_cache_maintenance;            ///< True if request was an AXI cache maintenance operation
  xtsc_request::burst_t m_burst;                        ///< Request burst type
  xtsc_address          m_pc;                           ///< Program counter associated with request (artificial)
  u32                   m_num_transfers;                ///< Number of response transfers (artificial)
  u32                   m_transfer_num;                 ///< Number of this transfer.  Should be 1 except for BLOCK_READ|BURST_READ.  (artificial)
  void                 *m_user_data;                    ///< Arbitrary data supplied by user 

  u64                   m_tag;                          ///< Unique tag per request-response set (artificial)
  static stream_dumper  m_stream_dumper;                ///< To assist with printing (dumping)
};


/**
 * Dump an xtsc_response object.
 *
 * This operator dumps an xtsc_response object using the 
 * xtsc_response::dump() method.
 *
 */
XTSC_API std::ostream& operator<<(std::ostream& os, const xtsc_response& response);


/**
 * Dump an xtsc_response object.
 *
 * This operator dumps an xtsc_response object using the
 * xtsc_response::stream_dumper::dump() method.
 *
 */
XTSC_API std::ostream& operator<<(std::ostream& os, const xtsc_response::stream_dumper& dumper);


} // namespace xtsc



#endif  // _xtsc_response_h_
