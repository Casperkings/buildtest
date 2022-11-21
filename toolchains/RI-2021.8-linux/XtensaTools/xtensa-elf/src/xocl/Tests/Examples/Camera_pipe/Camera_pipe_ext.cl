uchar64 avg_uchar64(uchar64 a, uchar64 b) { 
  return rhadd(a, b); 
}

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

      short32 r01 = vload32(0, psrcr_r);
      short32 g01 = vload32(0, psrc_tmp_g_r);
      short32 b01 = vload32(0, psrc_tmp_b_r);

      psrcg_gr += inputPitch_deinterleave;
      psrc_tmp_r_gr += inputPitch_deinterleave;
      psrc_tmp_b_gr += inputPitch_deinterleave;

      psrcr_r += inputPitch_deinterleave;
      psrc_tmp_g_r += inputPitch_deinterleave;
      psrc_tmp_b_r += inputPitch_deinterleave;

      short64 r0, g0, b0;
      r0.lo = r00; r0.hi = r01;
      g0.lo = g00; g0.hi = g01;
      b0.lo = b00; b0.hi = b01;

      int64 R_0 = xt_mul32((short64)local_src_matrix[3], (short64)1);
      R_0 = xt_mad32((short64)local_src_matrix[0], r0, R_0);
      R_0 = xt_mad32((short64)local_src_matrix[1], g0, R_0);
      R_0 = xt_mad32((short64)local_src_matrix[2], b0, R_0);

      int64 G_0 = xt_mul32((short64)local_src_matrix[3 + 4 * 1], (short64)1);
      G_0 = xt_mad32((short64)local_src_matrix[0 + 4 * 1], r0, G_0);
      G_0 = xt_mad32((short64)local_src_matrix[1 + 4 * 1], g0, G_0);
      G_0 = xt_mad32((short64)local_src_matrix[2 + 4 * 1], b0, G_0);

      int64 B_0 = xt_mul32((short64)local_src_matrix[3 + 4 * 2], (short64)1);
      B_0 = xt_mad32((short64)local_src_matrix[0 + 4 * 2], r0, B_0);
      B_0 = xt_mad32((short64)local_src_matrix[1 + 4 * 2], g0, B_0);
      B_0 = xt_mad32((short64)local_src_matrix[2 + 4 * 2], b0, B_0);

      short64 _R0 = xt_convert_short64(R_0, 8);
      short64 _G0 = xt_convert_short64(G_0, 8);
      short64 _B0 = xt_convert_short64(B_0, 8);

      _R0 = clamp(_R0, (short64)0, (short64)1023);
      _G0 = clamp(_G0, (short64)0, (short64)1023);
      _B0 = clamp(_B0, (short64)0, (short64)1023);

      uchar64 R = xt_gather64(local_src_curve, (ushort *)&_R0);
      uchar64 G = xt_gather64(local_src_curve, (ushort *)&_G0);
      uchar64 B = xt_gather64(local_src_curve, (ushort *)&_B0);
      uchar64 rgb_d[3] = {R, G, B};
      xt_interleave_x3(rgb_d, pdstrgb);

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

      short32 r11 = vload32(0, psrc_tmp_r_gb);
      short32 g11 = vload32(0, psrcg_gb);
      short32 b11 = vload32(0, psrc_tmp_b_gb);

      psrcb_b += inputPitch_deinterleave;
      psrc_tmp_r_b += inputPitch_deinterleave;
      psrc_tmp_g_b += inputPitch_deinterleave;

      psrcg_gb += inputPitch_deinterleave;
      psrc_tmp_r_gb += inputPitch_deinterleave;
      psrc_tmp_b_gb += inputPitch_deinterleave;

      short64 r1, g1, b1;
      r1.lo = r10; r1.hi = r11;
      g1.lo = g10; g1.hi = g11;
      b1.lo = b10; b1.hi = b11;

      int64 R_1 = xt_mul32((short64)local_src_matrix[3], (short64)1);
      R_1 = xt_mad32((short64)local_src_matrix[0], r1, R_1);
      R_1 = xt_mad32((short64)local_src_matrix[1], g1, R_1);
      R_1 = xt_mad32((short64)local_src_matrix[2], b1, R_1);

      int64 G_1 = xt_mul32((short64)local_src_matrix[3 + 4 * 1], (short64)1);
      G_1 = xt_mad32((short64)local_src_matrix[0 + 4 * 1], r1, G_1);
      G_1 = xt_mad32((short64)local_src_matrix[1 + 4 * 1], g1, G_1);
      G_1 = xt_mad32((short64)local_src_matrix[2 + 4 * 1], b1, G_1);

      int64 B_1 = xt_mul32((short64)local_src_matrix[3 + 4 * 2], (short64)1);
      B_1 = xt_mad32((short64)local_src_matrix[0 + 4 * 2], r1, B_1);
      B_1 = xt_mad32((short64)local_src_matrix[1 + 4 * 2], g1, B_1);
      B_1 = xt_mad32((short64)local_src_matrix[2 + 4 * 2], b1, B_1);

      short64 _R1 = xt_convert_short64(R_1, 8);
      short64 _G1 = xt_convert_short64(G_1, 8);
      short64 _B1 = xt_convert_short64(B_1, 8);

      _R1 = clamp(_R1, (short64)0, (short64)1023);
      _G1 = clamp(_G1, (short64)0, (short64)1023);
      _B1 = clamp(_B1, (short64)0, (short64)1023);

      uchar64 R = xt_gather64(local_src_curve, (ushort *)&_R1);
      uchar64 G = xt_gather64(local_src_curve, (ushort *)&_G1);
      uchar64 B = xt_gather64(local_src_curve, (ushort *)&_B1);
      uchar64 rgb_d[3] = {R, G, B};
      xt_interleave_x3(rgb_d, pdstrgb);

      pdstrgb += 2 * outputPitch;
    }
  }
}

void sharpen(unsigned char *input, int inputPitch, int inputWidth,
             int inputHeight, __local unsigned char *output, int outputPitch,
             unsigned char strength) {

  int x, y;
  for (x = 2 * 3; x < inputWidth - 2 * 3; x += 64) {
    __local uchar *pdstrgb = output + x - 2 * 3;
    uchar *psrcrgb = input + 1 * inputPitch + x;
    uchar64 x00 = vload64(0, psrcrgb - 3);
    uchar64 x01 = vload64(0, psrcrgb + 0);
    uchar64 x02 = vload64(0, psrcrgb + 3);
    psrcrgb += inputPitch;

    uchar64 x10 = vload64(0, psrcrgb - 3);
    uchar64 x11 = vload64(0, psrcrgb + 0);
    uchar64 x12 = vload64(0, psrcrgb + 3);
    psrcrgb += inputPitch;

    uchar64 xRow0 = avg_uchar64(avg_uchar64(x00, x02), x01);
    uchar64 xRow1 = avg_uchar64(avg_uchar64(x10, x12), x11);

    for (y = 2; y < inputHeight - 2; y += 1) {
      uchar64 x20 = vload64(0, psrcrgb - 3);
      uchar64 x21 = vload64(0, psrcrgb + 0);
      uchar64 x22 = vload64(0, psrcrgb + 3);
      psrcrgb += inputPitch;
      uchar64 xRow2 = avg_uchar64(avg_uchar64(x20, x22), x21);

      uchar64 xunsharp = avg_uchar64(avg_uchar64(xRow0, xRow2), xRow1);

      xRow0 = xRow1;
      xRow1 = xRow2;

      int64 accMask = xt_sub24(x11, xunsharp);
      short64 mask = xt_convert24_short64(accMask);

      int64 xtemp = xt_mul24(mask, (uchar64)strength);

      xtemp = xt_mad24(x11, (uchar64)32, xtemp);
      vstore64(xt_convert24_uchar64_sat_rnd(xtemp, 5), 0, pdstrgb);
      pdstrgb += outputPitch;
      x11 = x21;
    }
  }
}
