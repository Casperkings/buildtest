//#include <float.h>
//#include <math.h>
#include "xi_lib.h"
#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/tie/xt_misc.h>
#define GATHER_DELAY 1

#ifndef IVP_PACKLN_2X64
#define IVP_PACKLN_2X64 IVP_PACKLN_2X96
#endif

#define OFFSET_PTR_8U(ptr, offset)  (xb_vecNx8U *)(((uint8_t *)(ptr)) + offset)
#define OFFSET_PTR_8U2(ptr, offset) (uint8_t *)(((uint8_t *)(ptr)) + offset)
#define OFFSET_PTR_16U(ptr, offset)                                            \
  (xb_vecNx16U *)(((uint16_t *)(ptr)) + offset)

#define GATHER_DELAY_U8    14
#define GATHER_DELAY_U8A   0
#define GATHER_DELAY_U8Oa  14
#define GATHER_DELAY_S16   16
#define GATHER_DELAY_S16Oa 12

#define XI_Q13_18 int
XI_ERR_TYPE xiWarpPerspectiveU8_Q13_18(const xi_pTile src, xi_pTile dst,
                                       const int *perspective) {
  int sstride = XI_TILE_GET_PITCH(src);
  int dstride = XI_TILE_GET_PITCH(dst);
  int width = XI_TILE_GET_WIDTH(dst);
  int height = XI_TILE_GET_HEIGHT(dst);
  int edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  int edge_height = XI_TILE_GET_EDGE_HEIGHT(src);

  // move upper left corner to the beginning of the data to exclude negative
  // offsets
  int xs = XI_TILE_GET_X_COORD(src) - edge_width;
  int ys = XI_TILE_GET_Y_COORD(src) - edge_height;
  int xd = XI_TILE_GET_X_COORD(dst);
  int yd = XI_TILE_GET_Y_COORD(dst);

  xb_vecNx16 xmin = 0;
  xb_vecNx16 xmax = XT_MAX(0, XI_TILE_GET_WIDTH(src) + 2 * edge_width - 2);
  xb_vecNx16 ymin = 0;
  xb_vecNx16 ymax = XT_MAX(0, XI_TILE_GET_HEIGHT(src) + 2 * edge_height - 2);

  xb_vecNx16 xseq = IVP_ADDNX16(IVP_SEQNX16(), xd);

  int a11 = perspective[0];
  int a12 = perspective[1];
  int a13 = perspective[2];
  int a21 = perspective[3];
  int a22 = perspective[4];
  int a23 = perspective[5];
  int a31 = perspective[6];
  int a32 = perspective[7];
  int a33 = perspective[8];

  // recalculate coefficients in order to get rid of (xs + 0.5)
  // xsrc = ((a11*(xd + 0.5 + x) + a12*(yd + 0.5 + y) + a13) / (a31*(xd + 0.5 +
  // x) + a32*(yd + 0.5 + y) + a33)) - xs - 0.5 =
  //       = (b11*(xd + x) + b12*(yd + y) + (b11 + b12)/2 + b13) / (a31*(xd + x)
  //       + a32*(yd + y) + (a31 + a32)/2 + a33)
  a11 = a11 - a31 * xs - a31 / 2;
  a12 = a12 - a32 * xs - a32 / 2;
  a13 = a13 - a33 * xs - a33 / 2;
  a21 = a21 - a31 * ys - a31 / 2;
  a22 = a22 - a32 * ys - a32 / 2;
  a23 = a23 - a33 * ys - a33 / 2;

  // vector offsets for X, Y and D. 2 N_2 vectors are required to produce N
  // outputs at a time to allow N gathers
  xb_vecNx16 xseqL = IVP_SHFLNX16I(xseq, IVP_SHFLI_DOUBLE_1_LO);

  xb_vecN_2x64w XW = IVP_MULN_2X16X32_0(xseqL, (xb_vecN_2x32v)a11);
  xb_vecN_2x64w YW = IVP_MULN_2X16X32_0(xseqL, (xb_vecN_2x32v)a21);
  xb_vecN_2x64w DW = IVP_MULN_2X16X32_0(xseqL, (xb_vecN_2x32v)a31);
  xb_vecN_2x32Uv Xseq1 = IVP_PACKLN_2X64W(XW);
  xb_vecN_2x32Uv Yseq1 = IVP_PACKLN_2X64W(YW);
  xb_vecN_2x32Uv Dseq1 = IVP_PACKLN_2X64W(DW);

  xb_vecNx16 xseqH = IVP_SHFLNX16I(xseq, IVP_SHFLI_DOUBLE_1_HI);

  XW = IVP_MULN_2X16X32_0(xseqH, (xb_vecN_2x32v)a11);
  YW = IVP_MULN_2X16X32_0(xseqH, (xb_vecN_2x32v)a21);
  DW = IVP_MULN_2X16X32_0(xseqH, (xb_vecN_2x32v)a31);
  xb_vecN_2x32Uv Xseq2 = IVP_PACKLN_2X64W(XW);
  xb_vecN_2x32Uv Yseq2 = IVP_PACKLN_2X64W(YW);
  xb_vecN_2x32Uv Dseq2 = IVP_PACKLN_2X64W(DW);

  uint8_t *__restrict psrc =
      (uint8_t *)XI_TILE_GET_DATA_PTR(src) - edge_width - sstride * edge_height;
  uint8_t *__restrict psrcps = psrc + sstride;
  xb_vecNx8U *pdst = OFFSET_PTR_8U(XI_TILE_GET_DATA_PTR(dst), 0);
  xb_vecNx8U *rdst;

  valign a_store = IVP_ZALIGN();
  XI_Q13_18 offset1 = a12 * yd + (a11 + a12) / 2 + a13;
  XI_Q13_18 offset2 = a22 * yd + (a21 + a22) / 2 + a23;
  XI_Q13_18 offset3 = a32 * yd + (a31 + a32) / 2 + a33;

  a11 *= XCHAL_IVPN_SIMD_WIDTH;
  a21 *= XCHAL_IVPN_SIMD_WIDTH;
  a31 *= XCHAL_IVPN_SIMD_WIDTH;

  for (int x = 0; x < width; x += XCHAL_IVPN_SIMD_WIDTH, pdst += 1,
           offset1 += a11, offset2 += a21, offset3 += a31) {
    XI_Q13_18 yoffset1 = offset1;
    XI_Q13_18 yoffset2 = offset2;
    XI_Q13_18 yoffset3 = offset3;

    for (int y = 0; y < height;
         ++y, yoffset1 += a12, yoffset2 += a22, yoffset3 += a32) {
      xb_vecN_2x32Uv X1 = Xseq1 + (xb_vecN_2x32Uv)yoffset1;
      xb_vecN_2x32Uv X2 = Xseq2 + (xb_vecN_2x32Uv)yoffset1;

      xb_vecN_2x32Uv Y1 = Yseq1 + (xb_vecN_2x32Uv)yoffset2;
      xb_vecN_2x32Uv Y2 = Yseq2 + (xb_vecN_2x32Uv)yoffset2;

      xb_vecN_2x32Uv D1 = Dseq1 + (xb_vecN_2x32Uv)yoffset3;
      xb_vecN_2x32Uv D2 = Dseq2 + (xb_vecN_2x32Uv)yoffset3;

      xb_vecN_2x32v ns1, ns2, as1, as2;
      ns1 = IVP_NSAUN_2X32(D1);
      as1 = (xb_vecN_2x32v)16 - ns1;
      D1 = IVP_SLLN_2X32U(D1, ns1);
      ns2 = IVP_NSAUN_2X32(D2);
      as2 = (xb_vecN_2x32v)16 - ns2;
      D2 = IVP_SLLN_2X32U(D2, ns2);

      xb_vecN_2x32v Q1, R1, Q2, R2;

      // use upper 16 bits of normalized divisor
      IVP_DIVN_2X32X16U(Q1, R1, X1, IVP_MOVNX16_FROMN_2X32(D1), 1);

      IVP_DIVN_2X32X16U(Q2, R2, X2, IVP_MOVNX16_FROMN_2X32(D2), 1);

      // account for normalization of the divisor (by as=(16-ns))
      // Assumption: there are enough fractional bits in the quotient for good
      // bilinear interpolation
      R1 = IVP_SLLN_2X32(Q1, ns1);
      R2 = IVP_SLLN_2X32(Q2, ns2);
      Q1 = IVP_SRLN_2X32(Q1, as1);
      Q2 = IVP_SRLN_2X32(Q2, as2);

      // extract integer and fractional parts (treat 32b number as 16b number to
      // do this)
      xb_vecNx16 Xint =
          IVP_SELNX16I(IVP_MOVNX16_FROMN_2X32(Q2), IVP_MOVNX16_FROMN_2X32(Q1),
                       IVP_SELI_EXTRACT_1_OF_2_OFF_0);
      xb_vecNx16 Xrem =
          IVP_SELNX16I(IVP_MOVNX16_FROMN_2X32(R2), IVP_MOVNX16_FROMN_2X32(R1),
                       IVP_SELI_EXTRACT_1_OF_2_OFF_0);
      Xrem = IVP_SRLNX16(Xrem, 1); // Q15

      // use upper 16 bits of divisor - treat 32b divisor as 16b to do this
      IVP_DIVN_2X32X16U(Q1, R1, Y1, IVP_MOVNX16_FROMN_2X32(D1), 1);
      IVP_DIVN_2X32X16U(Q2, R2, Y2, IVP_MOVNX16_FROMN_2X32(D2), 1);
      // assumption - there are enough bits in quotient for good bilinear
      // interpolation
      R1 = IVP_SLLN_2X32(Q1, ns1);
      R2 = IVP_SLLN_2X32(Q2, ns2);
      Q1 = IVP_SRLN_2X32(Q1, as1);
      Q2 = IVP_SRLN_2X32(Q2, as2);

      xb_vecNx16 Yint =
          IVP_SELNX16I(IVP_MOVNX16_FROMN_2X32(Q2), IVP_MOVNX16_FROMN_2X32(Q1),
                       IVP_SELI_EXTRACT_1_OF_2_OFF_0);
      xb_vecNx16 Yrem =
          IVP_SELNX16I(IVP_MOVNX16_FROMN_2X32(R2), IVP_MOVNX16_FROMN_2X32(R1),
                       IVP_SELI_EXTRACT_1_OF_2_OFF_0);
      Yrem = IVP_SRLNX16(Yrem, 1); // Q15

      vboolN bxmin, bymin, inside;
      vboolN bxmax, bymax;
      IVP_BMAXNX16(bxmin, Xint, xmin, Xint);
      IVP_BMAXNX16(bymin, Yint, ymin, Yint);
      IVP_BMINNX16(bxmax, Xint, xmax, Xint);
      IVP_BMINNX16(bymax, Yint, ymax, Yint);
      inside = ~(bxmin | bxmax | bymin | bymax);

      // calc indices - 16b is enough
      xb_vecNx16 vaddr = Xint;
      IVP_MULANX16PACKL(vaddr, (xb_vecNx16)sstride, Yint);
      xb_vecNx16 v00 = 0, v01 = 0, v10 = 0, v11 = 0;

      v00 = IVP_GATHERNX8UT_V(psrc, vaddr, inside, GATHER_DELAY);
      v01 =
          IVP_GATHERNX8UT_V(psrc, vaddr + (xb_vecNx16)1, inside, GATHER_DELAY);
      v10 = IVP_GATHERNX8UT_V(psrcps, vaddr, inside, GATHER_DELAY);
      v11 = IVP_GATHERNX8UT_V(psrcps, vaddr + (xb_vecNx16)1, inside,
                              GATHER_DELAY);

      // interpolation
      v00 = IVP_MOVNX16T(v00, (xb_vecNx16)0, inside);
      IVP_MULANX16PACKQT(v00, Xrem, IVP_SUBNX16(v01, v00), inside);
      IVP_MULANX16PACKQT(v10, Xrem, IVP_SUBNX16(v11, v10), inside);
      IVP_MULANX16PACKQT(v00, Yrem, IVP_SUBNX16(v10, v00), inside);

      rdst = OFFSET_PTR_8U(pdst, y * dstride);
      IVP_SAVNX8U_XP(v00, a_store, rdst, width - x);
      IVP_SAPOSNX8U_FP(a_store, rdst);
    }
  }

  return (XI_ERR_OK);
}
