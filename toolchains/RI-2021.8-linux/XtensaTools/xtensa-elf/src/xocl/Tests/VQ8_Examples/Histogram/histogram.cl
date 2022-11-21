#define TILE_W 128
#define TILE_H 64

__kernel __attribute__((reqd_work_group_size(1, 1, 1)))
__attribute__((reqd_work_dim_size(2))) void
oclComputeHist(__global uchar *restrict Src, __global ushort *restrict Dst, 
              int width, int height, int numHistBins,
              __local uchar *restrict local_src,
              __local ushort *restrict local_dst,
              __local ushort *restrict local_hist) {
  int i, j;
  int idx, idy;
  int num_rows = TILE_H;
  int row_size = TILE_W;

  i = get_global_id(0);
  j = get_global_id(1);

  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src + (i * num_rows * width) + (j * row_size), row_size,
      num_rows, row_size, width, 0);
  wait_group_events(1, &evtA);

#if NATIVE_KERNEL

  extern void ivpCalcHist(const __local uchar * restrict src, 
                          __local ushort * restrict dst,
                          __local ushort * restrict hist, 
                          int width, int height, int numHistBins);

  ivpCalcHist(local_src, local_dst, local_hist, TILE_W, TILE_H, numHistBins);
#else
  // We have a per-lane histogram

  // Zero all the histograms
  __local ushort64 *p = (__local ushort64 *)local_hist;
  for (int x = 0; x < (numHistBins * 64)/64; ++x) {
    *p++ = (ushort64)0;
  }

  const ushort64 seq = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
                        29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
                        43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
                        57, 58, 59, 60, 61, 62, 63};
  ushort64 offsets = seq * (ushort64)numHistBins;
 
  for (int x = 0; x < TILE_W/64; ++x) {
    __local uchar *src_p = (local_src + x*64);
    for (int y = 0; y < TILE_H; ++y) {
      ushort64 v = convert_ushort64(*(__local uchar64 *)src_p);
      ushort64 hist_offsets = offsets + v;
      ushort64 count = xt_gather64(local_hist, (ushort *)&hist_offsets);
      xt_scatter64(count + (ushort64)1, local_hist, (ushort *)&hist_offsets);
      src_p += TILE_W;
    }
  }

  // Wait for scatter to finish
#pragma flush_memory

  // Reduce the per-lane histogram
  __local ushort *p1 = local_hist;
  __local ushort64 *dst_p = (__local ushort64 *)local_dst;
  for (int x = 0; x < numHistBins/64; ++x) {
    __local ushort *p2 = p1;
    ushort64 sum = 0;
    for (int y = 0; y < 64; ++y) {
      sum += *(__local ushort64 *)p2;
      p2 += 256;
    }
    *dst_p++ = sum;
    p1 += 64;
  }
#endif

  event_t evtB = xt_async_work_group_2d_copy(
                   Dst + (i*(width/TILE_W) + j)*numHistBins,
                   local_dst, numHistBins*sizeof(ushort), 1, 0, 0, 0);
  wait_group_events(1, &evtB);
}
