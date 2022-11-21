#define TILE_W 128
#define TILE_H 64
#define W_EXT 0
#define H_EXT 0
#define IMAGE_PAD 0
#define DST_BIT_DEPTH 32

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernelRowTiled(__global uchar *restrict Src, int SrcPitch,
                  __global ushort *restrict Dst, const int DstPitch, int width,
                  int height, int dstWidth, int dstHeight,
                  __local uchar *restrict local_src,
                  __local ushort *restrict local_dst) {
  int i, j;
  int idx, idy;
  int num_rows = TILE_H;
  int row_size = TILE_W;
  int local_src_pitch, local_dst_pitch;
  int pad = IMAGE_PAD;
  int inTilePitch;
  int inTileW, inTileH, outTileW, outTileH;
  int dstBytes;
  int err = 0;

  inTileW = TILE_W;
  inTileH = TILE_H;
  inTilePitch = inTileW + 2 * W_EXT;
  local_src_pitch = row_size + pad + pad;

  outTileW = TILE_W;
  outTileH = TILE_H;
  local_dst_pitch = outTileW; // + 2*W_EXT;
  dstBytes = 2;

  i = get_global_id(0) * num_rows;
  j = get_global_id(1) * row_size;
  int i_out = get_global_id(0) * outTileH;
  int j_out = get_global_id(1) * outTileW;

  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch,
      (num_rows + pad + pad), local_src_pitch, SrcPitch, 0);
  wait_group_events(1, &evtA);

#if NATIVE_KERNEL
#include "IntegralU8U32_ext.cl"
#elif XT_OCL_EXT
#include "IntegralU8U32_ext.cl"
#else
  // kernel using standard OpenCL C

  const ushort64 RotateLeft1 = {63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74,
                                75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
                                87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,
                                99, 100, 101, 102, 103, 104, 105, 106, 107,
                                108, 109, 110, 111, 112, 113, 114, 115, 116, 
                                117, 118, 119, 120, 121, 122, 123, 124, 125,
                                126};
  const ushort64 RotateLeft2 = {62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73,
                                74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85,
                                86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97,
                                98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
                                108, 109, 110, 111, 112, 113, 114, 115, 116,
                                117, 118, 119, 120, 121, 122, 123, 124, 125};
  const ushort64 RotateLeft3 = {61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72,
                                73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
                                85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
                                97, 98, 99, 100, 101, 102, 103, 104, 105, 106,
                                107, 108, 109, 110, 111, 112, 113, 114, 115, 
                                116, 117, 118, 119, 120, 121, 122, 123, 124};
  const ushort64 RotateLeft4 = {60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
                                72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83,
                                84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
                                96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 
                                106, 107, 108, 109, 110, 111, 112, 113, 114, 
                                115, 116, 117, 118, 119, 120, 121, 122, 123};
  const ushort64 RotateLeft8 = {56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
                                68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
                                80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
                                92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 
                                103, 104, 105, 106, 107, 108, 109, 110, 111, 
                                112, 113, 114, 115, 116, 117, 118, 119};
  const ushort64 RotateLeft16 = RotateLeft8 - (ushort64)8;
  const ushort64 RotateLeft32 = RotateLeft16 - (ushort64)16;

  {
    __local uchar *pSrc = local_src;
    __local ushort *pDst = local_dst;

    for (int y = 0; y < inTileH; y++) {
      ushort64 x00 = convert_ushort64(vload64(0, pSrc));
      pSrc += local_src_pitch;
      ushort64 x01 = shuffle2((ushort64)0, x00, RotateLeft1);
      ushort64 x02 = shuffle2((ushort64)0, x00, RotateLeft2);
      ushort64 x03 = shuffle2((ushort64)0, x00, RotateLeft3);

      x03 = x00 + x01 + x02 + x03;

      ushort64 x04 = shuffle2((ushort64)0, x03, RotateLeft4) + x03;
      ushort64 x05 = shuffle2((ushort64)0, x04, RotateLeft8) + x04;
      ushort64 x06 = shuffle2((ushort64)0, x05, RotateLeft16) + x05;
      ushort64 x07 = shuffle2((ushort64)0, x06, RotateLeft32) + x06;

      vstore64(x07, 0, pDst);
      pDst += local_dst_pitch;
    }
  }

  for (idx = 64; idx < inTileW; idx += 64) {
    __local uchar *pSrc = local_src + idx;
    __local ushort *pDst = local_dst + idx;

    for (int y = 0; y < inTileH; y += 1) {
      ushort64 x00 = convert_ushort64(vload64(0, pSrc));
      pSrc += local_src_pitch;
      ushort64 x01 = shuffle2((ushort64)0, x00, RotateLeft1);
      ushort64 x02 = shuffle2((ushort64)0, x00, RotateLeft2);
      ushort64 x03 = shuffle2((ushort64)0, x00, RotateLeft3);

      x03 = x00 + x01 + x02 + x03;

      ushort64 x04 = shuffle2((ushort64)0, x03, RotateLeft4) + x03;
      ushort64 x05 = shuffle2((ushort64)0, x04, RotateLeft8) + x04;
      ushort64 x06 = shuffle2((ushort64)0, x05, RotateLeft16) + x05;
      ushort64 x07 = shuffle2((ushort64)0, x06, RotateLeft32) + x06;

      ushort64 Out = x07 + (ushort64)(pDst[-1]);
      vstore64(Out, 0, pDst);
      pDst += local_dst_pitch;
    }
  }
#endif

  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (i * DstPitch) + j, local_dst, outTileW * dstBytes, outTileH,
      DstPitch * dstBytes, local_dst_pitch * dstBytes, 0);

  wait_group_events(1, &evtD);
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernelLastColSums(__global ushort *restrict Src, int SrcPitch,
                     __global uint *restrict Dst, const int DstPitch, int width,
                     int height, int dstWidth, int dstHeight,
                     __local ushort *restrict local_src,
                     __local uint *restrict local_dst) {
  int i, j;
  int idx, idy;
  int num_rows = TILE_H;
  int row_size = TILE_W;
  int local_src_pitch, local_dst_pitch;
  int pad = IMAGE_PAD;
  int inTilePitch;
  int inTileW, inTileH, outTileW, outTileH;
  int dstBytes;
  int srcBytes;

  inTileW = width / TILE_W;
  inTileH = height;
  inTilePitch = inTileW + 2 * W_EXT;
  local_src_pitch = width / TILE_W;
  srcBytes = 2;

  outTileW = width / TILE_W;
  outTileH = height;
  local_dst_pitch = outTileW;
  dstBytes = 4;

  event_t evtA = xt_async_work_group_2d_copy(local_src, Src + TILE_W - 1,
                                             srcBytes, (inTileH)*width / TILE_W,
                                             srcBytes, TILE_W * srcBytes, 0);

  wait_group_events(1, &evtA);
//#if NATIVE_KERNEL
//#include "IntegralU8U32_ivp.c"
//#else
  // kernel using standard OpenCL C

  for (int y = 0; y < inTileH; y++) {
    local_dst[y * outTileW] = 0;
    for (idx = 1; idx < inTileW; idx += 1) {
      local_dst[y * outTileW + idx] =
          local_dst[y * outTileW + idx - 1] + local_src[y * outTileW + idx - 1];
    }
  }
//#endif
  event_t evtD =
      xt_async_work_group_2d_copy(Dst, local_dst, outTileW * dstBytes, outTileH,
                                  outTileW * dstBytes, outTileW * dstBytes, 0);

  wait_group_events(1, &evtD);
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernelRowFull(__global ushort *restrict Src,
                 __global uint *restrict SrcLastCol, int SrcPitch,
                 __global uint *restrict Dst, const int DstPitch, int width,
                 int height, int dstWidth, int dstHeight,
                 __local ushort *restrict local_src,
                 __local uint *restrict local_src_lastCol,
                 __local uint *restrict local_dst) {
  int i, j;
  int idx, idy;
  int num_rows = TILE_H;
  int row_size = TILE_W;
  int local_src_pitch, local_dst_pitch;
  int pad = IMAGE_PAD;
  int inTilePitch;
  int inTileW, inTileH, outTileW, outTileH;
  int dstBytes;
  int srcBytes;
  int srcBytes_lastCol;
  int err = 0;

  inTileW = TILE_W;
  inTileH = TILE_H;
  inTilePitch = inTileW + 2 * W_EXT;
  local_src_pitch = row_size + pad + pad;
  srcBytes = 2;
  srcBytes_lastCol = 4;
  outTileW = TILE_W;
  outTileH = TILE_H;
  local_dst_pitch = outTileW; // + 2*W_EXT;
  dstBytes = 4;

  i = get_global_id(0) * num_rows;
  j = get_global_id(1) * row_size;
  int i_out = get_global_id(0) * outTileH;
  int j_out = get_global_id(1) * outTileW;

  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch * srcBytes,
      (num_rows + pad + pad), local_src_pitch * srcBytes, SrcPitch * srcBytes,
      0);

  event_t evtB = xt_async_work_group_2d_copy(
      local_src_lastCol,
      SrcLastCol +
          (get_global_id(0) * inTileH * (width / TILE_W) + get_global_id(1)),
      srcBytes_lastCol, (num_rows + pad + pad), srcBytes_lastCol,
      (width / TILE_W) * srcBytes_lastCol, 0);
  wait_group_events(1, &evtA);
  wait_group_events(1, &evtB);

//#if NATIVE_KERNEL
//#include "IntegralU8U32_ivp.c"
//#else
  // kernel using standard OpenCL C

  for (int y = 0; y < inTileH; y++) {
    __local ushort *pSrc = local_src + (y * local_src_pitch);
    uint64 LastRowSum = local_src_lastCol[y];
    __local uint *pDst = local_dst + (y * local_src_pitch);
    for (idx = 0; idx < inTileW; idx += 64) {
      uint64 x00 = convert_uint64(vload64(0, pSrc));
      pSrc += 64;
      x00 += LastRowSum;
      vstore64(x00, 0, pDst);
      pDst += 64;
    }
  }
//#endif

  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (i * DstPitch) + j, local_dst, outTileW * dstBytes, outTileH,
      DstPitch * dstBytes, local_dst_pitch * dstBytes, 0);
  wait_group_events(1, &evtD);
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernelColTiled(__global uint *restrict Src, int SrcPitch,
                  __global uint *restrict Dst, const int DstPitch, int width,
                  int height, int dstWidth, int dstHeight,
                  __local uint *restrict local_src,
                  __local uint *restrict local_dst) {
  int i, j;
  int idx, idy;
  int num_rows = TILE_H;
  int row_size = TILE_W;
  int local_src_pitch, local_dst_pitch;
  int pad = IMAGE_PAD;
  int inTilePitch;
  int inTileW, inTileH, outTileW, outTileH;
  int dstBytes;
  int srcBytes;
  int err = 0;

  inTileW = TILE_W;
  inTileH = TILE_H;
  inTilePitch = inTileW + 2 * W_EXT;
  local_src_pitch = row_size + pad + pad;
  srcBytes = 4;

  outTileW = TILE_W;
  outTileH = TILE_H;
  local_dst_pitch = outTileW; // + 2*W_EXT;
  dstBytes = 4;

  i = get_global_id(0) * num_rows;
  j = get_global_id(1) * row_size;
  int i_out = get_global_id(0) * outTileH;
  int j_out = get_global_id(1) * outTileW;

  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch * srcBytes,
      (num_rows + pad + pad), local_src_pitch * srcBytes, SrcPitch * srcBytes,
      0);
  wait_group_events(1, &evtA);

//#if NATIVE_KERNEL
//#include "IntegralU8U32_ivp.c"
//#else
  // kernel using standard OpenCL C

  for (idx = 0; idx < inTileW; idx += 64) {
    __local uint *pSrc = local_src + idx;
    __local uint *pDst = local_dst + idx;
    uint64 Sum = 0;
    for (int y = 0; y < inTileH; y++) {
      uint64 x00 = vload64(0, pSrc);
      Sum += x00;
      pSrc += local_src_pitch;

      vstore64(Sum, 0, pDst);
      pDst += local_dst_pitch;
    }
  }
//#endif

  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (i * DstPitch) + j, local_dst, outTileW * dstBytes, outTileH,
      DstPitch * dstBytes, local_dst_pitch * dstBytes, 0);
  wait_group_events(1, &evtD);
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernelLastRowSums(__global uint *restrict Src, int SrcPitch,
                     __global uint *restrict Dst, const int DstPitch, int width,
                     int height, int dstWidth, int dstHeight,
                     __local uint *restrict local_src,
                     __local uint *restrict local_dst) {
  int i, j;
  int idx, idy;
  int num_rows = TILE_H;
  int row_size = TILE_W;
  int local_src_pitch, local_dst_pitch;
  int pad = IMAGE_PAD;
  int inTilePitch;
  int inTileW, inTileH, outTileW, outTileH;
  int dstBytes;
  int srcBytes;

  inTileW = width;
  inTileH = height / TILE_H;
  inTilePitch = inTileW + 2 * W_EXT;
  local_src_pitch = width;
  srcBytes = 4;

  outTileW = width;
  outTileH = height / TILE_H;
  local_dst_pitch = outTileW;
  dstBytes = 4;

  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (TILE_H - 1) * SrcPitch, inTileW * srcBytes, inTileH,
      inTileW * srcBytes, SrcPitch * TILE_H * srcBytes, 0);
  wait_group_events(1, &evtA);

//#if NATIVE_KERNEL
//#include "IntegralU8U32_ivp.c"
//#else
  // kernel using standard OpenCL C

  for (idx = 0; idx < inTileW; idx += 64) {
    __local uint *pSrc = local_src + idx;
    __local uint *pDst = local_dst + idx;
    uint64 Sum = 0;
    for (int y = 0; y < inTileH; y++) {
      vstore64(Sum, 0, pDst);
      pDst += local_dst_pitch;
      uint64 x00 = vload64(0, pSrc);
      Sum += x00;
      pSrc += local_src_pitch;
    }
  }
//#endif

  event_t evtD =
      xt_async_work_group_2d_copy(Dst, local_dst, outTileW * dstBytes, outTileH,
                                  outTileW * dstBytes, outTileW * dstBytes, 0);
  wait_group_events(1, &evtD);
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernelColFull(__global uint *restrict Src,
                 __global uint *restrict SrcLastRow, int SrcPitch,
                 __global uint *restrict Dst, const int DstPitch, int width,
                 int height, int dstWidth, int dstHeight,
                 __local uint *restrict local_src,
                 __local uint *restrict local_src_lastRow,
                 __local uint *restrict local_dst) {
  int i, j;
  int idx, idy;
  int num_rows = TILE_H;
  int row_size = TILE_W;
  int local_src_pitch, local_dst_pitch;
  int pad = IMAGE_PAD;
  int inTilePitch;
  int inTileW, inTileH, outTileW, outTileH;
  int dstBytes;
  int srcBytes;
  int err = 0;

  inTileW = TILE_W;
  inTileH = TILE_H;
  inTilePitch = inTileW + 2 * W_EXT;
  local_src_pitch = row_size + pad + pad;
  srcBytes = 4;
  outTileW = TILE_W;
  outTileH = TILE_H;
  local_dst_pitch = outTileW; // + 2*W_EXT;
  dstBytes = 4;

  i = get_global_id(0) * num_rows;
  j = get_global_id(1) * row_size;
  int i_out = get_global_id(0) * outTileH;
  int j_out = get_global_id(1) * outTileW;

  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch * srcBytes,
      (num_rows + pad + pad), local_src_pitch * srcBytes, SrcPitch * srcBytes,
      0);
  event_t evtB = xt_async_work_group_2d_copy(
      local_src_lastRow,
      SrcLastRow + (get_global_id(0) * (width) + get_global_id(1) * TILE_W),
      TILE_W * srcBytes, 1, TILE_W * srcBytes, width * srcBytes, 0);
  wait_group_events(1, &evtA);
  wait_group_events(1, &evtB);

//#if NATIVE_KERNEL
//#include "IntegralU8U32_ivp.c"
//#else
  // kernel using standard OpenCL C

  for (idx = 0; idx < inTileW; idx += 64) {
    __local uint *pSrc = local_src + idx;
    uint64 LastRowSum = vload64(0, local_src_lastRow + idx);
    __local uint *pDst = local_dst + idx;
    for (int y = 0; y < inTileH; y++) {
      uint64 x00 = vload64(0, pSrc);
      pSrc += local_src_pitch;
      x00 += LastRowSum;
      vstore64(x00, 0, pDst);
      pDst += local_dst_pitch;
    }
  }
//#endif

  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (i * DstPitch) + j, local_dst, outTileW * dstBytes, outTileH,
      DstPitch * dstBytes, local_dst_pitch * dstBytes, 0);
  wait_group_events(1, &evtD);
}
