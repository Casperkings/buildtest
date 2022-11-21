#include <stdint.h>
#include <xtensa/tie/xt_ivpn.h>

void ivpCalcHist(const uint8_t *restrict src, uint16_t *restrict dst,
                 uint16_t *restrict hist, int width, int height,
                 int numHistBins) {
  // We have a per-lane histogram

  // Zero all the histograms
  xb_vecNx16U *p = (xb_vecNx16U *)hist;
  for (int x = 0; x < (numHistBins * 64) / 64; ++x) {
    *p++ = 0;
  }

  xb_vecNx16U offsets = IVP_SEQNX16U() * (xb_vecNx16U)numHistBins;

  for (int x = 0; x < width / 64; ++x) {
    const uint8_t *src_p = (src + x * 64);
    for (int y = 0; y < height; ++y) {
      xb_vecNx16U v = *(const xb_vecNx8U *)src_p;
      xb_vecNx16U hist_offsets = (offsets + v) << 1;
      xb_vecNx16U count = IVP_GATHERNX16U((uint16_t *)hist, hist_offsets);
      IVP_SCATTERNX16U(count + (xb_vecNx16U)1, (uint16_t *)hist, hist_offsets);
      src_p += width;
    }
  }

  // Wait for scatter to finish
#pragma flush_memory

  // Reduce the per-lane histogram
  uint16_t *p1 = hist;
  xb_vecNx16U *dst_p = (xb_vecNx16U *)dst;
  for (int x = 0; x < numHistBins / 64; ++x) {
    uint16_t *p2 = p1;
    xb_vecNx16U sum = 0;
    for (int y = 0; y < 64; ++y) {
      sum += *(xb_vecNx16U *)p2;
      p2 += 256;
    }
    *dst_p++ = sum;
    p1 += 64;
  }
}

void ivpReduceHist(uint16_t *src, uint16_t *dst, int numTiles,
                   int numHistBins) {
  uint16_t *p1 = src;
  xb_vecNx16U *dst_p = (xb_vecNx16U *)dst;
  for (int x = 0; x < numHistBins / 64; ++x) {
    uint16_t *p2 = p1;
    xb_vecNx16U sum = 0;
    for (int y = 0; y < numTiles; ++y) {
      sum += *(xb_vecNx16U *)p2;
      p2 += 256;
    }
    *dst_p++ = sum;
    p1 += 64;
  }
}
