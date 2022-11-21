/*
 * Copyright (c) 2001-2007 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

#include <xtensa/xtbsp.h>

/* This examples counts in hex (at 10 counts per second) on the display. */

static char nibble_to_hex[] = { '0', '1', '2', '3', 
                                '4', '5', '6', '7', 
                                '8', '9', 'a', 'b', 
                                'c', 'd', 'e', 'f' };

static void hex_to_ascii(char *hexValue, const int value) 
{
  int i          = 0;
  int shiftvalue = 0;
  int use_blanks = 1;
  int nibble     = 0;

  for (i = 0; i < 8; ++i)
  {
    shiftvalue = 28 - (i << 2);
    nibble = (value >> shiftvalue) & 0xf;

    // This will turn leading zero's into blanks....
    if ( use_blanks )
      {
        if ( nibble == 0 )
          {
            hexValue[i] = ' ';
            continue;
          }
        use_blanks = 0;
      }

    hexValue[i] = nibble_to_hex[ (value >> shiftvalue) & 0xf ];
  }
  
  return;
}



int main(int argc, char *argv[])
{
  int i = 0;
  int j = 0;
  int value = 0;
  char hexValue[9];

  xtbsp_display_blank();

  for(;;)
  {
    hex_to_ascii(hexValue, value);
    hexValue[8] = 0;

    xtbsp_display_string(hexValue);
    //xtbsp_delay_cycles(160000);
    xtbsp_delay_ns(10000000);

    ++value;
  }

  return 0;
}

