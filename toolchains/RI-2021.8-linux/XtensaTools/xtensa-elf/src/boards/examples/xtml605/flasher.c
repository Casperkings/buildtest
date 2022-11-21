/* Copyright (c) 2011-2013 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

 /*
 This is simple example flash programmer for XT-ML605 and XT-KC705 boards.
 By default it writes an image from offset 0x61000000 in system RAM to Flash.
 By default it unlocks and erases the entire Flash before programming.
 It does not perform full Flash status checks or use all its capabilities.
 Meant to run under a debugger. The user should connect to the board and load
 this program, then load the image, optionally change some variables, and run.
 Variables image_base, image_size, flash_base, and flash_top may be changed.
 */

#include    <stdlib.h>
#include    <xtensa/board.h>
#include    <xtensa/xtbsp.h>

/* Flash device is an Intel Strata Flash with top parameter block(s). */
/* XT-ML605 daughter card has 16 MBytes of flash memory. */
/* XT-KC705 daughter card has 128 MBYtes of flash memory */
/* The parameter block(s) is/are subdivided into smaller blocks. */

/* Defines for ML605 */
#define ML605_FLASHADDR (XSHAL_IOBLOCK_BYPASS_VADDR + \
                         ML605_FLASH_IOBLOCK_OFS) /* base address of flash */
#define ML605_FLASHSIZE  ML605_FLASH_MAXSIZE  /* size of entire flash (bytes) */
#define ML605_FLASHTOP  (ML605_FLASHADDR + \
                         ML605_FLASHSIZE)     /* top+1 address of entire flash */

/* Defines for KC705 */
#define KC705_FLASHADDR (XSHAL_IOBLOCK_BYPASS_VADDR + \
                         KC705_FLASH_IOBLOCK_OFS) /* base address of flash */
#define KC705_FLASHSIZE  KC705_FLASH_MAXSIZE  /* size of entire flash (bytes) */
#define KC705_FLASHTOP  (KC705_FLASHADDR + \
                         KC705_FLASHSIZE)     /* top+1 address of entire flash */

#ifdef  BOARD_IS_KC705
#define BOARD_NAME "KC705"
#define FLASHADDR KC705_FLASHADDR
#define QUERY_FLASH_GEOMETRY		/* KC705 may carry different FLASH chips,
					 * query FLASH geometry from chip
					 */
#else  /* BOARD_IS_KC705 */
#define BOARD_NAME "ML605"
#define FLASHADDR ML605_FLASHADDR
#endif /* BOARD_IS_KC705 */

#ifdef  QUERY_FLASH_GEOMETRY
#define FLASH_SCALE(a) ((a) << 1)	/* 16-bit interface */
#define FLASHSIZE flash_size
#define FLASHTOP  flash_top
#define BLOCKSIZE block_size
#define BLOCKSIZE_SHIFT block_size_shift
#define PARAMSIZE param_size
#define PARAMSIZE_SHIFT param_size_shift
#define PARAMADDR param_addr
#define IMAGESIZE_DEFAULT 0xffffffff
#define FLASHTOP_DEFAULT 0xffffffff
unsigned flash_size;
unsigned flash_top;
unsigned block_size;
unsigned block_size_shift;
unsigned param_size;
unsigned param_size_shift;
unsigned param_addr;
#else

#ifdef  BOARD_IS_KC705
#define FLASHSIZE KC705_FLASHSIZE
#define FLASHTOP  KC705_FLASHTOP 
#else  /* BOARD_IS_KC705 */
#define FLASHSIZE ML605_FLASHSIZE
#define FLASHTOP  ML605_FLASHTOP 
#endif /* BOARD_IS_KC705 */

#define BLOCKSIZE 0x20000                 /* size of regular block (bytes) */
#define BLOCKSIZE_SHIFT 17                /* Amount of shift corresponding to
						size, assuming it to be power of 2 */
#define PARAMSIZE 0x8000                  /* size of parameter block (bytes) */
#define PARAMSIZE_SHIFT 15                /* Amount of shift corresponding to
						size, assuming it to be power of 2 */
#define PARAMADDR (FLASHTOP  - BLOCKSIZE) /* base address of parameter blocks */
#endif /* QUERY_FLASH_GEOMETRY */


#define PROGRAM_SUSPEND_CMD  0xB0         /* Program suspend command */
#define PROGRAM_RESUME_CMD   0xD0         /* Program resume command */
#ifdef XTBOARD_RAM_VADDR
#define BINARYADDR (XTBOARD_RAM_VADDR + 0x1000000)  /* base of RAM image */
#else
#define BINARYADDR 0                      /* (must set image_base manually) */
#endif

#define flashread(addr) (*(volatile short *)(addr))
#define flashwrite(addr, data) *(volatile short *)(addr) = (data)

/* User can change these variables from debugger after loading. */
/* Set flash_base and flash_top (block aligned) to limit region affected. */
/* Set image_size to avoid programming entire region. */
/* To erase (a region of) flash w/o programming, set image_size = 0. */
unsigned flash_base = FLASHADDR;    /* where image starts in flash */
unsigned image_base = BINARYADDR;   /* where image starts in memory */
#ifdef QUERY_FLASH_GEOMETRY
unsigned image_size = IMAGESIZE_DEFAULT; /* size of image, amount programmed */
unsigned flash_top = FLASHTOP_DEFAULT;   /* top+1 of region erased for image */
#else
unsigned image_size = FLASHSIZE;    /* size of image, amount programmed */
unsigned flash_top  = FLASHTOP;     /* top+1 of region erased for image */
#endif

/* Buffer strings until newline, to play nice with gdbio. */
char charbuf[256];
int  charnext = 0;
static void outflush(void)
{
    write(1, charbuf, charnext);
    charnext = 0;
}
static void outchar(char c)
{
    charbuf[charnext++] = c;
    if (charnext >= sizeof(charbuf) || c == '\n')
	outflush();
}


/* Write string to primary console device. Much lower overhead than printf(). */
static void putstring(const char *s)
{
  char c;

  while ((c = *s++) != '\0')
    outchar(c);
}

/* Write hex number of len digits to primary console device (with leading 0s). */
static void puthex(unsigned hex, unsigned len)
{
  unsigned lsh, mask, nib;

  putstring("0x");
  if (len == 0) return;
  lsh = (len-1) << 2;
  mask = 0xF << lsh;
  while (len-- > 0) {
    nib = (hex & mask) >> lsh;
    outchar(nib<10 ? '0'+nib : 'A'+(nib-10));
    hex <<= 4;
  }
}

/* Write message with address and flash status code and exit with error code. */
static void error(int err, const char *msg, unsigned addr, unsigned short stat)
{
  flashwrite(FLASHADDR, 0x50);          /* clear status register */
  flashwrite(FLASHADDR, 0xFF);          /* restore read array (normal) mode */
  putstring("Error ");
  putstring(msg); 
  putstring(" at address ");
  puthex(addr, 8);
  putstring(", flash status ");
  puthex(stat, 2);
  putstring("\n");
  exit(err);
}

/* Erase all flash blocks overlapping a given address range (unlock first). */
static void eraseFlash(unsigned from, unsigned to, unsigned blksz)
{
  unsigned char stat;
  unsigned flashpos;

  flashwrite(FLASHADDR, 0x50);          /* clear status register */
  for (                                 /* for each block ... */
    flashpos = from & ~(blksz-1);       /*   assume power of 2 aligned blocks */
    flashpos < to;
    flashpos += blksz
    ) {
    /* unlock block */
    flashwrite(flashpos, 0x60);         /*   clear block lock bits */
    flashwrite(flashpos, 0xD0);         /*   clear block lock confirm */
    while (((stat = flashread(FLASHADDR)) & 0x80) == 0);
    if (stat & ~0x80)
      error(2, "unlocking flash block", flashpos, stat);
    /* erase block */
    flashwrite(flashpos, 0x20);         /*   block erase mode */
    flashwrite(flashpos, 0xD0);         /*   block erase confirm */
    while (((stat = flashread(FLASHADDR)) & 0x80) == 0);
    if (stat & ~0x80)
      error(3, "erasing flash block", flashpos, stat);
  }
  flashwrite(FLASHADDR, 0xFF);          /* restore read array (normal) mode */
}

/* Program image from RAM to flash (caller ensures blank space is available). */
static void programFlash(unsigned image, unsigned flash, unsigned size)
{
  unsigned short stat;
  unsigned short *imagepos;
  unsigned flashpos;
  unsigned hword;

  flashwrite(FLASHADDR, 0x50);          /* clear status register */
  for (                                 /* Write 32 bytes at a time */
    flashpos = flash, imagepos = (unsigned short *)image;
    flashpos < flash + (size & ~0x3F) && flashpos < FLASHTOP;
    flashpos += 64
    ) {
    flashwrite(flashpos, 0xE8);         /* Write to buffer program mode */
    flashwrite(flashpos, 31);         /* Write N = Number of bytes - 1 */
    for ( hword = 0; hword < 64; hword+=2, ++imagepos )
        flashwrite((flashpos + hword), *imagepos);    /* Write to buffer */
    flashwrite(flashpos, 0xD0);         /* End buffer program mode */
    while (((stat = flashread(FLASHADDR)) & 0x80) == 0);
    if (stat & ~0x80)
      error(4, "programming flash word", flashpos, stat);
  }

  /* Write remaining bits */
  flashwrite(flashpos, 0xE8);         /* Write to buffer program mode */
  flashwrite(flashpos, ((((size & 0x3F) + 1) >> 1) - 1));  /* Write N = Number of bytes - 1 */
  for ( hword = 0; hword < (size & 0x3F); hword+=2, ++imagepos )
      flashwrite((flashpos + hword), *imagepos);    /* Write to buffer */
  flashwrite(flashpos, 0xD0);         /* End buffer program mode */
  while (((stat = flashread(FLASHADDR)) & 0x80) == 0);
  if (stat & ~0x80)
    error(4, "programming flash word", flashpos, stat);
#if 0 /* Write byte mode, doesn't work when the data emulates command byte */
  for (                                 /* for each 16b word ... */
    flashpos = flash, imagepos = (unsigned char *)image;
    flashpos < flash + size && flashpos < FLASHTOP;
    flashpos++, ++imagepos
    ) {
    flashwrite(flashpos, 0x40);         /*   word/byte program mode */
    flashwrite(flashpos, *imagepos);    /*   data */
    /* The data may emulate a program suspend command byte, in that
     * case a program resume command must be issued */
    while (((stat = flashread(FLASHADDR)) & 0x80) == 0);

    if (stat & ~0x80)
      error(4, "programming flash word", flashpos, stat);
  }
#endif
  flashwrite(FLASHADDR, 0x50);          /* clear status register */  
  flashwrite(FLASHADDR, 0xFF);          /* restore read array (normal) mode */
#if 1 /* Optional */
  /* Verify by reading the flash back */
  putstring("Verifying...\n"); 
  for (                                 /* for each byte ... */
    flashpos = flash, imagepos = (unsigned short *)image;
    flashpos < flash + size && flashpos < FLASHTOP;
    flashpos+=2, ++imagepos
    ) {
    hword = (flashread(flashpos) & 0xFFFF);
    if(hword != *imagepos) {
        putstring("Error verifying flash\n");
        break;
    }
  }
#endif
}

#ifdef QUERY_FLASH_GEOMETRY
static int queryFlashGeometry(void)
{
  int rc = 0;
  unsigned nebr;
  unsigned i;
  unsigned eb[2];

  flashwrite(FLASHADDR + FLASH_SCALE(0x55), 0x98); /* Select Query output mode */

  if (flashread(FLASHADDR + FLASH_SCALE(0x10)) != 'Q' ||
      flashread(FLASHADDR + FLASH_SCALE(0x11)) != 'R' ||
      flashread(FLASHADDR + FLASH_SCALE(0x12)) != 'Y') {
    putstring("Error - can't find QRY singnature.\n");
    goto error;
  }

  flash_size = 1u << flashread(FLASHADDR + FLASH_SCALE(0x27));
  if (flash_top == FLASHTOP_DEFAULT)
    flash_top = FLASHADDR + flash_size;

  nebr = flashread(FLASHADDR + FLASH_SCALE(0x2c)) & 0xff;
  if (nebr != 2) {
    putstring("Error - wrong number of erase block regions.\n");
    goto error;
  }

  /* Read bytes of Erase Block Region Information words
   * and combine them back to words. We can't read words
   * because data representation depends on FLASH interface
   * width and only LSB of each 'maximum device buswidth'
   * word is valid.
   */
  for (i = 0; i < 2; ++i) {
    unsigned j;
    union {
      unsigned u;
      unsigned char b[4];
    } v;
    for (j = 0; j < 4; ++j) {
      v.b[j] = flashread(FLASHADDR + FLASH_SCALE(0x2d + i * 4 + j));
    }
    eb[i] = v.u;
  }

  block_size = (eb[0] >> 16) ? (eb[0] >> 16) << 8 : 128;
  param_size = (eb[1] >> 16) ? (eb[1] >> 16) << 8 : 128;
  for (i = 0; block_size > 1u << i; ++i) {
  }
  block_size_shift = i;
  for (i = 0; param_size > 1u << i; ++i) {
  }
  param_size_shift = i;

  param_addr = FLASHADDR + (((eb[0] & 0xffff) + 1) * block_size);

  if (image_size == IMAGESIZE_DEFAULT)
    image_size = flash_size;
  rc = 1;
error:
  flashwrite(FLASHADDR, 0xFF); /* Select normal array data output mode */
  return rc;
}
#else
static int queryFlashGeometry(void)
{
  return 1;
}
#endif /* QUERY_FLASH_GEOMETRY */

/* Erase and program flash. Notify user of progress via console. */
int main(int argc, char *argv[]){
  if (!queryFlashGeometry()) {
    putstring("Error - couldn't query flash geometry.\n");
    return 1;
  }
  if (flash_base < FLASHADDR || flash_top > FLASHTOP || flash_top < flash_base) {
    putstring("Error - flash region is not entirely within flash.\n");
    return 1;
  }
  if (flash_base + image_size > flash_top) {
    putstring("Error - image is too big for flash region.\n");
    return 1;
  }
  putstring("Programming Flash on " BOARD_NAME " board.\n");
  putstring("Erase Flash...\n");
  
  image_size = (image_size > 0x20000) ? image_size : 0x20000;
  if(flash_base + image_size < PARAMADDR) {
    eraseFlash(flash_base, 
             (((flash_base + image_size + BLOCKSIZE - 1) >>
                       BLOCKSIZE_SHIFT) << BLOCKSIZE_SHIFT),
             BLOCKSIZE);
  }else {
    eraseFlash(flash_base, PARAMADDR, BLOCKSIZE);
    eraseFlash(PARAMADDR, 
             (((flash_base + image_size + PARAMSIZE - 1) >>
              PARAMSIZE_SHIFT) << PARAMSIZE_SHIFT),          
              PARAMSIZE);
  }

  putstring("Program Flash...\n");
  programFlash(image_base, flash_base, image_size);
  putstring("Flash Done !!!\n");

  return 0;
}

