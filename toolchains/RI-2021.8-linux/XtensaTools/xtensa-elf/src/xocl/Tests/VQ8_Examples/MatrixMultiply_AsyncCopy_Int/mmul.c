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

#define IVP_SAVN_2X32POS_FP IVP_SAPOSN_2X32_FP

#define PARTSIZE (128)
#define PARTBITS (7)
XI_ERR_TYPE xiMatrixMultiply_U8U32Sa(const xi_pArray src0, const xi_pArray src1,
                                     xi_pArray dst) {
  // XI_ERROR_CHECKS()
  //{
  //	XI_CHECK_ARRAY_S8(src0);
  //	XI_CHECK_ARRAY_S8(src1);
  //	XI_CHECK_ARRAY_S32(dst);
  //
  //	XI_CHECK_ERROR(XI_ARRAY_GET_WIDTH(src0) == XI_ARRAY_GET_HEIGHT(src1),
  //			XI_ERR_DATASIZE,   "Matrix dimensions of arguments are
  //inconsistent"); 	XI_CHECK_ERROR(XI_ARRAY_GET_HEIGHT(src0) ==
  //XI_ARRAY_GET_HEIGHT(dst), 			XI_ERR_DATASIZE,   "Matrix dimensions of arguments
  //are inconsistent"); 	XI_CHECK_ERROR(XI_ARRAY_GET_WIDTH(src1) ==
  //XI_ARRAY_GET_WIDTH(dst), 			XI_ERR_DATASIZE,   "Matrix dimensions of arguments
  //are inconsistent");
  //}

  const uint8_t *src0_ptr = (uint8_t *)XI_TILE_GET_DATA_PTR(src0);
  const uint8_t *src1_ptr = (uint8_t *)XI_TILE_GET_DATA_PTR(src1);

  uint32_t *dst_ptr = (uint32_t *)XI_TILE_GET_DATA_PTR(dst);

  const int stride_src0 = XI_TILE_GET_PITCH(src0);
  const int stride_src1 = XI_TILE_GET_PITCH(src1);
  const int stride_src1_4 = stride_src1 * 4;
  const int stride_dst = XI_TILE_GET_PITCH(dst);

  const int w_src0 = XI_TILE_GET_WIDTH(src0);

  const int w_dst = XI_TILE_GET_WIDTH(dst);
  const int h_dst = XI_TILE_GET_HEIGHT(dst);

  // printf("w, h = %d %d %d %d %d %d\n", XI_TILE_GET_WIDTH(src0),
  // XI_TILE_GET_HEIGHT(src0), XI_TILE_GET_WIDTH(src1), XI_TILE_GET_HEIGHT(src1),
  // XI_TILE_GET_WIDTH(dst), XI_TILE_GET_HEIGHT(dst)); printf("Pitch0 = %d\t
  // Pitch1 = %d\t Pitchdst = %d\n", stride_src0, stride_src1, stride_dst);
  // return 0;
  xb_vec2Nx8U *restrict rsrc_a;
  xb_vec2Nx8U *restrict rsrc_b;
  xb_vec2Nx8U *restrict rsrc_c;
  xb_vec2Nx8U *restrict rsrc_d;
#if XCHAL_HAVE_NX
  xb_vecN_2x32Uv *restrict pvsrc0;
  xb_vecN_2x32Uv *restrict pvsrc1;
  xb_vecN_2x32Uv *restrict pvsrc2;
  xb_vecN_2x32Uv *restrict pvsrc3;
#else
  xb_vecNx8U *restrict pvsrc0;
  xb_vecNx8U *restrict pvsrc1;
  xb_vecNx8U *restrict pvsrc2;
  xb_vecNx8U *restrict pvsrc3;
#endif
  xb_vecN_2x32Uv *restrict rdst;

  int nparts = (w_src0 + (PARTSIZE - 1)) >> PARTBITS;
  int partsize = XT_MIN(PARTSIZE, w_src0);

  valign a_store = IVP_ZALIGN();
  int x = 0;
  for (; x < w_dst; x += (XCHAL_IVPN_SIMD_WIDTH * 2)) {
    int y = 0;
    for (; y < h_dst - 3; y += 4) {
      xb_vecN_2x32Uv vec_out_a0 = 0, vec_out_a1 = 0, vec_out_a2 = 0,
                     vec_out_a3 = 0;
      xb_vecN_2x32Uv vec_out_b0 = 0, vec_out_b1 = 0, vec_out_b2 = 0,
                     vec_out_b3 = 0;
      xb_vecN_2x32Uv vec_out_c0 = 0, vec_out_c1 = 0, vec_out_c2 = 0,
                     vec_out_c3 = 0;
      xb_vecN_2x32Uv vec_out_d0 = 0, vec_out_d1 = 0, vec_out_d2 = 0,
                     vec_out_d3 = 0;

      xb_vec2Nx24 Accw0 = 0, Accw1 = 0, Accw2 = 0, Accw3 = 0;
#if XCHAL_HAVE_NX

      pvsrc0 =
          (xb_vecN_2x32Uv *)&src0_ptr[y * stride_src0]; // OFFSET_PTR_N_2X32(src0_ptr,
                                                        // y, stride_src0, 0);
      pvsrc1 =
          (xb_vecN_2x32Uv
               *)&src0_ptr[(y + 1) * stride_src0]; // OFFSET_PTR_N_2X32(src0_ptr,
                                                   // y + 1, stride_src0, 0);
      pvsrc2 =
          (xb_vecN_2x32Uv
               *)&src0_ptr[(y + 2) * stride_src0]; // OFFSET_PTR_N_2X32(src0_ptr,
                                                   // y + 2, stride_src0, 0);
      pvsrc3 =
          (xb_vecN_2x32Uv
               *)&src0_ptr[(y + 3) * stride_src0]; // OFFSET_PTR_N_2X32(src0_ptr,
                                                   // y + 3, stride_src0, 0);
#else

      pvsrc0 = (xb_vecNx8U *)&src0_ptr[y * stride_src0];
      pvsrc1 = (xb_vecNx8U *)&src0_ptr[(y + 1) * stride_src0];
      pvsrc2 = (xb_vecNx8U *)&src0_ptr[(y + 2) * stride_src0];
      pvsrc3 = (xb_vecNx8U *)&src0_ptr[(y + 3) * stride_src0];
#endif
      rsrc_a = OFFSET_PTR_2NX8U(src1_ptr, 0, stride_src1, x);
      rsrc_b = OFFSET_PTR_2NX8U(src1_ptr, 1, stride_src1, x);
      rsrc_c = OFFSET_PTR_2NX8U(src1_ptr, 2, stride_src1, x);
      rsrc_d = OFFSET_PTR_2NX8U(src1_ptr, 3, stride_src1, x);
#if XCHAL_HAVE_NX
      valign a_load = IVP_LAN_2X32U_PP(pvsrc0);
      valign b_load = IVP_LAN_2X32U_PP(pvsrc1);
      valign c_load = IVP_LAN_2X32U_PP(pvsrc2);
      valign d_load = IVP_LAN_2X32U_PP(pvsrc3);
#else
      valign a_load = IVP_LANX8U_PP(pvsrc0);
      valign b_load = IVP_LANX8U_PP(pvsrc1);
      valign c_load = IVP_LANX8U_PP(pvsrc2);
      valign d_load = IVP_LANX8U_PP(pvsrc3);
#endif

      int partstart = 0;
      int partend = partsize;
      for (int a = 0; a < nparts; a++) {
        Accw0 = 0;
        Accw1 = 0;
        Accw2 = 0;
        Accw3 = 0;
        for (int i_2 = partstart; i_2 < partend - 3; i_2 += 4) {
          xb_vec2Nx8U vec_a, vec_b, vec_c, vec_d;

          IVP_LV2NX8U_XP(vec_a, rsrc_a, stride_src1_4);
          IVP_LV2NX8U_XP(vec_b, rsrc_b, stride_src1_4);
          IVP_LV2NX8U_XP(vec_c, rsrc_c, stride_src1_4);
          IVP_LV2NX8U_XP(vec_d, rsrc_d, stride_src1_4);

#if XCHAL_HAVE_NX
          xb_vecN_2x32Uv vec_mul0, vec_mul1, vec_mul2, vec_mul3;
          IVP_LAVN_2X32U_XP(vec_mul0, a_load, pvsrc0, 4);
          IVP_LAVN_2X32U_XP(vec_mul1, b_load, pvsrc1, 4);
          IVP_LAVN_2X32U_XP(vec_mul2, c_load, pvsrc2, 4);
          IVP_LAVN_2X32U_XP(vec_mul3, d_load, pvsrc3, 4);

          IVP_MULUUQA2N8XR8(Accw0, vec_d, vec_c, vec_b, vec_a,
                            IVP_EXTRPRN_2X32(vec_mul0, 0));
          IVP_MULUUQA2N8XR8(Accw1, vec_d, vec_c, vec_b, vec_a,
                            IVP_EXTRPRN_2X32(vec_mul1, 0));
          IVP_MULUUQA2N8XR8(Accw2, vec_d, vec_c, vec_b, vec_a,
                            IVP_EXTRPRN_2X32(vec_mul2, 0));
          IVP_MULUUQA2N8XR8(Accw3, vec_d, vec_c, vec_b, vec_a,
                            IVP_EXTRPRN_2X32(vec_mul3, 0));
#else
          xb_vecNx16U vec_mul0, vec_mul1, vec_mul2, vec_mul3;
          IVP_LAVNX8U_XP(vec_mul0, a_load, pvsrc0, 4);
          IVP_LAVNX8U_XP(vec_mul1, b_load, pvsrc1, 4);
          IVP_LAVNX8U_XP(vec_mul2, c_load, pvsrc2, 4);
          IVP_LAVNX8U_XP(vec_mul3, d_load, pvsrc3, 4);

          IVP_MULUSPA2N8XR16(
              Accw0, vec_b, vec_a,
              IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vec_mul0), 0));
          IVP_MULUSPA2N8XR16(
              Accw0, vec_d, vec_c,
              IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vec_mul0), 1));
          IVP_MULUSPA2N8XR16(
              Accw1, vec_b, vec_a,
              IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vec_mul1), 0));
          IVP_MULUSPA2N8XR16(
              Accw1, vec_d, vec_c,
              IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vec_mul1), 1));
          IVP_MULUSPA2N8XR16(
              Accw2, vec_b, vec_a,
              IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vec_mul2), 0));
          IVP_MULUSPA2N8XR16(
              Accw2, vec_d, vec_c,
              IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vec_mul2), 1));
          IVP_MULUSPA2N8XR16(
              Accw3, vec_b, vec_a,
              IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vec_mul3), 0));
          IVP_MULUSPA2N8XR16(
              Accw3, vec_d, vec_c,
              IVP_EXTRN_2X32(IVP_MOVN_2X32_FROMNX16(vec_mul3), 1));

          // IVP_MULQA2N8XR8(Accw0, vec_d, vec_c, vec_b, vec_a,
          // IVP_MOVAV32(vec_mul0)); IVP_MULQA2N8XR8(Accw1, vec_d, vec_c, vec_b,
          // vec_a, IVP_MOVAV32(vec_mul1)); IVP_MULQA2N8XR8(Accw2, vec_d, vec_c,
          // vec_b, vec_a, IVP_MOVAV32(vec_mul2)); IVP_MULQA2N8XR8(Accw3, vec_d,
          // vec_c, vec_b, vec_a, IVP_MOVAV32(vec_mul3));
#endif
        }
        partstart = partend;
        partend = XT_MIN(partend + PARTSIZE, w_src0);

        vec_out_a0 = IVP_ADDN_2X32U(vec_out_a0, IVP_CVT32S2NX24LL(Accw0));
        vec_out_a1 = IVP_ADDN_2X32U(vec_out_a1, IVP_CVT32S2NX24LH(Accw0));
        vec_out_a2 = IVP_ADDN_2X32U(vec_out_a2, IVP_CVT32S2NX24HL(Accw0));
        vec_out_a3 = IVP_ADDN_2X32U(vec_out_a3, IVP_CVT32S2NX24HH(Accw0));

        vec_out_b0 = IVP_ADDN_2X32U(vec_out_b0, IVP_CVT32S2NX24LL(Accw1));
        vec_out_b1 = IVP_ADDN_2X32U(vec_out_b1, IVP_CVT32S2NX24LH(Accw1));
        vec_out_b2 = IVP_ADDN_2X32U(vec_out_b2, IVP_CVT32S2NX24HL(Accw1));
        vec_out_b3 = IVP_ADDN_2X32U(vec_out_b3, IVP_CVT32S2NX24HH(Accw1));

        vec_out_c0 = IVP_ADDN_2X32U(vec_out_c0, IVP_CVT32S2NX24LL(Accw2));
        vec_out_c1 = IVP_ADDN_2X32U(vec_out_c1, IVP_CVT32S2NX24LH(Accw2));
        vec_out_c2 = IVP_ADDN_2X32U(vec_out_c2, IVP_CVT32S2NX24HL(Accw2));
        vec_out_c3 = IVP_ADDN_2X32U(vec_out_c3, IVP_CVT32S2NX24HH(Accw2));

        vec_out_d0 = IVP_ADDN_2X32U(vec_out_d0, IVP_CVT32S2NX24LL(Accw3));
        vec_out_d1 = IVP_ADDN_2X32U(vec_out_d1, IVP_CVT32S2NX24LH(Accw3));
        vec_out_d2 = IVP_ADDN_2X32U(vec_out_d2, IVP_CVT32S2NX24HL(Accw3));
        vec_out_d3 = IVP_ADDN_2X32U(vec_out_d3, IVP_CVT32S2NX24HH(Accw3));
      }
      Accw0 = 0;
      Accw1 = 0;
      Accw2 = 0;
      Accw3 = 0;
      uint8_t *p_src0 = (uint8_t *)pvsrc0;
      uint8_t *p_src1 = (uint8_t *)pvsrc1;
      uint8_t *p_src2 = (uint8_t *)pvsrc2;
      uint8_t *p_src3 = (uint8_t *)pvsrc3;
      for (int i = (w_src0 & (~0x3)); i < w_src0; i++) /* upto 3 iterations*/
      {
        rsrc_a = OFFSET_PTR_2NX8U(src1_ptr, i + 0, stride_src1, x);
        xb_vec2Nx8U vec_a;
        valign a_load = IVP_LA_PP(rsrc_a);
        IVP_LA2NX8U_XP(vec_a, a_load, rsrc_a, w_dst - x);
        int16_t mul0 = *p_src0;
        IVP_MULUSA2N8XR16(Accw0, vec_a, mul0);
        p_src0++;
        int16_t mul1 = *p_src1;
        IVP_MULUSA2N8XR16(Accw1, vec_a, mul1);
        p_src1++;
        int16_t mul2 = *p_src2;
        IVP_MULUSA2N8XR16(Accw2, vec_a, mul2);
        p_src2++;
        int16_t mul3 = *p_src3;
        IVP_MULUSA2N8XR16(Accw3, vec_a, mul3);
        p_src3++;
      }

      vec_out_a0 = IVP_ADDN_2X32U(vec_out_a0, IVP_CVT32S2NX24LL(Accw0));
      vec_out_a1 = IVP_ADDN_2X32U(vec_out_a1, IVP_CVT32S2NX24LH(Accw0));
      vec_out_a2 = IVP_ADDN_2X32U(vec_out_a2, IVP_CVT32S2NX24HL(Accw0));
      vec_out_a3 = IVP_ADDN_2X32U(vec_out_a3, IVP_CVT32S2NX24HH(Accw0));

      rdst = OFFSET_PTR_N_2X32U(dst_ptr, y, stride_dst, x);
      IVP_SAVN_2X32U_XP(vec_out_a0, a_store, rdst, (w_dst - x) * 4);
      IVP_SAVN_2X32U_XP(vec_out_a1, a_store, rdst,
                        (w_dst - x) * 4 - XCHAL_IVPN_SIMD_WIDTH * 2);
      IVP_SAVN_2X32U_XP(vec_out_a2, a_store, rdst,
                        (w_dst - x) * 4 - XCHAL_IVPN_SIMD_WIDTH * 4);
      IVP_SAVN_2X32U_XP(vec_out_a3, a_store, rdst,
                        (w_dst - x) * 4 - XCHAL_IVPN_SIMD_WIDTH * 6);
      IVP_SAVN_2X32POS_FP(a_store, rdst);

      vec_out_b0 = IVP_ADDN_2X32U(vec_out_b0, IVP_CVT32S2NX24LL(Accw1));
      vec_out_b1 = IVP_ADDN_2X32U(vec_out_b1, IVP_CVT32S2NX24LH(Accw1));
      vec_out_b2 = IVP_ADDN_2X32U(vec_out_b2, IVP_CVT32S2NX24HL(Accw1));
      vec_out_b3 = IVP_ADDN_2X32U(vec_out_b3, IVP_CVT32S2NX24HH(Accw1));

      rdst = OFFSET_PTR_N_2X32U(dst_ptr, y + 1, stride_dst, x);
      IVP_SAVN_2X32U_XP(vec_out_b0, a_store, rdst, (w_dst - x) * 4);
      IVP_SAVN_2X32U_XP(vec_out_b1, a_store, rdst,
                        (w_dst - x) * 4 - XCHAL_IVPN_SIMD_WIDTH * 2);
      IVP_SAVN_2X32U_XP(vec_out_b2, a_store, rdst,
                        (w_dst - x) * 4 - XCHAL_IVPN_SIMD_WIDTH * 4);
      IVP_SAVN_2X32U_XP(vec_out_b3, a_store, rdst,
                        (w_dst - x) * 4 - XCHAL_IVPN_SIMD_WIDTH * 6);
      IVP_SAVN_2X32POS_FP(a_store, rdst);

      vec_out_c0 = IVP_ADDN_2X32U(vec_out_c0, IVP_CVT32S2NX24LL(Accw2));
      vec_out_c1 = IVP_ADDN_2X32U(vec_out_c1, IVP_CVT32S2NX24LH(Accw2));
      vec_out_c2 = IVP_ADDN_2X32U(vec_out_c2, IVP_CVT32S2NX24HL(Accw2));
      vec_out_c3 = IVP_ADDN_2X32U(vec_out_c3, IVP_CVT32S2NX24HH(Accw2));

      rdst = OFFSET_PTR_N_2X32U(dst_ptr, y + 2, stride_dst, x);
      IVP_SAVN_2X32U_XP(vec_out_c0, a_store, rdst, (w_dst - x) * 4);
      IVP_SAVN_2X32U_XP(vec_out_c1, a_store, rdst,
                        (w_dst - x) * 4 - XCHAL_IVPN_SIMD_WIDTH * 2);
      IVP_SAVN_2X32U_XP(vec_out_c2, a_store, rdst,
                        (w_dst - x) * 4 - XCHAL_IVPN_SIMD_WIDTH * 4);
      IVP_SAVN_2X32U_XP(vec_out_c3, a_store, rdst,
                        (w_dst - x) * 4 - XCHAL_IVPN_SIMD_WIDTH * 6);
      IVP_SAVN_2X32POS_FP(a_store, rdst);

      vec_out_d0 = IVP_ADDN_2X32U(vec_out_d0, IVP_CVT32S2NX24LL(Accw3));
      vec_out_d1 = IVP_ADDN_2X32U(vec_out_d1, IVP_CVT32S2NX24LH(Accw3));
      vec_out_d2 = IVP_ADDN_2X32U(vec_out_d2, IVP_CVT32S2NX24HL(Accw3));
      vec_out_d3 = IVP_ADDN_2X32U(vec_out_d3, IVP_CVT32S2NX24HH(Accw3));

      rdst = OFFSET_PTR_N_2X32U(dst_ptr, y + 3, stride_dst, x);
      IVP_SAVN_2X32U_XP(vec_out_d0, a_store, rdst, (w_dst - x) * 4);
      IVP_SAVN_2X32U_XP(vec_out_d1, a_store, rdst,
                        (w_dst - x) * 4 - XCHAL_IVPN_SIMD_WIDTH * 2);
      IVP_SAVN_2X32U_XP(vec_out_d2, a_store, rdst,
                        (w_dst - x) * 4 - XCHAL_IVPN_SIMD_WIDTH * 4);
      IVP_SAVN_2X32U_XP(vec_out_d3, a_store, rdst,
                        (w_dst - x) * 4 - XCHAL_IVPN_SIMD_WIDTH * 6);
      IVP_SAVN_2X32POS_FP(a_store, rdst);
    }

    for (; y < h_dst; y++) /* upto 3 iterations*/
    {
      xb_vecN_2x32Uv vec_out_a0 = 0, vec_out_a1 = 0, vec_out_a2 = 0,
                     vec_out_a3 = 0;
#if XCHAL_HAVE_NX
      uint32_t *psrc1 = (uint32_t *)&src0_ptr[y * stride_src0];
#else
      uint8_t *psrc1 = (uint8_t *)&src0_ptr[y * stride_src0];
#endif
      xb_vec2Nx24 Accw = 0;

      rsrc_a = OFFSET_PTR_2NX8U(src1_ptr, 0, stride_src1, x);
      rsrc_b = OFFSET_PTR_2NX8U(src1_ptr, 1, stride_src1, x);
      rsrc_c = OFFSET_PTR_2NX8U(src1_ptr, 2, stride_src1, x);
      rsrc_d = OFFSET_PTR_2NX8U(src1_ptr, 3, stride_src1, x);

      int partstart = 0;
      int partend = partsize;
      int i;
      for (int a = 0; a < nparts; a++) {
        Accw = 0;
        for (i = partstart; i < partend - 3; i += 4) {
          xb_vec2Nx8U vec_a, vec_b, vec_c, vec_d;
          IVP_LV2NX8U_XP(vec_a, rsrc_a, stride_src1 * 4);
          IVP_LV2NX8U_XP(vec_b, rsrc_b, stride_src1 * 4);
          IVP_LV2NX8U_XP(vec_c, rsrc_c, stride_src1 * 4);
          IVP_LV2NX8U_XP(vec_d, rsrc_d, stride_src1 * 4);

#if XCHAL_HAVE_NX
          uint32_t mul = *psrc1;
          IVP_MULUUQA2N8XR8(Accw, vec_d, vec_c, vec_b, vec_a, mul);
          psrc1++;
#else
          uint32_t mul = (psrc1[0]) | (psrc1[1] << 16);
          IVP_MULUSPA2N8XR16(Accw, vec_b, vec_a, mul);
          mul = (psrc1[2]) | (psrc1[3] << 16);
          IVP_MULUSPA2N8XR16(Accw, vec_d, vec_c, mul);
          // IVP_MULQA2N8XR8(Accw, vec_d, vec_c, vec_b, vec_a, mul);
          psrc1 += 4;
#endif
        }
        partstart = partend;
        partend = XT_MIN(partend + PARTSIZE, w_src0);

        vec_out_a0 = IVP_ADDN_2X32U(vec_out_a0, IVP_CVT32S2NX24LL(Accw));
        vec_out_a1 = IVP_ADDN_2X32U(vec_out_a1, IVP_CVT32S2NX24LH(Accw));
        vec_out_a2 = IVP_ADDN_2X32U(vec_out_a2, IVP_CVT32S2NX24HL(Accw));
        vec_out_a3 = IVP_ADDN_2X32U(vec_out_a3, IVP_CVT32S2NX24HH(Accw));
      }
      Accw = 0;
      uint8_t *p_src1 = (uint8_t *)psrc1;
      for (; i < w_src0; i++) /* upto 3 iterations*/
      {
        rsrc_a = OFFSET_PTR_2NX8U(src1_ptr, i + 0, stride_src1, x);
        xb_vec2Nx8U vec_a;
        valign a_load = IVP_LA_PP(rsrc_a);
        IVP_LA2NX8U_XP(vec_a, a_load, rsrc_a, w_dst - x);
        int16_t mul = *p_src1;
        IVP_MULUSA2N8XR16(Accw, vec_a, mul);
        p_src1++;
      }

      vec_out_a0 = IVP_ADDN_2X32U(vec_out_a0, IVP_CVT32S2NX24LL(Accw));
      vec_out_a1 = IVP_ADDN_2X32U(vec_out_a1, IVP_CVT32S2NX24LH(Accw));
      vec_out_a2 = IVP_ADDN_2X32U(vec_out_a2, IVP_CVT32S2NX24HL(Accw));
      vec_out_a3 = IVP_ADDN_2X32U(vec_out_a3, IVP_CVT32S2NX24HH(Accw));

      rdst = OFFSET_PTR_N_2X32U(dst_ptr, y, stride_dst, x);
      IVP_SAVN_2X32U_XP(vec_out_a0, a_store, rdst, (w_dst - x) * 4);
      IVP_SAVN_2X32U_XP(vec_out_a1, a_store, rdst,
                        (w_dst - x) * 4 - XCHAL_IVPN_SIMD_WIDTH * 2);
      IVP_SAVN_2X32U_XP(vec_out_a2, a_store, rdst,
                        (w_dst - x) * 4 - XCHAL_IVPN_SIMD_WIDTH * 4);
      IVP_SAVN_2X32U_XP(vec_out_a3, a_store, rdst,
                        (w_dst - x) * 4 - XCHAL_IVPN_SIMD_WIDTH * 6);
      IVP_SAVN_2X32POS_FP(a_store, rdst);
    }
  }
  return 0; // XI_ERROR_STATUS();
}