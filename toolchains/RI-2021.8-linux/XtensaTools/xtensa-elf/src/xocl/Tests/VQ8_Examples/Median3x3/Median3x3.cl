#define TILE_W 256
#define TILE_H 64
#define W_EXT 1
#define H_EXT 1
#define IMAGE_PAD 1
#define DST_BIT_DEPTH 8
#define SRC_BIT_DEPTH 8

void sort_pix_8U2(uchar128 *p0, uchar128 *p1) {
  uchar128 min_res;
  uchar128 max_res;
  min_res = min(*p0, *p1);
  max_res = max(*p0, *p1);
  *p1 = max_res;
  *p0 = min_res;
}

void sort_row_8U2(uchar128 *v0, uchar128 *v1, uchar128 *v2) {
  sort_pix_8U2(v1, v2);
  sort_pix_8U2(v0, v1);
  sort_pix_8U2(v1, v2);
}

uchar128 get_median_8U2(uchar128 v0, uchar128 v1, uchar128 v2, uchar128 v3,
                        uchar128 v4, uchar128 v5, uchar128 v6, uchar128 v7,
                        uchar128 v8) {
  uchar128 vtmp;
  uchar128 vmed;

  vtmp = max(v0, v3);
  v0 = max(v4, v7);
  vmed = min(v4, v7);
  vmed = max(v1, vmed);
  v1 = min(v5, v8);
  v2 = min(v2, v1);
  vmed = min(vmed, v0);
  v1 = min(vmed, v2);
  v2 = max(vmed, v2);
  vtmp = max(vtmp, v6);
  vmed = max(vtmp, v1);
  vmed = min(vmed, v2);

  return vmed;
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernel(__global uchar *restrict Src, int SrcPitch,
          __global uchar *restrict Dst, const int DstPitch, int width,
          int height, int dstWidth, int dstHeight,
          __local uchar *restrict local_src, __local uchar *restrict local_dst,
          __global int *restrict err_codes) {
  int i, j;
  int idx, idy;
  int num_rows = TILE_H;
  int row_size = TILE_W;
  int local_src_pitch, local_dst_pitch;
  int pad = IMAGE_PAD;
  int inTilePitch;
  int inTileW, inTileH, outTileW, outTileH;
  int dstBytes, srcBytes;
  int err = 0;

  inTileW = TILE_W;
  inTileH = TILE_H;
  inTilePitch = inTileW + 2 * W_EXT;
  local_src_pitch = row_size + pad + pad;

  outTileW = TILE_W;
  outTileH = TILE_H;
  local_dst_pitch = outTileW; // + 2*W_EXT;
  dstBytes = DST_BIT_DEPTH >> 3;
  srcBytes = SRC_BIT_DEPTH >> 3;

  i = get_global_id(0) * num_rows;
  j = get_global_id(1) * row_size;

  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch,
      (num_rows + pad + pad), local_src_pitch, SrcPitch, 0);

  wait_group_events(1, &evtA);

#if NATIVE_KERNEL
  #include "Median3x3_ivp.c"
#else
  // kernel using standard OpenCL C
  const uchar128 maskshift1 = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
    97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125,
    126, 127, 128};
  const uchar128 maskshift2 = {
    2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
    34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
    66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
    97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125,
    126, 127, 128, 129};

  for (idx = 0; idx < row_size; idx += 128) {
    int y = 0;
    __local uchar *pSrc0 = local_src + y * local_src_pitch + idx;
    uchar128 x00 = (vload128(0, pSrc0));
    uchar128 xtail = (vload128(0, pSrc0 + 128));
    uchar128 x01 = shuffle2(x00, xtail, maskshift1);
    uchar128 x02 = shuffle2(x00, xtail, maskshift2);
    sort_row_8U2(&x00, &x01, &x02);

    __local uchar *pSrc1 = local_src + (y + 1) * local_src_pitch + idx;

    uchar128 x10 = (vload128(0, pSrc1));
    xtail = (vload128(0, pSrc1 + 128));
    uchar128 x11 = shuffle2(x10, xtail, maskshift1);
    uchar128 x12 = shuffle2(x10, xtail, maskshift2);
    sort_row_8U2(&x10, &x11, &x12);

    __local uchar *pSrc2 = local_src + (y + 2) * local_src_pitch + idx;
    __local uchar *pDst = local_dst + idx;
    for (int y = 0; y < num_rows; ++y) {
      uchar128 x20 = (vload128(0, pSrc2));
      uchar128 xtail = (vload128(0, pSrc2 + 128));
      pSrc2 += local_src_pitch;
      uchar128 x21 = shuffle2(x20, xtail, maskshift1);
      uchar128 x22 = shuffle2(x20, xtail, maskshift2);

      sort_row_8U2(&x20, &x21, &x22);

      uchar128 xmed;
      xmed = get_median_8U2(x00, x01, x02, x10, x11, x12, x20, x21, x22);

      vstore128(xmed, 0, pDst);
      pDst += local_dst_pitch;

      x00 = x10;
      x01 = x11;
      x02 = x12;
      x10 = x20;
      x11 = x21;
      x12 = x22;
    }
  }
#endif

  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (i * DstPitch) + j, local_dst, outTileW * dstBytes, num_rows,
      DstPitch * dstBytes, local_dst_pitch * dstBytes, 0);

  wait_group_events(1, &evtD);
}
