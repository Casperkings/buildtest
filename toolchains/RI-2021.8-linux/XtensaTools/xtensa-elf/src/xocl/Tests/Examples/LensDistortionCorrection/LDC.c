#include "../../Native/LensDistortionCorrection_ref/LDC_parameters.h"
#include "xi_lib.h"

#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/tie/xt_misc.h>

#define USE_GATHER_V
#define GATHER_DELAY (2)

#define GEN_GSR_REGS() xb_gsr gs0, gs1, gs2, gs3;
#define GEN_SGR_REGS()

short sv_rect_j[32] __attribute__((section(".dram0.data"))) = {
    32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17,
    16, 15, 14, 13, 12, 11, 10, 9,  8,  7,  6,  5,  4,  3,  2,  1};
short sv_j_uv[32] __attribute__((section(".dram0.data"))) = {
    0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,
    0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30};
short sv_i_uv[32] __attribute__((section(".dram0.data"))) = {
    1, 5, 9, 13, 17, 21, 25, 29, 33, 37, 41, 45, 49, 53, 57, 61,
    1, 5, 9, 13, 17, 21, 25, 29, 33, 37, 41, 45, 49, 53, 57, 61,
};
short sv_rect_j_uv[32] __attribute__((section(".dram0.data"))) = {
    32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2,
    32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2};
short sv_rect_i_uv[32] __attribute__((section(".dram0.data"))) = {
    63, 59, 55, 51, 47, 43, 39, 35, 31, 27, 23, 19, 15, 11, 7, 3,
    63, 59, 55, 51, 47, 43, 39, 35, 31, 27, 23, 19, 15, 11, 7, 3,
};
short sv_reorder_0[32] __attribute__((section(".dram0.data"))) = {
    0,  4,  8,  12, 16, 20, 24, 28, 32, 36,  40,  44,  48,  52,  56,  60,
    64, 68, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108, 112, 116, 120, 124};
short sv_reorder_1[32] __attribute__((section(".dram0.data"))) = {
    1,  5,  9,  13, 17, 21, 25, 29, 33, 37,  41,  45,  49,  53,  57,  61,
    65, 69, 73, 77, 81, 85, 89, 93, 97, 101, 105, 109, 113, 117, 121, 125,
};
short sv_reorder_2[32] __attribute__((section(".dram0.data"))) = {
    2,  6,  10, 14, 18, 22, 26, 30, 34, 38,  42,  46,  50,  54,  58,  62,
    66, 70, 74, 78, 82, 86, 90, 94, 98, 102, 106, 110, 114, 118, 122, 126,
};
short sv_reorder_3[32] __attribute__((section(".dram0.data"))) = {
    3,  7,  11, 15, 19, 23, 27, 31, 35, 39,  43,  47,  51,  55,  59,  63,
    67, 71, 75, 79, 83, 87, 91, 95, 99, 103, 107, 111, 115, 119, 123, 127,
};

void image_transform_420sp(short *input_x, short *input_y, const xi_pTile srcY,
                           const xi_pTile srcUV, xi_pTile dstY, xi_pTile dstUV,
                           int bbx, int bby) {

  int input_width = XI_TILE_GET_WIDTH(srcY);
  int input_height = XI_TILE_GET_HEIGHT(srcY);
  int input_stride = XI_TILE_GET_PITCH(srcY);
  unsigned char *input_image_y = XI_TILE_GET_DATA_PTR(srcY);
  unsigned char *input_image_uv = XI_TILE_GET_DATA_PTR(srcUV);
  int output_stride = XI_TILE_GET_PITCH(dstY);
  unsigned char *__restrict__ output_image_y = XI_TILE_GET_DATA_PTR(dstY);
  unsigned char *output_image_uv = XI_TILE_GET_DATA_PTR(dstUV);
  // printf("%x\n", &bbx);

  short i, j;
  short x, y, x0, x1, y0, y1;
  short x_int, y_int, x_dec, y_dec;
  short pixel_values[4], res0, res1;

  // vector pointers to point short arrays
  xb_vecNx16 *__restrict xvp_rect_j = (xb_vecNx16 *)sv_rect_j;
  xb_vecNx8U *__restrict xvp_output_image_y = (xb_vecNx8U *)output_image_y;
  xb_vecNx16 *__restrict xvp_j_uv = (xb_vecNx16 *)sv_j_uv;
  xb_vecNx16 *__restrict xvp_i_uv = (xb_vecNx16 *)sv_j_uv;
  xb_vecNx16 *__restrict xvp_rect_j_uv = (xb_vecNx16 *)sv_rect_j_uv;
  xb_vecNx16 *__restrict xvp_rect_i_uv = (xb_vecNx16 *)sv_rect_i_uv;
  xb_vec2Nx8U *__restrict xvp_output_image_uv = (xb_vec2Nx8U *)output_image_uv;

  xb_vecNx16 xv_x_dec_uv, xv_x_dec_uv2;
  xb_vecNx16 xv_y_dec_uv, xv_y_dec_uv2;

  // translate rectangle relative to bounding box to reduce the range of integer
  // part
  input_x[0] -= bbx << PIXEL_Q;
  input_x[1] -= bbx << PIXEL_Q;
  input_x[2] -= bbx << PIXEL_Q;
  input_x[3] -= bbx << PIXEL_Q;
  input_y[0] -= bby << PIXEL_Q;
  input_y[1] -= bby << PIXEL_Q;
  input_y[2] -= bby << PIXEL_Q;
  input_y[3] -= bby << PIXEL_Q;

  // Convert scalar to vector by replication
  xb_vecNx16 xv_input_x0 = IVP_MOVVA16(input_x[0]);
  xb_vecNx16 xv_input_x1 = IVP_MOVVA16(input_x[1]);
  xb_vecNx16 xv_input_x2 = IVP_MOVVA16(input_x[2]);
  xb_vecNx16 xv_input_x3 = IVP_MOVVA16(input_x[3]);
  xb_vecNx16 xv_input_y0 = IVP_MOVVA16(input_y[0]);
  xb_vecNx16 xv_input_y1 = IVP_MOVVA16(input_y[1]);
  xb_vecNx16 xv_input_y2 = IVP_MOVVA16(input_y[2]);
  xb_vecNx16 xv_input_y3 = IVP_MOVVA16(input_y[3]);

  xb_vecNx16 *__restrict pvecAddrTL;

  // printf("IVP\n");

  /*******************************************************************************************
   * Luma
   *******************************************************************************************/

  /*
   *  Loop-invariant part (== j part)
   *    - Vector generation of the coordinates: x0, x1, y0, y1 at 4 corners of
   * rectangle
   *
   */
  xb_vecNx16 xv_x0, xv_x1;
  xb_vecNx16 xv_y0, xv_y1;
  xb_vecNx48 acc;
  xb_vecNx16 xv_j = IVP_SEQNX16();

  // x0 = ((RECT_SIZE - j)*input_x[0] + j*input_x[1] + RECT_SIZE/2)/RECT_SIZE;
  acc = IVP_MULPNX16(*xvp_rect_j, xv_input_x0, xv_j, xv_input_x1);
  xv_x0 = IVP_PACKVRNX48(acc, 5);

  // x1 = ((RECT_SIZE - j)*input_x[2] + j*input_x[3] + RECT_SIZE/2)/RECT_SIZE;
  acc = IVP_MULPNX16(*xvp_rect_j, xv_input_x2, xv_j, xv_input_x3);
  xv_x1 = IVP_PACKVRNX48(acc, 5);

  // y0 = ((RECT_SIZE - j)*input_y[0] + j*input_y[1] + RECT_SIZE/2)/RECT_SIZE;
  acc = IVP_MULPNX16(*xvp_rect_j, xv_input_y0, xv_j, xv_input_y1);
  xv_y0 = IVP_PACKVRNX48(acc, 5);

  // y1 = ((RECT_SIZE - j)*input_y[2] + j*input_y[3] + RECT_SIZE/2)/RECT_SIZE;
  acc = IVP_MULPNX16(*xvp_rect_j, xv_input_y2, xv_j, xv_input_y3);
  xv_y1 = IVP_PACKVRNX48(acc, 5);

  /*
   *  Loop-variant part (== i part)
   *    - Vector linear interpolation: x, y, x_int, y_int, x_dec, y_dec
   *    - Vector address generation to look-up input_image_y
   *
   */
  // xb_vecNx16 * __restrict xvp_x 		= (xb_vecNx16 *) sv_x;
  // xb_vecNx16 * __restrict xvp_y 		= (xb_vecNx16 *) sv_y;
  // xb_vecNx16 * __restrict xvp_x_dec 	= (xb_vecNx16 *) sv_x_dec;
  // xb_vecNx16 * __restrict xvp_y_dec 	= (xb_vecNx16 *) sv_y_dec;
  // xb_vecNx16 * __restrict xvp_pixel_values_0_addr = (xb_vecNx16 *)
  // iv_pixel_values_0_addr;

  xb_vecNx16 xv_v0 = 1;
  xb_vecNx16 xv_v1 = 1;
  xb_vecNx16 xv_i = IVP_SEQNX16();

  xb_vecNx16 xint, xfrac, yint, yfrac;

  /*
   * Now uses gathers and other loops have been merge to make use of gather
   * latency
   */

  // xvp_x_dec = (xb_vecNx16 *) sv_x_dec;
  // xvp_y_dec = (xb_vecNx16 *) sv_y_dec;

  // xb_vecNx8 * __restrict pvecPixel0 = (xb_vecNx8*)sv_pixels01;
  // xb_vecNx8 * __restrict pvecPixel2 = (xb_vecNx8*)sv_pixels23;

  GEN_GSR_REGS();

  /*******************************************************************************************
   * calculate co-ordinates, perform gathers and interpolation
   *******************************************************************************************/

  for (i = 0; i < RECT_SIZE; i++) { // LOOP_OF_INTEREST %LDC_1

    xb_vecNx16 xv_rect_i = IVP_MOVVA16(32 - i);
    xb_vecNx16 xv_i = IVP_MOVVA16(i), vp_x, vp_y;

    // x = ((RECT_SIZE - i)*x0 + i*x1 + RECT_SIZE/2)/RECT_SIZE;
    acc = IVP_MULPNX16(xv_rect_i, xv_x0, xv_i, xv_x1);

    xint = IVP_PACKVRNX48(acc, 5);
    xfrac = xint & (xb_vecNx16)(PIXEL_DEC - 1);
    xint = xint >> (xb_vecNx16)(PIXEL_Q);

    // y = ((RECT_SIZE - i)*y0 + i*y1 + RECT_SIZE/2)/RECT_SIZE;
    acc = IVP_MULPNX16(xv_rect_i, xv_y0, xv_i, xv_y1);
    yint = IVP_PACKVRNX48(acc, 5);
    yfrac = yint & (xb_vecNx16)(PIXEL_DEC - 1);
    yint = yint >> ((xb_vecNx16)PIXEL_Q);

    // since rectangle is translated to input tile, 16b is enough for this
    // calculation
    xb_vecNx16 vecAddr = xint; //*xvp_x;
    IVP_MULANX16PACKL(vecAddr, yint, (xb_vecNx16)input_stride);

    valign vaA;
    xb_vecNx16 vecPixel01, vecPixel23, vecPixelTemp0, vecPixelTemp1;
    xb_vecNx16 vecAddrTL = vecAddr; //&(iv_pixel_values_0_addr[RECT_SIZE*i]);

#ifndef USE_GATHER_V
    gs0 = IVP_GATHERANX8U(input_image_y, vecAddrTL);
    xb_vecNx16 vecTL = IVP_GATHERDNX8U(gs0);
    vecTL = vecTL & (xb_vecNx16)0xff;
#else
    xb_vecNx16 vecTL = IVP_GATHERNX8U_V(input_image_y, vecAddrTL, GATHER_DELAY);
#endif

    xb_vecNx16 vecAddrTR = vecAddrTL + (xb_vecNx16)1;

#ifndef USE_GATHER_V
    gs1 = IVP_GATHERANX8U(input_image_y, vecAddrTR);
    xb_vecNx16 vecTR = IVP_GATHERDNX8U(gs1);
    vecTR = vecTR & (xb_vecNx16)0xff;
#else
    xb_vecNx16 vecTR = IVP_GATHERNX8U_V(input_image_y, vecAddrTR, GATHER_DELAY);
#endif

    acc = IVP_MULNX16(((xb_vecNx16)(PIXEL_DEC)-xfrac), vecTL);
    IVP_MULANX16(acc, xfrac, vecTR);
    vecTL = IVP_PACKVRNX48(acc, 4);

    xb_vecNx16 vecAddrBL = vecAddrTL + (xb_vecNx16)input_stride;

#ifndef USE_GATHER_V
    gs2 = IVP_GATHERANX8U(input_image_y, vecAddrBL);
    xb_vecNx16 vecBL = IVP_GATHERDNX8U(gs2);
    vecBL = vecBL & (xb_vecNx16)0xff;
#else
    xb_vecNx16 vecBL = IVP_GATHERNX8U_V(input_image_y, vecAddrBL, GATHER_DELAY);
#endif

    xb_vecNx16 vecAddrBR = vecAddrBL + (xb_vecNx16)1;

#ifndef USE_GATHER_V
    gs3 = IVP_GATHERANX8U(input_image_y, vecAddrBR);
    xb_vecNx16 vecBR = IVP_GATHERDNX8U(gs3);
    vecBR = vecBR & (xb_vecNx16)0xff;
#else
    xb_vecNx16 vecBR = IVP_GATHERNX8U_V(input_image_y, vecAddrBR, GATHER_DELAY);
#endif

    acc = IVP_MULPNX16(((xb_vecNx16)(PIXEL_DEC)-xfrac), vecBL, xfrac, vecBR);
    vecBL = IVP_PACKVRNX48(acc, 4);

    acc = IVP_MULPNX16(((xb_vecNx16)(PIXEL_DEC)-yfrac), vecTL, yfrac, vecBL);
    vecTL = IVP_PACKVRNX48(acc, 4);

    *xvp_output_image_y = (xb_vecNx8U)vecTL;

    // increment pointers
    xvp_output_image_y++;
  }

  /*******************************************************************************************
   * UV part
   * can luma calculations be reused for chroma ?
   * processing two rows at a time
   *******************************************************************************************/

  // Vectorized "Linear interpolation of the coordinates

  // x0 = ((RECT_SIZE - j)*input_x[0] + j*input_x[1] + RECT_SIZE/2)/RECT_SIZE;
  acc = IVP_MULPNX16(*xvp_rect_j_uv, xv_input_x0, *xvp_j_uv,
                     xv_input_x1); // (RECT_SIZE-j)*input_x[0]
  xv_x0 = IVP_PACKVRNX48(acc, 5);

  // x1 = ((RECT_SIZE - j)*input_x[2] + j*input_x[3] + RECT_SIZE/2)/RECT_SIZE;
  acc = IVP_MULPNX16(*xvp_rect_j_uv, xv_input_x2, *xvp_j_uv,
                     xv_input_x3); // (RECT_SIZE-j)*input_x[2]
  xv_x1 = IVP_PACKVRNX48(acc, 5);

  // y0 = ((RECT_SIZE - j)*input_y[0] + j*input_y[1] + RECT_SIZE/2)/RECT_SIZE;
  acc = IVP_MULPNX16(*xvp_rect_j_uv, xv_input_y0, *xvp_j_uv,
                     xv_input_y1); // (RECT_SIZE-j)*input_y[0]
  xv_y0 = IVP_PACKVRNX48(acc, 5);

  // y1 = ((RECT_SIZE - j)*input_y[2] + j*input_y[3] + RECT_SIZE/2)/RECT_SIZE;
  acc = IVP_MULPNX16(*xvp_rect_j_uv, xv_input_y2, *xvp_j_uv,
                     xv_input_y3); // (RECT_SIZE-j)*input_y[2]
  xv_y1 = IVP_PACKVRNX48(acc, 5);

  xb_vec2Nx8 xv_x_dec;
  xb_vec2Nx8 xv_y_dec;

  unsigned char *input_image_uv_stride = input_image_uv + input_stride;

  /*
  PRINT_VEC16(xv_x0, "X0\n");
  PRINT_VEC16(xv_x1, "X1\n");
  PRINT_VEC16(xv_y0, "Y0\n");
  PRINT_VEC16(xv_y1, "Y1\n");
  */

  for (i = 1; i < 64; i += 8) { // LOOP_OF_INTEREST %LDC_2
    xb_vecNx48 acc;

    xb_vecNx16 xv_rect_2_i = IVP_SELNX16I(IVP_MOVVA16(RECT_SIZE * 2 - (i + 4)),
                                          IVP_MOVVA16(RECT_SIZE * 2 - (i)),
                                          IVP_SELI_ROTATE_RIGHT_16);
    xb_vecNx16 xv_i = IVP_SELNX16I(IVP_MOVVA16(i + 4), IVP_MOVVA16(i),
                                   IVP_SELI_ROTATE_RIGHT_16);

    // x = ((RECT_SIZE*2 - i)*x0 + i*x1 + RECT_SIZE)/(RECT_SIZE*2);
    acc = IVP_MULPNX16(xv_rect_2_i, xv_x0, xv_i, xv_x1); // (RECT_SIZE*2-i)*x0
    xb_vecNx16 xvp_x_uv = IVP_PACKVRNX48(acc, 6);
    xb_vecNx16 xvp_x_dec_uv =
        xvp_x_uv &
        (xb_vecNx16)(PIXEL_DEC * 2 - 1); // x_dec = x&(PIXEL_DEC*2 - 1);
    xvp_x_uv = xvp_x_uv >> (xb_vecNx16)(PIXEL_Q + 1);

    // y = ((RECT_SIZE*2 - i)*y0 + i*y1 + RECT_SIZE)/(RECT_SIZE*2);
    acc = IVP_MULPNX16(xv_rect_2_i, xv_y0, xv_i, xv_y1); // (RECT_SIZE*2-i)*y0
    xb_vecNx16 xvp_y_uv = IVP_PACKVRNX48(acc, 6);
#ifdef CASE1
    xvp_y_uv -= (xb_vecNx16)(PIXEL_DEC / 2); // y -= PIXEL_DEC/2;
#else
    // causes an error of negative index that needs to be investigated
    // xvp_y_uv -= (xb_vecNx16)(PIXEL_DEC/2);					// y -=
    // PIXEL_DEC/2;
#endif

    xb_vecNx16 xvp_y_dec_uv =
        xvp_y_uv &
        (xb_vecNx16)(PIXEL_DEC * 2 - 1); // y_dec = y&(PIXEL_DEC*2 - 1);
    xvp_y_uv =
        xvp_y_uv >> (xb_vecNx16)(PIXEL_Q + 1); // y_int = y>>(PIXEL_Q + 1);

    // PRINT_VEC16_2(xvp_x_dec_uv, xvp_y_dec_uv, "");

    // linearized address
    xb_vecNx16 vecAddr = xvp_x_uv << 1;
    IVP_MULANX16PACKL(vecAddr, xvp_y_uv, (xb_vecNx16)input_stride);

    // PRINT_VEC16_3(*xvp_x_uv, *xvp_y_uv, vecAddr, "");

    xb_vecNx16 vecAddrTL = vecAddr;
    xb_vecNx16 vecAddrTR = vecAddr + (xb_vecNx16)2;
#ifndef USE_GATHER_V
    gs0 = IVP_GATHERANX16(input_image_uv, vecAddr);
    xb_vecNx16 vecTL = IVP_GATHERDNX16(gs0);

    gs1 = IVP_GATHERANX16(input_image_uv, vecAddrTR);
    xb_vecNx16 vecTR = IVP_GATHERDNX16(gs1);

    gs2 = IVP_GATHERANX16(input_image_uv_stride,
                          vecAddr); // add boolean for unused values
    xb_vecNx16 vecBL = IVP_GATHERDNX16(gs2);

    gs3 = IVP_GATHERANX16(input_image_uv_stride, vecAddrTR);
    xb_vecNx16 vecBR = IVP_GATHERDNX16(gs3);
#else
    xb_vecNx16 vecTL =
        IVP_GATHERNX16_V((int16_t *)input_image_uv, vecAddr, GATHER_DELAY);
    xb_vecNx16 vecTR =
        IVP_GATHERNX16_V((int16_t *)input_image_uv, vecAddrTR, GATHER_DELAY);
    xb_vecNx16 vecBL = IVP_GATHERNX16_V((int16_t *)input_image_uv_stride,
                                        vecAddr, GATHER_DELAY);
    xb_vecNx16 vecBR = IVP_GATHERNX16_V((int16_t *)input_image_uv_stride,
                                        vecAddrTR, GATHER_DELAY);
#endif

    // PRINT_VEC8(vecTR, "");
    // PRINT_VEC16(vecAddrTR, "");

    xb_vec2Nx8U xv_res0, xv_res1;
    xb_vec2Nx24 acc2n;

    // res0 = ((PIXEL_DEC*2 - x_dec)*pixel_values[0] + x_dec*pixel_values[1] +
    // PIXEL_DEC)/PIXEL_DEC*2; replicate for U and V parts
    xb_vec2Nx8U xvp_x_dec_uv_2n = IVP_SEL2NX8I(
        IVP_MOV2NX8_FROMNX16(xvp_x_dec_uv), IVP_MOV2NX8_FROMNX16(xvp_x_dec_uv),
        IVP_SELI_8B_INTERLEAVE_1_EVEN);
    // xvp_x_dec_uv = IVP_SELNX16I(xvp_x_dec_uv,
    // xvp_x_dec_uv,IVP_SELI_INTERLEAVE_1_LO);
    acc2n = IVP_MULUUP2NX8((((xb_vec2Nx8U)(2 * PIXEL_DEC)) - xvp_x_dec_uv_2n),
                           IVP_MOV2NX8_FROMNX16(vecTL), xvp_x_dec_uv_2n,
                           IVP_MOV2NX8_FROMNX16(vecTR));
    xv_res0 = IVP_PACKVRU2NX24(acc2n, 5);

    // xv_x_dec = *xvp_x_dec_uv_dup_8++;
    // res1 = ((PIXEL_DEC*2 - x_dec)*pixel_values[2] + x_dec*pixel_values[3] +
    // PIXEL_DEC)/PIXEL_DEC*2;
    acc2n = IVP_MULUUP2NX8((((xb_vec2Nx8U)(2 * PIXEL_DEC)) - xvp_x_dec_uv_2n),
                           IVP_MOV2NX8_FROMNX16(vecBL), xvp_x_dec_uv_2n,
                           IVP_MOV2NX8_FROMNX16(vecBR));
    xv_res1 = IVP_PACKVRU2NX24(acc2n, 5);

    // PRINT_VEC8_2(xv_res0, xv_res1, "");

    xb_vec2Nx8U xvp_y_dec_uv_2n = IVP_SEL2NX8I(
        IVP_MOV2NX8_FROMNX16(xvp_y_dec_uv), IVP_MOV2NX8_FROMNX16(xvp_y_dec_uv),
        IVP_SELI_8B_INTERLEAVE_1_EVEN);
    // xvp_y_dec_uv = IVP_SELNX16I(xvp_y_dec_uv,
    // xvp_y_dec_uv,IVP_SELI_INTERLEAVE_1_LO);
    acc2n = IVP_MULUUP2NX8((((xb_vec2Nx8U)(2 * PIXEL_DEC)) - xvp_y_dec_uv_2n),
                           xv_res0, xvp_y_dec_uv_2n, xv_res1);
    xb_vec2Nx8U xvp_output_image_uv_tmp = IVP_PACKVRU2NX24(acc2n, 5);
    *xvp_output_image_uv++ = (xb_vec2Nx8U)xvp_output_image_uv_tmp;
  }
};
