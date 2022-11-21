/*
 * main.c
 *
 *  Created on: Jul 6, 2015
 *      Author: yusongh
 */

//#ifdef MAIN_TEST

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <xtensa/hal.h>

#include "idma_tests.h"

#define DRAM0      __attribute__ ((section(".dram0.data")))
#define DRAM1      __attribute__ ((section(".dram1.data")))

// context structure
typedef struct {
	int procIndex;
	int dmaX;
	int dmaY;
	int procX;
	int procY;
	int imageWidth;
	int imageHeight;
	int tileWidth;
	int tileHeight;
	int32_t dmaCount;
	int32_t scheduled;
	uint8_t *pImageIn;
	uint8_t *pImageOut;
	uint8_t *pImageInDMA;
	uint8_t *pImageOutDMA;
} Context;

// number of iDMA descriptors in the buffer
#define NUM_DESCRIPTORS     4

// image settings
#define IMAGE_WIDTH         256
#define IMAGE_HEIGHT        256
#define IMAGE_SIZE          (IMAGE_WIDTH * IMAGE_HEIGHT)
#define IMAGE_PITCH         IMAGE_WIDTH

// tile settings
#define TILE_WIDTH          128
#define TILE_HEIGHT         128
#define TILE_SIZE           (TILE_WIDTH * TILE_HEIGHT)

/* Double buffer in the Local Memory, DMA fills */
ALIGNDCACHE uint8_t DRAM0 dramInBuf[2][TILE_SIZE];
ALIGNDCACHE uint8_t DRAM1 dramOutBuf[2][TILE_SIZE];

/* Double buffer in the main memory, IO fills */
ALIGNDCACHE uint8_t imageIn[IMAGE_SIZE];
ALIGNDCACHE uint8_t imageOut[IMAGE_SIZE];

// allocate iDMA buffer
IDMA_BUFFER_DEFINE(dmaBuffer, NUM_DESCRIPTORS, IDMA_2D_DESC);

// descriptor callback function
void idma_cb_func(void* data)
{
	Context *ctx = (Context *)data;
	ctx->dmaCount++;
	ctx->scheduled--;
}

// Wait for DMA done
static inline void wait_dma(Context *ctx, int32_t count)
{
	while (ctx->scheduled > count) {
		idma_wait_desc(1);
	}
}

// Update input DMA descriptor
static int update_input_dma(Context *ctx)
{
	if (ctx->dmaY >= ctx->imageHeight)
		return 0;

	printf("Update input DMA: %d, %d\n", ctx->dmaY, ctx->dmaX);
	ctx->dmaX += ctx->tileWidth;
	if (ctx->dmaX >= ctx->imageWidth) {
		ctx->dmaX = 0;
		ctx->dmaY += ctx->tileHeight;
	}

	if (ctx->dmaY >= ctx->imageHeight) {
		return 1;
	} else {
		ctx->pImageInDMA = ctx->pImageIn + ctx->dmaY * ctx->imageWidth + ctx->dmaX;
		idma_update_desc_src(ctx->pImageInDMA);
		return 0;
	}
}

// Update output DMA descriptor
static int update_output_dma(Context *ctx)
{
	printf("Update output DMA: %d, %d\n", ctx->procY, ctx->procX);
	ctx->procX += ctx->tileWidth;
	if (ctx->procX >= ctx->imageWidth) {
		ctx->procX = 0;
		ctx->procY += ctx->tileHeight;
	}

	if (ctx->procY >= ctx->imageHeight) {
		return 1;
	} else {
		ctx->pImageOutDMA = ctx->pImageOut + ctx->procY * ctx->imageWidth + ctx->procX;
		idma_update_desc_dst(ctx->pImageOutDMA);
		return 0;
	}
}

// Process a tile and update output DMA
static void process_tile(Context *ctx)
{
	printf("Process tile: %d, %d\n", ctx->procY, ctx->procX);
}

/* THIS IS THE EXAMPLE, main() CALLS IT */
int
test(void* arg, int unused)
{
	Context ctx = {0};

	ctx.imageWidth = IMAGE_WIDTH;
	ctx.imageHeight = IMAGE_HEIGHT;
	ctx.tileWidth = TILE_WIDTH;
	ctx.tileHeight = TILE_HEIGHT;
	ctx.pImageIn = ctx.pImageInDMA = imageIn;
	ctx.pImageOut = ctx.pImageOutDMA = imageOut;

	printf("imageIn = %p\n", imageIn);
	printf("imageOut = %p\n", imageOut);
	printf("dramInBuf = %p, %p\n", dramInBuf[0], dramInBuf[1]);
	printf("dramOutBuf = %p, %p\n", dramOutBuf[0], dramOutBuf[1]);

	// initialize dma
	idma_log_handler(idmaLogHander);
	idma_init(IDMA_CH_ARG  IDMA_OCD_HALT_ON, MAX_BLOCK_2, 16, TICK_CYCLES_2, 100000, idmaErrCB);

	// create dma loop buffer
	DPRINT("Create dma buffer.\n");
	idma_init_loop(IDMA_CH_ARG  dmaBuffer, IDMA_2D_DESC, NUM_DESCRIPTORS, &ctx, idma_cb_func);

	// add descriptors to dma buffer and schedule the first input/output dma
	uint32_t desc_opts = DESC_NOTIFY_W_INT;
	idma_add_2d_desc(dramInBuf[0], ctx.pImageInDMA, TILE_WIDTH, desc_opts, TILE_HEIGHT, IMAGE_PITCH, TILE_WIDTH);
	idma_add_2d_desc(ctx.pImageOutDMA, dramOutBuf[0], TILE_WIDTH, desc_opts, TILE_HEIGHT, TILE_WIDTH, IMAGE_PITCH);
	idma_schedule_desc(IDMA_CH_ARG  2);
	ctx.scheduled = 2;
	update_input_dma(&ctx);

	// add descriptors to dma buffer and schedule the second input/output dma
	idma_add_2d_desc(dramInBuf[1], ctx.pImageInDMA, TILE_WIDTH, desc_opts, TILE_HEIGHT, IMAGE_PITCH, TILE_WIDTH);
	idma_add_2d_desc(ctx.pImageOutDMA, dramOutBuf[1], TILE_WIDTH, desc_opts, TILE_HEIGHT, TILE_WIDTH, IMAGE_PITCH);
	idma_schedule_desc(IDMA_CH_ARG  2);
	ctx.scheduled += 2;
	update_input_dma(&ctx);

	// processing loop
	while (1) {
		// wait for input/output dma done
		wait_dma(&ctx, 2);

		// process a tile
		process_tile(&ctx);

		// schedule next input DMA
		idma_schedule_desc(IDMA_CH_ARG  1);
		ctx.scheduled += 1;
		update_input_dma(&ctx);

		// schedule next output DMA
		idma_schedule_desc(IDMA_CH_ARG  1);
		ctx.scheduled += 1;
		int done = update_output_dma(&ctx);
		if (done)
			break;
	}

	// wait for all DMA done
	wait_dma(&ctx, 0);

	exit(0);
	return -1;
}

//#endif

int
main (int argc, char**argv)
{
   int ret = 0;
   printf("\n\n\nTest '%s'\n\n\n", argv[0]);

#if defined _XOS
   ret = test_xos();
#else
   ret = test(0, 0);
#endif
  // test() exits so this is never reached
  return ret;
}
