#include "xi_lib.h"
{
  extern void xvCornerHarris5x5Tile(
      int8_t * p_s8Dx, int8_t * p_s8Dy, __local uint8_t * p_u8In,
      __local uint8_t * p_u8Out, uint32_t u32Width, uint32_t u32Height,
      int32_t s32SrcStride, uint32_t u32Shift);

  xvCornerHarris5x5Tile(Dx, Dy, local_src, local_dst, outTileW, outTileH,
                        local_src_pitch, 5);
}
