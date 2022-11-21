#ifndef _XTSC_MEMORY_BASE_H_
#define _XTSC_MEMORY_BASE_H_

// Copyright (c) 2005-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

#include <xtsc/xtsc.h>
#include <xtsc/xtsc_memory_b.h>
#include <cstring>


namespace xtsc_component {


// Use of this class is deprecated, use xtsc::xtsc_memory_b directly instead.
class XTSC_COMP_API xtsc_memory_base : public xtsc::xtsc_memory_b {
public:


  xtsc_memory_base(const char          *name,
                   const char          *kind,
                   xtsc::u32            byte_width,
                   xtsc::xtsc_address   start_byte_address,
                   xtsc::xtsc_address   memory_byte_size,
                   xtsc::u32            page_byte_size,
                   const char          *initial_value_file,
                   xtsc::u8             memory_fill_byte);

};



}  // namespace xtsc_component


#endif  // _XTSC_MEMORY_BASE_H_
