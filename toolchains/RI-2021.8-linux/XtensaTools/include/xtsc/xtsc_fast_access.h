#ifndef _XTSC_FAST_ACCESS_H_
#define _XTSC_FAST_ACCESS_H_

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

// For ctags
#undef xtsc_fast_access

namespace xtsc {


class xtsc_request_if;



/**
 * Interface for fast access (turbo mode).
 *
 * In order to use fast access with callbacks for a memory,
 * the user designer should implement this interface.
 * 
 * When the core wants access to a memory for the first time,
 * it will make a fast_access_request.  The memory should then return
 * an object that implements this interface.
 *
 * After the request has been granted, memory accesses to the specified
 * block of memory will be made as a series of nb_fast_access_read
 * and nb_fast_access_write requests.
 */
class XTSC_API xtsc_fast_access_if {
public:

  /**
   * Method to read bytes from a memory
   *
   * @param address    Local memory address
   * @param size8      Number of bytes to read.  
   *                   size8 is a power 2 <= the size passed when the
   *                   fast access was requested.
   *                   
   * @param dst        memory to store the result.
   *                   This buffer is byte-ordered.
   */
  virtual void nb_fast_access_read(xtsc_address address, u32 size8, u8 *dst) = 0;


  /**
   * Method to write bytes to a memory
   *
   * @param address    Local memory address
   * @param size8      Number of bytes to write.  
   *                   size8 is a power 2 <= the size passed when the
   *                   fast access request word size.
   *                   
   * @param src        value that should be written.
   *                   This buffer is byte ordered.
   */
  virtual void nb_fast_access_write(xtsc_address address, u32 size8, const u8 *src) = 0;


  /// virtual destructor
  virtual ~xtsc_fast_access_if() {}

};



/**
 * Value class for a block that surrounds a target request address.
 */
class XTSC_API xtsc_fast_access_block {
public:

  /**
   * Constructor for a block that surrounds a target address.
   *
   * This constructor will throw an exception if the target address is not contained in
   * the block.
   *
   * @param target_address      Target address.
   * @param block_beg_address   First address in the block.
   * @param block_end_address   Last address in the block.
   */
  xtsc_fast_access_block(xtsc_address target_address, xtsc_address block_beg_address, xtsc_address block_end_address);


  /**
   * Constructor for a block that spans the entire address space.
   *
   * @param target_address      Target address.
   */
  xtsc_fast_access_block(xtsc_address target_address);

  
  /**
   * Method to remove an address range from a block.
   *
   * This method will throw an exception if the target address is contained in the block
   * to be removed.
   *
   * @param local_target_address   Target address.  Typically this is the same address
   *                               as m_target_address; however, if address translation
   *                               is being applied by the caller, then all three
   *                               address parameters may be specified using local (i.e.
   *                               translated) addressing.
   * @param local_beg_address      First address in the block to be removed using local
   *                               addressing.
   * @param local_end_address      Last address in the block to be removed using local
   *                               addressing.
   *
   * @return true if any addresses were removed from the block.
   */
  bool remove_address_range(xtsc_address local_target_address, xtsc_address local_beg_address, xtsc_address local_end_address);


  /**
   * Method to remove any addresses outside of the specified block
   *
   * @param block        The bounding block where the block address
   *                     is the translation of this block's address.
   *
   * @return true if any addresses were removed from the block.
   */
  bool restrict_to_block(const xtsc_fast_access_block &block);


  /**
   * Method to change the addresses of a block. The block_beg_address
   * and block_end_address will be moved relative to the new target address.
   *
   * @param new_target_address         The new target address.
   */
  void translate_address(xtsc_address new_target_address);


  /**
   * Query method to get the target address of this block. The target
   * address will always be inside the block.
   *
   * @return the target address of this block.
   */
  xtsc_address get_target_address() const { return m_target_address; }


  /**
   * Query method to get the first address in the block.
   *
   * @return the first address in the block.
   */
  xtsc_address get_block_beg_address() const { return m_block_beg_address; }


  /**
   * Query method to get the last address in the block.
   *
   * @return the last address in the block.
   */
  xtsc_address get_block_end_address() const { return m_block_end_address; }


#ifndef DOXYGEN_SKIP
  // For backward compatibility
  xtsc_address get_address() const { return get_target_address(); }
  xtsc_address get_block_address() const { return get_block_beg_address(); }
  void reset_address(xtsc_address new_target_address) { translate_address(new_target_address); }
#endif // DOXYGEN_SKIP

private:

  xtsc_address  m_target_address;
  xtsc_address  m_block_beg_address;
  xtsc_address  m_block_end_address;

};


  
/**
 * Opaque class for storing callback functions and data to implement
 * fast access data transfers with function callbacks
 */
class xtsc_fast_access_callbacks;



/**
 * Opaque class for storing custom callback functions and data to implement
 * fast access data transfers with custom function callbacks
 */
class xtsc_fast_access_custom_callbacks;



/**
 * Opaque class for raw memory block information to implement fast
 * access data transfers with direct memory access.
 */
class xtsc_raw_block;



/**
 * Interface to be implemented by memory-interface masters that wish to support
 * revocation of previously-granted fast access requests.
 *
 * A memory-interface master that requests fast access (mainly xtsc_core) and that
 * wishes to allow the downstream memory-interface slave to subsequently revoke any
 * granted fast access requests should implement this interface and pass a pointer
 * to the implementation to the xtsc_fast_access_request::set_fast_access_revocation_if
 * method before calling nb_fast_access() with the xtsc_fast_access_request object.
 */
class XTSC_API xtsc_fast_access_revocation_if {
public:

  /**
   * This method revokes any previously-granted fast access requests.
   * The callee is allowed to re-request fast access by subsequently re-calling
   * the nb_fast_access() method.
   */
  virtual void revoke_fast_access() = 0;

  /// virtual destructor
  virtual ~xtsc_fast_access_revocation_if() {}

};



/**
 * Class to hold request and response information to set up fast access data
 * transfers.
 *
 * A fast access request is issued when a core first needs the data at
 * a memory address.  The request includes the original request
 * address and the endianness and word size of the core.
 *
 * Routers and arbiters will translate the request address and forward it.
 * The translated address can be queried with
 *   get_translated_request_address()
 *
 * Terminal devices (for example, memories) should invoke one of 
 *    deny_access()
 *    allow_raw_access()
 *    allow_callbacks_access()
 *    allow_custom_callbacks_access()
 *    allow_peek_poke_access()
 *    allow_interface_access()
 *
 * If the permissions are only valid for a subset of the range that the
 *    device maps, use restrict_to_block() or remove_address_range().
 * 
 * When allowing only read or write access use deny_read_access() and deny_write_access() 
 *    after the appropriate allow_* method is invoked.
 *
 * If none of these is invoked, access is denied
 *
 */
class XTSC_API xtsc_fast_access_request {
public:

  typedef enum {
    ACCESS_NONE,
    ACCESS_DENY,
    ACCESS_RAW,
    ACCESS_CALLBACKS,
    ACCESS_CUSTOM_CALLBACKS,
    ACCESS_INTERFACE,
    ACCESS_PEEKPOKE
  } access_type;

  // for callbacks
  typedef void (*xtsc_fast_access_read_callback)  (void *, u32 *dst, xtsc_address address, u32 size8);
  typedef void (*xtsc_fast_access_write_callback) (void *, xtsc_address address, u32 size8, const u32 *src);

  // for callbacks with custom data
  typedef void (*xtsc_fast_access_custom_read_callback)  (void *, u32 *dst, xtsc_address address, u32 size8, const u32 *custom_data);
  typedef void (*xtsc_fast_access_custom_write_callback) (void *, xtsc_address address, u32 size8, const u32 *src, const u32 *custom_data);

  
  /**
   * Constructor to allow a user device to initiate a fast access request.
   *
   * @param requestor            A reference to the object that initiated
   *                             this request.
   * @param request_address      The original request address
   * @param request_word_size    The maximum word size of a fast access
   *                             data transfer resulting from this request.
   * @param request_big_endian   The endianness of the requestor.
   */
  xtsc_fast_access_request(sc_core::sc_object&  request_object,
                           xtsc_address         request_address,
                           u32                  request_word_size,
                           bool                 request_big_endian);


  /// Destructor for a request object.
  ~xtsc_fast_access_request();


  /// Get the request object
  sc_core::sc_object &get_request_object() const { return *m_request_object; }


  /// Get the request address
  xtsc_address get_request_address() const { return m_request_address; }


  /// Get the request word size
  u32 get_request_word_size() const { return m_request_word_size; }


  /// Get the request endianness
  u32 get_request_big_endian() const { return m_request_big_endian; }


  /**
   * Use the result of a fast access request to read data from the
   * target.
   * @param request_if  the interface for issuing an nb_peek
   *                    request if the peek/poke method is used.
   * @param address the address (in the request address space)
   *                must be aligned to size.
   * @param size    a power of 2 no greater than the request word size
   * @param dst     a byte-ordered buffer of size bytes where the response
   *                will be written
   * @return false  if the access cannot be performed because:
   *                size is not a power of 2
   *                size is greater than the request word size
   *                address is not aligned to size.
   *                the target denies read access.
   *                the address is not in the range handled by the
   *                target.
   */
  bool fast_read(xtsc_request_if& request_if, xtsc_address address, u32 size, u8 *dst);


  /**
   * Use the result of a fast access request to read data from the
   * target.
   * @param request_if  the interface for issuing an nb_poke
   *                    request if the peek/poke method is used.
   * @param address the address (in the request address space)
   *                must be aligned to size.
   * @param size    a power of 2 no greater than the request word size
   * @param src     a byte-ordered buffer of size bytes to write into the
   *                target.
   * @return false  if the access cannot be performed because:
   *                size is not a power of 2
   *                size is greater than the request word size
   *                address is not aligned to size.
   *                the target denies write access.
   *                the address is not in the range handled by the
   *                target.
   */
  bool fast_write(xtsc_request_if &request_if, xtsc_address address, u32 size, const u8 *src);


  /**
   * Method to translate a request address.  This should be used by a
   * router before forwarding a fast access request
   */
  bool translate_request_address(xtsc_address translated_address);


  /// Method to deny fast access for the request.
  void deny_access();


  /**
   * Method to allow direct memory access for the request.
   * This method will throw an exception if the difference between the
   * original request address and the translated request address is not divisible by 4.
   * It will also throw an exception if (swizzle & 3) is not 0 or 3.
   *
   * @param block_beg_address   The first address that is allowed fast access.
   * @param raw_data            The host memory address for memory that includes the
   *                            first stored word of block_beg_address
   * @param swizzle             0 for memories stored in byte order. See documentation
   *                            for the swizzle of more efficent orders when simulating
   *                            a big endian host on a little endian target.
   */
  void allow_raw_access(xtsc_address block_beg_address, u32 *raw_data, u32 size, u32 swizzle);


  /**
   * Method to allow fast access through an xtsc_fast_access_if object.
   * Data transfers will bypass routers and arbitors with this method of fast access.
   *
   * @param access_if        The object that implements nb_fast_access_read and nb_fast_access_write
   */
  void allow_interface_access(xtsc_fast_access_if *access_if);

  
  /**
   * Method to allow fast access using the nb_peek and nb_poke methods
   * Data transfers will NOT bypass routers and arbitors with this method of fast access.
   */
  void allow_peek_poke_access();


  /**
   * Method to allow direct function callbacks for fast access.
   * This can be faster than interface access when the target and host endianness differ. However,
   * the functions expect data in a target-related order and must be implemented with care.
   * The interface access uses this method, but convert the data into a byte-ordered buffer to make
   * it easier to use.
   *
   * @param callback_arg        Argument to pass to the read and write callbacks when
   *                            they are invoked.  This should be a pointer to a
   *                            persistent object.
   * @param read_callback       The function to invoke to read data from a memory
   * @param write_callback      The function to invoke to write data to a memory.
   */
  void allow_callbacks_access(void                             *callback_arg,
                              xtsc_fast_access_read_callback    read_callback,
                              xtsc_fast_access_write_callback   write_callback);


  /**
   * Method to allow direct function callbacks for fast access with custom data.
   * @param callback_arg                Argument to pass to the read and write custom
   *                                    callbacks when they are invoked.
   * @param custom_read_callback        Function to invoke to read data from a memory.
   * @param custom_write_callback       Function to invoke to write data to a memory.
   */
  void allow_custom_callbacks_access(void                                    *callback_arg,
                                     xtsc_fast_access_custom_read_callback    custom_read_callback,
                                     xtsc_fast_access_custom_write_callback   custom_write_callback);


  /// Method to deny read access.  Invoke after allowing access.
  void deny_read_access();


  /// Method to deny write access.  Invoke after allowing access.
  void deny_write_access();


  /**
   * Method to remove a range of addresses from a fast address response.  It may be used when a
   * device only allows fast access on a subset of its address space or by routers.
   *
   * @param local_address  translated request/response address.  In the device allowing
   *                       fast access, this is translated_request_address from the
   *                       request.
   * @param start_address  first address to remove
   * @param end_address    last address to remove
   */
  bool remove_address_range(xtsc_address local_address, xtsc_address start_address, xtsc_address end_address);


  /**
   * Method to remove the range of addresses from a request that are outside of the specified block.
   *
   * @param min_block      block that specifies a range from block_address to block_end_address that should
   *                       not be removed from the response.  The address of the min_block is the
   *                       translated_request_address from the request.
   */
  bool restrict_to_block(const xtsc_fast_access_block &min_block);

  
  /**
   * Get the response access type.
   * @return response access type.
   */
  access_type get_access_type() const { return m_access_type; }

  
  /**
   * Get the response access type as a C-string.
   * @return response access type as a C-string.
   */
  const char *get_access_type_c_str() const;


  /**
   * Get the response write permissions
   * @return true if the response allows write access
   */
  bool is_writable() const { return m_is_writable; }


  /**
   * Get the response read permissions
   * @return true if the response allows read access
   */
  bool is_readable() const { return m_is_readable; }


  /**
   * Get the original response block.  For all but the raw access,
   * this block will contain all of memory.  For any response the
   * target address of the block will be the translated request
   * address.
   *
   * @return block initially specified in the allow or deny method invoked on the request.
   */
  xtsc_fast_access_block get_response_block() const;


  /**
   * Get the result block.  The target address for the block is the original request address.
   * The block start and block end are the ones resulting after all removals and restrictions
   * have been applied.
   *
   * @return the fast access result block
   */
  xtsc_fast_access_block get_result_block() const;


  /**
   * Get the result block translated to a local_address.  The local_address will usually be
   * the translated_request_address when the nb_fast_access method is invoked.
   *
   * @param local_address     the local translation of the original request address.
   *
   * @return the translated fast access result block
   */
  xtsc_fast_access_block get_local_block(xtsc_address local_address) const;


  /**
   * Get the translated request address.  This is reset with translate_request_address()
   *
   * @return the translated request address.
   */
  xtsc_address get_translated_request_address() const {
    return m_translated_request_address;
  }


  /**
   * For raw access, get the raw pointer address associated with the first word of the result block.
   * This method will throw an exception if the access type is not ACCESS_RAW.
   *
   * @return the raw pointer address of the first word of the result block.
   */
  u32 *get_raw_data() const;


  /**
   * For raw access, get the raw pointer address associated with the first word of the response block.
   * This method will throw an exception if the access type is not ACCESS_RAW.
   *
   * @return the raw pointer address of the first word of the response block.
   */
  u32 *get_orig_raw_data() const;


  /**
   * For raw access, get the swizzle that defines the memory layout.
   * This method will throw an exception if the access type is not ACCESS_RAW.
   *
   * @return the swizzle that defines the memory layout.
   */
  u32 get_swizzle() const;


  /**
   * For interface access, get the interface object.
   * This method will throw an exception if the access type is not ACCESS_INTERFACE.
   *
   * @return the interface object for INTERFACE access
   */
  xtsc_fast_access_if *get_fast_access_if() const;


  /**
   * For callback access, get the read callback function.
   * This method will throw an exception if the access type is not ACCESS_CALLBACKS.
   *
   * @return the read callback function for CALLBACK access
   */
  xtsc_fast_access_read_callback get_read_callback() const;


  /**
   * For callback access, get the write callback function.
   * This method will throw an exception if the access type is not ACCESS_CALLBACKS.
   *
   * @return the write callback function for CALLBACK access
   */
  xtsc_fast_access_write_callback get_write_callback() const;


  /**
   * For callback access or custom callback access, get the callback argument.
   * This method will throw an exception if the access type is neither ACCESS_CALLBACKS
   * nor ACCESS_CUSTOM_CALLBACKS.
   *
   * @return the callback argument for callback access or custom callback access.
   */
  void *get_callback_arg() const;


  /// Deprecated.  Use get_callback_arg()
  void *get_callback_data() const { return get_callback_arg(); }


  /**
   * For custom callback access, get the custom read callback function.
   * This method will throw an exception if the access type is not ACCESS_CUSTOM_CALLBACKS.
   *
   * @return the custom read callback function for custom callback access
   */
  xtsc_fast_access_custom_read_callback get_custom_read_callback() const;


  /**
   * For custom callback access, get the custom write callback function.
   * This method will throw an exception if the access type is not ACCESS_CUSTOM_CALLBACKS.
   *
   * @return the custom write callback function for custom callback access
   */
  xtsc_fast_access_custom_write_callback get_custom_write_callback() const;


  /// Set the interface object for revoking fast access 
  void set_fast_access_revocation_if(xtsc_fast_access_revocation_if *p_if) { m_fast_access_revocation_if = p_if; }


  /// Return the interface object for revoking fast access (or NULL if none)
  xtsc_fast_access_revocation_if *get_fast_access_revocation_if() const { return m_fast_access_revocation_if; }


private:
  bool reset_block(const xtsc_fast_access_block &block);
  void clear_response();
  xtsc_fast_access_request(const xtsc_fast_access_request &);
  xtsc_fast_access_request& operator=(const xtsc_fast_access_request &);


protected:

  /* request data */
  sc_core::sc_object                   *m_request_object;
  xtsc_address                          m_request_address;
  u32                                   m_request_word_size;
  bool                                  m_request_big_endian;

  xtsc_address                          m_translated_request_address;
  xtsc_fast_access_revocation_if       *m_fast_access_revocation_if;

  /* the rest of the members are for the response */
  access_type                           m_access_type;
  xtsc_fast_access_block                m_response_block;
  xtsc_fast_access_block                m_result_block; // response block translated to request_address with removals

  bool                                  m_is_readable;
  bool                                  m_is_writable;

  // These 3 could be a union
  xtsc_raw_block                       *m_raw_block;
  xtsc_fast_access_callbacks           *m_access_callbacks;
  xtsc_fast_access_custom_callbacks    *m_access_custom_callbacks;
  xtsc_fast_access_if                  *m_access_if;

};
 

}  // namespace xtsc


#endif /* _XTSC_FAST_ACCESS_H_ */
