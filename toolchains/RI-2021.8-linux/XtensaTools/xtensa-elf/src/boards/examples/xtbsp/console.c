/* Copyright (c) 2006-2007 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

/*
This example exercises the characer I/O of the minimal board support API
(xtbsp) on a supported board, when linked with the board's LSP. It accepts
simple console input and echoes it back as console output. The console device
is a UART connected to an RS-232 serial port on most boards. In such cases,
a terminal emulator connected to the serial port should be set to 38400 8N1.
*/

#include <ctype.h>
#include <string.h>
#include <xtensa/xtbsp.h>

#define putchar(c) outbyte(c)
#define getchar()  inbyte()

static void putstr(const char *s)
{
    char c;

    while ((c = *s) != '\0') {
        if (c == '\n') {
            putchar('\r');
            putchar('\n');
        }
        else if (iscntrl(c) && c != '\r') {
            putchar('^');
            putchar('@' + c);
        }
        else putchar(c);
        ++s;
    }
}

static void echochar(char c)
{
    if (c == '\b' || c == '\x7f') {
        putchar('\b');
        putchar(' ');
        putchar('\b');
    }
    else if (iscntrl(c)) {
        putchar('^');
        putchar('@' + c);
    }
    else if (isprint(c)) {
        putchar(c);
    }
}

static void getline(char *s, int size, int echo_on)
{
    int len = 0;

    while(1) {
        int c = getchar();
      
        switch (c) {
        case '\b':      /* BS */
        case '\x7f':    /* DEL */
            if (len > 0) {
                --len;
                if (echo_on) {
                    echochar(c);
                    if (iscntrl(s[len])) echochar(c);   /* '^' */
                }
            }
            else
                putchar('\a');
            break;
      
        case 0:         /* NUL */
            putchar('\a');
            break;
      
        case '\x15':    /* ^U */
            if (echo_on)
                while (len > 0) {
                    --len;
                    echochar('\b');
                    if (iscntrl(s[len])) echochar('\b');   /* '^' */
                }
            len = 0;
            break;
      
        case '\n':      /* LF */
        case '\r':      /* CR */
            s[len] = '\0';
            putchar('\r');
            putchar('\n');
            return;
      
        default:
            if (size > 0 && len < size) {
                s[len++] = c;
                if (echo_on)
                    echochar(c);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    char buf[81];

    putstr("\n\n"
           "/-----------------------------------------------------------\\\n"
           "| This is a test of the board's character I/O functionality |\n"
           "\\----------------------------------------------------------/\n");

    while(1) {
        putstr("\nType something> ");
        getline(buf, 80, 1);
        putstr("You typed \"");
        putstr(buf);
        putstr("\"\n");
        if (strcmp(buf, "quit") == 0) {
            putstr("Bye!\n");
            break;
        }
    }

    return 0;
}


