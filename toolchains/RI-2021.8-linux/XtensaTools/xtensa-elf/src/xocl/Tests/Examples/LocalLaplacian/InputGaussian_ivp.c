//#include "xi_lib.h"
{
  extern XI_ERR_TYPE xiPyrDown_3x3_U8(const xi_pTile src, xi_pArray dst);
  extern XI_ERR_TYPE xiEdgeRepeatU8(xi_pTile src, int top, int bottom, int left,
                                    int right);

  xi_tile src_t;
  xi_tile dst_t;

  // SRC
  src_t.pBuffer = local_src;
  src_t.pData = (local_src + (inTileEdge * local_src_pitch + inTileEdge));
  XI_TILE_SET_BUFF_SIZE((&src_t), local_src_pitch * (inTileH + 2 * inTileEdge) *
                                      srcBytes);
  XI_TILE_SET_WIDTH((&src_t), inTileW);
  XI_TILE_SET_HEIGHT((&src_t), inTileH);
  XI_TILE_SET_PITCH((&src_t), local_src_pitch);
  XI_TILE_SET_TYPE((&src_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&src_t), inTileEdge);
  XI_TILE_SET_EDGE_HEIGHT((&src_t), inTileEdge);
  XI_TILE_SET_X_COORD((&src_t), 0);
  XI_TILE_SET_Y_COORD((&src_t), 0);

  // DST_DX
  dst_t.pBuffer = local_dst;
  dst_t.pData = local_dst + (outTileEdge * local_dst_pitch + outTileEdge);
  XI_TILE_SET_BUFF_SIZE(
      (&dst_t), (local_dst_pitch * (outTileH + (2 * outTileEdge)) * dstBytes));
  XI_TILE_SET_WIDTH((&dst_t), outTileW);
  XI_TILE_SET_HEIGHT((&dst_t), outTileH);
  XI_TILE_SET_PITCH((&dst_t), local_dst_pitch);
  XI_TILE_SET_TYPE((&dst_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&dst_t), outTileEdge);
  XI_TILE_SET_EDGE_HEIGHT((&dst_t), outTileEdge);
  XI_TILE_SET_X_COORD((&dst_t), 0);
  XI_TILE_SET_Y_COORD((&dst_t), 0);
  int err = 0;
  err = xiPyrDown_3x3_U8(&src_t, (xi_pArray)&dst_t);
  err = xiEdgeRepeatU8(&dst_t, (indY == 0), (indY == indY_size), (indX == 0),
                       (indX == indX_size));
  // err = xiPyrDown_3x3_U8_ref(&src_t, (xi_pArray)&dst_t);
  // err = xiEdgeRepeatU8_ref(&dst_t, (indY == 0), (indY == indY_size), (indX ==
  // 0), (indX == indX_size));
}
