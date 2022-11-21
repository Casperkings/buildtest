/* Copyright (c) 2003-2010 Tensilica Inc.  ALL RIGHTS RESERVED.
   These coded instructions, statements, and computer programs are the
   copyrighted works and confidential proprietary information of Tensilica Inc.
   They may not be modified, copied, reproduced, distributed, or disclosed to
   third parties in any manner, medium, or form, in whole or in part, without
   the prior written consent of Tensilica Inc.  */

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "bootloader.h"
#include "xtload-api.h"

/* -------------------------------------------------------------------
   Encoding and binary related
*/

typedef struct command_word_t {
  union {
    union {
      struct {
	uint32_t data      : 16;
	uint32_t prid      : 12;
	uint32_t broadcast : 1;
	uint32_t address   : 3;
      } xfer;
      struct {
	uint32_t data      : 29;
	uint32_t address   : 3;
      } bootloader;
    } cmd;
    uint32_t word;
  };
} command_word_t;


static command_word_t
encode_xfer_command (int cmd, int corenum)
{
  command_word_t word;
  word.cmd.xfer.address = BLA_XFER_CONFIG_WRITE;
  if (corenum == -1) {
    word.cmd.xfer.broadcast = 1;
    word.cmd.xfer.prid = 0;
  }
  else {
    word.cmd.xfer.broadcast = 0;
    word.cmd.xfer.prid = corenum;
  }
  word.cmd.xfer.data = cmd;
  return word;
}


static command_word_t
encode_bl_command (int cmd, int data)
{
  command_word_t word;
  word.cmd.bootloader.address = cmd;
  word.cmd.bootloader.data = data;
  return word;
}


static uint32_t
host_to_little_endian (uint32_t word)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
  return word;
#elif __BYTE_ORDER == __BIG_ENDIAN
  return (((word & 0x000000FF) << 24) | ((word & 0x0000FF00) << 8) |
	  ((word & 0x00FF0000) >> 8)  | ((word & 0xFF000000) >> 24));
#else
  #error Host has unknown endianness
#endif
}


/* -------------------------------------------------------------------
   Output
*/

#define XTENSA_BYTES_PER_WORD 4

static void
write_command_word (uint32_t raw)
{
  uint32_t cooked = host_to_little_endian(raw);
  xtload_user_output_word (cooked);
}

static void
write_data_word (uint32_t raw, int swap)
{
  uint32_t cooked;
  if (swap)
    cooked = (((raw & 0x000000FF) << 24) | ((raw & 0x0000FF00) << 8) |
	      ((raw & 0x00FF0000) >> 8)  | ((raw & 0xFF000000) >> 24));
  else
    cooked = raw;
  xtload_user_output_word (cooked);
}


static void
generate_setup_binary_transaction (xtload_address_t address, xtload_word_count_t words, int mode)
{
  command_word_t cmd;
  xtload_address_t xfer_addr = address / XTENSA_BYTES_PER_WORD;
  /* Strange detail of the hardware here. The count is word and
     zero-based.  So to write one word, binary length is set to 0.  */
  xtload_word_count_t xfer_words = words - 1;

  cmd = encode_bl_command(BLA_CORE_BINARY_ADDRESS, xfer_addr);
  write_command_word (cmd.word);
  cmd = encode_bl_command(BLA_BINARY_LENGTH, xfer_words);
  write_command_word (cmd.word);
  cmd = encode_bl_command(BLA_BOOTLOADER_MODE, mode);
  write_command_word (cmd.word);
}


static inline void
generate_bl_command (int bl_register, int value)
{
  command_word_t cmd = encode_bl_command(bl_register, value);
  write_command_word (cmd.word);
}


/* -------------------------------------------------------------------
   Interface
*/

void
xtload_bootloader_sleep (void)
{
  generate_bl_command (BLA_BOOTLOADER_MODE, BOOTLOADER_MODE_POWERSAVE);
}


void
xtload_bootloader_wake (void)
{
  /* Strictly speaking, all you have to do is write anything to the
     boot loader. It wakes and the actual operation is
     ignored. However, this most accurately reflects what really
     happens.  */
  generate_bl_command (BLA_BOOTLOADER_MODE, BOOTLOADER_MODE_CONFIGURE);
}


void
xtload_bootloader_done (void)
{
  generate_bl_command (BLA_DONE_OUTPUT_PIN_CTRL, 1);
}

void
xtload_bootloader_not_done (void)
{
  generate_bl_command (BLA_DONE_OUTPUT_PIN_CTRL, 0);
}

void
xtload_stall_and_target (core_number_t core)
{
  command_word_t command = encode_xfer_command (XFER_CMD_ACCEPT_CMDS_STALL, core);
  write_command_word(command.word);
}

void xtload_ignore_and_stall (core_number_t core)
{
  command_word_t command = encode_xfer_command (XFER_CMD_BYPASS_STALL, core);
  write_command_word(command.word);
}

void xtload_ignore_and_cont (core_number_t core)
{
  command_word_t command = encode_xfer_command (XFER_CMD_BYPASS_CONT, core);
  write_command_word(command.word);
}

void xtload_reset_and_cont (core_number_t core)
{
  command_word_t command = encode_xfer_command (CORE_CMD_RESET, core);
  write_command_word(command.word);
}

void xtload_stall_and_reset (core_number_t core)
{
  command_word_t command = encode_xfer_command (CORE_CMD_RESET_AND_STALL, core);
  write_command_word(command.word);
}


void
xtload_read_register (core_number_t core)
{
  command_word_t word;
  word.cmd.xfer.broadcast = 0;
  word.cmd.xfer.prid = core;
  word.cmd.xfer.address = BLA_XFER_CONFIG_READ;
  word.cmd.xfer.data = 0;
  write_command_word (word.word);
}


void xtload_read_words (xtload_address_t address, xtload_word_count_t words)
{
  generate_setup_binary_transaction (address, words, BOOTLOADER_MODE_READ);
}


void
xtload_write_words (int swap, xtload_address_t address, 
		    xtload_word_ptr_t data, xtload_word_count_t words)
{
  uint32_t i;

  generate_setup_binary_transaction (address, words, BOOTLOADER_MODE_LOAD);

  for (i = 0; i < words; i++)
    write_data_word (data[i], swap);
}


void
xtload_setup_write_words (xtload_address_t address, 
			  xtload_word_count_t words)
{
  generate_setup_binary_transaction (address, words, BOOTLOADER_MODE_LOAD);
}


void
xtload_zero_words (xtload_address_t address, xtload_word_count_t words)
{
  generate_setup_binary_transaction (address, words, BOOTLOADER_MODE_ZERO);
}
