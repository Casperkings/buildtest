#define TILE_W 128
#define TILE_H 64

__kernel void
oclComputeReduction(__global ushort *restrict Src, 
                    __global ushort *restrict Dst, 
                    int width, int height, int numHistBins,
                    __local ushort *restrict local_src,
                    __local ushort *restrict local_dst) {

  event_t evtA = xt_async_work_group_2d_copy(
      local_src, Src, (width/TILE_W)*(height/TILE_H)*numHistBins*sizeof(ushort),
      1, 0, 0, 0);
  wait_group_events(1, &evtA);

#if NATIVE_KERNEL
  extern void ivpReduceHist(__local ushort *src, __local ushort *dst, 
                            int numTiles, int numHistBins);

  ivpReduceHist(local_src, local_dst, 
                (width/TILE_W)*(height/TILE_H), numHistBins);
#else
  __local ushort *p1 = local_src;
  __local ushort64 *dst_p = (__local ushort64 *)local_dst;
  for (int x = 0; x < numHistBins/64; ++x) {
    __local ushort *p2 = p1;
    ushort64 sum = 0;
    for (int y = 0; y < (width/TILE_W)*(height/TILE_H); ++y) {
      sum += *(__local ushort64 *)p2;
      p2 += 256;
    }
    *dst_p++ = sum;
    p1 += 64;
  }

#endif

  event_t evtB = xt_async_work_group_2d_copy(Dst, local_dst, 
                                             numHistBins*sizeof(ushort), 
                                             1, 0, 0, 0);
  wait_group_events(1, &evtB);
}
