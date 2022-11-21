#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W 256
#define TILE_H 64
#define W_EXT 2
#define H_EXT 2
#define IMAGE_PAD 2
#define OUT_TILE_W (128 * 3)
#define OUT_TILE_H (35 * 3)

#define XI_Q13_18 int
#define XI_Q15_16 int

__constant int kxintseq[] __attribute__((section(".dram0.data"))) = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
  46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
  61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 
  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113,
  114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127
};

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernel(__global uchar *restrict Src, int SrcPitch,
          __global uchar *restrict Dst, const int DstPitch, int width,
          int height, int dstWidth, int dstHeight, unsigned int xscale,
          unsigned int yscale, unsigned int xshift, unsigned int yshift,
          __local uchar *restrict local_src, __local uchar *restrict local_dst,
          __global int *restrict err_codes) {
  int i, j;
  int out_i, out_j;
  int idx, idy;
  int local_src_pitch, local_dst_pitch;
  int pad = IMAGE_PAD;
  int inTilePitch;
  int inTileW, inTileH, outTileW, outTileH;
  int dstBytes;
  int err = 0;

  inTileW = TILE_W;
  inTileH = TILE_H;
  inTilePitch = inTileW + 2 * W_EXT;
  local_src_pitch = inTileW + pad + pad;

  outTileW = OUT_TILE_W;
  outTileH = OUT_TILE_H;
  local_dst_pitch = outTileW;
  dstBytes = DST_BIT_DEPTH >> 3;

  i = get_global_id(0) * inTileH;
  j = get_global_id(1) * inTileW;

  out_i = get_global_id(0) * outTileH;
  out_j = get_global_id(1) * outTileW;

  int xs = j - W_EXT;
  int ys = i - H_EXT;
  int xd = out_j;
  int yd = out_i;

  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch,
      (inTileH + pad + pad), local_src_pitch, SrcPitch, 0);
  wait_group_events(1, &evtA);
#if NATIVE_KERNEL
  #include "Bilinear_ivp.c"
#elif XT_OCL_EXT
  #include "Bilinear_ext.cl"
#else
  int64 xintseq = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                   11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                   22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 
                   33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 
                   44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
                   55, 56, 57, 58, 59, 60, 61, 62, 63};

  int xstep = max(1, min(1 * 64, (int)(((2 * 64) << 18) / (xscale))));

  xintseq += (int64)xd;

  int64 wx = xt_mul((int64)xscale, xintseq);

  int min = xd * xscale;
  XI_Q13_18 offset =
      ((xshift - ((xs) << 18) + (xscale >> 1) - (1 << 17))) - (xscale * xstep);

  wx += (int64)offset;
  min += offset;

  XI_Q13_18 xoffset0 = (xscale * xstep);

  // kernel using standard OpenCL C
  for (idx = 0; idx < outTileW; idx += xstep) {
    XI_Q13_18 ysrc =
        (yd * yscale + yshift - (ys << 18) + (yscale >> 1) - (1 << 17)) +
        4; // +4 for yf calculation round in cycle
    wx += (int64)xoffset0;
    min += xoffset0;
    int64 x = wx >> 18;

    short64 xf = convert_short64((wx & (int64)((1 << 18) - 1)) >> 3);
    short64 one_xf = ((short64)(1 << 15) - (xf));
    x -= (int64)(min >> 18);
    ushort64 x0sh = convert_ushort64(x);

    ushort64 x0nsh = x0sh + (ushort64)1;
    __local uchar *src0 = local_src + (min >> 18);
    __local uchar *dst = local_dst + idx;
    for (int y = 0; y < outTileH; ++y) {
      __local uchar *src00 = src0 + ((ysrc >> 18) * local_src_pitch);
      __local uchar *src1 = src00 + local_src_pitch;
      short yf = (ysrc & ((1 << 18) - 1)) >> 3;
      short64 x00 = convert_short64(vload64(0, src00));
      short64 x01 = convert_short64(vload64(0, src00 + 64));
      short64 x10 = convert_short64(vload64(0, src1));
      short64 x11 = convert_short64(vload64(0, src1 + 64));

      short64 dst00 = shuffle2(x00, x01, (x0sh));
      short64 dst01 = shuffle2(x00, x01, (x0nsh));
      short64 dst10 = shuffle2(x10, x11, (x0sh));
      short64 dst11 = shuffle2(x10, x11, (x0nsh));

      int64 dst00_int;
      dst00_int = xt_mul32(dst00, (short64)((1 << 15) - yf));
      dst00_int += xt_mul32(dst10, (short64)yf);
      dst00_int += (int64)(1 << 14);
      dst00_int >>= 15;
      dst00 = convert_short64(dst00_int);
      // int64 dst00 = ((int64)dst00 * (int64)((1 << 15) - yf) + (int64)dst10 *
      // (int64)yf + (int64)(1 << 14)) >> 15;

      dst00_int = xt_mul32(dst01, (short64)((1 << 15) - yf));
      dst00_int += xt_mul32(dst11, (short64)(yf));
      dst00_int += (int64)(1 << 14);
      dst00_int >>= 15;
      dst01 = convert_short64(dst00_int);
      // int64 dst01 = ((int64)dst01 * (int64)((1 << 15) - yf) + (int64)dst11 *
      // (int64)yf + (int64)(1 << 14)) >> 15;

      dst00_int = xt_mul32(dst00, one_xf);
      dst00_int += xt_mul32(dst01, xf);
      dst00_int += (int64)(1 << 14);
      dst00_int >>= 15;
      dst00 = convert_short64(dst00_int);
      // dst00 = (dst00 * ((1 << 15) - xf) + dst01 * xf + (1 << 14)) >> 15;

      vstore64(convert_uchar64(dst00), 0, dst);
      dst += outTileW;
      ysrc += yscale;
    }
  }
#endif
  event_t evtD =
      xt_async_work_group_2d_copy(Dst + (out_i * DstPitch) + out_j, // dst
                                  local_dst,                        // src
                                  outTileW * dstBytes,              // width
                                  outTileH,                         // height
                                  DstPitch * dstBytes,              // dstpitch
                                  local_dst_pitch * dstBytes,       // src pitch
                                  0);

  wait_group_events(1, &evtD);
}
