//#include "xi_lib.h"
{

  extern XI_ERR_TYPE xiKEdgeRepeatS16(xi_pTile src, int top, int bottom,
                                      int left, int right);
  extern XI_ERR_TYPE xiRemap(const xi_pTile src, xi_pTile dst,
                             __local int16_t *table);

  // call to IVP func
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
  XI_TILE_SET_TYPE((&dst_t), XI_TILE_S16);
  XI_TILE_SET_EDGE_WIDTH((&dst_t), outTileEdge * dstElements);
  XI_TILE_SET_EDGE_HEIGHT((&dst_t), outTileEdge);
  XI_TILE_SET_X_COORD((&dst_t), 0);
  XI_TILE_SET_Y_COORD((&dst_t), 0);
  int err = 0;
  err = xiRemap(&src_t, &dst_t, (__local int16_t *)local_lut);
  err = xiKEdgeRepeatS16(&dst_t, (indY == 0), (indY == indY_size), (indX == 0),
                         (indX == indX_size));
  // err = xiRemap_ref(&src_t, &dst_t, (__local int16_t *)local_lut);
  // err = xiKEdgeRepeatS16_ref(&dst_t, (indY == 0), (indY == indY_size), (indX
  // == 0), (indX == indX_size));
}
