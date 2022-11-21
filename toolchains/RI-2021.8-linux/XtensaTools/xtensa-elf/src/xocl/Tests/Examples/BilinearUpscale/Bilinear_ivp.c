#include "xi_lib.h"
{
  extern XI_ERR_TYPE xiResizeBilinear_U8_Q13_18(
      const xi_pTile src, xi_pTile dst, XI_Q13_18 xscale, XI_Q13_18 yscale,
      XI_Q13_18 xshift, XI_Q13_18 yshift);

  xi_tile src_t, dst_t;
  // SRC
  src_t.pBuffer = local_src;
  src_t.pData = local_src + inTilePitch * IMAGE_PAD + IMAGE_PAD;
  // XI_TILE_SET_BUFF_PTR    ((&src_t), memSrcTile);
  // XI_TILE_SET_DATA_PTR    ((&src_t), ((uint8_t*)memSrcTile) +
  // inTilePitch*IMAGE_PAD + IMAGE_PAD);
  XI_TILE_SET_BUFF_SIZE((&src_t), inTilePitch * (inTileH + 2 * IMAGE_PAD));
  XI_TILE_SET_WIDTH((&src_t), inTileW);
  XI_TILE_SET_HEIGHT((&src_t), inTileH);
  XI_TILE_SET_PITCH((&src_t), inTilePitch);
  XI_TILE_SET_TYPE((&src_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&src_t), IMAGE_PAD);
  XI_TILE_SET_EDGE_HEIGHT((&src_t), IMAGE_PAD);
  XI_TILE_SET_X_COORD((&src_t), j);
  XI_TILE_SET_Y_COORD((&src_t), i);

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
  XI_TILE_SET_X_COORD((&dst_t), out_j);
  XI_TILE_SET_Y_COORD((&dst_t), out_i);

  XI_Q13_18 scale_x = (XI_Q13_18)xscale; //(src_t.width  << 18) / dst_t.width;
  XI_Q13_18 scale_y = (XI_Q13_18)yscale; //(src_t.height << 18) / dst_t.height;

  XI_Q13_18 shift_x = (XI_Q13_18)xshift; //(0.0f * (1 << 18));
  XI_Q13_18 shift_y = (XI_Q13_18)yshift; //(0.0f * (1 << 18));
  // IVP
  err = xiResizeBilinear_U8_Q13_18(&src_t, &dst_t, scale_x, scale_y, shift_x,
                                   shift_y);
}
