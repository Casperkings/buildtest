#ifndef HALIDE_MEMCOPY_H
#define HALIDE_MEMCOPY_H

/** \file
 * Defines the lowering pass that injects DMA copy calls when requests to 
 * perform DMA operations appears in the schedule.
 */

#include <map>

#include "IR.h"
#include "Schedule.h"
#include "Target.h"

namespace Halide {
namespace Internal {

/** Compute the actual region to be prefetched and place it to the
  * placholder prefetch. Wrap the prefetch call with condition when
  * applicable. */
Stmt inject_memcopy(Stmt s, const std::map<std::string, Function> &env);

}  // namespace Internal
}  // namespace Halide

#endif
