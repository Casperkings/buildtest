
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

void *__vec_memcpy(char *__restrict dest, const char *__restrict src, int n)
{
  int i;
  valign uin, uout;
  xb_vec4Mx8 * __restrict src_p = (xb_vec4Mx8 *)src;
  xb_vec4Mx8 * __restrict dest_p = (xb_vec4Mx8 *)dest;
  xb_vec4Mx8 v;
  
  if(n <= 0)
    return dest;

  uin = PDX_LA_4MX8_PP(src_p);
  uout = PDX_Z_ALIGN();
  for (i = 0; i < n>>(LG_SIMD_M_4+1); ++i) {
	 PDX_LA_4MX8_IP(v, uin, src_p);
	 PDX_SA_4MX8_IP(v, uout, dest_p);
	 PDX_LA_4MX8_IP(v, uin, src_p);
	 PDX_SA_4MX8_IP(v, uout, dest_p);
  }
  int rem = n & (sizeof(xb_vec4Mx8)*2-1);
  if (rem > 0) {
	 PDX_LAV_4MX8_XP(v, uin, src_p, rem);
	 PDX_SAV_4MX8_XP(v, uout, dest_p, rem);
	 PDX_SAPOS_4MX8_FP(uout, dest_p);
	 if (rem > sizeof(xb_vec4Mx8)) {
	    PDX_LAV_4MX8_XP(v, uin, src_p, rem- sizeof(xb_vec4Mx8));
	    PDX_SAV_4MX8_XP(v, uout, dest_p, rem- sizeof(xb_vec4Mx8));
	    PDX_SAPOS_4MX8_FP(uout, dest_p);
	 }
  }
  else
    PDX_SAPOS_4MX8_FP(uout, dest_p);

  return dest;
}

#endif // defined(PDX_LA_4MX8_IP)
#endif // XCHAL_HAVE_FUSIONG || XCHAL_HAVE_PDX || XCHAL_HAVE_PDXNX 

#if XCHAL_HAVE_BBENEP
#include <xtensa/tie/xt_bben.h>

#define SIMD_N XCHAL_BBEN_SIMD_WIDTH 
#define SIMD_N_M1 (SIMD_N-1)
#define SIMD_N_2 (2*XCHAL_BBEN_SIMD_WIDTH)
#define SIMD_N_2_M1  (SIMD_N_2-1)

#define LG_SIMD_N_2 ((SIMD_N == 4) ? 3 : \
                   ((SIMD_N == 8) ? 4 : \
                   ((SIMD_N == 16) ? 5 : \
                   ((SIMD_N == 32) ? 6 : \
                   ((SIMD_N == 64) ? 7 : \
                   ((SIMD_N == 128) ? 8 : 0))))))

#define min(a,b) (a) < (b) ? (a) : (b)

#ifdef BBE_LA2NX8_IP

void *__vec_memcpy(char *__restrict dest, const char *__restrict src, int n)
{
  xb_vec2Nx8 * __restrict srcp;
  xb_vec2Nx8 * __restrict destp;
  xb_vec2Nx8 V;
  valign vaload;
  valign vastore;
  int i;

  char *orig_dest = dest;
  if(n <= 0)
    return orig_dest;

  // 16-bit aligned
  srcp = (xb_vec2Nx8 *) src;
  destp = (xb_vec2Nx8 *) dest;

  vaload = BBE_LA2NX8_PP(srcp);
  vastore = BBE_ZALIGN();
  //get no.of bytes to make store aligned
  int iters = min(n, (SIMD_N_2 - (SIMD_N_2_M1  & (int) dest)));
  BBE_LAV2NX8_XP(V, vaload, srcp, iters);
  BBE_SAV2NX8_XP(V, vastore, destp, iters);
  BBE_SAPOS_FP(vastore, destp); 
  // now store is vector aligned
  for (i=0; i<n-iters-SIMD_N_2_M1; i+=SIMD_N_2) {
    BBE_LA2NX8_IP(V, vaload, srcp);
    *destp++ = V;
  }
  if (n-iters-i > 0) {
    vastore = BBE_ZALIGN();
    BBE_LAV2NX8_XP(V, vaload,srcp, (n-iters-i));
    BBE_SAV2NX8_XP(V, vastore, destp, (n-iters-i));
  }
  BBE_SAPOS_FP(vastore, destp);
  return orig_dest;
}

#else // !defined(BBE_LA2NX8_IP)

void *__vec_memcpy(char *__restrict dest, const char *__restrict src, int n)
{
  xb_vecNx16 * __restrict srcp;
  xb_vecNx16 * __restrict destp;
  xb_vecNx16 V;
  valign vaload;
  valign vastore;
  int i;

  char *orig_dest = dest;
  if(n <= 0)
          return orig_dest;

  if ( ((int) dest) &1) {
#pragma frequency_hint NEVER
    if (((int) src) &1) {
      *dest++ = *src++;
      n--;
      if (n==0) return orig_dest;
    } else {
	return memcpy(dest, src,n);
    }
  } else if (((int) src) & 1) {
#pragma frequency_hint NEVER
    return memcpy(dest, src,n);
  }


  // 16-bit aligned
  srcp = (xb_vecNx16 *) src;
  destp = (xb_vecNx16 *) dest;

  if (n & 0x1) {
	    dest[n-1] = src[n-1];
	    n--;
  }
  n = n >> 1; // n is number of shorts to copy
  vaload = BBE_LANX16_PP(srcp);
  vastore = BBE_ZALIGN();
  int iters = min(n, (SIMD_N_2 - (SIMD_N_2_M1  & (int) dest))>>1);
  if (iters == 0)
    return orig_dest;
  BBE_LAVNX16_XP(V, vaload,srcp,iters<<1);
  BBE_SAVNX16_XP(V, vastore, destp, iters<<1);
  BBE_SAVNX16POS_FP(vastore, destp); 
  // now store is vector aligned
  for (i=0; i<n-iters-SIMD_N_M1; i+=SIMD_N) {
	    BBE_LANX16_IP(V, vaload, srcp);
            *destp++ = V;
  }
  if (n-iters-i > 0) {
    vastore = BBE_ZALIGN();
    BBE_LAVNX16_XP(V, vaload,srcp, (n-iters-i)<<1);
    BBE_SAVNX16_XP(V, vastore, destp, (n-iters-i)<<1);
    BBE_SAVNX16POS_FP(vastore, destp);
  }
  return orig_dest;
} /* __vec_memcpy */

#endif // !defined(BBE_LA2NX8_IP)

#else // !XCHAL_HAVE_BBENEP

#if XCHAL_HAVE_BBE16
#include <xtensa/tie/xt_bbe16.h>

void *__vec_memcpy(char *__restrict dest, const char *__restrict src, int n)
{
  valign out_align;
  xb_vec8x16 * __restrict out_align_addr, * __restrict in;
  int i;
  xb_vec8x20 t,t2,tmerge;
  vsel sel;


  if ( !(((int) dest) &1) && !(((int) src) &1)) { // 16-bit aligned
    if (!(((int) src) &15)) { // input is 128-bit aligned
      in = (xb_vec8x16 *) ((int )src);
      out_align_addr = (xb_vec8x16 *)(dest-16);
      out_align = BBE_ZALIGN128();
      i=0;
      if (n>15) {
        for(i = 0; i < n>>4; i++) {
          t = in[i];
          BBE_SA8X16S_IU(t, out_align, out_align_addr, 16);
        }
        BBE_SA8X16S_F(out_align, out_align_addr);
      }
      for (i=16*i; i<n; i++) {
        dest[i] = src[i];
      }
      return dest;
    }
    if (n < 64) {
      return memcpy(dest, src, n);
    }
    sel = BBE_SEL4V8X20(BBE_MOVVI8X20(56) + (xb_vec8x20)(short)(((int)src>>1)&7), 0);
    in = (xb_vec8x16 *) (((int )src) & (~15));
    in--;

    out_align_addr = (xb_vec8x16 *)(dest-16);
    out_align = BBE_ZALIGN128();

    BBE_LV8X16S_XU(t, in, 16);
    for(i = 0; i < n>>4; i++) {
       BBE_LV8X16S_XU(t2, in, 16);
       tmerge = BBE_SEL8X20(t2, t, sel);
       BBE_SA8X16S_IU(tmerge, out_align, out_align_addr, 16);
       t = t2;
    }
    BBE_SA8X16S_F(out_align, out_align_addr);

    for (i=16*i; i<n; i++) {
        dest[i] = src[i];
    }
    return dest;
  } else { // not 16-bit aligned
    return memcpy(dest, src, n);
  }
} /* vec_memcpy */

#endif // XCHAL_HAVE_BBE16
#endif // !XCHAL_HAVE_BBENEP

#if (XCHAL_HAVE_HIFI3 || XCHAL_HAVE_HIFI4 || XCHAL_HAVE_HIFI5 || XCHAL_HAVE_FUSION)
#include <xtensa/tie/xt_hifi2.h>

#ifdef AE_SA64POS_FP

void *__vec_memcpy(char *__restrict dest, const char *__restrict src, int n)
{
#if (XCHAL_HAVE_HIFI5)
  ae_int8x16 * __restrict d_align_addr, * __restrict s_align_addr;
#else
  ae_int16x4 * __restrict d_align_addr, * __restrict s_align_addr;
#endif
  int i;
  void *orig_dest = dest;

  if (n < 32) {
    return memcpy(dest, src, n);
  }

#if (XCHAL_HAVE_HIFI5)
  if ( !(((int) dest) %16) && !(((int) src) %16)) { // 128-bit aligned
    d_align_addr = (ae_int8x16 *) dest;
    s_align_addr = (ae_int8x16 *) src;
    ae_int8x8 t,t2;
    for (i=0; i<n>>4; i++) {
        AE_L8X8X2_IP(t, t2, s_align_addr, 16);
        AE_S8X8X2_IP(t, t2, d_align_addr, 16);
    }
  }
  else {
    d_align_addr = (ae_int8x16 *) dest;
    s_align_addr = (ae_int8x16 *) src;
    ae_int8x8 t,t2;
    ae_valignx2 d_align = AE_ZALIGN128();
    ae_valignx2 s_align = AE_LA128_PP(s_align_addr);
    for (i=0; i<n>>4; i++) {
        AE_LA8X8X2_IP(t, t2, s_align, s_align_addr);
        AE_SA8X8X2_IP(t, t2, d_align, d_align_addr);
    }
    AE_SA128POS_FP(d_align, d_align_addr);
  }

  {
    const ae_int8 * __restrict s_src = (const ae_int8 *) s_align_addr;
    ae_int8 * __restrict s_dest = (ae_int8 *) d_align_addr;
    for (i=0; i<(n%16); i++) {
      s_dest[i] = s_src[i];
    }
    return orig_dest;
  }

#else // !(XCHAL_HAVE_HIFI5)

#if (XCHAL_HAVE_HIFI4)
  if ( !(((int) dest) %8) && !(((int) src) %8)) { // 64-bit aligned
    ae_int16x4 t;
    s_align_addr = (ae_int16x4 *) src;
    d_align_addr = (ae_int16x4 *) dest;
    for (i=0; i<n>>3; i++) {
      AE_L16X4_IP(t, s_align_addr, 8);
      AE_S16X4_IP(t, d_align_addr, 8);
    }
    {
      const char * __restrict s_src = (const char *) s_align_addr;
      char * __restrict s_dest = (char *) d_align_addr;
      for (i=0;i<(n%8); i++) {
        s_dest[i] = s_src[i];
      }
    }
    return orig_dest;
  }
#endif

  if ( (((int) dest) %2) || (((int) src) %2)) { // 16-bit aligned
    if ( (((int) dest) %2) && (((int) src) %2)) { // 16-bit aligned
      *dest++ = *src++;
       n--;
    } else {
      return memcpy(dest, src, n);
    }
  }
  int n2 = n/2;
  ae_valign d_align = AE_ZALIGN64();
  d_align_addr = (ae_int16x4 *) dest;
  s_align_addr = (ae_int16x4 *) src;
  ae_valign s_align = AE_LA64_PP(s_align_addr);
  ae_int16x4 t,t2;
  for (i=0; i<n2>>3; i++) {
      AE_LA16X4_IP(t, s_align, s_align_addr);
      AE_LA16X4_IP(t2, s_align, s_align_addr);
      AE_SA16X4_IP(t, d_align, d_align_addr);
      AE_SA16X4_IP(t2, d_align, d_align_addr);
  }
  AE_SA64POS_FP(d_align, d_align_addr);
  const ae_int16 * __restrict s_src = (const ae_int16 *) s_align_addr;
  ae_int16 * __restrict s_dest = (ae_int16 *) d_align_addr;
  for (i=0; i<(n2%8); i++) {
    s_dest[i] = s_src[i];
  }
  if (n % 2) {
    dest[n-1] = src[n-1];
  }
  return orig_dest;

#endif // !(XCHAL_HAVE_HIFI5)

} /* vec_memcpy */

#else // !defined(AE_SA64POS_FP)

void *__vec_memcpy(char *__restrict dest, const char *__restrict src, int n)
{
  ae_int16x4 * __restrict d_align_addr, * __restrict s_align_addr;
  int i;
  void *orig_dest = dest;

  if (n < 32) {
    return memcpy(dest, src, n);
  }

  if ( !(((int) dest) %8) && !(((int) src) %8)) { // 64-bit aligned
    ae_int16x4 t;
    s_align_addr = (ae_int16x4 *) src;
    d_align_addr = (ae_int16x4 *) dest;
    for (i=0; i<n>>3; i++) {
      AE_L16X4_IP(t, s_align_addr, 8);
      AE_S16X4_IP(t, d_align_addr, 8);
    }
    {
      const char * __restrict s_src = (const char *) s_align_addr;
      char * __restrict s_dest = (char *) d_align_addr;
      for (i=0; i<(n%8); i++) {
        s_dest[i] = s_src[i];
      }
    }
    return orig_dest;
  }
  return memcpy(dest, src, n);
} /* vec_memcpy */

#endif // !defined(AE_SA64POS_FP)
#endif // (XCHAL_HAVE_HIFI3 || XCHAL_HAVE_HIFI4 || XCHAL_HAVE_HIFI5 || XCHAL_HAVE_FUSION)


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

void *__vec_memcpy(char *__restrict dest, const char *__restrict src, int n)
{
  int i;
  valign uin, uout;
  xb_vec2Nx8 * __restrict src_p = (xb_vec2Nx8 *)src;
  xb_vec2Nx8 * __restrict dest_p = (xb_vec2Nx8 *)dest;
  xb_vec2Nx8 v;
  
  if(n <= 0)
    return dest;

  uin = IVP_LA2NX8_PP(src_p);
  uout = IVP_ZALIGN();
  for (i = 0; i < n>>(LG_SIMD_N_2+1); ++i) {
	 IVP_LA2NX8_IP(v, uin, src_p);
	 IVP_SA2NX8_IP(v, uout, dest_p);
	 IVP_LA2NX8_IP(v, uin, src_p);
	 IVP_SA2NX8_IP(v, uout, dest_p);
  }
  int rem = n & (sizeof(xb_vec2Nx8)*2-1);
  if (rem > 0) {
    IVP_LAV2NX8_XP(v, uin, src_p, rem);
    IVP_SAV2NX8_XP(v, uout, dest_p, rem);
    IVP_SAPOS2NX8_FP(uout, dest_p);
    if (rem > sizeof(xb_vec2Nx8)) {
      IVP_LAV2NX8_XP(v, uin, src_p, rem- sizeof(xb_vec2Nx8));
      IVP_SAV2NX8_XP(v, uout, dest_p, rem- sizeof(xb_vec2Nx8));
      IVP_SAPOS2NX8_FP(uout, dest_p);	
    }
  }
  else 
    IVP_SAPOS2NX8_FP(uout, dest_p);
  return dest;
}

#endif // defined(IVP_LA2NX8_IP)
#endif // XCHAL_HAVE_VISION
