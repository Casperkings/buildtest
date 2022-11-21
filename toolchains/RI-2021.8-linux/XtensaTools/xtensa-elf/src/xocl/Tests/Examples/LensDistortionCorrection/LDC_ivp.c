#include "xi_lib.h"
{
  extern void image_transform_420sp(
      ushort * input_x, ushort * input_y, const xi_pTile srcY,
      const xi_pTile srcUV, xi_pTile dstY, xi_pTile dstUV, int bbx, int bby);

  xi_tile srcY_t, srcUV_t;
  xi_tile dstY_t, dstUV_t;

  // SRC
  srcY_t.pBuffer = local_srcY;
  srcY_t.pData = local_srcY;
  XI_TILE_SET_BUFF_SIZE((&srcY_t), local_src_pitch * inTileH);
  XI_TILE_SET_WIDTH((&srcY_t), inTileW);
  XI_TILE_SET_HEIGHT((&srcY_t), inTileH);
  XI_TILE_SET_PITCH((&srcY_t), local_src_pitch);
  XI_TILE_SET_TYPE((&srcY_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&srcY_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&srcY_t), 0);
  XI_TILE_SET_X_COORD((&srcY_t), 0);
  XI_TILE_SET_Y_COORD((&srcY_t), 0);

  srcUV_t.pBuffer = local_srcUV;
  srcUV_t.pData = local_srcUV;
  XI_TILE_SET_BUFF_SIZE((&srcUV_t), (local_src_pitch * (inTileH)) / 2);
  XI_TILE_SET_WIDTH((&srcUV_t), inTileW);
  XI_TILE_SET_HEIGHT((&srcUV_t), (inTileH) / 2);
  XI_TILE_SET_PITCH((&srcUV_t), local_src_pitch);
  XI_TILE_SET_TYPE((&srcUV_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&srcUV_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&srcUV_t), 0);
  XI_TILE_SET_X_COORD((&srcUV_t), 0);
  XI_TILE_SET_Y_COORD((&srcUV_t), 0);

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
  XI_TILE_SET_X_COORD((&dstY_t), 0);
  XI_TILE_SET_Y_COORD((&dstY_t), 0);

  dstUV_t.pBuffer = local_dstUV;
  dstUV_t.pData = local_dstUV;
  XI_TILE_SET_BUFF_SIZE((&dstUV_t), (local_dst_pitch * (outTileH)) / 2);
  XI_TILE_SET_WIDTH((&dstUV_t), outTileW);
  XI_TILE_SET_HEIGHT((&dstUV_t), outTileH / 2);
  XI_TILE_SET_PITCH((&dstUV_t), (local_dst_pitch));
  XI_TILE_SET_TYPE((&dstUV_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&dstUV_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&dstUV_t), 0);
  XI_TILE_SET_X_COORD((&dstUV_t), 0);
  XI_TILE_SET_Y_COORD((&dstUV_t), 0);

  image_transform_420sp(inputxt, inputyt, &srcY_t, &srcUV_t, &dstY_t, &dstUV_t,
                        tlx, tly);
}
