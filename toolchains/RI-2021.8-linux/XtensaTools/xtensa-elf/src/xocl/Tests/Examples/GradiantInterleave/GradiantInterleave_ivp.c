#include "xi_lib.h"
{
  extern XI_ERR_TYPE xiHOGGradient_interleave_U8S16(const xi_pTile src,
                                                    xi_pArray dst_dx_dy);

  xi_tile src_t, dstX_t, dstY_t;
  // SRC
  src_t.pBuffer = local_src;
  src_t.pData = local_src + inTilePitch * IMAGE_PAD + IMAGE_PAD;
  XI_TILE_SET_BUFF_SIZE((&src_t), inTilePitch * (inTileH + 2 * IMAGE_PAD));
  XI_TILE_SET_WIDTH((&src_t), inTileW);
  XI_TILE_SET_HEIGHT((&src_t), inTileH);
  XI_TILE_SET_PITCH((&src_t), inTilePitch);
  XI_TILE_SET_TYPE((&src_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&src_t), IMAGE_PAD);
  XI_TILE_SET_EDGE_HEIGHT((&src_t), IMAGE_PAD);
  XI_TILE_SET_X_COORD((&src_t), 0);
  XI_TILE_SET_Y_COORD((&src_t), 0);

  // DST_X
  dstX_t.pBuffer = (__local uchar *)local_dstxy;
  dstX_t.pData = (__local uchar *)local_dstxy;
  XI_TILE_SET_BUFF_SIZE((&dstX_t), (local_dst_pitch * (outTileH)*dstBytes));
  XI_TILE_SET_WIDTH((&dstX_t), outTileW);
  XI_TILE_SET_HEIGHT((&dstX_t), outTileH);
  XI_TILE_SET_PITCH((&dstX_t), (local_dst_pitch));
  XI_TILE_SET_TYPE((&dstX_t), XI_TILE_S16);
  XI_TILE_SET_EDGE_WIDTH((&dstX_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&dstX_t), 0);
  XI_TILE_SET_X_COORD((&dstX_t), 0);
  XI_TILE_SET_Y_COORD((&dstX_t), 0);

  // IVP
  err = xiHOGGradient_interleave_U8S16(&src_t, (xi_pArray)&dstX_t);
}
