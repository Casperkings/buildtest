/* DWARF2 exception handling and frame unwinding for Xtensa.
   Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006,
   2007, 2008
   Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   In addition to the permissions in the GNU General Public License, the
   Free Software Foundation gives you unlimited permission to link the
   compiled version of this file into combinations with other programs,
   and to distribute those combinations without any restriction coming
   from the use of this file.  (The General Public License restrictions
   do apply in other respects; for example, they cover modification of
   the file, and distribution when not linked into a combined
   executable.)

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.  */

#if __XTENSA_CALL0_ABI__

/* Call0 ABI uses standard dwarf unwind mechanism.  */
#include "unwind-dw2.c"

#else

#include "tconfig.h"
#include "tsystem.h"
#include "coretypes.h"
#include "tm.h"
#include "dwarf2.h"
#include "unwind.h"
#ifdef __USING_SJLJ_EXCEPTIONS__
# define NO_SIZE_OF_ENCODED_VALUE
#endif
#include "unwind-pe.h"
#include "unwind-dw2-fde.h"
#include "unwind-dw2-xtensa.h"
#include <xtensa/config/core.h>

#ifndef __USING_SJLJ_EXCEPTIONS__

/* The standard CIE and FDE structures work fine for Xtensa but the
   variable-size register window save areas are not a good fit for the rest
   of the standard DWARF unwinding mechanism.  Nor is that mechanism
   necessary, since the register save areas are always in fixed locations
   in each stack frame.  This file is a stripped down and customized version
   of the standard DWARF unwinding code.  It needs to be customized to have
   builtin logic for finding the save areas and also to track the stack
   pointer value (besides the CFA) while unwinding since the primary save
   area is located below the stack pointer.  It is stripped down to reduce
   code size and ease the maintenance burden of tracking changes in the
   standard version of the code.  */

#ifndef DWARF_REG_TO_UNWIND_COLUMN
#define DWARF_REG_TO_UNWIND_COLUMN(REGNO) (REGNO)
#endif

#if !XCHAL_HAVE_XEA3
#define XTENSA_RA_FIELD_MASK 0x3FFFFFFF
#endif 

/* This is the register and unwind state for a particular frame.  This
   provides the information necessary to unwind up past a frame and return
   to its caller.  */
struct _Unwind_Context
{
  /* Track register window save areas of 4 registers each, instead of
     keeping separate addresses for the individual registers.  */
  _Unwind_Word *reg[DWARF_FRAME_REGISTERS+1];

  void *cfa;
  void *sp;
  void *ra;

#if !XCHAL_HAVE_XEA3
  /* Cache the 2 high bits to replace the window size in return addresses.  */
  _Unwind_Word ra_high_bits;
#endif 

  void *lsda;
  struct dwarf_eh_bases bases;
  /* Signal frame context.  */
#define SIGNAL_FRAME_BIT ((~(_Unwind_Word) 0 >> 1) + 1)
  _Unwind_Word flags;
  /* 0 for now, can be increased when further fields are added to
     struct _Unwind_Context.  */
  _Unwind_Word version;
};

/* Byte size of every register managed by these routines.  */
static unsigned char dwarf_reg_size_table[DWARF_FRAME_REGISTERS+1];


/* Read unaligned data from the instruction buffer.  */

union unaligned
{
  void *p;
  unsigned u2 __attribute__ ((mode (HI)));
  unsigned u4 __attribute__ ((mode (SI)));
  unsigned u8 __attribute__ ((mode (DI)));
  signed s2 __attribute__ ((mode (HI)));
  signed s4 __attribute__ ((mode (SI)));
  signed s8 __attribute__ ((mode (DI)));
} __attribute__ ((packed));

static void uw_update_context (struct _Unwind_Context *, _Unwind_FrameState *);
static _Unwind_Reason_Code uw_frame_state_for (struct _Unwind_Context *,
					       _Unwind_FrameState *);

static inline void *
read_pointer (const void *p) { const union unaligned *up = p; return up->p; }

static inline int
read_1u (const void *p) { return *(const unsigned char *) p; }

static inline int
read_1s (const void *p) { return *(const signed char *) p; }

static inline int
read_2u (const void *p) { const union unaligned *up = p; return up->u2; }

static inline int
read_2s (const void *p) { const union unaligned *up = p; return up->s2; }

static inline unsigned int
read_4u (const void *p) { const union unaligned *up = p; return up->u4; }

static inline int
read_4s (const void *p) { const union unaligned *up = p; return up->s4; }

static inline unsigned long
read_8u (const void *p) { const union unaligned *up = p; return up->u8; }

static inline unsigned long
read_8s (const void *p) { const union unaligned *up = p; return up->s8; }


static inline _Unwind_Word
_Unwind_IsSignalFrame (struct _Unwind_Context *context)
{
  return (context->flags & SIGNAL_FRAME_BIT) ? 1 : 0;
}

static inline void
_Unwind_SetSignalFrame (struct _Unwind_Context *context, int val)
{
  if (val)
    context->flags |= SIGNAL_FRAME_BIT;
  else
    context->flags &= ~SIGNAL_FRAME_BIT;
}

/* Get the value of register INDEX as saved in CONTEXT.  */

inline _Unwind_Word
_Unwind_GetGR (struct _Unwind_Context *context, int index)
{
  _Unwind_Word *ptr;

  index = DWARF_REG_TO_UNWIND_COLUMN (index);
  if (index >= DWARF_XTENSA_OSP_SAVE_COLUMN)
    ptr = context->reg[index];
  else
    ptr = context->reg[index >> 2] + (index & 3);

  return *ptr;
}

/* Get the value of the CFA as saved in CONTEXT.  */

_Unwind_Word
_Unwind_GetCFA (struct _Unwind_Context *context)
{
  return (_Unwind_Ptr) context->cfa;
}

/* Overwrite the saved value for register INDEX in CONTEXT with VAL.  */

inline void
_Unwind_SetGR (struct _Unwind_Context *context, int index, _Unwind_Word val)
{
  _Unwind_Word *ptr;

  index = DWARF_REG_TO_UNWIND_COLUMN (index);
  if (index >= DWARF_XTENSA_OSP_SAVE_COLUMN)
    ptr = context->reg[index];
  else
    ptr = context->reg[index >> 2] + (index & 3);

  *ptr = val;
}

/* Retrieve the return address for CONTEXT.  */

inline _Unwind_Ptr
_Unwind_GetIP (struct _Unwind_Context *context)
{
  return (_Unwind_Ptr) context->ra;
}

/* Retrieve the return address and flag whether that IP is before
   or after first not yet fully executed instruction.  */

inline _Unwind_Ptr
_Unwind_GetIPInfo (struct _Unwind_Context *context, int *ip_before_insn)
{
  *ip_before_insn = _Unwind_IsSignalFrame (context);
  return (_Unwind_Ptr) context->ra;
}

/* Overwrite the return address for CONTEXT with VAL.  */

inline void
_Unwind_SetIP (struct _Unwind_Context *context, _Unwind_Ptr val)
{
  context->ra = (void *) val;
}

void *
_Unwind_GetLanguageSpecificData (struct _Unwind_Context *context)
{
  return context->lsda;
}

_Unwind_Ptr
_Unwind_GetRegionStart (struct _Unwind_Context *context)
{
  return (_Unwind_Ptr) context->bases.func;
}

void *
_Unwind_FindEnclosingFunction (void *pc)
{
  struct dwarf_eh_bases bases;
  const struct dwarf_fde *fde = _Unwind_Find_FDE (pc-1, &bases);
  if (fde)
    return bases.func;
  else
    return NULL;
}

_Unwind_Ptr
_Unwind_GetDataRelBase (struct _Unwind_Context *context)
{
  return (_Unwind_Ptr) context->bases.dbase;
}

_Unwind_Ptr
_Unwind_GetTextRelBase (struct _Unwind_Context *context)
{
  return (_Unwind_Ptr) context->bases.tbase;
}

#ifdef MD_UNWIND_SUPPORT
#include MD_UNWIND_SUPPORT
#endif

/* Extract any interesting information from the CIE for the translation
   unit F belongs to.  Return a pointer to the byte after the augmentation,
   or NULL if we encountered an undecipherable augmentation.  */

static const unsigned char *
extract_cie_info (const struct dwarf_cie *cie, struct _Unwind_Context *context,
		  _Unwind_FrameState *fs)
{
  const unsigned char *aug = cie->augmentation;
  const unsigned char *p = aug + strlen ((const char *)aug) + 1;
  const unsigned char *ret = NULL;
  _Unwind_Word utmp;

  /* g++ v2 "eh" has pointer immediately following augmentation string,
     so it must be handled first.  */
  if (aug[0] == 'e' && aug[1] == 'h')
    {
      fs->eh_ptr = read_pointer (p);
      p += sizeof (void *);
      aug += 2;
    }

  /* Immediately following the augmentation are the code and
     data alignment and return address column.  */
  p = read_uleb128 (p, &fs->code_align);
  p = read_sleb128 (p, &fs->data_align);

  if (cie->version == 1)
    fs->retaddr_column = *p++;
  else
    p = read_uleb128 (p, &fs->retaddr_column);
  fs->lsda_encoding = DW_EH_PE_omit;

  /* If the augmentation starts with 'z', then a uleb128 immediately
     follows containing the length of the augmentation field following
     the size.  */
  if (*aug == 'z')
    {
      p = read_uleb128 (p, &utmp);
      ret = p + utmp;

      fs->saw_z = 1;
      ++aug;
    }

  /* Iterate over recognized augmentation subsequences.  */
  while (*aug != '\0')
    {
      /* "L" indicates a byte showing how the LSDA pointer is encoded.  */
      if (aug[0] == 'L')
	{
	  fs->lsda_encoding = *p++;
	  aug += 1;
	}

      /* "R" indicates a byte indicating how FDE addresses are encoded.  */
      else if (aug[0] == 'R')
	{
	  fs->fde_encoding = *p++;
	  aug += 1;
	}

      /* "P" indicates a personality routine in the CIE augmentation.  */
      else if (aug[0] == 'P')
	{
	  _Unwind_Ptr personality;
	  
	  p = read_encoded_value (context, *p, p + 1, &personality);
	  fs->personality = (_Unwind_Personality_Fn) personality;
	  aug += 1;
	}

      /* "S" indicates a signal frame.  */
      else if (aug[0] == 'S')
	{
	  fs->signal_frame = 1;
	  aug += 1;
	}

      /* Otherwise we have an unknown augmentation string.
	 Bail unless we saw a 'z' prefix.  */
      else
	return ret;
    }

  return ret ? ret : p;
}

/* Decode DWARF 2 call frame information. Takes pointers the
   instruction sequence to decode, current register information and
   CIE info, and the PC range to evaluate.  */

static void
execute_cfa_program (const unsigned char *insn_ptr,
		     const unsigned char *insn_end,
		     struct _Unwind_Context *context,
		     _Unwind_FrameState *fs)
{
  struct frame_state_reg_info *unused_rs = NULL;

  /* Don't allow remember/restore between CIE and FDE programs.  */
  fs->regs.prev = NULL;

  /* The comparison with the return address uses < rather than <= because
     we are only interested in the effects of code before the call; for a
     noreturn function, the return address may point to unrelated code with
     a different stack configuration that we are not interested in.  We
     assume that the call itself is unwind info-neutral; if not, or if
     there are delay instructions that adjust the stack, these must be
     reflected at the point immediately before the call insn.
     In signal frames, return address is after last completed instruction,
     so we add 1 to return address to make the comparison <=.  */
  while (insn_ptr < insn_end
	 && fs->pc < context->ra + _Unwind_IsSignalFrame (context))
    {
      unsigned char insn = *insn_ptr++;
      _Unwind_Word reg, utmp;
      _Unwind_Sword offset, stmp;

      if ((insn & 0xc0) == DW_CFA_advance_loc)
	fs->pc += (insn & 0x3f) * fs->code_align;
      else if ((insn & 0xc0) == DW_CFA_offset)
	{
	  reg = insn & 0x3f;
	  insn_ptr = read_uleb128 (insn_ptr, &utmp);
	  offset = (_Unwind_Sword) utmp * fs->data_align;
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].how
	    = REG_SAVED_OFFSET;
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].loc.offset = offset;
	}
      else if ((insn & 0xc0) == DW_CFA_restore)
	{
	  reg = insn & 0x3f;
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].how = REG_UNSAVED;
	}
      else switch (insn)
	{
	case DW_CFA_set_loc:
	  {
	    _Unwind_Ptr pc;
	    
	    insn_ptr = read_encoded_value (context, fs->fde_encoding,
					   insn_ptr, &pc);
	    fs->pc = (void *) pc;
	  }
	  break;

	case DW_CFA_advance_loc1:
	  fs->pc += read_1u (insn_ptr) * fs->code_align;
	  insn_ptr += 1;
	  break;
	case DW_CFA_advance_loc2:
	  fs->pc += read_2u (insn_ptr) * fs->code_align;
	  insn_ptr += 2;
	  break;
	case DW_CFA_advance_loc4:
	  fs->pc += read_4u (insn_ptr) * fs->code_align;
	  insn_ptr += 4;
	  break;

	case DW_CFA_offset_extended:
	  insn_ptr = read_uleb128 (insn_ptr, &reg);
	  insn_ptr = read_uleb128 (insn_ptr, &utmp);
	  offset = (_Unwind_Sword) utmp * fs->data_align;
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].how
	    = REG_SAVED_OFFSET;
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].loc.offset = offset;
	  break;

	case DW_CFA_restore_extended:
	  insn_ptr = read_uleb128 (insn_ptr, &reg);
	  /* FIXME, this is wrong; the CIE might have said that the
	     register was saved somewhere.  */
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN(reg)].how = REG_UNSAVED;
	  break;

	case DW_CFA_undefined:
	case DW_CFA_same_value:
	  insn_ptr = read_uleb128 (insn_ptr, &reg);
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN(reg)].how = REG_UNSAVED;
	  break;

	case DW_CFA_nop:
	  break;

	case DW_CFA_register:
	  {
	    _Unwind_Word reg2;
	    insn_ptr = read_uleb128 (insn_ptr, &reg);
	    insn_ptr = read_uleb128 (insn_ptr, &reg2);
	    fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].how = REG_SAVED_REG;
	    fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].loc.reg = reg2;
	  }
	  break;

	case DW_CFA_remember_state:
	  {
	    struct frame_state_reg_info *new_rs;
	    if (unused_rs)
	      {
		new_rs = unused_rs;
		unused_rs = unused_rs->prev;
	      }
	    else
	      new_rs = alloca (sizeof (struct frame_state_reg_info));

	    *new_rs = fs->regs;
	    fs->regs.prev = new_rs;
	  }
	  break;

	case DW_CFA_restore_state:
	  {
	    struct frame_state_reg_info *old_rs = fs->regs.prev;
	    fs->regs = *old_rs;
	    old_rs->prev = unused_rs;
	    unused_rs = old_rs;
	  }
	  break;

	case DW_CFA_def_cfa:
	  insn_ptr = read_uleb128 (insn_ptr, &fs->cfa_reg);
	  insn_ptr = read_uleb128 (insn_ptr, &utmp);
	  fs->cfa_offset = utmp;
	  fs->cfa_how = CFA_REG_OFFSET;
	  break;

	case DW_CFA_def_cfa_register:
	  insn_ptr = read_uleb128 (insn_ptr, &fs->cfa_reg);
	  fs->cfa_how = CFA_REG_OFFSET;
	  break;

	case DW_CFA_def_cfa_offset:
	  insn_ptr = read_uleb128 (insn_ptr, &utmp);
	  fs->cfa_offset = utmp;
	  /* cfa_how deliberately not set.  */
	  break;

	case DW_CFA_def_cfa_expression:
	  fs->cfa_exp = insn_ptr;
	  fs->cfa_how = CFA_EXP;
	  insn_ptr = read_uleb128 (insn_ptr, &utmp);
	  insn_ptr += utmp;
	  break;

	case DW_CFA_expression:
	  insn_ptr = read_uleb128 (insn_ptr, &reg);
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].how = REG_SAVED_EXP;
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].loc.exp = insn_ptr;
	  insn_ptr = read_uleb128 (insn_ptr, &utmp);
	  insn_ptr += utmp;
	  break;

	  /* Dwarf3.  */
	case DW_CFA_offset_extended_sf:
	  insn_ptr = read_uleb128 (insn_ptr, &reg);
	  insn_ptr = read_sleb128 (insn_ptr, &stmp);
	  offset = stmp * fs->data_align;
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].how
	    = REG_SAVED_OFFSET;
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].loc.offset = offset;
	  break;

	case DW_CFA_def_cfa_sf:
	  insn_ptr = read_uleb128 (insn_ptr, &fs->cfa_reg);
	  insn_ptr = read_sleb128 (insn_ptr, &fs->cfa_offset);
	  fs->cfa_how = CFA_REG_OFFSET;
	  fs->cfa_offset *= fs->data_align;
	  break;

	case DW_CFA_def_cfa_offset_sf:
	  insn_ptr = read_sleb128 (insn_ptr, &fs->cfa_offset);
	  fs->cfa_offset *= fs->data_align;
	  /* cfa_how deliberately not set.  */
	  break;

	case DW_CFA_val_offset:
	  insn_ptr = read_uleb128 (insn_ptr, &reg);
	  insn_ptr = read_uleb128 (insn_ptr, &utmp);
	  offset = (_Unwind_Sword) utmp * fs->data_align;
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].how
	    = REG_SAVED_VAL_OFFSET;
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].loc.offset = offset;
	  break;

	case DW_CFA_val_offset_sf:
	  insn_ptr = read_uleb128 (insn_ptr, &reg);
	  insn_ptr = read_sleb128 (insn_ptr, &stmp);
	  offset = stmp * fs->data_align;
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].how
	    = REG_SAVED_VAL_OFFSET;
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].loc.offset = offset;
	  break;

	case DW_CFA_val_expression:
	  insn_ptr = read_uleb128 (insn_ptr, &reg);
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].how
	    = REG_SAVED_VAL_EXP;
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].loc.exp = insn_ptr;
	  insn_ptr = read_uleb128 (insn_ptr, &utmp);
	  insn_ptr += utmp;
	  break;

	case DW_CFA_GNU_window_save:
	  /* ??? Hardcoded for SPARC register window configuration.  */
	  for (reg = 16; reg < 32; ++reg)
	    {
	      fs->regs.reg[reg].how = REG_SAVED_OFFSET;
	      fs->regs.reg[reg].loc.offset = (reg - 16) * sizeof (void *);
	    }
	  break;

	case DW_CFA_GNU_negative_offset_extended:
	  /* Obsoleted by DW_CFA_offset_extended_sf, but used by
	     older PowerPC code.  */
	  insn_ptr = read_uleb128 (insn_ptr, &reg);
	  insn_ptr = read_uleb128 (insn_ptr, &utmp);
	  offset = (_Unwind_Word) utmp * fs->data_align;
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].how
	    = REG_SAVED_OFFSET;
	  fs->regs.reg[DWARF_REG_TO_UNWIND_COLUMN (reg)].loc.offset = -offset;
	  break;

	default:
	  gcc_unreachable ();
	}
    }
}


/* Given the _Unwind_Context CONTEXT for a stack frame, look up the FDE for
   its caller and decode it into FS.  This function also sets the
   lsda member of CONTEXT, as it is really information
   about the caller's frame.  */

static _Unwind_Reason_Code
uw_frame_state_for (struct _Unwind_Context *context, _Unwind_FrameState *fs)
{
  const struct dwarf_fde *fde;
  const struct dwarf_cie *cie;
  const unsigned char *aug;
  int window_size;
  _Unwind_Word *ra_ptr;

  memset (fs, 0, sizeof (*fs));
  context->lsda = 0;

  fde = _Unwind_Find_FDE (context->ra + _Unwind_IsSignalFrame (context) - 1,
			  &context->bases);
  if (fde == NULL)
    {
#ifdef MD_FALLBACK_FRAME_STATE_FOR
      _Unwind_Reason_Code reason;
      /* Couldn't find frame unwind info for this function.  Try a
	 target-specific fallback mechanism.  This will necessarily
	 not provide a personality routine or LSDA.  */
      reason = MD_FALLBACK_FRAME_STATE_FOR (context, fs);
      if (reason != _URC_END_OF_STACK)
	return reason;
#endif
      /* The frame was not recognized and handled by the fallback function,
	 but it is not really the end of the stack.  Fall through here and
	 unwind it anyway.  */
    }
  else
    {
      cie = get_cie (fde);
      fs->pc = context->bases.func; 
      const unsigned char *insn, *end;
      insn = extract_cie_info (cie, context, fs);
      if (insn == NULL)
	/* CIE contained unknown augmentation.  */
	return _URC_FATAL_PHASE1_ERROR;

      /* First decode all the insns in the CIE.  */
      end = (unsigned char *) next_fde ((struct dwarf_fde *) cie);
      execute_cfa_program (insn, end, context, fs);

      /* Locate augmentation for the fde.  */
      aug = (unsigned char *) fde + sizeof (*fde);
      aug += 2 * size_of_encoded_value (fs->fde_encoding);
      insn = NULL;
      if (fs->saw_z)
	{
	  _Unwind_Word i;
	  aug = read_uleb128 (aug, &i);
	  insn = aug + i;
	}
      if (fs->lsda_encoding != DW_EH_PE_omit)
	{
	  _Unwind_Ptr lsda;

	  aug = read_encoded_value (context, fs->lsda_encoding, aug, &lsda);
	  context->lsda = (void *) lsda;
	}
      
      /* Then the insns in the FDE up to our target PC.  */
      if (insn == NULL)
        insn = aug;
      end = (unsigned char *) next_fde (fde);
      execute_cfa_program (insn, end, context, fs);
    }

  /* Check for the end of the stack.  This needs to be checked after
     the MD_FALLBACK_FRAME_STATE_FOR check for signal frames because
     the contents of context->reg[0] are undefined at a signal frame,
     and register a0 may appear to be zero.  (The return address in
     context->ra comes from register a4 or a8).  */
  ra_ptr = context->reg[0];
  if (ra_ptr && *ra_ptr == 0)
    return _URC_END_OF_STACK;

  /* Find the window size from the high bits of the return address.  */
#if XCHAL_HAVE_XEA3
    window_size = 8;
#else
  if (ra_ptr)
    window_size = (*ra_ptr >> 30) * 4;
  else
    window_size = 8;
#endif

  fs->retaddr_column = window_size;

  return _URC_NO_REASON;
}

/* Set the pointer to a register INDEX as saved in CONTEXT.  */

static inline void
_Unwind_SetGRPtr (struct _Unwind_Context *context, int index, void *p)
{
  index = DWARF_REG_TO_UNWIND_COLUMN (index);
  context->reg[index] = p;
}

/* Get the pointer to a register INDEX as saved in CONTEXT.  */

static inline void *
_Unwind_GetGRPtr (struct _Unwind_Context *context, int index)
{
  index = DWARF_REG_TO_UNWIND_COLUMN (index);
  return context->reg[index];
}

static inline void *
_Unwind_GetPtr (struct _Unwind_Context *context, int index)
{
  return (void *)(_Unwind_Ptr) _Unwind_GetGR (context, index);
}

static void
uw_update_context_1 (struct _Unwind_Context *context, _Unwind_FrameState *fs)
{
  struct _Unwind_Context orig_context = *context;
  _Unwind_Word *sp, *cfa, *next_cfa;
  int i, k;

  if (fs->signal_regs)
    {
      cfa = (_Unwind_Word *) fs->signal_regs[1];
      next_cfa = (_Unwind_Word *) cfa[-3];

      for (i = 0; i < 4; i++)
	context->reg[i] = fs->signal_regs + (i << 2);
    }
  else
    {
#if XCHAL_HAVE_XEA3
      int window_size = 2;
#else
      int window_size = fs->retaddr_column >> 2;
#endif

      sp = (_Unwind_Word *) orig_context.sp;
      cfa = (_Unwind_Word *) orig_context.cfa;

      /* Compute the addresses of all TIE callee-saved registers saved in
         this frame.  */
      if (fs->cfa_how != CFA_UNSET) {
        void *adjusted_cfa = NULL;
        if (fs->cfa_reg != __builtin_dwarf_sp_column()) {
          switch (fs->cfa_how) {
          case CFA_REG_OFFSET:
            adjusted_cfa = _Unwind_GetPtr(&orig_context, fs->cfa_reg);
            adjusted_cfa += fs->cfa_offset;
            break;
          default:
            gcc_unreachable();
          }
        }
        else
          adjusted_cfa = orig_context.sp + fs->cfa_offset;

        for (k = DWARF_XTENSA_OSP_SAVE_COLUMN;
             k < DWARF_FRAME_REGISTERS + 1; ++k) {
          switch (fs->regs.reg[k].how)
          {
          case REG_SAVED_OFFSET:
            _Unwind_SetGRPtr (context, k,
                (void *) (adjusted_cfa + fs->regs.reg[k].loc.offset));
            break;
          default:
              break;
          }
        }
      }

      // Only for functions with calls to eh_return
      if (_Unwind_GetGRPtr(context, DWARF_XTENSA_OSP_SAVE_COLUMN) &&
          _Unwind_GetGR (context, DWARF_XTENSA_OSP_SAVE_COLUMN)) {
        void *orig_sp =
            (void *)(_Unwind_GetGR (context, DWARF_XTENSA_OSP_SAVE_COLUMN));
        cfa = orig_sp + fs->cfa_offset;
        _Unwind_SetGR(context, DWARF_XTENSA_OSP_SAVE_COLUMN, 0);
      }


#if XCHAL_HAVE_XEA3
      next_cfa = (_Unwind_Word *) cfa[-7];
#else
      next_cfa = (_Unwind_Word *) cfa[-3];
#endif

#if XCHAL_HAVE_XEA3
      /* Find the extra save area below next_cfa.  */
      for (i = 0; i < window_size; i++)
	context->reg[i] = sp - 8  + 4*i;
#else
      /* Registers a0-a3 are in the save area below sp.  */
        context->reg[0] = sp - 4;
      /* Find the extra save area below next_cfa.  */
      for (i = 1; i < window_size; i++)
        context->reg[i] = next_cfa - 4 * (1 + window_size - i);
#endif
      /* Remaining registers rotate from previous save areas.  */
      for (i = window_size; i < 4; i++)
	context->reg[i] = orig_context.reg[i - window_size];
    }

  context->sp = cfa;
  context->cfa = next_cfa;

  _Unwind_SetSignalFrame (context, fs->signal_frame);
}

/* CONTEXT describes the unwind state for a frame, and FS describes the FDE
   of its caller.  Update CONTEXT to refer to the caller as well.  Note
   that the lsda member is not updated here, but later in
   uw_frame_state_for.  */

static void
uw_update_context (struct _Unwind_Context *context, _Unwind_FrameState *fs)
{
  uw_update_context_1 (context, fs);

  /* Compute the return address now, since the return address column
     can change from frame to frame.  */
  if (fs->signal_ra != 0)
    context->ra = (void *) fs->signal_ra;
  else
#if XCHAL_HAVE_XEA3
    context->ra = (void *) (_Unwind_GetGR (context, fs->retaddr_column));
#else
    context->ra = (void *) ((_Unwind_GetGR (context, fs->retaddr_column)
			     & XTENSA_RA_FIELD_MASK) | context->ra_high_bits);
#endif
}

static void
uw_advance_context (struct _Unwind_Context *context, _Unwind_FrameState *fs)
{
  uw_update_context (context, fs);
}

/* Fill in CONTEXT for top-of-stack.  The only valid registers at this
   level will be the return address and the CFA.  */

#define uw_init_context(CONTEXT)					   \
  do									   \
    {									   \
      __builtin_unwind_init ();						   \
      uw_init_context_1 (CONTEXT, __builtin_dwarf_cfa (),		   \
			 __builtin_return_address (0));			   \
    }									   \
  while (0)

static inline void
init_dwarf_reg_size_table (void)
{
  __builtin_init_dwarf_reg_size_table (dwarf_reg_size_table);
}

// Note, LLVM inlines this function when -O2 and above. Use attribute 'noinline'
// to prevent inlining
static void __attribute__((noinline))
uw_init_context_1 (struct _Unwind_Context *context, void *outer_cfa,
		   void *outer_ra)
{
  void *ra = __builtin_return_address (0);
  void *cfa = __builtin_dwarf_cfa ();
  void *orig_cfa = outer_cfa;
  _Unwind_FrameState fs;
  _Unwind_Reason_Code code;

  memset (context, 0, sizeof (struct _Unwind_Context));
  context->ra = ra;

  memset (&fs, 0, sizeof (fs));
  fs.retaddr_column = 8;
  context->sp = cfa;

#if __GTHREADS
    {
      static __gthread_once_t once_regsizes = __GTHREAD_ONCE_INIT;
      if (__gthread_once (&once_regsizes, init_dwarf_reg_size_table) != 0
    || dwarf_reg_size_table[0] == 0)
        init_dwarf_reg_size_table ();
    }
#else
    if (dwarf_reg_size_table[0] == 0)
      init_dwarf_reg_size_table ();
#endif

#if !XCHAL_HAVE_XEA3
  context->ra_high_bits =
    ((_Unwind_Word) uw_init_context_1) & ~XTENSA_RA_FIELD_MASK;
#endif

  code = uw_frame_state_for (context, &fs);
  gcc_assert (code == _URC_NO_REASON);
  fs.retaddr_column = 8;

  context->cfa = orig_cfa;
  fs.cfa_reg = __builtin_dwarf_sp_column();
  uw_update_context_1 (context, &fs);
  context->ra = outer_ra;
}


/* Install TARGET into CURRENT so that we can return to it.  This is a
   macro because __builtin_eh_return must be invoked in the context of
   our caller.  */

#define uw_install_context(CURRENT, TARGET)				 \
  ;									 \
    {									 \
      long offset = uw_install_context_1 ((CURRENT), (TARGET));		 \
      void *handler = __builtin_frob_return_addr ((TARGET)->ra);	 \
      __builtin_eh_return (offset, handler);				 \
    }									 \
  ;

static long
uw_install_context_1 (struct _Unwind_Context *current,
		      struct _Unwind_Context *target)
{
  long i;

  /* The eh_return insn assumes a window size of 8, so don't bother copying
     the save areas for registers a8-a15 since they won't be reloaded.  */
  for (i = 0; i < 2; ++i)
    {
      void *c = current->reg[i];
      void *t = target->reg[i];

      if (t && c && t != c)
	memcpy (c, t, 4 * sizeof (_Unwind_Word));
    }

  /* reload TIE callee-saved registers */
  for (i = DWARF_XTENSA_OSP_SAVE_COLUMN + 1; i < DWARF_FRAME_REGISTERS+1; ++i)
    {
      void *c = current->reg[i];
      void *t = target->reg[i];

      if (t && c && t != c)
        memcpy (c, t, dwarf_reg_size_table[i]);
    }
  return 0;
}

static inline _Unwind_Ptr
uw_identify_context (struct _Unwind_Context *context)
{
  return _Unwind_GetIP (context);
}


#include "unwind.inc"

#if defined (USE_GAS_SYMVER) && defined (SHARED) && defined (USE_LIBUNWIND_EXCEPTIONS)
alias (_Unwind_Backtrace);
alias (_Unwind_DeleteException);
alias (_Unwind_FindEnclosingFunction);
alias (_Unwind_ForcedUnwind);
alias (_Unwind_GetDataRelBase);
alias (_Unwind_GetTextRelBase);
alias (_Unwind_GetCFA);
alias (_Unwind_GetGR);
alias (_Unwind_GetIP);
alias (_Unwind_GetLanguageSpecificData);
alias (_Unwind_GetRegionStart);
alias (_Unwind_RaiseException);
alias (_Unwind_Resume);
alias (_Unwind_Resume_or_Rethrow);
alias (_Unwind_SetGR);
alias (_Unwind_SetIP);
#endif

#endif /* !USING_SJLJ_EXCEPTIONS */

#endif /* !__XTENSA_CALL0_ABI__  */
