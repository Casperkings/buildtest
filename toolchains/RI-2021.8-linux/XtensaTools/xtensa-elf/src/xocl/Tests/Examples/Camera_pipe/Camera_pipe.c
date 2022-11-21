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

#define XI__SEL2NX8I(tail, body, unused)                                       \
  IVP_MOV2NX8U_FROMNX16(IVP_SELNX16I(IVP_MOVNX16_FROM2NX8U(tail),              \
                                     IVP_MOVNX16_FROM2NX8U(body),              \
                                     IVP_SELI_ROTATE_RIGHT_1))

#define SORT_PIX(p0, p1)                                                       \
  temp = IVP_MINNX16(p0, p1);                                                  \
  p1 = IVP_MAXNX16(p0, p1);                                                    \
  p0 = temp;

#define SORT_ROW(v0, v1, v2)                                                   \
  {                                                                            \
    xb_vecNx16 temp;                                                           \
    SORT_PIX(v1, v2);                                                          \
    SORT_PIX(v0, v1);                                                          \
    SORT_PIX(v1, v2);                                                          \
  }

// This macro calculates the median of values {v0,...,v8}
// NOTE: sets (v0, v1, v2), (v3, v4, v5), (v6, v7, v8) have to be sorted
#define GET_MEDIAN(v0, v1, v2, v3, v4, v5, v6, v7, v8, vtmp, vmed)             \
  {                                                                            \
    vtmp = IVP_MAXNX16(v0, v3);                                                \
    v0 = IVP_MAXNX16(v4, v7);                                                  \
    vmed = IVP_MINNX16(v4, v7);                                                \
    vmed = IVP_MAXNX16(v1, vmed);                                              \
    v1 = IVP_MINNX16(v5, v8);                                                  \
    v2 = IVP_MINNX16(v2, v1);                                                  \
    vmed = IVP_MINNX16(vmed, v0);                                              \
    v1 = IVP_MINNX16(vmed, v2);                                                \
    v2 = IVP_MAXNX16(vmed, v2);                                                \
    vtmp = IVP_MAXNX16(vtmp, v6);                                              \
    vmed = IVP_MAXNX16(vtmp, v1);                                              \
    vmed = IVP_MINNX16(vmed, v2);                                              \
  }

//////////////

#define SORT_PIX_8U2(p0, p1)                                                   \
  IVP_BMINU2NX8(unused, temp, p0, p1);                                         \
  IVP_BMAXU2NX8(unused, p1, p0, p1);                                           \
  p0 = temp;

#define SORT_ROW_8U2(v0, v1, v2)                                               \
  {                                                                            \
    vbool2N unused;                                                            \
    xb_vec2Nx8 temp;                                                           \
    SORT_PIX_8U2(v1, v2);                                                      \
    SORT_PIX_8U2(v0, v1);                                                      \
    SORT_PIX_8U2(v1, v2);                                                      \
  }

#define GET_MEDIAN_8U2(v0, v1, v2, v3, v4, v5, v6, v7, v8, vtmp, vmed)         \
  {                                                                            \
    vbool2N unused;                                                            \
    IVP_BMAXU2NX8(unused, vtmp, v0, v3);                                       \
    IVP_BMAXU2NX8(unused, v0, v4, v7);                                         \
    IVP_BMINU2NX8(unused, vmed, v4, v7);                                       \
    IVP_BMAXU2NX8(unused, vmed, v1, vmed);                                     \
    IVP_BMINU2NX8(unused, v1, v5, v8);                                         \
    IVP_BMINU2NX8(unused, v2, v2, v1);                                         \
    IVP_BMINU2NX8(unused, vmed, vmed, v0);                                     \
    IVP_BMINU2NX8(unused, v1, vmed, v2);                                       \
    IVP_BMAXU2NX8(unused, v2, vmed, v2);                                       \
    IVP_BMAXU2NX8(unused, vtmp, vtmp, v6);                                     \
    IVP_BMAXU2NX8(unused, vmed, vtmp, v1);                                     \
    IVP_BMINU2NX8(unused, vmed, vmed, v2);                                     \
  }

xb_vecNx16 _CLAMP(xb_vecNx16 a, xb_vecNx16 min, xb_vecNx16 max) {
  xb_vecNx16 out = IVP_MOVNX16T(min, a, IVP_LTNX16(a, min));
  out = IVP_MOVNX16T(out, max, IVP_LTNX16(out, max));

  return out;
}

void hot_pixel_suppression(short *restrict input, int inputPitch,
                           int inputWidth, int inputHeight,
                           short *restrict output, int outputPitch) {
  int x, y;

  valign a_store = IVP_ZALIGN();
  for (x = 2; x < inputWidth - 2; x += 32) {
    xb_vecNx16 *pdst = OFFSET_PTR_NX16(output, 0, 0, x - 2);
    for (y = 2; y < inputHeight - 2; y++) {
      xb_vecNx16 xRight, xLeft, xBot, xTop, xCurrent;

      xb_vecNx16 *psrc = OFFSET_PTR_NX16(input, y, inputPitch, x);
      valign a = IVP_LANX16_PP(psrc);
      IVP_LANX16_IP(xCurrent, a, psrc);

      psrc = OFFSET_PTR_NX16(input, (y - 2), inputPitch, x);
      a = IVP_LANX16_PP(psrc);
      IVP_LANX16_IP(xTop, a, psrc);

      psrc = OFFSET_PTR_NX16(input, (y + 2), inputPitch, x);
      a = IVP_LANX16_PP(psrc);
      IVP_LANX16_IP(xBot, a, psrc);

      psrc = OFFSET_PTR_NX16(input, y, inputPitch, x + 2);
      a = IVP_LANX16_PP(psrc);
      IVP_LANX16_IP(xLeft, a, psrc);

      psrc = OFFSET_PTR_NX16(input, y, inputPitch, x - 2);
      a = IVP_LANX16_PP(psrc);
      IVP_LANX16_IP(xRight, a, psrc);

      xb_vecNx16 xout = IVP_MINNX16(
          xCurrent,
          IVP_MAXNX16(IVP_MAXNX16(IVP_MAXNX16(xTop, xBot), xLeft), xRight));

      IVP_SANX16_IP(xout, a_store, pdst);
      IVP_SAPOSNX16_FP(a_store, pdst);
      pdst = OFFSET_PTR_NX16(pdst, 1, outputPitch, -32);
    }
  }
}

void deinterleave(short *restrict input, int inputPitch, int inputWidth,
                  int inputHeight, int outputPitch, short *restrict pr_r,
                  short *restrict pb_b, short *restrict pg_gr,
                  short *restrict pg_gb) {
  int x, y;

  valign a_store = IVP_ZALIGN();
  for (x = 0; x < inputWidth; x += 64) {
    xb_vecNx16 *pdstg_gr = OFFSET_PTR_NX16(pg_gr, 0, 0, x / 2);
    xb_vecNx16 *pdstr_r = OFFSET_PTR_NX16(pr_r, 0, 0, x / 2);
    xb_vecNx16 *pdstb_b = OFFSET_PTR_NX16(pb_b, 0, 0, x / 2);
    xb_vecNx16 *pdstg_gb = OFFSET_PTR_NX16(pg_gb, 0, 0, x / 2);
    xb_vecNx16 *psrc = OFFSET_PTR_NX16(input, 0, 0, x);
    // xb_vecNx16 *rpsrc;
    for (y = 0; y < inputHeight; y += 2) {
      xb_vecNx16 x00, x01, x02, x03;

      valign a = IVP_LANX16_PP(psrc);
      IVP_LANX16_IP(x00, a, psrc);
      IVP_LANX16_IP(x01, a, psrc);

      psrc = OFFSET_PTR_NX16(psrc, 1, inputPitch, -64);
      a = IVP_LANX16_PP(psrc);
      IVP_LANX16_IP(x02, a, psrc);
      IVP_LANX16_IP(x03, a, psrc);

      psrc = OFFSET_PTR_NX16(psrc, 1, inputPitch, -64);

      xb_vecNx16 r_r, g_gr, b_b, g_gb;

      IVP_DSELNX16I(r_r, g_gr, x01, x00, IVP_DSELI_DEINTERLEAVE_1);
      IVP_DSELNX16I(g_gb, b_b, x03, x02, IVP_DSELI_DEINTERLEAVE_1);

      IVP_SANX16_IP(g_gr, a_store, pdstg_gr);
      IVP_SAPOSNX16_FP(a_store, pdstg_gr);
      pdstg_gr = OFFSET_PTR_NX16(pdstg_gr, 1, outputPitch, -32);

      IVP_SANX16_IP(r_r, a_store, pdstr_r);
      IVP_SAPOSNX16_FP(a_store, pdstr_r);
      pdstr_r = OFFSET_PTR_NX16(pdstr_r, 1, outputPitch, -32);

      IVP_SANX16_IP(b_b, a_store, pdstb_b);
      IVP_SAPOSNX16_FP(a_store, pdstb_b);
      pdstb_b = OFFSET_PTR_NX16(pdstb_b, 1, outputPitch, -32);

      IVP_SANX16_IP(g_gb, a_store, pdstg_gb);
      IVP_SAPOSNX16_FP(a_store, pdstg_gb);
      pdstg_gb = OFFSET_PTR_NX16(pdstg_gb, 1, outputPitch, -32);
    }
  }
}

void Demosaic_pass1(short *restrict pg_gr, short *restrict pg_gb,
                    int inputPitch, int inputWidth, int inputHeight,
                    short *restrict tmp_g_r, short *restrict tmp_g_b) {
  int x, y;
  int outputPitch = inputPitch;
  valign a_store = IVP_ZALIGN();
  for (x = 1; x < inputWidth - 1; x += 32) {
    xb_vecNx16 *psrcg_gr = OFFSET_PTR_NX16(pg_gr, 1, inputPitch, x);
    xb_vecNx16 *psrcg_gb = OFFSET_PTR_NX16(pg_gb, 1, inputPitch, x);
    xb_vecNx16 *pdst_tmp_g_r = OFFSET_PTR_NX16(tmp_g_r, 1, inputPitch, x);
    xb_vecNx16 *pdst_tmp_g_b = OFFSET_PTR_NX16(tmp_g_b, 1, inputPitch, x);

    for (y = 1; y < inputHeight - 1; y++) {
      xb_vecNx16 g_gr_c, g_gr_nc, g_gr_nr;
      valign a = IVP_LANX16_PP(psrcg_gr);
      IVP_LANX16_IP(g_gr_c, a, psrcg_gr);

      psrcg_gr = OFFSET_PTR_NX16(psrcg_gr, 0, 0, -31);
      a = IVP_LANX16_PP(psrcg_gr);
      IVP_LANX16_IP(g_gr_nc, a, psrcg_gr);

      psrcg_gr = OFFSET_PTR_NX16(psrcg_gr, 1, inputPitch, -33);
      a = IVP_LANX16_PP(psrcg_gr);
      IVP_LANX16_IP(g_gr_nr, a, psrcg_gr);

      psrcg_gr = OFFSET_PTR_NX16(psrcg_gr, 0, 0, -32);

      xb_vecNx16 g_gb_c, g_gb_pr, g_gb_pc;
      a = IVP_LANX16_PP(psrcg_gb);
      IVP_LANX16_IP(g_gb_c, a, psrcg_gb);

      psrcg_gb = OFFSET_PTR_NX16(psrcg_gb, -1, inputPitch, -32);
      a = IVP_LANX16_PP(psrcg_gb);
      IVP_LANX16_IP(g_gb_pr, a, psrcg_gb);

      psrcg_gb = OFFSET_PTR_NX16(psrcg_gb, 1, inputPitch, -33);
      a = IVP_LANX16_PP(psrcg_gb);
      IVP_LANX16_IP(g_gb_pc, a, psrcg_gb);

      psrcg_gb = OFFSET_PTR_NX16(psrcg_gb, 1, inputPitch, -31);

      xb_vecNx16 gv_r = IVP_AVGRNX16(g_gb_pr, g_gb_c);
      xb_vecNx16U gvd_r = IVP_ABSSUBNX16(g_gb_pr, g_gb_c);
      xb_vecNx16 gh_r = IVP_AVGRNX16(g_gr_nc, g_gr_c);
      xb_vecNx16U ghd_r = IVP_ABSSUBNX16(g_gr_nc, g_gr_c);

      xb_vecNx16 g_r = IVP_MOVNX16T(gh_r, gv_r, IVP_LTNX16(ghd_r, gvd_r));

      xb_vecNx16 gv_b = IVP_AVGRNX16(g_gr_nr, g_gr_c);
      xb_vecNx16U gvd_b = IVP_ABSSUBNX16(g_gr_nr, g_gr_c);
      xb_vecNx16 gh_b = IVP_AVGRNX16(g_gb_pc, g_gb_c);
      xb_vecNx16U ghd_b = IVP_ABSSUBNX16(g_gb_pc, g_gb_c);

      xb_vecNx16 g_b = IVP_MOVNX16T(gh_b, gv_b, IVP_LTNX16(ghd_b, gvd_b));

      IVP_SANX16_IP(g_r, a_store, pdst_tmp_g_r);
      IVP_SAPOSNX16_FP(a_store, pdst_tmp_g_r);
      pdst_tmp_g_r = OFFSET_PTR_NX16(pdst_tmp_g_r, 1, outputPitch, -32);

      IVP_SANX16_IP(g_b, a_store, pdst_tmp_g_b);
      IVP_SAPOSNX16_FP(a_store, pdst_tmp_g_b);
      pdst_tmp_g_b = OFFSET_PTR_NX16(pdst_tmp_g_b, 1, outputPitch, -32);
    }
  }
}

void Demosaic_pass2(short *restrict pr_r, short *restrict pb_b,
                    short *restrict pg_gr, short *restrict pg_gb,
                    int inputPitch_deinterleave, short *restrict tmp_g_r,
                    short *restrict tmp_g_b, int inputWidth, int inputHeight,
                    short *restrict tmp_r_gr, short *restrict tmp_b_gr,
                    short *restrict tmp_r_gb, short *restrict tmp_b_gb,
                    short *restrict tmp_r_b, short *restrict tmp_b_r) {
  int x, y;
  int outputPitch = inputPitch_deinterleave;
  valign a_store = IVP_ZALIGN();
  for (x = 2; x < inputWidth - 2; x += 32) {
    xb_vecNx16 *psrcr_r = OFFSET_PTR_NX16(pr_r, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrcb_b = OFFSET_PTR_NX16(pb_b, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrcg_gb =
        OFFSET_PTR_NX16(pg_gb, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrcg_gr =
        OFFSET_PTR_NX16(pg_gr, 2, inputPitch_deinterleave, x);

    xb_vecNx16 *psrcg_r =
        OFFSET_PTR_NX16(tmp_g_r, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrcg_b =
        OFFSET_PTR_NX16(tmp_g_b, 2, inputPitch_deinterleave, x);

    xb_vecNx16 *pdst_tmp_r_gr =
        OFFSET_PTR_NX16(tmp_r_gr, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *pdst_tmp_b_gr =
        OFFSET_PTR_NX16(tmp_b_gr, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *pdst_tmp_r_gb =
        OFFSET_PTR_NX16(tmp_r_gb, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *pdst_tmp_b_gb =
        OFFSET_PTR_NX16(tmp_b_gb, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *pdst_tmp_r_b =
        OFFSET_PTR_NX16(tmp_r_b, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *pdst_tmp_b_r =
        OFFSET_PTR_NX16(tmp_b_r, 2, inputPitch_deinterleave, x);

    for (y = 2; y < inputHeight - 2; y++) {
      xb_vecNx16 g_gr_c, g_gb_c;

      valign a = IVP_LANX16_PP(psrcg_gr);
      IVP_LANX16_IP(g_gr_c, a, psrcg_gr);
      psrcg_gr = OFFSET_PTR_NX16(psrcg_gr, 1, inputPitch_deinterleave, -32);

      a = IVP_LANX16_PP(psrcg_gb);
      IVP_LANX16_IP(g_gb_c, a, psrcg_gb);
      psrcg_gb = OFFSET_PTR_NX16(psrcg_gb, 1, inputPitch_deinterleave, -32);

      xb_vecNx16 g_r_c, g_r_pc, g_r_nr, g_r_pc_nr;
      a = IVP_LANX16_PP(psrcg_r);
      IVP_LANX16_IP(g_r_c, a, psrcg_r);

      psrcg_r = OFFSET_PTR_NX16(psrcg_r, 0, 0, -33);
      a = IVP_LANX16_PP(psrcg_r);
      IVP_LANX16_IP(g_r_pc, a, psrcg_r);

      psrcg_r = OFFSET_PTR_NX16(psrcg_r, 1, inputPitch_deinterleave, -32);
      a = IVP_LANX16_PP(psrcg_r);
      IVP_LANX16_IP(g_r_pc_nr, a, psrcg_r);

      psrcg_r = OFFSET_PTR_NX16(psrcg_r, 0, inputPitch_deinterleave, -31);
      a = IVP_LANX16_PP(psrcg_r);
      IVP_LANX16_IP(g_r_nr, a, psrcg_r);
      psrcg_r = OFFSET_PTR_NX16(psrcg_r, 0, 0, -32);

      xb_vecNx16 r_r_c, r_r_pc, r_r_nr, r_r_pc_nr;
      a = IVP_LANX16_PP(psrcr_r);
      IVP_LANX16_IP(r_r_c, a, psrcr_r);

      psrcr_r = OFFSET_PTR_NX16(psrcr_r, 0, 0, -33);
      a = IVP_LANX16_PP(psrcr_r);
      IVP_LANX16_IP(r_r_pc, a, psrcr_r);

      psrcr_r = OFFSET_PTR_NX16(psrcr_r, 1, inputPitch_deinterleave, -32);
      a = IVP_LANX16_PP(psrcr_r);
      IVP_LANX16_IP(r_r_pc_nr, a, psrcr_r);

      psrcr_r = OFFSET_PTR_NX16(psrcr_r, 0, 0, -31);
      a = IVP_LANX16_PP(psrcr_r);
      IVP_LANX16_IP(r_r_nr, a, psrcr_r);
      psrcr_r = OFFSET_PTR_NX16(psrcr_r, 0, 0, -32);

      xb_vecNx16 g_b_c, g_b_pr, g_b_nc, g_b_nc_pr;
      a = IVP_LANX16_PP(psrcg_b);
      IVP_LANX16_IP(g_b_c, a, psrcg_b);

      psrcg_b = OFFSET_PTR_NX16(psrcg_b, -1, inputPitch_deinterleave, -32);
      a = IVP_LANX16_PP(psrcg_b);
      IVP_LANX16_IP(g_b_pr, a, psrcg_b);

      psrcg_b = OFFSET_PTR_NX16(psrcg_b, 0, 0, -31);
      a = IVP_LANX16_PP(psrcg_b);
      IVP_LANX16_IP(g_b_nc_pr, a, psrcg_b);

      psrcg_b = OFFSET_PTR_NX16(psrcg_b, 1, inputPitch_deinterleave, -32);
      a = IVP_LANX16_PP(psrcg_b);
      IVP_LANX16_IP(g_b_nc, a, psrcg_b);
      psrcg_b = OFFSET_PTR_NX16(psrcg_b, 1, inputPitch_deinterleave, -33);

      xb_vecNx16 b_b_c, b_b_pr, b_b_nc, b_b_nc_pr;
      a = IVP_LANX16_PP(psrcb_b);
      IVP_LANX16_IP(b_b_c, a, psrcb_b);

      psrcb_b = OFFSET_PTR_NX16(psrcb_b, -1, inputPitch_deinterleave, -32);
      a = IVP_LANX16_PP(psrcb_b);
      IVP_LANX16_IP(b_b_pr, a, psrcb_b);

      psrcb_b = OFFSET_PTR_NX16(psrcb_b, 0, 0, -31);
      a = IVP_LANX16_PP(psrcb_b);
      IVP_LANX16_IP(b_b_nc_pr, a, psrcb_b);

      psrcb_b = OFFSET_PTR_NX16(psrcb_b, 1, inputPitch_deinterleave, -32);
      a = IVP_LANX16_PP(psrcb_b);
      IVP_LANX16_IP(b_b_nc, a, psrcb_b);
      psrcb_b = OFFSET_PTR_NX16(psrcb_b, 1, inputPitch_deinterleave, -33);

      xb_vecNx16 correction;
      correction = g_gr_c - IVP_AVGRNX16(g_r_c, g_r_pc);
      xb_vecNx16 r_gr = correction + IVP_AVGRNX16(r_r_pc, r_r_c);

      // Do the same for other reds and blues at green sites
      correction = g_gr_c - IVP_AVGRNX16(g_b_c, g_b_pr);
      xb_vecNx16 b_gr = correction + IVP_AVGRNX16(b_b_c, b_b_pr);

      correction = g_gb_c - IVP_AVGRNX16(g_r_c, g_r_nr);
      xb_vecNx16 r_gb = correction + IVP_AVGRNX16(r_r_c, r_r_nr);

      correction = g_gb_c - IVP_AVGRNX16(g_b_c, g_b_nc);
      xb_vecNx16 b_gb = correction + IVP_AVGRNX16(b_b_c, b_b_nc);

      IVP_SANX16_IP(r_gr, a_store, pdst_tmp_r_gr);
      IVP_SAPOSNX16_FP(a_store, pdst_tmp_r_gr);
      pdst_tmp_r_gr = OFFSET_PTR_NX16(pdst_tmp_r_gr, 1, outputPitch, -32);

      IVP_SANX16_IP(b_gr, a_store, pdst_tmp_b_gr);
      IVP_SAPOSNX16_FP(a_store, pdst_tmp_b_gr);
      pdst_tmp_b_gr = OFFSET_PTR_NX16(pdst_tmp_b_gr, 1, outputPitch, -32);

      IVP_SANX16_IP(r_gb, a_store, pdst_tmp_r_gb);
      IVP_SAPOSNX16_FP(a_store, pdst_tmp_r_gb);
      pdst_tmp_r_gb = OFFSET_PTR_NX16(pdst_tmp_r_gb, 1, outputPitch, -32);

      IVP_SANX16_IP(b_gb, a_store, pdst_tmp_b_gb);
      IVP_SAPOSNX16_FP(a_store, pdst_tmp_b_gb);
      pdst_tmp_b_gb = OFFSET_PTR_NX16(pdst_tmp_b_gb, 1, outputPitch, -32);

      correction = g_b_c - IVP_AVGRNX16(g_r_c, g_r_pc_nr);
      xb_vecNx16 rp_b = correction + IVP_AVGRNX16(r_r_c, r_r_pc_nr);
      xb_vecNx16U rpd_b = IVP_ABSSUBNX16(r_r_c, r_r_pc_nr);

      correction = g_b_c - IVP_AVGRNX16(g_r_pc, g_r_nr);
      xb_vecNx16 rn_b = correction + IVP_AVGRNX16(r_r_pc, r_r_nr);
      xb_vecNx16U rnd_b = IVP_ABSSUBNX16(r_r_pc, r_r_nr);

      xb_vecNx16 r_b = IVP_MOVNX16T(rp_b, rn_b, IVP_LTNX16(rpd_b, rnd_b));

      correction = g_r_c - IVP_AVGRNX16(g_b_c, g_b_nc_pr);
      xb_vecNx16 bp_r = correction + IVP_AVGRNX16(b_b_c, b_b_nc_pr);
      xb_vecNx16U bpd_r = IVP_ABSSUBNX16(b_b_c, b_b_nc_pr);

      correction = g_r_c - IVP_AVGRNX16(g_b_nc, g_b_pr);
      xb_vecNx16 bn_r = correction + IVP_AVGRNX16(b_b_nc, b_b_pr);
      xb_vecNx16U bnd_r = IVP_ABSSUBNX16(b_b_nc, b_b_pr);

      xb_vecNx16 b_r = IVP_MOVNX16T(bp_r, bn_r, IVP_LTNX16(bpd_r, bnd_r));

      IVP_SANX16_IP(b_r, a_store, pdst_tmp_b_r);
      IVP_SAPOSNX16_FP(a_store, pdst_tmp_b_r);
      pdst_tmp_b_r = OFFSET_PTR_NX16(pdst_tmp_b_r, 1, outputPitch, -32);

      IVP_SANX16_IP(r_b, a_store, pdst_tmp_r_b);
      IVP_SAPOSNX16_FP(a_store, pdst_tmp_r_b);
      pdst_tmp_r_b = OFFSET_PTR_NX16(pdst_tmp_r_b, 1, outputPitch, -32);
    }
  }
}

void Rearrange(short *restrict pr_r, short *restrict pb_b,
               short *restrict pg_gr, short *restrict pg_gb,
               int inputPitch_deinterleave, short *restrict tmp_g_r,
               short *restrict tmp_g_b, short *restrict tmp_r_gr,
               short *restrict tmp_b_gr, short *restrict tmp_r_gb,
               short *restrict tmp_b_gb, short *restrict tmp_r_b,
               short *restrict tmp_b_r, int inputWidth, int inputHeight) {
  int x, y;
  valign a_store = IVP_ZALIGN();

  for (x = 2; x < inputWidth - 2; x += 32) {
    xb_vecNx16 *psrcr_r = OFFSET_PTR_NX16(pr_r, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrcb_b = OFFSET_PTR_NX16(pb_b, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrcg_gb =
        OFFSET_PTR_NX16(pg_gb, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrcg_gr =
        OFFSET_PTR_NX16(pg_gr, 2, inputPitch_deinterleave, x);

    xb_vecNx16 *psrc_tmp_g_r =
        OFFSET_PTR_NX16(tmp_g_r, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_g_b =
        OFFSET_PTR_NX16(tmp_g_b, 2, inputPitch_deinterleave, x);

    xb_vecNx16 *psrc_tmp_r_gr =
        OFFSET_PTR_NX16(tmp_r_gr, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_b_gr =
        OFFSET_PTR_NX16(tmp_b_gr, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_r_gb =
        OFFSET_PTR_NX16(tmp_r_gb, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_b_gb =
        OFFSET_PTR_NX16(tmp_b_gb, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_r_b =
        OFFSET_PTR_NX16(tmp_r_b, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_b_r =
        OFFSET_PTR_NX16(tmp_b_r, 2, inputPitch_deinterleave, x);

    for (y = 2; y < inputHeight - 2; y++) {
      xb_vecNx16 r_gr, r_r, r_b, r_gb, g_gr, g_r, g_b, g_gb, b_gr, b_r, b_b,
          b_gb;

      valign a = IVP_LANX16_PP(psrc_tmp_r_gr);
      IVP_LANX16_IP(r_gr, a, psrc_tmp_r_gr);
      psrc_tmp_r_gr = OFFSET_PTR_NX16(psrc_tmp_r_gr, 0, 0, -32);

      a = IVP_LANX16_PP(psrcr_r);
      IVP_LANX16_IP(r_r, a, psrcr_r);
      psrcr_r = OFFSET_PTR_NX16(psrcr_r, 0, 0, -32);

      a = IVP_LANX16_PP(psrcg_gr);
      IVP_LANX16_IP(g_gr, a, psrcg_gr);
      psrcg_gr = OFFSET_PTR_NX16(psrcg_gr, 0, 0, -32);

      a = IVP_LANX16_PP(psrc_tmp_g_r);
      IVP_LANX16_IP(g_r, a, psrc_tmp_g_r);
      psrc_tmp_g_r = OFFSET_PTR_NX16(psrc_tmp_g_r, 0, 0, -32);

      a = IVP_LANX16_PP(psrc_tmp_b_gr);
      IVP_LANX16_IP(b_gr, a, psrc_tmp_b_gr);
      psrc_tmp_b_gr = OFFSET_PTR_NX16(psrc_tmp_b_gr, 0, 0, -32);

      a = IVP_LANX16_PP(psrc_tmp_b_r);
      IVP_LANX16_IP(b_r, a, psrc_tmp_b_r);
      psrc_tmp_b_r = OFFSET_PTR_NX16(psrc_tmp_b_r, 0, 0, -32);

      xb_vecNx16 r00, r01;
      xb_vecNx16 g00, g01;
      xb_vecNx16 b00, b01;
      IVP_DSELNX16I(r01, r00, r_r, r_gr, IVP_DSELI_INTERLEAVE_1);
      IVP_DSELNX16I(g01, g00, g_r, g_gr, IVP_DSELI_INTERLEAVE_1);
      IVP_DSELNX16I(b01, b00, b_r, b_gr, IVP_DSELI_INTERLEAVE_1);

      IVP_SANX16_IP(r00, a_store, psrc_tmp_r_gr);
      IVP_SAPOSNX16_FP(a_store, psrc_tmp_r_gr);
      psrc_tmp_r_gr =
          OFFSET_PTR_NX16(psrc_tmp_r_gr, 1, inputPitch_deinterleave, -32);

      IVP_SANX16_IP(r01, a_store, psrcr_r);
      IVP_SAPOSNX16_FP(a_store, psrcr_r);
      psrcr_r = OFFSET_PTR_NX16(psrcr_r, 1, inputPitch_deinterleave, -32);

      IVP_SANX16_IP(g00, a_store, psrcg_gr);
      IVP_SAPOSNX16_FP(a_store, psrcg_gr);
      psrcg_gr = OFFSET_PTR_NX16(psrcg_gr, 1, inputPitch_deinterleave, -32);

      IVP_SANX16_IP(g01, a_store, psrc_tmp_g_r);
      IVP_SAPOSNX16_FP(a_store, psrc_tmp_g_r);
      psrc_tmp_g_r =
          OFFSET_PTR_NX16(psrc_tmp_g_r, 1, inputPitch_deinterleave, -32);

      IVP_SANX16_IP(b00, a_store, psrc_tmp_b_gr);
      IVP_SAPOSNX16_FP(a_store, psrc_tmp_b_gr);
      psrc_tmp_b_gr =
          OFFSET_PTR_NX16(psrc_tmp_b_gr, 1, inputPitch_deinterleave, -32);

      IVP_SANX16_IP(b01, a_store, psrc_tmp_b_r);
      IVP_SAPOSNX16_FP(a_store, psrc_tmp_b_r);
      psrc_tmp_b_r =
          OFFSET_PTR_NX16(psrc_tmp_b_r, 1, inputPitch_deinterleave, -32);
    }
  }

  for (x = 2; x < inputWidth - 2; x += 32) {
    xb_vecNx16 *psrcr_r = OFFSET_PTR_NX16(pr_r, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrcb_b = OFFSET_PTR_NX16(pb_b, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrcg_gb =
        OFFSET_PTR_NX16(pg_gb, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrcg_gr =
        OFFSET_PTR_NX16(pg_gr, 2, inputPitch_deinterleave, x);

    xb_vecNx16 *psrc_tmp_g_r =
        OFFSET_PTR_NX16(tmp_g_r, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_g_b =
        OFFSET_PTR_NX16(tmp_g_b, 2, inputPitch_deinterleave, x);

    xb_vecNx16 *psrc_tmp_r_gr =
        OFFSET_PTR_NX16(tmp_r_gr, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_b_gr =
        OFFSET_PTR_NX16(tmp_b_gr, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_r_gb =
        OFFSET_PTR_NX16(tmp_r_gb, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_b_gb =
        OFFSET_PTR_NX16(tmp_b_gb, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_r_b =
        OFFSET_PTR_NX16(tmp_r_b, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_b_r =
        OFFSET_PTR_NX16(tmp_b_r, 2, inputPitch_deinterleave, x);

    for (y = 2; y < inputHeight - 2; y++) {
      xb_vecNx16 r_gr, r_r, r_b, r_gb, g_gr, g_r, g_b, g_gb, b_gr, b_r, b_b,
          b_gb;

      valign a;
      a = IVP_LANX16_PP(psrc_tmp_r_b);
      IVP_LANX16_IP(r_b, a, psrc_tmp_r_b);
      psrc_tmp_r_b = OFFSET_PTR_NX16(psrc_tmp_r_b, 0, 0, -32);

      a = IVP_LANX16_PP(psrc_tmp_r_gb);
      IVP_LANX16_IP(r_gb, a, psrc_tmp_r_gb);
      psrc_tmp_r_gb = OFFSET_PTR_NX16(psrc_tmp_r_gb, 0, 0, -32);

      a = IVP_LANX16_PP(psrc_tmp_g_b);
      IVP_LANX16_IP(g_b, a, psrc_tmp_g_b);
      psrc_tmp_g_b = OFFSET_PTR_NX16(psrc_tmp_g_b, 0, 0, -32);

      a = IVP_LANX16_PP(psrcg_gb);
      IVP_LANX16_IP(g_gb, a, psrcg_gb);
      psrcg_gb = OFFSET_PTR_NX16(psrcg_gb, 0, 0, -32);

      a = IVP_LANX16_PP(psrcb_b);
      IVP_LANX16_IP(b_b, a, psrcb_b);
      psrcb_b = OFFSET_PTR_NX16(psrcb_b, 0, 0, -32);

      a = IVP_LANX16_PP(psrc_tmp_b_gb);
      IVP_LANX16_IP(b_gb, a, psrc_tmp_b_gb);
      psrc_tmp_b_gb = OFFSET_PTR_NX16(psrc_tmp_b_gb, 0, 0, -32);

      xb_vecNx16 r10, r11;
      xb_vecNx16 g10, g11;
      xb_vecNx16 b10, b11;
      IVP_DSELNX16I(r11, r10, r_gb, r_b, IVP_DSELI_INTERLEAVE_1);
      IVP_DSELNX16I(g11, g10, g_gb, g_b, IVP_DSELI_INTERLEAVE_1);
      IVP_DSELNX16I(b11, b10, b_gb, b_b, IVP_DSELI_INTERLEAVE_1);

      IVP_SANX16_IP(r10, a_store, psrc_tmp_r_b);
      IVP_SAPOSNX16_FP(a_store, psrc_tmp_r_b);
      psrc_tmp_r_b =
          OFFSET_PTR_NX16(psrc_tmp_r_b, 1, inputPitch_deinterleave, -32);

      IVP_SANX16_IP(r11, a_store, psrc_tmp_r_gb);
      IVP_SAPOSNX16_FP(a_store, psrc_tmp_r_gb);
      psrc_tmp_r_gb =
          OFFSET_PTR_NX16(psrc_tmp_r_gb, 1, inputPitch_deinterleave, -32);

      IVP_SANX16_IP(g10, a_store, psrc_tmp_g_b);
      IVP_SAPOSNX16_FP(a_store, psrc_tmp_g_b);
      psrc_tmp_g_b =
          OFFSET_PTR_NX16(psrc_tmp_g_b, 1, inputPitch_deinterleave, -32);

      IVP_SANX16_IP(g11, a_store, psrcg_gb);
      IVP_SAPOSNX16_FP(a_store, psrcg_gb);
      psrcg_gb = OFFSET_PTR_NX16(psrcg_gb, 1, inputPitch_deinterleave, -32);

      IVP_SANX16_IP(b10, a_store, psrcb_b);
      IVP_SAPOSNX16_FP(a_store, psrcb_b);
      psrcb_b = OFFSET_PTR_NX16(psrcb_b, 1, inputPitch_deinterleave, -32);

      IVP_SANX16_IP(b11, a_store, psrc_tmp_b_gb);
      IVP_SAPOSNX16_FP(a_store, psrc_tmp_b_gb);
      psrc_tmp_b_gb =
          OFFSET_PTR_NX16(psrc_tmp_b_gb, 1, inputPitch_deinterleave, -32);
    }
  }
}

void apply_curve(short *restrict pr_r, short *restrict pb_b,
                 short *restrict pg_gr, short *restrict pg_gb,
                 int inputPitch_deinterleave, short *restrict tmp_g_r,
                 short *restrict tmp_g_b, short *restrict tmp_r_gr,
                 short *restrict tmp_b_gr, short *restrict tmp_r_gb,
                 short *restrict tmp_b_gb, short *restrict tmp_r_b,
                 short *restrict tmp_b_r, int inputWidth, int inputHeight,
                 unsigned char *restrict output, int outputPitch,
                 unsigned char *restrict local_src_curve,
                 short *restrict local_src_matrix) {
  int x, y;
  valign a_store = IVP_ZALIGN();
  xb_vec2Nx8 vindex0 = IVP_SEQ2NX8();
  int *matrix = (int *)local_src_matrix;
  IVP_ADD2NX8T(vindex0, vindex0, -XCHAL_IVPN_SIMD_WIDTH,
               IVP_LTU2NX8(XCHAL_IVPN_SIMD_WIDTH - 1, vindex0));
  xb_vec2Nx8 vindex1 = IVP_ADD2NX8(vindex0, XCHAL_IVPN_SIMD_WIDTH);

  xb_vec2Nx8 tmp0, tmp1, tmp2, tmp3, step0, step1;
  IVP_DSEL2NX8I(tmp1, tmp0, vindex1, vindex0,
                IVP_DSELI_8B_INTERLEAVE_C3_STEP_0);
  IVP_DSEL2NX8I_H(tmp3, tmp2, vindex1, vindex0,
                  IVP_DSELI_8B_INTERLEAVE_C3_STEP_1);
  step0 = IVP_SEL2NX8I(tmp2, tmp0, IVP_SELI_8B_INTERLEAVE_1_LO);
  step1 = IVP_SEL2NX8I(tmp2, tmp1, IVP_SELI_8B_INTERLEAVE_1_LO);
  vbool2N pred1 = IVP_LTR2N(XCHAL_IVPN_SIMD_WIDTH);
  vbool2N pred2 = IVP_EXTBI2NX8(vindex0, 0);
  vbool2N pred = IVP_ANDB2N(pred1, pred2);
  pred = IVP_NOTB2N(pred);

  for (x = 2; x < inputWidth - 2; x += 32) {
    xb_vecNx16 *psrcr_r = OFFSET_PTR_NX16(pr_r, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrcb_b = OFFSET_PTR_NX16(pb_b, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrcg_gb =
        OFFSET_PTR_NX16(pg_gb, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrcg_gr =
        OFFSET_PTR_NX16(pg_gr, 2, inputPitch_deinterleave, x);

    xb_vecNx16 *psrc_tmp_g_r =
        OFFSET_PTR_NX16(tmp_g_r, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_g_b =
        OFFSET_PTR_NX16(tmp_g_b, 2, inputPitch_deinterleave, x);

    xb_vecNx16 *psrc_tmp_r_gr =
        OFFSET_PTR_NX16(tmp_r_gr, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_b_gr =
        OFFSET_PTR_NX16(tmp_b_gr, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_r_gb =
        OFFSET_PTR_NX16(tmp_r_gb, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_b_gb =
        OFFSET_PTR_NX16(tmp_b_gb, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_r_b =
        OFFSET_PTR_NX16(tmp_r_b, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_b_r =
        OFFSET_PTR_NX16(tmp_b_r, 2, inputPitch_deinterleave, x);

    xb_vecNx8U *pdstrgb = OFFSET_PTR_NX8U(output, 0, 0, 2 * (x - 2) * 3);

    for (y = 2; y < inputHeight - 2; y += 1) {
      xb_vecNx16 R_00, R_01, R_10, R_11;
      xb_vecNx16 G_00, G_01, G_10, G_11;
      xb_vecNx16 B_00, B_01, B_10, B_11;
      valign a = IVP_LANX16_PP(psrc_tmp_r_gr);
      IVP_LANX16_IP(R_00, a, psrc_tmp_r_gr);
      psrc_tmp_r_gr =
          OFFSET_PTR_NX16(psrc_tmp_r_gr, 1, inputPitch_deinterleave, -32);

      a = IVP_LANX16_PP(psrcr_r);
      IVP_LANX16_IP(R_01, a, psrcr_r);
      psrcr_r = OFFSET_PTR_NX16(psrcr_r, 1, inputPitch_deinterleave, -32);

      a = IVP_LANX16_PP(psrcg_gr);
      IVP_LANX16_IP(G_00, a, psrcg_gr);
      psrcg_gr = OFFSET_PTR_NX16(psrcg_gr, 1, inputPitch_deinterleave, -32);

      a = IVP_LANX16_PP(psrc_tmp_g_r);
      IVP_LANX16_IP(G_01, a, psrc_tmp_g_r);
      psrc_tmp_g_r =
          OFFSET_PTR_NX16(psrc_tmp_g_r, 1, inputPitch_deinterleave, -32);

      a = IVP_LANX16_PP(psrc_tmp_b_gr);
      IVP_LANX16_IP(B_00, a, psrc_tmp_b_gr);
      psrc_tmp_b_gr =
          OFFSET_PTR_NX16(psrc_tmp_b_gr, 1, inputPitch_deinterleave, -32);

      a = IVP_LANX16_PP(psrc_tmp_b_r);
      IVP_LANX16_IP(B_01, a, psrc_tmp_b_r);
      psrc_tmp_b_r =
          OFFSET_PTR_NX16(psrc_tmp_b_r, 1, inputPitch_deinterleave, -32);

      xb_vecNx48 R_00_ = IVP_MULPN16XR16(G_00, R_00, matrix[0]);
      IVP_MULPAN16XR16(R_00_, (xb_vecNx16)1, B_00, matrix[1]);

      xb_vecNx48 R_01_ = IVP_MULPN16XR16(G_01, R_01, matrix[0]);
      IVP_MULPAN16XR16(R_01_, (xb_vecNx16)1, B_01, matrix[1]);

      xb_vecNx48 G_00_ = IVP_MULPN16XR16(G_00, R_00, matrix[0 + 2]);
      IVP_MULPAN16XR16(G_00_, (xb_vecNx16)1, B_00, matrix[1 + 2]);

      xb_vecNx48 G_01_ = IVP_MULPN16XR16(G_01, R_01, matrix[0 + 2]);
      IVP_MULPAN16XR16(G_01_, (xb_vecNx16)1, B_01, matrix[1 + 2]);

      xb_vecNx48 B_00_ = IVP_MULPN16XR16(G_00, R_00, matrix[0 + 4]);
      IVP_MULPAN16XR16(B_00_, (xb_vecNx16)1, B_00, matrix[1 + 4]);

      xb_vecNx48 B_01_ = IVP_MULPN16XR16(G_01, R_01, matrix[0 + 4]);
      IVP_MULPAN16XR16(B_01_, (xb_vecNx16)1, B_01, matrix[1 + 4]);

      R_00 =
          _CLAMP(IVP_PACKVRNRNX48(R_00_, 8), (xb_vecNx16)0, (xb_vecNx16)1023);
      R_01 =
          _CLAMP(IVP_PACKVRNRNX48(R_01_, 8), (xb_vecNx16)0, (xb_vecNx16)1023);
      G_00 =
          _CLAMP(IVP_PACKVRNRNX48(G_00_, 8), (xb_vecNx16)0, (xb_vecNx16)1023);
      G_01 =
          _CLAMP(IVP_PACKVRNRNX48(G_01_, 8), (xb_vecNx16)0, (xb_vecNx16)1023);
      B_00 =
          _CLAMP(IVP_PACKVRNRNX48(B_00_, 8), (xb_vecNx16)0, (xb_vecNx16)1023);
      B_01 =
          _CLAMP(IVP_PACKVRNRNX48(B_01_, 8), (xb_vecNx16)0, (xb_vecNx16)1023);

      xb_vecNx16U R00 = IVP_GATHERDNX8U(IVP_GATHERANX8U(local_src_curve, R_00));
      xb_vecNx16U R01 = IVP_GATHERDNX8U(IVP_GATHERANX8U(local_src_curve, R_01));
      xb_vecNx16U G00 = IVP_GATHERDNX8U(IVP_GATHERANX8U(local_src_curve, G_00));
      xb_vecNx16U G01 = IVP_GATHERDNX8U(IVP_GATHERANX8U(local_src_curve, G_01));
      xb_vecNx16U B00 = IVP_GATHERDNX8U(IVP_GATHERANX8U(local_src_curve, B_00));
      xb_vecNx16U B01 = IVP_GATHERDNX8U(IVP_GATHERANX8U(local_src_curve, B_01));

      xb_vecNx16 tmp0, tmp1;
      xb_vecNx16 xOut0a, xOut0b, xOut0c;
      IVP_DSELNX16I(tmp1, tmp0, G00, R00, IVP_DSELI_INTERLEAVE_1);
      IVP_DSELNX16(xOut0b, xOut0a, B00, tmp0, step0);
      IVP_DSELNX16T(xOut0b, xOut0c, B00, tmp1, step1, pred);

      xb_vecNx16 xOut1a, xOut1b, xOut1c;
      IVP_DSELNX16I(tmp1, tmp0, G01, R01, IVP_DSELI_INTERLEAVE_1);
      IVP_DSELNX16(xOut1b, xOut1a, B01, tmp0, step0);
      IVP_DSELNX16T(xOut1b, xOut1c, B01, tmp1, step1, pred);

      IVP_SANX8U_IP(IVP_MOVNX8U_FROMNX16(xOut0a), a_store, pdstrgb);
      IVP_SANX8U_IP(IVP_MOVNX8U_FROMNX16(xOut0b), a_store, pdstrgb);
      IVP_SANX8U_IP(IVP_MOVNX8U_FROMNX16(xOut0c), a_store, pdstrgb);
      IVP_SANX8U_IP(IVP_MOVNX8U_FROMNX16(xOut1a), a_store, pdstrgb);
      IVP_SANX8U_IP(IVP_MOVNX8U_FROMNX16(xOut1b), a_store, pdstrgb);
      IVP_SANX8U_IP(IVP_MOVNX8U_FROMNX16(xOut1c), a_store, pdstrgb);
      IVP_SAPOSNX8U_FP(a_store, pdstrgb);
      pdstrgb = OFFSET_PTR_NX8U(pdstrgb, 2, outputPitch, -192);
    }
  }

  for (x = 2; x < inputWidth - 2; x += 32) {
    xb_vecNx16 *psrcr_r = OFFSET_PTR_NX16(pr_r, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrcb_b = OFFSET_PTR_NX16(pb_b, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrcg_gb =
        OFFSET_PTR_NX16(pg_gb, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrcg_gr =
        OFFSET_PTR_NX16(pg_gr, 2, inputPitch_deinterleave, x);

    xb_vecNx16 *psrc_tmp_g_r =
        OFFSET_PTR_NX16(tmp_g_r, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_g_b =
        OFFSET_PTR_NX16(tmp_g_b, 2, inputPitch_deinterleave, x);

    xb_vecNx16 *psrc_tmp_r_gr =
        OFFSET_PTR_NX16(tmp_r_gr, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_b_gr =
        OFFSET_PTR_NX16(tmp_b_gr, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_r_gb =
        OFFSET_PTR_NX16(tmp_r_gb, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_b_gb =
        OFFSET_PTR_NX16(tmp_b_gb, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_r_b =
        OFFSET_PTR_NX16(tmp_r_b, 2, inputPitch_deinterleave, x);
    xb_vecNx16 *psrc_tmp_b_r =
        OFFSET_PTR_NX16(tmp_b_r, 2, inputPitch_deinterleave, x);

    xb_vecNx8U *pdstrgb =
        OFFSET_PTR_NX8U(output, 1, outputPitch, 2 * (x - 2) * 3);

    for (y = 2; y < inputHeight - 2; y += 1) {
      xb_vecNx16 R_00, R_01, R_10, R_11;
      xb_vecNx16 G_00, G_01, G_10, G_11;
      xb_vecNx16 B_00, B_01, B_10, B_11;
      valign a;
      a = IVP_LANX16_PP(psrc_tmp_r_b);
      IVP_LANX16_IP(R_10, a, psrc_tmp_r_b);
      psrc_tmp_r_b =
          OFFSET_PTR_NX16(psrc_tmp_r_b, 1, inputPitch_deinterleave, -32);

      a = IVP_LANX16_PP(psrc_tmp_r_gb);
      IVP_LANX16_IP(R_11, a, psrc_tmp_r_gb);
      psrc_tmp_r_gb =
          OFFSET_PTR_NX16(psrc_tmp_r_gb, 1, inputPitch_deinterleave, -32);

      a = IVP_LANX16_PP(psrc_tmp_g_b);
      IVP_LANX16_IP(G_10, a, psrc_tmp_g_b);
      psrc_tmp_g_b =
          OFFSET_PTR_NX16(psrc_tmp_g_b, 1, inputPitch_deinterleave, -32);

      a = IVP_LANX16_PP(psrcg_gb);
      IVP_LANX16_IP(G_11, a, psrcg_gb);
      psrcg_gb = OFFSET_PTR_NX16(psrcg_gb, 1, inputPitch_deinterleave, -32);

      a = IVP_LANX16_PP(psrcb_b);
      IVP_LANX16_IP(B_10, a, psrcb_b);
      psrcb_b = OFFSET_PTR_NX16(psrcb_b, 1, inputPitch_deinterleave, -32);

      a = IVP_LANX16_PP(psrc_tmp_b_gb);
      IVP_LANX16_IP(B_11, a, psrc_tmp_b_gb);
      psrc_tmp_b_gb =
          OFFSET_PTR_NX16(psrc_tmp_b_gb, 1, inputPitch_deinterleave, -32);

      xb_vecNx48 R_10_ = IVP_MULPN16XR16(G_10, R_10, matrix[0]);
      IVP_MULPAN16XR16(R_10_, (xb_vecNx16)1, B_10, matrix[1]);

      xb_vecNx48 R_11_ = IVP_MULPN16XR16(G_11, R_11, matrix[0]);
      IVP_MULPAN16XR16(R_11_, (xb_vecNx16)1, B_11, matrix[1]);

      xb_vecNx48 G_10_ = IVP_MULPN16XR16(G_10, R_10, matrix[0 + 2]);
      IVP_MULPAN16XR16(G_10_, (xb_vecNx16)1, B_10, matrix[1 + 2]);

      xb_vecNx48 G_11_ = IVP_MULPN16XR16(G_11, R_11, matrix[0 + 2]);
      IVP_MULPAN16XR16(G_11_, (xb_vecNx16)1, B_11, matrix[1 + 2]);

      xb_vecNx48 B_10_ = IVP_MULPN16XR16(G_10, R_10, matrix[0 + 4]);
      IVP_MULPAN16XR16(B_10_, (xb_vecNx16)1, B_10, matrix[1 + 4]);

      xb_vecNx48 B_11_ = IVP_MULPN16XR16(G_11, R_11, matrix[0 + 4]);
      IVP_MULPAN16XR16(B_11_, (xb_vecNx16)1, B_11, matrix[1 + 4]);

      R_10 =
          _CLAMP(IVP_PACKVRNRNX48(R_10_, 8), (xb_vecNx16)0, (xb_vecNx16)1023);
      R_11 =
          _CLAMP(IVP_PACKVRNRNX48(R_11_, 8), (xb_vecNx16)0, (xb_vecNx16)1023);
      G_10 =
          _CLAMP(IVP_PACKVRNRNX48(G_10_, 8), (xb_vecNx16)0, (xb_vecNx16)1023);
      G_11 =
          _CLAMP(IVP_PACKVRNRNX48(G_11_, 8), (xb_vecNx16)0, (xb_vecNx16)1023);
      B_10 =
          _CLAMP(IVP_PACKVRNRNX48(B_10_, 8), (xb_vecNx16)0, (xb_vecNx16)1023);
      B_11 =
          _CLAMP(IVP_PACKVRNRNX48(B_11_, 8), (xb_vecNx16)0, (xb_vecNx16)1023);

      xb_vecNx16U R10 = IVP_GATHERDNX8U(IVP_GATHERANX8U(local_src_curve, R_10));
      xb_vecNx16U R11 = IVP_GATHERDNX8U(IVP_GATHERANX8U(local_src_curve, R_11));
      xb_vecNx16U G10 = IVP_GATHERDNX8U(IVP_GATHERANX8U(local_src_curve, G_10));
      xb_vecNx16U G11 = IVP_GATHERDNX8U(IVP_GATHERANX8U(local_src_curve, G_11));
      xb_vecNx16U B10 = IVP_GATHERDNX8U(IVP_GATHERANX8U(local_src_curve, B_10));
      xb_vecNx16U B11 = IVP_GATHERDNX8U(IVP_GATHERANX8U(local_src_curve, B_11));

      xb_vecNx16 xOut2a, xOut2b, xOut2c, tmp1, tmp0;
      IVP_DSELNX16I(tmp1, tmp0, G10, R10, IVP_DSELI_INTERLEAVE_1);
      IVP_DSELNX16(xOut2b, xOut2a, B10, tmp0, step0);
      IVP_DSELNX16T(xOut2b, xOut2c, B10, tmp1, step1, pred);

      xb_vecNx16 xOut3a, xOut3b, xOut3c;
      IVP_DSELNX16I(tmp1, tmp0, G11, R11, IVP_DSELI_INTERLEAVE_1);
      IVP_DSELNX16(xOut3b, xOut3a, B11, tmp0, step0);
      IVP_DSELNX16T(xOut3b, xOut3c, B11, tmp1, step1, pred);

      IVP_SANX8U_IP(IVP_MOVNX8U_FROMNX16(xOut2a), a_store, pdstrgb);
      IVP_SANX8U_IP(IVP_MOVNX8U_FROMNX16(xOut2b), a_store, pdstrgb);
      IVP_SANX8U_IP(IVP_MOVNX8U_FROMNX16(xOut2c), a_store, pdstrgb);
      IVP_SANX8U_IP(IVP_MOVNX8U_FROMNX16(xOut3a), a_store, pdstrgb);
      IVP_SANX8U_IP(IVP_MOVNX8U_FROMNX16(xOut3b), a_store, pdstrgb);
      IVP_SANX8U_IP(IVP_MOVNX8U_FROMNX16(xOut3c), a_store, pdstrgb);
      IVP_SAPOSNX8U_FP(a_store, pdstrgb);
      pdstrgb = OFFSET_PTR_NX8U(pdstrgb, 2, outputPitch, -192);
    }
  }
}

void sharpen(unsigned char *restrict input, int inputPitch, int inputWidth,
             int inputHeight, unsigned char *restrict output, int outputPitch,
             unsigned char strength) {

  int x, y;
  valign a_store = IVP_ZALIGN();
  for (x = 2 * 3; x < inputWidth - 2 * 3; x += 64) {
    xb_vec2Nx8U *pdstrgb = OFFSET_PTR_2NX8U(output, 0, 0, x - 2 * 3);
    xb_vec2Nx8U *psrcrgb = OFFSET_PTR_2NX8U(input, 1, inputPitch, x - 3);
    xb_vec2Nx8U x00, x01, x02;
    valign a = IVP_LA2NX8U_PP(psrcrgb);
    IVP_LA2NX8U_IP(x00, a, psrcrgb);
    psrcrgb = OFFSET_PTR_2NX8U(psrcrgb, 0, 0, -61);
    a = IVP_LA2NX8U_PP(psrcrgb);
    IVP_LA2NX8U_IP(x01, a, psrcrgb);
    psrcrgb = OFFSET_PTR_2NX8U(psrcrgb, 0, 0, -61);
    a = IVP_LA2NX8U_PP(psrcrgb);
    IVP_LA2NX8U_IP(x02, a, psrcrgb);

    psrcrgb = OFFSET_PTR_2NX8U(psrcrgb, 1, inputPitch, -64 - 6);

    xb_vec2Nx8U x10, x11, x12;
    a = IVP_LA2NX8U_PP(psrcrgb);
    IVP_LA2NX8U_IP(x10, a, psrcrgb);
    psrcrgb = OFFSET_PTR_2NX8U(psrcrgb, 0, 0, -61);
    a = IVP_LA2NX8U_PP(psrcrgb);
    IVP_LA2NX8U_IP(x11, a, psrcrgb);
    psrcrgb = OFFSET_PTR_2NX8U(psrcrgb, 0, 0, -61);
    a = IVP_LA2NX8U_PP(psrcrgb);
    IVP_LA2NX8U_IP(x12, a, psrcrgb);

    psrcrgb = OFFSET_PTR_2NX8U(psrcrgb, 1, inputPitch, -64 - 6);

    xb_vec2Nx8U xRow0 = IVP_AVGRU2NX8U(IVP_AVGRU2NX8U(x00, x02), x01);
    xb_vec2Nx8U xRow1 = IVP_AVGRU2NX8U(IVP_AVGRU2NX8U(x10, x12), x11);

    for (y = 2; y < inputHeight - 2; y += 1) {
      xb_vec2Nx8U x20, x21, x22;
      a = IVP_LA2NX8U_PP(psrcrgb);
      IVP_LA2NX8U_IP(x20, a, psrcrgb);
      psrcrgb = OFFSET_PTR_2NX8U(psrcrgb, 0, 0, -61);
      a = IVP_LA2NX8U_PP(psrcrgb);
      IVP_LA2NX8U_IP(x21, a, psrcrgb);
      psrcrgb = OFFSET_PTR_2NX8U(psrcrgb, 0, 0, -61);
      a = IVP_LA2NX8U_PP(psrcrgb);
      IVP_LA2NX8U_IP(x22, a, psrcrgb);

      psrcrgb = OFFSET_PTR_2NX8U(psrcrgb, 1, inputPitch, -64 - 6);

      xb_vec2Nx8U xRow2 = IVP_AVGRU2NX8U(IVP_AVGRU2NX8U(x20, x22), x21);

      xb_vec2Nx8U xunsharp =
          IVP_AVGRU2NX8U(IVP_AVGRU2NX8U(xRow0, xRow2), xRow1);

      xRow0 = xRow1;
      xRow1 = xRow2;

      xb_vec2Nx24 Accmask = IVP_SUBWU2NX8(x11, xunsharp);

      xb_vecNx16 MaskA = IVP_PACKL2NX24_0(Accmask);
      xb_vecNx16 MaskB = IVP_PACKL2NX24_1(Accmask);

      xb_vec2Nx24 xtempAcc = IVP_MULUSI2NR8X16(strength, MaskB, MaskA);

      IVP_MULUUA2NX8(xtempAcc, x11, (xb_vec2Nx8U)32);

      xb_vec2Nx8U xout = IVP_PACKVRU2NX24(xtempAcc, 5);

      IVP_SA2NX8U_IP(xout, a_store, pdstrgb);
      IVP_SAPOS2NX8U_FP(a_store, pdstrgb);
      pdstrgb = OFFSET_PTR_2NX8U(pdstrgb, 1, outputPitch, -64);

      x11 = x21;
    }
  }
}