#include <xtensa/xtruntime.h>

uint32_t xtos_save_reg_mem[XTOS_SAVE_REG_MEM_SIZE/sizeof(uint32_t)] __attribute__((weak));
uint32_t xtos_save_stack_mem[XTOS_SAVE_STACK_MEM_SIZE/sizeof(uint32_t)] __attribute__((weak));
uint32_t xtos_save_stack_size __attribute__((weak)) = XTOS_SAVE_STACK_MEM_SIZE;

