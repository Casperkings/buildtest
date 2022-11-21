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

XI_ERR_TYPE xiPyrDown_3x3_U8(const xi_pTile src, xi_pArray dst) {

  const int32_t sstride = XI_TILE_GET_PITCH(src);
  const int32_t dstride = XI_ARRAY_GET_PITCH(dst);
  const int32_t width = XI_TILE_GET_WIDTH(src);
  const int32_t height = XI_TILE_GET_HEIGHT(src);

  uint8_t *restrict tdst = (uint8_t *)XI_ARRAY_GET_DATA_PTR(dst);
  xb_vec2Nx8U *restrict rsrc;
  xb_vecNx8U *restrict rdst;

  xb_vec2Nx8U *restrict rsrc2;
  uint8_t *restrict tsrc =
      (uint8_t *)OFFSET_PTR_2NX8U(XI_TILE_GET_DATA_PTR(src), 0, sstride, -1);

  xb_vec2Nx8U One2 = IVP_MOV2NX8_FROMNX16(0x0201);
  xb_vec2Nx8U Two4 = IVP_MOV2NX8_FROMNX16(0x0402);

  valign a_store = IVP_ZALIGN();

  int32_t j = 0;
  for (; j < width - 2 * XCHAL_IVPN_SIMD_WIDTH;
       j += 4 * XCHAL_IVPN_SIMD_WIDTH) {
    xb_vec2Nx8U *psrc = OFFSET_PTR_2NX8U(tsrc, 0, sstride, j);
    rsrc = OFFSET_PTR_2NX8U(psrc, -1, sstride, 0);

    int32_t ltail = XT_MIN((width - j + 1) / 2 - XCHAL_IVPN_SIMD_WIDTH,
                           XCHAL_IVPN_SIMD_WIDTH);
    int32_t dstep = dstride - (XCHAL_IVPN_SIMD_WIDTH + ltail);
    int32_t sstep =
        2 * sstride - XT_MIN(6 * XCHAL_IVPN_SIMD_WIDTH, width - j + 2);

    valign a_load = IVP_LA2NX8U_PP(rsrc);
    xb_vec2Nx8U rw0;
    IVP_LA2NX8U_IP(rw0, a_load, rsrc);
    xb_vec2Nx8U rw1;
    IVP_LAV2NX8U_XP(rw1, a_load, rsrc,
                    width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);
    xb_vec2Nx8U rw2;
    IVP_LAV2NX8U_XP(rw2, a_load, rsrc,
                    width - j + 2 - 4 * XCHAL_IVPN_SIMD_WIDTH);
    rsrc = OFFSET_PTR_2NX8U(rsrc, -1, sstride, 0);

    rsrc2 = OFFSET_PTR_2NX8U(psrc, 1, sstride, 0);
    rdst = OFFSET_PTR_NX8U(tdst, -1, dstep, j / 2);
    for (int32_t i = 0; i < height; i += 2) {
      xb_vec2Nx24 w0 = IVP_MULUU2NX8(rw0, One2);
      xb_vec2Nx24 w1 = IVP_MULUU2NX8(rw1, One2);
      xb_vecNx16 w2 = IVP_ANDNX16(IVP_MOVNX16_FROM2NX8(rw2), 0xFF);

      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstep, 0);
      a_load = IVP_LA2NX8U_PP(rsrc);
      IVP_LA2NX8U_IP(rw0, a_load, rsrc);
      IVP_LAV2NX8U_XP(rw1, a_load, rsrc,
                      width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_LAV2NX8U_XP(rw2, a_load, rsrc,
                      width - j + 2 - 4 * XCHAL_IVPN_SIMD_WIDTH);

      IVP_MULUUA2NX8(w0, rw0, Two4);
      IVP_MULUUA2NX8(w1, rw1, Two4);
      xb_vecNx16 w3 = IVP_ANDNX16(IVP_MOVNX16_FROM2NX8(rw2), 0xFF);
      w2 = IVP_ADDNX16(w2, w3);
      w2 = IVP_ADDNX16(w2, w3);

      a_load = IVP_LA2NX8U_PP(rsrc2);
      IVP_LA2NX8U_IP(rw0, a_load, rsrc2);
      IVP_LAV2NX8U_XP(rw1, a_load, rsrc2,
                      width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_LAV2NX8U_XP(rw2, a_load, rsrc2,
                      width - j + 2 - 4 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc2 = OFFSET_PTR_2NX8U(rsrc2, 1, sstep, 0);

      IVP_MULUUA2NX8(w0, rw0, One2);
      IVP_MULUUA2NX8(w1, rw1, One2);
      w2 = IVP_ADDNX16(w2, IVP_ANDNX16(IVP_MOVNX16_FROM2NX8(rw2), 0xFF));

      xb_vecNx16U vd00 = IVP_PACKL2NX24_0(w0);
      xb_vecNx16U vd01 = IVP_PACKL2NX24_0(w1);
      // xb_vecNx16U vd20 = IVP_ADDNX16U(IVP_PACKL2NX24_1(w0), 8);
      // xb_vecNx16U vd21 = IVP_ADDNX16U(IVP_PACKL2NX24_1(w1), 8);

      xb_vecNx16U vd20 = IVP_PACKL2NX24_1(w0);
      xb_vecNx16U vd21 = IVP_PACKL2NX24_1(w1);

      xb_vecNx16U vd10 = IVP_SELNX16I(vd01, vd00, IVP_SELI_ROTATE_RIGHT_1);
      xb_vecNx16U vd11 = IVP_SELNX16I(w2, vd01, IVP_SELI_ROTATE_RIGHT_1);

      vd00 = IVP_SRAINX16(IVP_ADDNX16U(vd10, IVP_ADDNX16U(vd00, vd20)), 4);
      vd01 = IVP_SRAINX16(IVP_ADDNX16U(vd11, IVP_ADDNX16U(vd01, vd21)), 4);

      rdst = OFFSET_PTR_NX8U(rdst, 1, dstep, 0);
      IVP_SANX8U_IP(vd00, a_store, rdst);
      IVP_SAVNX8U_XP(vd01, a_store, rdst, ltail);
      IVP_SAPOSNX8U_FP(a_store, rdst);
    }
  }

  if (j < width) {
    xb_vec2Nx8U *psrc = OFFSET_PTR_2NX8U(tsrc, 0, sstride, j);
    rsrc = OFFSET_PTR_2NX8U(psrc, -1, sstride, 0);

    int32_t ltail = (width - j + 1) / 2;
    int32_t dstep = dstride - ltail;
    int32_t sstep = 2 * sstride - width + j - 2;

    valign a_load = IVP_LA2NX8U_PP(rsrc);
    xb_vec2Nx8U rw0;
    IVP_LAV2NX8U_XP(rw0, a_load, rsrc, width - j + 2);
    xb_vec2Nx8U rw1;
    IVP_LAV2NX8U_XP(rw1, a_load, rsrc,
                    width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);
    rsrc = OFFSET_PTR_2NX8U(rsrc, -1, sstride, 0);

    rsrc2 = OFFSET_PTR_2NX8U(psrc, 1, sstride, 0);
    rdst = OFFSET_PTR_NX8U(tdst, -1, dstep, j / 2);
    for (int32_t i = 0; i < height; i += 2) {
      xb_vec2Nx24 w0 = IVP_MULUU2NX8(rw0, One2);
      xb_vecNx16 w1 = IVP_ANDNX16(IVP_MOVNX16_FROM2NX8(rw1), 0xFF);

      rsrc = OFFSET_PTR_2NX8U(rsrc, 1, sstep, 0);
      a_load = IVP_LA2NX8U_PP(rsrc);
      IVP_LAV2NX8U_XP(rw0, a_load, rsrc, width - j + 2);
      IVP_LAV2NX8U_XP(rw1, a_load, rsrc,
                      width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);

      IVP_MULUUA2NX8(w0, rw0, Two4);

      w1 = IVP_ADDNX16(w1, IVP_ANDNX16(IVP_MOVNX16_FROM2NX8(rw1), 0xFF));
      w1 = IVP_ADDNX16(w1, IVP_ANDNX16(IVP_MOVNX16_FROM2NX8(rw1), 0xFF));

      a_load = IVP_LA2NX8U_PP(rsrc2);
      IVP_LAV2NX8U_XP(rw0, a_load, rsrc2, width - j + 2);
      IVP_LAV2NX8U_XP(rw1, a_load, rsrc2,
                      width - j + 2 - 2 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc2 = OFFSET_PTR_2NX8U(rsrc2, 1, sstep, 0);

      IVP_MULUUA2NX8(w0, rw0, One2);
      w1 = IVP_ADDNX16(w1, IVP_ANDNX16(IVP_MOVNX16_FROM2NX8(rw1), 0xFF));

      xb_vecNx16U vd00 = IVP_PACKL2NX24_0(w0);
      // xb_vecNx16U vd20 = IVP_ADDNX16U(IVP_PACKL2NX24_1(w0), 8);
      xb_vecNx16U vd20 = IVP_PACKL2NX24_1(w0);
      xb_vecNx16U vd10 = IVP_SELNX16I(w1, vd00, IVP_SELI_ROTATE_RIGHT_1);

      vd00 = IVP_SRAINX16(IVP_ADDNX16U(vd00, IVP_ADDNX16U(vd10, vd20)), 4);

      rdst = OFFSET_PTR_NX8U(rdst, 1, dstep, 0);
      IVP_SAVNX8U_XP(vd00, a_store, rdst, ltail);
      IVP_SAPOSNX8U_FP(a_store, rdst);
    }
  }

  return 0;
}
XI_ERR_TYPE xiPyrDown_3x3_U8_ref(const xi_pTile src, xi_pArray dst) {

  int kernel[] = {

      1, 2, 1, 2, 4, 2, 1, 2, 1};

  const uint8_t ksize = 3;
  const uint8_t N_div = ksize / 2;

  const uint8_t *src_img = (const uint8_t *)(XI_TILE_GET_DATA_PTR(src));
  uint8_t *dst_img = (uint8_t *)(XI_ARRAY_GET_DATA_PTR(dst));

  for (int i = 0; i < XI_TILE_GET_HEIGHT(src); i += 2)
    for (int j = 0; j < XI_TILE_GET_WIDTH(src); j += 2) {
      int32_t sum = 0;
      for (int k = -N_div, kk = 0; k <= N_div; k++, kk++)
        for (int l = -N_div, ll = 0; l <= N_div; l++, ll++)
          sum += src_img[(i + k) * XI_TILE_GET_PITCH(src) + (j + l)] *
                 kernel[kk * ksize + ll];

      dst_img[(i / 2) * XI_ARRAY_GET_PITCH(dst) + (j / 2)] = (sum) / 16;
    }

  return XI_ERR_OK;
}
#if 0 
XI_ERR_TYPE xiRemap(const xi_pTile src, xi_pTile dst, const int16_t *table)
{
	const int32_t width   = XI_TILE_GET_WIDTH(src);
	const int32_t height  = XI_TILE_GET_HEIGHT(src);
	const int32_t sstride = XI_TILE_GET_PITCH(src);
	const int32_t dstride = XI_TILE_GET_PITCH(dst);
	const int32_t source_edge_width = XI_TILE_GET_EDGE_WIDTH(src);
	const int32_t source_edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
	const int32_t dest_edge_width = XI_TILE_GET_EDGE_WIDTH(dst);
	const int32_t dest_edge_height = XI_TILE_GET_EDGE_HEIGHT(dst);
	int twidth, theight;
	int i, j;
	xb_vecNx16 *restrict pvectable, *restrict rdst, vec;
	valign va1, va2;
	uint8_t* restrict tsrc = (uint8_t *)OFFSET_PTR_2NX8U(XI_TILE_GET_DATA_PTR(src), - source_edge_height, sstride, - source_edge_width);
	int16_t* restrict tdst = (int16_t *)OFFSET_PTR_NX16(XI_TILE_GET_DATA_PTR(dst), - dest_edge_height, dstride, - dest_edge_width);

	twidth = width + 2 * source_edge_width;
	theight = height + 2 * source_edge_height;
//printf("w = %d\th = %d\tss = %d\tds = %d\n", width, height, sstride, dstride);
//printf("%d, %d, %d, %d\n",source_edge_width, source_edge_height, dest_edge_width, dest_edge_height);
//printf("src pointers = %d\t%d\n", XI_TILE_GET_BUFF_PTR(src), XI_TILE_GET_DATA_PTR(src));
//printf("dst ptr = %d\t%d\n", XI_TILE_GET_BUFF_PTR(dst), XI_TILE_GET_DATA_PTR(dst));
//printf("start ptr = %d\t%d\n", tsrc, tdst);
	va2 = IVP_ZALIGN();
	for (i = 0; i < twidth; i++)
	{
		rdst = OFFSET_PTR_NX16(tdst, 0, dstride, i*8);
		for (j= 0; j < theight; j++)
		{ 
			uint8_t gval = tsrc[j * sstride + i];
			pvectable = (xb_vecNx16 *)(table + (gval * 8));
			va1 = IVP_LANX16_PP(pvectable);
			IVP_LAVNX16_XP(vec, va1, pvectable, 16);
			IVP_SAVNX16_XP(vec, va2, rdst, 16);
			IVP_SAPOSNX16_FP(va2, rdst);
			rdst = OFFSET_PTR_NX16(rdst, 1, (dstride - 8), 0);
		} 
	}
#if 0
	{
		uint8_t *p_src = XI_TILE_GET_DATA_PTR(src);
		int16_t *p_dst = XI_TILE_GET_DATA_PTR(dst);
		uint8_t *p_src1;
		int16_t	*p_dst1;
		//p_src1 = p_src - 131;
		//p_dst1 = p_dst - 1048;
		p_src1 = p_src - 1;
		p_dst1 = p_dst - 8;
		printf("{\n");
		for(int f = 0; f < 130; f++){
		printf("(%d, %d),\n", p_src1[f], p_dst1[8*f]);}
		printf("}\n");
	}
#endif
	return 0;
}
#else
XI_ERR_TYPE xiRemap1(const xi_pTile src, xi_pTile dst, const int16_t *table) {
  const int32_t width = XI_TILE_GET_WIDTH(src);
  const int32_t height = XI_TILE_GET_HEIGHT(src);
  const int32_t sstride = XI_TILE_GET_PITCH(src);
  const int32_t dstride = XI_TILE_GET_PITCH(dst);
  const int32_t source_edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  const int32_t source_edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
  const int32_t dest_edge_width = XI_TILE_GET_EDGE_WIDTH(dst);
  const int32_t dest_edge_height = XI_TILE_GET_EDGE_HEIGHT(dst);
  int twidth, theight;
  int i, j;
  xb_vecNx16 *restrict pvectable, *restrict rdst, vec1, vec2, vec3, vec4;
  valign va1, va2;
  uint8_t *restrict tsrc = (uint8_t *)OFFSET_PTR_2NX8U(
      XI_TILE_GET_DATA_PTR(src), -source_edge_height, sstride,
      -source_edge_width);
  int16_t *restrict tdst = (int16_t *)OFFSET_PTR_NX16(
      XI_TILE_GET_DATA_PTR(dst), -dest_edge_height, dstride, -dest_edge_width);

  twidth = width + 2 * source_edge_width;
  theight = height + 2 * source_edge_height;

  va2 = IVP_ZALIGN();
  for (i = 0; i < twidth - 3; i += 4) {
    rdst = OFFSET_PTR_NX16(tdst, 0, dstride, i * 8);
    for (j = 0; j < theight; j++) {
      uint8_t gval = tsrc[j * sstride + i];
      pvectable = (xb_vecNx16 *)(table + (gval * 8));
      va1 = IVP_LANX16_PP(pvectable);
      IVP_LAVNX16_XP(vec1, va1, pvectable, 16);

      gval = tsrc[j * sstride + i + 1];
      pvectable = (xb_vecNx16 *)(table + (gval * 8));
      va1 = IVP_LANX16_PP(pvectable);
      IVP_LAVNX16_XP(vec2, va1, pvectable, 16);

      gval = tsrc[j * sstride + i + 2];
      pvectable = (xb_vecNx16 *)(table + (gval * 8));
      va1 = IVP_LANX16_PP(pvectable);
      IVP_LAVNX16_XP(vec3, va1, pvectable, 16);

      gval = tsrc[j * sstride + i + 3];
      pvectable = (xb_vecNx16 *)(table + (gval * 8));
      va1 = IVP_LANX16_PP(pvectable);
      IVP_LAVNX16_XP(vec4, va1, pvectable, 16);

      vec1 = IVP_SELNX16I(vec2, vec1, IVP_SELI_16B_PACK_8);
      vec3 = IVP_SELNX16I(vec4, vec3, IVP_SELI_16B_PACK_8);
      vec1 = IVP_SELNX16I(vec3, vec1, IVP_SELI_16B_EXTRACT_LO_HALVES);
      IVP_SANX16_IP(vec1, va2, rdst);
      IVP_SAPOSNX16_FP(va2, rdst);
      rdst = OFFSET_PTR_NX16(rdst, 1, (dstride - 32), 0);
    }
  }
  if (i < twidth) {
    int flag1 = ((twidth - i) > 1);
    int flag2 = ((twidth - i) > 2);
    rdst = OFFSET_PTR_NX16(tdst, 0, dstride, i * 8);
    for (j = 0; j < theight; j++) {
      uint8_t gval = tsrc[j * sstride + i];
      pvectable = (xb_vecNx16 *)(table + (gval * 8));
      va1 = IVP_LANX16_PP(pvectable);
      IVP_LAVNX16_XP(vec1, va1, pvectable, 16);

      gval = tsrc[j * sstride + i + 1 * flag1];
      pvectable = (xb_vecNx16 *)(table + (gval * 8));
      va1 = IVP_LANX16_PP(pvectable);
      IVP_LAVNX16_XP(vec2, va1, pvectable, 16);

      gval = tsrc[j * sstride + i + 2 * flag2];
      pvectable = (xb_vecNx16 *)(table + (gval * 8));
      va1 = IVP_LANX16_PP(pvectable);
      IVP_LAVNX16_XP(vec3, va1, pvectable, 16);

      vec1 = IVP_SELNX16I(vec2, vec1, IVP_SELI_16B_PACK_8);
      vec3 = IVP_SELNX16I(vec3, vec3, IVP_SELI_16B_PACK_8);
      vec1 = IVP_SELNX16I(vec3, vec1, IVP_SELI_16B_EXTRACT_LO_HALVES);
      IVP_SAVNX16_XP(vec1, va2, rdst, (twidth - i) * 16);
      IVP_SAPOSNX16_FP(va2, rdst);
      rdst = OFFSET_PTR_NX16(rdst, 1, (dstride - ((twidth - i) * 8)), 0);
    }
  }
  return 0;
}
XI_ERR_TYPE xiRemap(const xi_pTile src, xi_pTile dst, const int16_t *table) {
  const int32_t width = XI_TILE_GET_WIDTH(src);
  const int32_t height = XI_TILE_GET_HEIGHT(src);
  const int32_t sstride = XI_TILE_GET_PITCH(src);
  const int32_t dstride = XI_TILE_GET_PITCH(dst);
  const int32_t source_edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  const int32_t source_edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
  const int32_t dest_edge_width = XI_TILE_GET_EDGE_WIDTH(dst);
  const int32_t dest_edge_height = XI_TILE_GET_EDGE_HEIGHT(dst);
  int twidth, theight;
  int i, j;
  xb_vecNx16 *restrict pvectable, *restrict rdst;
  valign va1, va2;
  uint8_t *restrict tsrc =
      (uint8_t *)OFFSET_PTR_2NX8U(XI_TILE_GET_DATA_PTR(src), 0, 0, 0);
  int16_t *restrict tdst =
      (int16_t *)OFFSET_PTR_NX16(XI_TILE_GET_DATA_PTR(dst), 0, 0, 0);
  xb_vec2Nx8U *restrict pvecg, vecg;
  xb_vecNx16 vec1, vecseq, vecpattern, vecoffset, vecgatheroffset, vecremap;
  xb_gsr vg;
  twidth = width;
  theight = height;
  vecseq = IVP_SEQNX16U();
  vecpattern = IVP_SRLINX16(vecseq, 3);
  vecoffset = IVP_ANDNX16(vecseq, 7);
  va2 = IVP_ZALIGN();
  for (i = 0; i < twidth - 3; i += 4) {
    rdst = OFFSET_PTR_NX16(tdst, 0, dstride, i * 8);
    for (j = 0; j < theight; j++) {
      pvecg = OFFSET_PTR_2NX8U(tsrc, j, sstride, i);
      va1 = IVP_LA2NX8_PP(pvecg);
      IVP_LAV2NX8_XP(vecg, va1, pvecg, 4);
      vecg = IVP_SEL2NX8I(0, vecg, IVP_SELI_8B_INTERLEAVE_1_LO);
      vec1 = IVP_MOVNX16_FROM2NX8(vecg);
      vec1 = IVP_SLLINX16(vec1, 3);
      vec1 = IVP_SHFLNX16(vec1, vecpattern);
      vec1 = IVP_ADDNX16(vec1, vecoffset);
      vecgatheroffset = IVP_SLLINX16(vec1, 1);
      vg = IVP_GATHERANX16(table, vecgatheroffset);
      vecremap = IVP_GATHERDNX16(vg);
      IVP_SANX16_IP(vecremap, va2, rdst);
      IVP_SAPOSNX16_FP(va2, rdst);
      rdst = OFFSET_PTR_NX16(rdst, 1, (dstride - 32), 0);
    }
  }
  if (i < twidth) {
    vboolN vb;
    vb = IVP_LTNX16(IVP_SEQNX16U(), 8 * (twidth - i));
    rdst = OFFSET_PTR_NX16(tdst, 0, dstride, i * 8);
    for (j = 0; j < theight; j++) {
      pvecg = OFFSET_PTR_2NX8U(tsrc, j, sstride, i);
      va1 = IVP_LA2NX8_PP(pvecg);
      IVP_LAV2NX8_XP(vecg, va1, pvecg, (twidth - i));
      vecg = IVP_SEL2NX8I(0, vecg, IVP_SELI_8B_INTERLEAVE_1_LO);
      vec1 = IVP_MOVNX16_FROM2NX8(vecg);
      vec1 = IVP_SLLINX16(vec1, 3);
      vec1 = IVP_SHFLNX16(vec1, vecpattern);
      vec1 = IVP_ADDNX16(vec1, vecoffset);
      vecgatheroffset = IVP_SLLINX16(vec1, 1);
      vg = IVP_GATHERANX16T(table, vecgatheroffset, vb);
      vecremap = IVP_GATHERDNX16(vg);
      IVP_SAVNX16_XP(vecremap, va2, rdst, (twidth - i) * 16);
      IVP_SAPOSNX16_FP(va2, rdst);
      rdst = OFFSET_PTR_NX16(rdst, 1, (dstride - ((twidth - i) * 8)), 0);
    }
  }
  return 0;
}

#endif
XI_ERR_TYPE xiRemap_ref(const xi_pTile src, xi_pTile dst,
                        const int16_t *table) {
  const int32_t width = XI_TILE_GET_WIDTH(src);
  const int32_t height = XI_TILE_GET_HEIGHT(src);
  const int32_t sstride = XI_TILE_GET_PITCH(src);
  const int32_t dstride = XI_TILE_GET_PITCH(dst);
  const int32_t source_edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  const int32_t source_edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
  const int32_t dest_edge_width = XI_TILE_GET_EDGE_WIDTH(dst);
  const int32_t dest_edge_height = XI_TILE_GET_EDGE_HEIGHT(dst);
  int i, j, k;
  uint8_t *psrc = XI_TILE_GET_DATA_PTR(src);
  int16_t *pdst = XI_TILE_GET_DATA_PTR(dst);
  // printf("src pointers = %d\t%d\n", XI_TILE_GET_BUFF_PTR(src),
  // XI_TILE_GET_DATA_PTR(src));
  // printf("dst ptr = %d\t%d\n", XI_TILE_GET_BUFF_PTR(dst),
  // XI_TILE_GET_DATA_PTR(dst));

  // for (j= - source_edge_height; j < (height + source_edge_height); j++)
  for (j = 0; j < height; j++) {
    // for (i = - source_edge_width; i < (width + source_edge_width); i++)
    for (i = 0; i < width; i++) {
      uint8_t gval = psrc[(j * sstride) + i];
      // printf("gval = %d, val = %d\n", gval, table[(gval * 8) + 0]);
      // return 0;
      for (k = 0; k < 8; k++) {
        pdst[(j * dstride) + (i * 8) + k] = table[(gval * 8) + k];
        // printf("gval = %d, val = %d, pdst = %d\n", gval, table[(gval * 8) +
        // k], pdst[j * dstride + (i*8) + k]);
      }
      // return 0;
    }
  }
#if 0
	{
		uint8_t *p_src = XI_TILE_GET_DATA_PTR(src);
		int16_t *p_dst = XI_TILE_GET_DATA_PTR(dst);
		uint8_t *p_src1;
		int16_t	*p_dst1;
		//p_src1 = p_src - 131;
		//p_dst1 = p_dst - 1048;
		p_src1 = p_src - 1;
		p_dst1 = p_dst - 8;
		printf("{\n");
		for(int f = 0; f < 130; f++){
		printf("(%d, %d),\n", p_src1[f], p_dst1[8*f]);}
		printf("}\n");
	}
#endif
  return 0;
}
int xiCompareTilesS16(const xi_pTile src, const xi_pTile dst) {
  const int32_t width = XI_TILE_GET_WIDTH(src);
  const int32_t height = XI_TILE_GET_HEIGHT(src);
  const int32_t sstride = XI_TILE_GET_PITCH(src);
  const int32_t dstride = XI_TILE_GET_PITCH(dst);
  const int32_t source_edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  const int32_t source_edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
  const int32_t dest_edge_width = XI_TILE_GET_EDGE_WIDTH(dst);
  const int32_t dest_edge_height = XI_TILE_GET_EDGE_HEIGHT(dst);
  int i, j; //, flag;
  int16_t *psrc = XI_TILE_GET_DATA_PTR(src);
  int16_t *pdst = XI_TILE_GET_DATA_PTR(dst);

  // flag = 0;
  for (j = -source_edge_height; j < (height + source_edge_height); j++) {
    for (i = -source_edge_width; i < (width + source_edge_width); i++) {
      if (psrc[j * sstride + i] != pdst[j * dstride + i]) {
        // printf("IVP = %d\tRef = %d (i, j) = (%d, %d)\n", psrc[j * sstride +
        // i], pdst[j *dstride + i], i, j);
        // flag = -1;
        return (-1);
      }
    }
  }
  // return flag;
  return 0;
}

XI_ERR_TYPE xiPyrDownK_3x3_S16(const xi_pTile src, xi_pArray dst) {
  const int32_t sstride = XI_TILE_GET_PITCH(src);
  const int32_t dstride = XI_ARRAY_GET_PITCH(dst);
  const int32_t width = XI_TILE_GET_WIDTH(src);
  const int32_t height = XI_TILE_GET_HEIGHT(src);
  const int32_t edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  const int32_t edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
  int16_t *restrict tdst = (int16_t *)XI_ARRAY_GET_DATA_PTR(dst);
  xb_vecNx16 *restrict rsrc;
  xb_vecNx16 *restrict rdst;

  xb_vecNx16 *restrict rsrc2;
  int16_t *restrict tsrc =
      (int16_t *)OFFSET_PTR_NX16(XI_TILE_GET_DATA_PTR(src), 0, sstride, -8);

  xb_vecNx16 vecseq = IVP_SEQNX16U();
  xb_vecNx16 vecpattern = IVP_SRLINX16(vecseq, 3);
  vecpattern = IVP_ANDNX16(vecpattern, 1);
  xb_vecNx16 One2 = IVP_ADDNX16(vecpattern, 1);
  xb_vecNx16 Two4 = IVP_SLLINX16(One2, 1);

  xb_vecNx16 vecpattern1 = IVP_SRLINX16(vecseq, 3);
  vecpattern1 = IVP_SLLINX16(vecpattern1, 4);
  xb_vecNx16 vecseq1 = IVP_ANDNX16(vecseq, 7);
  vecpattern1 = IVP_ADDNX16(vecseq1, vecpattern1);
  xb_vecNx16 vecpattern2 = IVP_ADDNX16(vecpattern1, 8);
  valign a_store = IVP_ZALIGN();
#if 1
  int32_t j = 0;
  for (; j < width - XCHAL_IVPN_SIMD_WIDTH; j += 2 * XCHAL_IVPN_SIMD_WIDTH) {
    xb_vecNx16 *psrc = OFFSET_PTR_NX16(tsrc, 0, sstride, j);
    rsrc = OFFSET_PTR_NX16(psrc, -1, sstride, 0);

    int32_t ltail = XT_MIN(((width - j + 8) / 16) * 8, XCHAL_IVPN_SIMD_WIDTH);
    int32_t dstep = dstride - ltail;
    int32_t sstep =
        2 * sstride - XT_MIN(3 * XCHAL_IVPN_SIMD_WIDTH, width - j + 16);

    valign a_load = IVP_LANX16_PP(rsrc);
    xb_vecNx16 rw0;
    IVP_LANX16_IP(rw0, a_load, rsrc);
    xb_vecNx16 rw1;
    IVP_LAVNX16_XP(
        rw1, a_load, rsrc,
        2 * (width - j + 16) -
            2 * XCHAL_IVPN_SIMD_WIDTH); // 16 total edge elements//2 - bytes
    xb_vecNx16 rw2;
    IVP_LAVNX16_XP(rw2, a_load, rsrc,
                   2 * (width - j + 16) - 4 * XCHAL_IVPN_SIMD_WIDTH);
    rsrc = OFFSET_PTR_NX16(rsrc, -1, sstride, 0);

    rsrc2 = OFFSET_PTR_NX16(psrc, 1, sstride, 0);
    rdst = OFFSET_PTR_NX16(tdst, -1, dstep, j / 2);
    for (int32_t i = 0; i < height; i += 2) {
      xb_vecNx48 w0 = IVP_MULNX16(rw0, One2);
      xb_vecNx48 w1 = IVP_MULNX16(rw1, One2);
      xb_vecNx16 w2 = rw2;

      rsrc = OFFSET_PTR_NX16(rsrc, 1, sstep, 0);
      a_load = IVP_LANX16_PP(rsrc);
      IVP_LANX16_IP(rw0, a_load, rsrc);
      IVP_LAVNX16_XP(rw1, a_load, rsrc,
                     2 * (width - j + 16) - 2 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_LAVNX16_XP(rw2, a_load, rsrc,
                     2 * (width - j + 16) - 4 * XCHAL_IVPN_SIMD_WIDTH);

      IVP_MULANX16(w0, rw0, Two4);
      IVP_MULANX16(w1, rw1, Two4);
      xb_vecNx16 w3 = rw2;
      w2 = IVP_ADDNX16(w2, w3);
      w2 = IVP_ADDNX16(w2, w3);

      a_load = IVP_LANX16_PP(rsrc2);
      IVP_LANX16_IP(rw0, a_load, rsrc2);
      IVP_LAVNX16_XP(rw1, a_load, rsrc2,
                     2 * (width - j + 16) - 2 * XCHAL_IVPN_SIMD_WIDTH);
      IVP_LAVNX16_XP(rw2, a_load, rsrc2,
                     2 * (width - j + 16) - 4 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc2 = OFFSET_PTR_NX16(rsrc2, 1, sstep, 0);

      IVP_MULANX16(w0, rw0, One2);
      IVP_MULANX16(w1, rw1, One2);
      w2 = IVP_ADDNX16(w2, rw2);

      xb_vecNx16U vd00 = IVP_PACKLNX48(w0);
      xb_vecNx16U vd01 = IVP_PACKLNX48(w1);
      xb_vecNx16U vd10 = IVP_SELNX16(vd01, vd00, vecpattern1);
      xb_vecNx16U vd11 = IVP_SELNX16(vd01, vd00, vecpattern2);
      xb_vecNx16U vd12 = IVP_SELNX16I(w2, vd10, IVP_SELI_ROTATE_RIGHT_8);

      vd00 = IVP_SRAINX16(IVP_ADDNX16(vd12, IVP_ADDNX16(vd10, vd11)), 4);

      rdst = OFFSET_PTR_NX16(rdst, 1, dstep, 0);
      // vd00 = IVP_MULNX16PACKL(IVP_SEQNX16() , 7);
      IVP_SAVNX16_XP(vd00, a_store, rdst, 2 * ltail);
      IVP_SAPOSNX16_FP(a_store, rdst);
    }
  }

  if (j < width) {

    xb_vecNx16 *psrc = OFFSET_PTR_NX16(tsrc, 0, sstride, j);
    rsrc = OFFSET_PTR_NX16(psrc, -1, sstride, 0);

    int32_t ltail = ((width - j + 8) / 16) * 8;
    int32_t dstep = dstride - ltail;
    int32_t sstep = 2 * sstride - width + j - 16;

    valign a_load = IVP_LANX16_PP(rsrc);
    xb_vecNx16 rw0;
    IVP_LAVNX16_XP(rw0, a_load, rsrc, 2 * (width - j + 16));
    xb_vecNx16 rw1;
    IVP_LAVNX16_XP(rw1, a_load, rsrc,
                   2 * (width - j + 16) - 2 * XCHAL_IVPN_SIMD_WIDTH);
    rsrc = OFFSET_PTR_NX16(rsrc, -1, sstride, 0);

    rsrc2 = OFFSET_PTR_NX16(psrc, 1, sstride, 0);
    rdst = OFFSET_PTR_NX16(tdst, -1, dstep, j / 2);
    for (int32_t i = 0; i < height; i += 2) {
      xb_vecNx48 w0 = IVP_MULNX16(rw0, One2);
      xb_vecNx16 w1 = rw1; // IVP_ANDNX16(IVP_MOVNX16_FROM2NX8(rw1), 0xFF);

      rsrc = OFFSET_PTR_NX16(rsrc, 1, sstep, 0);
      a_load = IVP_LANX16_PP(rsrc);
      IVP_LAVNX16_XP(rw0, a_load, rsrc, 2 * (width - j + 16));
      IVP_LAVNX16_XP(rw1, a_load, rsrc,
                     2 * (width - j + 16) - 2 * XCHAL_IVPN_SIMD_WIDTH);

      IVP_MULANX16(w0, rw0, Two4);

      w1 = IVP_ADDNX16(w1, rw1);
      w1 = IVP_ADDNX16(w1, rw1);

      a_load = IVP_LANX16_PP(rsrc2);
      IVP_LAVNX16_XP(rw0, a_load, rsrc2, 2 * (width - j + 16));
      IVP_LAVNX16_XP(rw1, a_load, rsrc2,
                     2 * (width - j + 16) - 2 * XCHAL_IVPN_SIMD_WIDTH);
      rsrc2 = OFFSET_PTR_NX16(rsrc2, 1, sstep, 0);

      IVP_MULANX16(w0, rw0, One2);
      w1 = IVP_ADDNX16(w1, rw1);

      xb_vecNx16U vd00 = IVP_PACKLNX48(w0);
      xb_vecNx16U vd10 = IVP_SELNX16(w1, vd00, vecpattern1);
      xb_vecNx16U vd11 = IVP_SELNX16(w1, vd00, vecpattern2);
      xb_vecNx16U vd12 = IVP_SELNX16I(0, vd10, IVP_SELI_ROTATE_RIGHT_8);

      vd00 = IVP_SRAINX16(IVP_ADDNX16(vd12, IVP_ADDNX16(vd10, vd11)), 4);

      rdst = OFFSET_PTR_NX16(rdst, 1, dstep, 0);
      IVP_SAVNX16_XP(vd00, a_store, rdst, 2 * ltail);
      IVP_SAPOSNX16_FP(a_store, rdst);
    }
  }
#endif
  // int16_t* restrict pdst1 = (int16_t*)XI_ARRAY_GET_DATA_PTR(dst);
  // printf("%d, %d, %d, %d, %d, %d, %d, %d\n", pdst1[0], pdst1[1], pdst1[2],
  // pdst1[3], pdst1[4], pdst1[5], pdst1[6], pdst1[7]);

  return 0;
}
XI_ERR_TYPE xiPyrDownK_3x3_S16_ref(const xi_pTile src, xi_pArray dst) {
  const int32_t sstride = XI_TILE_GET_PITCH(src);
  const int32_t dstride = XI_ARRAY_GET_PITCH(dst);
  const int32_t width = XI_TILE_GET_WIDTH(src);
  const int32_t height = XI_TILE_GET_HEIGHT(src);
  const int32_t edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  const int32_t edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
  const int32_t dwidth = XI_TILE_GET_WIDTH(dst);
  const int32_t dheight = XI_TILE_GET_HEIGHT(dst);
  int16_t *restrict psrc = (int16_t *)XI_ARRAY_GET_DATA_PTR(src);
  int16_t *restrict pdst = (int16_t *)XI_ARRAY_GET_DATA_PTR(dst);

  // downscale filter
  int kernel[9] = {1, 2, 1, 2, 4, 2, 1, 2, 1};
  int x, y, k;
  for (x = 0; x < (dwidth / 8); x++) {
    for (y = 0; y < dheight; y++) {
      for (k = 0; k < 8; k++) {
        int16_t sum = 0;
        int ksize = 3;
        int N_div = ksize / 2;
        // downsample the pyramid (non separable implementation) with kernel [1
        // 2 1]
        for (int m = -N_div, mm = 0; m <= N_div; m++, mm++)
          for (int n = -N_div, nn = 0; n <= N_div; n++, nn++)
            sum +=
                psrc[(((2 * y) + m) * sstride) + ((2 * x * 8) + (8 * n)) + k] *
                kernel[mm * ksize + nn];

        pdst[(y * dstride) + (8 * x) + k] = sum >> 4;
      }
    }
  }
  // printf("%d, %d, %d, %d, %d, %d, %d, %d\n", pdst[0], pdst[1], pdst[2],
  // pdst[3], pdst[4], pdst[5], pdst[6], pdst[7]);

  return 0;
}
// src - to be upsampled and added to src2
XI_ERR_TYPE xiPyrUpK_S16_ref(const xi_pTile src, const xi_pTile src2,
                             xi_pTile dst) {
  const int32_t sstride = XI_TILE_GET_PITCH(src);
  const int32_t dstride = XI_TILE_GET_PITCH(dst);
  const int32_t swidth = XI_TILE_GET_WIDTH(src);
  const int32_t sheight = XI_TILE_GET_HEIGHT(src);
  const int32_t edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  const int32_t edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
  const int32_t dwidth = XI_TILE_GET_WIDTH(dst);
  const int32_t dheight = XI_TILE_GET_HEIGHT(dst);
  int16_t *restrict psrc = (int16_t *)XI_TILE_GET_DATA_PTR(src);
  int16_t *restrict pdst = (int16_t *)XI_TILE_GET_DATA_PTR(dst);
  int16_t *restrict psrc2 = (int16_t *)XI_TILE_GET_DATA_PTR(src2);
  const int32_t sstride2 = XI_TILE_GET_PITCH(src2);
  int x, y, k;
  // printf("dwidth = %d, dheight = %d, sstride = %d, dstride = %d\n", dwidth,
  // dheight, sstride, dstride);
  for (x = 0; x < (dwidth / 8); x++) {
    for (y = 0; y < dheight; y++) {
      for (k = 0; k < 8; k++) {
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

        upsampleVal = psrc[(y1 * sstride) + (x1 * 8) + k] +
                      psrc[(y2 * sstride) + (x2 * 8) + k] +
                      psrc[(y3 * sstride) + (x3 * 8) + k] +
                      psrc[(y4 * sstride) + (x4 * 8) + k];
        upsampleVal = upsampleVal >> 2; // Q2.8
        pdst[(y * dstride) + (x * 8) + k] =
            psrc2[(y * sstride2) + (8 * x) + k] - upsampleVal;
      }
    }
  }
  return 0;
}
// src - to be upsampled and added to src2
XI_ERR_TYPE xiPyrUpK_S16(const xi_pTile src, const xi_pTile src2,
                         xi_pTile dst) {
  const int32_t sstride = XI_TILE_GET_PITCH(src);
  const int32_t dstride = XI_TILE_GET_PITCH(dst);
  const int32_t swidth = XI_TILE_GET_WIDTH(src);
  const int32_t sheight = XI_TILE_GET_HEIGHT(src);
  const int32_t edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  const int32_t edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
  const int32_t dwidth = XI_TILE_GET_WIDTH(dst);
  const int32_t dheight = XI_TILE_GET_HEIGHT(dst);
  xb_vecNx16 *restrict psrc = (xb_vecNx16 *)XI_TILE_GET_DATA_PTR(src);
  xb_vecNx16 *restrict pdst = (xb_vecNx16 *)XI_TILE_GET_DATA_PTR(dst);
  xb_vecNx16 *restrict psrc2 = (xb_vecNx16 *)XI_TILE_GET_DATA_PTR(src2);
  const int32_t sstride2 = XI_TILE_GET_PITCH(src2);

  int x, y, k;
  xb_vecNx16 *restrict rdst;
  xb_vecNx16 *restrict rsrc, *restrict rsrc2;
  // create patterns
  xb_vecNx16 vecseq = IVP_SEQNX16U();
  xb_vecNx16 vecpattern = IVP_SRLINX16(vecseq, 3);
  xb_vecNx16 v0101 = IVP_ANDNX16(vecpattern, 1);
  xb_vecNx16 v0404 = IVP_SLLINX16(v0101, 2);
  xb_vecNx16 v0011 = IVP_SRLINX16(vecseq, 4);
  xb_vecNx16 v0415 = IVP_ADDNX16(v0404, v0011);
  xb_vecNx16 v0415X8 = IVP_SLLINX16(v0415, 3);
  xb_vecNx16 vecseq1 = IVP_ANDNX16(vecseq, 7);
  xb_vecNx16 vecpattern1 = IVP_ADDNX16(vecseq1, v0415X8);
  xb_vecNx16 vecpattern2 = IVP_ADDNX16(vecpattern1, 16);
  valign a_store = IVP_ZALIGN();
  int32_t j = 0;
  for (; j <= (swidth - XCHAL_IVPN_SIMD_WIDTH); j += XCHAL_IVPN_SIMD_WIDTH) {
    rsrc = OFFSET_PTR_NX16(psrc, 0, sstride, j);

    int32_t dstep = dstride - (2 * XCHAL_IVPN_SIMD_WIDTH);
    int32_t sstep = sstride - XT_MIN(2 * XCHAL_IVPN_SIMD_WIDTH, swidth - j + 8);
    int32_t sstep2 = sstride2 - (2 * XCHAL_IVPN_SIMD_WIDTH);
    valign a_load = IVP_LANX16_PP(rsrc);
    xb_vecNx16 rw0;
    IVP_LANX16_IP(rw0, a_load, rsrc);
    xb_vecNx16 rw1;
    IVP_LAVNX16_XP(rw1, a_load, rsrc,
                   2 * (swidth - j + 8) -
                       2 * XCHAL_IVPN_SIMD_WIDTH); // 8 edge elements minimum
                                                   // needed//2 - bytes
    rdst = OFFSET_PTR_NX16(pdst, 0, dstride, j * 2);
    rsrc2 = OFFSET_PTR_NX16(psrc2, 0, sstride2, j * 2);
    for (int32_t i = 0; i < sheight; i++) {
      xb_vecNx16 vd00 = rw0;
      xb_vecNx16 vd01 = rw1;
      xb_vecNx16 v1 = IVP_SELNX16I(vd01, vd00, IVP_SELI_ROTATE_RIGHT_8);
      v1 = IVP_ADDNX16(vd00, v1);
      v1 = IVP_SRAINX16(v1, 1);
      xb_vecNx16 vres00 = IVP_SELNX16(v1, vd00, vecpattern1);
      xb_vecNx16 vres01 = IVP_SELNX16(v1, vd00, vecpattern2);

      valign a_load2 = IVP_LANX16_PP(rsrc2);
      xb_vecNx16 vecsrc20;
      IVP_LANX16_IP(vecsrc20, a_load2, rsrc2);
      xb_vecNx16 vecsrc21;
      IVP_LANX16_IP(vecsrc21, a_load2, rsrc2);
      rsrc2 = OFFSET_PTR_NX16(rsrc2, 1, sstep2, 0);

      vres00 = IVP_SUBNX16(vecsrc20, vres00);
      vres01 = IVP_SUBNX16(vecsrc21, vres01);

      IVP_SANX16_IP(vres00, a_store, rdst);
      IVP_SANX16_IP(vres01, a_store, rdst);
      IVP_SAPOSNX16_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX16(rdst, 1, dstep, 0);

      rsrc = OFFSET_PTR_NX16(rsrc, 1, sstep, 0);
      valign a_load = IVP_LANX16_PP(rsrc);
      IVP_LANX16_IP(rw0, a_load, rsrc);
      IVP_LAVNX16_XP(rw1, a_load, rsrc,
                     2 * (swidth - j + 8) -
                         2 * XCHAL_IVPN_SIMD_WIDTH); // 8 edge elements minimum
                                                     // needed//2 - bytes

      xb_vecNx16 vtemp10 = IVP_ADDNX16(rw0, vd00);
      xb_vecNx16 vd10 = IVP_SRAINX16(vtemp10, 1);
      xb_vecNx16 vemp11 = IVP_ADDNX16(rw1, vd01);
      v1 = IVP_SELNX16I(vemp11, vtemp10, IVP_SELI_ROTATE_RIGHT_8);
      xb_vecNx16 vd11 = IVP_ADDNX16(vtemp10, v1);
      vd11 = IVP_SRAINX16(vd11, 2);
      xb_vecNx16 vres10 = IVP_SELNX16(vd11, vd10, vecpattern1);
      xb_vecNx16 vres11 = IVP_SELNX16(vd11, vd10, vecpattern2);

      a_load2 = IVP_LANX16_PP(rsrc2);
      IVP_LANX16_IP(vecsrc20, a_load2, rsrc2);
      IVP_LANX16_IP(vecsrc21, a_load2, rsrc2);
      rsrc2 = OFFSET_PTR_NX16(rsrc2, 1, sstep2, 0);

      vres10 = IVP_SUBNX16(vecsrc20, vres10);
      vres11 = IVP_SUBNX16(vecsrc21, vres11);

      IVP_SANX16_IP(vres10, a_store, rdst);
      IVP_SANX16_IP(vres11, a_store, rdst);
      IVP_SAPOSNX16_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX16(rdst, 1, dstep, 0);
    }
  }
  if (j < swidth) {
    rsrc = OFFSET_PTR_NX16(psrc, 0, sstride, j);

    int32_t dstep = dstride - 2 * (swidth - j);
    int32_t sstep = sstride - (swidth - j + 8);
    int32_t sstep2 = sstride2 - 2 * (swidth - j);
    valign a_load = IVP_LANX16_PP(rsrc);
    xb_vecNx16 rw0;
    IVP_LAVNX16_XP(rw0, a_load, rsrc, 2 * (swidth - j + 8));

    rdst = OFFSET_PTR_NX16(pdst, 0, dstride, j * 2);
    rsrc2 = OFFSET_PTR_NX16(psrc2, 0, sstride2, j * 2);
    for (int32_t i = 0; i < sheight; i++) {
      xb_vecNx16 vd00 = rw0;
      xb_vecNx16 v1 = IVP_SELNX16I(0, vd00, IVP_SELI_ROTATE_RIGHT_8);
      v1 = IVP_ADDNX16(vd00, v1);
      v1 = IVP_SRAINX16(v1, 1);
      xb_vecNx16 vres00 = IVP_SELNX16(v1, vd00, vecpattern1);
      xb_vecNx16 vres01 = IVP_SELNX16(v1, vd00, vecpattern2);

      valign a_load2 = IVP_LANX16_PP(rsrc2);
      xb_vecNx16 vecsrc20;
      IVP_LAVNX16_XP(vecsrc20, a_load2, rsrc2, 4 * (swidth - j));
      xb_vecNx16 vecsrc21;
      IVP_LAVNX16_XP(vecsrc21, a_load2, rsrc2,
                     4 * (swidth - j) - (2 * XCHAL_IVPN_SIMD_WIDTH));
      rsrc2 = OFFSET_PTR_NX16(rsrc2, 1, sstep2, 0);

      vres00 = IVP_SUBNX16(vecsrc20, vres00);
      vres01 = IVP_SUBNX16(vecsrc21, vres01);

      IVP_SAVNX16_XP(vres00, a_store, rdst, 4 * (swidth - j));
      IVP_SAVNX16_XP(vres01, a_store, rdst,
                     4 * (swidth - j) - (2 * XCHAL_IVPN_SIMD_WIDTH));
      IVP_SAPOSNX16_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX16(rdst, 1, dstep, 0);

      rsrc = OFFSET_PTR_NX16(rsrc, 1, sstep, 0);
      valign a_load = IVP_LANX16_PP(rsrc);
      IVP_LAVNX16_XP(rw0, a_load, rsrc, 2 * (swidth - j + 8));

      xb_vecNx16 vtemp10 = IVP_ADDNX16(rw0, vd00);
      xb_vecNx16 vd10 = IVP_SRAINX16(vtemp10, 1);
      v1 = IVP_SELNX16I(0, vtemp10, IVP_SELI_ROTATE_RIGHT_8);
      xb_vecNx16 vd11 = IVP_ADDNX16(vtemp10, v1);
      vd11 = IVP_SRAINX16(vd11, 2);
      xb_vecNx16 vres10 = IVP_SELNX16(vd11, vd10, vecpattern1);
      xb_vecNx16 vres11 = IVP_SELNX16(vd11, vd10, vecpattern2);

      a_load2 = IVP_LANX16_PP(rsrc2);
      IVP_LAVNX16_XP(vecsrc20, a_load2, rsrc2, 4 * (swidth - j));
      IVP_LAVNX16_XP(vecsrc21, a_load2, rsrc2,
                     4 * (swidth - j) - (2 * XCHAL_IVPN_SIMD_WIDTH));
      rsrc2 = OFFSET_PTR_NX16(rsrc2, 1, sstep2, 0);

      vres10 = IVP_SUBNX16(vecsrc20, vres10);
      vres11 = IVP_SUBNX16(vecsrc21, vres11);

      IVP_SAVNX16_XP(vres10, a_store, rdst, 4 * (swidth - j));
      IVP_SAVNX16_XP(vres11, a_store, rdst,
                     4 * (swidth - j) - (2 * XCHAL_IVPN_SIMD_WIDTH));
      IVP_SAPOSNX16_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX16(rdst, 1, dstep, 0);
    }
  }
  // int16_t *pdst1 = (int16_t *)XI_TILE_GET_DATA_PTR(dst);
  // printf("%d, %d, %d, %d, %d, %d, %d, %d\n", pdst1[8*0], pdst1[8*1],
  // pdst1[8*2], pdst1[8*3], pdst1[8*4], pdst1[8*5], pdst1[8*6], pdst1[8*7]);
  return 0;
}
#define CLIP(value, min, max) XT_MAX(min, XT_MIN(max, value))
XI_ERR_TYPE xiFusePyr_S16_ref(const xi_pTile src, const xi_pTile srcgray,
                              xi_pTile dst) {
  const int32_t sstride = XI_TILE_GET_PITCH(src);
  const int32_t sgstride = XI_TILE_GET_PITCH(srcgray);
  const int32_t dstride = XI_TILE_GET_PITCH(dst);
  const int32_t swidth = XI_TILE_GET_WIDTH(src);
  const int32_t sheight = XI_TILE_GET_HEIGHT(src);
  const int32_t edge_width =
      0; // XI_TILE_GET_EDGE_WIDTH(dst);//no need to do for edges
  const int32_t edge_height =
      0; // XI_TILE_GET_EDGE_HEIGHT(dst);//no need to do for edges
  const int32_t dwidth = XI_TILE_GET_WIDTH(dst);
  const int32_t dheight = XI_TILE_GET_HEIGHT(dst);
  int16_t *restrict psrc = (int16_t *)XI_TILE_GET_DATA_PTR(src);
  uint8_t *restrict psrcgray = (uint8_t *)XI_TILE_GET_DATA_PTR(srcgray);
  int16_t *restrict pdst = (int16_t *)XI_TILE_GET_DATA_PTR(dst);
  int levels = 8;
  int x, y;
  // printf("dwidth = %d, dheight = %d, edge_width = %d, edge_height = %d\n",
  // dwidth, dheight, edge_width, edge_height); printf("sstride = %d, sgstride =
  // %d, dstride = %d\n", sstride, sgstride, dstride);

  for (x = -edge_width; x < (dwidth + edge_width); x++) {
    for (y = -edge_height; y < (dheight + edge_height); y++) {
      // Split input pyramid value into integer and floating parts
      // float level = (pDataGPyramid[(y * GPyramidXStride) + x] *
      // (float)(levels-1));
      int16_t level = (int16_t)(
          psrcgray[(y * sgstride) + x] *
          (levels -
           1)); // pDataGPyramid//Q8 in 8 bits unsigned//level//Q3.8 in 16 bits
      int li = CLIP((level >> 8), 0, levels - 2);
      // float lf = level - (float)li;
      int16_t lf = (uint8_t)(
          level - ((int16_t)li << 8)); // Q0.8 in 16 bits (unsigned number)
      int32_t a = ((((int16_t)1 << 8) - lf) *
                   psrc[(y * sstride) + (8 * x) + li]); // Q3.8 + Q0.8//Q3.16
      int32_t b =
          (lf * psrc[(y * sstride) + (8 * x) + (li + 1)]); // Q3.8 + Q0.8//Q3.16
      pdst[(y * dstride) + x] =
          (int16_t)((a + b) >> 8); // pDataOutLPyramid//Q3.8
    }
  }
  return 0;
}

XI_ERR_TYPE xiFusePyr_S16(const xi_pTile src, const xi_pTile srcgray,
                          xi_pTile dst) {
  const int32_t sstride = XI_TILE_GET_PITCH(src);
  const int32_t sgstride = XI_TILE_GET_PITCH(srcgray);
  const int32_t dstride = XI_TILE_GET_PITCH(dst);
  const int32_t swidth = XI_TILE_GET_WIDTH(src);
  const int32_t sheight = XI_TILE_GET_HEIGHT(src);
  const int32_t edge_width = XI_TILE_GET_EDGE_WIDTH(dst);
  const int32_t edge_height = XI_TILE_GET_EDGE_HEIGHT(dst);
  const int32_t dwidth = XI_TILE_GET_WIDTH(dst);
  const int32_t dheight = XI_TILE_GET_HEIGHT(dst);
  int16_t *restrict psrc = (int16_t *)XI_TILE_GET_DATA_PTR(src);
  uint8_t *restrict psrcgray = (uint8_t *)XI_TILE_GET_DATA_PTR(srcgray);
  int16_t *restrict pdst = (int16_t *)XI_TILE_GET_DATA_PTR(dst);
  xb_vecNx8U *restrict rsrcgray;
  xb_vecNx16 *restrict rdst;
  int16_t *restrict rsrc;
  xb_vecNx16 vecseq = IVP_SEQNX16U();
  xb_vecNx16 vecseqX8 = IVP_SLLINX16(vecseq, 3);
  xb_gsr vg1, vg2;
  valign a_store = IVP_ZALIGN();
  int32_t j = 0;

  for (; j <= (dwidth - XCHAL_IVPN_SIMD_WIDTH); j += XCHAL_IVPN_SIMD_WIDTH) {
    rsrc = (int16_t *)OFFSET_PTR_NX16(psrc, 0, sstride, (j * 8));
    rsrcgray = OFFSET_PTR_NX8U(psrcgray, 0, sgstride, j);
    rdst = OFFSET_PTR_NX16(pdst, 0, dstride, j);

    int32_t dstep = dstride - XCHAL_IVPN_SIMD_WIDTH;
    int32_t sgstep = sgstride - XCHAL_IVPN_SIMD_WIDTH;
    int32_t sstep = sstride; // - (XCHAL_IVPN_SIMD_WIDTH * 8);
    for (int32_t i = 0; i < dheight; i++) {
      valign a_load = IVP_LANX8U_PP(rsrcgray);
      xb_vecNx16 vecg;
      IVP_LANX8U_IP(vecg, a_load, rsrcgray);
      rsrcgray = OFFSET_PTR_NX8U(rsrcgray, 1, sgstep, 0);
      xb_vecNx16 veclevel = IVP_MULNX16PACKL(vecg, 7);
      xb_vecNx16 vecli, veclf1, veclf2;
      vecli = IVP_SRAINX16(veclevel, 8);
      // vecli is +ve always
      vecli = IVP_MINNX16(vecli, 6);
      veclf1 = IVP_SUBNX16(veclevel, IVP_SLLINX16(vecli, 8));
      veclf2 = IVP_SUBNX16((1 << 8), veclf1);

      xb_vecNx16 vec1 = IVP_ADDNX16(vecseqX8, vecli);
      xb_vecNx16 vecoffset1 = IVP_SLLINX16(vec1, 1);
      xb_vecNx16 vec2 = IVP_ADDNX16(vec1, 1);
      xb_vecNx16 vecoffset2 = IVP_SLLINX16(vec2, 1);
      vg1 = IVP_GATHERANX16(rsrc, vecoffset1);
      vec1 = IVP_GATHERDNX16(vg1);
      vg2 = IVP_GATHERANX16(rsrc, vecoffset2);
      vec2 = IVP_GATHERDNX16(vg2);
      rsrc = (int16_t *)OFFSET_PTR_NX16(rsrc, 1, sstep, 0);
      xb_vecNx48 w = IVP_MULPNX16(vec1, veclf2, vec2, veclf1);
      xb_vecNx16 vecout = IVP_PACKVRNRNX48(w, 8);

      IVP_SANX16_IP(vecout, a_store, rdst);
      IVP_SAPOSNX16_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX16(rdst, 1, dstep, 0);
    }
  }
  if (j < dwidth) {
    rsrc = (int16_t *)OFFSET_PTR_NX16(psrc, 0, sstride, (j * 8));
    rsrcgray = OFFSET_PTR_NX8U(psrcgray, 0, sgstride, j);
    rdst = OFFSET_PTR_NX16(pdst, 0, dstride, j);
    vboolN vb = IVP_LTNX16(vecseq, (dwidth - j));
    int32_t dstep = dstride - (dwidth - j);
    int32_t sgstep = sgstride - (dwidth - j);
    int32_t sstep = sstride; // - ((dwidth - j) * 8);

    for (int32_t i = 0; i < dheight; i++) {
      valign a_load = IVP_LANX8U_PP(rsrcgray);
      xb_vecNx16 vecg;
      IVP_LAVNX8U_XP(vecg, a_load, rsrcgray, dwidth - j);
      rsrcgray = OFFSET_PTR_NX8U(rsrcgray, 1, sgstep, 0);

      xb_vecNx16 veclevel = IVP_MULNX16PACKL(vecg, 7);
      xb_vecNx16 vecli, veclf1, veclf2;
      vecli = IVP_SRAINX16(veclevel, 8);
      // vecli is +ve always
      vecli = IVP_MINNX16(vecli, 6);
      veclf1 = IVP_SUBNX16(veclevel, IVP_SLLINX16(vecli, 8));
      veclf2 = IVP_SUBNX16((1 << 8), veclf1);

      xb_vecNx16 vec1 = IVP_ADDNX16(vecseqX8, vecli);
      xb_vecNx16 vecoffset1 = IVP_SLLINX16(vec1, 1);
      xb_vecNx16 vec2 = IVP_ADDNX16(vec1, 1);
      xb_vecNx16 vecoffset2 = IVP_SLLINX16(vec2, 1);

      vg1 = IVP_GATHERANX16T(rsrc, vecoffset1, vb);
      vec1 = IVP_GATHERDNX16(vg1);
      vg2 = IVP_GATHERANX16T(rsrc, vecoffset2, vb);
      vec2 = IVP_GATHERDNX16(vg2);
      rsrc = (int16_t *)OFFSET_PTR_NX16(rsrc, 1, sstep, 0);

      xb_vecNx48 w = IVP_MULPNX16(vec1, veclf2, vec2, veclf1);
      xb_vecNx16 vecout = IVP_PACKVRNRNX48(w, 8);

      IVP_SAVNX16_XP(vecout, a_store, rdst, 2 * (dwidth - j));
      IVP_SAPOSNX16_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX16(rdst, 1, dstep, 0);
    }
  }

  return 0;
}

// src1 - to be upsampled
XI_ERR_TYPE xiPyrUp_S16S16_ref(const xi_pTile src1, const xi_pTile src0,
                               xi_pTile dst) {
  const int32_t dwidth = XI_TILE_GET_WIDTH(dst);
  const int32_t dheight = XI_TILE_GET_HEIGHT(dst);
  const int32_t s1stride = XI_TILE_GET_PITCH(src1);
  const int32_t s0stride = XI_TILE_GET_PITCH(src0);
  const int32_t dstride = XI_TILE_GET_PITCH(dst);
  // const int32_t edge_width = XI_TILE_GET_EDGE_WIDTH(dst);
  // const int32_t edge_height = XI_TILE_GET_EDGE_HEIGHT(dst);

  int16_t *restrict psrc1 = (int16_t *)XI_TILE_GET_DATA_PTR(src1);
  int16_t *restrict psrc0 = (int16_t *)XI_TILE_GET_DATA_PTR(src0);
  int16_t *restrict pdst = (int16_t *)XI_TILE_GET_DATA_PTR(dst);
  int x, y;

  for (x = 0; x < dwidth; x++) {
    for (y = 0; y < dheight; y++) {
      int16_t upsampleVal, num;
      int x1, y1, x2, y2, x3, y3, x4, y4;
      x1 = (x / 2);
      y1 = (y / 2);
      x2 = (x / 2) + (x % 2);
      y2 = (y / 2);
      x3 = (x / 2);
      y3 = (y / 2) + (y % 2);
      x4 = (x / 2) + (x % 2);
      y4 = (y / 2) + (y % 2);
      upsampleVal = psrc1[(y1 * s1stride) + x1] + psrc1[(y2 * s1stride) + x2] +
                    psrc1[(y3 * s1stride) + x3] + psrc1[(y4 * s1stride) + x4];
      upsampleVal = upsampleVal >> 2; // Q8

      pdst[(y * dstride) + x] = psrc0[(y * s0stride) + x] + upsampleVal;
    }
  }
  return 0;
}
// src1 - to be upsampled
XI_ERR_TYPE xiPyrUp_S16S16(const xi_pTile src1, const xi_pTile src0,
                           xi_pTile dst) {
  const int32_t dwidth = XI_TILE_GET_WIDTH(dst);
  const int32_t dheight = XI_TILE_GET_HEIGHT(dst);
  const int32_t s1width = XI_TILE_GET_WIDTH(src1);
  const int32_t s1height = XI_TILE_GET_HEIGHT(src1);
  const int32_t s1stride = XI_TILE_GET_PITCH(src1);
  const int32_t s0stride = XI_TILE_GET_PITCH(src0);
  const int32_t dstride = XI_TILE_GET_PITCH(dst);
  // const int32_t edge_width = XI_TILE_GET_EDGE_WIDTH(dst);
  // const int32_t edge_height = XI_TILE_GET_EDGE_HEIGHT(dst);

  xb_vecNx16 *restrict psrc1 = (xb_vecNx16 *)XI_TILE_GET_DATA_PTR(src1);
  int16_t *restrict psrc0 = (int16_t *)XI_TILE_GET_DATA_PTR(src0);
  int16_t *restrict pdst = (int16_t *)XI_TILE_GET_DATA_PTR(dst);
  xb_vecNx16 *restrict rsrc1;
  xb_vecNx16 *restrict rdst;
  xb_vecNx16 *restrict rsrc0;
  valign a_store = IVP_ZALIGN();
  int32_t j = 0;
  for (; j <= (s1width - XCHAL_IVPN_SIMD_WIDTH); j += XCHAL_IVPN_SIMD_WIDTH) {
    rsrc1 = OFFSET_PTR_NX16(psrc1, 0, s1stride, j);

    int32_t dstep = dstride - (2 * XCHAL_IVPN_SIMD_WIDTH);
    int32_t s1step =
        s1stride - XT_MIN(2 * XCHAL_IVPN_SIMD_WIDTH, (s1width - j + 1));
    int32_t s0step = s0stride - (2 * XCHAL_IVPN_SIMD_WIDTH);
    valign a_load = IVP_LANX16_PP(rsrc1);
    xb_vecNx16 rw0;
    IVP_LANX16_IP(rw0, a_load, rsrc1);
    xb_vecNx16 rw1;
    IVP_LAVNX16_XP(rw1, a_load, rsrc1,
                   2 * (s1width - j + 1) -
                       2 * XCHAL_IVPN_SIMD_WIDTH); // minimum 1 edge element is
                                                   // needed//2 - bytes

    rdst = OFFSET_PTR_NX16(pdst, 0, dstride, j * 2);
    rsrc0 = OFFSET_PTR_NX16(psrc0, 0, s0stride, j * 2);
    for (int32_t i = 0; i < s1height; i++) {
      xb_vecNx16 vd00 = rw0;
      xb_vecNx16 vd01 = rw1;
      xb_vecNx16 v1 = IVP_SELNX16I(vd01, vd00, IVP_SELI_ROTATE_RIGHT_1);
      v1 = IVP_ADDNX16(vd00, v1);
      v1 = IVP_SRAINX16(v1, 1);
      xb_vecNx16 vres00 = IVP_SELNX16I(v1, vd00, IVP_SELI_16B_INTERLEAVE_1_LO);
      xb_vecNx16 vres01 = IVP_SELNX16I(v1, vd00, IVP_SELI_16B_INTERLEAVE_1_HI);

      valign a_load0 = IVP_LANX16_PP(rsrc0);
      xb_vecNx16 vecsrc00;
      IVP_LANX16_IP(vecsrc00, a_load0, rsrc0);
      xb_vecNx16 vecsrc01;
      IVP_LANX16_IP(vecsrc01, a_load0, rsrc0);
      rsrc0 = OFFSET_PTR_NX16(rsrc0, 1, s0step, 0);

      vres00 = IVP_ADDNX16(vecsrc00, vres00);
      vres01 = IVP_ADDNX16(vecsrc01, vres01);

      IVP_SANX16_IP(vres00, a_store, rdst);
      IVP_SANX16_IP(vres01, a_store, rdst);
      IVP_SAPOSNX16_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX16(rdst, 1, dstep, 0);

      rsrc1 = OFFSET_PTR_NX16(rsrc1, 1, s1step, 0);
      valign a_load = IVP_LANX16_PP(rsrc1);
      IVP_LANX16_IP(rw0, a_load, rsrc1);
      IVP_LAVNX16_XP(rw1, a_load, rsrc1,
                     2 * (s1width - j + 1) -
                         2 * XCHAL_IVPN_SIMD_WIDTH); // minimum 1 edge element
                                                     // is needed//2 - bytes

      xb_vecNx16 vtemp10 = IVP_ADDNX16(rw0, vd00);
      xb_vecNx16 vd10 = IVP_SRAINX16(vtemp10, 1);
      xb_vecNx16 vemp11 = IVP_ADDNX16(rw1, vd01);
      v1 = IVP_SELNX16I(vemp11, vtemp10, IVP_SELI_ROTATE_RIGHT_1);
      xb_vecNx16 vd11 = IVP_ADDNX16(vtemp10, v1);
      vd11 = IVP_SRAINX16(vd11, 2);
      xb_vecNx16 vres10 =
          IVP_SELNX16I(vd11, vd10, IVP_SELI_16B_INTERLEAVE_1_LO);
      xb_vecNx16 vres11 =
          IVP_SELNX16I(vd11, vd10, IVP_SELI_16B_INTERLEAVE_1_HI);

      a_load0 = IVP_LANX16_PP(rsrc0);
      IVP_LANX16_IP(vecsrc00, a_load0, rsrc0);
      IVP_LANX16_IP(vecsrc01, a_load0, rsrc0);
      rsrc0 = OFFSET_PTR_NX16(rsrc0, 1, s0step, 0);

      vres10 = IVP_ADDNX16(vecsrc00, vres10);
      vres11 = IVP_ADDNX16(vecsrc01, vres11);

      IVP_SANX16_IP(vres10, a_store, rdst);
      IVP_SANX16_IP(vres11, a_store, rdst);
      IVP_SAPOSNX16_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX16(rdst, 1, dstep, 0);
    }
  }
  if (j < s1width) {
    rsrc1 = OFFSET_PTR_NX16(psrc1, 0, s1stride, j);

    int32_t dstep = dstride - (2 * (s1width - j));
    int32_t s1step = s1stride - (s1width - j + 1);
    int32_t s0step = s0stride - (2 * (s1width - j));

    valign a_load = IVP_LANX16_PP(rsrc1);
    xb_vecNx16 rw0;
    IVP_LAVNX16_XP(rw0, a_load, rsrc1, 2 * (s1width - j + 1));

    rdst = OFFSET_PTR_NX16(pdst, 0, dstride, j * 2);
    rsrc0 = OFFSET_PTR_NX16(psrc0, 0, s0stride, j * 2);
    for (int32_t i = 0; i < s1height; i++) {
      xb_vecNx16 vd00 = rw0;
      xb_vecNx16 v1 = IVP_SELNX16I(0, vd00, IVP_SELI_ROTATE_RIGHT_1);
      v1 = IVP_ADDNX16(vd00, v1);
      v1 = IVP_SRAINX16(v1, 1);
      xb_vecNx16 vres00 = IVP_SELNX16I(v1, vd00, IVP_SELI_16B_INTERLEAVE_1_LO);
      xb_vecNx16 vres01 = IVP_SELNX16I(v1, vd00, IVP_SELI_16B_INTERLEAVE_1_HI);

      valign a_load0 = IVP_LANX16_PP(rsrc0);
      xb_vecNx16 vecsrc00;
      IVP_LAVNX16_XP(vecsrc00, a_load0, rsrc0, 4 * (s1width - j));
      xb_vecNx16 vecsrc01;
      IVP_LAVNX16_XP(vecsrc01, a_load0, rsrc0,
                     4 * (s1width - j) - (2 * XCHAL_IVPN_SIMD_WIDTH));
      rsrc0 = OFFSET_PTR_NX16(rsrc0, 1, s0step, 0);

      vres00 = IVP_ADDNX16(vecsrc00, vres00);
      vres01 = IVP_ADDNX16(vecsrc01, vres01);

      IVP_SAVNX16_XP(vres00, a_store, rdst, 4 * (s1width - j));
      IVP_SAVNX16_XP(vres01, a_store, rdst,
                     4 * (s1width - j) - (2 * XCHAL_IVPN_SIMD_WIDTH));
      IVP_SAPOSNX16_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX16(rdst, 1, dstep, 0);

      rsrc1 = OFFSET_PTR_NX16(rsrc1, 1, s1step, 0);
      valign a_load = IVP_LANX16_PP(rsrc1);
      IVP_LAVNX16_XP(rw0, a_load, rsrc1, 2 * (s1width - j + 1));

      xb_vecNx16 vtemp10 = IVP_ADDNX16(rw0, vd00);
      xb_vecNx16 vd10 = IVP_SRAINX16(vtemp10, 1);
      v1 = IVP_SELNX16I(0, vtemp10, IVP_SELI_ROTATE_RIGHT_1);
      xb_vecNx16 vd11 = IVP_ADDNX16(vtemp10, v1);
      vd11 = IVP_SRAINX16(vd11, 2);
      xb_vecNx16 vres10 =
          IVP_SELNX16I(vd11, vd10, IVP_SELI_16B_INTERLEAVE_1_LO);
      xb_vecNx16 vres11 =
          IVP_SELNX16I(vd11, vd10, IVP_SELI_16B_INTERLEAVE_1_HI);

      a_load0 = IVP_LANX16_PP(rsrc0);
      IVP_LAVNX16_XP(vecsrc00, a_load0, rsrc0, 4 * (s1width - j));
      IVP_LAVNX16_XP(vecsrc01, a_load0, rsrc0,
                     4 * (s1width - j) - (2 * XCHAL_IVPN_SIMD_WIDTH));
      rsrc0 = OFFSET_PTR_NX16(rsrc0, 1, s0step, 0);

      vres10 = IVP_ADDNX16(vecsrc00, vres10);
      vres11 = IVP_ADDNX16(vecsrc01, vres11);

      IVP_SAVNX16_XP(vres10, a_store, rdst, 4 * (s1width - j));
      IVP_SAVNX16_XP(vres11, a_store, rdst,
                     4 * (s1width - j) - (2 * XCHAL_IVPN_SIMD_WIDTH));
      IVP_SAPOSNX16_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX16(rdst, 1, dstep, 0);
    }
  }
  return 0;
}
// src1 - to be upsampled
XI_ERR_TYPE xiPyrUp_S16U8_ref(const xi_pTile src1, const xi_pTile src0,
                              xi_pTile dst) {
  const int32_t dwidth = XI_TILE_GET_WIDTH(dst);
  const int32_t dheight = XI_TILE_GET_HEIGHT(dst);
  const int32_t s1stride = XI_TILE_GET_PITCH(src1);
  const int32_t s0stride = XI_TILE_GET_PITCH(src0);
  const int32_t dstride = XI_TILE_GET_PITCH(dst);
  // const int32_t edge_width = XI_TILE_GET_EDGE_WIDTH(dst);
  // const int32_t edge_height = XI_TILE_GET_EDGE_HEIGHT(dst);

  int16_t *restrict psrc1 = (int16_t *)XI_TILE_GET_DATA_PTR(src1);
  int16_t *restrict psrc0 = (int16_t *)XI_TILE_GET_DATA_PTR(src0);
  uint8_t *restrict pdst = (uint8_t *)XI_TILE_GET_DATA_PTR(dst);
  int x, y;

  for (x = 0; x < dwidth; x++) {
    for (y = 0; y < dheight; y++) {
      int16_t upsampleVal, num;
      int x1, y1, x2, y2, x3, y3, x4, y4;
      x1 = (x / 2);
      y1 = (y / 2);
      x2 = (x / 2) + (x % 2);
      y2 = (y / 2);
      x3 = (x / 2);
      y3 = (y / 2) + (y % 2);
      x4 = (x / 2) + (x % 2);
      y4 = (y / 2) + (y % 2);
      upsampleVal = psrc1[(y1 * s1stride) + x1] + psrc1[(y2 * s1stride) + x2] +
                    psrc1[(y3 * s1stride) + x3] + psrc1[(y4 * s1stride) + x4];
      upsampleVal = upsampleVal >> 2; // Q8

      pdst[(y * dstride) + x] =
          (uint8_t)CLIP(psrc0[(y * s0stride) + x] + upsampleVal, 0, 255);
    }
  }
  return 0;
}
// src1 - to be upsampled
XI_ERR_TYPE xiPyrUp_S16U8(const xi_pTile src1, const xi_pTile src0,
                          xi_pTile dst) {
  const int32_t dwidth = XI_TILE_GET_WIDTH(dst);
  const int32_t dheight = XI_TILE_GET_HEIGHT(dst);
  const int32_t s1width = XI_TILE_GET_WIDTH(src1);
  const int32_t s1height = XI_TILE_GET_HEIGHT(src1);
  const int32_t s1stride = XI_TILE_GET_PITCH(src1);
  const int32_t s0stride = XI_TILE_GET_PITCH(src0);
  const int32_t dstride = XI_TILE_GET_PITCH(dst);
  // const int32_t edge_width = XI_TILE_GET_EDGE_WIDTH(dst);
  // const int32_t edge_height = XI_TILE_GET_EDGE_HEIGHT(dst);

  xb_vecNx16 *restrict psrc1 = (xb_vecNx16 *)XI_TILE_GET_DATA_PTR(src1);
  int16_t *restrict psrc0 = (int16_t *)XI_TILE_GET_DATA_PTR(src0);
  uint8_t *restrict pdst = (uint8_t *)XI_TILE_GET_DATA_PTR(dst);
  xb_vecNx16 *restrict rsrc1;
  xb_vecNx8U *restrict rdst;
  xb_vecNx16 *restrict rsrc0;
  valign a_store = IVP_ZALIGN();
  int32_t j = 0;
  for (; j <= (s1width - XCHAL_IVPN_SIMD_WIDTH); j += XCHAL_IVPN_SIMD_WIDTH) {
    rsrc1 = OFFSET_PTR_NX16(psrc1, 0, s1stride, j);

    int32_t dstep = dstride - (2 * XCHAL_IVPN_SIMD_WIDTH);
    int32_t s1step =
        s1stride - XT_MIN(2 * XCHAL_IVPN_SIMD_WIDTH, (s1width - j + 1));
    int32_t s0step = s0stride - (2 * XCHAL_IVPN_SIMD_WIDTH);
    valign a_load = IVP_LANX16_PP(rsrc1);
    xb_vecNx16 rw0;
    IVP_LANX16_IP(rw0, a_load, rsrc1);
    xb_vecNx16 rw1;
    IVP_LAVNX16_XP(rw1, a_load, rsrc1,
                   2 * (s1width - j + 1) -
                       2 * XCHAL_IVPN_SIMD_WIDTH); // minimum 1 edge element is
                                                   // needed//2 - bytes

    rdst = OFFSET_PTR_NX8U(pdst, 0, dstride, j * 2);
    rsrc0 = OFFSET_PTR_NX16(psrc0, 0, s0stride, j * 2);
    for (int32_t i = 0; i < s1height; i++) {
      xb_vecNx16 vd00 = rw0;
      xb_vecNx16 vd01 = rw1;
      xb_vecNx16 v1 = IVP_SELNX16I(vd01, vd00, IVP_SELI_ROTATE_RIGHT_1);
      v1 = IVP_ADDNX16(vd00, v1);
      v1 = IVP_SRAINX16(v1, 1);
      xb_vecNx16 vres00 = IVP_SELNX16I(v1, vd00, IVP_SELI_16B_INTERLEAVE_1_LO);
      xb_vecNx16 vres01 = IVP_SELNX16I(v1, vd00, IVP_SELI_16B_INTERLEAVE_1_HI);

      valign a_load0 = IVP_LANX16_PP(rsrc0);
      xb_vecNx16 vecsrc00;
      IVP_LANX16_IP(vecsrc00, a_load0, rsrc0);
      xb_vecNx16 vecsrc01;
      IVP_LANX16_IP(vecsrc01, a_load0, rsrc0);
      rsrc0 = OFFSET_PTR_NX16(rsrc0, 1, s0step, 0);

      vres00 = IVP_ADDNX16(vecsrc00, vres00);
      vres01 = IVP_ADDNX16(vecsrc01, vres01);

      vres00 = IVP_MAXNX16(vres00, 0);
      vres00 = IVP_MINNX16(vres00, 255);
      vres01 = IVP_MAXNX16(vres01, 0);
      vres01 = IVP_MINNX16(vres01, 255);

      IVP_SANX8U_IP(vres00, a_store, rdst);
      IVP_SANX8U_IP(vres01, a_store, rdst);
      IVP_SAPOSNX8U_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX8U(rdst, 1, dstep, 0);

      rsrc1 = OFFSET_PTR_NX16(rsrc1, 1, s1step, 0);
      valign a_load = IVP_LANX16_PP(rsrc1);
      IVP_LANX16_IP(rw0, a_load, rsrc1);
      IVP_LAVNX16_XP(rw1, a_load, rsrc1,
                     2 * (s1width - j + 1) -
                         2 * XCHAL_IVPN_SIMD_WIDTH); // minimum 1 edge element
                                                     // is needed//2 - bytes

      xb_vecNx16 vtemp10 = IVP_ADDNX16(rw0, vd00);
      xb_vecNx16 vd10 = IVP_SRAINX16(vtemp10, 1);
      xb_vecNx16 vemp11 = IVP_ADDNX16(rw1, vd01);
      v1 = IVP_SELNX16I(vemp11, vtemp10, IVP_SELI_ROTATE_RIGHT_1);
      xb_vecNx16 vd11 = IVP_ADDNX16(vtemp10, v1);
      vd11 = IVP_SRAINX16(vd11, 2);
      xb_vecNx16 vres10 =
          IVP_SELNX16I(vd11, vd10, IVP_SELI_16B_INTERLEAVE_1_LO);
      xb_vecNx16 vres11 =
          IVP_SELNX16I(vd11, vd10, IVP_SELI_16B_INTERLEAVE_1_HI);

      a_load0 = IVP_LANX16_PP(rsrc0);
      IVP_LANX16_IP(vecsrc00, a_load0, rsrc0);
      IVP_LANX16_IP(vecsrc01, a_load0, rsrc0);
      rsrc0 = OFFSET_PTR_NX16(rsrc0, 1, s0step, 0);

      vres10 = IVP_ADDNX16(vecsrc00, vres10);
      vres11 = IVP_ADDNX16(vecsrc01, vres11);

      vres10 = IVP_MAXNX16(vres10, 0);
      vres10 = IVP_MINNX16(vres10, 255);
      vres11 = IVP_MAXNX16(vres11, 0);
      vres11 = IVP_MINNX16(vres11, 255);

      IVP_SANX8U_IP(vres10, a_store, rdst);
      IVP_SANX8U_IP(vres11, a_store, rdst);
      IVP_SAPOSNX8U_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX8U(rdst, 1, dstep, 0);
    }
  }
  if (j < s1width) {
    rsrc1 = OFFSET_PTR_NX16(psrc1, 0, s1stride, j);

    int32_t dstep = dstride - (2 * (s1width - j));
    int32_t s1step = s1stride - (s1width - j + 1);
    int32_t s0step = s0stride - (2 * (s1width - j));

    valign a_load = IVP_LANX16_PP(rsrc1);
    xb_vecNx16 rw0;
    IVP_LAVNX16_XP(rw0, a_load, rsrc1, 2 * (s1width - j + 1));

    rdst = OFFSET_PTR_NX8U(pdst, 0, dstride, j * 2);
    rsrc0 = OFFSET_PTR_NX16(psrc0, 0, s0stride, j * 2);
    for (int32_t i = 0; i < s1height; i++) {
      xb_vecNx16 vd00 = rw0;
      xb_vecNx16 v1 = IVP_SELNX16I(0, vd00, IVP_SELI_ROTATE_RIGHT_1);
      v1 = IVP_ADDNX16(vd00, v1);
      v1 = IVP_SRAINX16(v1, 1);
      xb_vecNx16 vres00 = IVP_SELNX16I(v1, vd00, IVP_SELI_16B_INTERLEAVE_1_LO);
      xb_vecNx16 vres01 = IVP_SELNX16I(v1, vd00, IVP_SELI_16B_INTERLEAVE_1_HI);

      valign a_load0 = IVP_LANX16_PP(rsrc0);
      xb_vecNx16 vecsrc00;
      IVP_LAVNX16_XP(vecsrc00, a_load0, rsrc0, 4 * (s1width - j));
      xb_vecNx16 vecsrc01;
      IVP_LAVNX16_XP(vecsrc01, a_load0, rsrc0,
                     4 * (s1width - j) - (2 * XCHAL_IVPN_SIMD_WIDTH));
      rsrc0 = OFFSET_PTR_NX16(rsrc0, 1, s0step, 0);

      vres00 = IVP_ADDNX16(vecsrc00, vres00);
      vres01 = IVP_ADDNX16(vecsrc01, vres01);

      vres00 = IVP_MAXNX16(vres00, 0);
      vres00 = IVP_MINNX16(vres00, 255);
      vres01 = IVP_MAXNX16(vres01, 0);
      vres01 = IVP_MINNX16(vres01, 255);

      IVP_SAVNX8U_XP(vres00, a_store, rdst, 2 * (s1width - j));
      IVP_SAVNX8U_XP(vres01, a_store, rdst,
                     2 * (s1width - j) - (2 * XCHAL_IVPN_SIMD_WIDTH));
      IVP_SAPOSNX8U_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX8U(rdst, 1, dstep, 0);

      rsrc1 = OFFSET_PTR_NX16(rsrc1, 1, s1step, 0);
      valign a_load = IVP_LANX16_PP(rsrc1);
      IVP_LAVNX16_XP(rw0, a_load, rsrc1, 2 * (s1width - j + 1));

      xb_vecNx16 vtemp10 = IVP_ADDNX16(rw0, vd00);
      xb_vecNx16 vd10 = IVP_SRAINX16(vtemp10, 1);
      v1 = IVP_SELNX16I(0, vtemp10, IVP_SELI_ROTATE_RIGHT_1);
      xb_vecNx16 vd11 = IVP_ADDNX16(vtemp10, v1);
      vd11 = IVP_SRAINX16(vd11, 2);
      xb_vecNx16 vres10 =
          IVP_SELNX16I(vd11, vd10, IVP_SELI_16B_INTERLEAVE_1_LO);
      xb_vecNx16 vres11 =
          IVP_SELNX16I(vd11, vd10, IVP_SELI_16B_INTERLEAVE_1_HI);

      a_load0 = IVP_LANX16_PP(rsrc0);
      IVP_LAVNX16_XP(vecsrc00, a_load0, rsrc0, 4 * (s1width - j));
      IVP_LAVNX16_XP(vecsrc01, a_load0, rsrc0,
                     4 * (s1width - j) - (2 * XCHAL_IVPN_SIMD_WIDTH));
      rsrc0 = OFFSET_PTR_NX16(rsrc0, 1, s0step, 0);

      vres10 = IVP_ADDNX16(vecsrc00, vres10);
      vres11 = IVP_ADDNX16(vecsrc01, vres11);

      vres10 = IVP_MAXNX16(vres10, 0);
      vres10 = IVP_MINNX16(vres10, 255);
      vres11 = IVP_MAXNX16(vres11, 0);
      vres11 = IVP_MINNX16(vres11, 255);

      IVP_SAVNX8U_XP(vres10, a_store, rdst, 2 * (s1width - j));
      IVP_SAVNX8U_XP(vres11, a_store, rdst,
                     2 * (s1width - j) - (2 * XCHAL_IVPN_SIMD_WIDTH));
      IVP_SAPOSNX8U_FP(a_store, rdst);
      rdst = OFFSET_PTR_NX8U(rdst, 1, dstep, 0);
    }
  }
  return 0;
}

XI_ERR_TYPE xiEdgeRepeatU8_ref(xi_pTile src, int top, int bottom, int left,
                               int right) {
  const int32_t width = XI_TILE_GET_WIDTH(src);
  const int32_t height = XI_TILE_GET_HEIGHT(src);
  const int32_t stride = XI_TILE_GET_PITCH(src);
  const int32_t edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  const int32_t edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
  uint8_t *restrict psrc = (uint8_t *)XI_TILE_GET_DATA_PTR(src);
  int x, y;
  // Repeat the edges
  // top edge
  if (top == 1) {
    for (x = -edge_width; x < (width + edge_width); x++) {
      for (y = -edge_height; y < 0; y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
  }
  // bottom edge
  if (bottom == 1) {
    for (x = -edge_width; x < (width + edge_width); x++) {
      for (y = height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
  }
  // left edge
  if (left == 1) {
    for (x = -edge_width; x < 0; x++) {
      for (y = -edge_height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
  }
  // right edge
  if (right == 1) {
    for (x = width; x < (width + edge_width); x++) {
      for (y = -edge_height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
  }
  return 0;
}

XI_ERR_TYPE xiEdgeRepeatS16_ref(xi_pTile src, int top, int bottom, int left,
                                int right) {
  const int32_t width = XI_TILE_GET_WIDTH(src);
  const int32_t height = XI_TILE_GET_HEIGHT(src);
  const int32_t stride = XI_TILE_GET_PITCH(src);
  const int32_t edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  const int32_t edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
  int16_t *restrict psrc = (int16_t *)XI_TILE_GET_DATA_PTR(src);
  int x, y;
  // Repeat the edges
  // top edge
  if (top == 1) {
    for (x = -edge_width; x < (width + edge_width); x++) {
      for (y = -edge_height; y < 0; y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
  }
  // bottom edge
  if (bottom == 1) {
    for (x = -edge_width; x < (width + edge_width); x++) {
      for (y = height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
  }
  // left edge
  if (left == 1) {
    for (x = -edge_width; x < 0; x++) {
      for (y = -edge_height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
  }
  // right edge
  if (right == 1) {
    for (x = width; x < (width + edge_width); x++) {
      for (y = -edge_height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
  }
  return 0;
}

XI_ERR_TYPE xiKEdgeRepeatS16_ref(xi_pTile src, int top, int bottom, int left,
                                 int right) {
  const int32_t width1 = XI_TILE_GET_WIDTH(src);
  const int32_t height = XI_TILE_GET_HEIGHT(src);
  const int32_t stride = XI_TILE_GET_PITCH(src);
  const int32_t edge_width1 = XI_TILE_GET_EDGE_WIDTH(src);
  const int32_t edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
  int16_t *restrict psrc = (int16_t *)XI_TILE_GET_DATA_PTR(src);
  int x, y, k;
  int levels = 8;
  int32_t edge_width = edge_width1 / 8;
  int32_t width = width1 / 8;
  // Repeat the edges
  // top edge
  if (top == 1) {
    for (x = -edge_width; x < (width + edge_width); x++) {
      for (y = -edge_height; y < 0; y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        for (k = 0; k < levels; k++) {
          psrc[(y * stride) + (8 * x) + k] = psrc[(y1 * stride) + (8 * x1) + k];
        }
      }
    }
  }
  // bottom edge
  if (bottom == 1) {
    for (x = -edge_width; x < (width + edge_width); x++) {
      for (y = height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        for (k = 0; k < levels; k++) {
          psrc[(y * stride) + (8 * x) + k] = psrc[(y1 * stride) + (8 * x1) + k];
        }
      }
    }
  }
  // left edge
  if (left == 1) {
    for (x = -edge_width; x < 0; x++) {
      for (y = -edge_height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        for (k = 0; k < levels; k++) {
          psrc[(y * stride) + (8 * x) + k] = psrc[(y1 * stride) + (8 * x1) + k];
        }
      }
    }
  }
  // right edge
  if (right == 1) {
    for (x = width; x < (width + edge_width); x++) {
      for (y = -edge_height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        for (k = 0; k < levels; k++) {
          psrc[(y * stride) + (8 * x) + k] = psrc[(y1 * stride) + (8 * x1) + k];
        }
      }
    }
  }
  return 0;
}

XI_ERR_TYPE xiEdgeRepeatU8(xi_pTile src, int top, int bottom, int left,
                           int right) {
  const int32_t width = XI_TILE_GET_WIDTH(src);
  const int32_t height = XI_TILE_GET_HEIGHT(src);
  const int32_t stride = XI_TILE_GET_PITCH(src);
  const int32_t edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  const int32_t edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
  uint8_t *restrict psrc = (uint8_t *)XI_TILE_GET_DATA_PTR(src);
  int x, y;
  xb_vec2Nx8U *restrict rsrc, *restrict rdst;
  // Repeat the edges
  // top edge
  if (top == 1) {
    for (y = -edge_height; y < 0; y++) {
      for (x = -edge_width; x < 0; x++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
    for (y = -edge_height; y < 0; y++) {
      rsrc = OFFSET_PTR_2NX8U(psrc, 0, 0, 0);
      rdst = OFFSET_PTR_2NX8U(psrc, y, stride, 0);
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      valign a_load1 = IVP_ZALIGN();
      for (x = 0; x < width; x += 64) {
        xb_vec2Nx8U vec;
        IVP_LAV2NX8U_XP(vec, a_load, rsrc, width - x);
        IVP_SAV2NX8U_XP(vec, a_load1, rdst, width - x);
      }
      IVP_SAPOS2NX8U_FP(a_load1, rdst);
    }
    for (y = -edge_height; y < 0; y++) {
      for (x = width; x < (width + edge_width); x++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
  }
  // bottom edge
  if (bottom == 1) {
    for (x = -edge_width; x < 0; x++) {
      for (y = height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }

    for (y = height; y < (height + edge_height); y++) {
      rsrc = OFFSET_PTR_2NX8U(psrc, (height - 1), stride, 0);
      rdst = OFFSET_PTR_2NX8U(psrc, y, stride, 0);
      valign a_load = IVP_LA2NX8U_PP(rsrc);
      valign a_load1 = IVP_ZALIGN();
      for (x = 0; x < width; x += 64) {
        xb_vec2Nx8U vec;
        IVP_LAV2NX8U_XP(vec, a_load, rsrc, width - x);
        IVP_SAV2NX8U_XP(vec, a_load1, rdst, width - x);
      }
    }
    for (x = width; x < (width + edge_width); x++) {
      for (y = height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
  }

  // left edge
  if (left == 1) {
    for (x = -edge_width; x < 0; x++) {
      for (y = -edge_height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
  }
  // right edge
  if (right == 1) {
    for (x = width; x < (width + edge_width); x++) {
      for (y = -edge_height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
  }
  return 0;
}

XI_ERR_TYPE xiEdgeRepeatS16(xi_pTile src, int top, int bottom, int left,
                            int right) {
  const int32_t width = XI_TILE_GET_WIDTH(src);
  const int32_t height = XI_TILE_GET_HEIGHT(src);
  const int32_t stride = XI_TILE_GET_PITCH(src);
  const int32_t edge_width = XI_TILE_GET_EDGE_WIDTH(src);
  const int32_t edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
  int16_t *restrict psrc = (int16_t *)XI_TILE_GET_DATA_PTR(src);
  int x, y;
  xb_vecNx16 *restrict rsrc, *restrict rdst;
  // Repeat the edges
  // top edge
  if (top == 1) {
    for (y = -edge_height; y < 0; y++) {
      for (x = -edge_width; x < 0; x++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
    for (y = -edge_height; y < 0; y++) {
      rsrc = OFFSET_PTR_NX16(psrc, 0, 0, 0);
      rdst = OFFSET_PTR_NX16(psrc, y, stride, 0);
      valign a_load = IVP_LANX16_PP(rsrc);
      valign a_load1 = IVP_ZALIGN();
      for (x = 0; x < width; x += 32) {
        xb_vecNx16 vec;
        IVP_LAVNX16_XP(vec, a_load, rsrc, 2 * (width - x));
        IVP_SAVNX16_XP(vec, a_load1, rdst, 2 * (width - x));
      }
      IVP_SAPOSNX16_FP(a_load1, rdst);
    }
    for (y = -edge_height; y < 0; y++) {
      for (x = width; x < (width + edge_width); x++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
  }
  // bottom edge
  if (bottom == 1) {
    for (x = -edge_width; x < 0; x++) {
      for (y = height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
    for (y = height; y < (height + edge_height); y++) {
      rsrc = OFFSET_PTR_NX16(psrc, (height - 1), stride, 0);
      rdst = OFFSET_PTR_NX16(psrc, y, stride, 0);
      valign a_load = IVP_LANX16_PP(rsrc);
      valign a_load1 = IVP_ZALIGN();
      for (x = 0; x < width; x += 32) {
        xb_vecNx16 vec;
        IVP_LAVNX16_XP(vec, a_load, rsrc, 2 * (width - x));
        IVP_SAVNX16_XP(vec, a_load1, rdst, 2 * (width - x));
      }
      IVP_SAPOSNX16_FP(a_load1, rdst);
    }
    for (x = width; x < (width + edge_width); x++) {
      for (y = height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
  }
  // left edge
  if (left == 1) {
    for (x = -edge_width; x < 0; x++) {
      for (y = -edge_height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
  }
  // right edge
  if (right == 1) {
    for (x = width; x < (width + edge_width); x++) {
      for (y = -edge_height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        psrc[(y * stride) + x] = psrc[(y1 * stride) + x1];
      }
    }
  }
  return 0;
}

XI_ERR_TYPE xiKEdgeRepeatS16(xi_pTile src, int top, int bottom, int left,
                             int right) {
  const int32_t width1 = XI_TILE_GET_WIDTH(src);
  const int32_t height = XI_TILE_GET_HEIGHT(src);
  const int32_t stride = XI_TILE_GET_PITCH(src);
  const int32_t edge_width1 = XI_TILE_GET_EDGE_WIDTH(src);
  const int32_t edge_height = XI_TILE_GET_EDGE_HEIGHT(src);
  int16_t *restrict psrc = (int16_t *)XI_TILE_GET_DATA_PTR(src);
  xb_vecNx16 *restrict rsrc, *restrict rdst;
  int x, y, k;
  int levels = 8;
  int32_t edge_width = edge_width1 / 8;
  int32_t width = width1 / 8;
  // Repeat the edges
  // top edge
  if (top == 1) {

    for (y = -edge_height; y < 0; y++) {
      for (x = -edge_width; x < 0; x++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        for (k = 0; k < levels; k++) {
          psrc[(y * stride) + (8 * x) + k] = psrc[(y1 * stride) + (8 * x1) + k];
        }
      }
    }

    for (y = -edge_height; y < 0; y++) {
      rsrc = OFFSET_PTR_NX16(psrc, 0, 0, 0);
      rdst = OFFSET_PTR_NX16(psrc, y, stride, 0);
      valign a_load = IVP_LANX16_PP(rsrc);
      valign a_load1 = IVP_ZALIGN();
      for (x = 0; x < width1; x += 32) {
        xb_vecNx16 vec;
        IVP_LAVNX16_XP(vec, a_load, rsrc, 2 * (width1 - x));
        IVP_SAVNX16_XP(vec, a_load1, rdst, 2 * (width1 - x));
      }
      IVP_SAPOSNX16_FP(a_load1, rdst);
    }

    for (y = -edge_height; y < 0; y++) {
      for (x = width; x < (width + edge_width); x++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        for (k = 0; k < levels; k++) {
          psrc[(y * stride) + (8 * x) + k] = psrc[(y1 * stride) + (8 * x1) + k];
        }
      }
    }
  }
  // bottom edge
  if (bottom == 1) {
    for (x = -edge_width; x < 0; x++) {
      for (y = height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        for (k = 0; k < levels; k++) {
          psrc[(y * stride) + (8 * x) + k] = psrc[(y1 * stride) + (8 * x1) + k];
        }
      }
    }
    for (y = height; y < (height + edge_height); y++) {
      rsrc = OFFSET_PTR_NX16(psrc, (height - 1), stride, 0);
      rdst = OFFSET_PTR_NX16(psrc, y, stride, 0);
      valign a_load = IVP_LANX16_PP(rsrc);
      valign a_load1 = IVP_ZALIGN();
      for (x = 0; x < width1; x += 32) {
        xb_vecNx16 vec;
        IVP_LAVNX16_XP(vec, a_load, rsrc, 2 * (width1 - x));
        IVP_SAVNX16_XP(vec, a_load1, rdst, 2 * (width1 - x));
      }
      IVP_SAPOSNX16_FP(a_load1, rdst);
    }

    for (x = width; x < (width + edge_width); x++) {
      for (y = height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        for (k = 0; k < levels; k++) {
          psrc[(y * stride) + (8 * x) + k] = psrc[(y1 * stride) + (8 * x1) + k];
        }
      }
    }
  }
  // left edge
  if (left == 1) {
    for (x = -edge_width; x < 0; x++) {
      for (y = -edge_height; y < 0; y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        for (k = 0; k < levels; k++) {
          psrc[(y * stride) + (8 * x) + k] = psrc[(y1 * stride) + (8 * x1) + k];
        }
      }
    }
    for (x = -edge_width; x < 0; x++) {
      rsrc = OFFSET_PTR_NX16(psrc, 0, 0, 0);
      rdst = OFFSET_PTR_NX16(psrc, 0, 0, (8 * x));
      valign a_load1 = IVP_ZALIGN();
      int32_t sstep = stride - 8;
      int32_t dstep = stride - 8;
      for (y = 0; y < height; y++) {
        valign a_load = IVP_LANX16_PP(rsrc);
        xb_vecNx16 vec;
        IVP_LAVNX16_XP(vec, a_load, rsrc, 16);
        IVP_SAVNX16_XP(vec, a_load1, rdst, 16);
        IVP_SAPOSNX16_FP(a_load1, rdst);
        rsrc = OFFSET_PTR_NX16(rsrc, 1, sstep, 0);
        rdst = OFFSET_PTR_NX16(rdst, 1, dstep, 0);
      }
    }
    for (x = -edge_width; x < 0; x++) {
      for (y = height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        for (k = 0; k < levels; k++) {
          psrc[(y * stride) + (8 * x) + k] = psrc[(y1 * stride) + (8 * x1) + k];
        }
      }
    }
  }
  // right edge
  if (right == 1) {
    for (x = width; x < (width + edge_width); x++) {
      for (y = -edge_height; y < 0; y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        for (k = 0; k < levels; k++) {
          psrc[(y * stride) + (8 * x) + k] = psrc[(y1 * stride) + (8 * x1) + k];
        }
      }
    }
    for (x = width; x < (width + edge_width); x++) {
      for (y = -edge_height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        for (k = 0; k < levels; k++) {
          psrc[(y * stride) + (8 * x) + k] = psrc[(y1 * stride) + (8 * x1) + k];
        }
      }
    }
    for (x = width; x < (width + edge_width); x++) {
      rsrc = OFFSET_PTR_NX16(psrc, 0, 0, 8 * (width - 1));
      rdst = OFFSET_PTR_NX16(psrc, 0, 0, (8 * x));
      valign a_load1 = IVP_ZALIGN();
      int32_t sstep = stride - 8;
      int32_t dstep = stride - 8;
      for (y = 0; y < height; y++) {
        valign a_load = IVP_LANX16_PP(rsrc);
        xb_vecNx16 vec;
        IVP_LAVNX16_XP(vec, a_load, rsrc, 16);
        IVP_SAVNX16_XP(vec, a_load1, rdst, 16);
        IVP_SAPOSNX16_FP(a_load1, rdst);
        rsrc = OFFSET_PTR_NX16(rsrc, 1, sstep, 0);
        rdst = OFFSET_PTR_NX16(rdst, 1, dstep, 0);
      }
    }
    for (x = width; x < (width + edge_width); x++) {
      for (y = height; y < (height + edge_height); y++) {
        int x1 = CLIP(x, 0, (width - 1));
        int y1 = CLIP(y, 0, (height - 1));
        for (k = 0; k < levels; k++) {
          psrc[(y * stride) + (8 * x) + k] = psrc[(y1 * stride) + (8 * x1) + k];
        }
      }
    }
  }
  return 0;
}
