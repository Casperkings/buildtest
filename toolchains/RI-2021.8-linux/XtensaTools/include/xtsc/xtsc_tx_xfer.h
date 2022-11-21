#ifndef _XTSC_TX_XFER_H_
#define _XTSC_TX_XFER_H_

// Copyright (c) 2005-2010 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

/**
 * @file 
 */


#include <xtsc/xtsc_types.h>


namespace xtsc {


/**
 * This class carries the information of a TLM transaction on the TX Xtensa XFER
 * (boot loader) interface.
 *
 * @see xtsc_tx_xfer_if
 */
class XTSC_API xtsc_tx_xfer {
  friend class xtsc_tx_loader;
public:


  /**
   * Constructor for an empty xtsc_tx_xfer object used to create a pool of pre-allocated
   * xtsc_tx_xfer objects.  Before using an xtsc_tx_xfer object that was created with this
   * constructor, either assign another xtsc_tx_xfer object to it or call the initialize()
   * method on it.
   */
  xtsc_tx_xfer();


  /**
   * Constructor for a normal xtsc_tx_xfer.
   */
  xtsc_tx_xfer(bool done, u32 address, u32 data, bool config_xfer, bool write, bool turbo);


  /**
   * Method to re-initialize a pre-existing xtsc_tx_xfer.
   */
  void initialize(bool done, u32 address, u32 data, bool config_xfer, bool write, bool turbo, u64 tag = 0ULL);


  /**
   * Get whether or not this transaction is for Done pin control
   *
   * @see set_done()
   */
  bool get_done() const { return m_done; }


  /**
   * Get the word address.
   *
   * @see set_address()
   */
  u32 get_address() const { return m_address; }


  /**
   * Get the data.
   *
   * @see set_data()
   */
  u32 get_data() const { return m_data; }


  /**
   * Get whether or not this is a configuration transaction.
   *
   * @see set_config_xfer()
   */
  bool get_config_xfer() const { return m_config_xfer; }


  /**
   * Get whether or not this is a write transaction.
   *
   * @see set_write()
   */
  bool get_write() const { return m_write; }


  /**
   * Get whether or not this read transaction is carrying read data..
   *
   * @see set_read_data()
   */
  bool get_read_data() const { return m_read_data; }


  /**
   * Get whether or not the transaction should be handled in turbo style.
   *
   * @see set_turbo()
   */
  bool get_turbo() const { return m_turbo; }


  /**
   * Get this xfer transaction's tag.  This is an artificial number (not in hardware) 
   * useful for correlating xfer transactions, for example, a log file.
   */
  u64 get_tag() const { return m_tag; }


  /**
   * Set whether or not this transaction is for Done pin control.
   *
   * @param     done    If true, then m_data carries the new value of the Done pin (0 or
   *                    1).  If false, then this is a regular transaction.
   *
   */
  void set_done(bool done) { m_done = done; }


  /// Set the word address
  void set_address(u32 address) { m_address = address; }


  /**
   * Set the data.
   *
   * For write transacitions, this method may only be called by the boot loader
   * (although typically, the constructor or the initialize() method would be used
   * instead of this method).  For read transactions, this method must only be called by
   * the TX targeted by the read transaction.
   *
   * @see get_data()
   */
  void set_data(u32 data) { m_data = data; }


  /**
   * Set whether or not this is a configuration transaction.
   *
   * @see get_config_xfer()
   */
  void set_config_xfer(bool config_xfer) { m_config_xfer = config_xfer; }


  /**
   * Set whether or not this is a write transaction.
   *
   * @see get_write()
   */
  void set_write(bool write) { m_write = write; }


  /**
   * Set whether or not this read transaction is carrying read data.
   *
   * Before calling this method, the TX targeted by a read transaction should first call
   * get_read_data() and throw an exception if it returns true.  It should then call
   * this method with an argument of true.
   *
   * @see get_read_data()
   */
  void set_read_data(bool read_data) { m_read_data = read_data; }


  /**
   * Set whether or not this transaction should be handled turbo style.  
   *
   * In turbo style, the transaction should propagate completely around the TX chain
   * without yielding to the SystemC kernel (that is, in a single delta cycle), this
   * includes any reading or writing of configuration registers and any reading or
   * writing of the DRAM or IRAM memories.  In non-turbo style, each TX in the TX chain
   * has a write transaction for one clock cycle.  For a read transaction, each
   * non-targeted TX also has the transaction for one clock cycle while the TX targeted
   * by the read transaction has the transaction for two cycles (the extra cycle is to
   * allow the read transaction to the local memory (DRAM or IRAM) to take place prior
   * to the transaction being passed to the next TX in the chain.
   *
   * @see get_turbo()
   */
  void set_turbo(bool turbo) { m_turbo = turbo; }


  /**
   * This method dumps this xfer transactions's info to the specified ostream object.
   *
   * The format of the output is:
   * \verbatim
       tag=<Tag> <Type>! #<Address>=<Data>*

       Where:
        <Tag>           is m_tag in decimal.
        <Type>          is Done if m_done is true, else is Write if m_write is true,
                        else is Read.
        !               indicates m_turbo is true.
        #               indicates m_config_xfer is true.
        <Address>       is m_address in hexadecimal (word address).
        <Data>          is the contents of m_data in hexadecimal.
        *               indicates m_read_data is false (so data is suspect).
     \endverbatim
   *
   * @param     os              The ostream object to which the info should be dumped.
   *
   */
  void dump(std::ostream& os = std::cout) const;


private:

  // Initialize to all 0's
  void zeroize();

  bool          m_done;                 ///<  m_data contains Done pin value (XFER block just passes this one along)
  u32           m_address;              ///<  The word address  (byte address = m_address*4)
  u32           m_data;                 ///<  The data
  bool          m_config_xfer;          ///<  True if configuration transaction, false if regular transaction
  bool          m_write;                ///<  True if write transaction, false if read transaction
  bool          m_read_data;            ///<  Set to true by the TX core targeted by a read transaction
  bool          m_turbo;                ///<  Use fast-access (peek/poke)
  u64           m_tag;                  ///<  Unique tag per XFER transaction (artificial)
};



/**
 * Dump an xtsc_tx_xfer object.
 *
 * This operator dumps an xtsc_tx_xfer object using the xtsc_tx_xfer::dump() method.
 *
 */
XTSC_API std::ostream& operator<<(std::ostream& os, const xtsc_tx_xfer& xfer);


} // namespace xtsc


#endif  // _XTSC_TX_XFER_H_
