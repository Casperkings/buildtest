/* Copyright (c) 2010 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

 /*
 This is simple example flash programmer for the XT-AV110 board.
 By default it writes an image from offset 0x61000000 in system RAM to Flash.
 By default it unlocks and erases the entire Flash before programming.
 It does not perform full Flash status checks or use all its capabilities.
 Meant to run under a debugger. The user should connect to the board and load
 this program, then load the image, optionally change some variables, and run.
 Variables image_base, image_size, flash_base, and flash_top may be changed.
 */

#include    <stdlib.h>
#include    <xtensa/xtav110.h>
#include    <xtensa/xtbsp.h>

/* Flash device is an Intel PC28F128J3C120 Flash 128Mb with top parameter block(s). */
/* The parameter block(s) is/are subdivided into smaller blocks. */

#define FLASHADDR XTBOARD_FLASH_VADDR     /* base address of flash */
#define FLASHSIZE XTBOARD_FLASH_MAXSIZE   /* size of entire flash (bytes) */
#define FLASHTOP  (FLASHADDR + FLASHSIZE) /* top+1 address of entire flash */
#define BLOCKSIZE 0x20000                 /* size of each block (bytes) */
#define BLOCKSIZE_SHIFT 17                /* Amount of shift corresponding to
                                          size, assuming it to be power of 2 */
#define PROGRAM_SUSPEND_CMD  0xB0         /* Program suspend command */
#define PROGRAM_RESUME_CMD   0xD0         /* Program resume command */
#ifdef XTBOARD_RAM_VADDR
#define BINARYADDR (XTBOARD_RAM_VADDR + 0x1000000)  /* base of RAM image */
#else
#define BINARYADDR 0                      /* (must set image_base manually) */
#endif

#define flashread(addr) (*(volatile unsigned char *)addr)
#define flashwrite(addr, data) *(volatile unsigned char *)addr = data

/* User can change these variables from debugger after loading. */
/* Set flash_base and flash_top (block aligned) to limit region affected. */
/* Set image_size to avoid programming entire region. */
/* To erase (a region of) flash w/o programming, set image_size = 0. */
unsigned flash_base = FLASHADDR;    /* where image starts in flash */
unsigned flash_top  = FLASHTOP;     /* top+1 of region erased for image */
unsigned image_base = BINARYADDR;   /* where image starts in memory */
unsigned image_size = FLASHSIZE;    /* size of image, amount programmed */


/* Write string to primary console device. Much lower overhead than printf(). */
static void putstring(const char *s)
{
  char c;

  while ((c = *s++) != '\0') {
    outbyte(c);
    if (c == '\n')
      outbyte('\r');
  }
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
    outbyte(nib<10 ? '0'+nib : 'A'+(nib-10));
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
  unsigned char stat;
  unsigned char *imagepos;
  unsigned flashpos;
  unsigned byte;

  flashwrite(FLASHADDR, 0x50);          /* clear status register */
  for (                                 /* Write 32 bytes at a time */
    flashpos = flash, imagepos = (unsigned char *)image;
    flashpos < flash + (size & ~0x1F) && flashpos < FLASHTOP;
    flashpos += 32
    ) {
    flashwrite(flashpos, 0xE8);         /* Write to buffer program mode */
    flashwrite(flashpos, 31);         /* Write N = Number of bytes - 1 */
    for ( byte = 0; byte < 32; byte++, ++imagepos )
        flashwrite((flashpos + byte), *imagepos);    /* Write to buffer */
    flashwrite(flashpos, 0xD0);         /* End buffer program mode */
    while (((stat = flashread(FLASHADDR)) & 0x80) == 0);
    if (stat & ~0x80)
      error(4, "programming flash word", flashpos, stat);
  }

  /* Write remaining bits */
  flashwrite(flashpos, 0xE8);         /* Write to buffer program mode */
  flashwrite(flashpos, (size & 0x1F) - 1);  /* Write N = Number of bytes - 1 */
  for ( byte = 0; byte < (size & 0x1F); byte++, ++imagepos )
      flashwrite((flashpos + byte), *imagepos);    /* Write to buffer */
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
  flashwrite(FLASHADDR, 0xFF);          /* restore read array (normal) mode */
#if 1 /* Optional */
  /* Verify by reading the flash back */
  putstring("Verifying...\n"); 
  for (                                 /* for each byte ... */
    flashpos = flash, imagepos = (unsigned char *)image;
    flashpos < flash + size && flashpos < FLASHTOP;
    flashpos++, ++imagepos
    ) {
    byte = flashread(flashpos);
    if(byte != *imagepos) {
        putstring("Error verifying flash\n");
        break;
    }
  }
#endif
}

/* Erase and program flash. Notify user of progress via console. */
int main(int argc, char *argv[]){
  if (flash_base < FLASHADDR || flash_top > FLASHTOP || flash_top < flash_base) {
    putstring("Error - flash region is not entirely within flash.\n");
    return 1;
  }
  if (flash_base + image_size > flash_top) {
    putstring("Error - image is too big for flash region.\n");
    return 1;
  }
  putstring("Programming Flash on XT-AV110 board.\n");
  putstring("Erase Flash...\n");
  eraseFlash(flash_base, 
             (((flash_base + image_size + BLOCKSIZE - 1) >>
                       BLOCKSIZE_SHIFT) << BLOCKSIZE_SHIFT),
             BLOCKSIZE);
  putstring("Program Flash...\n");
  programFlash(image_base, flash_base, image_size);
  putstring("Flash Done !!!\n");

  return 0;
}

