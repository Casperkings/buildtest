/*
 * Copyright (c) 1999-2013 Tensilica Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <xtensa/config/core.h>
#include <xtensa/core-macros.h>
#include "xtensa-libdb-macros.h"
#include <xtensa/xtruntime-frames.h>
#if XCHAL_HAVE_WINDOWED
#include <xtensa/tie/xt_regwin.h>
#endif
#include <stdarg.h>
#include "xmon.h"
#include "xmon-internal.h"

#define XMON_LOG_SIZE 200
char    _msg_buf[XMON_LOG_SIZE];

int xmon_hex(const uint8_t ch)
{
  if (ch >= 'a' && ch <= 'f')
    return ch-'a'+10;
  if (ch >= '0' && ch <= '9')
    return ch-'0';
  if (ch >= 'A' && ch <= 'F')
    return ch-'A'+10;
  return -1;
}

void
_xmon_convert_var_to_hex(const uint32_t value, char* buf)
{
  int i;
  for (i = 0; i < 4; i++) 
  {
    uint8_t ch = (value >> 8*i) & 0xff;
    *buf++ = hexchars[ch >> 4];
    *buf++ = hexchars[ch & 0xf];
  }
  *buf = 0;
}

#if 0
static void
HexToString (char* out, int in)
{
  char buf[10] = {0};
  register char *s = buf + 10 - 1;

  register int t;
  register unsigned int u = in;
  int total_chars = 0;

  while (u) {
    t = u % 16;
    if( t >= 10 )
      t += 'a' - '0' - 10;
    *--s = t + '0';
    total_chars++;
    u /= 16;
  }
  while(total_chars--)
    *out++ = buf[10 - total_chars - 2];
  *out = '\0';
}
#endif

 int
xmon_charToHex (const unsigned char c)
{
  int i = (int)(c > '9' ? (c | 0x20) - ('a' - 10) : c - '0');
  return (i > 15 || i < 0) ? -1 : i;
}

/* While we find nice hex chars, build an int.
 * Return number of chars processed.
 * FIXME: Should parse this better and indicate if error (use strtoul()?).
 */
int
xmon_hexToInt(const char **ptr, unsigned int *intValue)
{
  int numChars = 0;
  int hexValue;

  *intValue = 0;

  while (**ptr)
  {
    hexValue = xmon_hex(**ptr);

    if (hexValue < 0)
      break;

    *intValue = (*intValue << 4) | hexValue;
    numChars ++;

    (*ptr)++;
  }
  //XLOG("Found %d chars\n", numChars);
  return (numChars);
}


int
xmon_is_bigendian()
{
  static int first_pass = 1;
  static int bigendian  = 0;

  if ( first_pass )
    {
      unsigned char test[4];
      unsigned long *pTest  = (unsigned long *) &test[0];

      test[0] = 0x12;
      test[1] = 0x34;
      test[2] = 0x56;
      test[3] = 0x78;

      if ( *pTest == 0x12345678 )
        bigendian = 1;

      first_pass = 0;
    }

  return bigendian;
}

/* string functions */
int
xmon_strlen( const char *cp )
{
  int i;
  if( cp == 0 )
    return 0;
  i = 0;
  while ( *cp )
    {
      i++;
      cp++;
    }
  return i;
}

#if 0
void
_strncpy( char *d, const char *s, int n )
{
  char *cp = d;
  if( d && s )
    {
      while( *s && --n != 0)
        {
          *cp = *s;
          cp++;
          s++;
        }
      *cp = 0;
    }
}
#endif

int
xmon_strncmp(const char* s1, const char* s2, int n)
{
  while(n--)
    if(*s1++!=*s2++)
      return *(unsigned char*)(s1 - 1) - *(unsigned char*)(s2 - 1);
  return 0;
}

char*
_xmon_strchr(const char *s, int c)
{
  while (*s != (char) c) {
  if (!*s++) {
      return NULL;
  }
  }
  return (char *)s;
}

void
xmon_memset(uint8_t *ptr, const uint8_t value, int num)
{
  while (num > 0)
  {
    *ptr = value;
    ++ptr;
    --num;
  }
}


/*
 * This macro will prints into a string.
 * Note that the pointer passed as "arg" will be moved forward on write.
 */
#define OUTPUT(arg, p, n) {                       \
        int   x1 = n;                             \
        char *x2 = ((char *)p);                   \
        while (x1) {                              \
            *(arg) = *(x2);                       \
            arg++; x2++; x1--;                    \
        }                                         \
}

/**************************************************************************/
/*
 *  Simple formatted output routine supporting only %x, %s, %d and %u
 */
int
xt_vprintf(char * outarg, const char * fmt, va_list ap)
{
    int total = 0;
    char c, space = ' ', buf[11];    /* largest 32-bit integer output (octal) */

    while ((c = *(char*)fmt++) != 0) {
        if (c != '%') {
            OUTPUT(outarg, &c, 1);
            total++;
            if(total > XMON_LOG_SIZE - 1)
              goto done;
        }
        else {
            int width = 0, len = 1, rightjust = 1;
            unsigned n;
            char *s = buf, *t;
            int sign = 0;

            while (1) {
                c = *(char*)fmt++;
                switch (c) {

                /* ignore (not supported) */
                case 'c':
                    buf[0] = va_arg(ap, int);
                    goto donefmt;
                case '#':
                case 'h':
                case 'l':
                case 'j':
                case 'z':
                case 't':
                case ' ':
                    sign = ' ';
                    break;
                case '+':
                    sign = '+';
                    break;
                case '-':
                    rightjust = 0;
                    break;

                case 'd':
                    n = va_arg(ap, int);
                    if ((int)n < 0) {
                        sign = '-';
                        n = -(int)n;
                    }
                    if (sign) {
                        if(rightjust) {
                          *s++ = sign;
                        }
                        else {
                          OUTPUT(outarg, &sign, 1); //(*out)(outarg, &sign, 1);
                          width--;
                          total++;
                          if(total > XMON_LOG_SIZE - 1)
                            goto done;
                        }
                    }
                    goto do_decimal;


                case 'u':
                    n = va_arg(ap, int);
do_decimal:
                    {
                        /*  (avoids division or multiplication)  */
                        int digit, i, seen = 0;

                        for (digit = 0; n >= 1000000000; digit++) {
                            n -= 1000000000;
                        }
                        for (i = 9;;) {
                            if (!seen && digit != 0) {
                                seen = i;
                            }
                            if (seen) {
                                *s++ = '0' + digit;
                            }
                            for (digit = 0; n >= 100000000; digit++) {
                                n -= 100000000;
                            }
                            if (--i == 0) {
                                *s++ = '0' + digit;
                                len = s - buf;
                                s = buf;
                                goto donefmt;
                            }
                            n = ((n << 1) + (n << 3));
                        }
                    }
                    /*NOTREACHED*/

                case 's':
                    s = va_arg(ap, char*);
                    if (s == 0) {
                        s = (char*)"(null)";
                    }
                    for (t = s; *t; t++) ;
                    len = t - s;
                    goto donefmt;

                case 'x':
                    n = va_arg(ap, unsigned);
                    s = buf + 8;
                    do {
                        *--s = "0123456789abcdef"[n & 0xF];
                        n = (unsigned)n >> 4;
                    } while (n);
                    len = buf + 8 - s;
                    goto donefmt;

                case 0:
                    goto done;

                default:
                  if (c >= '0' && c <= '9') {
                    width = ((width<<1) + (width<<3)) + (c - '0');
                  }
                  else {
                    buf[0] = c;    /* handles case of '%' */
                    goto donefmt;
                  }
                }
            }
            /*NOTREACHED*/
donefmt:
            total += (len < width)  ? width : len;

            if(total > XMON_LOG_SIZE - 1)
              total = XMON_LOG_SIZE;
            if(len > XMON_LOG_SIZE - 1)
              len = XMON_LOG_SIZE;

            OUTPUT(outarg, s, len);

            for (; len < width; len++) {
                if(len > XMON_LOG_SIZE - 1)
                  goto done;
                OUTPUT(outarg, &space, 1);
            }
        }
    }
done:
    return total;
}

/*
 * xt_sprintf - Replacement for sprintf
 */
int
xmon_sprintf(char * buf, const char * fmt, ...)
{
    int     n;
    va_list ap;

    va_start(ap, fmt);
    n = xt_vprintf(buf, fmt, ap);
    va_end(ap);
    buf[n] = 0;
    return n;
}

 
void _xmon_log(const int type, const char* fmt, ...)
{
  int  n = 0;
  va_list ap;

  if (!gAppLogOn && !gGdbLogOn)
    return;

  n += xmon_sprintf(_msg_buf, "XMON: ");

  va_start(ap, fmt);
  n += xt_vprintf(_msg_buf+n, (char*)fmt, ap);
  va_end(ap);

  _msg_buf[n] = 0;

  if(gAppLogOn)
    _xlogger(XMON_LOG, _msg_buf);

  if(gGdbLogOn)
    _xmon_putConsoleString(_msg_buf);
}

void _XMON_XGDB(const char* fmt, ...)
{
  va_list ap;
  int n = 0;

  va_start(ap, fmt);
  n += xmon_sprintf(_msg_buf, "XMON: ");
  n += xt_vprintf(_msg_buf, (char*)fmt, ap);
  va_end(ap);
  _msg_buf[n] = 0;

  _xmon_putConsoleString(_msg_buf);
}

