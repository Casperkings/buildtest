  __local char *pDst = local_dst;
  __local char *pCoef = local_coef;
  // kernel using standard OpenCL C
  for (int h = 0; h < outTileH; h++) {
    for (int w = 0; w < outTileW; w++) {
      for (int d = 0; d < depth; d += 128) {
        __local char *pvecCoeff = (__local char *)pCoef + d;

        int128 Acc = (int128)0;
        for (int id = 0; id < depth; id++) {
#pragma unroll 5
          for (int y = 0; y < 5; y++) {
            // 0
            __local char *pInCurr =
                &local_src[id + (h + y) * local_src_pitch + w * depth];
            char feat = *pInCurr;
            pInCurr += depth;
            char128 Coefvec = vload128(0, pvecCoeff);
            pvecCoeff += depth;
            Acc = xt_mad32(Coefvec, (char128)feat, Acc);

            // 1
            feat = *pInCurr;
            pInCurr += depth;
            Coefvec = vload128(0, pvecCoeff);
            pvecCoeff += depth;
            Acc = xt_mad32(Coefvec, (char128)feat, Acc);

            // 2
            feat = *pInCurr;
            pInCurr += depth;
            Coefvec = vload128(0, pvecCoeff);
            pvecCoeff += depth;
            Acc = xt_mad32(Coefvec, (char128)feat, Acc);

            // 3
            feat = *pInCurr;
            pInCurr += depth;
            Coefvec = vload128(0, pvecCoeff);
            pvecCoeff += depth;
            Acc = xt_mad32(Coefvec, (char128)feat, Acc);

            // 4
            feat = *pInCurr;
            Coefvec = vload128(0, pvecCoeff);
            pvecCoeff += depth;
            Acc = xt_mad32(Coefvec, (char128)feat, Acc);
          }
        }
        char128 res = xt_convert_char128_sat_rnd(Acc, shift);
        vstore128(res, 0, pDst);
        pDst += 128;
      }
    }
  }
