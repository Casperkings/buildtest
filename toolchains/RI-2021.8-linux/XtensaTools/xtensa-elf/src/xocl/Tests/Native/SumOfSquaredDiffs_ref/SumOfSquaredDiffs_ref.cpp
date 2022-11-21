#include "xi_api_ref.h"
#include <math.h>

// Assumptions
// nlength must be 16
// width must be a multiple of nlength
XI_ERR_TYPE SumOfSquaredDiffsU8S32_Ref(const uint8_t *pSrc1,
                                       const uint8_t *pSrc2, int32_t *pDstRef,
                                       int32_t width, int32_t nvectors,
                                       int32_t nlength) {
  int vind, x, n, k;
  k = 0;
  for (x = 0; x < width; x += nlength) {
    const uint8_t *src_ptr;
    int32_t *dst_ptr;
    src_ptr = &pSrc1[x];
    dst_ptr = &pDstRef[k];
    for (vind = 0; vind < nvectors; vind++) {
      const uint8_t *templ_ptr;
      int32_t sq = 0;
      templ_ptr = &pSrc2[vind * width + x];
      for (n = 0; n < nlength; n++) {
        int16_t diff = templ_ptr[n] - src_ptr[n];
        sq += diff * diff;
      }
      dst_ptr[vind] = sq;
    }
    k += nvectors;
  }

  return XI_ERR_OK;
}
