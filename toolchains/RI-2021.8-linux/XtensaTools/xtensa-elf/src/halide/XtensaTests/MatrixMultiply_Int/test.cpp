#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include "halide_benchmark.h"
#include "HalideBuffer.h"
#include "xi_api_ref.h"

using namespace Halide::Runtime;
using namespace Halide::Tools;

#define PRINT_MAT 0

uint8_t *a, *b;
uint *copt, *cref;

unsigned int wA, hA, wB, hB, wC, hC, l0, l1;

void matrix_multiply_ref()
{
  xi_array src1_t;
  xi_array src2_t;
  xi_array dst_t;

  // SRC1
  XI_ARRAY_SET_DATA_PTR    ((&src1_t), ((uint8_t*)a));
  XI_ARRAY_SET_BUFF_PTR    ((&src1_t), ((uint8_t*)a));
  XI_ARRAY_SET_BUFF_SIZE   ((&src1_t), wA*hA*sizeof(uint8_t) );
  XI_ARRAY_SET_WIDTH       ((&src1_t), wA);
  XI_ARRAY_SET_HEIGHT      ((&src1_t), hA);
  XI_ARRAY_SET_PITCH       ((&src1_t), wA);
  XI_ARRAY_SET_TYPE        ((&src1_t), XI_TILE_U8);

  // SRC1
  XI_ARRAY_SET_DATA_PTR    ((&src2_t), ((uint8_t*)b));
  XI_ARRAY_SET_BUFF_PTR    ((&src2_t), ((uint8_t*)b));
  XI_ARRAY_SET_BUFF_SIZE   ((&src2_t), wB*hB*sizeof(uint8_t) );
  XI_ARRAY_SET_WIDTH       ((&src2_t), wB);
  XI_ARRAY_SET_HEIGHT      ((&src2_t), hB);
  XI_ARRAY_SET_PITCH       ((&src2_t), wB);
  XI_ARRAY_SET_TYPE        ((&src2_t), XI_TILE_U8);

  //DST_DX
  XI_ARRAY_SET_DATA_PTR    ((&dst_t), cref);
  XI_ARRAY_SET_BUFF_PTR    ((&dst_t), cref);
  XI_ARRAY_SET_BUFF_SIZE   ((&dst_t), wC*hC*sizeof(uint));
  XI_ARRAY_SET_WIDTH       ((&dst_t), wC);
  XI_ARRAY_SET_HEIGHT      ((&dst_t), hC);
  XI_ARRAY_SET_PITCH       ((&dst_t), wC);
  XI_ARRAY_SET_TYPE        ((&dst_t), XI_TILE_U16);

  xiMatrixMultiply_S8S32_ref(&src1_t, &src2_t, &dst_t);
}

#include "mat_mul_int.h"

int main(int argc, char* argv[])
{
  int i, j;
  int error = 0;

  if (argc < 6) {
    std::cout << " Usage: <matMul> <dim1> <dim2> <dim3> <local size 0> <local size 1>\n";
    return -1;
  }

  hA = hC = strtol(argv[1], NULL, 10);
  wA = hB = strtol(argv[2], NULL, 10);
  wB = wC = strtol(argv[3], NULL, 10);

  l0 = strtol(argv[4], NULL, 10);
  l1 = strtol(argv[5], NULL, 10);
  
  Buffer<uint8_t> aMat(wA, hA);
  Buffer<uint8_t> bMat(wB, hB);
  Buffer<uint32_t>cMat(wC, hC);
  
  // init matrix A
  a = (uint8_t *) malloc(sizeof(uint8_t) * hA * wA);

  for (i = 0; i < (int)hA; i++) {
    for (j = 0; j < (int)wA; j++) {
      a[i * wA + j] = (2 * i - j) % 256;
      aMat(j, i) = (2 * i - j) % 256;
    }
  }

#if PRINT_MAT
  std::cout << "Input A\n";
  for (i = 0; i < (int)hA; i++) {
    std::cout << "\n";
    for (j = 0; j < (int)wA; j++) {
      std::cout << (int)aMat(j, i) << " ";
    }
  }
#endif

  
  // init matrix B
  b = (uint8_t *)malloc(sizeof(uint8_t) * hB * wB);
  for (i = 0; i < (int)hB; i++) {
    for (j = 0; j < (int)wB; j++) {
      b[i * wB + j] = (-11 * i + 3 * j) % 256;
      bMat(j, i)= ((-11 * i) + (3 * j)) % 256;
    }
  }
  cref = (uint *)malloc(sizeof(uint) * hC * wC);
  copt = (uint *)malloc(sizeof(uint) * hC * wC);

  memset(copt, 0, (sizeof(uint) * hC * wC));

#if PRINT_MAT
  std::cout << "\nInput B\n";
  for (i = 0; i < (int)hB; i++) {
    std::cout << "\n";
    for (j = 0; j < (int)wB; j++) {
      std::cout << std::hex << (int)bMat(j, i) << " ";
    }
  }
#endif

  matrix_multiply_ref();

#if PRINT_MAT
  std::cout << "Reference Output\n";
  for (i = 0; i < (int)hC; i++) {
    std::cout << "\n";
    for (j = 0; j < (int)wC; j++) {
      std::cout << cref[i * wC + j] << " ";
    }
  }

  std::cout << "\n";
#endif

  mat_mul_int(aMat, bMat, cMat);
  cMat.device_sync();
  cMat.copy_to_host();
  
  // copy to memory from buffer
  std::cout<<"Copy to Buffer\n";
  
  for (int y = 0; y < cMat.height(); y++) {
    for (int x = 0; x < cMat.width(); x++) 
      copt[y*cMat.width() + x ] = cMat(x, y);
   }   

#if PRINT_MAT
  std::cout << "OCL Output\n";
  for (i = 0; i < (int)hC; i++) {
    std::cout << "\n";
    for (j = 0; j < (int)wC; j++) {
      std::cout << std::hex << cMat(j, i) << " ";
    }
  }
  std::cout << "\n";
#endif

  for (i = 0; i < (int)hC; i++) {
    for (j = 0; j < (int)wC; j++) {
      if (cMat(j, i) != cref[i * wC + j]) {
        error = 1;
        std::cout << "Error " << i << " " << j << " " 
                  << std::hex << (int)copt[i*wC + j]
                  << " " << std::hex << (int)cref[i*wC + j] << "\n";
      }
    }
  }

  if (error == 0)
    std::cout << "\nCOMPARE_TEST_PASS\n";
  else
    std::cout << "\nCOMPARE_TEST_FAIL\n";
}
