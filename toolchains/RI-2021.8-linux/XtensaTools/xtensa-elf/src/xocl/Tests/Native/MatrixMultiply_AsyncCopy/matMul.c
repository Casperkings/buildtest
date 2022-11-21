#include <stdio.h>

#if defined(__XTENSA__)
#include <sys/times.h>
#include <xtensa/sim.h>
#include <xtensa/tie/xt_ivpn.h>
#include <xtensa/tie/xt_misc.h>

// iDMA related fles
#define IDMA_USE_INTR 0
#include <xtensa/idma.h>
#endif

#include <sys/times.h>
#include <time.h>

#define TIME_DECL int stm, etm;
#define TIME_START()                                                           \
  {                                                                            \
    XT_MEMW();                                                                 \
    __asm__ __volatile__("rsr.ccount %0" : "=a"(stm)::"memory");               \
  }
#define TIME_END()                                                             \
  {                                                                            \
    __asm__ __volatile__("rsr.ccount %0" : "=a"(etm)::"memory");               \
    XT_MEMW();                                                                 \
  }
#define TIME_DIFF(x)                                                           \
  {                                                                            \
    int cycles = etm - stm;                                                    \
    x = cycles;                                                                \
  }
#define TIME_DISPLAY(test)                                                     \
  {                                                                            \
    int cycles = etm - stm;                                                    \
    printf("%s cycles = %d\n", test, cycles);                                  \
  }
#define N 128

#ifdef __clang__
typedef float floatN __attribute__((ext_vector_type(XCHAL_VISION_SIMD16/2)));
#define vec_float floatN
#else
#define vec_float xb_vecN_2xf32
#endif

#define ALIGN (XCHAL_VISION_SIMD16*2)

float __attribute__((aligned(ALIGN))) a[N][N]
    __attribute__((section(".sram.data")));
float __attribute__((aligned(ALIGN))) b[N][N]
    __attribute__((section(".sram.data")));
float __attribute__((aligned(ALIGN))) cref[N][N]
    __attribute__((section(".sram.data")));
float __attribute__((aligned(ALIGN))) copt[N][N]
    __attribute__((section(".sram.data")));

float __attribute__((aligned(ALIGN))) tileA[2][XCHAL_VISION_SIMD16][N]
    __attribute__((section(".dram0.data")));
float __attribute__((aligned(ALIGN))) tileB[2][N][XCHAL_VISION_SIMD16]
    __attribute__((section(".dram0.data")));
float __attribute__((aligned(ALIGN))) tileC[2][XCHAL_VISION_SIMD16]
                                              [XCHAL_VISION_SIMD16]
    __attribute__((section(".dram0.data")));

float __attribute__((aligned(ALIGN))) tileA1[XCHAL_VISION_SIMD16][N]
    __attribute__((section(".dram0.data")));
float __attribute__((aligned(ALIGN))) tileB1[N][XCHAL_VISION_SIMD16]
    __attribute__((section(".dram0.data")));

// DMA related definitions and declarations
#define DMA_CMD_BUFFER_SIZE 20
IDMA_BUFFER_DEFINE(dma_cmd_buf, DMA_CMD_BUFFER_SIZE, IDMA_2D_DESC);
#define PING 0
#define PONG 1
int in_set = 0;      // this can take 0 or 1, 0 = ping and 1 = pong
int out_set = 0;     // this can take 0 or 1, 0 = ping and 1 = pong
int process_set = 0; // this can take 0 or 1, 0 = ping and 1 = pong
int in_dma_descr[2];
int out_dma_descr[2];

void matrix_multiply_ref() {
  int i, j, k;
  for (j = 0; j < N; j += 1) {
    for (k = 0; k < N; k += 1) {
      for (i = 0; i < N; i += 1) {
        cref[i][j] += a[i][k] * b[k][j];
      }
    }
  }
}

void matrix_multiply_opt_xtract_tile() {
  vec_float *__restrict cp = (vec_float *)&tileC[process_set];
  vec_float *__restrict bp = (vec_float *)&tileB[process_set];

  int ic;
  int ib;
  for (ib = 0, ic = 0; ib < XCHAL_VISION_SIMD16; ib += 4, ic += 8) {
    cp[ic] = 0;
    cp[ic + 1] = 0;
    cp[ic + 2] = 0;
    cp[ic + 3] = 0;
    cp[ic + 4] = 0;
    cp[ic + 5] = 0;
    cp[ic + 6] = 0;
    cp[ic + 7] = 0;

    int kb = 0;
    for (kb = 0; kb < N; kb += 2) {
      vec_float arep = tileA[process_set][ib][kb];
      vec_float arep_kp1 = tileA[process_set][ib][kb + 1];
      cp[ic] += (arep * bp[2 * kb]);
      cp[ic] += (arep_kp1 * bp[2 * (kb + 1)]);
      cp[ic + 1] += (arep * bp[2 * kb + 1]);
      cp[ic + 1] += (arep_kp1 * bp[2 * (kb + 1) + 1]);

      vec_float arep1 = tileA[process_set][ib + 1][kb];
      vec_float arep1_kp1 = tileA[process_set][ib + 1][kb + 1];
      cp[ic + 2] += (arep1 * bp[2 * kb]);
      cp[ic + 2] += (arep1_kp1 * bp[2 * (kb + 1)]);
      cp[ic + 3] += (arep1 * bp[2 * kb + 1]);
      cp[ic + 3] += (arep1_kp1 * bp[2 * (kb + 1) + 1]);

      vec_float arep2 = tileA[process_set][ib + 2][kb];
      vec_float arep2_kp1 = tileA[process_set][ib + 2][kb + 1];
      cp[ic + 4] += (arep2 * bp[2 * kb]);
      cp[ic + 4] += (arep2_kp1 * bp[2 * (kb + 1)]);
      cp[ic + 5] += (arep2 * bp[2 * kb + 1]);
      cp[ic + 5] += (arep2_kp1 * bp[2 * (kb + 1) + 1]);

      vec_float arep3 = tileA[process_set][ib + 3][kb];
      vec_float arep3_kp1 = tileA[process_set][ib + 3][kb + 1];
      cp[ic + 6] += (arep3 * bp[2 * kb]);
      cp[ic + 6] += (arep3_kp1 * bp[2 * (kb + 1)]);
      cp[ic + 7] += (arep3 * bp[2 * kb + 1]);
      cp[ic + 7] += (arep3_kp1 * bp[2 * (kb + 1) + 1]);
    }
  }
}

static void wait_dma(unsigned desc) {
  if (desc)
    while (idma_desc_done(desc) == 0)
      ;
}

void extract_tile(int tile_i, int tile_j) {
  // int i, j;
  int ti, tj;
  void *gbuf, *gbuf1;
  void *lbuf, *lbuf1;
  ti = tile_i * XCHAL_VISION_SIMD16;
  tj = tile_j * XCHAL_VISION_SIMD16;

  gbuf = (void *)&a[ti][0];
  lbuf = (void *)&tileA[in_set][0][0];

  idma_add_2d_desc(dma_cmd_buf, lbuf, gbuf, sizeof(float) * N, 0, 
                   XCHAL_VISION_SIMD16, sizeof(float) * N, sizeof(float) * N);

  gbuf1 = (void *)&b[0][tj];
  lbuf1 = (void *)&tileB[in_set][0][0];
  idma_add_2d_desc(dma_cmd_buf, lbuf1, gbuf1, 
                   sizeof(float) * XCHAL_VISION_SIMD16, 0, N,
                   sizeof(float) * N, sizeof(float) * XCHAL_VISION_SIMD16);

  in_dma_descr[in_set] = idma_schedule_desc(2);
  in_set = (in_set + 1) % 2;
  // wait_dma(in_dma_desc);
}

void copy_tile(int tile_i, int tile_j) {
  int ti, tj;
  void *gbuf, *lbuf;
  ti = tile_i * XCHAL_VISION_SIMD16;
  tj = tile_j * XCHAL_VISION_SIMD16;

  gbuf = (void *)&copt[ti][tj];
  lbuf = (void *)&tileC[out_set][0][0];

  idma_add_2d_desc(dma_cmd_buf, gbuf, lbuf, 
                   sizeof(float) * XCHAL_VISION_SIMD16, 0, XCHAL_VISION_SIMD16,
                   sizeof(float) * XCHAL_VISION_SIMD16, sizeof(float) * N);
  out_dma_descr[out_set] = idma_schedule_desc(1);
  out_set = (out_set + 1) % 2;
  // wait_dma(in_dma_desc);
}

#if IDMA_DEBUG
void idmaLogHander(const char *str) { printf("**iDMAlib**: %s", str); }
#endif

void launch_tile_matMul() {
  int ci, cj;
  int i, j;

  for (i = 0; i < N / XCHAL_VISION_SIMD16; i++) {
    for (j = 0; j < N / XCHAL_VISION_SIMD16; j++) {
      // extract into tileA, tileB
      extract_tile(i, j);

      // skip the processing for the first time
      if ((i == 0) && (j == 0)) {
        ci = i;
        cj = j;
        continue;
      }

      if (!((i == 0) && (j == 1))) {
        // wait for output DMA
        wait_dma(out_dma_descr[process_set]);
      }
      wait_dma(in_dma_descr[process_set]);
#if IDMA_DEBUG
      printf("------------> Process\n");
#endif
      matrix_multiply_opt_xtract_tile();
      process_set = (process_set + 1) % 2;

      // copy tileC to C
      copy_tile(ci, cj);
      ci = i;
      cj = j;
    }
  }

  // process the last tile
  // wait for dma
  wait_dma(out_dma_descr[process_set]);
  wait_dma(in_dma_descr[process_set]);
  matrix_multiply_opt_xtract_tile();
  // copy tileC to C
  copy_tile(ci, cj);
}

int main() {
  int fail = 0;

  TIME_DECL;
  int i, j;

  xt_iss_profile_disable();
  for (i = 0; i < N; i++) {
    for (j = 0; j < N; j++) {
      a[i][j] = i + 1; //(2 * i - j) % 4;
      b[i][j] = j + 1; //(-11 * i + 3 * j) % 4;
      cref[i][j] = 0;
    }
  }

#if 0
	printf("Matrix A: ");
	for (i = 0; i < N; i++) {
		printf("\n");
		for (j = 0; j < N; j++) {
			printf("%d ", (int)a[i][j]);
		}
	}

	printf("\nMatrix B: ");
	for (i = 0; j < N; i++) {
		printf("\n");
		for (j = 0; j < N; j++) {
			printf("%d ", (int)b[i][j]);
		}
	}
#endif

  matrix_multiply_ref();

#if 0
	printf("\nOutput: ");
	for (i = 0; i < N; i++) {
		printf("\n");
		for (j = 0; j < N; j++) {
			printf("%d ", (int)cref[i][j]);
		}
	}
#endif

  {
    // iDMA initializations
#if IDMA_DEBUG
    idma_log_handler(idmaLogHander);
#endif
    idma_init(0, MAX_BLOCK_16, 64, TICK_CYCLES_8, 100000, NULL);
    idma_init_loop(dma_cmd_buf, IDMA_2D_DESC, DMA_CMD_BUFFER_SIZE, NULL, NULL);
    in_set = PING;
    out_set = PING;
    process_set = PING;
    in_dma_descr[0] = in_dma_descr[1] = 0;
    out_dma_descr[0] = out_dma_descr[1] = 0;
  }

  {
    int tot_cycles = 0, diff;
    // int ci, cj;
    // code for tiled version
    // memset(copt, 0, sizeof(float) * N * N);
    xt_iss_profile_enable();

    TIME_START()
    launch_tile_matMul();
    TIME_END()

    xt_iss_profile_disable();
    TIME_DIFF(diff);
    tot_cycles += diff;

    printf("matMul_float Cycles %d\n", tot_cycles);
    fail = 0;

    for (i = 0; i < N; i++) {
      for (j = 0; j < N; j++) {
        if (copt[i][j] != cref[i][j]) {
          fail = 1;
          // break;
          printf("Error [%d][%d] %f %f \n", i, j, copt[i][j], cref[i][j]);
        }
      }
    }

    if (fail)
      printf("\nmatMul_float : FAIL\n");
    else
      printf("\nmatMul_float: PASS\n");
  }
}
