#include "xi_lib.h"
#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/tie/xt_misc.h>
#include <math.h>
//#include "xi_imgproc.h"
//#include "xi_intrin.h"
//#include "xi_wide_arithm.h"

#define OFFSET_PTR_N_2X32U(ptr, nrows, stride, in_row_offset)                  \
  ((xb_vecN_2x32Uv *)((uint32_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_N_2X32(ptr, nrows, stride, in_row_offset)                   \
  ((xb_vecN_2x32v *)((int32_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_NX16U(ptr, nrows, stride, in_row_offset)                    \
  ((xb_vecNx16U *)((uint16_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_NX16(ptr, nrows, stride, in_row_offset)                     \
  ((xb_vecNx16 *)((int16_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_N_2X16(ptr, nrows, stride, in_row_offset)                   \
  ((xb_vecN_2x16 *)((int16_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_NX8U(ptr, nrows, stride, in_row_offset)                     \
  ((xb_vecNx8U *)((uint8_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_NX8(ptr, nrows, stride, in_row_offset)                      \
  ((xb_vecNx8 *)((int8_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_2NX8U(ptr, nrows, stride, in_row_offset)                    \
  ((xb_vec2Nx8U *)((uint8_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))
#define OFFSET_PTR_2NX8(ptr, nrows, stride, in_row_offset)                     \
  ((xb_vec2Nx8 *)((int8_t *)(ptr) + (in_row_offset) + (nrows) * (stride)))

#define XI_IVP_GATHERNX16_V(a, b, c)                                           \
  IVP_GATHERNX16_V((const short int *)(a), b, c)
#define XI_IVP_GATHERNX16T_V(a, b, c, d)                                       \
  IVP_GATHERNX16T_V((const short int *)(a), b, c, d)
XI_ERR_TYPE xiTranspose_I16(const xi_pArray src, xi_pArray dst) {
  int sstride = XI_ARRAY_GET_PITCH(src);
  int dstride = XI_ARRAY_GET_PITCH(dst);
  int swidth = XI_ARRAY_GET_WIDTH(src);
  int dwidth = XI_ARRAY_GET_WIDTH(dst);

  int16_t *restrict psrc = (int16_t *)XI_ARRAY_GET_DATA_PTR(src);
  int16_t *restrict pdst = (int16_t *)XI_ARRAY_GET_DATA_PTR(dst);

  valign a0 = IVP_ZALIGN();
  valign a1 = IVP_ZALIGN();
  valign a2 = IVP_ZALIGN();
  valign a3 = IVP_ZALIGN();

  xb_vecNx16 offsets =
      IVP_ADDNX16(IVP_MULNX16PACKL(IVP_SRLINX16(IVP_SEQNX16(), 2), 2 * sstride),
                  IVP_SLLINX16(IVP_ANDNX16(3, IVP_SEQNX16()), 1));

  int i, j = 0;
  for (i = 0; i < (dwidth & -XCHAL_IVPN_SIMD_WIDTH);
       i += XCHAL_IVPN_SIMD_WIDTH) {
    for (j = 0; j < (swidth & ~3); j += 4) {
      /* gathering by 4 16-bit elements per row, total 8 rows per gather */
      xb_vecNx16 v0 = XI_IVP_GATHERNX16_V(
          (short *)OFFSET_PTR_NX16(psrc, i + 0 * XCHAL_IVPN_SIMD_WIDTH / 4,
                                   sstride, j),
          offsets, 15);
      xb_vecNx16 v1 = XI_IVP_GATHERNX16_V(
          (short *)OFFSET_PTR_NX16(psrc, i + 1 * XCHAL_IVPN_SIMD_WIDTH / 4,
                                   sstride, j),
          offsets, 15);
      xb_vecNx16 v2 = XI_IVP_GATHERNX16_V(
          (short *)OFFSET_PTR_NX16(psrc, i + 2 * XCHAL_IVPN_SIMD_WIDTH / 4,
                                   sstride, j),
          offsets, 15);
      xb_vecNx16 v3 = XI_IVP_GATHERNX16_V(
          (short *)OFFSET_PTR_NX16(psrc, i + 3 * XCHAL_IVPN_SIMD_WIDTH / 4,
                                   sstride, j),
          offsets, 15);

      IVP_DSELNX16I(v1, v0, v1, v0, IVP_DSELI_DEINTERLEAVE_2);
      IVP_DSELNX16I(v3, v2, v3, v2, IVP_DSELI_DEINTERLEAVE_2);
      IVP_DSELNX16I(v2, v0, v2, v0, IVP_DSELI_DEINTERLEAVE_1);
      IVP_DSELNX16I(v3, v1, v3, v1, IVP_DSELI_DEINTERLEAVE_1);

      xb_vecNx16 *vdst_ptr0 = OFFSET_PTR_NX16(pdst, j, dstride, i);
      xb_vecNx16 *vdst_ptr1 = OFFSET_PTR_NX16(vdst_ptr0, 1, dstride, 0);
      xb_vecNx16 *vdst_ptr2 = OFFSET_PTR_NX16(vdst_ptr1, 1, dstride, 0);
      xb_vecNx16 *vdst_ptr3 = OFFSET_PTR_NX16(vdst_ptr2, 1, dstride, 0);

      IVP_SANX16_IP(v0, a0, vdst_ptr0);
      IVP_SAPOSNX16_FP(a0, vdst_ptr0);

      IVP_SANX16_IP(v2, a1, vdst_ptr1);
      IVP_SAPOSNX16_FP(a1, vdst_ptr1);

      IVP_SANX16_IP(v1, a2, vdst_ptr2);
      IVP_SAPOSNX16_FP(a2, vdst_ptr2);

      IVP_SANX16_IP(v3, a3, vdst_ptr3);
      IVP_SAPOSNX16_FP(a3, vdst_ptr3);
    }
  }

  if (j < swidth) {
    vboolN h_mask = IVP_LENX16(IVP_ANDNX16(3, IVP_SEQNX16()), (swidth - 1) & 3);
    int num1 = (swidth - j - 1) * 2 * XCHAL_IVPN_SIMD_WIDTH;
    int num2 = (swidth - j - 2) * 2 * XCHAL_IVPN_SIMD_WIDTH;

    for (i = 0; i < (dwidth & -XCHAL_IVPN_SIMD_WIDTH);
         i += XCHAL_IVPN_SIMD_WIDTH) {
      xb_vecNx16 offsets0 = IVP_MOVNX16T(offsets, 0, h_mask);
      /* gathering by 4 16-bit elements per row, total 8 rows per gather */
      xb_vecNx16 v0 = XI_IVP_GATHERNX16T_V(
          (short *)OFFSET_PTR_NX16(psrc, i + 0 * XCHAL_IVPN_SIMD_WIDTH / 4,
                                   sstride, j),
          offsets0, h_mask, 15);
      xb_vecNx16 v1 = XI_IVP_GATHERNX16T_V(
          (short *)OFFSET_PTR_NX16(psrc, i + 1 * XCHAL_IVPN_SIMD_WIDTH / 4,
                                   sstride, j),
          offsets0, h_mask, 15);
      xb_vecNx16 v2 = XI_IVP_GATHERNX16T_V(
          (short *)OFFSET_PTR_NX16(psrc, i + 2 * XCHAL_IVPN_SIMD_WIDTH / 4,
                                   sstride, j),
          offsets0, h_mask, 15);
      xb_vecNx16 v3 = XI_IVP_GATHERNX16T_V(
          (short *)OFFSET_PTR_NX16(psrc, i + 3 * XCHAL_IVPN_SIMD_WIDTH / 4,
                                   sstride, j),
          offsets0, h_mask, 15);

      IVP_DSELNX16I(v1, v0, v1, v0, IVP_DSELI_DEINTERLEAVE_2);
      IVP_DSELNX16I(v3, v2, v3, v2, IVP_DSELI_DEINTERLEAVE_2);
      IVP_DSELNX16I(v2, v0, v2, v0, IVP_DSELI_DEINTERLEAVE_1);
      IVP_DSELNX16I(v3, v1, v3, v1, IVP_DSELI_DEINTERLEAVE_1);

      xb_vecNx16 *vdst_ptr0 = OFFSET_PTR_NX16(pdst, j, dstride, i);
      xb_vecNx16 *vdst_ptr1 = OFFSET_PTR_NX16(vdst_ptr0, 1, dstride, 0);
      xb_vecNx16 *vdst_ptr2 = OFFSET_PTR_NX16(vdst_ptr1, 1, dstride, 0);

      IVP_SANX16_IP(v0, a0, vdst_ptr0);
      IVP_SAPOSNX16_FP(a0, vdst_ptr0);

      IVP_SAVNX16_XP(v2, a1, vdst_ptr1, num1);
      IVP_SAPOSNX16_FP(a1, vdst_ptr1);

      IVP_SAVNX16_XP(v1, a2, vdst_ptr2, num2);
      IVP_SAPOSNX16_FP(a2, vdst_ptr2);
    }
  }

  if (i < dwidth) {
    vboolN h_mask0 = IVP_LTNX16(IVP_SRLINX16(IVP_SEQNX16(), 2), dwidth - i);
    vboolN h_mask1 = IVP_LTNX16(IVP_SRLINX16(IVP_SEQNX16(), 2), dwidth - i - 8);
    vboolN h_mask2 =
        IVP_LTNX16(IVP_SRLINX16(IVP_SEQNX16(), 2), dwidth - i - 16);
    vboolN h_mask3 =
        IVP_LTNX16(IVP_SRLINX16(IVP_SEQNX16(), 2), dwidth - i - 24);

    for (j = 0; j < (swidth & ~3); j += 4) {
      /* gathering up to 4 16-bit elements per row, total 8 rows per gather */
      xb_vecNx16 offsets0 = IVP_MOVNX16T(offsets, 0, h_mask0);
      xb_vecNx16 offsets1 = IVP_MOVNX16T(offsets, 0, h_mask1);
      xb_vecNx16 offsets2 = IVP_MOVNX16T(offsets, 0, h_mask2);
      xb_vecNx16 offsets3 = IVP_MOVNX16T(offsets, 0, h_mask3);

      xb_vecNx16 v0 = XI_IVP_GATHERNX16T_V(
          (short *)OFFSET_PTR_NX16(psrc, i + 0 * XCHAL_IVPN_SIMD_WIDTH / 4,
                                   sstride, j),
          offsets0, h_mask0, 15);
      xb_vecNx16 v1 = XI_IVP_GATHERNX16T_V(
          (short *)OFFSET_PTR_NX16(psrc, i + 1 * XCHAL_IVPN_SIMD_WIDTH / 4,
                                   sstride, j),
          offsets1, h_mask1, 15);
      xb_vecNx16 v2 = XI_IVP_GATHERNX16T_V(
          (short *)OFFSET_PTR_NX16(psrc, i + 2 * XCHAL_IVPN_SIMD_WIDTH / 4,
                                   sstride, j),
          offsets2, h_mask2, 15);
      xb_vecNx16 v3 = XI_IVP_GATHERNX16T_V(
          (short *)OFFSET_PTR_NX16(psrc, i + 3 * XCHAL_IVPN_SIMD_WIDTH / 4,
                                   sstride, j),
          offsets3, h_mask3, 15);

      IVP_DSELNX16I(v1, v0, v1, v0, IVP_DSELI_DEINTERLEAVE_2);
      IVP_DSELNX16I(v3, v2, v3, v2, IVP_DSELI_DEINTERLEAVE_2);
      IVP_DSELNX16I(v2, v0, v2, v0, IVP_DSELI_DEINTERLEAVE_1);
      IVP_DSELNX16I(v3, v1, v3, v1, IVP_DSELI_DEINTERLEAVE_1);

      xb_vecNx16 *vdst_ptr0 = OFFSET_PTR_NX16(pdst, j, dstride, i);
      xb_vecNx16 *vdst_ptr1 = OFFSET_PTR_NX16(vdst_ptr0, 1, dstride, 0);
      xb_vecNx16 *vdst_ptr2 = OFFSET_PTR_NX16(vdst_ptr1, 1, dstride, 0);
      xb_vecNx16 *vdst_ptr3 = OFFSET_PTR_NX16(vdst_ptr2, 1, dstride, 0);

      IVP_SAVNX16_XP(v0, a0, vdst_ptr0, 2 * (dwidth - i));
      IVP_SAPOSNX16_FP(a0, vdst_ptr0);

      IVP_SAVNX16_XP(v2, a1, vdst_ptr1, 2 * (dwidth - i));
      IVP_SAPOSNX16_FP(a1, vdst_ptr1);

      IVP_SAVNX16_XP(v1, a2, vdst_ptr2, 2 * (dwidth - i));
      IVP_SAPOSNX16_FP(a2, vdst_ptr2);

      IVP_SAVNX16_XP(v3, a3, vdst_ptr3, 2 * (dwidth - i));
      IVP_SAPOSNX16_FP(a3, vdst_ptr3);
    }

    if (j < swidth) {
      vboolN h_mask =
          IVP_LENX16(IVP_ANDNX16(3, IVP_SEQNX16()), (swidth - 1) & 3);
      vboolN h_mask0 = IVP_ANDBN(
          h_mask, IVP_LTNX16(IVP_SRLINX16(IVP_SEQNX16(), 2), dwidth - i));
      vboolN h_mask1 = IVP_ANDBN(
          h_mask, IVP_LTNX16(IVP_SRLINX16(IVP_SEQNX16(), 2), dwidth - i - 8));
      vboolN h_mask2 = IVP_ANDBN(
          h_mask, IVP_LTNX16(IVP_SRLINX16(IVP_SEQNX16(), 2), dwidth - i - 16));
      vboolN h_mask3 = IVP_ANDBN(
          h_mask, IVP_LTNX16(IVP_SRLINX16(IVP_SEQNX16(), 2), dwidth - i - 24));
      int num1 = j < (swidth - 1) ? 2 * (dwidth - i) : 0;
      int num2 = j < (swidth - 2) ? 2 * (dwidth - i) : 0;

      /* gathering by 4 16-bit elements per row, total 8 rows per gather */
      xb_vecNx16 offsets0 = IVP_MOVNX16T(offsets, 0, h_mask0);
      xb_vecNx16 offsets1 = IVP_MOVNX16T(offsets, 0, h_mask1);
      xb_vecNx16 offsets2 = IVP_MOVNX16T(offsets, 0, h_mask2);
      xb_vecNx16 offsets3 = IVP_MOVNX16T(offsets, 0, h_mask3);

      xb_vecNx16 v0 = XI_IVP_GATHERNX16T_V(
          (short *)OFFSET_PTR_NX16(psrc, i + 0 * XCHAL_IVPN_SIMD_WIDTH / 4,
                                   sstride, j),
          offsets0, h_mask0, 15);
      xb_vecNx16 v1 = XI_IVP_GATHERNX16T_V(
          (short *)OFFSET_PTR_NX16(psrc, i + 1 * XCHAL_IVPN_SIMD_WIDTH / 4,
                                   sstride, j),
          offsets1, h_mask1, 15);
      xb_vecNx16 v2 = XI_IVP_GATHERNX16T_V(
          (short *)OFFSET_PTR_NX16(psrc, i + 2 * XCHAL_IVPN_SIMD_WIDTH / 4,
                                   sstride, j),
          offsets2, h_mask2, 15);
      xb_vecNx16 v3 = XI_IVP_GATHERNX16T_V(
          (short *)OFFSET_PTR_NX16(psrc, i + 3 * XCHAL_IVPN_SIMD_WIDTH / 4,
                                   sstride, j),
          offsets3, h_mask3, 15);

      IVP_DSELNX16I(v1, v0, v1, v0, IVP_DSELI_DEINTERLEAVE_2);
      IVP_DSELNX16I(v3, v2, v3, v2, IVP_DSELI_DEINTERLEAVE_2);
      IVP_DSELNX16I(v2, v0, v2, v0, IVP_DSELI_DEINTERLEAVE_1);
      IVP_DSELNX16I(v3, v1, v3, v1, IVP_DSELI_DEINTERLEAVE_1);

      xb_vecNx16 *vdst_ptr0 = OFFSET_PTR_NX16(pdst, j, dstride, i);
      xb_vecNx16 *vdst_ptr1 = OFFSET_PTR_NX16(vdst_ptr0, 1, dstride, 0);
      xb_vecNx16 *vdst_ptr2 = OFFSET_PTR_NX16(vdst_ptr1, 1, dstride, 0);

      IVP_SAVNX16_XP(v0, a0, vdst_ptr0, 2 * (dwidth - i));
      IVP_SAPOSNX16_FP(a0, vdst_ptr0);

      IVP_SAVNX16_XP(v2, a1, vdst_ptr1, num1);
      IVP_SAPOSNX16_FP(a1, vdst_ptr1);

      IVP_SAVNX16_XP(v1, a2, vdst_ptr2, num2);
      IVP_SAPOSNX16_FP(a2, vdst_ptr2);
    }
  }
  return 0;
}
