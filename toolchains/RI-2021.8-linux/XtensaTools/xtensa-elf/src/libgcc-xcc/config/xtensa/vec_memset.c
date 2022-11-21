/*
 * Copyright (c) 2010-2013 by Tensilica Inc. ALL RIGHTS RESERVED.
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

#include <xtensa/config/core.h>
#include <string.h>

#if XCHAL_HAVE_FUSIONG || XCHAL_HAVE_PDX || XCHAL_HAVE_PDXNX
#include <xtensa/tie/xt_pdxn.h>

#ifdef PDX_LA_4MX8_IP

#if XCHAL_HAVE_FUSIONG 
#define SIMD_M XCHAL_FUSIONG_SIMD32
#else
#define SIMD_M (XCHAL_PDXN_SIMD_WIDTH/2)
#endif

#define LG_SIMD_M_4 ((SIMD_M == 2) ? 3 : \
                   ((SIMD_M == 4) ? 4 : \
                   ((SIMD_M == 8) ? 5 : \
                   ((SIMD_M == 16) ? 6 : \
                   ((SIMD_M == 32) ? 7 : \
                   ((SIMD_M == 64) ? 8 : 0))))))

void * __vec_memset(void * dest, int c, size_t n)
{
  int i;
  valign uout;
  xb_vec4Mx8 * __restrict dest_p = (xb_vec4Mx8 *)dest;
  xb_vec4Mx8 v;
  
  if (n == 0)
    return dest;

  v = PDX_MOVR8_A32(c);
  uout = PDX_Z_ALIGN();
  for (i = 0; i < n>>LG_SIMD_M_4; ++i) {
    PDX_SA_4MX8_IP(v, uout, dest_p);
  }
  PDX_SAV_4MX8_XP(v, uout, dest_p, n & (sizeof(xb_vec4Mx8)-1));
  PDX_SAPOS_4MX8_FP(uout, dest_p);
  return dest;
}

#endif // defined(PDX_LA_4MX8_IP)
#endif // XCHAL_HAVE_FUSIONG || XCHAL_HAVE_PDX || XCHAL_HAVE_PDXNX


#if XCHAL_HAVE_BBENEP
#include <xtensa/tie/xt_bben.h>

#define SIMD_N XCHAL_BBEN_SIMD_WIDTH
#define SIMD_N_M1 (SIMD_N-1)

#define LG_SIMD_N_2 ((SIMD_N == 4) ? 3 : \
                    ((SIMD_N == 8) ? 4 : \
                    ((SIMD_N == 16) ? 5 : \
                    ((SIMD_N == 32) ? 6 : \
                    ((SIMD_N == 64) ? 7 : \
                    ((SIMD_N == 128) ? 8 : 0))))))

#ifdef BBE_LA2NX8_IP

void * __vec_memset(void * s, int c, size_t n)
{
  int i;
  valign uout;
  xb_vec2Nx8 * __restrict dest_p = (xb_vec2Nx8 *)s;
  xb_vec2Nx8 v;

  if (n == 0)
    return s;

  v = BBE_MOVVA8(c);
  uout = BBE_ZALIGN();
  for (i = 0; i < n>>LG_SIMD_N_2; ++i) {
    BBE_SA2NX8_IP(v, uout, dest_p);
  }
  BBE_SAV2NX8_XP(v, uout, dest_p, n & (sizeof(xb_vec2Nx8)-1));
  BBE_SAPOS_FP(uout, dest_p);
  return s;
}

#else // !BBE_LA2NX8_IP

void * __vec_memset(void * s, int c, size_t n)
{
   //unsigned reg2;
   unsigned ab0;
   short inv0;
   int wdrem1;
   xb_vecNx16 V;
   xb_vecNx16 *aliadr4;
   valign A;
   xb_vecNx16 V0;
   int i;

   char *cp = (char *) s;
   int addr = ((int) s);
   if (!n) return s;
   if (addr & 1) {
#pragma frequency_hint NEVER
         *cp++ = c;
         n--;
         if (!n) return s;
   }
   if (n & 1) {
#pragma frequency_hint NEVER
         cp[n-1] = c;
         n--;
         if (!n) return s;
   }

   ab0 = (n / (unsigned)(2)) - 1U;
   inv0 = (short)(c);
   inv0 = inv0 | (inv0 << 8);
   wdrem1 = ((int)(ab0) + 1) & SIMD_N_M1;
   V = (xb_vecNx16)(inv0);
   aliadr4 = (xb_vecNx16 *)(&(cp)[0]);
   A = BBE_ZALIGN();

   for(i = 0; ab0 >= ((unsigned)(i) + SIMD_N_M1); i = i + SIMD_N)
   {
     V0 = V;
     BBE_SANX16_IP(V0, A, aliadr4);
   }
   V0 = V;
   BBE_SAVNX16_XP(V0, A, aliadr4, wdrem1<< 1);
   BBE_SAVNX16POS_FP(A, aliadr4);
   return s;
} /* vecmemset */
#endif // !BBE_LA2NX8_IP

#else // !XCHAL_HAVE_BBENEP

#if XCHAL_HAVE_BBE16
#include <xtensa/tie/xt_bbe16.h>

void * __vec_memset(void *s, int c, size_t n)
{
  int i;
  int addr = ((int) s);
  unsigned char cdata = ((unsigned char) c);
  valign out_align;
  if (n > 0) {
    if (addr & 0x1) {
        *(unsigned char *)addr=cdata;
        n--;
        addr++;
    }
    i = 0;
    if (n > 15) {
      xb_vec8x16 *v_addr = (xb_vec8x16 *) addr;
      v_addr--;
      short sdata= cdata ;
      sdata = sdata | (sdata << 8);
      xb_vec8x20 vdata = sdata;
      out_align = BBE_ZALIGN128();
      for(i = 0; i < n/16; i++) {
        BBE_SA8X16S_IU(vdata, out_align, v_addr, 16);
      }
      BBE_SA8X16S_F(out_align, v_addr);
    }
    for (i=16*i; i<n; i++) {
        ((unsigned char *) addr)[i] = cdata;
    }
  }
  return s;
}

#endif // XCHAL_HAVE_BBE16
#endif // !XCHAL_HAVE_BBENEP


#if (XCHAL_HAVE_HIFI2 || XCHAL_HAVE_HIFI3 || XCHAL_HAVE_HIFI4 || XCHAL_HAVE_HIFI5 || XCHAL_HAVE_FUSION)
#include <xtensa/tie/xt_hifi2.h>

#ifdef AE_SA64POS_FP

void * __vec_memset(void *s, int c, size_t n)
{
  if (n > 15) {
    int i;
    int addr = ((int) s);
    unsigned char cdata = ((unsigned char) c);
    if (addr & 0x1) {
        *(unsigned char *)addr=cdata;
        n--;
        addr++;
    }
    if (n & 0x1) {
        ((unsigned char *)addr)[n-1]=cdata;
        n--;
    }
    // addr is now guaranteed to be 16-bit aligned and n is a multiple of 2
#if (XCHAL_HAVE_HIFI5)
    ae_int8x16 *v_addr = (ae_int8x16 *) addr;
    ae_int8x8 vdata = AE_MOVDA8(cdata);
    ae_valignx2 out_alignx2 = AE_ZALIGN128();
    for(i = 0; i < n>>4; i++) {
        AE_SA8X8X2_IP(vdata, vdata, out_alignx2, v_addr);
    }
    AE_SA128POS_FP(out_alignx2, v_addr);
    ae_int8 *saddr = (ae_int8 *)addr;
    for (i=i<<4; i<n; i++) {
        saddr[i] = AE_MOVINT8_FROMINT8X8(vdata);
    }
#else // !XCHAL_HAVE_HIFI5
    n = n >> 1;
    ae_int16x4 *v_addr = (ae_int16x4 *) addr;
    short sdata= cdata ;
    sdata = sdata | (sdata << 8);
    ae_int16x4 vdata = sdata;
    ae_valign out_align = AE_ZALIGN64();
    for(i = 0; i < n>>2; i++) {
        AE_SA16X4_IP(vdata, out_align, v_addr);
    }
    AE_SA64POS_FP(out_align, v_addr);
    ae_int16 *saddr = (ae_int16 *)addr;
    for (i=i<<2; i<n; i++) {
        saddr[i] = sdata;
    }
#endif // !XCHAL_HAVE_HIFI5
  } else {
    memset(s, c, n);
  }
  return s;
}

#else // !defined(AE_SA64POS_FP)

#if XCHAL_HAVE_HIFI2

void * __vec_memset(void *s, int c, size_t n)
{
  int addr = ((int) s);
  int i;
  if (c==0) { // hifi has 48/56-bit stores but no 64-bit ones
    while (n>0 && addr & 7) {
      *(unsigned char *)addr=0;
      n--;
      addr++;
    }
    ae_p24x2s *simd_addr = (ae_p24x2s *) addr;
    for(i = 0; i < n>>3; i++) {
        simd_addr[i] = AE_ZEROP48();
    }
    for (i=8*i; i<n; i++) {
        ((unsigned char *) addr)[i] = 0;
    }
  } else {
    unsigned char cdata = ((unsigned char) c);
    while (n>0 && addr & 3) {
      *(unsigned char *)addr=cdata;
      n--;
      addr++;
    }
    int data= cdata ;
    data = data | (data << 8) | (data << 16) | (data << 24);
    ae_q32s simd_data = *(ae_q32s *) &data;
    ae_q32s *simd_addr = (ae_q32s *) addr;
    for(i = 0; i < n>>2; i++) {
        simd_addr[i] = simd_data;
    }
    for (i=4*i; i<n; i++) {
        ((unsigned char *) addr)[i] = cdata;
    }
  }
  return s;

}

#else // !XCHAL_HAVE_HIFI2

void * __vec_memset(void *s, int c, size_t n)
{
  int addr = ((int) s);
  int i;
  if (c==0) { // hifi has 48/56-bit stores but no 64-bit ones
    while (n>0 && addr & 7) {
      *(unsigned char *)addr=0;
      n--;
      addr++;
    }
    ae_p24x2s *simd_addr = (ae_p24x2s *) addr;
    for(i = 0; i < n>>3; i++) {
        simd_addr[i] = AE_ZEROP48();
    }
    for (i=8*i; i<n; i++) {
        ((unsigned char *) addr)[i] = 0;
    }
  } else {
    unsigned char cdata = ((unsigned char) c);
    while (n>0 && addr & 3) {
      *(unsigned char *)addr=cdata;
      n--;
      addr++;
    }
    int data= cdata ;
    data = data | (data << 8) | (data << 16) | (data << 24);
    ae_q32s simd_data = *(ae_q32s *) &data;
    ae_q32s *simd_addr = (ae_q32s *) addr;
    for(i = 0; i < n>>2; i++) {
        simd_addr[i] = simd_data;
    }
    for (i=4*i; i<n; i++) {
        ((unsigned char *) addr)[i] = cdata;
    }
  }
  return s;

}
#endif // !XCHAL_HAVE_HIFI2
#endif // !!defined(AE_SA64POS_FP)
#endif // (XCHAL_HAVE_HIFI2 || XCHAL_HAVE_HIFI3 || XCHAL_HAVE_HIFI4 || XCHAL_HAVE_HIFI5 || XCHAL_HAVE_FUSION)


#if XCHAL_HAVE_VISION
#include <xtensa/tie/xt_ivpn.h>

#ifdef IVP_LA2NX8_IP

#define SIMD_N XCHAL_VISION_SIMD16

#define LG_SIMD_N_2 ((SIMD_N == 4) ? 3 : \
                   ((SIMD_N == 8) ? 4 : \
                   ((SIMD_N == 16) ? 5 : \
                   ((SIMD_N == 32) ? 6 : \
                   ((SIMD_N == 64) ? 7 : \
                   ((SIMD_N == 128) ? 8 : 0))))))

void * __vec_memset(void * dest, int c, size_t n)
{
  int i;
  valign uout;
  xb_vec2Nx8 * __restrict dest_p = (xb_vec2Nx8 *)dest;
  xb_vec2Nx8 v;
  
  if (n == 0)
    return dest;

  v = IVP_MOVVA8(c);
  uout = IVP_ZALIGN();
  for (i = 0; i < n>>LG_SIMD_N_2; ++i) {
    IVP_SA2NX8_IP(v, uout, dest_p);
  }
  IVP_SAV2NX8_XP(v, uout, dest_p, n & (sizeof(xb_vec2Nx8)-1));
  IVP_SAPOS2NX8_FP(uout, dest_p);
  return dest;
}

#endif // defined(IVP_LA2NX8_IP)
#endif // XCHAL_HAVE_VISION

