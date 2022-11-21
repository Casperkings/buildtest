/*  Core-instance-specific parameters  (this file was generated -- do not edit)  */

/* Copyright (c) 2005-`(localtime)[5]+1900` Tensilica Inc.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

; #  $corename is passed from the Makefile
; my @cores = $layout->cores;
; my ($corenum) = grep($cores[$_]->name eq $corename, 0 .. $#cores);
; my $core = $cores[$corenum];
#define XSHAL_CORE_INDEX	`$corenum`

/*  The following are identical to the corresponding XMCORE_`$corenum`_xxx macros
 *  in <xtensa/system/mpsystem.h>:  */

/*#define XCORE_PRID			...*/
#define XCORE_CORE_NAME			"`$core->name`"
#define XCORE_CORE_NAME_ID		`$core->name`
#define XCORE_CONFIG_NAME		"`$core->config->name`"
#define XCORE_CONFIG_NAME_ID		`$core->config->name`
/*#define XCORE_CONFIG_INDEX		...*/

