/*  mingloss.c  -  glue functions for libgloss in absence of I/O  */

/* $Id: //depot/rel/Homewood/ib.8/Xtensa/OS/boards/mingloss.c#1 $ */

/*
 * Copyright (c) 2001-2002, 2005-2006 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */

/* For min-rt, provide a circular buffer for C library tty output: */

#define STDOUT_BUFSIZE	256	/* must be power of 2 */

char _minrt_stdout[STDOUT_BUFSIZE];
int  _minrt_stdout_next = 0;


void outbyte( int c )
{
    int next = _minrt_stdout_next;
    _minrt_stdout[next++] = c;
    _minrt_stdout_next = next & (STDOUT_BUFSIZE-1);
}

int inbyte( void )
{
    return -1;		/* always EOF */
}

