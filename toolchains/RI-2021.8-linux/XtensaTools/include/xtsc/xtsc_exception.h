#ifndef _XTSC_EXCEPTION_H_
#define _XTSC_EXCEPTION_H_

// Copyright (c) 2005-2010 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

/**
 * @file 
 */


#include <stdexcept>
#include <iostream>
#include "xtsc/xtsc_types.h"


// This is for the many commercial tools which kindly tell you that an exception has been
// thrown but often neglect to tell you what the exception is and, instead, claim that
// std::exception is an "UNKNOWN EXCEPTION"
#if defined(XTSC_SD) || defined(MTI_SYSTEMC) || defined(NCSC_REG_H) || defined(VCS_SYSTEMC)
#define XTSC_PRINT_EXCEPTION_MESSAGES
#endif

namespace xtsc {

#ifndef DOXYGEN_SKIP
XTSC_PRAGMA_WARNING(push)
XTSC_PRAGMA_WARNING(disable:4275)
#endif // DOXYGEN_SKIP

/**
 * Base class for all XTSC exceptions.
 */
class XTSC_API xtsc_exception : public std::runtime_error {
public:
  /// Constructor.
  xtsc_exception(const std::string& message) : std::runtime_error(message) {
#if defined(XTSC_PRINT_EXCEPTION_MESSAGES)
    if (!getenv("XTSC_NO_PRINT_EXCEPTION_MESSAGES")) {
      std::cerr << "xtsc_exception: " << message << std::endl;
    }
#endif
  }
};
#ifndef DOXYGEN_SKIP
XTSC_PRAGMA_WARNING(pop)
#endif // DOXYGEN_SKIP


} // namespace xtsc


#endif  // _XTSC_EXCEPTION_H_
