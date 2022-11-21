#include "xi_lib.h"
#define IMAGE_PAD     0
#define DST_BIT_DEPTH 32
{
  extern XI_ERR_TYPE xiMatrixMultiply_U8U32Sa(
      const xi_pArray src0, const xi_pArray src1, xi_pArray dst);

  int err = 0;

  int dstBytes = DST_BIT_DEPTH >> 3;
  // Source and destination tiles
  xi_tile src0_t, src1_t, dst_t;
  int in1TileW, in1TileH, in2TileW, in2TileH, outTileW, outTileH;
  int in1TilePitch, in2TilePitch, outTilePitch;

  in1TileW = widthA;
  in1TileH = 128;
  in2TileW = 128; // widthB;
  in2TileH = widthA;
  outTileW = 128; // widthB;
  outTileH = 128;
  in1TilePitch = in1TileW + 2 * IMAGE_PAD;
  in2TilePitch = in2TileW + 2 * IMAGE_PAD;
  outTilePitch = outTileW;
  // SRC
  src0_t.pBuffer = local_A;
  src0_t.pData = local_A; // + inTilePitch*IMAGE_PAD + IMAGE_PAD;
  XI_TILE_SET_BUFF_SIZE((&src0_t), in1TilePitch * (in1TileH + 2 * IMAGE_PAD));
  XI_TILE_SET_WIDTH((&src0_t), in1TileW);
  XI_TILE_SET_HEIGHT((&src0_t), in1TileH);
  XI_TILE_SET_PITCH((&src0_t), in1TilePitch);
  XI_TILE_SET_TYPE((&src0_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&src0_t), IMAGE_PAD);
  XI_TILE_SET_EDGE_HEIGHT((&src0_t), IMAGE_PAD);
  XI_TILE_SET_X_COORD((&src0_t), 0);
  XI_TILE_SET_Y_COORD((&src0_t), 0);

  // SRC
  src1_t.pBuffer = local_B;
  src1_t.pData = local_B; // + inTilePitch*IMAGE_PAD + IMAGE_PAD;
  XI_TILE_SET_BUFF_SIZE((&src1_t), in2TilePitch * (in2TileH + 2 * IMAGE_PAD));
  XI_TILE_SET_WIDTH((&src1_t), in2TileW);
  XI_TILE_SET_HEIGHT((&src1_t), in2TileH);
  XI_TILE_SET_PITCH((&src1_t), in2TilePitch);
  XI_TILE_SET_TYPE((&src1_t), XI_TILE_U8);
  XI_TILE_SET_EDGE_WIDTH((&src1_t), IMAGE_PAD);
  XI_TILE_SET_EDGE_HEIGHT((&src1_t), IMAGE_PAD);
  XI_TILE_SET_X_COORD((&src1_t), 0);
  XI_TILE_SET_Y_COORD((&src1_t), 0);

  // DST_
  dst_t.pBuffer = (__local uchar *)local_C;
  dst_t.pData = (__local uchar *)local_C;
  XI_TILE_SET_BUFF_SIZE((&dst_t), (outTilePitch * (outTileH)*dstBytes));
  XI_TILE_SET_WIDTH((&dst_t), outTileW);
  XI_TILE_SET_HEIGHT((&dst_t), outTileH);
  XI_TILE_SET_PITCH((&dst_t), (outTilePitch));
  XI_TILE_SET_TYPE((&dst_t), XI_TILE_U32);
  XI_TILE_SET_EDGE_WIDTH((&dst_t), 0);
  XI_TILE_SET_EDGE_HEIGHT((&dst_t), 0);
  XI_TILE_SET_X_COORD((&dst_t), 0);
  XI_TILE_SET_Y_COORD((&dst_t), 0);

  // IVP
  err = xiMatrixMultiply_U8U32Sa((xi_pArray)&src0_t, (xi_pArray)&src1_t,
                                 (xi_pArray)&dst_t);
}
