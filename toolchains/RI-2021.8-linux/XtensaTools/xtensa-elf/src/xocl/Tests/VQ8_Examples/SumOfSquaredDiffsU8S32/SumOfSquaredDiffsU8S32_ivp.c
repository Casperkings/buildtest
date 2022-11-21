#include "xi_lib.h"
{
  extern XI_ERR_TYPE SumOfSquaredDiffsU8S32(
      const __local uint8_t *src1, const __local uint8_t *src2,
      __local uint32_t *dst, int32_t width, int32_t nvectors, int32_t nlength);

  SumOfSquaredDiffsU8S32(local_src1, local_src, local_dst, TILE_W, TILE_H, 16);
}
