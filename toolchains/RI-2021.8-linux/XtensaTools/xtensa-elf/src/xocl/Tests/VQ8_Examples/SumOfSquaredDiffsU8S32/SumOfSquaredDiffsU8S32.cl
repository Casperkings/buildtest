#define TILE_W 128
#define TILE_H 64
#define W_EXT 0
#define H_EXT 0
#define IMAGE_PAD 0
#define DST_BIT_DEPTH 32

__constant int mask_lo[] __attribute__((section(".dram0.data"))) = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

__constant int mask_hi[] __attribute__((section(".dram0.data"))) = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

void reduce_add16(ushort64 vs, uint *res_lo, uint *res_hi) {
  const uint32 zero = (uint32)0;
  const uint32 mask16 = {16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 
                         29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 
                         42, 43, 44, 45, 46, 47};
  const uint32 mask8 = {8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 
                        22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 
                        35, 36, 37, 38, 39};
  const uint32 mask4 = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 
                        19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 
                        32, 33, 34, 35};
  const uint32 mask2 = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
                        18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
                        32, 33};
  const uint32 mask1 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                        17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                        31, 32};

  uint64 v = convert_uint64(vs);
  uint32 v16_lo_0 = v.lo;
  uint32 v16_lo_1 = shuffle2(v.lo, zero, mask16);
  uint32 v16_hi_0 = v.hi;
  uint32 v16_hi_1 = shuffle2(v.hi, zero, mask16);

  uint32 v16_lo = v16_lo_0 + v16_lo_1;
  uint32 v16_hi = v16_hi_0 + v16_hi_1;

  uint32 v8_lo_0 = v16_lo;
  uint32 v8_lo_1 = shuffle2(v16_lo, zero, mask8);
  uint32 v8_hi_0 = v16_hi;
  uint32 v8_hi_1 = shuffle2(v16_hi, zero, mask8);

  uint32 v8_lo = v8_lo_0 + v8_lo_1;
  uint32 v8_hi = v8_hi_0 + v8_hi_1;

  uint32 v4_lo_0 = v8_lo;
  uint32 v4_lo_1 = shuffle2(v8_lo, zero, mask4);
  uint32 v4_hi_0 = v8_hi;
  uint32 v4_hi_1 = shuffle2(v8_hi, zero, mask4);

  uint32 v4_lo = v4_lo_0 + v4_lo_1;
  uint32 v4_hi = v4_hi_0 + v4_hi_1;

  uint32 v2_lo_0 = v4_lo;
  uint32 v2_lo_1 = shuffle2(v4_lo, zero, mask2);
  uint32 v2_hi_0 = v4_hi;
  uint32 v2_hi_1 = shuffle2(v4_hi, zero, mask2);

  uint32 v2_lo = v2_lo_0 + v2_lo_1;
  uint32 v2_hi = v2_hi_0 + v2_hi_1;

  uint32 v1_lo_0 = v2_lo;
  uint32 v1_lo_1 = shuffle2(v2_lo, zero, mask1);
  uint32 v1_hi_0 = v2_hi;
  uint32 v1_hi_1 = shuffle2(v2_hi, zero, mask1);

  uint32 v1_lo = v1_lo_0 + v1_lo_1;
  uint32 v1_hi = v1_hi_0 + v1_hi_1;

  *res_lo = v1_lo[0];
  *res_hi = v1_hi[0];
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernel(__global uchar *restrict Src, __global uchar *restrict Src1,
          int SrcPitch, __global uint *restrict Dst, const int DstPitch,
          int width, int height, int dstWidth, int dstHeight,
          __local uchar *restrict local_src, __local uchar *restrict local_src1,
          __local uint *restrict local_dst, __global int *err_codes) {
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

  outTileW = TILE_H;      // TILE_W/32;
  outTileH = TILE_W / 32; // TILE_H;
  local_dst_pitch = outTileW;
  dstBytes = DST_BIT_DEPTH >> 3;

  i = get_global_id(0) * num_rows;
  j = get_global_id(1) * row_size;
  int i_out = get_global_id(1) * outTileH;
  int j_out = get_global_id(0) * outTileW;
  int once = 1;
  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch,
      (num_rows + pad + pad), local_src_pitch, SrcPitch, 0);

  event_t evtB = xt_async_work_group_2d_copy(
      local_src1, Src1 + j, local_src_pitch, 1, local_src_pitch, SrcPitch, 0);
  wait_group_events(1, &evtA);
  wait_group_events(1, &evtB);
#if NATIVE_KERNEL
#include "SumOfSquaredDiffsU8S32_ivp.c"
#elif XT_OCL_EXT
#include "SumOfSquaredDiffsU8S32_ext.cl"
#else
  // kernel using standard OpenCL C

  __local uchar *pSrc1 = local_src1;
#pragma nounroll
  for (int y = 0; y < inTileH; y += 64) {
    __local uchar *pSrc = local_src + y * local_src_pitch;
    __local int *pDst = (__local int *)local_dst + y;
#pragma nounroll
    for (idx = 0; idx < inTileW; idx += 64) {
      short64 xT = convert_short64(*((__local uchar64 *)(pSrc1 + idx)));
      __local int *pDst_tmp = pDst;
      for (int x = 0; x < 64; ++x) {
        short64 xS = convert_short64(
                       *((__local uchar64 *)(pSrc + x*inTilePitch + idx)));
        ushort64 xS_T = abs_diff(xS, xT);
        ushort64 sqDiff = xS_T * xS_T;
        uint res_lo, res_hi;
        reduce_add16(sqDiff, &res_lo, &res_hi);
        *pDst_tmp = res_lo;
        *(pDst_tmp + local_dst_pitch) = res_hi;
        pDst_tmp++;
      }
      pDst += 2*local_dst_pitch;
    }
  }

#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (i_out * DstPitch) + j_out, local_dst, outTileW * dstBytes,
      outTileH, DstPitch * dstBytes, local_dst_pitch * dstBytes, 0);

  wait_group_events(1, &evtD);
}
