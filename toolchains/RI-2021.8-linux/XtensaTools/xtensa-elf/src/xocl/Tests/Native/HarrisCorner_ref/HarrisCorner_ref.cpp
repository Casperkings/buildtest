#include "xi_api_ref.h"
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLIPU8(a) (uint8_t) MIN(MAX(0, a), 255)

#define FIX_SOBEL_ROUNDING1 1
/*
 * 3x3 Sobel along both x and y direction
 * Input:
 * 		p_u8Src:      Pointer to uint8_t input image with appropriate
 * boundary padding of edge 1 (Must point to start of actual image without
 * padding) u32Width:     Width of input image (without considering padding
 * width) u32Height:    Height of input image (without considering padding
 * height) s32SrcStride: Stride of padded image s32DstStride: Stride of both
 * outputs Output: p_s8DstDx:    Pointer to normalized 3x3 Sobel output along x
 * direction p_s8DstDy:    Pointer to normalized 3x3 Sobel output along y
 * direction
 */
void sobel3x3dxdy_U8S8_ref(const uint8_t *const p_u8Src,
                           int8_t *const p_s8DstDx, int8_t *const p_s8DstDy,
                           const uint32_t u32Width, const uint32_t u32Height,
                           const int32_t s32SrcStride,
                           const int32_t s32DstStride) {
  uint32_t i, j;
  int32_t accDx, accDy;
  const uint8_t *p_in_u8;
  int16_t p_s16KernelDx[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
  int16_t p_s16KernelDy[9] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};

  for (i = 0; i < u32Height; i++) {
    p_in_u8 = (uint8_t *)&p_u8Src[i * s32SrcStride];
    for (j = 0; j < u32Width; j++) {
      accDx = 0;
      accDx += p_in_u8[-1 - s32SrcStride] * p_s16KernelDx[0];
      accDx += p_in_u8[0 - s32SrcStride] * p_s16KernelDx[1];
      accDx += p_in_u8[1 - s32SrcStride] * p_s16KernelDx[2];
      accDx += p_in_u8[-1] * p_s16KernelDx[3];
      accDx += p_in_u8[0] * p_s16KernelDx[4];
      accDx += p_in_u8[1] * p_s16KernelDx[5];
      accDx += p_in_u8[-1 + s32SrcStride] * p_s16KernelDx[6];
      accDx += p_in_u8[0 + s32SrcStride] * p_s16KernelDx[7];
      accDx += p_in_u8[1 + s32SrcStride] * p_s16KernelDx[8];

      accDy = 0;
      accDy += p_in_u8[-1 - s32SrcStride] * p_s16KernelDy[0];
      accDy += p_in_u8[0 - s32SrcStride] * p_s16KernelDy[1];
      accDy += p_in_u8[1 - s32SrcStride] * p_s16KernelDy[2];
      accDy += p_in_u8[-1] * p_s16KernelDy[3];
      accDy += p_in_u8[0] * p_s16KernelDy[4];
      accDy += p_in_u8[1] * p_s16KernelDy[5];
      accDy += p_in_u8[-1 + s32SrcStride] * p_s16KernelDy[6];
      accDy += p_in_u8[0 + s32SrcStride] * p_s16KernelDy[7];
      accDy += p_in_u8[1 + s32SrcStride] * p_s16KernelDy[8];

#if defined(FIX_SOBEL_ROUNDING1)
      p_s8DstDx[s32DstStride * i + j] = (int8_t)(
          0x000000FF &
          (accDx >> 3)); // large positive values may become negative and large
                         // negative values may become positive because of this
      p_s8DstDy[s32DstStride * i + j] = (int8_t)(0x000000FF & (accDy >> 3));
#elif defined(                                                                 \
    FIX_SOBEL_ROUNDING) // Round and Saturate to signed 8 bit range accurately
      accDx = ((accDx + 1) >> 1);
      accDy = ((accDy + 1) >> 1);
      p_s8DstDx[s32DstStride * i + j] =
          int8_t((accDx > 127) ? 127 : ((accDx < -127) ? -127 : accDx));
      p_s8DstDy[s32DstStride * i + j] =
          int8_t((accDy > 127) ? 127 : ((accDy < -127) ? -127 : accDy));

#else
      int8_t dx, dy;
      dx = (int8_t)(
          0x000000FF &
          (accDx >> 1)); // large positive values may become negative and large
                         // negative values may become positive because of this
      dy = (int8_t)(0x000000FF & (accDy >> 1));
      p_s8DstDx[s32DstStride * i + j] =
          (int8_t)((dx < -127) ? -127 : dx); // Limit   -127<= dx/dy <= 127
      p_s8DstDy[s32DstStride * i + j] = (int8_t)((dy < -127) ? -127 : dy);
#endif
      // accDx = ((accDx+1)>>1) ;
      // accDy = ((accDy+1)>>1) ;
      // p_s8DstDx[s32DstStride*i + j] = int8_t((accDx>127) ? 127:
      // ((accDx<-127)? -127: accDx)) ; p_s8DstDy[s32DstStride*i + j] =
      // int8_t((accDy>127) ? 127: ((accDy<-127)? -127: accDy)) ;

      p_in_u8++;
    }
  }
}

void cornerHarris5x5_U8_ref(const uint8_t *const p_u8Src,
                            uint8_t *const p_u8Dst, const uint32_t u32Width,
                            const uint32_t u32Height,
                            const int32_t s32SrcStride,
                            const int32_t s32DstStride,
                            const uint32_t u32Shift) {
  int8_t *p_s8Dx = (int8_t *)malloc((u32Width + 4) * (u32Height + 4));
  int8_t *p_s8Dy = (int8_t *)malloc((u32Width + 4) * (u32Height + 4));

  const uint8_t *p_u8SobelSrc = p_u8Src - 2 * s32SrcStride - 2;

  int sobelWidth = u32Width + 4;
  int sobelHeight = u32Height + 4;
  int sobelStride = sobelWidth;
  sobel3x3dxdy_U8S8_ref(p_u8SobelSrc, p_s8Dx, p_s8Dy, sobelWidth, sobelHeight,
                        s32SrcStride, sobelStride);

  uint32_t i, j, k, l;
  int16_t s16Dx[5][5], s16Dy[5][5];
  int32_t s32SigmaDx2, s32SigmaDxDy, s32SigmaDy2, s32Result;
  uint8_t u8Result;

  for (i = 0; i < u32Height; i++) {
    for (j = 0; j < u32Width; j++) {
      s32Result = 0;
      s32SigmaDxDy = 0;
      s32SigmaDx2 = 0;
      s32SigmaDy2 = 0;
      for (k = 0; k < 5; k++) {
        for (l = 0; l < 5; l++) {
          s16Dx[k][l] = p_s8Dx[(i + k) * sobelStride + (j + l)];
          s16Dy[k][l] = p_s8Dy[(i + k) * sobelStride + (j + l)];
        }
      }
      for (k = 0; k < 5; k++) {
        for (l = 0; l < 5; l++) {
          s32SigmaDxDy += s16Dx[k][l] * s16Dy[k][l];
        }
      }

#if defined(FIX_SOBEL_ROUNDING1)
      s32SigmaDxDy >>= 6;
#else
      s32SigmaDxDy >>= 10;
#endif
      for (k = 0; k < 5; k++) {
        for (l = 0; l < 5; l++) {
          s32SigmaDx2 += s16Dx[k][l] * s16Dx[k][l];
        }
      }
#if defined(FIX_SOBEL_ROUNDING1)
      s32SigmaDx2 >>= 6;
#else
      s32SigmaDx2 >>= 10;
#endif
      for (k = 0; k < 5; k++) {
        for (l = 0; l < 5; l++) {
          s32SigmaDy2 += s16Dy[k][l] * s16Dy[k][l];
        }
      }
#if defined(FIX_SOBEL_ROUNDING1)
      s32SigmaDy2 >>= 6;
#else
      s32SigmaDy2 >>= 10;
#endif

      s32Result = s32SigmaDx2 * s32SigmaDy2 - (s32SigmaDxDy * s32SigmaDxDy) -
                  ((s32SigmaDx2 + s32SigmaDy2) >> u32Shift);
      s32Result >>= 8;
      u8Result = CLIPU8(s32Result);
      p_u8Dst[i * s32DstStride + j] = u8Result;
    }
  }

  free(p_s8Dx);
  free(p_s8Dy);
}
