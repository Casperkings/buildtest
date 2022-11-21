#include "xi_lib.h"

#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/tie/xt_misc.h>

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

typedef int32_t XI_Q13_18;
typedef int32_t XI_Q15_16;

#define XCHAL_IVPN_SIMD_WIDTH  XCHAL_VISION_SIMD16
#define XCHAL_IVPN_SIMD_WIDTH_ XCHAL_IVPN_SIMD_WIDTH

#define IVP_SAV2NX8UPOS_FP IVP_SAPOS2NX8U_FP
#define IVP_SAVNX8UPOS_FP  IVP_SAPOSNX8U_FP
#define XI_IVP_GATHERNX8UT_V(a, b, c, d)                                       \
  IVP_GATHERNX8UT_V((const unsigned char *)(a), b, c, d)

XI_ERR_TYPE xiResizeBilinear_U8_Q13_18(const xi_pTile src, xi_pTile dst,
                                       XI_Q13_18 xscale, XI_Q13_18 yscale,
                                       XI_Q13_18 xshift, XI_Q13_18 yshift) {
  // XI_ERROR_CHECKS()
  //{
  //    XI_CHECK_TILE_U8(src);
  //    XI_CHECK_TILE_U8(dst);
  //    XI_CHECK_TILES_ARE_NOT_OVERLAP(src, dst);
  //
  //    XI_CHECK_ERROR(xscale != 0 && yscale !=0, XI_ERR_SCALE, "The scale
  //    factors should not be equal to zero");
  //}

  int sstride = XI_TILE_GET_PITCH(src);
  int dstride = XI_TILE_GET_PITCH(dst);

  int edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  int edge_height = XI_TILE_GET_EDGE_HEIGHT(src);

  int dwidth = XI_TILE_GET_WIDTH(dst);
  int dheight = XI_TILE_GET_HEIGHT(dst);

  int xs = XI_TILE_GET_X_COORD(src) - edge_width;
  int ys = XI_TILE_GET_Y_COORD(src) - edge_height;
  int xd = XI_TILE_GET_X_COORD(dst);
  int yd = XI_TILE_GET_Y_COORD(dst);

  // XI_ERROR_CHECKS_CONTINUE()
  //{
  //    XI_CHECK_ERROR(XT_ABS((xshift >> 16) - (xs << 2) + (((xd +  dwidth - 1)
  //    * xscale) >> 16)) < 32768, XI_ERR_OVERFLOW, "The scale factor and width
  //    lead to int overflow"); XI_CHECK_ERROR(XT_ABS((xshift >> 16) - (xs << 2)
  //    +   (xd +  dwidth - 1) * (xscale >> 16)) < 32768, XI_ERR_OVERFLOW, "The
  //    scale factor and width lead to int overflow");
  //    XI_CHECK_ERROR(XT_ABS((yshift >> 16) - (ys << 2) + (((yd + dheight - 1)
  //    * yscale) >> 16)) < 32768, XI_ERR_OVERFLOW, "The scale factor and width
  //    lead to int overflow"); XI_CHECK_ERROR(XT_ABS((yshift >> 16) - (ys << 2)
  //    +   (yd + dheight - 1) * (yscale >> 16)) < 32768, XI_ERR_OVERFLOW, "The
  //    scale factor and width lead to int overflow");
  //
  //    int x0 = (xd * xscale + xshift + (xscale >> 1) - (xs << 18) - (1 << 17))
  //    >> 18; int y0 = (yd * yscale + yshift + (yscale >> 1) - (ys << 18) - (1
  //    << 17) + 4) >> 18; // for +4 see comments below int x1 = ((xd + dwidth -
  //    1) * xscale + xshift + (xscale >> 1) - (xs << 18) - (1 << 17)) >> 18;
  //    int y1 = ((yd + dheight - 1) * yscale + yshift + (yscale >> 1) - (ys <<
  //    18) - (1 << 17) + 4) >> 18;
  //
  //    int xmin = XT_MIN(x0, x1); (void)xmin; // ensure variables are used in
  //    XI_ERROR_LEVEL_NO_ERROR mode int ymin = XT_MIN(y0, y1); (void)ymin; int
  //    xmax = XT_MAX(x0, x1) + 1; (void)xmax; int ymax = XT_MAX(y0, y1) + 1;
  //    (void)ymax;
  //
  //    XI_CHECK_ERROR2(xmin >= 0 && ymin >= 0 &&
  //                    xmax < XI_TILE_GET_WIDTH(src) + 2 * edge_width && ymax <
  //                    XI_TILE_GET_HEIGHT(src) + 2 * edge_height,
  //                    XI_ERR_DATASIZE,
  //                    ("The input tile (src) has to include all pixels from
  //                    (%d, %d) - (%d, %d) rectangle", xmin + xs, ymin + ys,
  //                    xmax + xs, ymax + ys));
  //}
  uint8_t *restrict psrc = (uint8_t *)OFFSET_PTR_NX8U(
      XI_TILE_GET_DATA_PTR(src), -edge_height, sstride, -edge_width);
  xb_vec2Nx8U *restrict pdst = (xb_vec2Nx8U *)XI_TILE_GET_DATA_PTR(dst);
  xb_vec2Nx8U *restrict rdst;
#if XCHAL_HAVE_VISION
  xb_vecNx8U *restrict rdstN;
#endif

  valign a_store = IVP_ZALIGN();

#if XCHAL_HAVE_VISION
  if (XT_ABS(xscale) < (5 << 18)) {
#endif
    int xstep =
        XT_MAX(1, XT_MIN(2 * XCHAL_IVPN_SIMD_WIDTH,
                         ((4 * XCHAL_IVPN_SIMD_WIDTH) << 18) / XT_ABS(xscale)));
    int swidth = XI_TILE_GET_WIDTH(src) + 2 * edge_width;

    xb_vecNx16 xseq = IVP_SEQNX16();
    vboolN mask0 = IVP_LTNX16(xseq, xstep);
    vboolN mask1 = IVP_LTNX16(IVP_ADDNX16(xseq, XCHAL_IVPN_SIMD_WIDTH), xstep);
    xseq = IVP_ADDNX16(xseq, xd);

    xb_vecNx16 hx = IVP_MULNX16PACKL(xseq, xscale >> 16);
    xb_vecNx48 wx = IVP_MULUSNX16(xscale, xseq);
    IVP_MULUSANX16(wx, (1 << 15), hx);
    IVP_MULUSANX16(wx, (1 << 15), hx);

    XI_Q13_18 offset = ((xshift - ((xs) << 18) + (xscale >> 1) - (1 << 17))) -
                       ((xscale * xstep));

    IVP_MULUUANX16(wx, 1, offset);
    IVP_MULUSANX16(wx, (1 << 15), (offset >> 16));
    IVP_MULUSANX16(wx, (1 << 15), (offset >> 16));

    XI_Q15_16 xoffset0 = (xscale * xstep);
    XI_Q15_16 xoffset1 = xscale * (XCHAL_IVPN_SIMD_WIDTH);

    for (int x = 0; x < dwidth;
         x += xstep, pdst = OFFSET_PTR_2NX8U(pdst, 0, dstride, xstep)) {
      XI_Q13_18 ysrc =
          (yd * yscale + yshift - (ys << 18) + (yscale >> 1) - (1 << 17)) +
          4; // +4 for yf calculation round in cycle

      IVP_MULUUANX16(wx, 1, xoffset0);
      IVP_MULUSANX16(wx, (1 << 15), (xoffset0 >> 16));
      IVP_MULUSANX16(wx, (1 << 15), (xoffset0 >> 16));
      xb_vecNx48 nx = wx;
      IVP_MULUUANX16(nx, 1, xoffset1);
      IVP_MULUSANX16(nx, (1 << 15), (xoffset1 >> 16));
      IVP_MULUSANX16(nx, (1 << 15), (xoffset1 >> 16));

      xb_vecNx16 xsrc0 = IVP_PACKMNX48(wx);
      xb_vecNx16 fsrc0 =
          IVP_SRLINX16(IVP_PACKVRNR2NX24_0(IVP_MOV2NX24_FROMNX48(wx), 2), 1);
      xb_vecNx16 xsrc1 = IVP_PACKMNX48(nx);
      xb_vecNx16 fsrc1 =
          IVP_SRLINX16(IVP_PACKVRNR2NX24_0(IVP_MOV2NX24_FROMNX48(nx), 2), 1);
      xsrc0 = IVP_SRAINX16(xsrc0, 2);
      xsrc1 = IVP_SRAINX16(xsrc1, 2);

      IVP_DSELNX16I(fsrc1, fsrc0, fsrc1, fsrc0, IVP_DSELI_DEINTERLEAVE_1);
      int16_t x_min = IVP_MOVAV16(IVP_MINNX16(IVP_RMINNX16T(xsrc0, mask0),
                                              IVP_RMINNX16T(xsrc1, mask1)));
      IVP_DSELNX16I(xsrc1, xsrc0, xsrc1, xsrc0, IVP_DSELI_DEINTERLEAVE_1);
      xsrc0 = IVP_SUBNX16(xsrc0, x_min);
      xsrc1 = IVP_SUBNX16(xsrc1, x_min);
      xb_vecNx16 xsrc0n = IVP_ADDNX16(xsrc0, 1);
      xb_vecNx16 xsrc1n = IVP_ADDNX16(xsrc1, 1);

#if XCHAL_HAVE_VISION
      xb_vec2Nx8 xsrc =
          IVP_SEL2NX8I(IVP_MOV2NX8_FROMNX16(xsrc1), IVP_MOV2NX8_FROMNX16(xsrc0),
                       IVP_SELI_8B_INTERLEAVE_1_EVEN);
      xb_vec2Nx8 xsrcn = IVP_SEL2NX8I(IVP_MOV2NX8_FROMNX16(xsrc1n),
                                      IVP_MOV2NX8_FROMNX16(xsrc0n),
                                      IVP_SELI_8B_INTERLEAVE_1_EVEN);
#endif

      rdst = pdst;
      int dstep =
          dstride - dwidth + XT_MAX(dwidth, x + 2 * XCHAL_IVPN_SIMD_WIDTH);
      dstep -= 2 * XCHAL_IVPN_SIMD_WIDTH;

      xb_vec2Nx8U *qsrc = OFFSET_PTR_2NX8U(psrc, 0, sstride, x_min);

#if XCHAL_HAVE_VISION && XCHAL_VISION_TYPE == 6

      xb_vecN_2x32v vysrc = ysrc;
      xb_vecN_2x32v vyscale = yscale;

      /* vone_q15 = {32767, 0, 0, ...} */
      xb_vecNx16 vone_q15 =
          IVP_MOVNX16T(32767, 0, IVP_EQNX16(IVP_SEQNX16(), 0));
      /* vone = {1, 0, 0, ...} */
      xb_vecNx16 vone = IVP_MOVNX16T(1, 0, IVP_EQNX16(IVP_SEQNX16(), 0));

      xb_vecN_2x32v vshift =
          IVP_MOVN_2X32T(3, 18, IVP_EQN_2X32(IVP_SEQN_2X32(), 0));

      for (int y = 0; y < dheight; ++y) {
        xb_vecN_2x32v vysrc_sh = IVP_SRAN_2X32(vysrc, vshift);

        /* yf = FRAC15(vysrc) << 16 + ADD_SATURATION(1, 32767 - FRAC15(vysrc))
         */
        // vyf = (vysrc & ((1 << 18) - 1)) >> 3
        xb_vecN_2x32v vyf = IVP_ANDN_2X32(vysrc_sh, (1 << 15) - 1);
        xb_vecNx16 vyf_s16 = IVP_MOVNX16_FROMN_2X32(vyf);
        // vyf_s16 = {FRAC15(vysrc), FRAC15(vysrc),...}
        vyf_s16 = IVP_REPNX16(vyf_s16, 0);
        IVP_SUBNX16T(vyf_s16, vone_q15, vyf_s16, IVP_EQNX16(IVP_SEQNX16(), 0));
        vyf_s16 = IVP_ADDSNX16(vone, vyf_s16);
        vyf = IVP_MOVN_2X32_FROMNX16(vyf_s16);
        int yf = IVP_EXTRN_2X32(vyf, 0);

        /* ysrc_sh = (vysrc >> 18) */
        int ysrc_sh = IVP_EXTRN_2X32(vysrc_sh, 1);

        xb_vec2Nx8U *rsrc0 = OFFSET_PTR_2NX8U(qsrc, ysrc_sh, sstride, 0);
        xb_vec2Nx8U *rsrc1 = OFFSET_PTR_2NX8U(rsrc0, 1, sstride, 0);

        vysrc = IVP_ADDN_2X32(vysrc, vyscale);

        xb_vec2Nx8U val00, val01;
        valign a_load0 = IVP_LA2NX8U_PP(rsrc0);
        IVP_LAV2NX8U_XP(val00, a_load0, rsrc0, swidth - x_min);
        IVP_LAV2NX8U_XP(val01, a_load0, rsrc0,
                        swidth - x_min - 2 * XCHAL_IVPN_SIMD_WIDTH);

        xb_vec2Nx8U val10, val11;
        valign a_load1 = IVP_LA2NX8U_PP(rsrc1);
        IVP_LAV2NX8U_XP(val10, a_load1, rsrc1, swidth - x_min);
        IVP_LAV2NX8U_XP(val11, a_load1, rsrc1,
                        swidth - x_min - 2 * XCHAL_IVPN_SIMD_WIDTH);

        xb_vec2Nx8U vdst00 = IVP_SEL2NX8(val01, val00, xsrc);
        xb_vec2Nx8U vdst01 = IVP_SEL2NX8(val01, val00, xsrcn);
        xb_vec2Nx8U vdst10 = IVP_SEL2NX8(val11, val10, xsrc);
        xb_vec2Nx8U vdst11 = IVP_SEL2NX8(val11, val10, xsrcn);

        xb_vec2Nx24 w0 = IVP_MULUSP2N8XR16(vdst10, vdst00, yf);
        xb_vec2Nx24 w1 = IVP_MULUSP2N8XR16(vdst11, vdst01, yf);
        xb_vec2Nx8U v0 = IVP_PACKVRU2NX24(w0, 15);
        xb_vec2Nx8U v1 = IVP_PACKVRU2NX24(w1, 15);

        w0 = IVP_MULUSI2NX8X16(v1, fsrc1, fsrc0);
        IVP_MULUSAI2NX8X16(w0, v0, IVP_ADDSNX16(1, IVP_SUBNX16(32767, fsrc1)),
                           IVP_ADDSNX16(1, IVP_SUBNX16(32767, fsrc0)));
        vdst00 = IVP_PACKVRU2NX24(w0, 15);

        IVP_SAV2NX8U_XP(vdst00, a_store, rdst, dwidth - x);
        IVP_SAV2NX8UPOS_FP(a_store, rdst);
        rdst = OFFSET_PTR_2NX8U(rdst, 1, dstep, 0);
      }
#else
    for (int y = 0; y < dheight; ++y, ysrc += yscale) {
      xb_vec2Nx8U *rsrc0 = OFFSET_PTR_2NX8U(qsrc, ysrc >> 18, sstride, 0);
      xb_vec2Nx8U *rsrc1 = OFFSET_PTR_2NX8U(rsrc0, 1, sstride, 0);
      int16_t yf = (ysrc & ((1 << 18) - 1)) >> 3;

      xb_vec2Nx8U val00, val01;
      valign a_load0 = IVP_LA2NX8U_PP(rsrc0);
      IVP_LAV2NX8U_XP(val00, a_load0, rsrc0, swidth - x_min);
      IVP_LAV2NX8U_XP(val01, a_load0, rsrc0,
                      swidth - x_min - 2 * XCHAL_IVPN_SIMD_WIDTH);

      xb_vec2Nx8U val10, val11;
      valign a_load1 = IVP_LA2NX8U_PP(rsrc1);
      IVP_LAV2NX8U_XP(val10, a_load1, rsrc1, swidth - x_min);
      IVP_LAV2NX8U_XP(val11, a_load1, rsrc1,
                      swidth - x_min - 2 * XCHAL_IVPN_SIMD_WIDTH);

#if XCHAL_HAVE_VISION
      xb_vec2Nx8U vdst00 = IVP_SEL2NX8(val01, val00, xsrc);
      xb_vec2Nx8U vdst01 = IVP_SEL2NX8(val01, val00, xsrcn);
      xb_vec2Nx8U vdst10 = IVP_SEL2NX8(val11, val10, xsrc);
      xb_vec2Nx8U vdst11 = IVP_SEL2NX8(val11, val10, xsrcn);
#else
      xb_vec2Nx8U vdst00 =
          IVP_SEL2NX8(val01, val00, IVP_MOVVSV(xsrc1, 0), IVP_MOVVSV(xsrc0, 0));
      xb_vec2Nx8U vdst01 = IVP_SEL2NX8(val01, val00, IVP_MOVVSV(xsrc1n, 0),
                                       IVP_MOVVSV(xsrc0n, 0));
      xb_vec2Nx8U vdst10 =
          IVP_SEL2NX8(val11, val10, IVP_MOVVSV(xsrc1, 0), IVP_MOVVSV(xsrc0, 0));
      xb_vec2Nx8U vdst11 = IVP_SEL2NX8(val11, val10, IVP_MOVVSV(xsrc1n, 0),
                                       IVP_MOVVSV(xsrc0n, 0));
#endif

      xb_vecNx16 one_yf = IVP_ADDSNX16(1, IVP_SUBNX16(32767, yf));
      xb_vec2Nx24 w1 = IVP_MULUSI2NX8X16(vdst10, yf, yf);
      IVP_MULUSAI2NX8X16(w1, vdst00, one_yf, one_yf);
      xb_vec2Nx24 w0 = IVP_MULUSI2NX8X16(vdst11, yf, yf);
      IVP_MULUSAI2NX8X16(w0, vdst01, one_yf, one_yf);

      w0 = IVP_MULUSI2NX8X16(IVP_PACKVRU2NX24(w0, 15), fsrc1, fsrc0);
      IVP_MULUSAI2NX8X16(w0, IVP_PACKVRU2NX24(w1, 15),
                         IVP_ADDSNX16(1, IVP_SUBNX16(32767, fsrc1)),
                         IVP_ADDSNX16(1, IVP_SUBNX16(32767, fsrc0)));
      vdst00 = IVP_PACKVRU2NX24(w0, 15);

      IVP_SAV2NX8U_XP(vdst00, a_store, rdst, dwidth - x);
      IVP_SAV2NX8UPOS_FP(a_store, rdst);
      rdst = OFFSET_PTR_2NX8U(rdst, 1, dstep, 0);
    }
#endif
    }
#if XCHAL_HAVE_VISION
  } else {
    xb_vecN_2x64w xoffsw = IVP_MULN_2X16X32_0(
        1, xshift - (xs << 18) + xd * xscale + (xscale >> 1) - (1 << 17));
    IVP_MULUSAN_2X16X32_0(xoffsw, IVP_MOVNX16_FROMN_2X32(IVP_SEQN_2X32()),
                          xscale);

    for (int x = 0; x < dwidth; x += XCHAL_IVPN_SIMD_WIDTH,
             pdst = OFFSET_PTR_2NX8U(pdst, 0, dstride, XCHAL_IVPN_SIMD_WIDTH)) {
      xb_vecN_2x32v xoffs_0 = IVP_PACKVRNRN_2X64W(xoffsw, 18);
      xb_vecN_2x32v fsrc_0 = IVP_PACKVRNRN_2X64W(xoffsw, 2);
      IVP_MULUSAN_2X16X32_0(xoffsw, XCHAL_IVPN_SIMD_WIDTH / 2, xscale);
      xb_vecN_2x32v xoffs_1 = IVP_PACKVRNRN_2X64W(xoffsw, 18);
      xb_vecN_2x32v fsrc_1 = IVP_PACKVRNRN_2X64W(xoffsw, 2);
      IVP_MULUSAN_2X16X32_0(xoffsw, XCHAL_IVPN_SIMD_WIDTH / 2, xscale);
      xb_vecNx16 xoffs16 = IVP_SELNX16I(IVP_MOVNX16_FROMN_2X32(xoffs_1),
                                        IVP_MOVNX16_FROMN_2X32(xoffs_0),
                                        IVP_SELI_16B_EXTRACT_1_OF_2_OFF_0);
      xb_vecNx16 fsrc16 = IVP_SELNX16I(IVP_MOVNX16_FROMN_2X32(fsrc_1),
                                       IVP_MOVNX16_FROMN_2X32(fsrc_0),
                                       IVP_SELI_16B_EXTRACT_1_OF_2_OFF_0);
      fsrc16 = IVP_SRLINX16(fsrc16, 1);

      vboolN xmask0 = IVP_LTRSN(2 * (dwidth - x));
      vboolN xmask1 = IVP_LTRSN(2 * (dwidth - x - XCHAL_IVPN_SIMD_WIDTH / 2));

      XI_Q13_18 yoffs =
          (yd * yscale + yshift - (ys << 18) + (yscale >> 1) - (1 << 17)) +
          4; // +4 for yf calculation round in cycle

      xb_vecNx16 xoffs00, xoffs01;
      IVP_DSELNX16I(xoffs01, xoffs00, IVP_ADDNX16(xoffs16, 1), xoffs16,
                    IVP_DSELI_INTERLEAVE_1);

      rdstN = (xb_vecNx8U *)pdst;
      int dstep = dstride - XT_MIN(XCHAL_IVPN_SIMD_WIDTH_, dwidth - x);

      xoffs00 = IVP_MOVNX16T(xoffs00, 0, xmask0);
      xoffs01 = IVP_MOVNX16T(xoffs01, 0, xmask1);

#if XCHAL_VISION_TYPE == 6
      xb_vecN_2x32v vyoffs = yoffs;
      xb_vecN_2x32v vyscale = yscale;

      /* vone_q15 = {32767, 0, 0, ...} */
      xb_vecNx16 vone_q15 =
          IVP_MOVNX16T(32767, 0, IVP_EQNX16(IVP_SEQNX16(), 0));
      /* vone = {1, 0, 0, ...} */
      xb_vecNx16 vone = IVP_MOVNX16T(1, 0, IVP_EQNX16(IVP_SEQNX16(), 0));

      xb_vecN_2x32v vshift =
          IVP_MOVN_2X32T(3, 18, IVP_EQN_2X32(IVP_SEQN_2X32(), 0));
      uint32_t offset = yoffs;

      for (int y = 0; y < dheight; y++) {
        xb_vecN_2x32v vysrc_sh = IVP_SRAN_2X32(vyoffs, vshift);

        /* yf = FRAC15(vyoffs) << 16 + ADD_SATURATION(1, 32767 - FRAC15(vyoffs))
         */
        // vyf = (vyoffs & ((1 << 18) - 1)) >> 3
        xb_vecN_2x32v vyf = IVP_ANDN_2X32(vysrc_sh, (1 << 15) - 1);
        xb_vecNx16 vyf_s16 = IVP_MOVNX16_FROMN_2X32(vyf);
        // vyf_s16 = {FRAC15(vyoffs), FRAC15(vyoffs),...}
        vyf_s16 = IVP_REPNX16(vyf_s16, 0);
        IVP_SUBNX16T(vyf_s16, vone_q15, vyf_s16, IVP_EQNX16(IVP_SEQNX16(), 0));
        vyf_s16 = IVP_ADDSNX16(vone, vyf_s16);
        vyf = IVP_MOVN_2X32_FROMNX16(vyf_s16);
        int yf = IVP_EXTRN_2X32(vyf, 0);

        vyoffs = IVP_ADDN_2X32(vyoffs, vyscale);

        uint8_t *psrc0 =
            (uint8_t *)OFFSET_PTR_NX8U(psrc, (offset >> 18), sstride, 0);
        uint8_t *psrc1 = (uint8_t *)OFFSET_PTR_NX8U(psrc0, 1, sstride, 0);

        xb_vecNx16 vdst00 = XI_IVP_GATHERNX8UT_V(psrc0, xoffs00, xmask0, 4);
        xb_vecNx16 vdst01 = XI_IVP_GATHERNX8UT_V(psrc0, xoffs01, xmask1, 4);
        xb_vecNx16 vdst10 = XI_IVP_GATHERNX8UT_V(psrc1, xoffs00, xmask0, 4);
        xb_vecNx16 vdst11 = XI_IVP_GATHERNX8UT_V(psrc1, xoffs01, xmask1, 4);

        offset += yscale;

        xb_vecNx48 w00 = IVP_MULUSPN16XR16(vdst10, vdst00, yf);
        vdst00 = IVP_PACKVRNX48(w00, 15);
        xb_vecNx48 w01 = IVP_MULUSPN16XR16(vdst11, vdst01, yf);
        vdst01 = IVP_PACKVRNX48(w01, 15);

        IVP_DSELNX16I(vdst01, vdst00, vdst01, vdst00, IVP_DSELI_DEINTERLEAVE_1);

        w00 = IVP_MULUSPNX16(IVP_SUBNX16(1 << 15, fsrc16), vdst00, fsrc16,
                             vdst01);
        vdst00 = IVP_PACKVRNX48(w00, 15);

        IVP_SAVNX8U_XP(vdst00, a_store, rdstN, dwidth - x);
        IVP_SAVNX8UPOS_FP(a_store, rdstN);
        rdstN = OFFSET_PTR_NX8U(rdstN, 1, dstep, 0);
      }
#else
      for (int y = 0; y < dheight; y++) {
        int16_t yf = (yoffs & ((1 << 18) - 1)) >> 3;

        uint8_t *psrc0 =
            (uint8_t *)OFFSET_PTR_NX8U(psrc, (yoffs >> 18), sstride, 0);
        uint8_t *psrc1 = (uint8_t *)OFFSET_PTR_NX8U(psrc0, 1, sstride, 0);

        xb_vecNx16 vdst00 = XI_IVP_GATHERNX8UT_V(psrc0, xoffs00, xmask0, 4);
        xb_vecNx16 vdst01 = XI_IVP_GATHERNX8UT_V(psrc0, xoffs01, xmask1, 4);
        xb_vecNx16 vdst10 = XI_IVP_GATHERNX8UT_V(psrc1, xoffs00, xmask0, 4);
        xb_vecNx16 vdst11 = XI_IVP_GATHERNX8UT_V(psrc1, xoffs01, xmask1, 4);

        IVP_MULANX16PACKQ(vdst00, yf, IVP_SUBNX16(vdst10, vdst00));
        IVP_MULANX16PACKQ(vdst01, yf, IVP_SUBNX16(vdst11, vdst01));

        IVP_DSELNX16I(vdst01, vdst00, vdst01, vdst00, IVP_DSELI_DEINTERLEAVE_1);

        IVP_MULANX16PACKQ(vdst00, fsrc16, IVP_SUBNX16(vdst01, vdst00));

        IVP_SAVNX8U_XP(vdst00, a_store, rdstN, dwidth - x);
        IVP_SAVNX8UPOS_FP(a_store, rdstN);
        rdstN = OFFSET_PTR_NX8U(rdstN, 1, dstep, 0);

        yoffs += yscale;
      }
#endif
    }
  }
#endif

  return 0; // XI_ERROR_STATUS();
}
