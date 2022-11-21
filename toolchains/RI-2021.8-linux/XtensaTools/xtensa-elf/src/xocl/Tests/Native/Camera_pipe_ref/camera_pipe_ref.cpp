/*
 * Copyright (c) 2014-2017 Tensilica Inc. ALL RIGHTS RESERVED.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "xi_api_ref.h"

#define MAX2(a, b) (a) > (b) ? (a) : (b)

#define CLAMP(x, low, high)                                                    \
  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define HOT_PIX_PAD 2

// Make a linear luminance -> pixel value lookup table
void makeLUT(float contrast, int blackLevel, float gamma, unsigned char *lut,
             unsigned short minRaw, unsigned short maxRaw) {

  for (int i = 0; i <= minRaw; i++) {
    lut[i] = 0;
  }

  float invRange = 1.0f / (maxRaw - minRaw);
  float b = 2 - powf(2.0f, contrast / 100.0f);
  float a = 2 - 2 * b;
  for (int i = minRaw + 1; i <= maxRaw; i++) {
    // Get a linear luminance in the range 0-1
    float y = (i - minRaw) * invRange;
    // Gamma correct it
    y = powf(y, 1.0f / gamma);
    // Apply a piecewise quadratic contrast curve
    if (y > 0.5) {
      y = 1 - y;
      y = a * y * y + b * y;
      y = 1 - y;
    } else {
      y = a * y * y + b * y;
    }
    // Convert to 8 bit and save
    y = floor(y * 255 + 0.5f);
    if (y < 0) {
      y = 0;
    }
    if (y > 255) {
      y = 255;
    }
    lut[i] = (unsigned char)y;
  }

  // add a guard band
  for (int i = maxRaw + 1; i < 1024; i++) {
    lut[i] = 255;
  }
}

void hot_pixel_suppression(short *input, int inputPitch, int inputWidth,
                           int inputHeight, short *output, int outputPitch) {
  int x, y;

  for (x = 2; x < inputWidth - 2; x++) {
    for (y = 2; y < inputHeight - 2; y++) {
      int max =
          MAX2(input[y * inputPitch + x - 2], input[y * inputPitch + x + 2]);
      max = MAX2(max, input[(y - 2) * inputPitch + x]);
      max = MAX2(max, input[(y + 2) * inputPitch + x]);

      output[(y - 2) * outputPitch + (x - 2)] =
          CLAMP(input[y * inputPitch + x], 0, max);
    }
  }
}

void deinterleave(short *input, int inputPitch, int inputWidth, int inputHeight,
                  short *output, int outputPitch, short *r_r, short *b_b,
                  short *g_gr, short *g_gb) {
  int x, y;

  for (x = 0; x < inputWidth - 0; x += 2) {
    for (y = 0; y < inputHeight - 0; y += 2) {
      g_gr[((y) / 2) * outputPitch + ((x) / 2)] =
          input[(y + 0) * inputPitch + x + 0];
      g_gb[((y) / 2) * outputPitch + ((x) / 2)] =
          input[(y + 1) * inputPitch + x + 1];
      r_r[((y) / 2) * outputPitch + ((x) / 2)] =
          input[(y + 0) * inputPitch + x + 1];
      b_b[((y) / 2) * outputPitch + ((x) / 2)] =
          input[(y + 1) * inputPitch + x + 0];
    }
  }
}

short tmp_g_r[2048 * 2048];
short tmp_g_b[2048 * 2048];
short tmp_r_gr[2048 * 2048];
short tmp_b_gr[2048 * 2048];
short tmp_r_gb[2048 * 2048];
short tmp_b_gb[2048 * 2048];
short tmp_r_b[2048 * 2048];
short tmp_b_r[2048 * 2048];

// Average two positive values rounding up
int avg(int a, int b) { return ((a + b + 1) >> 1); }

int blur121(int a, int b, int c) { return avg(avg(a, c), b); }

void Demosaic(short *pr_r, short *pb_b, short *pg_gr, short *pg_gb,
              int inputPitch, int inputWidth, int inputHeight, short *output,
              int outputPitch) {
  int x, y;

  for (x = 1; x < inputWidth - 1; x += 1) {
    for (y = 1; y < inputHeight - 1; y += 1) {
      int g_gr_c = pg_gr[y * inputPitch + x];
      int g_gr_nc = pg_gr[y * inputPitch + x + 1];
      int g_gr_nr = pg_gr[(y + 1) * inputPitch + x];

      int g_gb_c = pg_gb[y * inputPitch + x];
      int g_gb_pr = pg_gb[(y - 1) * inputPitch + x];
      int g_gb_pc = pg_gb[y * inputPitch + x - 1];

      int gv_r = avg(g_gb_pr, g_gb_c);
      int gvd_r = abs(g_gb_pr - g_gb_c);
      int gh_r = avg(g_gr_nc, g_gr_c);
      int ghd_r = abs(g_gr_nc - g_gr_c);

      tmp_g_r[y * inputPitch + x] = ghd_r < gvd_r ? gh_r : gv_r;

      int gv_b = avg(g_gr_nr, g_gr_c);
      int gvd_b = abs(g_gr_nr - g_gr_c);
      int gh_b = avg(g_gb_pc, g_gb_c);
      int ghd_b = abs(g_gb_pc - g_gb_c);

      tmp_g_b[y * inputPitch + x] = ghd_b < gvd_b ? gh_b : gv_b;
    }
  }

  for (x = 2; x < inputWidth - 2; x += 1) {
    for (y = 2; y < inputHeight - 2; y += 1) {
      int g_gr_c = pg_gr[y * inputPitch + x];
      int g_gb_c = pg_gb[y * inputPitch + x];

      int g_r_c = tmp_g_r[y * inputPitch + x];
      int g_r_pc = tmp_g_r[y * inputPitch + x - 1];
      int g_r_nr = tmp_g_r[(y + 1) * inputPitch + x];
      int g_r_pc_nr = tmp_g_r[(y + 1) * inputPitch + x - 1];

      int r_r_c = pr_r[y * inputPitch + x];
      int r_r_pc = pr_r[y * inputPitch + x - 1];
      int r_r_nr = pr_r[(y + 1) * inputPitch + x];
      int r_r_pc_nr = pr_r[(y + 1) * inputPitch + x - 1];

      int g_b_c = tmp_g_b[y * inputPitch + x];
      int g_b_pr = tmp_g_b[(y - 1) * inputPitch + x];
      int g_b_nc = tmp_g_b[y * inputPitch + x + 1];
      int g_b_nc_pr = tmp_g_b[(y - 1) * inputPitch + x + 1];

      int b_b_c = pb_b[y * inputPitch + x];
      int b_b_pr = pb_b[(y - 1) * inputPitch + x];
      int b_b_nc = pb_b[y * inputPitch + x + 1];
      int b_b_nc_pr = pb_b[(y - 1) * inputPitch + x + 1];

      int correction;
      correction = g_gr_c - avg(g_r_c, g_r_pc);
      tmp_r_gr[y * inputPitch + x] = correction + avg(r_r_pc, r_r_c);

      // Do the same for other reds and blues at green sites
      correction = g_gr_c - avg(g_b_c, g_b_pr);
      tmp_b_gr[y * inputPitch + x] = correction + avg(b_b_c, b_b_pr);

      correction = g_gb_c - avg(g_r_c, g_r_nr);
      tmp_r_gb[y * inputPitch + x] = correction + avg(r_r_c, r_r_nr);

      correction = g_gb_c - avg(g_b_c, g_b_nc);
      tmp_b_gb[y * inputPitch + x] = correction + avg(b_b_c, b_b_nc);

      correction = g_b_c - avg(g_r_c, g_r_pc_nr);
      int rp_b = correction + avg(r_r_c, r_r_pc_nr);
      int rpd_b = abs(r_r_c - r_r_pc_nr);

      correction = g_b_c - avg(g_r_pc, g_r_nr);
      int rn_b = correction + avg(r_r_pc, r_r_nr);
      int rnd_b = abs(r_r_pc - r_r_nr);

      tmp_r_b[y * inputPitch + x] = rpd_b < rnd_b ? rp_b : rn_b;

      correction = g_r_c - avg(g_b_c, g_b_nc_pr);
      int bp_r = correction + avg(b_b_c, b_b_nc_pr);
      int bpd_r = abs(b_b_c - b_b_nc_pr);

      correction = g_r_c - avg(g_b_nc, g_b_pr);
      int bn_r = correction + avg(b_b_nc, b_b_pr);
      int bnd_r = abs(b_b_nc - b_b_pr);

      tmp_b_r[y * inputPitch + x] = bpd_r < bnd_r ? bp_r : bn_r;
    }
  }

  for (x = 2; x < inputWidth - 2; x += 1) {
    for (y = 2; y < inputHeight - 2; y += 1) {
      int r_gr = tmp_r_gr[y * inputPitch + x];
      int r_r = pr_r[y * inputPitch + x];
      int r_b = tmp_r_b[y * inputPitch + x];
      int r_gb = tmp_r_gb[y * inputPitch + x];

      int g_gr = pg_gr[y * inputPitch + x];
      int g_r = tmp_g_r[y * inputPitch + x];
      int g_b = tmp_g_b[y * inputPitch + x];
      int g_gb = pg_gb[y * inputPitch + x];

      int b_gr = tmp_b_gr[y * inputPitch + x];
      int b_r = tmp_b_r[y * inputPitch + x];
      int b_b = pb_b[y * inputPitch + x];
      int b_gb = tmp_b_gb[y * inputPitch + x];

      output[(y - 2) * outputPitch * 2 + (x - 2) * 3 * 2 + 0] = r_gr;
      output[(y - 2) * outputPitch * 2 + (x - 2) * 3 * 2 + 1] = g_gr;
      output[(y - 2) * outputPitch * 2 + (x - 2) * 3 * 2 + 2] = b_gr;

      output[(y - 2) * outputPitch * 2 + ((x - 2) * 2 + 1) * 3 + 0] = r_r;
      output[(y - 2) * outputPitch * 2 + ((x - 2) * 2 + 1) * 3 + 1] = g_r;
      output[(y - 2) * outputPitch * 2 + ((x - 2) * 2 + 1) * 3 + 2] = b_r;

      output[((y - 2) * 2 + 1) * outputPitch + (x - 2) * 3 * 2 + 0] = r_b;
      output[((y - 2) * 2 + 1) * outputPitch + (x - 2) * 3 * 2 + 1] = g_b;
      output[((y - 2) * 2 + 1) * outputPitch + (x - 2) * 3 * 2 + 2] = b_b;

      output[((y - 2) * 2 + 1) * outputPitch + ((x - 2) * 2 + 1) * 3 + 0] =
          r_gb;
      output[((y - 2) * 2 + 1) * outputPitch + ((x - 2) * 2 + 1) * 3 + 1] =
          g_gb;
      output[((y - 2) * 2 + 1) * outputPitch + ((x - 2) * 2 + 1) * 3 + 2] =
          b_gb;
    }
  }
}

void color_correct(short *input, int inputPitch, int inputWidth,
                   int inputHeight, short *output, int outputPitch,
                   short *matrix) {
  int x, y;
  for (x = 0; x < inputWidth; x += 1) {
    for (y = 0; y < inputHeight; y += 1) {
      int ir = input[y * inputPitch + x * 3];
      int ig = input[y * inputPitch + x * 3 + 1];
      int ib = input[y * inputPitch + x * 3 + 2];

      int r = matrix[3] + matrix[0] * ir + matrix[1] * ig + matrix[2] * ib;
      int g = matrix[3 + 4 * 1] + matrix[0 + 4 * 1] * ir +
              matrix[1 + 4 * 1] * ig + matrix[2 + 4 * 1] * ib;
      int b = matrix[3 + 4 * 2] + matrix[0 + 4 * 2] * ir +
              matrix[1 + 4 * 2] * ig + matrix[2 + 4 * 2] * ib;

      r = (r >> 8);
      g = (g >> 8);
      b = (b >> 8);

      output[y * outputPitch + x * 3 + 0] = r;
      output[y * outputPitch + x * 3 + 1] = g;
      output[y * outputPitch + x * 3 + 2] = b;
    }
  }
}

void apply_curve(short *input, int inputPitch, int inputWidth, int inputHeight,
                 unsigned char *output, int outputPitch, unsigned char *curve) {
  int x, y;
  for (x = 0; x < inputWidth; x += 1) {
    for (y = 0; y < inputHeight; y += 1) {
      int ir = input[y * inputPitch + x * 3];
      int ig = input[y * inputPitch + x * 3 + 1];
      int ib = input[y * inputPitch + x * 3 + 2];

      ir = CLAMP(ir, 0, 1023);
      ig = CLAMP(ig, 0, 1023);
      ib = CLAMP(ib, 0, 1023);

      output[y * outputPitch + x * 3 + 0] = curve[ir];
      output[y * outputPitch + x * 3 + 1] = curve[ig];
      output[y * outputPitch + x * 3 + 2] = curve[ib];
    }
  }
}

void sharpen(unsigned char *input, int inputPitch, int inputWidth,
             int inputHeight, unsigned char *output, int outputPitch,
             unsigned char strength) {
  int x, y;
  for (x = 2; x < inputWidth - 2; x += 1) {
    for (y = 2; y < inputHeight - 2; y += 1) {
      for (int i = 0; i < 3; i++) {

        unsigned char unsharp_x_1 =
            blur121(input[y * inputPitch + (x - 1) * 3 + i],
                    input[y * inputPitch + x * 3 + i],
                    input[y * inputPitch + (x + 1) * 3 + i]);

        unsigned char unsharp_x_0 =
            blur121(input[(y - 1) * inputPitch + (x - 1) * 3 + i],
                    input[(y - 1) * inputPitch + x * 3 + i],
                    input[(y - 1) * inputPitch + (x + 1) * 3 + i]);

        unsigned char unsharp_x_2 =
            blur121(input[(y + 1) * inputPitch + (x - 1) * 3 + i],
                    input[(y + 1) * inputPitch + x * 3 + i],
                    input[(y + 1) * inputPitch + (x + 1) * 3 + i]);

        short unsharp = blur121(unsharp_x_0, unsharp_x_1, unsharp_x_2);

        short mask = ((short)input[y * inputPitch + x * 3 + i]) - unsharp;

        short out =
            input[y * inputPitch + x * 3 + i] + ((mask * strength) / 32);

        out = out > 255 ? 255 : out;
        out = out < 0 ? 0 : out;

        output[(y - 2) * outputPitch + (x - 2) * 3 + i] = (unsigned char)out;
      }
    }
  }
}

void camera_pipe_process(short *input, int inputPitch, int inputWidth,
                         int inputHeight, unsigned char *output,
                         int outputPitch, short *matrix, unsigned char *curve) {

  short *hot_pix_output = (short *)malloc(inputPitch * inputHeight * 2);

  hot_pixel_suppression(input, inputPitch, inputWidth, inputHeight,
                        hot_pix_output, inputPitch - 4);

  short *tmp_r_r = (short *)malloc((inputWidth - 4) * (inputHeight - 4) / 2);
  short *tmp_b_b = (short *)malloc((inputWidth - 4) * (inputHeight - 4) / 2);
  short *tmp_g_gr = (short *)malloc((inputWidth - 4) * (inputHeight - 4) / 2);
  short *tmp_g_gb = (short *)malloc((inputWidth - 4) * (inputHeight - 4) / 2);

  deinterleave(hot_pix_output, inputPitch - 4, inputWidth - 4, inputHeight - 4,
               hot_pix_output, (inputWidth - 4) / 2, tmp_r_r, tmp_b_b, tmp_g_gr,
               tmp_g_gb);

  short *Demosaic_output =
      (short *)malloc((inputPitch - 12) * (inputHeight - 12) * 2 * 3);

  Demosaic(tmp_r_r, tmp_b_b, tmp_g_gr, tmp_g_gb, (inputWidth - 4) / 2,
           (inputWidth - 4) / 2, (inputHeight - 4) / 2, Demosaic_output,
           (inputPitch - 12) * 3);

  short *color_correct_output =
      (short *)malloc((inputPitch - 12) * (inputHeight - 12) * 2 * 3);

  color_correct(Demosaic_output, (inputPitch - 12) * 3, inputWidth - 12,
                inputHeight - 12, color_correct_output, (inputPitch - 12) * 3,
                matrix);

  unsigned char *apply_curve_output =
      (unsigned char *)malloc((inputPitch - 12) * (inputHeight - 12) * 3);

  apply_curve(color_correct_output, (inputPitch - 12) * 3, inputWidth - 12,
              inputHeight - 12, apply_curve_output, (inputPitch - 12) * 3,
              curve);

  sharpen(apply_curve_output, (inputPitch - 12) * 3, inputWidth - 12,
          inputHeight - 12, output, outputPitch, 128);
}