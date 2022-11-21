#include <xtensa/system/mpsystem.h>
#include <xtensa/mpcore.h>
#include <stdio.h>

int main(void)
{
   printf("Core %d of %d:  my name is %s of system %s, my config is %s.\n",
	XSHAL_CORE_INDEX, XMP_NUM_CORES,
	XCORE_CORE_NAME, XMP_SYS_NAME, XCORE_CONFIG_NAME);
   return 0;
}

