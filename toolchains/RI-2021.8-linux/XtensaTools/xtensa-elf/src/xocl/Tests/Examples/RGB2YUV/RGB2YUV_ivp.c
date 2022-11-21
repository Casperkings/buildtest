#include "xi_lib.h"
{
  extern XI_ERR_TYPE xiCvtColor_U8_RGB2YUV_BT709(
      const xi_pArray rgb, xi_pArray y, xi_pArray u, xi_pArray v);

  // call to IVP func
  // Source and destination tiles
  xi_tile src_t, dstY_t, dstU_t, dstV_t;

  // SRC
  src_t.pBuffer = local_src;
  src_t.pData = (local_src) + local_src_pitch * IMAGE_PAD + IMAGE_PAD;
  // XI_TILE_SET_BUFF_PTR    ((&src_t), local_src);
  // XI_TILE_SET_DATA_PTR    ((&src_t), ((uint8_t*)local_src) +
  // local_src_pitch*IMAGE_PAD + IMAGE_PAD);
  XI_TILE_SET_BUFF_SIZE((&src_t), local_src_pitch * (inTileH + 2 * IMAGE_PAD));
  XI_TILE_SET_WIDTH((&src_t), inTileW);
  XI_TILE_SET_HEIGHT((&src_t), inTileH);
  XI_TILE_SET_PITCH((&src_t), local_src_pitch);
  XI_TILE_SET_TYPE((&src_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&src_t), IMAGE_PAD);
  XI_TILE_SET_EDGE_HEIGHT((&src_t), IMAGE_PAD);
  XI_TILE_SET_X_COORD((&src_t), 0);
  XI_TILE_SET_Y_COORD((&src_t), 0);

  // DST_Y
  dstY_t.pBuffer = local_dstY;
  dstY_t.pData = local_dstY;
  // XI_TILE_SET_BUFF_PTR    ((&dstY_t), local_dstY);
  // XI_TILE_SET_DATA_PTR    ((&dstY_t), local_dstY);
  XI_TILE_SET_BUFF_SIZE((&dstY_t), (local_dst_pitch * (outTileH)));
  XI_TILE_SET_WIDTH((&dstY_t), outTileW);
  XI_TILE_SET_HEIGHT((&dstY_t), outTileH);
  XI_TILE_SET_PITCH((&dstY_t), (local_dst_pitch));
  XI_TILE_SET_TYPE((&dstY_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&dstY_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&dstY_t), 0);
  XI_TILE_SET_X_COORD((&dstY_t), 0);
  XI_TILE_SET_Y_COORD((&dstY_t), 0);

  // DST_U
  dstU_t.pBuffer = local_dstU;
  dstU_t.pData = local_dstU;
  // XI_TILE_SET_BUFF_PTR    ((&dstU_t), local_dstU);
  // XI_TILE_SET_DATA_PTR    ((&dstU_t), local_dstU);
  XI_TILE_SET_BUFF_SIZE((&dstU_t), (local_dst_pitch * (outTileH)));
  XI_TILE_SET_WIDTH((&dstU_t), outTileW);
  XI_TILE_SET_HEIGHT((&dstU_t), outTileH);
  XI_TILE_SET_PITCH((&dstU_t), (local_dst_pitch));
  XI_TILE_SET_TYPE((&dstU_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&dstU_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&dstU_t), 0);
  XI_TILE_SET_X_COORD((&dstU_t), 0);
  XI_TILE_SET_Y_COORD((&dstU_t), 0);

  // DST_V
  dstV_t.pBuffer = local_dstV;
  dstV_t.pData = local_dstV;
  // XI_TILE_SET_BUFF_PTR    ((&dstV_t), local_dstV);
  // XI_TILE_SET_DATA_PTR    ((&dstV_t), local_dstV);
  XI_TILE_SET_BUFF_SIZE((&dstV_t), (local_dst_pitch * (outTileH)));
  XI_TILE_SET_WIDTH((&dstV_t), outTileW);
  XI_TILE_SET_HEIGHT((&dstV_t), outTileH);
  XI_TILE_SET_PITCH((&dstV_t), (local_dst_pitch));
  XI_TILE_SET_TYPE((&dstV_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&dstV_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&dstV_t), 0);
  XI_TILE_SET_X_COORD((&dstV_t), 0);
  XI_TILE_SET_Y_COORD((&dstV_t), 0);
  int err;
  err = xiCvtColor_U8_RGB2YUV_BT709((xi_pArray)&src_t, (xi_pArray)&dstY_t,
                                    (xi_pArray)&dstU_t, (xi_pArray)&dstV_t);
}
