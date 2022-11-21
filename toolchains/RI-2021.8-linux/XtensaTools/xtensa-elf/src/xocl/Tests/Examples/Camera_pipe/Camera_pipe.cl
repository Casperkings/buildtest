#define TILE_W 128
#define TILE_H 64
#define W_EXT 8
#define H_EXT 8
#define IMAGE_PAD 8
#define DST_BIT_DEPTH 24
#define SRC_BIT_DEPTH 16

#if NATIVE_KERNEL
extern void hot_pixel_suppression(__local short *input, int inputPitch, 
                                  int inputWidth, int inputHeight, 
                                  short *output, int outputPitch);

extern void deinterleave(short *input, int inputPitch, int inputWidth, 
                         int inputHeight, int outputPitch, short *pr_r, 
                         short *pb_b, short *pg_gr, short *pg_gb);

extern void Demosaic_pass1(short *pg_gr, short *pg_gb, int inputPitch, 
                           int inputWidth, int inputHeight, short *tmp_g_r, 
                           short *tmp_g_b);

extern void Demosaic_pass2(short *pr_r, short *pb_b, short *pg_gr, short *pg_gb,
                           int inputPitch_deinterleave, short *tmp_g_r, 
                           short *tmp_g_b, int inputWidth, int inputHeight, 
                           short *tmp_r_gr, short *tmp_b_gr, short *tmp_r_gb, 
                           short *tmp_b_gb, short *tmp_r_b, short *tmp_b_r);

extern void Rearrange(short *pr_r, short *pb_b, short *pg_gr, short *pg_gb,
                      int inputPitch_deinterleave, short *tmp_g_r, 
                      short *tmp_g_b, short *tmp_r_gr, short *tmp_b_gr, 
                      short *tmp_r_gb, short *tmp_b_gb, short *tmp_r_b, 
                      short *tmp_b_r, int inputWidth, int inputHeight);

extern void apply_curve(short *pr_r, short *pb_b, short *pg_gr, short *pg_gb,
                        int inputPitch_deinterleave, short *tmp_g_r, 
                        short *tmp_g_b, short *tmp_r_gr, short *tmp_b_gr, 
                        short *tmp_r_gb, short *tmp_b_gb, short *tmp_r_b, 
                        short *tmp_b_r, int inputWidth, int inputHeight, 
                        unsigned char *output, int outputPitch, 
                        __local unsigned char *local_src_curve,
                        __local short *local_src_matrix);

extern void sharpen(unsigned char *input, int inputPitch, int inputWidth,
                    int inputHeight, __local unsigned char *output, 
                    int outputPitch, unsigned char strength);
#endif

// Average two positive values rounding up
short32 avg(short32 a, short32 b) {
  return rhadd(a, b);
}

#if XT_OCL_EXT
#include "Camera_pipe_ext.cl"
#elif !NATIVE_KERNEL

void hot_pixel_suppression(__local short *input, int inputPitch, int inputWidth,
                           int inputHeight, short *output, int outputPitch) {
  int x, y;

  for (x = 2; x < inputWidth - 2; x += 32) {
    short *pdst = output + x - 2;
    for (y = 2; y < inputHeight - 2; y++) {
      __local short *psrc = input + y * inputPitch + x;
      short32 xCurrent = vload32(0, psrc);
      short32 xTop = vload32(0, psrc - 2 * inputPitch);
      short32 xBot = vload32(0, psrc + 2 * inputPitch);
      short32 xLeft = vload32(0, psrc + 2);
      short32 xRight = vload32(0, psrc - 2);

      short32 xout = min(xCurrent, max(max(max(xTop, xBot), xLeft), xRight));

      vstore32(xout, 0, pdst);
      pdst += outputPitch;
    }
  }
}

void deinterleave(short *input, int inputPitch, int inputWidth, int inputHeight,
                  int outputPitch, short *pr_r, short *pb_b, short *pg_gr,
                  short *pg_gb) {
  int x, y;
  ushort32 deinterleaveA = {0,  2,  4,  6,  8,  10, 12, 14, 16, 18, 20,
                            22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42,
                            44, 46, 48, 50, 52, 54, 56, 58, 60, 62};
  ushort32 deinterleaveB = deinterleaveA + (ushort32)1;

  for (x = 0; x < inputWidth; x += 64) {
    short *pdstg_gr = pg_gr + x / 2;
    short *pdstr_r = pr_r + x / 2;
    short *pdstb_b = pb_b + x / 2;
    short *pdstg_gb = pg_gb + x / 2;
    short *psrc = input + x;
    for (y = 0; y < inputHeight; y += 2) {

      short32 x00 = vload32(0, psrc);
      short32 x01 = vload32(0, psrc + 32);
      short32 x02 = vload32(0, psrc + inputPitch);
      short32 x03 = vload32(0, psrc + inputPitch + 32);

      psrc += 2 * inputPitch;

      short32 g_gr = shuffle2(x00, x01, deinterleaveA);
      short32 r_r = shuffle2(x00, x01, deinterleaveB);
      short32 b_b = shuffle2(x02, x03, deinterleaveA);
      short32 g_gb = shuffle2(x02, x03, deinterleaveB);

      vstore32(g_gr, 0, pdstg_gr);
      pdstg_gr += outputPitch;

      vstore32(r_r, 0, pdstr_r);
      pdstr_r += outputPitch;

      vstore32(b_b, 0, pdstb_b);
      pdstb_b += outputPitch;

      vstore32(g_gb, 0, pdstg_gb);
      pdstg_gb += outputPitch;
    }
  }
}

void Demosaic_pass1(short *pg_gr, short *pg_gb, int inputPitch, int inputWidth,
                    int inputHeight, short *tmp_g_r, short *tmp_g_b) {
  int x, y;
  int outputPitch = inputPitch;
  for (x = 1; x < inputWidth - 1; x += 32) {
    short *psrcg_gr = (inputPitch * 1) + pg_gr + x;
    short *psrcg_gb = (inputPitch * 1) + pg_gb + x;
    short *pdst_tmp_g_r = (inputPitch * 1) + tmp_g_r + x;
    short *pdst_tmp_g_b = (inputPitch * 1) + tmp_g_b + x;

    for (y = 1; y < inputHeight - 1; y++) {
      short32 g_gr_c = vload32(0, psrcg_gr);
      short32 g_gr_nc = vload32(0, psrcg_gr + 1);
      short32 g_gr_nr = vload32(0, psrcg_gr + inputPitch);
      psrcg_gr += inputPitch;

      short32 g_gb_c = vload32(0, psrcg_gb);
      short32 g_gb_pr = vload32(0, psrcg_gb - inputPitch);
      short32 g_gb_pc = vload32(0, psrcg_gb - 1);
      psrcg_gb += inputPitch;

      short32 gv_r = avg(g_gb_pr, g_gb_c);
      ushort32 gvd_r = abs_diff(g_gb_pr, g_gb_c);
      short32 gh_r = avg(g_gr_nc, g_gr_c);
      ushort32 ghd_r = abs_diff(g_gr_nc, g_gr_c);

      short32 g_r = ghd_r < gvd_r ? gh_r : gv_r;

      short32 gv_b = avg(g_gr_nr, g_gr_c);
      ushort32 gvd_b = abs_diff(g_gr_nr, g_gr_c);
      short32 gh_b = avg(g_gb_pc, g_gb_c);
      ushort32 ghd_b = abs_diff(g_gb_pc, g_gb_c);

      short32 g_b = ghd_b < gvd_b ? gh_b : gv_b;

      vstore32(g_r, 0, pdst_tmp_g_r);
      pdst_tmp_g_r += outputPitch;

      vstore32(g_b, 0, pdst_tmp_g_b);
      pdst_tmp_g_b += outputPitch;
    }
  }
}

void Demosaic_pass2(short *pr_r, short *pb_b, short *pg_gr, short *pg_gb,
                    int inputPitch_deinterleave, short *tmp_g_r, short *tmp_g_b,
                    int inputWidth, int inputHeight, short *tmp_r_gr,
                    short *tmp_b_gr, short *tmp_r_gb, short *tmp_b_gb,
                    short *tmp_r_b, short *tmp_b_r) {
  int x, y;
  int outputPitch = inputPitch_deinterleave;
  for (x = 2; x < inputWidth - 2; x += 32) {
    short *psrcr_r = (2 * inputPitch_deinterleave) + pr_r + x;
    short *psrcb_b = (2 * inputPitch_deinterleave) + pb_b + x;
    short *psrcg_gb = (2 * inputPitch_deinterleave) + pg_gb + x;
    short *psrcg_gr = (2 * inputPitch_deinterleave) + pg_gr + x;

    short *psrcg_r = (2 * inputPitch_deinterleave) + tmp_g_r + x;
    short *psrcg_b = (2 * inputPitch_deinterleave) + tmp_g_b + x;

    short *pdst_tmp_r_gr = (2 * inputPitch_deinterleave) + tmp_r_gr + x;
    short *pdst_tmp_b_gr = (2 * inputPitch_deinterleave) + tmp_b_gr + x;
    short *pdst_tmp_r_gb = (2 * inputPitch_deinterleave) + tmp_r_gb + x;
    short *pdst_tmp_b_gb = (2 * inputPitch_deinterleave) + tmp_b_gb + x;
    short *pdst_tmp_r_b = (2 * inputPitch_deinterleave) + tmp_r_b + x;
    short *pdst_tmp_b_r = (2 * inputPitch_deinterleave) + tmp_b_r + x;

    for (y = 2; y < inputHeight - 2; y++) {
      short32 g_gr_c = vload32(0, psrcg_gr);
      psrcg_gr += inputPitch_deinterleave;
      short32 g_gb_c = vload32(0, psrcg_gb);
      psrcg_gb += inputPitch_deinterleave;

      short32 g_r_c = vload32(0, psrcg_r);
      short32 g_r_pc = vload32(0, psrcg_r - 1);
      short32 g_r_nr = vload32(0, psrcg_r + inputPitch_deinterleave);
      short32 g_r_pc_nr = vload32(0, psrcg_r - 1 + inputPitch_deinterleave);
      psrcg_r += inputPitch_deinterleave;

      short32 r_r_c = vload32(0, psrcr_r);
      short32 r_r_pc = vload32(0, psrcr_r - 1);
      short32 r_r_nr = vload32(0, psrcr_r + inputPitch_deinterleave);
      short32 r_r_pc_nr = vload32(0, psrcr_r - 1 + inputPitch_deinterleave);
      psrcr_r += inputPitch_deinterleave;

      short32 g_b_c = vload32(0, psrcg_b);
      short32 g_b_pr = vload32(0, psrcg_b - inputPitch_deinterleave);
      short32 g_b_nc = vload32(0, psrcg_b + 1);
      short32 g_b_nc_pr = vload32(0, psrcg_b + 1 - inputPitch_deinterleave);
      psrcg_b += inputPitch_deinterleave;

      short32 b_b_c = vload32(0, psrcb_b);
      short32 b_b_pr = vload32(0, psrcb_b - inputPitch_deinterleave);
      short32 b_b_nc = vload32(0, psrcb_b + 1);
      short32 b_b_nc_pr = vload32(0, psrcb_b + 1 - inputPitch_deinterleave);
      psrcb_b += inputPitch_deinterleave;

      short32 correction;
      correction = g_gr_c - avg(g_r_c, g_r_pc);
      short32 r_gr = correction + avg(r_r_pc, r_r_c);

      correction = g_gb_c - avg(g_r_c, g_r_nr);
      short32 r_gb = correction + avg(r_r_c, r_r_nr);

      vstore32(r_gr, 0, pdst_tmp_r_gr);
      pdst_tmp_r_gr += outputPitch;

      vstore32(r_gb, 0, pdst_tmp_r_gb);
      pdst_tmp_r_gb += outputPitch;
    }
  }

  for (x = 2; x < inputWidth - 2; x += 32) {
    short *psrcr_r = (2 * inputPitch_deinterleave) + pr_r + x;
    short *psrcb_b = (2 * inputPitch_deinterleave) + pb_b + x;
    short *psrcg_gb = (2 * inputPitch_deinterleave) + pg_gb + x;
    short *psrcg_gr = (2 * inputPitch_deinterleave) + pg_gr + x;

    short *psrcg_r = (2 * inputPitch_deinterleave) + tmp_g_r + x;
    short *psrcg_b = (2 * inputPitch_deinterleave) + tmp_g_b + x;

    short *pdst_tmp_r_gr = (2 * inputPitch_deinterleave) + tmp_r_gr + x;
    short *pdst_tmp_b_gr = (2 * inputPitch_deinterleave) + tmp_b_gr + x;
    short *pdst_tmp_r_gb = (2 * inputPitch_deinterleave) + tmp_r_gb + x;
    short *pdst_tmp_b_gb = (2 * inputPitch_deinterleave) + tmp_b_gb + x;
    short *pdst_tmp_r_b = (2 * inputPitch_deinterleave) + tmp_r_b + x;
    short *pdst_tmp_b_r = (2 * inputPitch_deinterleave) + tmp_b_r + x;

    for (y = 2; y < inputHeight - 2; y++) {
      short32 g_gr_c = vload32(0, psrcg_gr);
      psrcg_gr += inputPitch_deinterleave;
      short32 g_gb_c = vload32(0, psrcg_gb);
      psrcg_gb += inputPitch_deinterleave;

      short32 g_r_c = vload32(0, psrcg_r);
      short32 g_r_pc = vload32(0, psrcg_r - 1);
      short32 g_r_nr = vload32(0, psrcg_r + inputPitch_deinterleave);
      short32 g_r_pc_nr = vload32(0, psrcg_r - 1 + inputPitch_deinterleave);
      psrcg_r += inputPitch_deinterleave;

      short32 r_r_c = vload32(0, psrcr_r);
      short32 r_r_pc = vload32(0, psrcr_r - 1);
      short32 r_r_nr = vload32(0, psrcr_r + inputPitch_deinterleave);
      short32 r_r_pc_nr = vload32(0, psrcr_r - 1 + inputPitch_deinterleave);
      psrcr_r += inputPitch_deinterleave;

      short32 g_b_c = vload32(0, psrcg_b);
      short32 g_b_pr = vload32(0, psrcg_b - inputPitch_deinterleave);
      short32 g_b_nc = vload32(0, psrcg_b + 1);
      short32 g_b_nc_pr = vload32(0, psrcg_b + 1 - inputPitch_deinterleave);
      psrcg_b += inputPitch_deinterleave;

      short32 b_b_c = vload32(0, psrcb_b);
      short32 b_b_pr = vload32(0, psrcb_b - inputPitch_deinterleave);
      short32 b_b_nc = vload32(0, psrcb_b + 1);
      short32 b_b_nc_pr = vload32(0, psrcb_b + 1 - inputPitch_deinterleave);
      psrcb_b += inputPitch_deinterleave;

      short32 correction;

      // Do the same for other reds and blues at green sites
      correction = g_gr_c - avg(g_b_c, g_b_pr);
      short32 b_gr = correction + avg(b_b_c, b_b_pr);

      correction = g_gb_c - avg(g_b_c, g_b_nc);
      short32 b_gb = correction + avg(b_b_c, b_b_nc);

      vstore32(b_gr, 0, pdst_tmp_b_gr);
      pdst_tmp_b_gr += outputPitch;

      vstore32(b_gb, 0, pdst_tmp_b_gb);
      pdst_tmp_b_gb += outputPitch;
    }
  }

  for (x = 2; x < inputWidth - 2; x += 32) {
    short *psrcr_r = (2 * inputPitch_deinterleave) + pr_r + x;
    short *psrcb_b = (2 * inputPitch_deinterleave) + pb_b + x;
    short *psrcg_gb = (2 * inputPitch_deinterleave) + pg_gb + x;
    short *psrcg_gr = (2 * inputPitch_deinterleave) + pg_gr + x;

    short *psrcg_r = (2 * inputPitch_deinterleave) + tmp_g_r + x;
    short *psrcg_b = (2 * inputPitch_deinterleave) + tmp_g_b + x;

    short *pdst_tmp_r_gr = (2 * inputPitch_deinterleave) + tmp_r_gr + x;
    short *pdst_tmp_b_gr = (2 * inputPitch_deinterleave) + tmp_b_gr + x;
    short *pdst_tmp_r_gb = (2 * inputPitch_deinterleave) + tmp_r_gb + x;
    short *pdst_tmp_b_gb = (2 * inputPitch_deinterleave) + tmp_b_gb + x;
    short *pdst_tmp_r_b = (2 * inputPitch_deinterleave) + tmp_r_b + x;
    short *pdst_tmp_b_r = (2 * inputPitch_deinterleave) + tmp_b_r + x;

    for (y = 2; y < inputHeight - 2; y++) {
      short32 g_gr_c = vload32(0, psrcg_gr);
      psrcg_gr += inputPitch_deinterleave;
      short32 g_gb_c = vload32(0, psrcg_gb);
      psrcg_gb += inputPitch_deinterleave;

      short32 g_r_c = vload32(0, psrcg_r);
      short32 g_r_pc = vload32(0, psrcg_r - 1);
      short32 g_r_nr = vload32(0, psrcg_r + inputPitch_deinterleave);
      short32 g_r_pc_nr = vload32(0, psrcg_r - 1 + inputPitch_deinterleave);
      psrcg_r += inputPitch_deinterleave;

      short32 r_r_c = vload32(0, psrcr_r);
      short32 r_r_pc = vload32(0, psrcr_r - 1);
      short32 r_r_nr = vload32(0, psrcr_r + inputPitch_deinterleave);
      short32 r_r_pc_nr = vload32(0, psrcr_r - 1 + inputPitch_deinterleave);
      psrcr_r += inputPitch_deinterleave;

      short32 g_b_c = vload32(0, psrcg_b);
      short32 g_b_pr = vload32(0, psrcg_b - inputPitch_deinterleave);
      short32 g_b_nc = vload32(0, psrcg_b + 1);
      short32 g_b_nc_pr = vload32(0, psrcg_b + 1 - inputPitch_deinterleave);
      psrcg_b += inputPitch_deinterleave;

      short32 b_b_c = vload32(0, psrcb_b);
      short32 b_b_pr = vload32(0, psrcb_b - inputPitch_deinterleave);
      short32 b_b_nc = vload32(0, psrcb_b + 1);
      short32 b_b_nc_pr = vload32(0, psrcb_b + 1 - inputPitch_deinterleave);
      psrcb_b += inputPitch_deinterleave;

      short32 correction;

      correction = g_b_c - avg(g_r_c, g_r_pc_nr);
      short32 rp_b = correction + avg(r_r_c, r_r_pc_nr);
      ushort32 rpd_b = abs_diff(r_r_c, r_r_pc_nr);

      correction = g_b_c - avg(g_r_pc, g_r_nr);
      short32 rn_b = correction + avg(r_r_pc, r_r_nr);
      ushort32 rnd_b = abs_diff(r_r_pc, r_r_nr);

      short32 r_b = rpd_b < rnd_b ? rp_b : rn_b;

      vstore32(r_b, 0, pdst_tmp_r_b);
      pdst_tmp_r_b += outputPitch;
    }
  }

  for (x = 2; x < inputWidth - 2; x += 32) {
    short *psrcr_r = (2 * inputPitch_deinterleave) + pr_r + x;
    short *psrcb_b = (2 * inputPitch_deinterleave) + pb_b + x;
    short *psrcg_gb = (2 * inputPitch_deinterleave) + pg_gb + x;
    short *psrcg_gr = (2 * inputPitch_deinterleave) + pg_gr + x;

    short *psrcg_r = (2 * inputPitch_deinterleave) + tmp_g_r + x;
    short *psrcg_b = (2 * inputPitch_deinterleave) + tmp_g_b + x;

    short *pdst_tmp_r_gr = (2 * inputPitch_deinterleave) + tmp_r_gr + x;
    short *pdst_tmp_b_gr = (2 * inputPitch_deinterleave) + tmp_b_gr + x;
    short *pdst_tmp_r_gb = (2 * inputPitch_deinterleave) + tmp_r_gb + x;
    short *pdst_tmp_b_gb = (2 * inputPitch_deinterleave) + tmp_b_gb + x;
    short *pdst_tmp_r_b = (2 * inputPitch_deinterleave) + tmp_r_b + x;
    short *pdst_tmp_b_r = (2 * inputPitch_deinterleave) + tmp_b_r + x;

    for (y = 2; y < inputHeight - 2; y++) {
      short32 g_gr_c = vload32(0, psrcg_gr);
      psrcg_gr += inputPitch_deinterleave;
      short32 g_gb_c = vload32(0, psrcg_gb);
      psrcg_gb += inputPitch_deinterleave;

      short32 g_r_c = vload32(0, psrcg_r);
      short32 g_r_pc = vload32(0, psrcg_r - 1);
      short32 g_r_nr = vload32(0, psrcg_r + inputPitch_deinterleave);
      short32 g_r_pc_nr = vload32(0, psrcg_r - 1 + inputPitch_deinterleave);
      psrcg_r += inputPitch_deinterleave;

      short32 r_r_c = vload32(0, psrcr_r);
      short32 r_r_pc = vload32(0, psrcr_r - 1);
      short32 r_r_nr = vload32(0, psrcr_r + inputPitch_deinterleave);
      short32 r_r_pc_nr = vload32(0, psrcr_r - 1 + inputPitch_deinterleave);
      psrcr_r += inputPitch_deinterleave;

      short32 g_b_c = vload32(0, psrcg_b);
      short32 g_b_pr = vload32(0, psrcg_b - inputPitch_deinterleave);
      short32 g_b_nc = vload32(0, psrcg_b + 1);
      short32 g_b_nc_pr = vload32(0, psrcg_b + 1 - inputPitch_deinterleave);
      psrcg_b += inputPitch_deinterleave;

      short32 b_b_c = vload32(0, psrcb_b);
      short32 b_b_pr = vload32(0, psrcb_b - inputPitch_deinterleave);
      short32 b_b_nc = vload32(0, psrcb_b + 1);
      short32 b_b_nc_pr = vload32(0, psrcb_b + 1 - inputPitch_deinterleave);
      psrcb_b += inputPitch_deinterleave;

      short32 correction;

      correction = g_r_c - avg(g_b_c, g_b_nc_pr);
      short32 bp_r = correction + avg(b_b_c, b_b_nc_pr);
      ushort32 bpd_r = abs_diff(b_b_c, b_b_nc_pr);

      correction = g_r_c - avg(g_b_nc, g_b_pr);
      short32 bn_r = correction + avg(b_b_nc, b_b_pr);
      ushort32 bnd_r = abs_diff(b_b_nc, b_b_pr);

      short32 b_r = bpd_r < bnd_r ? bp_r : bn_r;

      vstore32(b_r, 0, pdst_tmp_b_r);
      pdst_tmp_b_r += outputPitch;
    }
  }
}

void Rearrange(short *pr_r, short *pb_b, short *pg_gr, short *pg_gb,
               int inputPitch_deinterleave, short *tmp_g_r, short *tmp_g_b,
               short *tmp_r_gr, short *tmp_b_gr, short *tmp_r_gb,
               short *tmp_b_gb, short *tmp_r_b, short *tmp_b_r, int inputWidth,
               int inputHeight) {
  int x, y;
  ushort32 interleaveA = {0,  32, 1,  33, 2,  34, 3,  35, 4,  36, 5,
                          37, 6,  38, 7,  39, 8,  40, 9,  41, 10, 42,
                          11, 43, 12, 44, 13, 45, 14, 46, 15, 47};
  ushort32 interleaveB = interleaveA + (ushort32)16;

  for (x = 2; x < inputWidth - 2; x += 32) {
    short *psrcr_r = (2 * inputPitch_deinterleave) + pr_r + x;
    short *psrcb_b = (2 * inputPitch_deinterleave) + pb_b + x;
    short *psrcg_gb = (2 * inputPitch_deinterleave) + pg_gb + x;
    short *psrcg_gr = (2 * inputPitch_deinterleave) + pg_gr + x;
    short *psrc_tmp_g_r = (2 * inputPitch_deinterleave) + tmp_g_r + x;
    short *psrc_tmp_g_b = (2 * inputPitch_deinterleave) + tmp_g_b + x;
    short *psrc_tmp_r_gr = (2 * inputPitch_deinterleave) + tmp_r_gr + x;
    short *psrc_tmp_b_gr = (2 * inputPitch_deinterleave) + tmp_b_gr + x;
    short *psrc_tmp_r_gb = (2 * inputPitch_deinterleave) + tmp_r_gb + x;
    short *psrc_tmp_b_gb = (2 * inputPitch_deinterleave) + tmp_b_gb + x;
    short *psrc_tmp_r_b = (2 * inputPitch_deinterleave) + tmp_r_b + x;
    short *psrc_tmp_b_r = (2 * inputPitch_deinterleave) + tmp_b_r + x;

    for (y = 2; y < inputHeight - 2; y++) {
      short32 r_gr = vload32(0, psrc_tmp_r_gr);
      short32 r_r = vload32(0, psrcr_r);
      short32 g_gr = vload32(0, psrcg_gr);
      short32 g_r = vload32(0, psrc_tmp_g_r);
      short32 b_gr = vload32(0, psrc_tmp_b_gr);
      short32 b_r = vload32(0, psrc_tmp_b_r);

      short32 r00 = shuffle2(r_gr, r_r, interleaveA);
      short32 r01 = shuffle2(r_gr, r_r, interleaveB);

      short32 g00 = shuffle2(g_gr, g_r, interleaveA);
      short32 g01 = shuffle2(g_gr, g_r, interleaveB);

      short32 b00 = shuffle2(b_gr, b_r, interleaveA);
      short32 b01 = shuffle2(b_gr, b_r, interleaveB);

      vstore32((r00), 0, psrc_tmp_r_gr);
      vstore32((r01), 0, psrcr_r);
      vstore32((g00), 0, psrcg_gr);
      vstore32((g01), 0, psrc_tmp_g_r);
      vstore32((b00), 0, psrc_tmp_b_gr);
      vstore32((b01), 0, psrc_tmp_b_r);

      psrc_tmp_r_gr += inputPitch_deinterleave;
      psrcr_r += inputPitch_deinterleave;
      psrcg_gr += inputPitch_deinterleave;
      psrc_tmp_g_r += inputPitch_deinterleave;
      psrc_tmp_b_gr += inputPitch_deinterleave;
      psrc_tmp_b_r += inputPitch_deinterleave;
    }
  }

  for (x = 2; x < inputWidth - 2; x += 32) {
    short *psrcr_r = (2 * inputPitch_deinterleave) + pr_r + x;
    short *psrcb_b = (2 * inputPitch_deinterleave) + pb_b + x;
    short *psrcg_gb = (2 * inputPitch_deinterleave) + pg_gb + x;
    short *psrcg_gr = (2 * inputPitch_deinterleave) + pg_gr + x;
    short *psrc_tmp_g_r = (2 * inputPitch_deinterleave) + tmp_g_r + x;
    short *psrc_tmp_g_b = (2 * inputPitch_deinterleave) + tmp_g_b + x;
    short *psrc_tmp_r_gr = (2 * inputPitch_deinterleave) + tmp_r_gr + x;
    short *psrc_tmp_b_gr = (2 * inputPitch_deinterleave) + tmp_b_gr + x;
    short *psrc_tmp_r_gb = (2 * inputPitch_deinterleave) + tmp_r_gb + x;
    short *psrc_tmp_b_gb = (2 * inputPitch_deinterleave) + tmp_b_gb + x;
    short *psrc_tmp_r_b = (2 * inputPitch_deinterleave) + tmp_r_b + x;
    short *psrc_tmp_b_r = (2 * inputPitch_deinterleave) + tmp_b_r + x;

    for (y = 2; y < inputHeight - 2; y++) {
      short32 r_b = vload32(0, psrc_tmp_r_b);
      short32 r_gb = vload32(0, psrc_tmp_r_gb);
      short32 g_b = vload32(0, psrc_tmp_g_b);
      short32 g_gb = vload32(0, psrcg_gb);
      short32 b_b = vload32(0, psrcb_b);
      short32 b_gb = vload32(0, psrc_tmp_b_gb);

      short32 r10 = shuffle2(r_b, r_gb, interleaveA);
      short32 r11 = shuffle2(r_b, r_gb, interleaveB);

      short32 g10 = shuffle2(g_b, g_gb, interleaveA);
      short32 g11 = shuffle2(g_b, g_gb, interleaveB);

      short32 b10 = shuffle2(b_b, b_gb, interleaveA);
      short32 b11 = shuffle2(b_b, b_gb, interleaveB);

      vstore32((r10), 0, psrc_tmp_r_b);
      vstore32((r11), 0, psrc_tmp_r_gb);
      vstore32((g10), 0, psrc_tmp_g_b);
      vstore32((g11), 0, psrcg_gb);
      vstore32((b10), 0, psrcb_b);
      vstore32((b11), 0, psrc_tmp_b_gb);

      psrc_tmp_r_b += inputPitch_deinterleave;
      psrc_tmp_r_gb += inputPitch_deinterleave;
      psrc_tmp_g_b += inputPitch_deinterleave;
      psrcg_gb += inputPitch_deinterleave;
      psrcb_b += inputPitch_deinterleave;
      psrc_tmp_b_gb += inputPitch_deinterleave;
    }
  }
}

void apply_curve(short *pr_r, short *pb_b, short *pg_gr, short *pg_gb,
                 int inputPitch_deinterleave, short *tmp_g_r, short *tmp_g_b,
                 short *tmp_r_gr, short *tmp_b_gr, short *tmp_r_gb,
                 short *tmp_b_gb, short *tmp_r_b, short *tmp_b_r,
                 int inputWidth, int inputHeight, unsigned char *output,
                 int outputPitch, __local unsigned char *local_src_curve,
                 __local short *local_src_matrix) {
  int x, y;

  const ushort32 InterleaveRGa = {0,  32, 1,  33, 2,  34, 3,  35, 4,  36, 5,
                                  37, 6,  38, 7,  39, 8,  40, 9,  41, 10, 42,
                                  11, 43, 12, 44, 13, 45, 14, 46, 15, 47};

  const ushort32 InterleaveRGb = {11, 43, 12, 44, 13, 45, 14, 46, 15, 47, 16,
                                  48, 17, 49, 18, 50, 19, 51, 20, 52, 21, 53,
                                  22, 54, 23, 55, 24, 56, 25, 57, 26, 58};

  const ushort32 InterleaveRGc = {53, 22, 54, 23, 55, 24, 56, 25, 57, 26, 58,
                                  27, 59, 28, 60, 29, 61, 30, 62, 31, 63, 0,
                                  0,  0,  0,  0,  0,  0,  0,  0,  0,  0};

  const ushort32 Mask0 = {0,  1,  32, 2,  3,  33, 4,  5,  34, 6,  7,
                          35, 8,  9,  36, 10, 11, 37, 12, 13, 38, 14,
                          15, 39, 16, 17, 40, 18, 19, 41, 20, 21};
  const ushort32 Mask1 = {42, 0,  1,  43, 2,  3,  44, 4,  5,  45, 6,
                          7,  46, 8,  9,  47, 10, 11, 48, 12, 13, 49,
                          14, 15, 50, 16, 17, 51, 18, 19, 52, 20};

  const ushort32 Mask2 = {0,  53, 1,  2,  54, 3,  4,  55, 5,  6,  56,
                          7,  8,  57, 9,  10, 58, 11, 12, 59, 13, 14,
                          60, 15, 16, 61, 17, 18, 62, 19, 20, 63};

  for (x = 2; x < inputWidth - 2; x += 32) {
    short *psrcr_r = (2 * inputPitch_deinterleave) + pr_r + x;
    short *psrcb_b = (2 * inputPitch_deinterleave) + pb_b + x;
    short *psrcg_gb = (2 * inputPitch_deinterleave) + pg_gb + x;
    short *psrcg_gr = (2 * inputPitch_deinterleave) + pg_gr + x;
    short *psrc_tmp_g_r = (2 * inputPitch_deinterleave) + tmp_g_r + x;
    short *psrc_tmp_g_b = (2 * inputPitch_deinterleave) + tmp_g_b + x;

    short *psrc_tmp_r_gr = (2 * inputPitch_deinterleave) + tmp_r_gr + x;
    short *psrc_tmp_b_gr = (2 * inputPitch_deinterleave) + tmp_b_gr + x;
    short *psrc_tmp_r_gb = (2 * inputPitch_deinterleave) + tmp_r_gb + x;
    short *psrc_tmp_b_gb = (2 * inputPitch_deinterleave) + tmp_b_gb + x;
    short *psrc_tmp_r_b = (2 * inputPitch_deinterleave) + tmp_r_b + x;
    short *psrc_tmp_b_r = (2 * inputPitch_deinterleave) + tmp_b_r + x;

    uchar *pdstrgb = output + 2 * (x - 2) * 3;

    for (y = 2; y < inputHeight - 2; y += 1) {
      short32 r00 = vload32(0, psrc_tmp_r_gr);
      short32 g00 = vload32(0, psrcg_gr);
      short32 b00 = vload32(0, psrc_tmp_b_gr);

      psrcg_gr += inputPitch_deinterleave;
      psrc_tmp_r_gr += inputPitch_deinterleave;
      psrc_tmp_b_gr += inputPitch_deinterleave;

      int32 R_00_ = xt_mul32((short32)local_src_matrix[3], (short32)1) +
                    xt_mul32((short32)local_src_matrix[0], r00) +
                    xt_mul32((short32)local_src_matrix[1], g00) +
                    xt_mul32((short32)local_src_matrix[2], b00);

      int32 G_00_ = xt_mul32((short32)local_src_matrix[3 + 4 * 1], (short32)1) +
                    xt_mul32((short32)local_src_matrix[0 + 4 * 1], r00) +
                    xt_mul32((short32)local_src_matrix[1 + 4 * 1], g00) +
                    xt_mul32((short32)local_src_matrix[2 + 4 * 1], b00);

      int32 B_00_ = xt_mul32((short32)local_src_matrix[3 + 4 * 2], (short32)1) +
                    xt_mul32((short32)local_src_matrix[0 + 4 * 2], r00) +
                    xt_mul32((short32)local_src_matrix[1 + 4 * 2], g00) +
                    xt_mul32((short32)local_src_matrix[2 + 4 * 2], b00);

      R_00_ = R_00_ >> 8;
      G_00_ = G_00_ >> 8;
      B_00_ = B_00_ >> 8;

      short32 _R00 = convert_short32(R_00_);
      short32 _G00 = convert_short32(G_00_);
      short32 _B00 = convert_short32(B_00_);

      _R00 = clamp(_R00, (short32)0, (short32)1023);
      _G00 = clamp(_G00, (short32)0, (short32)1023);
      _B00 = clamp(_B00, (short32)0, (short32)1023);

      ushort32 R00 =
          convert_ushort32(xt_gather32(local_src_curve, (ushort *)&_R00));
      ushort32 G00 =
          convert_ushort32(xt_gather32(local_src_curve, (ushort *)&_G00));
      ushort32 B00 =
          convert_ushort32(xt_gather32(local_src_curve, (ushort *)&_B00));

      ushort32 xtmp = shuffle2(R00, G00, InterleaveRGa);
      ushort32 xOut0a = shuffle2(xtmp, B00, Mask0);
      xtmp = shuffle2(R00, G00, InterleaveRGb);
      ushort32 xOut0b = shuffle2(xtmp, B00, Mask1);
      xtmp = shuffle2(R00, G00, InterleaveRGc);
      ushort32 xOut0c = shuffle2(xtmp, B00, Mask2);

      vstore32(convert_uchar32(xOut0a), 0, pdstrgb);
      vstore32(convert_uchar32(xOut0b), 0, pdstrgb + 32);
      vstore32(convert_uchar32(xOut0c), 0, pdstrgb + 64);

      pdstrgb += 2 * outputPitch;
    }
  }

  for (x = 2; x < inputWidth - 2; x += 32) {
    short *psrcr_r = (2 * inputPitch_deinterleave) + pr_r + x;
    short *psrcb_b = (2 * inputPitch_deinterleave) + pb_b + x;
    short *psrcg_gb = (2 * inputPitch_deinterleave) + pg_gb + x;
    short *psrcg_gr = (2 * inputPitch_deinterleave) + pg_gr + x;
    short *psrc_tmp_g_r = (2 * inputPitch_deinterleave) + tmp_g_r + x;
    short *psrc_tmp_g_b = (2 * inputPitch_deinterleave) + tmp_g_b + x;

    short *psrc_tmp_r_gr = (2 * inputPitch_deinterleave) + tmp_r_gr + x;
    short *psrc_tmp_b_gr = (2 * inputPitch_deinterleave) + tmp_b_gr + x;
    short *psrc_tmp_r_gb = (2 * inputPitch_deinterleave) + tmp_r_gb + x;
    short *psrc_tmp_b_gb = (2 * inputPitch_deinterleave) + tmp_b_gb + x;
    short *psrc_tmp_r_b = (2 * inputPitch_deinterleave) + tmp_r_b + x;
    short *psrc_tmp_b_r = (2 * inputPitch_deinterleave) + tmp_b_r + x;

    uchar *pdstrgb = output + 2 * (x - 2) * 3 + 32 * 3;

    for (y = 2; y < inputHeight - 2; y += 1) {
      short32 r01 = vload32(0, psrcr_r);
      short32 g01 = vload32(0, psrc_tmp_g_r);
      short32 b01 = vload32(0, psrc_tmp_b_r);

      psrcr_r += inputPitch_deinterleave;
      psrc_tmp_g_r += inputPitch_deinterleave;
      psrc_tmp_b_r += inputPitch_deinterleave;

      int32 R_01_ = xt_mul32((short32)local_src_matrix[3], (short32)1) +
                    xt_mul32((short32)local_src_matrix[0], r01) +
                    xt_mul32((short32)local_src_matrix[1], g01) +
                    xt_mul32((short32)local_src_matrix[2], b01);

      int32 G_01_ = xt_mul32((short32)local_src_matrix[3 + 4 * 1], (short32)1) +
                    xt_mul32((short32)local_src_matrix[0 + 4 * 1], r01) +
                    xt_mul32((short32)local_src_matrix[1 + 4 * 1], g01) +
                    xt_mul32((short32)local_src_matrix[2 + 4 * 1], b01);

      int32 B_01_ = xt_mul32((short32)local_src_matrix[3 + 4 * 2], (short32)1) +
                    xt_mul32((short32)local_src_matrix[0 + 4 * 2], r01) +
                    xt_mul32((short32)local_src_matrix[1 + 4 * 2], g01) +
                    xt_mul32((short32)local_src_matrix[2 + 4 * 2], b01);

      R_01_ = R_01_ >> 8;
      G_01_ = G_01_ >> 8;
      B_01_ = B_01_ >> 8;

      short32 _R01 = convert_short32(R_01_);
      short32 _G01 = convert_short32(G_01_);
      short32 _B01 = convert_short32(B_01_);

      _R01 = clamp(_R01, (short32)0, (short32)1023);
      _G01 = clamp(_G01, (short32)0, (short32)1023);
      _B01 = clamp(_B01, (short32)0, (short32)1023);

      ushort32 R01 =
          convert_ushort32(xt_gather32(local_src_curve, (ushort *)&_R01));
      ushort32 G01 =
          convert_ushort32(xt_gather32(local_src_curve, (ushort *)&_G01));
      ushort32 B01 =
          convert_ushort32(xt_gather32(local_src_curve, (ushort *)&_B01));

      ushort32 xtmp = shuffle2(R01, G01, InterleaveRGa);
      ushort32 xOut1a = shuffle2(xtmp, B01, Mask0);
      xtmp = shuffle2(R01, G01, InterleaveRGb);
      ushort32 xOut1b = shuffle2(xtmp, B01, Mask1);
      xtmp = shuffle2(R01, G01, InterleaveRGc);
      ushort32 xOut1c = shuffle2(xtmp, B01, Mask2);

      vstore32(convert_uchar32(xOut1a), 0, pdstrgb);
      vstore32(convert_uchar32(xOut1b), 0, pdstrgb + 32);
      vstore32(convert_uchar32(xOut1c), 0, pdstrgb + 64);

      pdstrgb += 2 * outputPitch;
    }
  }

  for (x = 2; x < inputWidth - 2; x += 32) {
    short *psrcr_r = (2 * inputPitch_deinterleave) + pr_r + x;
    short *psrcb_b = (2 * inputPitch_deinterleave) + pb_b + x;
    short *psrcg_gb = (2 * inputPitch_deinterleave) + pg_gb + x;
    short *psrcg_gr = (2 * inputPitch_deinterleave) + pg_gr + x;
    short *psrc_tmp_g_r = (2 * inputPitch_deinterleave) + tmp_g_r + x;
    short *psrc_tmp_g_b = (2 * inputPitch_deinterleave) + tmp_g_b + x;

    short *psrc_tmp_r_gr = (2 * inputPitch_deinterleave) + tmp_r_gr + x;
    short *psrc_tmp_b_gr = (2 * inputPitch_deinterleave) + tmp_b_gr + x;
    short *psrc_tmp_r_gb = (2 * inputPitch_deinterleave) + tmp_r_gb + x;
    short *psrc_tmp_b_gb = (2 * inputPitch_deinterleave) + tmp_b_gb + x;
    short *psrc_tmp_r_b = (2 * inputPitch_deinterleave) + tmp_r_b + x;
    short *psrc_tmp_b_r = (2 * inputPitch_deinterleave) + tmp_b_r + x;

    uchar *pdstrgb = output + outputPitch + 2 * (x - 2) * 3;

    for (y = 2; y < inputHeight - 2; y += 1) {
      short32 r10 = vload32(0, psrc_tmp_r_b);
      short32 g10 = vload32(0, psrc_tmp_g_b);
      short32 b10 = vload32(0, psrcb_b);

      psrcb_b += inputPitch_deinterleave;
      psrc_tmp_r_b += inputPitch_deinterleave;
      psrc_tmp_g_b += inputPitch_deinterleave;

      int32 R_10_ = xt_mul32((short32)local_src_matrix[3], (short32)1) +
                    xt_mul32((short32)local_src_matrix[0], r10) +
                    xt_mul32((short32)local_src_matrix[1], g10) +
                    xt_mul32((short32)local_src_matrix[2], b10);

      int32 G_10_ = xt_mul32((short32)local_src_matrix[3 + 4 * 1], (short32)1) +
                    xt_mul32((short32)local_src_matrix[0 + 4 * 1], r10) +
                    xt_mul32((short32)local_src_matrix[1 + 4 * 1], g10) +
                    xt_mul32((short32)local_src_matrix[2 + 4 * 1], b10);

      int32 B_10_ = xt_mul32((short32)local_src_matrix[3 + 4 * 2], (short32)1) +
                    xt_mul32((short32)local_src_matrix[0 + 4 * 2], r10) +
                    xt_mul32((short32)local_src_matrix[1 + 4 * 2], g10) +
                    xt_mul32((short32)local_src_matrix[2 + 4 * 2], b10);

      R_10_ = R_10_ >> 8;
      G_10_ = G_10_ >> 8;
      B_10_ = B_10_ >> 8;

      short32 _R10 = convert_short32(R_10_);
      short32 _G10 = convert_short32(G_10_);
      short32 _B10 = convert_short32(B_10_);

      _R10 = clamp(_R10, (short32)0, (short32)1023);
      _G10 = clamp(_G10, (short32)0, (short32)1023);
      _B10 = clamp(_B10, (short32)0, (short32)1023);

      ushort32 R10 =
          convert_ushort32(xt_gather32(local_src_curve, (ushort *)&_R10));
      ushort32 G10 =
          convert_ushort32(xt_gather32(local_src_curve, (ushort *)&_G10));
      ushort32 B10 =
          convert_ushort32(xt_gather32(local_src_curve, (ushort *)&_B10));

      ushort32 xtmp = shuffle2(R10, G10, InterleaveRGa);
      ushort32 xOut2a = shuffle2(xtmp, B10, Mask0);
      xtmp = shuffle2(R10, G10, InterleaveRGb);
      ushort32 xOut2b = shuffle2(xtmp, B10, Mask1);
      xtmp = shuffle2(R10, G10, InterleaveRGc);
      ushort32 xOut2c = shuffle2(xtmp, B10, Mask2);

      vstore32(convert_uchar32(xOut2a), 0, pdstrgb);
      vstore32(convert_uchar32(xOut2b), 0, pdstrgb + 32);
      vstore32(convert_uchar32(xOut2c), 0, pdstrgb + 64);
      pdstrgb += 2 * outputPitch;
    }
  }

  for (x = 2; x < inputWidth - 2; x += 32) {
    short *psrcr_r = (2 * inputPitch_deinterleave) + pr_r + x;
    short *psrcb_b = (2 * inputPitch_deinterleave) + pb_b + x;
    short *psrcg_gb = (2 * inputPitch_deinterleave) + pg_gb + x;
    short *psrcg_gr = (2 * inputPitch_deinterleave) + pg_gr + x;
    short *psrc_tmp_g_r = (2 * inputPitch_deinterleave) + tmp_g_r + x;
    short *psrc_tmp_g_b = (2 * inputPitch_deinterleave) + tmp_g_b + x;

    short *psrc_tmp_r_gr = (2 * inputPitch_deinterleave) + tmp_r_gr + x;
    short *psrc_tmp_b_gr = (2 * inputPitch_deinterleave) + tmp_b_gr + x;
    short *psrc_tmp_r_gb = (2 * inputPitch_deinterleave) + tmp_r_gb + x;
    short *psrc_tmp_b_gb = (2 * inputPitch_deinterleave) + tmp_b_gb + x;
    short *psrc_tmp_r_b = (2 * inputPitch_deinterleave) + tmp_r_b + x;
    short *psrc_tmp_b_r = (2 * inputPitch_deinterleave) + tmp_b_r + x;

    uchar *pdstrgb = output + outputPitch + 2 * (x - 2) * 3 + 32 * 3;

    for (y = 2; y < inputHeight - 2; y += 1) {
      short32 r11 = vload32(0, psrc_tmp_r_gb);
      short32 g11 = vload32(0, psrcg_gb);
      short32 b11 = vload32(0, psrc_tmp_b_gb);

      psrcg_gb += inputPitch_deinterleave;
      psrc_tmp_r_gb += inputPitch_deinterleave;
      psrc_tmp_b_gb += inputPitch_deinterleave;

      int32 R_11_ = xt_mul32((short32)local_src_matrix[3], (short32)1) +
                    xt_mul32((short32)local_src_matrix[0], r11) +
                    xt_mul32((short32)local_src_matrix[1], g11) +
                    xt_mul32((short32)local_src_matrix[2], b11);

      int32 G_11_ = xt_mul32((short32)local_src_matrix[3 + 4 * 1], (short32)1) +
                    xt_mul32((short32)local_src_matrix[0 + 4 * 1], r11) +
                    xt_mul32((short32)local_src_matrix[1 + 4 * 1], g11) +
                    xt_mul32((short32)local_src_matrix[2 + 4 * 1], b11);

      int32 B_11_ = xt_mul32((short32)local_src_matrix[3 + 4 * 2], (short32)1) +
                    xt_mul32((short32)local_src_matrix[0 + 4 * 2], r11) +
                    xt_mul32((short32)local_src_matrix[1 + 4 * 2], g11) +
                    xt_mul32((short32)local_src_matrix[2 + 4 * 2], b11);

      R_11_ = R_11_ >> 8;
      G_11_ = G_11_ >> 8;
      B_11_ = B_11_ >> 8;

      short32 _R11 = convert_short32(R_11_);
      short32 _G11 = convert_short32(G_11_);
      short32 _B11 = convert_short32(B_11_);

      _R11 = clamp(_R11, (short32)0, (short32)1023);
      _G11 = clamp(_G11, (short32)0, (short32)1023);
      _B11 = clamp(_B11, (short32)0, (short32)1023);

      ushort32 R11 =
          convert_ushort32(xt_gather32(local_src_curve, (ushort *)&_R11));
      ushort32 G11 =
          convert_ushort32(xt_gather32(local_src_curve, (ushort *)&_G11));
      ushort32 B11 =
          convert_ushort32(xt_gather32(local_src_curve, (ushort *)&_B11));

      ushort32 xtmp = shuffle2(R11, G11, InterleaveRGa);
      ushort32 xOut3a = shuffle2(xtmp, B11, Mask0);
      xtmp = shuffle2(R11, G11, InterleaveRGb);
      ushort32 xOut3b = shuffle2(xtmp, B11, Mask1);
      xtmp = shuffle2(R11, G11, InterleaveRGc);
      ushort32 xOut3c = shuffle2(xtmp, B11, Mask2);

      vstore32(convert_uchar32(xOut3a), 0, pdstrgb);
      vstore32(convert_uchar32(xOut3b), 0, pdstrgb + 32);
      vstore32(convert_uchar32(xOut3c), 0, pdstrgb + 64);
      pdstrgb += 2 * outputPitch;
    }
  }
}

void sharpen(unsigned char *input, int inputPitch, int inputWidth,
             int inputHeight, __local unsigned char *output, int outputPitch,
             unsigned char strength) {

  int x, y;
  for (x = 2 * 3; x < inputWidth - 2 * 3; x += 32) {
    __local uchar *pdstrgb = output + x - 2 * 3;
    uchar *psrcrgb = input + 1 * inputPitch + x;
    short32 x00 = convert_short32(vload32(0, psrcrgb - 3));
    short32 x01 = convert_short32(vload32(0, psrcrgb + 0));
    short32 x02 = convert_short32(vload32(0, psrcrgb + 3));
    psrcrgb += inputPitch;

    short32 x10 = convert_short32(vload32(0, psrcrgb - 3));
    short32 x11 = convert_short32(vload32(0, psrcrgb + 0));
    short32 x12 = convert_short32(vload32(0, psrcrgb + 3));
    psrcrgb += inputPitch;
    short32 xRow0 = avg(avg(x00, x02), x01);
    short32 xRow1 = avg(avg(x10, x12), x11);

    for (y = 2; y < inputHeight - 2; y += 1) {
      short32 x20 = convert_short32(vload32(0, psrcrgb - 3));
      short32 x21 = convert_short32(vload32(0, psrcrgb));
      short32 x22 = convert_short32(vload32(0, psrcrgb + 3));
      psrcrgb += inputPitch;
      short32 xRow2 = avg(avg(x20, x22), x21);

      short32 xunsharp = avg(avg(xRow0, xRow2), xRow1);

      xRow0 = xRow1;
      xRow1 = xRow2;

      short32 mask = x11 - xunsharp;

      int32 xtemp = xt_mul32(mask, (short32)strength);
      xtemp = xtemp >> 5;

      short32 xout = x11 + convert_short32(xtemp);

      xout = xout > (short32)255 ? (short32)255 : xout;
      xout = xout < (short32)0 ? (short32)0 : xout;

      vstore32(convert_uchar32(xout), 0, pdstrgb);
      pdstrgb += outputPitch;
      x11 = x21;
    }
  }
}
#endif

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclKernel(__global short *restrict Src, __global short *restrict Src_matrix,
          __global uchar *restrict Src_curve, int SrcPitch,
          __global uchar *restrict Dst, const int DstPitch, int width,
          int height, int dstWidth, int dstHeight,
          __local short *restrict local_src,
          __local short *restrict local_src_matrix,
          __local uchar *restrict local_src_curve,
          __local uchar *restrict local_dst, __global short *restrict Dbg) {
  int i, j;
  int idx, idy;
  int num_rows = TILE_H;
  int row_size = TILE_W;
  int local_src_pitch, local_dst_pitch;
  int pad = IMAGE_PAD;
  int inTilePitch;
  int inTileW, inTileH, outTileW, outTileH;
  int dstBytes, srcBytes;
  int err = 0;

  inTileW = TILE_W + 2 * W_EXT;
  inTileH = TILE_H + 2 * H_EXT;
  inTilePitch = TILE_W + 2 * W_EXT;
  local_src_pitch = row_size + pad + pad;

  outTileW = TILE_W;
  outTileH = TILE_H;
  local_dst_pitch = outTileW; // + 2*W_EXT;
  dstBytes = DST_BIT_DEPTH >> 3;
  srcBytes = SRC_BIT_DEPTH >> 3;

  i = get_global_id(0) * num_rows;
  j = get_global_id(1) * row_size;

  int i_out = get_global_id(0) * num_rows / 2;
  int j_out = get_global_id(1) * row_size / 2;

  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * SrcPitch) + j, local_src_pitch * srcBytes,
      (num_rows + pad + pad), local_src_pitch * srcBytes, SrcPitch * srcBytes,
      0);

  event_t evtB = xt_async_work_group_2d_copy(local_src_matrix, Src_matrix,
                                             12 * 2, 1, 12 * 2, 12 * 2, 0);

  event_t evtC = xt_async_work_group_2d_copy(local_src_curve, Src_curve, 1024,
                                             1, 1024, 1024, 0);
  wait_group_events(1, &evtA);
  wait_group_events(1, &evtB);
  wait_group_events(1, &evtC);

  // kernel using standard OpenCL C
#define hot_pix_output_pitch (TILE_W + 2 * 6 + 32)
  short hot_pix_output[hot_pix_output_pitch * (TILE_H + 2 * 6)];

  hot_pixel_suppression(local_src, local_src_pitch, inTileW, inTileH,
                        hot_pix_output, hot_pix_output_pitch);

#define deinterleave_output_pitch (((TILE_W + 2 * 6) / 2) + 32)
  short deinterleave_outputg_gr[deinterleave_output_pitch *
                                ((TILE_H + 2 * 6) / 2)];
  short deinterleave_outputr_r[deinterleave_output_pitch *
                               ((TILE_H + 2 * 6) / 2)];
  short deinterleave_outputb_b[deinterleave_output_pitch *
                               ((TILE_H + 2 * 6) / 2)];
  short deinterleave_outputg_gb[deinterleave_output_pitch *
                                ((TILE_H + 2 * 6) / 2)];

  deinterleave(hot_pix_output, hot_pix_output_pitch, inTileW - 4, inTileH - 4,
               deinterleave_output_pitch, deinterleave_outputr_r,
               deinterleave_outputb_b, deinterleave_outputg_gr,
               deinterleave_outputg_gb);

#define tmp_pass1_pitch deinterleave_output_pitch //(((TILE_W+2*4)/2)+32)
  short tmp_g_r[tmp_pass1_pitch * ((TILE_H + 2 * 4) / 2)];
  short tmp_g_b[tmp_pass1_pitch * ((TILE_H + 2 * 4) / 2)];

  Demosaic_pass1(deinterleave_outputg_gr, deinterleave_outputg_gb,
                 deinterleave_output_pitch, (inTileW - 4) / 2,
                 (inTileH - 4) / 2, tmp_g_r, tmp_g_b);

#define tmp_pass2_pitch deinterleave_output_pitch //(((TILE_W+2*2)/2)+32)
  short tmp_r_gr[tmp_pass2_pitch * ((TILE_H + 2 * 2) / 2)];
  short tmp_b_gr[tmp_pass2_pitch * ((TILE_H + 2 * 2) / 2)];
  short tmp_r_gb[tmp_pass2_pitch * ((TILE_H + 2 * 2) / 2)];
  short tmp_b_gb[tmp_pass2_pitch * ((TILE_H + 2 * 2) / 2)];
  short tmp_r_b[tmp_pass2_pitch * ((TILE_H + 2 * 2) / 2)];
  short tmp_b_r[tmp_pass2_pitch * ((TILE_H + 2 * 2) / 2)];

  Demosaic_pass2(deinterleave_outputr_r, deinterleave_outputb_b,
                 deinterleave_outputg_gr, deinterleave_outputg_gb,
                 deinterleave_output_pitch, tmp_g_r, tmp_g_b, (inTileW - 4) / 2,
                 (inTileH - 4) / 2, tmp_r_gr, tmp_b_gr, tmp_r_gb, tmp_b_gb,
                 tmp_r_b, tmp_b_r);

  Rearrange(deinterleave_outputr_r, deinterleave_outputb_b,
            deinterleave_outputg_gr, deinterleave_outputg_gb,
            deinterleave_output_pitch, tmp_g_r, tmp_g_b, tmp_r_gr, tmp_b_gr,
            tmp_r_gb, tmp_b_gb, tmp_r_b, tmp_b_r, (inTileW - 4) / 2,
            (inTileH - 4) / 2);

#define rgb_interleave_pitch (((TILE_W + 4 * 2) * 3) + 192)
  uchar rgb_interleave[rgb_interleave_pitch * (TILE_H + 4 * 2)];

  apply_curve(deinterleave_outputr_r, deinterleave_outputb_b,
              deinterleave_outputg_gr, deinterleave_outputg_gb,
              deinterleave_output_pitch, tmp_g_r, tmp_g_b, tmp_r_gr, tmp_b_gr,
              tmp_r_gb, tmp_b_gb, tmp_r_b, tmp_b_r, (inTileW - 4) / 2,
              (inTileH - 4) / 2, rgb_interleave, rgb_interleave_pitch,
              local_src_curve, local_src_matrix);

  sharpen(rgb_interleave, rgb_interleave_pitch, (inTileW - 12) * 3,
          (inTileH - 12), local_dst, local_dst_pitch * dstBytes, 128);

  event_t evtD = xt_async_work_group_2d_copy(
      Dst + (i * DstPitch * dstBytes) + j * dstBytes, local_dst,
      outTileW * dstBytes, num_rows, DstPitch * dstBytes,
      local_dst_pitch * dstBytes, 0);

  wait_group_events(1, &evtD);
}
