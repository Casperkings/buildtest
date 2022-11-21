/* Copyright (c) 2007 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

 /*
 This is simple example flash programmer for the XT-AV60 board.
 By default it writes an image from offset 0x61000000 in system RAM to Flash.
 It erases the entire Flash (in a single operation) before programming.
 It does not perform full Flash status checks or use all its capabilities.
 Meant to run under a debugger. The user should connect to the board and load
 this program, then load the image, optionally change some variables, and run.
 Variables image_base, image_size, flash_base and flash_erase may be changed.
 */

#include    <stdlib.h>
#include    <xtensa/xtav60.h>
#include    <xtensa/xtbsp.h>

/* Flash device is an Atmel AT49BV322A, ST M29W320xB, or similar. */
/* This program uses chip erase mode so is agnostic to block geometry. */

#define FLASHADDR XTBOARD_FLASH_VADDR     /* base address of flash */
#define FLASHSIZE XTBOARD_FLASH_MAXSIZE   /* size of entire flash (bytes) */
#define FLASHTOP  (FLASHADDR + FLASHSIZE) /* top+1 address of entire flash */
#ifdef XTBOARD_RAM_VADDR
#define BINARYADDR (XTBOARD_RAM_VADDR + 0x1000000)  /* base of RAM image */
#else
#define BINARYADDR 0                      /* (must set image_base manually) */
#endif

#define CMDADDR(cmd) ((((cmd)&0xfff)<<1) | FLASHADDR)
#define flashread(addr) (*(volatile short *)addr)
#define flashwrite(addr, data) *(volatile short *)addr = data

/* User can change these variables from debugger after loading. */
/* Set flash_base to locate image other than at base of flash. */
/* Set image_size to avoid programming entire region. */
/* This program erases the entire flash at once, but clearing flash_erase
   allows multiple images to be programmed to different flash regions. */
/* To erase entire flash w/o programming, set image_size = 0. */
unsigned flash_base = FLASHADDR;    /* where image starts in flash */
unsigned image_base = BINARYADDR;   /* where image starts in memory */
unsigned image_size = FLASHSIZE;    /* size of image, amount programmed */
int      flash_erase = 1;           /* erase flash before programming */


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
  xtbsp_display_string("Flash Error !!!");
  flashwrite(FLASHADDR, 0xF0);          /* restore read mode and clear status */
  putstring("Error ");
  putstring(msg); 
  putstring(" at address ");
  puthex(addr, 8);
  putstring(", flash status ");
  puthex(stat, 2);
  putstring("\n");
  exit(err);
}

/* Erase entire flash. */
static void eraseFlash(void)
{
  flashwrite(CMDADDR(0x555), 0xAA);     /* 6 cycle chip erase command */
  flashwrite(CMDADDR(0xAAA), 0x55);
  flashwrite(CMDADDR(0x555), 0x80);
  flashwrite(CMDADDR(0x555), 0xAA);
  flashwrite(CMDADDR(0xAAA), 0x55);
  flashwrite(CMDADDR(0x555), 0x10);
  while (flashread(FLASHADDR) != 0xFFFFFFFF);
}

/* Program image from RAM to flash (caller ensures blank space is available). */
static void programFlash(unsigned image, unsigned flash, unsigned size)
{
  unsigned short stat, data7;
  unsigned short *imagepos;
  unsigned flashpos;

  flashwrite(FLASHADDR, 0xF0);          /* restore read mode and clear status */
  for (                                 /* for each 16b word ... */
    flashpos = flash, imagepos = (unsigned short *)image;
    flashpos < flash + size && flashpos < FLASHTOP;
    flashpos += 2, ++imagepos
    ) {
    data7 = *imagepos & 0x80;
    flashwrite(CMDADDR(0x555), 0xAA);   /*   3 cycle prefix to program mode */
    flashwrite(CMDADDR(0xAAA), 0x55);
    flashwrite(CMDADDR(0x555), 0xA0);
    flashwrite(flashpos, *imagepos);    /*   data */
    while ( ((stat = flashread(flashpos)) & 0x80) != data7 ) {  /* done? */
      if ((stat & 0x20) == 0) continue; /* no error signal in status bit 5 */
      if (((stat = flashread(flashpos)) & 0x80) == data7) break; /* recheck */
      error(1, "programming flash word", flashpos, stat);
    }
  }
}

/* Erase and program flash. Notify user of progress via display and console. */
int main(int argc, char *argv[]){
  if (flash_base < FLASHADDR  || flash_base >= FLASHTOP) {
    putstring("Error - flash region is not within flash.\n");
    return 1;
  }
  if (flash_base + image_size > FLASHTOP) {
    putstring("Error - image is too big for flash region.\n");
    return 1;
  }
  putstring("Programming Flash on XT-AV60 board.\n");
  if (flash_erase) {
    xtbsp_display_string("Erase Flash...");
    putstring("Erase Flash...\n");
    eraseFlash();
  }
  xtbsp_display_string("Program Flash...");
  putstring("Program Flash...\n");
  programFlash(image_base, flash_base, image_size);
  xtbsp_display_string("Flash Done !!!");
  putstring("Flash Done !!!\n");

  return 0;
}

