  __local char *pDst = local_dst;
  __local char *pCoef = local_coef;
  // kernel using standard OpenCL C
  for (int h = 0; h < outTileH; h++) {
    for (int w = 0; w < outTileW; w++) {
      for (int d = 0; d < depth; d += 64) {
        __local char *pvecCoeff = (__local char *)pCoef + d;

        int64 Acc = (int64)0;
        for (int id = 0; id < depth; id++) {
#pragma unroll 5
          for (int y = 0; y < 5; y++) {
            // 0
            __local char *pInCurr =
                &local_src[id + (h + y) * local_src_pitch + w * depth];
            char feat = *pInCurr;
            pInCurr += depth;
            char64 Coefvec = vload64(0, pvecCoeff);
            pvecCoeff += depth;
            Acc = xt_mad24(Coefvec, (char64)feat, Acc);

            // 1
            feat = *pInCurr;
            pInCurr += depth;
            Coefvec = vload64(0, pvecCoeff);
            pvecCoeff += depth;
            Acc = xt_mad24(Coefvec, (char64)feat, Acc);

            // 2
            feat = *pInCurr;
            pInCurr += depth;
            Coefvec = vload64(0, pvecCoeff);
            pvecCoeff += depth;
            Acc = xt_mad24(Coefvec, (char64)feat, Acc);

            // 3
            feat = *pInCurr;
            pInCurr += depth;
            Coefvec = vload64(0, pvecCoeff);
            pvecCoeff += depth;
            Acc = xt_mad24(Coefvec, (char64)feat, Acc);

            // 4
            feat = *pInCurr;
            Coefvec = vload64(0, pvecCoeff);
            pvecCoeff += depth;
            Acc = xt_mad24(Coefvec, (char64)feat, Acc);
          }
        }
        char64 res = xt_convert24_char64_sat_rnd(Acc, shift);
        vstore64(res, 0, pDst);
        pDst += 64;
      }
    }
  }
