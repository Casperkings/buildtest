/*******************************************************************************

Copyright (c) 2006-2010 by Tensilica Inc.  ALL RIGHTS RESERVED.
These coded instructions, statements, and computer programs are the
copyrighted works and confidential proprietary information of Tensilica Inc.
They may not be modified, copied, reproduced, distributed, or disclosed to
third parties in any manner, medium, or form, in whole or in part, without
the prior written consent of Tensilica Inc.
--------------------------------------------------------------------------------

lcd-splc780d-4bitmode.c  Driver for Sunplus SPLC780D LCD display 
controller (4 bit mode).

Simple driver for a SPLC780D LCD controller operating in 4 bit mode 
(board-independent implementation).

Used by board-support-packages with one or more LCDs driven by a SPLC780D 
operating in 4 bit mode.
Each instance of the controller uses a separate instance of the driver, 
initialized with the base address of the controller instance.

This is a very simple driver that implements only the minimal functionality
required by the BSP API (xtbsp.h) and handles the required timing loops by
the BSP's xtbsp_delay_ns() function (usually spins on the cycle counter).
More complex capabilities may be added later beyond what the BSP requires.

*******************************************************************************/

#include <xtensa/lcd-splc780d-4bitmode-board.h>  /* LCD controller and display info */
#include <xtensa/xtbsp.h>               /* BSP API */


/* Interface function comments are in the header <xtensa/lcd-splc780d-4bitmode.h>. */

void lcd_reset(splc780d_4bit_t *lcd)
{
    lcd->inst = SPLC780D_4BIT_INST_INIT1;
    xtbsp_delay_ns(20000000);    /* 20 ms */
    lcd->inst = SPLC780D_4BIT_INST_INIT2;
    xtbsp_delay_ns(15000000);     /* 15 ms */
    lcd->inst = SPLC780D_4BIT_INST_INIT2;
    xtbsp_delay_ns(5000000);     /* 5 ms */
    lcd->inst = SPLC780D_4BIT_INST_INIT2;
    xtbsp_delay_ns(5000000);     /* 5 ms */
    lcd->inst = SPLC780D_4BIT_INST_INIT3;
    xtbsp_delay_ns(5000000);     /* 5 ms */
}
    
void lcd_write_inst_byte(splc780d_4bit_t *lcd, unsigned char inst)
{
    lcd->inst = (inst & 0xF0);
    xtbsp_delay_ns(200);         /* 200 ns */
    lcd->inst = (inst << 4);
    xtbsp_delay_ns(1000000);     /* 1 ms */
}

void lcd_write_data_byte(splc780d_4bit_t *lcd, unsigned char data)
{
    lcd->data = (data & 0xF0);
    xtbsp_delay_ns(200);         /* 200 ns */
    lcd->data = (data << 4);
    xtbsp_delay_ns(1000000);     /* 1 ms */
}

void splc780d_4bit_init_default(splc780d_4bit_t *lcd)
{
    /* LCD reset sequence */
    lcd_reset(lcd);

    /* 4 bit data length,2 lines,5*8 */
    lcd_write_inst_byte(lcd,SPLC780D_4BIT_INST_SET_MODE);
    
    /* Set display ON, no cursor, no blinking */
    lcd_write_inst_byte(lcd,SPLC780D_4BIT_INST_DSPLY_ON);

    /* Clear display */
    lcd_write_inst_byte(lcd,SPLC780D_4BIT_INST_CLEAR);

    /* Set Cursor moving direction as increment */
    lcd_write_inst_byte(lcd,SPLC780D_4BIT_INST_CRSR_INC);
}

void splc780d_4bit_write_char(splc780d_4bit_t *lcd, unsigned pos, const char c)
{
    int line_no;

    /* Always start at the top line */
    line_no = 0; 

    /* Check display postition */
    if(pos >= DISPLAY_VISIBLE_LEN)
        return;

    int start_addr = line_no ? SPLC780D_4BIT_LINEB_ADDR : SPLC780D_4BIT_LINET_ADDR;

    lcd_write_inst_byte(lcd, start_addr | pos);
    lcd_write_data_byte(lcd, c);
}

void splc780d_4bit_write_string(splc780d_4bit_t *lcd, const char *s)
{
    unsigned pos;

    /* set pos to left and overwrite */
    lcd_write_inst_byte(lcd, SPLC780D_4BIT_LINET_ADDR); //Always use top line

    /* display chars up to end of string or visible window */
    for (pos = 0; *s != '\0' && pos < DISPLAY_VISIBLE_LEN; ++pos) {
        lcd_write_data_byte(lcd, *s++);
    }

    /* pad with spaces to end of visible window */
    for (; pos < DISPLAY_VISIBLE_LEN; ++pos) {
        lcd_write_data_byte(lcd, ' ');
    }
}

void splc780d_4bit_blank(splc780d_4bit_t *lcd)
{
    /* Clear display*/
    lcd_write_inst_byte(lcd, SPLC780D_4BIT_INST_CLEAR);
}

