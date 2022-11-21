#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W 128
#define TILE_H 64
#define W_EXT 0
#define H_EXT 0
#define IMAGE_PAD 0
#define OUT_TILE_W (64)
#define OUT_TILE_H (35)

#define XI_Q13_18 int
#define XI_Q15_16 int

__constant int kxintseq[] __attribute__((section(".dram0.data"))) = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
  46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
  61, 62, 63
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
  int32 xintseq = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                   11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                   22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

  int xstep = max(1, min(1 * 32, (int)(((2 * 32) << 18) / (xscale))));

  xintseq += (int32)xd;

  int32 wx = xt_mul((int32)xscale, xintseq);

  int min = xd * xscale;
  XI_Q13_18 offset =
      ((xshift - ((xs) << 18) + (xscale >> 1) - (1 << 17))) - (xscale * xstep);

  wx += (int32)offset;
  min += offset;

  XI_Q13_18 xoffset0 = (xscale * xstep);

  // kernel using standard OpenCL C
  for (idx = 0; idx < outTileW; idx += xstep) {
    XI_Q13_18 ysrc =
        (yd * yscale + yshift - (ys << 18) + (yscale >> 1) - (1 << 17)) +
        4; // +4 for yf calculation round in cycle
    wx += (int32)xoffset0;
    min += xoffset0;
    int32 x = wx >> 18;

    short32 xf = convert_short32((wx & (int32)((1 << 18) - 1)) >> 3);
    short32 one_xf = ((short32)(1 << 15) - (xf));
    x -= (int32)(min >> 18);
    ushort32 x0sh = convert_ushort32(x);

    ushort32 x0nsh = x0sh + (ushort32)1;
    __local uchar *src0 = local_src + (min >> 18);
    __local uchar *dst = local_dst + idx;
    for (int y = 0; y < outTileH; ++y) {
      __local uchar *src00 = src0 + ((ysrc >> 18) * local_src_pitch);
      __local uchar *src1 = src00 + local_src_pitch;
      short yf = (ysrc & ((1 << 18) - 1)) >> 3;
      short32 x00 = convert_short32(vload32(0, src00));
      short32 x01 = convert_short32(vload32(0, src00 + 32));
      short32 x10 = convert_short32(vload32(0, src1));
      short32 x11 = convert_short32(vload32(0, src1 + 32));

      short32 dst00 = shuffle2(x00, x01, (x0sh));
      short32 dst01 = shuffle2(x00, x01, (x0nsh));
      short32 dst10 = shuffle2(x10, x11, (x0sh));
      short32 dst11 = shuffle2(x10, x11, (x0nsh));

      int32 dst00_int;
      dst00_int = xt_mul32(dst00, (short32)((1 << 15) - yf));
      dst00_int += xt_mul32(dst10, (short32)yf);
      dst00_int += (int32)(1 << 14);
      dst00_int >>= 15;
      dst00 = convert_short32(dst00_int);
      // int32 dst00 = ((int32)dst00 * (int32)((1 << 15) - yf) + (int32)dst10 *
      // (int32)yf + (int32)(1 << 14)) >> 15;

      dst00_int = xt_mul32(dst01, (short32)((1 << 15) - yf));
      dst00_int += xt_mul32(dst11, (short32)(yf));
      dst00_int += (int32)(1 << 14);
      dst00_int >>= 15;
      dst01 = convert_short32(dst00_int);
      // int32 dst01 = ((int32)dst01 * (int32)((1 << 15) - yf) + (int32)dst11 *
      // (int32)yf + (int32)(1 << 14)) >> 15;

      dst00_int = xt_mul32(dst00, one_xf);
      dst00_int += xt_mul32(dst01, xf);
      dst00_int += (int32)(1 << 14);
      dst00_int >>= 15;
      dst00 = convert_short32(dst00_int);
      // dst00 = (dst00 * ((1 << 15) - xf) + dst01 * xf + (1 << 14)) >> 15;

      vstore32(convert_uchar32(dst00), 0, dst);
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
