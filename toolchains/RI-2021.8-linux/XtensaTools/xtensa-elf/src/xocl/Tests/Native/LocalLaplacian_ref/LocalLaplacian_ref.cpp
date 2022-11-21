#include "math.h"
#include "xi_api_ref.h"
#include <stdio.h>

#define XT_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define XT_MAX(a, b) (((a) < (b)) ? (b) : (a))
#define CLIP(value, min, max) XT_MAX(min, XT_MIN(max, value))
#define DEBUG_DUMP 0
// S-curve
float remap(float x, float alpha) {
  float y;
  float fx;
  fx = x / 256.0;
  y = alpha * fx * exp(-fx * fx * 0.5);
  return y;
}
void RemapTableGenerate(float alpha, int levels, int16_t table[][256]) {
  for (int k = 0; k < levels; k++) {
    for (int gray = 0; gray < 256; gray++) {
      float grayF = gray / 255.0;
      float val = remap(((levels - 1) * grayF - k) * 256, alpha);
      val = val + grayF;
      int16_t val_fx = (int16_t)(val * (1 << 8));
      table[k][gray] = val_fx; // Q2.8
    }
  }
}
#define PYRAMIDWIDTH (512)  //(128)
#define PYRAMIDHEIGHT (512) //(64)
#define PYRAMIDEDGE (1)
#define KLEVELS (8)
#define PYRAMID3DL1SIZE                                                        \
  ((PYRAMIDWIDTH + 2 * PYRAMIDEDGE) * (PYRAMIDHEIGHT + 2 * PYRAMIDEDGE) *      \
   KLEVELS)
#define PYRAMID3DL2SIZE                                                        \
  ((PYRAMIDWIDTH / 2 + 2 * PYRAMIDEDGE) *                                      \
   (PYRAMIDHEIGHT / 2 + 2 * PYRAMIDEDGE) * KLEVELS)
#define PYRAMID3DL3SIZE                                                        \
  ((PYRAMIDWIDTH / 4 + 2 * PYRAMIDEDGE) *                                      \
   (PYRAMIDHEIGHT / 4 + 2 * PYRAMIDEDGE) * KLEVELS)
#define PYRAMID2DL1SIZE                                                        \
  ((PYRAMIDWIDTH + 2 * PYRAMIDEDGE) * (PYRAMIDHEIGHT + 2 * PYRAMIDEDGE))
#define PYRAMID2DL2SIZE                                                        \
  ((PYRAMIDWIDTH / 2 + 2 * PYRAMIDEDGE) * (PYRAMIDHEIGHT / 2 + 2 * PYRAMIDEDGE))
#define PYRAMID2DL3SIZE                                                        \
  ((PYRAMIDWIDTH / 4 + 2 * PYRAMIDEDGE) * (PYRAMIDHEIGHT / 4 + 2 * PYRAMIDEDGE))

float InGaussianPyramid[PYRAMID3DL1SIZE + PYRAMID3DL2SIZE + PYRAMID3DL3SIZE];
float InLaplacianPyramid[PYRAMID3DL1SIZE + PYRAMID3DL2SIZE + PYRAMID3DL3SIZE];
float GaussianPyramid[PYRAMID2DL1SIZE + PYRAMID2DL2SIZE + PYRAMID2DL3SIZE];
float OutLaplacianPyramid[PYRAMID2DL1SIZE + PYRAMID2DL2SIZE + PYRAMID2DL3SIZE];
float OutGaussianPyramid[PYRAMID2DL1SIZE + PYRAMID2DL2SIZE + PYRAMID2DL3SIZE];
XI_ERR_TYPE xiLocalLaplacian_Gray_U8_ref(const xi_pTile src, xi_pTile dst,
                                         int levels, float alpha, float beta,
                                         int pyramid_levels) {
  // Get source and destination tile parameters
  int sstride = XI_TILE_GET_PITCH(src);
  int swidth = XI_TILE_GET_WIDTH(src);
  int sheight = XI_TILE_GET_HEIGHT(src);
  int edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  int edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
  int dstride = XI_TILE_GET_PITCH(dst);
  int width = XI_TILE_GET_WIDTH(dst);
  int height = XI_TILE_GET_HEIGHT(dst);
  uint8_t *psrc = (uint8_t *)(XI_TILE_GET_DATA_PTR(src));
  uint8_t *pdst = (uint8_t *)(XI_TILE_GET_DATA_PTR(dst));

  int x, y, k, l;

  float *pgPyramid_start, *pDataGPyramid, *pBufferGPyramidNext,
      *pDataGPyramidNext;
  float *pDataLPyramid, *pBufferLPyramid, *pBufferLPyramidNext;
  float *pBufferGPyramid, *pBufferGPyramidPrev, *pBufferLPyramidPrev,
      *pDataGPyramidPrev, *pDataLPyramidPrev;
  float *pBufferOutLPyramid, *pBufferOutGPyramid, *pDataOutLPyramid,
      *pDataOutGPyramid;
  float *pBufferOutGPyramidPrev, *pDataOutGPyramidPrev;
  float *pBufferOutLPyramidPrev, *pDataOutLPyramidPrev;
  int GPyramidWidth, GPyramidHeight, GPyramidXStride, GPyramidYStride,
      GPyramidEdge;
  int GPyramidWidthNext, GPyramidHeightNext, GPyramidXStrideNext,
      GPyramidYStrideNext, GPyramidEdgeNext;
  int GPyramidWidthPrev, GPyramidHeightPrev, GPyramidXStridePrev,
      GPyramidYStridePrev, GPyramidEdgePrev;
  int LPyramidWidth, LPyramidHeight, LPyramidXStride, LPyramidYStride;
  int LPyramidWidthPrev, LPyramidHeightPrev, LPyramidXStridePrev,
      LPyramidYStridePrev, LPyramidEdgePrev, LPyramidEdge;
  int OutLPyramidWidth, OutLPyramidHeight, OutLPyramidXStride, OutLPyramidEdge;
  int OutLPyramidWidthPrev, OutLPyramidHeightPrev,
      OutLPyramidXStridePrev /* , OutLPyramidYStridePrev */,
      OutLPyramidEdgePrev;
  int OutGPyramidWidth, OutGPyramidHeight,
      OutGPyramidXStride /* , OutGPyramidYStride */, OutGPyramidEdge;
  int OutGPyramidWidthPrev, OutGPyramidHeightPrev,
      OutGPyramidXStridePrev /* , OutGPyramidYStridePrev */,
      OutGPyramidEdgePrev;
  // Make the processed Gaussian pyramid.
  pgPyramid_start = InGaussianPyramid;
  GPyramidEdge = 1;
  GPyramidWidth = swidth;
  GPyramidHeight = sheight;
  GPyramidXStride = GPyramidWidth + 2 * GPyramidEdge;
  GPyramidYStride = GPyramidXStride * (GPyramidHeight + 2 * GPyramidEdge);
  pDataGPyramid =
      pgPyramid_start + (GPyramidEdge * GPyramidXStride) + GPyramidEdge;
  // Gaussian Pyramid Level - 0
  for (x = -GPyramidEdge; x < (GPyramidWidth + GPyramidEdge); x++) {
    for (y = -GPyramidEdge; y < (GPyramidHeight + GPyramidEdge); y++) {
      float grayF;
      int idx;
      uint8_t gray;
      gray = psrc[y * sstride + x];
      grayF = gray / 255.0;
      idx = (int)(grayF * (levels - 1) * 256.0);
      idx = CLIP(idx, 0, (levels - 1) * 256);
      for (k = 0; k < levels; k++) {
        float level;
        level = k * (1.0f / (levels - 1));
        pDataGPyramid[(k * GPyramidYStride) + (y * GPyramidXStride) + x] =
            beta * (grayF - level) + level + remap(idx - (256 * k), alpha);
      }
    }
  }

  // downscale filter
  int kernel[9] = {1, 2, 1, 2, 4, 2, 1, 2, 1};

  // Form the remaining pyramid levels in the order of decreasing resolution
  pgPyramid_start = InGaussianPyramid;
  GPyramidEdge = 1;
  GPyramidEdgeNext = 1;
  GPyramidWidth = swidth;
  GPyramidXStride = GPyramidWidth + 2 * GPyramidEdge;
  GPyramidWidthNext = swidth;
  GPyramidHeightNext = sheight;
  pBufferGPyramidNext = pgPyramid_start;
  pDataGPyramidNext =
      pgPyramid_start + (GPyramidEdge * GPyramidXStride) + GPyramidEdge;
  // Laplacian pointer update
  pBufferLPyramidNext = InLaplacianPyramid;
  for (l = 1; l < pyramid_levels; l++) // loop across pyramid levels
  {

    GPyramidWidth = GPyramidWidthNext;
    GPyramidHeight = GPyramidHeightNext;
    GPyramidWidthNext = GPyramidWidthNext / 2;
    GPyramidHeightNext = GPyramidHeightNext / 2;
    GPyramidXStride = GPyramidWidth + 2 * GPyramidEdge;
    GPyramidYStride = GPyramidXStride * (GPyramidHeight + 2 * GPyramidEdge);
    GPyramidXStrideNext = GPyramidWidthNext + 2 * GPyramidEdgeNext;
    GPyramidYStrideNext =
        GPyramidXStrideNext * (GPyramidHeightNext + 2 * GPyramidEdgeNext);
    pBufferGPyramidNext = pBufferGPyramidNext + (GPyramidYStride * levels);
    pDataGPyramid = pDataGPyramidNext;
    pDataGPyramidNext = pBufferGPyramidNext +
                        (GPyramidEdgeNext * GPyramidXStrideNext) +
                        GPyramidEdgeNext;
    pBufferLPyramidNext = pBufferLPyramidNext + (GPyramidYStride * levels);
    for (k = 0; k < levels; k++) {
      for (x = 0; x < GPyramidWidthNext; x++) {
        for (y = 0; y < GPyramidHeightNext; y++) {
          float sum = 0.0;
          int ksize = 3;
          int N_div = ksize / 2;
          // downsample the pyramid (non separable implementation) with kernel
          // [1 2 1]
          for (int m = -N_div, mm = 0; m <= N_div; m++, mm++)
            for (int n = -N_div, nn = 0; n <= N_div; n++, nn++)
              sum += pDataGPyramid[(k * GPyramidYStride) +
                                   (((2 * y) + m) * GPyramidXStride) +
                                   ((2 * x) + n)] *
                     kernel[mm * ksize + nn];

          pDataGPyramidNext[(k * GPyramidYStrideNext) +
                            (y * GPyramidXStrideNext) + x] = sum / 16.0;
        }
      }
    }

    // Repeat the edges of pyramid
    for (k = 0; k < levels; k++) {
      // top edge
      for (x = -GPyramidEdgeNext; x < (GPyramidWidthNext + GPyramidEdgeNext);
           x++) {
        for (y = -GPyramidEdgeNext; y < 0; y++) {
          int x1 = CLIP(x, 0, (GPyramidWidthNext - 1));
          int y1 = CLIP(y, 0, (GPyramidHeightNext - 1));
          pDataGPyramidNext[(k * GPyramidYStrideNext) +
                            (y * GPyramidXStrideNext) + x] =
              pDataGPyramidNext[(k * GPyramidYStrideNext) +
                                (y1 * GPyramidXStrideNext) + x1];
        }
      }
      // bottom edge
      for (x = -GPyramidEdgeNext; x < (GPyramidWidthNext + GPyramidEdgeNext);
           x++) {
        for (y = GPyramidHeightNext;
             y < (GPyramidHeightNext + GPyramidEdgeNext); y++) {
          int x1 = CLIP(x, 0, (GPyramidWidthNext - 1));
          int y1 = CLIP(y, 0, (GPyramidHeightNext - 1));
          pDataGPyramidNext[(k * GPyramidYStrideNext) +
                            (y * GPyramidXStrideNext) + x] =
              pDataGPyramidNext[(k * GPyramidYStrideNext) +
                                (y1 * GPyramidXStrideNext) + x1];
        }
      }
      // left edge
      for (x = -GPyramidEdgeNext; x < 0; x++) {
        for (y = -GPyramidEdgeNext; y < (GPyramidHeightNext + GPyramidEdgeNext);
             y++) {
          int x1 = CLIP(x, 0, (GPyramidWidthNext - 1));
          int y1 = CLIP(y, 0, (GPyramidHeightNext - 1));
          pDataGPyramidNext[(k * GPyramidYStrideNext) +
                            (y * GPyramidXStrideNext) + x] =
              pDataGPyramidNext[(k * GPyramidYStrideNext) +
                                (y1 * GPyramidXStrideNext) + x1];
        }
      }
      // right edge
      for (x = GPyramidWidthNext; x < (GPyramidWidthNext + GPyramidEdgeNext);
           x++) {
        for (y = -GPyramidEdgeNext; y < (GPyramidHeightNext + GPyramidEdgeNext);
             y++) {
          int x1 = CLIP(x, 0, (GPyramidWidthNext - 1));
          int y1 = CLIP(y, 0, (GPyramidHeightNext - 1));
          pDataGPyramidNext[(k * GPyramidYStrideNext) +
                            (y * GPyramidXStrideNext) + x] =
              pDataGPyramidNext[(k * GPyramidYStrideNext) +
                                (y1 * GPyramidXStrideNext) + x1];
        }
      }
    }
  }

  // Get its laplacian pyramid
  LPyramidEdge = 1;
  LPyramidWidth = GPyramidWidthNext;
  LPyramidHeight = GPyramidHeightNext;
  LPyramidXStride = LPyramidWidth + 2 * LPyramidEdge;
  LPyramidYStride = LPyramidXStride * (LPyramidHeight + 2 * LPyramidEdge);
  GPyramidEdge = 1;
  GPyramidWidth = GPyramidWidthNext;
  GPyramidHeight = GPyramidHeightNext;
  GPyramidXStride = GPyramidWidth + 2 * GPyramidEdge;
  GPyramidYStride = GPyramidXStride * (GPyramidHeight + 2 * GPyramidEdge);
  pDataGPyramid = pDataGPyramidNext;

  pDataLPyramid =
      pBufferLPyramidNext + (LPyramidEdge * LPyramidXStride) + LPyramidEdge;
  // Copy the lowest level of processed Gaussian pyramid into Laplacian pyramid
  for (x = -LPyramidEdge; x < (LPyramidWidth + LPyramidEdge); x++) {
    for (y = -LPyramidEdge; y < (LPyramidHeight + LPyramidEdge); y++) {
      for (k = 0; k < levels; k++) {
        pDataLPyramid[(k * LPyramidYStride) + (y * LPyramidXStride) + x] =
            pDataGPyramid[(k * GPyramidYStride) + (y * GPyramidXStride) + x];
      }
    }
  }
  // Form the higher levels of laplacian pyramid in the order of increasing
  // resolution
  {
    GPyramidEdge = 1;
    GPyramidEdgePrev = 1;
    GPyramidWidthPrev = GPyramidWidthNext;
    GPyramidHeightPrev = GPyramidHeightNext;
    GPyramidXStridePrev = GPyramidWidthPrev + 2 * GPyramidEdgePrev;
    pBufferGPyramidPrev = pBufferGPyramidNext;
    pDataGPyramidPrev = pBufferGPyramidPrev +
                        (GPyramidEdgePrev * GPyramidXStridePrev) +
                        GPyramidEdgePrev;
    pBufferLPyramidPrev = pBufferLPyramidNext;
    for (int j = pyramid_levels - 2; j >= 0; j--) {
      GPyramidWidth = GPyramidWidthPrev;
      GPyramidHeight = GPyramidHeightPrev;
      GPyramidWidthPrev = GPyramidWidthPrev * 2;
      GPyramidHeightPrev = GPyramidHeightPrev * 2;

      GPyramidXStride = GPyramidWidth + 2 * GPyramidEdge;
      GPyramidYStride = GPyramidXStride * (GPyramidHeight + 2 * GPyramidEdge);
      GPyramidXStridePrev = GPyramidWidthPrev + 2 * GPyramidEdgePrev;
      GPyramidYStridePrev =
          GPyramidXStridePrev * (GPyramidHeightPrev + 2 * GPyramidEdgePrev);
      LPyramidXStridePrev = GPyramidXStridePrev;
      LPyramidYStridePrev = GPyramidYStridePrev;

      pBufferGPyramidPrev =
          pBufferGPyramidPrev - (GPyramidYStridePrev * levels);
      pDataGPyramid = pDataGPyramidPrev;
      pDataGPyramidPrev = pBufferGPyramidPrev +
                          (GPyramidEdgePrev * GPyramidXStridePrev) +
                          GPyramidEdgePrev;
      pBufferLPyramidPrev =
          pBufferLPyramidPrev - (LPyramidYStridePrev * levels);
      pDataLPyramidPrev = pBufferLPyramidPrev +
                          (GPyramidEdgePrev * LPyramidXStridePrev) +
                          GPyramidEdgePrev;

      // Upsample (non separable implementation) the gaussian pyramid at level j
      // and substract it from gaussian pyramid at high resolution level j - 1
      for (x = 0; x < GPyramidWidthPrev; x++) {
        for (y = 0; y < GPyramidHeightPrev; y++) {
          for (k = 0; k < levels; k++) {
            float upsampleVal;
            int x1, y1, x2, y2, x3, y3, x4, y4;
            x1 = (x / 2);
            y1 = (y / 2);
            x2 = (x / 2) + (x % 2);
            y2 = (y / 2);
            x3 = (x / 2);
            y3 = (y / 2) + (y % 2);
            x4 = (x / 2);
            y4 = (y / 2) + (y % 2);

            upsampleVal = 0.25 * pDataGPyramid[(k * GPyramidYStride) +
                                               (y1 * GPyramidXStride) + x1] +
                          0.25 * pDataGPyramid[(k * GPyramidYStride) +
                                               (y2 * GPyramidXStride) + x2] +
                          0.25 * pDataGPyramid[(k * GPyramidYStride) +
                                               (y3 * GPyramidXStride) + x3] +
                          0.25 * pDataGPyramid[(k * GPyramidYStride) +
                                               (y4 * GPyramidXStride) + x4];
            pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                              (y * LPyramidXStridePrev) + x] =
                pDataGPyramidPrev[(k * GPyramidYStridePrev) +
                                  (y * GPyramidXStridePrev) + x] -
                upsampleVal;
          }
        }
      }
      LPyramidEdgePrev = GPyramidEdgePrev;
      LPyramidWidthPrev = GPyramidWidthPrev;
      LPyramidHeightPrev = GPyramidHeightPrev;
      // Repeat the edges of pyramid
      for (k = 0; k < levels; k++) {
        // top edge
        for (x = -LPyramidEdgePrev; x < (LPyramidWidthPrev + LPyramidEdgePrev);
             x++) {
          for (y = -LPyramidEdgePrev; y < 0; y++) {
            int x1 = CLIP(x, 0, (LPyramidWidthPrev - 1));
            int y1 = CLIP(y, 0, (LPyramidHeightPrev - 1));
            pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                              (y * LPyramidXStridePrev) + x] =
                pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                                  (y1 * LPyramidXStridePrev) + x1];
          }
        }
        // bottom edge
        for (x = -LPyramidEdgePrev; x < (LPyramidWidthPrev + LPyramidEdgePrev);
             x++) {
          for (y = LPyramidHeightPrev;
               y < (LPyramidHeightPrev + LPyramidEdgePrev); y++) {
            int x1 = CLIP(x, 0, (LPyramidWidthPrev - 1));
            int y1 = CLIP(y, 0, (LPyramidHeightPrev - 1));
            pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                              (y * LPyramidXStridePrev) + x] =
                pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                                  (y1 * LPyramidXStridePrev) + x1];
          }
        }
        // left edge
        for (x = -LPyramidEdgePrev; x < 0; x++) {
          for (y = -LPyramidEdgePrev;
               y < (LPyramidHeightPrev + LPyramidEdgePrev); y++) {
            int x1 = CLIP(x, 0, (LPyramidWidthPrev - 1));
            int y1 = CLIP(y, 0, (LPyramidHeightPrev - 1));
            pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                              (y * LPyramidXStridePrev) + x] =
                pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                                  (y1 * LPyramidXStridePrev) + x1];
          }
        }
        // right edge
        for (x = LPyramidWidthPrev; x < (LPyramidWidthPrev + LPyramidEdgePrev);
             x++) {
          for (y = -LPyramidEdgePrev;
               y < (LPyramidHeightPrev + LPyramidEdgePrev); y++) {
            int x1 = CLIP(x, 0, (LPyramidWidthPrev - 1));
            int y1 = CLIP(y, 0, (LPyramidHeightPrev - 1));
            pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                              (y * LPyramidXStridePrev) + x] =
                pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                                  (y1 * LPyramidXStridePrev) + x1];
          }
        }
      }
    }
  }

  // Make the Gaussian pyramid of the input
  GPyramidEdge = 1;
  GPyramidWidth = swidth;
  GPyramidHeight = sheight;
  GPyramidXStride = GPyramidWidth + 2 * GPyramidEdge;
  pDataGPyramid =
      GaussianPyramid + (GPyramidEdge * GPyramidXStride) + GPyramidEdge;
  // Pyramid Level - 0
  for (x = -GPyramidEdge; x < (GPyramidWidth + GPyramidEdge); x++) {
    for (y = -GPyramidEdge; y < (GPyramidHeight + GPyramidEdge); y++) {
      float grayF;
      uint8_t gray;
      gray = psrc[y * sstride + x];
      grayF = gray / 255.0;
      pDataGPyramid[(y * GPyramidXStride) + x] = grayF;
    }
  }
  // Form the remaining pyramid levels in the order of decreasing resolution
  GPyramidEdge = 1;
  GPyramidEdgeNext = 1;
  GPyramidWidth = swidth;
  GPyramidXStride = GPyramidWidth + 2 * GPyramidEdge;
  GPyramidWidthNext = swidth;
  GPyramidHeightNext = sheight;
  pBufferGPyramidNext = GaussianPyramid;
  pDataGPyramidNext =
      pBufferGPyramidNext + (GPyramidEdge * GPyramidXStride) + GPyramidEdge;
  for (l = 1; l < pyramid_levels; l++) // loop across pyramid levels
  {
    GPyramidWidth = GPyramidWidthNext;
    GPyramidHeight = GPyramidHeightNext;
    GPyramidWidthNext = GPyramidWidthNext / 2;
    GPyramidHeightNext = GPyramidHeightNext / 2;
    GPyramidXStride = GPyramidWidth + 2 * GPyramidEdge;
    GPyramidXStrideNext = GPyramidWidthNext + 2 * GPyramidEdgeNext;
    pBufferGPyramidNext =
        pBufferGPyramidNext +
        (GPyramidXStride * (GPyramidHeight + 2 * GPyramidEdge));
    pDataGPyramid = pDataGPyramidNext;
    pDataGPyramidNext = pBufferGPyramidNext +
                        (GPyramidEdgeNext * GPyramidXStrideNext) +
                        GPyramidEdgeNext;
    // downsample the pyramid (non separable implementation)
    for (x = 0; x < GPyramidWidthNext; x++) {
      for (y = 0; y < GPyramidHeightNext; y++) {
        float sum = 0.0;
        int ksize = 3;
        int N_div = ksize / 2;
        for (int m = -N_div, mm = 0; m <= N_div; m++, mm++)
          for (int n = -N_div, nn = 0; n <= N_div; n++, nn++)
            sum += pDataGPyramid[(((2 * y) + m) * GPyramidXStride) +
                                 ((2 * x) + n)] *
                   kernel[mm * ksize + nn];

        pDataGPyramidNext[(y * GPyramidXStrideNext) + x] = sum / 16.0;
      }
    }
    // Repeat the edges of pyramid
    // top edge
    for (x = -GPyramidEdgeNext; x < (GPyramidWidthNext + GPyramidEdgeNext);
         x++) {
      for (y = -GPyramidEdgeNext; y < 0; y++) {
        int x1 = CLIP(x, 0, (GPyramidWidthNext - 1));
        int y1 = CLIP(y, 0, (GPyramidHeightNext - 1));
        pDataGPyramidNext[(y * GPyramidXStrideNext) + x] =
            pDataGPyramidNext[(y1 * GPyramidXStrideNext) + x1];
      }
    }
    // bottom edge
    for (x = -GPyramidEdgeNext; x < (GPyramidWidthNext + GPyramidEdgeNext);
         x++) {
      for (y = GPyramidHeightNext; y < (GPyramidHeightNext + GPyramidEdgeNext);
           y++) {
        int x1 = CLIP(x, 0, (GPyramidWidthNext - 1));
        int y1 = CLIP(y, 0, (GPyramidHeightNext - 1));
        pDataGPyramidNext[(y * GPyramidXStrideNext) + x] =
            pDataGPyramidNext[(y1 * GPyramidXStrideNext) + x1];
      }
    }
    // left edge
    for (x = -GPyramidEdgeNext; x < 0; x++) {
      for (y = -GPyramidEdgeNext; y < (GPyramidHeightNext + GPyramidEdgeNext);
           y++) {
        int x1 = CLIP(x, 0, (GPyramidWidthNext - 1));
        int y1 = CLIP(y, 0, (GPyramidHeightNext - 1));
        pDataGPyramidNext[(y * GPyramidXStrideNext) + x] =
            pDataGPyramidNext[(y1 * GPyramidXStrideNext) + x1];
      }
    }
    // right edge
    for (x = GPyramidWidthNext; x < (GPyramidWidthNext + GPyramidEdgeNext);
         x++) {
      for (y = -GPyramidEdgeNext; y < (GPyramidHeightNext + GPyramidEdgeNext);
           y++) {
        int x1 = CLIP(x, 0, (GPyramidWidthNext - 1));
        int y1 = CLIP(y, 0, (GPyramidHeightNext - 1));
        pDataGPyramidNext[(y * GPyramidXStrideNext) + x] =
            pDataGPyramidNext[(y1 * GPyramidXStrideNext) + x1];
      }
    }
  }

  // Make the laplacian pyramid of the output
  OutLPyramidEdge = 1;
  GPyramidEdge = 1;
  LPyramidEdge = 1;
  OutLPyramidWidth = swidth;
  OutLPyramidHeight = sheight;
  OutLPyramidXStride = OutLPyramidWidth + (2 * OutLPyramidEdge);
  GPyramidXStride = swidth + (2 * GPyramidEdge);
  LPyramidWidth = swidth;
  LPyramidHeight = sheight;
  LPyramidXStride = LPyramidWidth + 2 * LPyramidEdge;
  LPyramidYStride = LPyramidXStride * (LPyramidHeight + 2 * LPyramidEdge);
  GPyramidHeight = sheight;
  pBufferLPyramid = InLaplacianPyramid;
  pDataLPyramid =
      pBufferLPyramid + (LPyramidEdge * LPyramidXStride) + LPyramidEdge;
  pBufferGPyramid = GaussianPyramid;
  pDataGPyramid =
      pBufferGPyramid + (GPyramidEdge * GPyramidXStride) + GPyramidEdge;
  pBufferOutLPyramid = OutLaplacianPyramid;
  pDataOutLPyramid = pBufferOutLPyramid +
                     (OutLPyramidEdge * OutLPyramidXStride) + OutLPyramidEdge;
  for (l = 0; l < pyramid_levels; l++) // loop across pyramid levels
  {
    for (x = -OutLPyramidEdge; x < (OutLPyramidWidth + OutLPyramidEdge); x++) {
      for (y = -OutLPyramidEdge; y < (OutLPyramidHeight + OutLPyramidEdge);
           y++) {
        // Split input pyramid value into integer and floating parts
        float level =
            (pDataGPyramid[(y * GPyramidXStride) + x] * (float)(levels - 1));
        int li = CLIP((int)level, 0, levels - 2);
        float lf = level - (float)li;
        pDataOutLPyramid[(y * OutLPyramidXStride) + x] =
            (1.0 - lf) * pDataLPyramid[(li * LPyramidYStride) +
                                       (y * LPyramidXStride) + x] +
            lf * pDataLPyramid[((li + 1) * LPyramidYStride) +
                               (y * LPyramidXStride) + x];
      }
    }

    pBufferLPyramid = pBufferLPyramid + (LPyramidYStride * levels);
    pBufferGPyramid = pBufferGPyramid +
                      (GPyramidXStride * (GPyramidHeight + 2 * GPyramidEdge));
    pBufferOutLPyramid =
        pBufferOutLPyramid +
        (OutLPyramidXStride * (OutLPyramidHeight + 2 * OutLPyramidEdge));
    OutLPyramidWidth = OutLPyramidWidth / 2;
    OutLPyramidHeight = OutLPyramidHeight / 2;
    OutLPyramidXStride = OutLPyramidWidth + (2 * OutLPyramidEdge);
    GPyramidXStride = OutLPyramidWidth + (2 * GPyramidEdge);
    GPyramidHeight = OutLPyramidHeight;
    LPyramidWidth = OutLPyramidWidth;
    LPyramidHeight = OutLPyramidHeight;
    LPyramidXStride = LPyramidWidth + 2 * LPyramidEdge;
    LPyramidYStride = LPyramidXStride * (LPyramidHeight + 2 * LPyramidEdge);
    pDataLPyramid =
        pBufferLPyramid + (LPyramidEdge * LPyramidXStride) + LPyramidEdge;
    pDataGPyramid =
        pBufferGPyramid + (GPyramidEdge * GPyramidXStride) + GPyramidEdge;
    pDataOutLPyramid = pBufferOutLPyramid +
                       (OutLPyramidEdge * OutLPyramidXStride) + OutLPyramidEdge;
  }

  OutLPyramidEdge = 1;
  OutGPyramidEdge = 1;
  OutLPyramidWidth = swidth;
  OutLPyramidHeight = sheight;
  OutGPyramidWidth = swidth;
  OutGPyramidHeight = sheight;
  OutLPyramidXStride = OutLPyramidWidth + (2 * OutLPyramidEdge);
  OutGPyramidXStride = OutGPyramidWidth + (2 * OutGPyramidEdge);
  pBufferOutLPyramid = OutLaplacianPyramid;
  pBufferOutGPyramid = OutGaussianPyramid;
  for (l = 1; l < pyramid_levels; l++) // loop across pyramid levels
  {
    pBufferOutLPyramid =
        pBufferOutLPyramid +
        (OutLPyramidXStride * (OutLPyramidHeight + 2 * OutLPyramidEdge));
    pBufferOutGPyramid =
        pBufferOutGPyramid +
        (OutGPyramidXStride * (OutGPyramidHeight + 2 * OutGPyramidEdge));
    OutLPyramidWidth = OutLPyramidWidth / 2;
    OutLPyramidHeight = OutLPyramidHeight / 2;
    OutGPyramidWidth = OutGPyramidWidth / 2;
    OutGPyramidHeight = OutGPyramidHeight / 2;
    OutLPyramidXStride = OutLPyramidWidth + (2 * OutLPyramidEdge);
    OutGPyramidXStride = OutGPyramidWidth + (2 * OutGPyramidEdge);
  }

  // Make the Gaussian pyramid of the output
  pDataOutLPyramid = pBufferOutLPyramid +
                     (OutLPyramidXStride * OutLPyramidEdge) + OutLPyramidEdge;
  pDataOutGPyramid = pBufferOutGPyramid +
                     (OutGPyramidXStride * OutGPyramidEdge) + OutGPyramidEdge;
  // last scale of pyramid
  for (x = -OutGPyramidEdge; x < (OutGPyramidWidth + OutGPyramidEdge); x++) {
    for (y = -OutGPyramidEdge; y < (OutGPyramidHeight + OutGPyramidEdge); y++) {
      // Copy the values of  lower scale laplacian pyramid into lower scale
      // gaussian pyramid
      pDataOutGPyramid[(y * OutGPyramidXStride) + x] =
          pDataOutLPyramid[(y * OutLPyramidXStride) + x];
    }
  }
  {
    OutGPyramidEdge = 1;
    OutGPyramidEdgePrev = 1;
    OutLPyramidEdgePrev = 1;
    OutGPyramidWidthPrev = OutGPyramidWidth;
    OutGPyramidHeightPrev = OutGPyramidHeight;
    OutGPyramidXStride = OutGPyramidWidth + 2 * OutGPyramidEdge;
    OutLPyramidWidthPrev = OutLPyramidWidth;
    OutLPyramidHeightPrev = OutLPyramidHeight;
    OutLPyramidXStridePrev = OutLPyramidWidth + 2 * OutLPyramidEdgePrev;
    pBufferOutGPyramidPrev = pBufferOutGPyramid;
    pDataOutGPyramidPrev = pBufferOutGPyramidPrev +
                           (OutGPyramidXStride * OutGPyramidEdge) +
                           OutGPyramidEdge;
    pBufferOutLPyramidPrev = pBufferOutLPyramid;

    for (int j = pyramid_levels - 2; j >= 0; j--) {
      OutGPyramidWidth = OutGPyramidWidthPrev;
      OutGPyramidHeight = OutGPyramidHeightPrev;
      OutGPyramidWidthPrev = OutGPyramidWidthPrev * 2;
      OutGPyramidHeightPrev = OutGPyramidHeightPrev * 2;
      OutGPyramidXStride = OutGPyramidWidth + 2 * OutGPyramidEdge;
      // OutGPyramidYStride = OutGPyramidXStride * (OutGPyramidHeight + 2 *
      // GPyramidEdge);
      OutGPyramidXStridePrev = OutGPyramidWidthPrev + 2 * OutGPyramidEdgePrev;
      // OutGPyramidYStridePrev = OutGPyramidXStridePrev *
      // (OutGPyramidHeightPrev + 2 * OutGPyramidEdgePrev);
      OutLPyramidWidthPrev = OutLPyramidWidthPrev * 2;
      OutLPyramidHeightPrev = OutLPyramidHeightPrev * 2;
      OutLPyramidXStridePrev = OutLPyramidWidthPrev + 2 * OutLPyramidEdgePrev;
      // OutLPyramidYStridePrev = OutLPyramidXStridePrev *
      // (OutLPyramidHeightPrev + 2 * OutLPyramidEdgePrev);
      pBufferOutGPyramidPrev =
          pBufferOutGPyramidPrev -
          (OutGPyramidXStridePrev *
           (OutGPyramidHeightPrev + 2 * OutGPyramidEdgePrev));
      pDataOutGPyramid = pDataOutGPyramidPrev;
      pDataOutGPyramidPrev = pBufferOutGPyramidPrev +
                             (OutGPyramidEdgePrev * OutGPyramidXStridePrev) +
                             OutGPyramidEdgePrev;
      pBufferOutLPyramidPrev =
          pBufferOutLPyramidPrev -
          (OutLPyramidXStridePrev *
           (OutLPyramidHeightPrev + 2 * OutLPyramidEdgePrev));
      pDataOutLPyramidPrev = pBufferOutLPyramidPrev +
                             (OutLPyramidEdgePrev * OutLPyramidXStridePrev) +
                             OutLPyramidEdgePrev;
      // Upsample (non separable implementation) the 2D gaussian pyramid at
      // scale n + 1 and add it to scale n of 2D output laplacian pyramid
      for (x = 0; x < OutGPyramidWidthPrev; x++) {
        for (y = 0; y < OutGPyramidHeightPrev; y++) {
          float upsampleVal;
          int x1, y1, x2, y2, x3, y3, x4, y4;
          x1 = (x / 2);
          y1 = (y / 2);
          x2 = (x / 2) + (x % 2);
          y2 = (y / 2);
          x3 = (x / 2);
          y3 = (y / 2) + (y % 2);
          x4 = (x / 2);
          y4 = (y / 2) + (y % 2);
          upsampleVal =
              0.25 * pDataOutGPyramid[(y1 * OutGPyramidXStride) + x1] +
              0.25 * pDataOutGPyramid[(y2 * OutGPyramidXStride) + x2] +
              0.25 * pDataOutGPyramid[(y3 * OutGPyramidXStride) + x3] +
              0.25 * pDataOutGPyramid[(y4 * OutGPyramidXStride) + x4];
          pDataOutGPyramidPrev[(y * OutGPyramidXStridePrev) + x] =
              pDataOutLPyramidPrev[(y * OutLPyramidXStridePrev) + x] +
              upsampleVal;
        }
      }
      // Repeat the edges of pyramid
      // top edge
      for (x = -OutGPyramidEdgePrev;
           x < (OutGPyramidWidthPrev + OutGPyramidEdgePrev); x++) {
        for (y = -OutGPyramidEdgePrev; y < 0; y++) {
          int x1 = CLIP(x, 0, (OutGPyramidWidthPrev - 1));
          int y1 = CLIP(y, 0, (OutGPyramidHeightPrev - 1));
          pDataOutGPyramidPrev[(y * OutGPyramidXStridePrev) + x] =
              pDataOutGPyramidPrev[(y1 * OutGPyramidXStridePrev) + x1];
        }
      }
      // bottom edge
      for (x = -OutGPyramidEdgePrev;
           x < (OutGPyramidWidthPrev + OutGPyramidEdgePrev); x++) {
        for (y = OutGPyramidHeightPrev;
             y < (OutGPyramidHeightPrev + OutGPyramidEdgePrev); y++) {
          int x1 = CLIP(x, 0, (OutGPyramidWidthPrev - 1));
          int y1 = CLIP(y, 0, (OutGPyramidHeightPrev - 1));
          pDataOutGPyramidPrev[(y * OutGPyramidXStridePrev) + x] =
              pDataOutGPyramidPrev[(y1 * OutGPyramidXStridePrev) + x1];
        }
      }
      // left edge
      for (x = -OutGPyramidEdgePrev; x < 0; x++) {
        for (y = -OutGPyramidEdgePrev;
             y < (OutGPyramidHeightPrev + OutGPyramidEdgePrev); y++) {
          int x1 = CLIP(x, 0, (OutGPyramidWidthPrev - 1));
          int y1 = CLIP(y, 0, (OutGPyramidHeightPrev - 1));
          pDataOutGPyramidPrev[(y * OutGPyramidXStridePrev) + x] =
              pDataOutGPyramidPrev[(y1 * OutGPyramidXStridePrev) + x1];
        }
      }
      // right edge
      for (x = OutGPyramidWidthPrev;
           x < (OutGPyramidWidthPrev + OutGPyramidEdgePrev); x++) {
        for (y = -OutGPyramidEdgePrev;
             y < (OutGPyramidHeightPrev + OutGPyramidEdgePrev); y++) {
          int x1 = CLIP(x, 0, (OutGPyramidWidthPrev - 1));
          int y1 = CLIP(y, 0, (OutGPyramidHeightPrev - 1));
          pDataOutGPyramidPrev[(y * OutGPyramidXStridePrev) + x] =
              pDataOutGPyramidPrev[(y1 * OutGPyramidXStridePrev) + x1];
        }
      }
    }
  }

  pDataOutGPyramid = OutGaussianPyramid +
                     ((swidth + (2 * edge_width)) * edge_height) + edge_width;
  for (x = 0; x < width; x++) {
    for (y = 0; y < height; y++) {
      pdst[y * dstride + x] = (uint8_t)CLIP(
          (int)(255 * pDataOutGPyramid[y * (swidth + 2 * edge_width) + x]), 0,
          255);
    }
  }
  return (XI_ERR_OK);
}

int16_t GaussianPyramid_fx[PYRAMID3DL1SIZE + PYRAMID3DL2SIZE + PYRAMID3DL3SIZE];
int16_t
    InLaplacianPyramid_fx[PYRAMID3DL1SIZE + PYRAMID3DL2SIZE + PYRAMID3DL3SIZE];
uint8_t
    InGaussianPyramid_fx[PYRAMID2DL1SIZE + PYRAMID2DL2SIZE + PYRAMID2DL3SIZE];
int16_t
    OutLaplacianPyramid_fx[PYRAMID2DL1SIZE + PYRAMID2DL2SIZE + PYRAMID2DL3SIZE];
int16_t
    OutGaussianPyramid_fx[PYRAMID2DL1SIZE + PYRAMID2DL2SIZE + PYRAMID2DL3SIZE];
XI_ERR_TYPE xiLocalLaplacian_Gray_U8_FixPt_ref(const xi_pTile src, xi_pTile dst,
                                               int levels, int pyramid_levels,
                                               int16_t *remap_table) {
  // Get source and destination tile parameters
  int sstride = XI_TILE_GET_PITCH(src);
  int swidth = XI_TILE_GET_WIDTH(src);
  int sheight = XI_TILE_GET_HEIGHT(src);
  int edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  int edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
  int dstride = XI_TILE_GET_PITCH(dst);
  int width = XI_TILE_GET_WIDTH(dst);
  int height = XI_TILE_GET_HEIGHT(dst);
  uint8_t *psrc = (uint8_t *)(XI_TILE_GET_DATA_PTR(src));
  uint8_t *pdst = (uint8_t *)(XI_TILE_GET_DATA_PTR(dst));

  int x, y, k, l;
  int16_t *pDataGPyramid, *pBufferGPyramidNext, *pDataGPyramidNext;
  int16_t *pDataLPyramid, *pBufferLPyramid, *pBufferLPyramidNext;
  int16_t *pBufferLPyramidPrev, *pDataLPyramidPrev;
  int16_t *pBufferGPyramidPrev, *pDataGPyramidPrev;
  int16_t *pBufferOutGPyramid, *pDataOutGPyramid;
  int16_t *pBufferOutLPyramid, *pDataOutLPyramid;
  int16_t *pBufferOutGPyramidPrev, *pDataOutGPyramidPrev;
  int16_t *pBufferOutLPyramidPrev, *pDataOutLPyramidPrev;
  uint8_t *pBufferInGPyramid, *pDataInGPyramid, *pBufferInGPyramidNext,
      *pDataInGPyramidNext;
  int GPyramidWidth, GPyramidHeight, GPyramidXStride, GPyramidYStride,
      GPyramidEdge;
  int GPyramidWidthNext, GPyramidHeightNext, GPyramidXStrideNext,
      GPyramidYStrideNext, GPyramidEdgeNext;
  int GPyramidWidthPrev, GPyramidHeightPrev, GPyramidXStridePrev,
      GPyramidYStridePrev, GPyramidEdgePrev;
  int LPyramidWidth, LPyramidHeight, LPyramidXStride, LPyramidYStride;
  int LPyramidWidthPrev, LPyramidHeightPrev, LPyramidXStridePrev,
      LPyramidYStridePrev, LPyramidEdgePrev, LPyramidEdge;
  int OutLPyramidWidth, OutLPyramidHeight, OutLPyramidXStride, OutLPyramidEdge;
  int OutLPyramidWidthPrev, OutLPyramidHeightPrev,
      OutLPyramidXStridePrev /* , OutLPyramidYStridePrev */,
      OutLPyramidEdgePrev;
  int OutGPyramidWidth, OutGPyramidHeight,
      OutGPyramidXStride /* , OutGPyramidYStride */, OutGPyramidEdge;
  int OutGPyramidWidthPrev, OutGPyramidHeightPrev,
      OutGPyramidXStridePrev /* , OutGPyramidYStridePrev */,
      OutGPyramidEdgePrev;
  // Make the processed Gaussian pyramid.
  GPyramidEdge = 1;
  GPyramidWidth = swidth;
  GPyramidHeight = sheight;
  GPyramidXStride = GPyramidWidth + 2 * GPyramidEdge;
  GPyramidYStride = GPyramidXStride * (GPyramidHeight + 2 * GPyramidEdge);
  pDataGPyramid =
      GaussianPyramid_fx + (GPyramidEdge * GPyramidXStride) + GPyramidEdge;
  // Gaussian Pyramid Level - 0
  for (x = -GPyramidEdge; x < (GPyramidWidth + GPyramidEdge); x++) {
    for (y = -GPyramidEdge; y < (GPyramidHeight + GPyramidEdge); y++) {
      uint8_t gray;
      gray = psrc[y * sstride + x];
      for (k = 0; k < levels; k++) {
        pDataGPyramid[(k * GPyramidYStride) + (y * GPyramidXStride) + x] =
            remap_table[k + gray * 8]; // pDataGPyramid//Q2.8( 1 sign + 1
                                       // integer+ 8 fractional)
      }
    }
  }
#if DEBUG_DUMP
  {
    FILE *fp;

    char fname[256];

    for (int k = 0; k < 8; k++) {
      sprintf(fname, "refGPyrLevel0_%d.yuv", k);
      fp = fopen(fname, "wb");
      fwrite(&GaussianPyramid_fx[k * GPyramidYStride], 2,
             GPyramidXStride * (GPyramidHeight + 2 * GPyramidEdge), fp);

      fclose(fp);
    }
  }
#endif
  // downscale filter
  uint8_t kernel[9] = {1, 2, 1, 2, 4, 2, 1, 2, 1};

  // Form the remaining pyramid levels in the order of decreasing resolution
  GPyramidEdge = 1;
  GPyramidEdgeNext = 1;
  GPyramidWidth = swidth;
  GPyramidXStride = GPyramidWidth + 2 * GPyramidEdge;
  GPyramidWidthNext = swidth;
  GPyramidHeightNext = sheight;
  pBufferGPyramidNext = GaussianPyramid_fx;
  pDataGPyramidNext =
      pBufferGPyramidNext + (GPyramidEdge * GPyramidXStride) + GPyramidEdge;
  // Laplacian pointer update
  pBufferLPyramidNext = InLaplacianPyramid_fx;
  for (l = 1; l < pyramid_levels; l++) // loop across pyramid levels
  {
    GPyramidWidth = GPyramidWidthNext;
    GPyramidHeight = GPyramidHeightNext;
    GPyramidWidthNext = GPyramidWidthNext / 2;
    GPyramidHeightNext = GPyramidHeightNext / 2;
    GPyramidXStride = GPyramidWidth + 2 * GPyramidEdge;
    GPyramidYStride = GPyramidXStride * (GPyramidHeight + 2 * GPyramidEdge);
    GPyramidXStrideNext = GPyramidWidthNext + 2 * GPyramidEdgeNext;
    GPyramidYStrideNext =
        GPyramidXStrideNext * (GPyramidHeightNext + 2 * GPyramidEdgeNext);
    pBufferGPyramidNext = pBufferGPyramidNext + (GPyramidYStride * levels);
    pDataGPyramid = pDataGPyramidNext;
    pDataGPyramidNext = pBufferGPyramidNext +
                        (GPyramidEdgeNext * GPyramidXStrideNext) +
                        GPyramidEdgeNext;
    pBufferLPyramidNext = pBufferLPyramidNext + (GPyramidYStride * levels);

    for (k = 0; k < levels; k++) {
      for (x = 0; x < GPyramidWidthNext; x++) {
        for (y = 0; y < GPyramidHeightNext; y++) {
          int16_t sum = 0;
          int ksize = 3;
          int N_div = ksize / 2;
          // downsample the pyramid (non separable implementation) with kernel
          // [1 2 1]
          for (int m = -N_div, mm = 0; m <= N_div; m++, mm++)
            for (int n = -N_div, nn = 0; n <= N_div; n++, nn++)
              sum += (int16_t)(pDataGPyramid[(k * GPyramidYStride) +
                                             (((2 * y) + m) * GPyramidXStride) +
                                             ((2 * x) + n)] *
                               kernel[mm * ksize + nn]);

          pDataGPyramidNext[(k * GPyramidYStrideNext) +
                            (y * GPyramidXStrideNext) + x] =
              (sum >> 4); // pDataGPyramidNext//Q2.8
        }
      }
    }

    // Repeat the edges of pyramid
    for (k = 0; k < levels; k++) {
      // top edge
      for (x = -GPyramidEdgeNext; x < (GPyramidWidthNext + GPyramidEdgeNext);
           x++) {
        for (y = -GPyramidEdgeNext; y < 0; y++) {
          int x1 = CLIP(x, 0, (GPyramidWidthNext - 1));
          int y1 = CLIP(y, 0, (GPyramidHeightNext - 1));
          pDataGPyramidNext[(k * GPyramidYStrideNext) +
                            (y * GPyramidXStrideNext) + x] =
              pDataGPyramidNext[(k * GPyramidYStrideNext) +
                                (y1 * GPyramidXStrideNext) + x1];
        }
      }
      // bottom edge
      for (x = -GPyramidEdgeNext; x < (GPyramidWidthNext + GPyramidEdgeNext);
           x++) {
        for (y = GPyramidHeightNext;
             y < (GPyramidHeightNext + GPyramidEdgeNext); y++) {
          int x1 = CLIP(x, 0, (GPyramidWidthNext - 1));
          int y1 = CLIP(y, 0, (GPyramidHeightNext - 1));
          pDataGPyramidNext[(k * GPyramidYStrideNext) +
                            (y * GPyramidXStrideNext) + x] =
              pDataGPyramidNext[(k * GPyramidYStrideNext) +
                                (y1 * GPyramidXStrideNext) + x1];
        }
      }
      // left edge
      for (x = -GPyramidEdgeNext; x < 0; x++) {
        for (y = -GPyramidEdgeNext; y < (GPyramidHeightNext + GPyramidEdgeNext);
             y++) {
          int x1 = CLIP(x, 0, (GPyramidWidthNext - 1));
          int y1 = CLIP(y, 0, (GPyramidHeightNext - 1));
          pDataGPyramidNext[(k * GPyramidYStrideNext) +
                            (y * GPyramidXStrideNext) + x] =
              pDataGPyramidNext[(k * GPyramidYStrideNext) +
                                (y1 * GPyramidXStrideNext) + x1];
        }
      }
      // right edge
      for (x = GPyramidWidthNext; x < (GPyramidWidthNext + GPyramidEdgeNext);
           x++) {
        for (y = -GPyramidEdgeNext; y < (GPyramidHeightNext + GPyramidEdgeNext);
             y++) {
          int x1 = CLIP(x, 0, (GPyramidWidthNext - 1));
          int y1 = CLIP(y, 0, (GPyramidHeightNext - 1));
          pDataGPyramidNext[(k * GPyramidYStrideNext) +
                            (y * GPyramidXStrideNext) + x] =
              pDataGPyramidNext[(k * GPyramidYStrideNext) +
                                (y1 * GPyramidXStrideNext) + x1];
        }
      }
    }
#if DEBUG_DUMP
    {
      FILE *fp;

      char fname[256];

      for (int k = 0; k < 8; k++) {
        sprintf(fname, "refGPyrLevel%d_%d.yuv", l, k);
        fp = fopen(fname, "wb");
        fwrite(&pBufferGPyramidNext[k * GPyramidYStrideNext], 2,
               GPyramidXStrideNext *
                   (GPyramidHeightNext + 2 * GPyramidEdgeNext),
               fp);

        fclose(fp);
      }
    }
#endif
  }

  // Get its laplacian pyramid
  LPyramidEdge = 1;
  LPyramidWidth = GPyramidWidthNext;
  LPyramidHeight = GPyramidHeightNext;
  LPyramidXStride = LPyramidWidth + 2 * LPyramidEdge;
  LPyramidYStride = LPyramidXStride * (LPyramidHeight + 2 * LPyramidEdge);
  GPyramidEdge = 1;
  GPyramidWidth = GPyramidWidthNext;
  GPyramidHeight = GPyramidHeightNext;
  GPyramidXStride = GPyramidWidth + 2 * GPyramidEdge;
  GPyramidYStride = GPyramidXStride * (GPyramidHeight + 2 * GPyramidEdge);
  pDataGPyramid = pDataGPyramidNext;

  pDataLPyramid =
      pBufferLPyramidNext + (LPyramidEdge * LPyramidXStride) + LPyramidEdge;
  // Copy the lowest level of processed Gaussian pyramid into Laplacian pyramid
  for (x = -LPyramidEdge; x < (LPyramidWidth + LPyramidEdge); x++) {
    for (y = -LPyramidEdge; y < (LPyramidHeight + LPyramidEdge); y++) {
      for (k = 0; k < levels; k++) {
        pDataLPyramid[(k * LPyramidYStride) + (y * LPyramidXStride) + x] =
            pDataGPyramid[(k * GPyramidYStride) + (y * GPyramidXStride) +
                          x]; // Q2.8
      }
    }
  }
  // Form the higher levels of laplacian pyramid in the order of increasing
  // resolution
  {
    GPyramidEdge = 1;
    GPyramidEdgePrev = 1;
    GPyramidWidthPrev = GPyramidWidthNext;
    GPyramidHeightPrev = GPyramidHeightNext;
    GPyramidXStridePrev = GPyramidWidthPrev + 2 * GPyramidEdgePrev;
    pBufferGPyramidPrev = pBufferGPyramidNext;
    pDataGPyramidPrev = pBufferGPyramidPrev +
                        (GPyramidEdgePrev * GPyramidXStridePrev) +
                        GPyramidEdgePrev;
    pBufferLPyramidPrev = pBufferLPyramidNext;
    for (int j = pyramid_levels - 2; j >= 0; j--) {
      GPyramidWidth = GPyramidWidthPrev;
      GPyramidHeight = GPyramidHeightPrev;
      GPyramidWidthPrev = GPyramidWidthPrev * 2;
      GPyramidHeightPrev = GPyramidHeightPrev * 2;

      GPyramidXStride = GPyramidWidth + 2 * GPyramidEdge;
      GPyramidYStride = GPyramidXStride * (GPyramidHeight + 2 * GPyramidEdge);
      GPyramidXStridePrev = GPyramidWidthPrev + 2 * GPyramidEdgePrev;
      GPyramidYStridePrev =
          GPyramidXStridePrev * (GPyramidHeightPrev + 2 * GPyramidEdgePrev);
      LPyramidXStridePrev = GPyramidXStridePrev;
      LPyramidYStridePrev = GPyramidYStridePrev;

      pBufferGPyramidPrev =
          pBufferGPyramidPrev - (GPyramidYStridePrev * levels);
      pDataGPyramid = pDataGPyramidPrev;
      pDataGPyramidPrev = pBufferGPyramidPrev +
                          (GPyramidEdgePrev * GPyramidXStridePrev) +
                          GPyramidEdgePrev;
      pBufferLPyramidPrev =
          pBufferLPyramidPrev - (LPyramidYStridePrev * levels);
      pDataLPyramidPrev = pBufferLPyramidPrev +
                          (GPyramidEdgePrev * LPyramidXStridePrev) +
                          GPyramidEdgePrev;

      // Upsample (non separable implementation) the gaussian pyramid at level j
      // and substract it from gaussian pyramid at high resolution level j - 1
      for (x = 0; x < GPyramidWidthPrev; x++) {
        for (y = 0; y < GPyramidHeightPrev; y++) {
          for (k = 0; k < levels; k++) {
            int16_t upsampleVal;
            int x1, y1, x2, y2, x3, y3, x4, y4;
            x1 = (x / 2);
            y1 = (y / 2);
            x2 = (x / 2) + (x % 2);
            y2 = (y / 2);
            x3 = (x / 2);
            y3 = (y / 2) + (y % 2);
            x4 = (x / 2) + (x % 2);
            y4 = (y / 2) + (y % 2);

            upsampleVal = pDataGPyramid[(k * GPyramidYStride) +
                                        (y1 * GPyramidXStride) + x1] +
                          pDataGPyramid[(k * GPyramidYStride) +
                                        (y2 * GPyramidXStride) + x2] +
                          pDataGPyramid[(k * GPyramidYStride) +
                                        (y3 * GPyramidXStride) + x3] +
                          pDataGPyramid[(k * GPyramidYStride) +
                                        (y4 * GPyramidXStride) + x4];
            upsampleVal = upsampleVal >> 2; // Q2.8

            pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                              (y * LPyramidXStridePrev) + x] =
                pDataGPyramidPrev[(k * GPyramidYStridePrev) +
                                  (y * GPyramidXStridePrev) + x] -
                upsampleVal; // Q3.8
          }
        }
      }
      LPyramidEdgePrev = GPyramidEdgePrev;
      LPyramidWidthPrev = GPyramidWidthPrev;
      LPyramidHeightPrev = GPyramidHeightPrev;

      // Repeat the edges of pyramid
      for (k = 0; k < levels; k++) {
        // top edge
        for (x = -LPyramidEdgePrev; x < (LPyramidWidthPrev + LPyramidEdgePrev);
             x++) {
          for (y = -LPyramidEdgePrev; y < 0; y++) {
            int x1 = CLIP(x, 0, (LPyramidWidthPrev - 1));
            int y1 = CLIP(y, 0, (LPyramidHeightPrev - 1));
            pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                              (y * LPyramidXStridePrev) + x] =
                pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                                  (y1 * LPyramidXStridePrev) + x1];
          }
        }
        // bottom edge
        for (x = -LPyramidEdgePrev; x < (LPyramidWidthPrev + LPyramidEdgePrev);
             x++) {
          for (y = LPyramidHeightPrev;
               y < (LPyramidHeightPrev + LPyramidEdgePrev); y++) {
            int x1 = CLIP(x, 0, (LPyramidWidthPrev - 1));
            int y1 = CLIP(y, 0, (LPyramidHeightPrev - 1));
            pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                              (y * LPyramidXStridePrev) + x] =
                pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                                  (y1 * LPyramidXStridePrev) + x1];
          }
        }
        // left edge
        for (x = -LPyramidEdgePrev; x < 0; x++) {
          for (y = -LPyramidEdgePrev;
               y < (LPyramidHeightPrev + LPyramidEdgePrev); y++) {
            int x1 = CLIP(x, 0, (LPyramidWidthPrev - 1));
            int y1 = CLIP(y, 0, (LPyramidHeightPrev - 1));
            pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                              (y * LPyramidXStridePrev) + x] =
                pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                                  (y1 * LPyramidXStridePrev) + x1];
          }
        }
        // right edge
        for (x = LPyramidWidthPrev; x < (LPyramidWidthPrev + LPyramidEdgePrev);
             x++) {
          for (y = -LPyramidEdgePrev;
               y < (LPyramidHeightPrev + LPyramidEdgePrev); y++) {
            int x1 = CLIP(x, 0, (LPyramidWidthPrev - 1));
            int y1 = CLIP(y, 0, (LPyramidHeightPrev - 1));
            pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                              (y * LPyramidXStridePrev) + x] =
                pDataLPyramidPrev[(k * LPyramidYStridePrev) +
                                  (y1 * LPyramidXStridePrev) + x1];
          }
        }
      }
#if DEBUG_DUMP
      {
        FILE *fp;

        char fname[256];

        for (int k = 0; k < 8; k++) {
          sprintf(fname, "refLPyrLevel%d_%d.yuv", j, k);
          fp = fopen(fname, "wb");
          fwrite(&pBufferLPyramidPrev[k * LPyramidYStridePrev], 2,
                 LPyramidXStridePrev *
                     (GPyramidHeightPrev + 2 * GPyramidEdgePrev),
                 fp);

          fclose(fp);
        }
      }
#endif
    }
  }

  // Make the Gaussian pyramid of the input
  GPyramidEdge = 1;
  GPyramidWidth = swidth;
  GPyramidHeight = sheight;
  GPyramidXStride = GPyramidWidth + 2 * GPyramidEdge;
  pDataInGPyramid =
      InGaussianPyramid_fx + (GPyramidEdge * GPyramidXStride) + GPyramidEdge;
  // Pyramid Level - 0
  for (x = -GPyramidEdge; x < (GPyramidWidth + GPyramidEdge); x++) {
    for (y = -GPyramidEdge; y < (GPyramidHeight + GPyramidEdge); y++) {
      pDataInGPyramid[(y * GPyramidXStride) + x] = psrc[y * sstride + x];
    }
  }
  // Form the remaining pyramid levels in the order of decreasing resolution
  GPyramidEdge = 1;
  GPyramidEdgeNext = 1;
  GPyramidWidth = swidth;
  GPyramidXStride = GPyramidWidth + 2 * GPyramidEdge;
  GPyramidWidthNext = swidth;
  GPyramidHeightNext = sheight;
  pBufferInGPyramidNext = InGaussianPyramid_fx;
  pDataInGPyramidNext =
      pBufferInGPyramidNext + (GPyramidEdge * GPyramidXStride) + GPyramidEdge;
  for (l = 1; l < pyramid_levels; l++) // loop across pyramid levels
  {
    GPyramidWidth = GPyramidWidthNext;
    GPyramidHeight = GPyramidHeightNext;
    GPyramidWidthNext = GPyramidWidthNext / 2;
    GPyramidHeightNext = GPyramidHeightNext / 2;
    GPyramidXStride = GPyramidWidth + 2 * GPyramidEdge;
    GPyramidXStrideNext = GPyramidWidthNext + 2 * GPyramidEdgeNext;
    pBufferInGPyramidNext =
        pBufferInGPyramidNext +
        (GPyramidXStride * (GPyramidHeight + 2 * GPyramidEdge));
    pDataInGPyramid = pDataInGPyramidNext;
    pDataInGPyramidNext = pBufferInGPyramidNext +
                          (GPyramidEdgeNext * GPyramidXStrideNext) +
                          GPyramidEdgeNext;
    // downsample the pyramid (non separable implementation)
    for (x = 0; x < GPyramidWidthNext; x++) {
      for (y = 0; y < GPyramidHeightNext; y++) {
        int16_t sum = 0;
        int ksize = 3;
        int N_div = ksize / 2;
        // downsample the pyramid (non separable implementation) with kernel [1
        // 2 1]
        for (int m = -N_div, mm = 0; m <= N_div; m++, mm++)
          for (int n = -N_div, nn = 0; n <= N_div; n++, nn++)
            sum += (int16_t)(pDataInGPyramid[(((2 * y) + m) * GPyramidXStride) +
                                             ((2 * x) + n)] *
                             kernel[mm * ksize + nn]);

        pDataInGPyramidNext[(y * GPyramidXStrideNext) + x] = (uint8_t)(
            sum >> 4); // pDataGPyramidNext//Q8 in 8 bits (no sign bit)
      }
    }

    // Repeat the edges of pyramid
    // top edge
    for (x = -GPyramidEdgeNext; x < (GPyramidWidthNext + GPyramidEdgeNext);
         x++) {
      for (y = -GPyramidEdgeNext; y < 0; y++) {
        int x1 = CLIP(x, 0, (GPyramidWidthNext - 1));
        int y1 = CLIP(y, 0, (GPyramidHeightNext - 1));
        pDataInGPyramidNext[(y * GPyramidXStrideNext) + x] =
            pDataInGPyramidNext[(y1 * GPyramidXStrideNext) + x1];
      }
    }
    // bottom edge
    for (x = -GPyramidEdgeNext; x < (GPyramidWidthNext + GPyramidEdgeNext);
         x++) {
      for (y = GPyramidHeightNext; y < (GPyramidHeightNext + GPyramidEdgeNext);
           y++) {
        int x1 = CLIP(x, 0, (GPyramidWidthNext - 1));
        int y1 = CLIP(y, 0, (GPyramidHeightNext - 1));
        pDataInGPyramidNext[(y * GPyramidXStrideNext) + x] =
            pDataInGPyramidNext[(y1 * GPyramidXStrideNext) + x1];
      }
    }
    // left edge
    for (x = -GPyramidEdgeNext; x < 0; x++) {
      for (y = -GPyramidEdgeNext; y < (GPyramidHeightNext + GPyramidEdgeNext);
           y++) {
        int x1 = CLIP(x, 0, (GPyramidWidthNext - 1));
        int y1 = CLIP(y, 0, (GPyramidHeightNext - 1));
        pDataInGPyramidNext[(y * GPyramidXStrideNext) + x] =
            pDataInGPyramidNext[(y1 * GPyramidXStrideNext) + x1];
      }
    }
    // right edge
    for (x = GPyramidWidthNext; x < (GPyramidWidthNext + GPyramidEdgeNext);
         x++) {
      for (y = -GPyramidEdgeNext; y < (GPyramidHeightNext + GPyramidEdgeNext);
           y++) {
        int x1 = CLIP(x, 0, (GPyramidWidthNext - 1));
        int y1 = CLIP(y, 0, (GPyramidHeightNext - 1));
        pDataInGPyramidNext[(y * GPyramidXStrideNext) + x] =
            pDataInGPyramidNext[(y1 * GPyramidXStrideNext) + x1];
      }
    }
#if DEBUG_DUMP
    {
      FILE *fp;

      char fname[256];

      for (int k = 0; k < 1; k++) {
        sprintf(fname, "refGrayLevel%d.yuv", l);
        fp = fopen(fname, "wb");

        fwrite(&pBufferInGPyramidNext[0], 2,
               GPyramidXStrideNext *
                   (GPyramidHeightNext + 2 * GPyramidEdgeNext),
               fp);

        fclose(fp);
      }
    }
#endif
  }

  // Make the laplacian pyramid of the output
  OutLPyramidEdge = 1;
  GPyramidEdge = 1;
  LPyramidEdge = 1;
  OutLPyramidWidth = swidth;
  OutLPyramidHeight = sheight;
  OutLPyramidXStride = OutLPyramidWidth + (2 * OutLPyramidEdge);
  GPyramidXStride = swidth + (2 * GPyramidEdge);
  LPyramidWidth = swidth;
  LPyramidHeight = sheight;
  LPyramidXStride = LPyramidWidth + 2 * LPyramidEdge;
  LPyramidYStride = LPyramidXStride * (LPyramidHeight + 2 * LPyramidEdge);
  GPyramidHeight = sheight;
  pBufferLPyramid = InLaplacianPyramid_fx;
  pDataLPyramid =
      pBufferLPyramid + (LPyramidEdge * LPyramidXStride) + LPyramidEdge;
  pBufferInGPyramid = InGaussianPyramid_fx;
  pDataInGPyramid =
      pBufferInGPyramid + (GPyramidEdge * GPyramidXStride) + GPyramidEdge;
  pBufferOutLPyramid = OutLaplacianPyramid_fx;
  pDataOutLPyramid = pBufferOutLPyramid +
                     (OutLPyramidEdge * OutLPyramidXStride) + OutLPyramidEdge;
  for (l = 0; l < pyramid_levels; l++) // loop across pyramid levels
  {
    for (x = -OutLPyramidEdge; x < (OutLPyramidWidth + OutLPyramidEdge); x++) {
      for (y = -OutLPyramidEdge; y < (OutLPyramidHeight + OutLPyramidEdge);
           y++) {
        // Split input pyramid value into integer and floating parts
        // float level = (pDataGPyramid[(y * GPyramidXStride) + x] *
        // (float)(levels-1));
        int16_t level =
            (int16_t)(pDataInGPyramid[(y * GPyramidXStride) + x] *
                      (levels - 1)); // pDataGPyramid//Q8 in 8 bits
                                     // unsigned//level//Q3.8 in 16 bits
        int li = CLIP((level >> 8), 0, levels - 2);
        // float lf = level - (float)li;
        int16_t lf = (uint8_t)(
            level - ((int16_t)li << 8)); // Q0.8 in 16 bits (unsigned number)
        int32_t a =
            ((((int16_t)1 << 8) - lf) *
             pDataLPyramid[(li * LPyramidYStride) + (y * LPyramidXStride) +
                           x]); // Q3.8 + Q0.8//Q3.16
        int32_t b =
            (lf *
             pDataLPyramid[((li + 1) * LPyramidYStride) +
                           (y * LPyramidXStride) + x]); // Q3.8 + Q0.8//Q3.16
        pDataOutLPyramid[(y * OutLPyramidXStride) + x] =
            (int16_t)((a + b) >> 8); // pDataOutLPyramid//Q3.8
      }
    }
#if DEBUG_DUMP
    {
      FILE *fp;

      char fname[256];

      for (int k = 0; k < 1; k++) {
        sprintf(fname, "refL2DLevel%d.yuv", l);
        fp = fopen(fname, "wb");

        fwrite(&pBufferOutLPyramid[0], 2,
               OutLPyramidXStride * (OutLPyramidHeight + 2 * OutLPyramidEdge),
               fp);

        fclose(fp);
      }
    }
#endif
    pBufferLPyramid = pBufferLPyramid + (LPyramidYStride * levels);
    pBufferInGPyramid = pBufferInGPyramid +
                        (GPyramidXStride * (GPyramidHeight + 2 * GPyramidEdge));
    pBufferOutLPyramid =
        pBufferOutLPyramid +
        (OutLPyramidXStride * (OutLPyramidHeight + 2 * OutLPyramidEdge));
    OutLPyramidWidth = OutLPyramidWidth / 2;
    OutLPyramidHeight = OutLPyramidHeight / 2;
    OutLPyramidXStride = OutLPyramidWidth + (2 * OutLPyramidEdge);
    GPyramidXStride = OutLPyramidWidth + (2 * GPyramidEdge);
    GPyramidHeight = OutLPyramidHeight;
    LPyramidWidth = OutLPyramidWidth;
    LPyramidHeight = OutLPyramidHeight;
    LPyramidXStride = LPyramidWidth + 2 * LPyramidEdge;
    LPyramidYStride = LPyramidXStride * (LPyramidHeight + 2 * LPyramidEdge);
    pDataLPyramid =
        pBufferLPyramid + (LPyramidEdge * LPyramidXStride) + LPyramidEdge;
    pDataInGPyramid =
        pBufferInGPyramid + (GPyramidEdge * GPyramidXStride) + GPyramidEdge;
    pDataOutLPyramid = pBufferOutLPyramid +
                       (OutLPyramidEdge * OutLPyramidXStride) + OutLPyramidEdge;
  }

  OutLPyramidEdge = 1;
  OutGPyramidEdge = 1;
  OutLPyramidWidth = swidth;
  OutLPyramidHeight = sheight;
  OutGPyramidWidth = swidth;
  OutGPyramidHeight = sheight;
  OutLPyramidXStride = OutLPyramidWidth + (2 * OutLPyramidEdge);
  OutGPyramidXStride = OutGPyramidWidth + (2 * OutGPyramidEdge);
  pBufferOutLPyramid = OutLaplacianPyramid_fx;
  pBufferOutGPyramid = OutGaussianPyramid_fx;
  for (l = 1; l < pyramid_levels; l++) // loop across pyramid levels
  {
    pBufferOutLPyramid =
        pBufferOutLPyramid +
        (OutLPyramidXStride * (OutLPyramidHeight + 2 * OutLPyramidEdge));
    pBufferOutGPyramid =
        pBufferOutGPyramid +
        (OutGPyramidXStride * (OutGPyramidHeight + 2 * OutGPyramidEdge));
    OutLPyramidWidth = OutLPyramidWidth / 2;
    OutLPyramidHeight = OutLPyramidHeight / 2;
    OutGPyramidWidth = OutGPyramidWidth / 2;
    OutGPyramidHeight = OutGPyramidHeight / 2;
    OutLPyramidXStride = OutLPyramidWidth + (2 * OutLPyramidEdge);
    OutGPyramidXStride = OutGPyramidWidth + (2 * OutGPyramidEdge);
  }

  // Make the Gaussian pyramid of the output
  pDataOutLPyramid = pBufferOutLPyramid +
                     (OutLPyramidXStride * OutLPyramidEdge) + OutLPyramidEdge;
  pDataOutGPyramid = pBufferOutGPyramid +
                     (OutGPyramidXStride * OutGPyramidEdge) + OutGPyramidEdge;
  // last scale of pyramid
  for (x = -OutGPyramidEdge; x < (OutGPyramidWidth + OutGPyramidEdge); x++) {
    for (y = -OutGPyramidEdge; y < (OutGPyramidHeight + OutGPyramidEdge); y++) {
      // Copy the values of  lower scale laplacian pyramid into lower scale
      // gaussian pyramid
      pDataOutGPyramid[(y * OutGPyramidXStride) + x] =
          pDataOutLPyramid[(y * OutLPyramidXStride) + x];
    }
  }
  {
    OutGPyramidEdge = 1;
    OutGPyramidEdgePrev = 1;
    OutLPyramidEdgePrev = 1;
    OutGPyramidWidthPrev = OutGPyramidWidth;
    OutGPyramidHeightPrev = OutGPyramidHeight;
    OutGPyramidXStride = OutGPyramidWidth + 2 * OutGPyramidEdge;
    OutLPyramidWidthPrev = OutLPyramidWidth;
    OutLPyramidHeightPrev = OutLPyramidHeight;
    OutLPyramidXStridePrev = OutLPyramidWidth + 2 * OutLPyramidEdgePrev;
    pBufferOutGPyramidPrev = pBufferOutGPyramid;
    pDataOutGPyramidPrev = pBufferOutGPyramidPrev +
                           (OutGPyramidXStride * OutGPyramidEdge) +
                           OutGPyramidEdge;
    pBufferOutLPyramidPrev = pBufferOutLPyramid;

    for (int j = pyramid_levels - 2; j >= 0; j--) {
      OutGPyramidWidth = OutGPyramidWidthPrev;
      OutGPyramidHeight = OutGPyramidHeightPrev;
      OutGPyramidWidthPrev = OutGPyramidWidthPrev * 2;
      OutGPyramidHeightPrev = OutGPyramidHeightPrev * 2;
      OutGPyramidXStride = OutGPyramidWidth + 2 * OutGPyramidEdge;
      // OutGPyramidYStride = OutGPyramidXStride * (OutGPyramidHeight + 2 *
      // GPyramidEdge);
      OutGPyramidXStridePrev = OutGPyramidWidthPrev + 2 * OutGPyramidEdgePrev;
      // OutGPyramidYStridePrev = OutGPyramidXStridePrev *
      // (OutGPyramidHeightPrev + 2 * OutGPyramidEdgePrev);
      OutLPyramidWidthPrev = OutLPyramidWidthPrev * 2;
      OutLPyramidHeightPrev = OutLPyramidHeightPrev * 2;
      OutLPyramidXStridePrev = OutLPyramidWidthPrev + 2 * OutLPyramidEdgePrev;
      // OutLPyramidYStridePrev = OutLPyramidXStridePrev *
      // (OutLPyramidHeightPrev + 2 * OutLPyramidEdgePrev);
      pBufferOutGPyramidPrev =
          pBufferOutGPyramidPrev -
          (OutGPyramidXStridePrev *
           (OutGPyramidHeightPrev + 2 * OutGPyramidEdgePrev));
      pDataOutGPyramid = pDataOutGPyramidPrev;
      pDataOutGPyramidPrev = pBufferOutGPyramidPrev +
                             (OutGPyramidEdgePrev * OutGPyramidXStridePrev) +
                             OutGPyramidEdgePrev;
      pBufferOutLPyramidPrev =
          pBufferOutLPyramidPrev -
          (OutLPyramidXStridePrev *
           (OutLPyramidHeightPrev + 2 * OutLPyramidEdgePrev));
      pDataOutLPyramidPrev = pBufferOutLPyramidPrev +
                             (OutLPyramidEdgePrev * OutLPyramidXStridePrev) +
                             OutLPyramidEdgePrev;
      // Upsample (non separable implementation) the 2D gaussian pyramid at
      // scale n + 1 and add it to scale n of 2D output laplacian pyramid
      for (x = 0; x < OutGPyramidWidthPrev; x++) {
        for (y = 0; y < OutGPyramidHeightPrev; y++) {
          int16_t upsampleVal;
          int x1, y1, x2, y2, x3, y3, x4, y4;
          x1 = (x / 2);
          y1 = (y / 2);
          x2 = (x / 2) + (x % 2);
          y2 = (y / 2);
          x3 = (x / 2);
          y3 = (y / 2) + (y % 2);
          x4 = (x / 2) + (x % 2);
          y4 = (y / 2) + (y % 2);
          upsampleVal = pDataOutGPyramid[(y1 * OutGPyramidXStride) + x1] +
                        pDataOutGPyramid[(y2 * OutGPyramidXStride) + x2] +
                        pDataOutGPyramid[(y3 * OutGPyramidXStride) + x3] +
                        pDataOutGPyramid[(y4 * OutGPyramidXStride) + x4];
          upsampleVal = upsampleVal >> 2; // Q8

          pDataOutGPyramidPrev[(y * OutGPyramidXStridePrev) + x] =
              pDataOutLPyramidPrev[(y * OutLPyramidXStridePrev) + x] +
              upsampleVal;
        }
      }

      // Repeat the edges of pyramid
      // top edge
      for (x = -OutGPyramidEdgePrev;
           x < (OutGPyramidWidthPrev + OutGPyramidEdgePrev); x++) {
        for (y = -OutGPyramidEdgePrev; y < 0; y++) {
          int x1 = CLIP(x, 0, (OutGPyramidWidthPrev - 1));
          int y1 = CLIP(y, 0, (OutGPyramidHeightPrev - 1));
          pDataOutGPyramidPrev[(y * OutGPyramidXStridePrev) + x] =
              pDataOutGPyramidPrev[(y1 * OutGPyramidXStridePrev) + x1];
        }
      }
      // bottom edge
      for (x = -OutGPyramidEdgePrev;
           x < (OutGPyramidWidthPrev + OutGPyramidEdgePrev); x++) {
        for (y = OutGPyramidHeightPrev;
             y < (OutGPyramidHeightPrev + OutGPyramidEdgePrev); y++) {
          int x1 = CLIP(x, 0, (OutGPyramidWidthPrev - 1));
          int y1 = CLIP(y, 0, (OutGPyramidHeightPrev - 1));
          pDataOutGPyramidPrev[(y * OutGPyramidXStridePrev) + x] =
              pDataOutGPyramidPrev[(y1 * OutGPyramidXStridePrev) + x1];
        }
      }
      // left edge
      for (x = -OutGPyramidEdgePrev; x < 0; x++) {
        for (y = -OutGPyramidEdgePrev;
             y < (OutGPyramidHeightPrev + OutGPyramidEdgePrev); y++) {
          int x1 = CLIP(x, 0, (OutGPyramidWidthPrev - 1));
          int y1 = CLIP(y, 0, (OutGPyramidHeightPrev - 1));
          pDataOutGPyramidPrev[(y * OutGPyramidXStridePrev) + x] =
              pDataOutGPyramidPrev[(y1 * OutGPyramidXStridePrev) + x1];
        }
      }
      // right edge
      for (x = OutGPyramidWidthPrev;
           x < (OutGPyramidWidthPrev + OutGPyramidEdgePrev); x++) {
        for (y = -OutGPyramidEdgePrev;
             y < (OutGPyramidHeightPrev + OutGPyramidEdgePrev); y++) {
          int x1 = CLIP(x, 0, (OutGPyramidWidthPrev - 1));
          int y1 = CLIP(y, 0, (OutGPyramidHeightPrev - 1));
          pDataOutGPyramidPrev[(y * OutGPyramidXStridePrev) + x] =
              pDataOutGPyramidPrev[(y1 * OutGPyramidXStridePrev) + x1];
        }
      }
#if DEBUG_DUMP
      {
        FILE *fp;

        char fname[256];

        for (int k = 0; k < 1; k++) {
          sprintf(fname, "refOut%d.yuv", j);
          fp = fopen(fname, "wb");

          fwrite(&pBufferOutGPyramidPrev[0], 2,
                 OutGPyramidXStridePrev *
                     (OutGPyramidHeightPrev + 2 * OutGPyramidEdgePrev),
                 fp);

          fclose(fp);
        }
      }
#endif
    }
  }

  pDataOutGPyramid = OutGaussianPyramid_fx +
                     ((swidth + (2 * edge_width)) * edge_height) + edge_width;
  for (x = 0; x < width; x++) {
    for (y = 0; y < height; y++) {
      pdst[y * dstride + x] = (uint8_t)CLIP(
          (int)pDataOutGPyramid[y * (swidth + 2 * edge_width) + x], 0, 255);
    }
  }
#if DEBUG_DUMP
  {
    FILE *fp;

    char fname[256];

    for (int k = 0; k < 1; k++) {
      sprintf(fname, "refOutfinal%d.yuv", 0);
      fp = fopen(fname, "wb");

      fwrite(&pdst[0], 1, dstride * (height + 2 * 0), fp);

      fclose(fp);
    }
  }
#endif
  return (XI_ERR_OK);
}
