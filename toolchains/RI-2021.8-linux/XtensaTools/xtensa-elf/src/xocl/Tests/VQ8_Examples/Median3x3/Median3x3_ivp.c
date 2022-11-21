#include "xi_lib.h"

{
  extern XI_ERR_TYPE xiMedianBlur_3x3_U8(const xi_pTile src, xi_pArray dst);

  xi_tile src_t, dst_t;

  // SRC
  src_t.pBuffer = local_src;
  src_t.pData = local_src + inTilePitch * IMAGE_PAD + IMAGE_PAD;
  // XI_TILE_SET_BUFF_PTR    ((&src_t), memSrcTile);
  // XI_TILE_SET_DATA_PTR    ((&src_t), ((uint8_t*)memSrcTile) +
  // inTilePitch*IMAGE_PAD + IMAGE_PAD);
  XI_TILE_SET_BUFF_SIZE((&src_t),
                        inTilePitch * (inTileH + 2 * IMAGE_PAD) * srcBytes);
  XI_TILE_SET_WIDTH((&src_t), inTileW);
  XI_TILE_SET_HEIGHT((&src_t), inTileH);
  XI_TILE_SET_PITCH((&src_t), inTilePitch);
  XI_TILE_SET_TYPE((&src_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&src_t), IMAGE_PAD);
  XI_TILE_SET_EDGE_HEIGHT((&src_t), IMAGE_PAD);
  XI_TILE_SET_X_COORD((&src_t), 0);
  XI_TILE_SET_Y_COORD((&src_t), 0);

  // DST_
  dst_t.pBuffer = local_dst;
  dst_t.pData = local_dst;
  // XI_TILE_SET_BUFF_PTR    ((&dst_t), memDstTile);
  // XI_TILE_SET_DATA_PTR    ((&dst_t), memDstTile);
  XI_TILE_SET_BUFF_SIZE((&dst_t), (local_dst_pitch * (outTileH)*dstBytes));
  XI_TILE_SET_WIDTH((&dst_t), outTileW);
  XI_TILE_SET_HEIGHT((&dst_t), outTileH);
  XI_TILE_SET_PITCH((&dst_t), (local_dst_pitch));
  XI_TILE_SET_TYPE((&dst_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&dst_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&dst_t), 0);
  XI_TILE_SET_X_COORD((&dst_t), 0);
  XI_TILE_SET_Y_COORD((&dst_t), 0);

  // IVP
  err = xiMedianBlur_3x3_U8(&src_t, (xi_pArray)&dst_t);
}
