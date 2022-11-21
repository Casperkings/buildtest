#ifndef _XTSC_ADDRESS_RANGE_ENTRY_H_
#define _XTSC_ADDRESS_RANGE_ENTRY_H_

// Copyright (c) 2005-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */


#include <xtsc/xtsc_types.h>


namespace xtsc {

/**
 * Address-range to port-number association (for example, a routing table entry).
 *
 * This class represents an address-range-to-port-number association such as
 * might constitute a single entry in a routing table or address translation
 * table.  It is a plain old data (or POD) class, whose members are meant to
 * be directly read or written; however, in normal usage, the data members
 * would be written just once (at construction time).
 *
 * @see xtsc_component::xtsc_router
 * @see xtsc_component::xtsc_arbiter
 */
class XTSC_API xtsc_address_range_entry {
public:

  typedef xtsc::xtsc_address    xtsc_address;
  typedef xtsc::u32             u32;

  /**
   * Constructor.
   *
   * @param   start_address8    The lowest byte address in the memory range.
   * @param   end_address8      The highest byte address in the memory range.
   * @param   port_num          The port number to associate with the given
   *                            address range.
   * @param   delta             The address translation to apply.  This amount
   *                            should be added to the address of each request.
   */
  xtsc_address_range_entry(xtsc_address start_address8, xtsc_address end_address8, u32 port_num, xtsc_address delta=0) :
    m_start_address8    (start_address8),
    m_end_address8      (end_address8),
    m_port_num          (port_num),
    m_delta             (delta)
  {}
  xtsc_address    m_start_address8;
  xtsc_address    m_end_address8;
  u32             m_port_num;
  xtsc_address    m_delta;

  void dump(std::ostream& os) const;

};


XTSC_API std::ostream& operator<<(std::ostream& os, const xtsc_address_range_entry& entry);

}  // namespace xtsc

#endif  // _XTSC_ADDRESS_RANGE_ENTRY_H_
