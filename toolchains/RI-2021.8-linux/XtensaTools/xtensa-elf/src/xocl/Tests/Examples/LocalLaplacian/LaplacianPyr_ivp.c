#include "xi_lib.h"
{
  extern XI_ERR_TYPE xiPyrDown_3x3_U8(const xi_pTile src, xi_pArray dst);
  extern XI_ERR_TYPE xiPyrUpK_S16(const xi_pTile src, const xi_pTile src2, 
                                  xi_pTile dst);
  extern XI_ERR_TYPE xiKEdgeRepeatS16(xi_pTile src, int top, int bottom, 
                                      int left, int right);

 xi_tile src_t1, src_t;
  xi_tile dst_t;
//SRC1
  src_t1.pBuffer =  (__local uchar *)local_srcPrev;
  src_t1.pData =  (__local uchar *)(local_srcPrev + (in1TileEdge * (local_srcPrev_pitch * srcElements) + (in1TileEdge * srcElements)));
  XI_TILE_SET_BUFF_SIZE((&src_t1), ((local_srcPrev_pitch * srcElements) * (in1TileH + (2 * in1TileEdge)) * srcBytes));
  XI_TILE_SET_WIDTH((&src_t1), in1TileW * srcElements);
  XI_TILE_SET_HEIGHT((&src_t1), in1TileH);
  XI_TILE_SET_PITCH((&src_t1), local_srcPrev_pitch * srcElements);
  XI_TILE_SET_TYPE((&src_t1), XI_TILE_S16);
  XI_TILE_SET_EDGE_WIDTH((&src_t1), in1TileEdge * srcElements);
  XI_TILE_SET_EDGE_HEIGHT((&src_t1), in1TileEdge);
  XI_TILE_SET_X_COORD((&src_t1), 0);
  XI_TILE_SET_Y_COORD((&src_t1), 0);
  
  //SRC
  src_t.pBuffer = (__local uchar *)local_src;
  src_t.pData = (__local uchar *)(local_src + (inTileEdge * (local_src_pitch * srcElements) + (inTileEdge * srcElements)));
  XI_TILE_SET_BUFF_SIZE((&src_t), ((local_src_pitch * srcElements) * (inTileH + (2 * inTileEdge)) * srcBytes));
  XI_TILE_SET_WIDTH((&src_t), inTileW * srcElements);
  XI_TILE_SET_HEIGHT((&src_t), inTileH);
  XI_TILE_SET_PITCH((&src_t), local_src_pitch * srcElements);
  XI_TILE_SET_TYPE((&src_t), XI_TILE_S16);
  XI_TILE_SET_EDGE_WIDTH((&src_t), inTileEdge * srcElements);
  XI_TILE_SET_EDGE_HEIGHT((&src_t), inTileEdge);
  XI_TILE_SET_X_COORD((&src_t), 0);
  XI_TILE_SET_Y_COORD((&src_t), 0);
  
//DST_DX
  dst_t.pBuffer = (__local uchar *)local_dst;
  dst_t.pData = (__local uchar *)(local_dst + (outTileEdge * (local_dst_pitch * dstElements) + (outTileEdge * dstElements)));
  XI_TILE_SET_BUFF_SIZE((&dst_t), ((local_dst_pitch * dstElements) * (outTileH + (2 * outTileEdge)) * dstBytes));
  XI_TILE_SET_WIDTH((&dst_t), outTileW * dstElements);
  XI_TILE_SET_HEIGHT((&dst_t), outTileH);
  XI_TILE_SET_PITCH((&dst_t), local_dst_pitch * dstElements);
  XI_TILE_SET_TYPE((&dst_t), XI_TILE_S16);
  XI_TILE_SET_EDGE_WIDTH((&dst_t), outTileEdge * dstElements);
  XI_TILE_SET_EDGE_HEIGHT((&dst_t), outTileEdge);
  XI_TILE_SET_X_COORD((&dst_t), 0);
  XI_TILE_SET_Y_COORD((&dst_t), 0);
  int err = 0;
  err = xiPyrUpK_S16(&src_t1, &src_t, &dst_t);   
  err = xiKEdgeRepeatS16(&dst_t, (indY == 0), (indY == indY_size), (indX == 0), (indX == indX_size));

  //err = xiPyrUpK_S16_ref(&src_t1, &src_t, &dst_t); 
  //err = xiKEdgeRepeatS16_ref(&dst_t, (indY == 0), (indY == indY_size), (indX == 0), (indX == indX_size));

}
