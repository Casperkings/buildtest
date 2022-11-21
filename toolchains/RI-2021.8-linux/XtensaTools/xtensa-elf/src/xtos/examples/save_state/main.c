/*
 * Copyright (c) 2021 by Cadence Design Systems. ALL RIGHTS RESERVED.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * EXAMPLE:
 * Populate registers with known values then force an exception with an illegal
 * instruction.  Exception handle calls xtos_save_system_state and returns.
 * Application then generates RAM image and xt-gdb recovery script.
 */

#include <xtensa/core-macros.h>
#include <xtensa/xtruntime.h>
#include <xtensa/hal.h>
#include <stdio.h>
#include <string.h>


extern void _set_reg_values();
extern int __stack;

#if XCHAL_HAVE_XEA3
#define EXCCAUSE EXCCAUSE_INSTRUCTION
#elif XCHAL_HAVE_XEA2
#define EXCCAUSE EXCCAUSE_ILLEGAL
#endif

const char *register_set[] =
{
	"topmark",
	"sar",
	"pc",
	"ps",
#if XCHAL_HAVE_XEA3
	"wb",
	"epc",
#elif XCHAL_HAVE_XEA2
	"windowstart",
	"windowbase",
	"epc1",
#endif
	"exccause",
	"excvaddr",
#if XCHAL_HAVE_XEA2
	"excsave1",
#endif
	"ar"
};


#if XCHAL_HAVE_XEA3
void exc_handler(ExcFrame * ef)
#elif (XCHAL_HAVE_XEA1 || XCHAL_HAVE_XEA2)
void exc_handler(UserFrame *ef)
#endif
{
	xtos_save_system_state();
	ef->pc += 3;
}


void print_reg_script(FILE *out)
{
	int i;
	int ar;

	for (i = 2; strcmp(register_set[i], "ar"); ++i)
	{
		fprintf(out, "set $%s = 0x%08x\n", register_set[i], xtos_save_reg_mem[i]);
	}
	for (ar = 0; ar < XCHAL_NUM_AREGS; ++ar, ++i)
	{
#ifdef __XTENSA_WINDOWED_ABI__
		fprintf(out, "set $ar%d = 0x%08x\n", ar, xtos_save_reg_mem[i]);
#else
		fprintf(out, "set $a%d = 0x%08x\n", ar, xtos_save_reg_mem[i]);
#endif
	}
}


int main()
{
	FILE *f;
	int rv;
	char *stack_top = (char *)&__stack - XTOS_SAVE_STACK_MEM_SIZE;

	rv = xtos_set_exception_handler(EXCCAUSE, (xtos_handler)exc_handler, NULL);
	if (rv != 0)
	{
		return rv;
	}

	_set_reg_values();

	printf("RAM size = %d\n", XTOS_SAVE_STACK_MEM_SIZE);

	f = fopen("ram_image.bin", "wb");
	fwrite((char *)xtos_save_stack_mem, sizeof(char), XTOS_SAVE_STACK_MEM_SIZE, f);
	fclose(f);

	f = fopen("load_cmds.txt", "w");
	fprintf(f, "restore ram_image.bin binary 0x%08x\n", (uint32_t)stack_top);
	print_reg_script(f);
	fclose(f);

	return 0;
}
