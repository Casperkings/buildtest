#ifndef _XTSC_TX_XFER_IF_H_
#define _XTSC_TX_XFER_IF_H_

// Copyright (c) 2009-2010 by Tensilica Inc.  ALL RIGHTS RESERVED.
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



class xtsc_tx_xfer;



/**
 * Interface for sending TLM TX XFER interface transactions.
 *
 * This interface is used for sending TLM XFER interface transactions around a boot
 * loader TX Xtensa chain.  A TX chain starts and ends at the boot loader and has one or
 * more TX cores in the chain.
 *
 * @see xtsc_tx_xfer
 * @see xtsc_core
 */
class XTSC_API xtsc_tx_xfer_if : virtual public sc_core::sc_interface {
public:


  /**
   * Method to send a TLM XFER transaction.
   *
   * This method is used to send an XFER transaction in the following three situations:
   *  - From the boot loader to the first TX core in the chain.
   *  - From one TX core in the chain to the next TX core in the chain.
   *  - From the last TX core in the chain to the boot loader.
   *
   * The boot loader is responsible for creating and destroying the xtsc_tx_xfer object.
   *
   * By contract, the boot loader may not destroy or re-use the xtsc_tx_xfer object until
   * the delta cycle after the final TX core in the chain has called the nb_tx_xfer()
   * method of the boot loader.
   *
   * By contract, a TX core may not modify the xtsc_tx_xfer object passed to it by a call
   * to its nb_tx_xfer() method after it has called the nb_tx_xfer() method of the downstream
   * module (either the next TX core in the chain or the boot loader).
   *
   * A TX core is allow to use an xtsc_tx_xfer in a read-only capacity (for example, for
   * logging) after it has called nb_tx_xfer of the downstream module so-long as it has not
   * yielded to the SystemC kernel.
   *
   * @param     tx_xfer         The xtsc_tx_xfer object.
   *
   * This non-blocking method must never call the SystemC wait() method (either directly
   * or indirectly).
   *
   * @see xtsc_tx_xfer
   */
  virtual void nb_tx_xfer(xtsc_tx_xfer& tx_xfer) = 0;

  
};



} // namespace xtsc


#endif  // _XTSC_TX_XFER_IF_H_
