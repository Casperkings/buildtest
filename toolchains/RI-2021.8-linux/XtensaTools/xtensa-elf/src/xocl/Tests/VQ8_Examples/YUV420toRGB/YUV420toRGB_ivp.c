#include "xi_lib.h"
{
  extern XI_ERR_TYPE xiCvtColor_U8_YUV2RGB_420_BT709(
      const xi_pArray y, const xi_pArray u, const xi_pArray v, xi_pArray rgb);

  // call to IVP func
  // Source and destination tiles
  xi_tile dst_t, srcY_t, srcU_t, srcV_t;

  // SRC
  dst_t.pBuffer = local_dst;
  dst_t.pData = (local_dst) + local_dst_pitch * IMAGE_PAD + IMAGE_PAD;
  // XI_TILE_SET_BUFF_PTR    ((&dst_t), local_src);
  // XI_TILE_SET_DATA_PTR    ((&dst_t), ((uint8_t*)local_src) +
  // local_src_pitch*IMAGE_PAD + IMAGE_PAD);
  XI_TILE_SET_BUFF_SIZE((&dst_t), local_dst_pitch * (outTileH + 2 * IMAGE_PAD));
  XI_TILE_SET_WIDTH((&dst_t), outTileW);
  XI_TILE_SET_HEIGHT((&dst_t), outTileH);
  XI_TILE_SET_PITCH((&dst_t), local_dst_pitch);
  XI_TILE_SET_TYPE((&dst_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&dst_t), IMAGE_PAD);
  XI_TILE_SET_EDGE_HEIGHT((&dst_t), IMAGE_PAD);
  XI_TILE_SET_X_COORD((&dst_t), 0);
  XI_TILE_SET_Y_COORD((&dst_t), 0);

  // DST_Y
  srcY_t.pBuffer = local_srcY;
  srcY_t.pData = local_srcY;
  // XI_TILE_SET_BUFF_PTR    ((&srcY_t), local_dstY);
  // XI_TILE_SET_DATA_PTR    ((&srcY_t), local_dstY);
  XI_TILE_SET_BUFF_SIZE((&srcY_t), (local_src_pitch * (inTileH)));
  XI_TILE_SET_WIDTH((&srcY_t), inTileW);
  XI_TILE_SET_HEIGHT((&srcY_t), inTileH);
  XI_TILE_SET_PITCH((&srcY_t), (local_src_pitch));
  XI_TILE_SET_TYPE((&srcY_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&srcY_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&srcY_t), 0);
  XI_TILE_SET_X_COORD((&srcY_t), 0);
  XI_TILE_SET_Y_COORD((&srcY_t), 0);

  // DST_U
  srcU_t.pBuffer = local_srcU;
  srcU_t.pData = local_srcU;
  // XI_TILE_SET_BUFF_PTR    ((&srcU_t), local_dstU);
  // XI_TILE_SET_DATA_PTR    ((&srcU_t), local_dstU);
  XI_TILE_SET_BUFF_SIZE((&srcU_t), (local_src_pitch * (inTileH)) / 4);
  XI_TILE_SET_WIDTH((&srcU_t), inTileW / 2);
  XI_TILE_SET_HEIGHT((&srcU_t), inTileH / 2);
  XI_TILE_SET_PITCH((&srcU_t), (local_src_pitch) / 2);
  XI_TILE_SET_TYPE((&srcU_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&srcU_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&srcU_t), 0);
  XI_TILE_SET_X_COORD((&srcU_t), 0);
  XI_TILE_SET_Y_COORD((&srcU_t), 0);

  // DST_V
  srcV_t.pBuffer = local_srcV;
  srcV_t.pData = local_srcV;
  // XI_TILE_SET_BUFF_PTR    ((&srcV_t), local_dstV);
  // XI_TILE_SET_DATA_PTR    ((&srcV_t), local_dstV);
  XI_TILE_SET_BUFF_SIZE((&srcV_t), (local_src_pitch * (inTileH)) / 4);
  XI_TILE_SET_WIDTH((&srcV_t), inTileW / 2);
  XI_TILE_SET_HEIGHT((&srcV_t), inTileH / 2);
  XI_TILE_SET_PITCH((&srcV_t), (local_src_pitch) / 2);
  XI_TILE_SET_TYPE((&srcV_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&srcV_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&srcV_t), 0);
  XI_TILE_SET_X_COORD((&srcV_t), 0);
  XI_TILE_SET_Y_COORD((&srcV_t), 0);

  int err;
  err = xiCvtColor_U8_YUV2RGB_420_BT709((xi_pArray)&srcY_t, (xi_pArray)&srcU_t,
                                        (xi_pArray)&srcV_t, (xi_pArray)&dst_t);
}
