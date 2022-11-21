//#include "xi_lib.h"
{
  extern XI_ERR_TYPE xiPyrUp_S16U8(const xi_pTile src1, const xi_pTile src0,
                                   xi_pTile dst);

  xi_tile src0_t, src1_t;
  xi_tile dst_t;

  // SRC1
  src1_t.pBuffer = (__local uchar *)local_src;
  src1_t.pData =
      (__local uchar *)(local_src +
                        (inTileEdge * (local_src_pitch * srcElements) +
                         (inTileEdge * srcElements)));
  XI_TILE_SET_BUFF_SIZE((&src1_t), ((local_src_pitch * srcElements) *
                                    (inTileH + (2 * inTileEdge)) * srcBytes));
  XI_TILE_SET_WIDTH((&src1_t), inTileW * srcElements);
  XI_TILE_SET_HEIGHT((&src1_t), inTileH);
  XI_TILE_SET_PITCH((&src1_t), local_src_pitch * srcElements);
  XI_TILE_SET_TYPE((&src1_t), XI_TILE_S16);
  XI_TILE_SET_EDGE_WIDTH((&src1_t), inTileEdge * srcElements);
  XI_TILE_SET_EDGE_HEIGHT((&src1_t), inTileEdge);
  XI_TILE_SET_X_COORD((&src1_t), 0);
  XI_TILE_SET_Y_COORD((&src1_t), 0);

  // SRC0
  src0_t.pBuffer = (__local uchar *)local_srcPrev;
  src0_t.pData =
      (__local uchar *)(local_srcPrev +
                        (in1TileEdge * (local_srcPrev_pitch * srcElements) +
                         (in1TileEdge * srcElements)));
  XI_TILE_SET_BUFF_SIZE((&src0_t), ((local_srcPrev_pitch * srcElements) *
                                    (in1TileH + (2 * in1TileEdge)) * srcBytes));
  XI_TILE_SET_WIDTH((&src0_t), in1TileW * srcElements);
  XI_TILE_SET_HEIGHT((&src0_t), in1TileH);
  XI_TILE_SET_PITCH((&src0_t), local_srcPrev_pitch * srcElements);
  XI_TILE_SET_TYPE((&src0_t), XI_TILE_S16);
  XI_TILE_SET_EDGE_WIDTH((&src0_t), in1TileEdge * srcElements);
  XI_TILE_SET_EDGE_HEIGHT((&src0_t), in1TileEdge);
  XI_TILE_SET_X_COORD((&src0_t), 0);
  XI_TILE_SET_Y_COORD((&src0_t), 0);

  // DST_DX
  dst_t.pBuffer = (__local uchar *)local_dst;
  dst_t.pData =
      (__local uchar *)(local_dst +
                        (outTileEdge * (local_dst_pitch * dstElements) +
                         (outTileEdge * dstElements)));
  XI_TILE_SET_BUFF_SIZE((&dst_t), ((local_dst_pitch * dstElements) *
                                   (outTileH + (2 * outTileEdge)) * dstBytes));
  XI_TILE_SET_WIDTH((&dst_t), outTileW * dstElements);
  XI_TILE_SET_HEIGHT((&dst_t), outTileH);
  XI_TILE_SET_PITCH((&dst_t), local_dst_pitch * dstElements);
  XI_TILE_SET_TYPE((&dst_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&dst_t), outTileEdge * dstElements);
  XI_TILE_SET_EDGE_HEIGHT((&dst_t), outTileEdge);
  XI_TILE_SET_X_COORD((&dst_t), 0);
  XI_TILE_SET_Y_COORD((&dst_t), 0);
  int err = 0;
  err = xiPyrUp_S16U8(&src1_t, &src0_t, &dst_t);

  // err = xiPyrUp_S16U8_ref(&src1_t, &src0_t, &dst_t);
}
