const uchar128 RotateLeft1 = {
    127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141,
    142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156,
    157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171,
    172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186,
    187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201,
    202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216,
    217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231,
    232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246,
    247, 248, 249, 250, 251, 252, 253, 254};
const uchar128 RotateLeft2 = RotateLeft1 - (uchar128)1;
const uchar128 RotateLeft3 = RotateLeft2 - (uchar128)1;
const ushort64 RotateLeft4 = {60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 
                              72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 
                              84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 
                              96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 
                              106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
                              116, 117, 118, 119, 120, 121, 122, 123};
const ushort64 RotateLeft8 = RotateLeft4 - (ushort64)4;
const ushort64 RotateLeft16 = RotateLeft8 - (ushort64)8;
const ushort64 RotateLeft32 = RotateLeft16 - (ushort64)16;

{
  __local uchar *pSrc = local_src;
  __local ushort *pDst = local_dst;

  for (int y = 0; y < inTileH; y++) {
    uchar128 x00 = vload128(0, pSrc);
    pSrc += local_src_pitch;
    uchar128 x01 = shuffle2((uchar128)0, x00, RotateLeft1);
    uchar128 x02 = shuffle2((uchar128)0, x00, RotateLeft2);
    uchar128 x03a = shuffle2((uchar128)0, x00, RotateLeft3);

    int128 x03Acc = xt_mul32(x00, (char128)1);
    x03Acc = xt_mad32(x01, (char128)1, x03Acc);
    x03Acc = xt_mad32(x02, (char128)1, x03Acc);
    x03Acc = xt_mad32(x03a, (char128)1, x03Acc);

    ushort128 x03 = convert_ushort128(x03Acc);

    ushort128 x04, x05, x06, x07;
    x04.lo = shuffle2((ushort64)0, x03.lo, RotateLeft4) + x03.lo;
    x04.hi = shuffle2(x03.lo, x03.hi, RotateLeft4) + x03.hi;

    x05.lo = shuffle2((ushort64)0, x04.lo, RotateLeft8) + x04.lo;
    x05.hi = shuffle2(x04.lo, x04.hi, RotateLeft8) + x04.hi;

    x06.lo = shuffle2((ushort64)0, x05.lo, RotateLeft16) + x05.lo;
    x06.hi = shuffle2(x05.lo, x05.hi, RotateLeft16) + x05.hi;

    x07.lo = shuffle2((ushort64)0, x06.lo, RotateLeft32) + x06.lo;
    x07.hi = shuffle2(x06.lo, x06.hi, RotateLeft32) + x06.hi;

    x07.hi += x07.lo;

    vstore128(x07, 0, pDst);
    pDst += local_dst_pitch;
  }
}

for (idx = 128; idx < inTileW; idx += 128) {
  __local uchar *pSrc = local_src + idx;
  __local ushort *pDst = local_dst + idx;
  for (int y = 0; y < inTileH; y += 1) {
    uchar128 x00 = vload128(0, pSrc);
    pSrc += local_src_pitch;
    uchar128 x01 = shuffle2((uchar128)0, x00, RotateLeft1);
    uchar128 x02 = shuffle2((uchar128)0, x00, RotateLeft2);
    uchar128 x03a = shuffle2((uchar128)0, x00, RotateLeft3);

    int128 x03Acc = xt_mul32(x00, (char128)1);
    x03Acc = xt_mad32(x01, (char128)1, x03Acc);
    x03Acc = xt_mad32(x02, (char128)1, x03Acc);
    x03Acc = xt_mad32(x03a, (char128)1, x03Acc);

    ushort128 x03 = convert_ushort128(x03Acc);

    ushort128 x04, x05, x06, x07;
    x04.lo = shuffle2((ushort64)0, x03.lo, RotateLeft4) + x03.lo;
    x04.hi = shuffle2(x03.lo, x03.hi, RotateLeft4) + x03.hi;

    x05.lo = shuffle2((ushort64)0, x04.lo, RotateLeft8) + x04.lo;
    x05.hi = shuffle2(x04.lo, x04.hi, RotateLeft8) + x04.hi;

    x06.lo = shuffle2((ushort64)0, x05.lo, RotateLeft16) + x05.lo;
    x06.hi = shuffle2(x05.lo, x05.hi, RotateLeft16) + x05.hi;

    x07.lo = shuffle2((ushort64)0, x06.lo, RotateLeft32) + x06.lo;
    x07.hi = shuffle2(x06.lo, x06.hi, RotateLeft32) + x06.hi;

    x07.hi += x07.lo;

    ushort128 Out = x07 + (ushort128)(pDst[-1]);
    vstore128(Out, 0, pDst);
    pDst += local_dst_pitch;
  }
}
