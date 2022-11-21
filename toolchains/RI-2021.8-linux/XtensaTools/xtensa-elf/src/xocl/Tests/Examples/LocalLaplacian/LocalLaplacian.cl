
#if NATIVE_KERNEL
#include "xi_lib.h"
#endif

__constant ushort kGSelect1[] __attribute__((section(".dram0.data"))) = {
  8, 9, 10, 11, 12, 13, 14, 15, 24, 25, 26,
  27, 28, 29, 30, 31, 40, 41, 42, 43, 44, 45,
  46, 47, 56, 57, 58, 59, 60, 61, 62, 63
};

__constant ushort kGSelect2[] __attribute__((section(".dram0.data"))) = {
  8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
  19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
  30, 31, 32, 33, 34, 35, 36, 37, 38, 39
};

__constant ushort kLSelect0[] __attribute__((section(".dram0.data"))) = {
  0, 1, 2, 3, 4, 5, 6, 7, 32, 33, 34,
  35, 36, 37, 38, 39, 8, 9, 10, 11, 12, 13,
  14, 15, 40, 41, 42, 43, 44, 45, 46, 47
};

__constant ushort kPSelect0[] __attribute__((section(".dram0.data"))) = {
  0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2,
  3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21,
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31
};

__constant ushort kPSelect1[] __attribute__((section(".dram0.data"))) = {
  0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2,
  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13,
  14, 15, 16, 17, 18, 19, 20, 21, 22, 23
};

__constant ushort kPSelect2[] __attribute__((section(".dram0.data"))) = {
  8, 9, 10, 11, 12, 13, 14, 15, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31
};

__constant ushort kPSelect3[] __attribute__((section(".dram0.data"))) = {
  8, 9, 10, 11, 12, 13, 14, 15, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31
};

__constant ushort kPSelect4[] __attribute__((section(".dram0.data"))) = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
  22, 23, 16, 17, 18, 19, 20, 21, 22, 23
};

__constant ushort kPSelect5[] __attribute__((section(".dram0.data"))) = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
  22, 23, 16, 17, 18, 19, 20, 21, 22, 23
};

__constant ushort kFOffsets[] __attribute__((section(".dram0.data"))) = {
  0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80,
  88,  96,  104, 112, 120, 128, 136, 144, 152, 160, 168,
  176, 184, 192, 200, 208, 216, 224, 232, 240, 248
};

__constant ushort kCSelect0[] __attribute__((section(".dram0.data"))) = {
  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3
};

__constant ushort kCOffsets[] __attribute__((section(".dram0.data"))) = {
  0, 2, 4, 6, 8, 10, 12, 14, 0, 2, 4, 6, 8, 10, 12, 14,
  0, 2, 4, 6, 8, 10, 12, 14, 0, 2, 4, 6, 8, 10, 12, 14
};

void PadEdges_uchar(__local uchar *local_dst, int outTileW, int outTileH,
                    int outTileEdge, int local_dst_pitch, int indX, int indY,
                    int indX_size, int indY_size) {
  if (outTileEdge != 0) {
    if (indY == 0) {
      __local uchar *dstEdge = local_dst + outTileEdge;
      __local uchar *srcEdge =
          local_dst + outTileEdge + outTileEdge * local_dst_pitch;
      // PAd top row
      for (int x = 0; x < outTileW; x += 64) {
        uchar64 tmp = vload64(0, srcEdge + x);
        vstore64(tmp, 0, dstEdge + x);
      }
    }

    if (indY == indY_size) {
      // padbottom
      __local uchar *srcEdge = local_dst + outTileEdge +
                               (outTileEdge + outTileH - 1) * local_dst_pitch;
      __local uchar *dstEdge =
          local_dst + outTileEdge + (outTileEdge + outTileH) * local_dst_pitch;
      for (int x = 0; x < outTileW; x += 64) {
        uchar64 tmp = vload64(0, srcEdge + x);
        vstore64(tmp, 0, dstEdge + x);
      }
    }

    if (indX == 0) {
      // PAd left col
      __local uchar *srcEdge =
          local_dst + outTileEdge + outTileEdge * local_dst_pitch;
      __local uchar *dstEdge = local_dst + outTileEdge * local_dst_pitch;
      for (int y = 0; y < outTileH; y += 1) {
        *(dstEdge) = *(srcEdge);
        srcEdge += local_dst_pitch;
        dstEdge += local_dst_pitch;
      }
    }

    if (indX == indX_size) {
      // pad right
      __local uchar *srcEdge = local_dst + outTileEdge + outTileW - 1 +
                               outTileEdge * local_dst_pitch;
      __local uchar *dstEdge = local_dst + outTileEdge + outTileW  +
                               outTileEdge * local_dst_pitch;
      for (int y = 0; y < outTileH; y += 1) {
        *(dstEdge) = *(srcEdge);
        srcEdge += local_dst_pitch;
        dstEdge += local_dst_pitch;
      }
    }

    if (indY == 0 && indX == 0) {
      // pad left top corener
      __local uchar *srcEdge = local_dst;
      __local uchar *dstEdge = local_dst;
      *(dstEdge+0) = *(srcEdge+1);
    }

    if (indY == indY_size && indX == 0) {
      // pad left bottom corener

      __local uchar *srcEdge =
          local_dst + (outTileEdge + outTileH) * local_dst_pitch;
      __local uchar *dstEdge =
          local_dst + (outTileEdge + outTileH) * local_dst_pitch;
      *(dstEdge+0) = *(srcEdge+1);
    }

    if (indY == indY_size && indX == indX_size) {
      // pad right bottom corener
      __local uchar *srcEdge = local_dst + outTileEdge + outTileW - 1 +
                               (outTileEdge + outTileH) * local_dst_pitch;
      __local uchar *dstEdge = local_dst + outTileEdge + outTileW +
                               (outTileEdge + outTileH) * local_dst_pitch;
      *(dstEdge) = *(srcEdge);
    }

    if (indY == 0 && indX == indX_size) {
      // pad right top corener
      __local uchar *srcEdge = local_dst + outTileEdge + outTileW - 1;
      __local uchar *dstEdge = local_dst + outTileEdge + outTileW;
      *(dstEdge) = *(srcEdge);
    }
  }
}

void PadEdges_short(__local short *local_dst, int outTileW, int outTileH,
                    int outTileEdge, int local_dst_pitch, int indX, int indY,
                    int indX_size, int indY_size) {
  if (outTileEdge != 0) {
    if (indY == 0) {
      __local short *dstEdge = local_dst + outTileEdge;
      __local short *srcEdge =
          local_dst + outTileEdge + outTileEdge * local_dst_pitch;
      // PAd top row
      for (int x = 0; x < outTileW; x += 32) {
        short32 tmp = vload32(0, srcEdge + x);
        vstore32(tmp, 0, dstEdge + x);
      }
    }

    if (indY == indY_size) {
      // padbottom
      __local short *srcEdge = local_dst + outTileEdge +
                               (outTileEdge + outTileH - 1) * local_dst_pitch;
      __local short *dstEdge =
          local_dst + outTileEdge + (outTileEdge + outTileH) * local_dst_pitch;
      for (int x = 0; x < outTileW; x += 32) {
        short32 tmp = vload32(0, srcEdge + x);
        vstore32(tmp, 0, dstEdge + x);
      }
    }

     if (indX == 0) {
      // PAd left col
      __local short *srcEdge =
          local_dst + outTileEdge + outTileEdge * local_dst_pitch;
      __local short *dstEdge = local_dst + outTileEdge * local_dst_pitch;
      for (int y = 0; y < outTileH; y += 1) {
        *(dstEdge) = *(srcEdge);
        srcEdge += local_dst_pitch;
        dstEdge += local_dst_pitch;
      }
    }

    if (indX == indX_size) {
      // pad right
      __local short *srcEdge = local_dst + outTileEdge + outTileW - 1+
                               outTileEdge * local_dst_pitch;
      __local short *dstEdge = local_dst + outTileEdge + outTileW +
                               outTileEdge * local_dst_pitch;
      for (int y = 0; y < outTileH; y += 1) {
        *(dstEdge) = *(srcEdge);
        srcEdge += local_dst_pitch;
        dstEdge += local_dst_pitch;
      }
    }

    if (indY == 0 && indX == 0) {
      // pad left top corener
      __local short *srcEdge = local_dst;
      __local short *dstEdge = local_dst;
      *(dstEdge+0) = *(srcEdge+1);
    }

    if (indY == indY_size && indX == 0) {
      // pad left bottom corener
      __local short *srcEdge =
          local_dst + (outTileEdge + outTileH) * local_dst_pitch;
      __local short *dstEdge =
          local_dst + (outTileEdge + outTileH) * local_dst_pitch;
      *(dstEdge+0) = *(srcEdge+1);
    }

    if (indY == indY_size && indX == indX_size) {
      // pad right bottom corener
      __local short *srcEdge = local_dst + outTileEdge + outTileW  - 1 +
                               (outTileEdge + outTileH) * local_dst_pitch;
      __local short *dstEdge = local_dst + outTileEdge + outTileW +
                               (outTileEdge + outTileH) * local_dst_pitch;
      *(dstEdge) = *(srcEdge);
    }

    if (indY == 0 && indX == indX_size) {
      // pad right top corener
      __local short *srcEdge = local_dst + outTileEdge + outTileW - 1; 
      __local short *dstEdge = local_dst + outTileEdge + outTileW;
      *(dstEdge) = *(srcEdge);
    }
  }
}

void PadEdges_shortK(__local short *local_dst, int outTileW, int outTileH,
                     int outTileEdge, int local_dst_pitch, int dstElements,
                     int indX, int indY, int indX_size, int indY_size) {
  if (outTileEdge != 0) {
    if (indY == 0) {
      __local short *dstEdge = local_dst + outTileEdge * dstElements;
      __local short *srcEdge =
          local_dst +
          (outTileEdge + outTileEdge * local_dst_pitch) * dstElements;
      // PAd top row
      for (int x = 0; x < outTileW * dstElements; x += 32) {
        short32 tmp = vload32(0, srcEdge + x);
        vstore32(tmp, 0, dstEdge + x);
      }
    }

    if (indY == indY_size) {
      // padbottom
      __local short *srcEdge =
          local_dst +
          (outTileEdge + (outTileEdge + outTileH - 1) * local_dst_pitch) *
              dstElements;
      __local short *dstEdge =
          local_dst +
          (outTileEdge + (outTileEdge + outTileH) * local_dst_pitch) *
              dstElements;
      for (int x = 0; x < outTileW * dstElements; x += 32) {
        short32 tmp = vload32(0, srcEdge + x);
        vstore32(tmp, 0, dstEdge + x);
      }
    }

    if (indX == 0) {
      // PAd left col
      __local short *srcEdge =
          local_dst +
          (outTileEdge + outTileEdge * local_dst_pitch) * dstElements;
      __local short *dstEdge =
          local_dst + (outTileEdge * local_dst_pitch) * dstElements;
      const ushort32 select = *(__constant ushort32 *)kPSelect1;
      for (int y = 0; y < outTileH; y += 1) {
        short32 tmp = vload32(0, srcEdge);
        tmp = shuffle(tmp, select);
        vstore32(tmp, 0, dstEdge);
        srcEdge += local_dst_pitch * dstElements;
        dstEdge += local_dst_pitch * dstElements;
      }
    }

    if (indX == indX_size) {
      // pad right
      __local short *srcEdge = local_dst + (outTileEdge + outTileW - 1 +
                                            outTileEdge * local_dst_pitch) *
                                               dstElements;
      __local short *dstEdge = local_dst + (outTileEdge + outTileW - 1 +
                                            outTileEdge * local_dst_pitch) *
                                               dstElements;
      const ushort32 select = *(__constant ushort32 *)kPSelect0;
      for (int y = 0; y < outTileH; y += 1) {
        short32 tmp = vload32(0, srcEdge);
        tmp = shuffle(tmp, select);
        vstore32(tmp, 0, dstEdge);
        srcEdge += local_dst_pitch * dstElements;
        dstEdge += local_dst_pitch * dstElements;
      }
    }

    if (indY == 0 && indX == 0) {
      // pad left top corener
      __local short *srcEdge = local_dst;
      __local short *dstEdge = local_dst;
      const ushort32 select = *(__constant ushort32 *)kPSelect2;
      short32 tmp = vload32(0, srcEdge);
      tmp = shuffle(tmp, select);
      vstore32(tmp, 0, dstEdge);
    }

    if (indY == indY_size && indX == 0) {
      // pad left bottom corener

      __local short *srcEdge =
          local_dst +
          ((outTileEdge + outTileH) * local_dst_pitch) * dstElements;
      __local short *dstEdge =
          local_dst +
          ((outTileEdge + outTileH) * local_dst_pitch) * dstElements;
      const ushort32 select = *(__constant ushort32 *)kPSelect3;
      short32 tmp = vload32(0, srcEdge);
      tmp = shuffle(tmp, select);
      vstore32(tmp, 0, dstEdge);
    }

    if (indY == indY_size && indX == indX_size) {
      // pad right bottom corener
      __local short *srcEdge =
          local_dst + (outTileEdge + outTileW +
                       (outTileEdge + outTileH) * local_dst_pitch - 3) *
                          dstElements;
      __local short *dstEdge =
          local_dst + (outTileEdge + outTileW +
                       (outTileEdge + outTileH) * local_dst_pitch - 3) *
                          dstElements;
      const ushort32 select = *(__constant ushort32 *)kPSelect4;
      short32 tmp = vload32(0, srcEdge);
      tmp = shuffle(tmp, select);
      vstore32(tmp, 0, dstEdge);
    }

    if (indY == 0 && indX == indX_size) {
      // pad right top corener
      __local short *srcEdge =
          local_dst + (outTileEdge + outTileW - 3) * dstElements;
      __local short *dstEdge =
          local_dst + (outTileEdge + outTileW - 3) * dstElements;
      const ushort32 select = *(__constant ushort32 *)kPSelect5;
      short32 tmp = vload32(0, srcEdge);
      tmp = shuffle(tmp, select);
      vstore32(tmp, 0, dstEdge);
    }
  }
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
ocl_InputGaussian(__global uchar *restrict Src, int inPitch, int inEdge,
                  int inWidth, int inHeight, int inTileW, int inTileH,
                  int inTileEdge, __global uchar *restrict Dst, int outPitch,
                  int outEdge, int outWidth, int outHeight, int outTileW,
                  int outTileH, int outTileEdge,
                  __local uchar *restrict local_src,
                  __local uchar *restrict local_dst) {
  int i, j;
  int local_src_pitch, local_dst_pitch;
  local_src_pitch = (inTileW + 2 * inTileEdge);
  local_dst_pitch = (outTileW + 2 * outTileEdge);

  int dstBytes = 1;
  int srcBytes = 1;

  i = get_global_id(0) * inTileH;
  j = get_global_id(1) * inTileW;

  int out_i = get_global_id(0) * outTileH;
  int out_j = get_global_id(1) * outTileW;

  int indX = get_global_id(1);
  int indY = get_global_id(0);
  int indX_size = (get_global_size(1) - 1);
  int indY_size = (get_global_size(0) - 1);

  int LeftEdge = !(get_global_id(1)), TopEdge = !(get_global_id(0));
  int DMAoutWidth =
      outTileW + LeftEdge + ((get_global_id(1) == (get_global_size(1) - 1)));
  int DMAoutHeight =
      outTileH + TopEdge + ((get_global_id(0) == (get_global_size(0) - 1)));

  event_t evtA = xt_async_work_group_2d_copy(
      local_src,
      (Src + ((inEdge * inPitch) + inEdge + (i * inPitch) + j - inTileEdge -
              (inTileEdge * inPitch))),
      local_src_pitch * srcBytes, (inTileH + 2 * inTileEdge),
      local_src_pitch * srcBytes, inPitch * srcBytes, 0);

  wait_group_events(1, &evtA);
#if NATIVE_KERNEL
	#include "InputGaussian_ivp.c"
#else
  // kernel using standard OpenCL C

  __local uchar *src = (__local uchar *)local_src;
  __local uchar *dstData =
      (__local uchar *)local_dst + outTileEdge * local_dst_pitch + outTileEdge;
  for (int x = 0; x < outTileW; x += 32) {
    __local uchar *dst = dstData + x;
    __local uchar *pSrc0 = src + x * 2;
    ushort32 x00 = (vload32(0, (__local ushort *)pSrc0));
    ushort32 x01 = x00 >> 8;
    ushort32 x02 = (vload32(0, (__local ushort *)(pSrc0 + 2)));
    x00 = x00 & (ushort32)0xFF;
    x02 = x02 & (ushort32)0xFF;
    ushort32 row0 = x00 + x01 * (ushort32)2 + x02;

    __local uchar *pSrc1 = src + local_src_pitch + x * 2;
    __local uchar *pSrc2 = src + 2 * local_src_pitch + x * 2;
    for (int y = 0; y < outTileH; ++y) {
      ushort32 x10 = (vload32(0, (__local ushort *)pSrc1));
      ushort32 x11 = x10 >> 8;
      ushort32 x12 = (vload32(0, (__local ushort *)(pSrc1 + 2)));
      x10 = x10 & (ushort32)0xFF;
      x12 = x12 & (ushort32)0xFF;
      ushort32 row1 = x10 + x11 * (ushort32)2 + x12;

      ushort32 x20 = (vload32(0, (__local ushort *)pSrc2));
      ushort32 x21 = x20 >> 8;
      ushort32 x22 = (vload32(0, (__local ushort *)(pSrc2 + 2)));
      x20 = x20 & (ushort32)0xFF;
      x22 = x22 & (ushort32)0xFF;
      ushort32 row2 = x20 + (x21 * (ushort32)2) + x22;

      ushort32 result = row0 + (row1 * (ushort32)2) + row2;
      result = (result) >> 4;

      vstore32(convert_uchar32(result), 0, dst);
      dst += local_dst_pitch;

      pSrc1 += 2 * local_src_pitch;
      pSrc2 += 2 * local_src_pitch;

      row0 = row2;
    }
  }

  PadEdges_uchar(local_dst, outTileW, outTileH, outTileEdge, local_dst_pitch,
                 indX, indY, indX_size, indY_size);
#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + ((outEdge * outPitch) + outEdge + (out_i * outPitch) + out_j -
             LeftEdge - (TopEdge * outPitch)),
      local_dst + ((outTileEdge * local_dst_pitch) + outTileEdge - LeftEdge -
                   (TopEdge * local_dst_pitch)),
      DMAoutWidth * dstBytes, DMAoutHeight, outPitch * dstBytes,
      local_dst_pitch * dstBytes, 0);

  wait_group_events(1, &evtD);
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
ocl_ContrastLevel(__global uchar *restrict Src, int inPitch, int inEdge,
                  int inWidth, int inHeight, int inTileW, int inTileH,
                  int inTileEdge, __global short *restrict Dst, int outPitch,
                  int outEdge, int outWidth, int outHeight, int outTileW,
                  int outTileH, int outTileEdge,
                  __global short *restrict remap_table,
                  __local short *restrict local_lut,
                  __local uchar *restrict local_src,
                  __local short *restrict local_dst) {
  int i, j;
  int local_src_pitch, local_dst_pitch;
  local_src_pitch = (inTileW + 2 * inTileEdge);
  local_dst_pitch = (outTileW + 2 * outTileEdge);

  int dstBytes = 2;
  int dstElements = 8;
  int srcBytes = 1;
  //__local short local_lut[256*8];

  i = get_global_id(0) * inTileH;
  j = get_global_id(1) * inTileW;

  int out_i = get_global_id(0) * outTileH;
  int out_j = get_global_id(1) * outTileW;

  int indX = get_global_id(1);
  int indY = get_global_id(0);
  int indX_size = (get_global_size(1) - 1);
  int indY_size = (get_global_size(0) - 1);

  int LeftEdge = !(get_global_id(1)), TopEdge = !(get_global_id(0));
  int DMAoutWidth =
      outTileW + LeftEdge + ((get_global_id(1) == (get_global_size(1) - 1)));
  int DMAoutHeight =
      outTileH + TopEdge + ((get_global_id(0) == (get_global_size(0) - 1)));

  event_t evtA = xt_async_work_group_2d_copy(
      local_lut, remap_table, 256 * 8 * 2, 1, 256 * 8 * 2, 256 * 8 * 2, 0);

  event_t evtB = xt_async_work_group_2d_copy(
      local_src,
      (Src + ((inEdge * inPitch) + inEdge + (i * inPitch) + j - inTileEdge -
              (inTileEdge * inPitch))),
      local_src_pitch * srcBytes, (inTileH + 2 * inTileEdge),
      local_src_pitch * srcBytes, inPitch * srcBytes, 0);

  wait_group_events(1, &evtA);
  wait_group_events(1, &evtB);
#if NATIVE_KERNEL
	#include "ContrastLevel_ivp.c"
#else
  __local uchar *src = local_src;
  __local short *dstData =
      local_dst + (outTileEdge * local_dst_pitch + outTileEdge) * dstElements;

  const ushort32 Select0 = *(__constant ushort32 *)kCSelect0;
  ushort32 Select1 = Select0 + (ushort32)4;
  ushort32 Select2 = Select1 + (ushort32)4;
  ushort32 Select3 = Select2 + (ushort32)4;
  const ushort32 Offsets = *(__constant ushort32 *)kCOffsets;
  //__local short* LUT = local_lut;
  // kernel using standard OpenCL C
  for (int x = 0; x < inTileW; x += 16) {
    __local short *dst = dstData + x * dstElements;
    __local uchar *pSrc = src + x;
    for (int y = 0; y < inTileH; ++y) {
      ushort32 xGray = convert_ushort32(vload32(0, pSrc));
      ushort32 xGray0 = shuffle(xGray, Select0);
      ushort32 xGray1 = shuffle(xGray, Select1);
      ushort32 xGray2 = shuffle(xGray, Select2);
      ushort32 xGray3 = shuffle(xGray, Select3);

      xGray0 = (xGray0 << 3) + (Offsets >> 1);
      xGray1 = (xGray1 << 3) + (Offsets >> 1);
      xGray2 = (xGray2 << 3) + (Offsets >> 1);
      xGray3 = (xGray3 << 3) + (Offsets >> 1);

      short32 Output0 = xt_gather32(local_lut, (ushort *)&xGray0);
      short32 Output1 = xt_gather32(local_lut, (ushort *)&xGray1);
      short32 Output2 = xt_gather32(local_lut, (ushort *)&xGray2);
      short32 Output3 = xt_gather32(local_lut, (ushort *)&xGray3);

      vstore32(Output0, 0, dst);
      vstore32(Output1, 0, dst + 32);
      vstore32(Output2, 0, dst + 64);
      vstore32(Output3, 0, dst + 96);

      pSrc += local_src_pitch;
      dst += local_dst_pitch * dstElements;
    }
  }
  PadEdges_shortK(local_dst, outTileW, outTileH, outTileEdge, local_dst_pitch,
                  dstElements, indX, indY, indX_size, indY_size);
#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + ((outEdge * outPitch) + outEdge + (out_i * outPitch) + out_j -
             LeftEdge - (TopEdge * outPitch)) *
                dstElements,
      local_dst + ((outTileEdge * local_dst_pitch) + outTileEdge - LeftEdge -
                   (TopEdge * local_dst_pitch)) *
                      dstElements,
      DMAoutWidth * dstBytes * dstElements, DMAoutHeight,
      outPitch * dstBytes * dstElements,
      local_dst_pitch * dstBytes * dstElements, 0);

  wait_group_events(1, &evtD);
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
ocl_GaussianPyr(__global short *restrict Src, int inPitch, int inEdge,
                int inWidth, int inHeight, int inTileW, int inTileH,
                int inTileEdge, __global short *restrict Dst, int outPitch,
                int outEdge, int outWidth, int outHeight, int outTileW,
                int outTileH, int outTileEdge,
                __local short *restrict local_src,
                __local short *restrict local_dst) {
  int i, j;
  int local_src_pitch, local_dst_pitch;
  local_src_pitch = (inTileW + 2 * inTileEdge);
  local_dst_pitch = (outTileW + 2 * outTileEdge);

  const int dstBytes = 2;
  const int dstElements = 8;
  const int srcElements = 8;
  const int srcBytes = 2;

  int indX = get_global_id(1);
  int indY = get_global_id(0);
  int indX_size = (get_global_size(1) - 1);
  int indY_size = (get_global_size(0) - 1);

  int LeftEdge = !(get_global_id(1)), TopEdge = !(get_global_id(0));
  int DMAoutWidth =
      outTileW + LeftEdge + ((get_global_id(1) == (get_global_size(1) - 1)));
  int DMAoutHeight =
      outTileH + TopEdge + ((get_global_id(0) == (get_global_size(0) - 1)));

  i = get_global_id(0) * inTileH;
  j = get_global_id(1) * inTileW;

  int out_i = get_global_id(0) * outTileH;
  int out_j = get_global_id(1) * outTileW;

  event_t evtA = xt_async_work_group_2d_copy(
      local_src,
      (Src + ((inEdge * inPitch) + inEdge + (i * inPitch) + j - inTileEdge -
              (inTileEdge * inPitch)) *
                 srcElements),
      local_src_pitch * srcBytes * srcElements, (inTileH + 2 * inTileEdge),
      local_src_pitch * srcBytes * srcElements,
      inPitch * srcBytes * srcElements, 0);

  wait_group_events(1, &evtA);
#if NATIVE_KERNEL
	#include "GaussianPyr_ivp.c"
#else
  const ushort32 Select1 = *(__constant ushort32 *)kGSelect1;
  const ushort32 Select0 = Select1 - (ushort32)8;
  const ushort32 Select2 = *(__constant ushort32 *)kGSelect2;

  // kernel using standard OpenCL C
  __local short *src = (__local short *)local_src;
  __local short *dstData = (__local short *)local_dst +
                           outTileEdge * local_dst_pitch * dstElements +
                           outTileEdge * dstElements;
  for (int x = 0; x < outTileW; x += 4) {
    __local short *dst = dstData + x * dstElements;
    __local short *pSrc0 = src + x * srcElements * 2;
    short32 x00 = (vload32(0, pSrc0));
    short32 xtail = vload32(0, pSrc0 + 32);
    short32 xtail1 = vload32(0, pSrc0 + 64);
    short32 x01 = shuffle2(x00, xtail, Select1);
    x00 = shuffle2(x00, xtail, Select0);
    short32 x02 = shuffle2(x00, xtail1, Select2);

    // short32 row0 = x00 + x01 * (short32)2 + x02;
    short32 row0 = x00 + (x01 << 1) + x02;

    __local short *pSrc1 =
        src + local_src_pitch * srcElements + x * srcElements * 2;
    __local short *pSrc2 =
        src + 2 * local_src_pitch * srcElements + x * srcElements * 2;
    for (int y = 0; y < outTileH; ++y) {
      short32 x10 = (vload32(0, pSrc1));
      short32 xtail = vload32(1, pSrc1);
      short32 xtail1 = vload32(2, pSrc1);
      short32 x11 = shuffle2(x10, xtail, Select1);
      x10 = shuffle2(x10, xtail, Select0);
      short32 x12 = shuffle2(x10, xtail1, Select2);

      // short32 row1 = x10 + (x11 * (short32)2) + x12;
      short32 row1 = x10 + (x11 << 1) + x12;

      short32 x20 = (vload32(0, pSrc2));
      xtail = vload32(1, pSrc2);
      xtail1 = vload32(2, pSrc2);
      short32 x21 = shuffle2(x20, xtail, Select1);
      x20 = shuffle2(x20, xtail, Select0);
      short32 x22 = shuffle2(x20, xtail1, Select2);

      // short32 row2 = x20 + x21 * (short32)2 + x22;
      short32 row2 = x20 + (x21 << 1) + x22;

      // short32 result = row0 + (row1  * (short32)2) + row2;
      short32 result = row0 + (row1 << 1) + row2;
      result = (result) >> 4;

      vstore32(result, 0, dst);
      dst += local_dst_pitch * dstElements;

      pSrc1 += 2 * local_src_pitch * srcElements;
      pSrc2 += 2 * local_src_pitch * srcElements;

      row0 = row2;
    }
  }

  PadEdges_shortK(local_dst, outTileW, outTileH, outTileEdge, local_dst_pitch,
                  dstElements, indX, indY, indX_size, indY_size);
#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + ((outEdge * outPitch) + outEdge + (out_i * outPitch) + out_j -
             LeftEdge - (TopEdge * outPitch)) *
                dstElements,
      local_dst + ((outTileEdge * local_dst_pitch) + outTileEdge - LeftEdge -
                   (TopEdge * local_dst_pitch)) *
                      dstElements,
      DMAoutWidth * dstBytes * dstElements, DMAoutHeight,
      outPitch * dstBytes * dstElements,
      local_dst_pitch * dstBytes * dstElements, 0);

  wait_group_events(1, &evtD);
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
ocl_LaplacianPyr(__global short *restrict Src, int inPitch, int inEdge,
                 int inWidth, int inHeight, int inTileW, int inTileH,
                 int inTileEdge, __global short *restrict SrcPrev, int in1Pitch,
                 int in1Edge, int in1Width, int in1Height, int in1TileW,
                 int in1TileH, int in1TileEdge, __global short *restrict Dst,
                 int outPitch, int outEdge, int outWidth, int outHeight,
                 int outTileW, int outTileH, int outTileEdge,
                 __local short *restrict local_src,
                 __local short *restrict local_srcPrev,
                 __local short *restrict local_dst) {
  int i, j;
  int local_src_pitch, local_dst_pitch;
  local_src_pitch = (inTileW + 2 * inTileEdge);
  int local_srcPrev_pitch = (in1TileW + 2 * in1TileEdge);
  local_dst_pitch = (outTileW + 2 * outTileEdge);

  int dstBytes = 2;
  int dstElements = 8;
  int srcElements = 8;
  int srcBytes = 2;

  i = get_global_id(0) * inTileH;
  j = get_global_id(1) * inTileW;
  int out_i = get_global_id(0) * outTileH;
  int out_j = get_global_id(1) * outTileW;
  int in1_i = get_global_id(0) * in1TileH;
  int in1_j = get_global_id(1) * in1TileW;

  int indX = get_global_id(1);
  int indY = get_global_id(0);
  int indX_size = (get_global_size(1) - 1);
  int indY_size = (get_global_size(0) - 1);

  int LeftEdge = !(get_global_id(1)), TopEdge = !(get_global_id(0));
  int DMAoutWidth =
      outTileW + LeftEdge + ((get_global_id(1) == (get_global_size(1) - 1)));
  int DMAoutHeight =
      outTileH + TopEdge + ((get_global_id(0) == (get_global_size(0) - 1)));

  event_t evtA = xt_async_work_group_2d_copy(
      local_src,
      (Src + ((inEdge * inPitch) + inEdge + (i * inPitch) + j - inTileEdge -
              (inTileEdge * inPitch)) *
                 srcElements),
      local_src_pitch * srcBytes * srcElements, (inTileH + 2 * inTileEdge),
      local_src_pitch * srcBytes * srcElements,
      inPitch * srcBytes * srcElements, 0);

  event_t evtB = xt_async_work_group_2d_copy(
      local_srcPrev,
      (SrcPrev + ((in1Edge * in1Pitch) + in1Edge + (in1_i * in1Pitch) + in1_j -
                  in1TileEdge - (in1TileEdge * in1Pitch)) *
                     srcElements),
      local_srcPrev_pitch * srcBytes * srcElements,
      (in1TileH + 2 * in1TileEdge),
      local_srcPrev_pitch * srcBytes * srcElements,
      in1Pitch * srcBytes * srcElements, 0);

  wait_group_events(1, &evtA);
  wait_group_events(1, &evtB);
#if NATIVE_KERNEL
	#include "LaplacianPyr_ivp.c"
#else
  const ushort32 Select0 = *(__constant ushort32 *)kLSelect0;
  const ushort32 Select1 = Select0 + (ushort32)16;

  // kernel using standard OpenCL C
  __local short *src = (__local short *)local_srcPrev +
                       in1TileEdge * local_srcPrev_pitch * srcElements +
                       in1TileEdge * srcElements;
  __local short *srcPrv = (__local short *)local_src;
  __local short *dstData = (__local short *)local_dst +
                           outTileEdge * local_dst_pitch * dstElements +
                           outTileEdge * dstElements;
  for (int x = 0; x < in1TileW; x += 4) {
    __local short *dst = dstData + x * dstElements * 2;
    __local short *pSrc0 = src + x * srcElements;
    __local short *pSrcPrv0 = srcPrv + x * srcElements * 2;
    short32 x00 = (vload32(0, pSrc0));
    short32 x01 = vload32(0, pSrc0 + 8);
    short32 xMid01 = x00 + x01;

    __local short *pSrc1 =
        src + local_srcPrev_pitch * srcElements + x * srcElements;
    __local short *pSrcPrv1 =
        srcPrv + local_src_pitch * srcElements + x * srcElements * 2;

    for (int y = 0; y < in1TileH; ++y) {
      short32 x10 = (vload32(0, pSrc1));
      short32 x11 = vload32(0, pSrc1 + 8);
      short32 xMid21 = x10 + x11;

      short32 xMid11 = (xMid01 + xMid21) >> 2;
      short32 xMid10 = (x00 + x10) >> 1;

      short32 xOut00 = shuffle2(x00, (xMid01) >> 1, Select0);
      short32 xOut01 = shuffle2(x00, (xMid01) >> 1, Select1);
      short32 xOut10 = shuffle2(xMid10, xMid11, Select0);
      short32 xOut11 = shuffle2(xMid10, xMid11, Select1);

      short32 xG00 = (vload32(0, pSrcPrv0));
      short32 xG01 = (vload32(0, pSrcPrv0 + 32));
      short32 xG10 = (vload32(0, pSrcPrv1));
      short32 xG11 = (vload32(0, pSrcPrv1 + 32));

      xOut00 = xG00 - xOut00;
      xOut01 = xG01 - xOut01;
      xOut10 = xG10 - xOut10;
      xOut11 = xG11 - xOut11;

      vstore32(xOut00, 0, dst);
      vstore32(xOut01, 0, dst + 32);
      dst += local_dst_pitch * dstElements;
      vstore32(xOut10, 0, dst);
      vstore32(xOut11, 0, dst + 32);
      dst += local_dst_pitch * dstElements;

      pSrc1 += local_srcPrev_pitch * srcElements;
      pSrcPrv0 += 2 * local_src_pitch * srcElements;
      pSrcPrv1 += 2 * local_src_pitch * srcElements;

      xMid01 = xMid21;
      x00 = x10;
    }
  }

  PadEdges_shortK(local_dst, outTileW, outTileH, outTileEdge, local_dst_pitch,
                  dstElements, indX, indY, indX_size, indY_size);
#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + ((outEdge * outPitch) + outEdge + (out_i * outPitch) + out_j -
             LeftEdge - (TopEdge * outPitch)) *
                dstElements,
      local_dst + ((outTileEdge * local_dst_pitch) + outTileEdge - LeftEdge -
                   (TopEdge * local_dst_pitch)) *
                      dstElements,
      DMAoutWidth * dstBytes * dstElements, DMAoutHeight,
      outPitch * dstBytes * dstElements,
      local_dst_pitch * dstBytes * dstElements, 0);

  wait_group_events(1, &evtD);
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
ocl_FuseLaplacianPyr(__global short *restrict Src, int inPitch, int inEdge,
                     int inWidth, int inHeight, int inTileW, int inTileH,
                     int inTileEdge, __global uchar *restrict SrcGray,
                     int in1Pitch, int in1Edge, int in1Width, int in1Height,
                     int in1TileW, int in1TileH, int in1TileEdge,
                     __global short *restrict Dst, int outPitch, int outEdge,
                     int outWidth, int outHeight, int outTileW, int outTileH,
                     int outTileEdge, __local short *restrict local_src,
                     __local uchar *restrict local_srcgray,
                     __local short *restrict local_dst) {
  int i, j;
  int local_src_pitch, local_dst_pitch;
  local_src_pitch = (inTileW + 2 * inTileEdge);
  int local_srcgray_pitch = (in1TileW + 2 * in1TileEdge);
  local_dst_pitch = (outTileW + 2 * outTileEdge);

  int dstBytes = 2;
  int dstElements = 1;
  int srcElements = 8;
  int srcBytes = 2;

  i = get_global_id(0) * inTileH;
  j = get_global_id(1) * inTileW;

  int out_i = get_global_id(0) * outTileH;
  int out_j = get_global_id(1) * outTileW;

  int indX = get_global_id(1);
  int indY = get_global_id(0);
  int indX_size = (get_global_size(1) - 1);
  int indY_size = (get_global_size(0) - 1);

  int LeftEdge = !(get_global_id(1)), TopEdge = !(get_global_id(0));
  int DMAoutWidth =
      outTileW + LeftEdge + ((get_global_id(1) == (get_global_size(1) - 1)));
  int DMAoutHeight =
      outTileH + TopEdge + ((get_global_id(0) == (get_global_size(0) - 1)));

  event_t evtA = xt_async_work_group_2d_copy(
      local_src,
      (Src + ((inEdge * inPitch) + inEdge + (i * inPitch) + j - inTileEdge -
              (inTileEdge * inPitch)) *
                 srcElements),
      local_src_pitch * srcBytes * srcElements, (inTileH + 2 * inTileEdge),
      local_src_pitch * srcBytes * srcElements,
      inPitch * srcBytes * srcElements, 0);

  event_t evtB = xt_async_work_group_2d_copy(
      local_srcgray,
      (SrcGray + ((in1Edge * in1Pitch) + in1Edge + (i * in1Pitch) + j -
                  in1TileEdge - (in1TileEdge * in1Pitch))),
      local_srcgray_pitch, (in1TileH + 2 * in1TileEdge), local_srcgray_pitch,
      in1Pitch, 0);

  wait_group_events(1, &evtA);
  wait_group_events(1, &evtB);
#if NATIVE_KERNEL
	#include "FuseLaplacianPyr_ivp.c"
#else
  const ushort32 Offsets = *(__constant ushort32 *)kFOffsets;
  __local uchar *srcG = (__local uchar *)local_srcgray;
  __local short *src = (__local short *)local_src;
  __local short *dstData = (__local short *)local_dst +
                           outTileEdge * local_dst_pitch * dstElements +
                           outTileEdge * dstElements;

  // kernel using standard OpenCL C
  for (int x = 0; x < inTileW; x += 32) {
    __local short *dst = dstData + x;
    __local uchar *pSrcG = srcG + x;
    __local short *pSrc = src + x * srcElements;
    for (int y = 0; y < inTileH; ++y) {
      ushort32 xGray = convert_ushort32(vload32(0, pSrcG));
      ushort32 xGrayLevel = xGray * (ushort32)7;
      ushort32 xGrayInt = xGrayLevel >> 8;

      ushort32 xGrayFracTemp = (xGrayLevel - (xGrayInt << 8));
      short32 xGrayFrac = (*(short32 *)&xGrayFracTemp);
      xGrayInt += Offsets;

      short32 L0 = xt_gather32(pSrc, (ushort *)&xGrayInt);
      xGrayInt += (ushort32)1;
      short32 L1 = xt_gather32(pSrc, (ushort *)&xGrayInt);
      
      int32 A = xt_mul32((short32)(1 << 8) - xGrayFrac, L0);
      int32 B = xt_mul32(xGrayFrac, L1);
      A = (A + B)>> 8;
      short32 Output = convert_short32(A);
      vstore32(Output, 0, dst);

      pSrc += local_src_pitch * srcElements;
      pSrcG += local_srcgray_pitch;
      dst += local_dst_pitch * dstElements;
    }
  }

  PadEdges_short(local_dst, outTileW, outTileH, outTileEdge, local_dst_pitch,
                 indX, indY, indX_size, indY_size);
#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + ((outEdge * outPitch) + outEdge + (out_i * outPitch) + out_j -
             LeftEdge - (TopEdge * outPitch)) *
                dstElements,
      local_dst + ((outTileEdge * local_dst_pitch) + outTileEdge - LeftEdge -
                   (TopEdge * local_dst_pitch)) *
                      dstElements,
      DMAoutWidth * dstBytes * dstElements, DMAoutHeight,
      outPitch * dstBytes * dstElements,
      local_dst_pitch * dstBytes * dstElements, 0);

  wait_group_events(1, &evtD);
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
ocl_FusedPyrUpSample(__global short *restrict Src, int inPitch, int inEdge,
                     int inWidth, int inHeight, int inTileW, int inTileH,
                     int inTileEdge, __global short *restrict SrcPrev,
                     int in1Pitch, int in1Edge, int in1Width, int in1Height,
                     int in1TileW, int in1TileH, int in1TileEdge,
                     __global short *restrict Dst, int outPitch, int outEdge,
                     int outWidth, int outHeight, int outTileW, int outTileH,
                     int outTileEdge, __local short *restrict local_src,
                     __local short *restrict local_srcPrev,
                     __local short *restrict local_dst) {
  int i, j;
  int local_src_pitch, local_dst_pitch;
  local_src_pitch = (inTileW + 2 * inTileEdge);
  int local_srcPrev_pitch = (in1TileW + 2 * in1TileEdge);
  local_dst_pitch = (outTileW + 2 * outTileEdge);

  int dstBytes = 2;
  int dstElements = 1;
  int srcElements = 1;
  int srcBytes = 2;

  i = get_global_id(0) * inTileH;
  j = get_global_id(1) * inTileW;

  int out_i = get_global_id(0) * outTileH;
  int out_j = get_global_id(1) * outTileW;
  int in1_i = get_global_id(0) * outTileH;
  int in1_j = get_global_id(1) * outTileW;

  int indX = get_global_id(1);
  int indY = get_global_id(0);
  int indX_size = (get_global_size(1) - 1);
  int indY_size = (get_global_size(0) - 1);

  int LeftEdge = !(get_global_id(1)), TopEdge = !(get_global_id(0));
  int DMAoutWidth =
      outTileW + LeftEdge + ((get_global_id(1) == (get_global_size(1) - 1)));
  int DMAoutHeight =
      outTileH + TopEdge + ((get_global_id(0) == (get_global_size(0) - 1)));

  event_t evtA = xt_async_work_group_2d_copy(
      local_src,
      (Src + ((inEdge * inPitch) + inEdge + (i * inPitch) + j - inTileEdge -
              (inTileEdge * inPitch)) *
                 srcElements),
      local_src_pitch * srcBytes * srcElements, (inTileH + 2 * inTileEdge),
      local_src_pitch * srcBytes * srcElements,
      inPitch * srcBytes * srcElements, 0);

  event_t evtB = xt_async_work_group_2d_copy(
      local_srcPrev,
      (SrcPrev + ((in1Edge * in1Pitch) + in1Edge + (in1_i * in1Pitch) + in1_j -
                  in1TileEdge - (in1TileEdge * in1Pitch)) *
                     srcElements),
      local_srcPrev_pitch * srcBytes * srcElements,
      (in1TileH + 2 * in1TileEdge),
      local_srcPrev_pitch * srcBytes * srcElements,
      in1Pitch * srcBytes * srcElements, 0);

  wait_group_events(1, &evtA);
  wait_group_events(1, &evtB);
#if NATIVE_KERNEL
	#include "FusedPyrUpSample_ivp.c"
#else
  const ushort32 Select0 = {0,  32, 1,  33, 2,  34, 3,  35, 4,  36, 5,
                            37, 6,  38, 7,  39, 8,  40, 9,  41, 10, 42,
                            11, 43, 12, 44, 13, 45, 14, 46, 15, 47};
  const ushort32 Select1 = Select0 + (ushort32)16;

  // kernel using standard OpenCL C
  __local short *src = (__local short *)local_src +
                       inTileEdge * local_src_pitch * srcElements +
                       inTileEdge * srcElements;
  __local short *srcPrv = (__local short *)local_srcPrev;
  __local short *dstData = (__local short *)local_dst +
                           outTileEdge * local_dst_pitch * dstElements +
                           outTileEdge * dstElements;
  for (int x = 0; x < inTileW; x += 32) {
    __local short *dst = dstData + x * dstElements * 2;
    __local short *pSrc0 = src + x * srcElements;
    __local short *pSrcPrv0 = srcPrv + x * srcElements * 2;
    short32 x00 = (vload32(0, pSrc0));
    short32 x01 = vload32(0, pSrc0 + 1);
    short32 xMid01 = x00 + x01;

    __local short *pSrc1 =
        src + local_src_pitch * srcElements + x * srcElements;
    __local short *pSrcPrv1 =
        srcPrv + local_srcPrev_pitch * srcElements + x * srcElements * 2;

    for (int y = 0; y < inTileH; ++y) {
      short32 x10 = (vload32(0, pSrc1));
      short32 x11 = vload32(0, pSrc1 + 1);
      short32 xMid21 = x10 + x11;

      short32 xMid11 = (xMid01 + xMid21) >> 2;
      short32 xMid10 = (x00 + x10) >> 1;

      short32 xOut00 = shuffle2(x00, (xMid01) >> 1, Select0);
      short32 xOut01 = shuffle2(x00, (xMid01) >> 1, Select1);
      short32 xOut10 = shuffle2(xMid10, xMid11, Select0);
      short32 xOut11 = shuffle2(xMid10, xMid11, Select1);

      short32 xG00 = (vload32(0, pSrcPrv0));
      short32 xG01 = (vload32(0, pSrcPrv0 + 32));
      short32 xG10 = (vload32(0, pSrcPrv1));
      short32 xG11 = (vload32(0, pSrcPrv1 + 32));

      xOut00 = xOut00 + xG00;
      xOut01 = xOut01 + xG01;
      xOut10 = xOut10 + xG10;
      xOut11 = xOut11 + xG11;

      vstore32(xOut00, 0, dst);
      vstore32(xOut01, 0, dst + 32);
      dst += local_dst_pitch * dstElements;
      vstore32(xOut10, 0, dst);
      vstore32(xOut11, 0, dst + 32);
      dst += local_dst_pitch * dstElements;

      pSrc1 += local_src_pitch * srcElements;
      pSrcPrv0 += 2 * local_srcPrev_pitch * srcElements;
      pSrcPrv1 += 2 * local_srcPrev_pitch * srcElements;

      xMid01 = xMid21;
      x00 = x10;
    }
  }

  PadEdges_short(local_dst, outTileW, outTileH, outTileEdge, local_dst_pitch,
                 indX, indY, indX_size, indY_size);
#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + ((outEdge * outPitch) + outEdge + (out_i * outPitch) + out_j -
             LeftEdge - (TopEdge * outPitch)) *
                dstElements,
      local_dst + ((outTileEdge * local_dst_pitch) + outTileEdge - LeftEdge -
                   (TopEdge * local_dst_pitch)) *
                      dstElements,
      DMAoutWidth * dstBytes * dstElements, DMAoutHeight,
      outPitch * dstBytes * dstElements,
      local_dst_pitch * dstBytes * dstElements, 0);

  wait_group_events(1, &evtD);
}

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
ocl_FusedPyrUpSampleClip(__global short *restrict Src, int inPitch, int inEdge,
                         int inWidth, int inHeight, int inTileW, int inTileH,
                         int inTileEdge, __global short *restrict SrcPrev,
                         int in1Pitch, int in1Edge, int in1Width, int in1Height,
                         int in1TileW, int in1TileH, int in1TileEdge,
                         __global uchar *restrict Dst, int outPitch,
                         int outEdge, int outWidth, int outHeight, int outTileW,
                         int outTileH, int outTileEdge,
                         __local short *restrict local_src,
                         __local short *restrict local_srcPrev,
                         __local uchar *restrict local_dst) {
  int i, j;
  int local_src_pitch, local_dst_pitch;
  local_src_pitch = (inTileW + 2 * inTileEdge);
  int local_srcPrev_pitch = (in1TileW + 2 * in1TileEdge);
  local_dst_pitch = (outTileW + 2 * outTileEdge);

  int dstBytes = 1;
  int dstElements = 1;
  int srcElements = 1;
  int srcBytes = 2;

  i = get_global_id(0) * inTileH;
  j = get_global_id(1) * inTileW;

  int DMAoutWidth = outTileW, DMAoutHeight = outTileH;
  int LeftEdge = 0, TopEdge = 0;
  int out_i = get_global_id(0) * outTileH;
  int out_j = get_global_id(1) * outTileW;
  int in1_i = get_global_id(0) * outTileH;
  int in1_j = get_global_id(1) * outTileW;
  event_t evtA = xt_async_work_group_2d_copy(
      local_src,
      (Src + ((inEdge * inPitch) + inEdge + (i * inPitch) + j - inTileEdge -
              (inTileEdge * inPitch)) *
                 srcElements),
      local_src_pitch * srcBytes * srcElements, (inTileH + 2 * inTileEdge),
      local_src_pitch * srcBytes * srcElements,
      inPitch * srcBytes * srcElements, 0);

  event_t evtB = xt_async_work_group_2d_copy(
      local_srcPrev,
      (SrcPrev + ((in1Edge * in1Pitch) + in1Edge + (in1_i * in1Pitch) + in1_j -
                  in1TileEdge - (in1TileEdge * in1Pitch)) *
                     srcElements),
      local_srcPrev_pitch * srcBytes * srcElements,
      (in1TileH + 2 * in1TileEdge),
      local_srcPrev_pitch * srcBytes * srcElements,
      in1Pitch * srcBytes * srcElements, 0);

  wait_group_events(1, &evtA);
  wait_group_events(1, &evtB);
#if NATIVE_KERNEL
	#include "FusedPyrUpSampleClip_ivp.c"
#else
  const ushort32 Select0 = {0,  32, 1,  33, 2,  34, 3,  35, 4,  36, 5,
                            37, 6,  38, 7,  39, 8,  40, 9,  41, 10, 42,
                            11, 43, 12, 44, 13, 45, 14, 46, 15, 47};
  const ushort32 Select1 = Select0 + (ushort32)16;

  // kernel using standard OpenCL C
  __local short *src = (__local short *)local_src +
                       inTileEdge * local_src_pitch * srcElements +
                       inTileEdge * srcElements;
  __local short *srcPrv = (__local short *)local_srcPrev;
  __local uchar *dstData = (__local uchar *)local_dst +
                           outTileEdge * local_dst_pitch * dstElements +
                           outTileEdge * dstElements;
  for (int x = 0; x < inTileW; x += 32) {
    __local uchar *dst = dstData + x * dstElements * 2;
    __local short *pSrc0 = src + x * srcElements;
    __local short *pSrcPrv0 = srcPrv + x * srcElements * 2;
    short32 x00 = (vload32(0, pSrc0));
    short32 x01 = vload32(0, pSrc0 + 1);
    short32 xMid01 = x00 + x01;

    __local short *pSrc1 =
        src + local_src_pitch * srcElements + x * srcElements;
    __local short *pSrcPrv1 =
        srcPrv + local_srcPrev_pitch * srcElements + x * srcElements * 2;

    for (int y = 0; y < inTileH; ++y) {
      short32 x10 = (vload32(0, pSrc1));
      short32 x11 = vload32(0, pSrc1 + 1);
      short32 xMid21 = x10 + x11;

      short32 xMid11 = (xMid01 + xMid21) >> 2;
      short32 xMid10 = (x00 + x10) >> 1;

      short32 xOut00 = shuffle2(x00, (xMid01) >> 1, Select0);
      short32 xOut01 = shuffle2(x00, (xMid01) >> 1, Select1);
      short32 xOut10 = shuffle2(xMid10, xMid11, Select0);
      short32 xOut11 = shuffle2(xMid10, xMid11, Select1);

      short32 xG00 = (vload32(0, pSrcPrv0));
      short32 xG01 = (vload32(0, pSrcPrv0 + 32));
      short32 xG10 = (vload32(0, pSrcPrv1));
      short32 xG11 = (vload32(0, pSrcPrv1 + 32));

      xOut00 = xOut00 + xG00;
      xOut01 = xOut01 + xG01;
      xOut10 = xOut10 + xG10;
      xOut11 = xOut11 + xG11;

      xOut00 = max(xOut00, (short32)0);
      xOut00 = min(xOut00, (short32)255);
      xOut01 = max(xOut01, (short32)0);
      xOut01 = min(xOut01, (short32)255);
      xOut10 = max(xOut10, (short32)0);
      xOut10 = min(xOut10, (short32)255);
      xOut11 = max(xOut11, (short32)0);
      xOut11 = min(xOut11, (short32)255);

      vstore32(convert_uchar32(xOut00), 0, dst);
      vstore32(convert_uchar32(xOut01), 0, dst + 32);
      dst += local_dst_pitch * dstElements;
      vstore32(convert_uchar32(xOut10), 0, dst);
      vstore32(convert_uchar32(xOut11), 0, dst + 32);
      dst += local_dst_pitch * dstElements;

      pSrc1 += local_src_pitch * srcElements;
      pSrcPrv0 += 2 * local_srcPrev_pitch * srcElements;
      pSrcPrv1 += 2 * local_srcPrev_pitch * srcElements;

      xMid01 = xMid21;
      x00 = x10;
    }
  }
#endif
  event_t evtD = xt_async_work_group_2d_copy(
      Dst + ((outEdge * outPitch) + outEdge + (out_i * outPitch) + out_j -
             LeftEdge - (TopEdge * outPitch)) *
                dstElements,
      local_dst + ((outTileEdge * local_dst_pitch) + outTileEdge - LeftEdge -
                   (TopEdge * local_dst_pitch)) *
                      dstElements,
      DMAoutWidth * dstBytes * dstElements, DMAoutHeight,
      outPitch * dstBytes * dstElements,
      local_dst_pitch * dstBytes * dstElements, 0);

  wait_group_events(1, &evtD);
}
