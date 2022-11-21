   const uchar64 RotateLeft1 = {63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100,  101,  102,  103,  104,  105,  106,  107,  108,  109,  110,  111,  112,  113,  114,  115,  116,  117,  118,  119,  120,  121,  122,  123,  124,  125,  126};
   const uchar64 RotateLeft2 = {62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100,  101,  102,  103,  104,  105,  106,  107,  108,  109,  110,  111,  112,  113,  114,  115,  116,  117,  118,  119,  120,  121,  122,  123,  124,  125};
   const uchar64 RotateLeft3 = {61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100,  101,  102,  103,  104,  105,  106,  107,  108,  109,  110,  111,  112,  113,  114,  115,  116,  117,  118,  119,  120,  121,  122,  123,  124};
   const ushort32 RotateLeft4 = {28, 29,30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59};
   const ushort32 RotateLeft8 = {24, 25,26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55};
   const ushort32 RotateLeft16 = RotateLeft8 - (ushort32) 8;
  
  {
  __local uchar *pSrc = local_src; 
  __local uint *pDst = local_dst; 
  uint64 Col_carry = 0;

#pragma nounroll
  for(int y = 0; y < inTileH; y++)
  {
    uchar64 x00 = vload64(0, pSrc);
    pSrc += local_src_pitch;
    uchar64 x01 = shuffle2((uchar64)0, x00, RotateLeft1);
    uchar64 x02 = shuffle2((uchar64)0, x00, RotateLeft2);
    uchar64 x03a = shuffle2((uchar64)0, x00, RotateLeft3);
    
    int64 x03Acc = xt_add24(x00, x01);
    x03Acc = xt_add24(x02, x03a, x03Acc);

    ushort64 x03 = xt_convert24_ushort64(x03Acc);
          
    ushort64 x04, x05, x06;
    x04.lo = shuffle2((ushort32)0, x03.lo, RotateLeft4) + x03.lo;
    x04.hi = shuffle2(x03.lo, x03.hi, RotateLeft4) + x03.hi;
    
    x05.lo = shuffle2((ushort32)0, x04.lo, RotateLeft8) + x04.lo;
    x05.hi = shuffle2(x04.lo, x04.hi, RotateLeft8) + x04.hi;
    
    x06.lo = shuffle2((ushort32)0, x05.lo, RotateLeft16) + x05.lo;
    x06.hi = shuffle2(x05.lo, x05.hi, RotateLeft16) + x05.hi;
    
    Col_carry.lo = xt_add32(x06.lo, 0, Col_carry.lo);
    Col_carry.hi = xt_add32(x06.hi, 0, Col_carry.hi);
    Col_carry.hi = xt_add32(x06.lo, 0, Col_carry.hi);
    
    vstore64(Col_carry, pDst);
    pDst += local_dst_pitch;
  }
  }

  for (idx = 64; idx < inTileW; idx += 64) {   
    __local uchar *pSrc = local_src + idx; 
    __local uint *pDst = local_dst + idx; 
    uint64 Col_carry = 0;
    for (int y = 0; y < inTileH; y+=1) {
      uchar64 x00 = vload64(0, pSrc);
      pSrc += local_src_pitch;
      uchar64 x01 = shuffle2((uchar64)0, x00, RotateLeft1);
      uchar64 x02 = shuffle2((uchar64)0, x00, RotateLeft2);
      uchar64 x03a = shuffle2((uchar64)0, x00, RotateLeft3);
      
      int64 x03Acc = xt_add24(x00, x01);
      x03Acc = xt_add24(x02, x03a, x03Acc);
      
      ushort64 x03 = xt_convert24_ushort64(x03Acc);
            
      ushort64 x04, x05, x06;
      x04.lo = shuffle2((ushort32)0, x03.lo, RotateLeft4) + x03.lo;
      x04.hi = shuffle2(x03.lo, x03.hi, RotateLeft4) + x03.hi;
      
      x05.lo = shuffle2((ushort32)0, x04.lo, RotateLeft8) + x04.lo;
      x05.hi = shuffle2(x04.lo, x04.hi, RotateLeft8) + x04.hi;
      
      x06.lo = shuffle2((ushort32)0, x05.lo, RotateLeft16) + x05.lo;
      x06.hi = shuffle2(x05.lo, x05.hi, RotateLeft16) + x05.hi;
      
      Col_carry.lo = xt_add32(x06.lo, 0, Col_carry.lo);
      Col_carry.hi = xt_add32(x06.hi, 0, Col_carry.hi);
      Col_carry.hi = xt_add32(x06.lo, 0, Col_carry.hi);
      uint64 Out = Col_carry + (uint64)(pDst[-1]);
      vstore64(Out, pDst);
      pDst += local_dst_pitch; 
    }
  }
