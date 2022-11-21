#include "xi_lib.h"
{
  extern XI_ERR_TYPE xiWarpPerspectiveU8_Q13_18(xi_pTile, xi_pTile,
                                                __constant uint * restrict);

  xi_tile srcY_t;
  xi_tile dstY_t;
  // SRC
  srcY_t.pBuffer = local_srcY;
  srcY_t.pData = local_srcY;
  XI_TILE_SET_BUFF_SIZE((&srcY_t), local_src_pitch * (inTileH));
  XI_TILE_SET_WIDTH((&srcY_t), inTileW);
  XI_TILE_SET_HEIGHT((&srcY_t), inTileH);
  XI_TILE_SET_PITCH((&srcY_t), local_src_pitch);
  XI_TILE_SET_TYPE((&srcY_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&srcY_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&srcY_t), 0);
  XI_TILE_SET_X_COORD((&srcY_t), startX);
  XI_TILE_SET_Y_COORD((&srcY_t), startY);

  // DST_DX
  dstY_t.pBuffer = local_dstY;
  dstY_t.pData = local_dstY;
  XI_TILE_SET_BUFF_SIZE((&dstY_t), (local_dst_pitch * (outTileH)));
  XI_TILE_SET_WIDTH((&dstY_t), outTileW);
  XI_TILE_SET_HEIGHT((&dstY_t), outTileH);
  XI_TILE_SET_PITCH((&dstY_t), (local_dst_pitch));
  XI_TILE_SET_TYPE((&dstY_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&dstY_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&dstY_t), 0);
  XI_TILE_SET_X_COORD((&dstY_t), out_j);
  XI_TILE_SET_Y_COORD((&dstY_t), out_i);

  xiWarpPerspectiveU8_Q13_18(&srcY_t, &dstY_t, perspective);
}
