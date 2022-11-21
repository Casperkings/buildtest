#include "xi_lib.h"
{
  extern void xvFilter5x5Bilateral(xi_pTile src, xi_pTile dst, void *tempDiv,
                                   __constant uchar *coeffBuff);

  xi_tile src_t, dst_t;
  // SRC
  src_t.pBuffer = local_src;
  src_t.pData = local_src + inTilePitch * H_EXT + W_EXT;
  XI_TILE_SET_BUFF_SIZE((&src_t), inTilePitch * (inTileH + 2 * H_EXT));
  XI_TILE_SET_WIDTH((&src_t), inTileW);
  XI_TILE_SET_HEIGHT((&src_t), inTileH);
  XI_TILE_SET_PITCH((&src_t), inTilePitch);
  XI_TILE_SET_TYPE((&src_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&src_t), W_EXT);
  XI_TILE_SET_EDGE_HEIGHT((&src_t), H_EXT);
  XI_TILE_SET_X_COORD((&src_t), 0);
  XI_TILE_SET_Y_COORD((&src_t), 0);

  // DST_
  dst_t.pBuffer = local_dst;
  dst_t.pData = local_dst;
  XI_TILE_SET_BUFF_SIZE((&dst_t), (local_dst_pitch * (outTileH)*dstBytes));
  XI_TILE_SET_WIDTH((&dst_t), outTileW);
  XI_TILE_SET_HEIGHT((&dst_t), outTileH);
  XI_TILE_SET_PITCH((&dst_t), (local_dst_pitch));
  XI_TILE_SET_TYPE((&dst_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&dst_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&dst_t), 0);
  XI_TILE_SET_X_COORD((&dst_t), 0);
  XI_TILE_SET_Y_COORD((&dst_t), 0);

  xvFilter5x5Bilateral(&src_t, &dst_t, tempDiv, Coef);
}
