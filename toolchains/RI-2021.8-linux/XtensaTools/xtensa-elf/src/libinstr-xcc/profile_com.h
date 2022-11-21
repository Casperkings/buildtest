/* 
  Copyright (C) 2002-2009 Tensilica, Inc.  All Rights Reserved.

  Copyright (C) 2000, 2001 Silicon Graphics, Inc.  All Rights Reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it would be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

  Further, this software is distributed without any warranty that it is
  free of the rightful claim of any third person regarding infringement 
  or the like.  Any license provided herein, whether implied or 
  otherwise, applies only to this software file.  Patent licenses, if 
  any, provided herein do not apply to combinations of this program with 
  other software, or any other product whatsoever.  

  You should have received a copy of the GNU General Public License along
  with this program; if not, write the Free Software Foundation, Inc., 59
  Temple Place - Suite 330, Boston MA 02111-1307, USA.

  Contact information:  Silicon Graphics, Inc., 1600 Amphitheatre Pky,
  Mountain View, CA 94043, or:

  http://www.sgi.com

  For further information regarding this notice, see:

  http://oss.sgi.com/projects/GenInfo/NoticeExplan

*/


//-*-c++-*-
// ====================================================================
// ====================================================================
//
// Module: profile_com.h
// $Revision: 1.13 $
// $Date: 2000/05/31 19:43:34 $
// $Author: dlstephe $
// $Source: /isms/cmplrs.src/osprey1.0/common/com/RCS/profile_com.h,v $
//
// Revision history:
//  24-Jul-98 - Original Version
//
// Description:
//
// ====================================================================
// ====================================================================

#ifndef profile_com_INCLUDED
#define profile_com_INCLUDED

#include <string.h>
#include <vector>
#include "defs.h"

#if defined(defs_INCLUDED) && ! defined(USE_STANDARD_TYPES)
#undef short                            // get around bogus type defs.
#undef int
#undef long
#endif // defined(defs_INCLUDED) && !defined(USE_STANDARD_TYPES)

// When to instrument?  Correlates to Instrumentation_Phase_Num
enum PROFILE_PHASE
{
  PROFILE_PHASE_NONE = -1,
  PROFILE_PHASE_BEFORE_VHO	= 0,
  PROFILE_PHASE_IPA_CUTOFF	= 0,	// phases less than or equal to
					// IPA_CUTOFF will not be
					// instrumented when the input file 
					// is an ipa-generated file.
  PROFILE_PHASE_BEFORE_LNO	= 1,
  PROFILE_PHASE_BEFORE_WOPT	= 2,
  PROFILE_PHASE_BEFORE_CG	= 3,
  PROFILE_PHASE_LAST		= 4,
};


/* Feedback File Format */

#define FB_NIDENT       16

#define INSTR_MAG          "\177INS"

#define INSTR_CURRENT      3

#define FB_WITH_IPA           0x01
#define FB_WITH_O0            0x02
#define FB_WITH_O1            0x04
#define FB_WITH_O2            0x08
#define FB_WITH_O3            0x10

#ifdef MONGOOSE_BE
#include "config_lno.h"
#include "config.h"

static inline UINT32 Calc_Feedback_Flags()
{
    UINT32 flags = 0;
    if (Current_LNO->IPA_Enabled)
      flags |= FB_WITH_IPA;
    switch (Opt_Level) 
      {
      case 0: flags |= FB_WITH_O0; break;
      case 1: flags |= FB_WITH_O1; break;
      case 2: flags |= FB_WITH_O2; break;
      case 3: flags |= FB_WITH_O3; break;
      }
    return flags;
}
#endif

enum INSTR_KIND {
  INSTR_KIND_NONE = 0,
  INSTR_KIND_INVOKE,
  INSTR_KIND_BRANCH,
  INSTR_KIND_LOOP,
  INSTR_KIND_CIRCUIT,
  INSTR_KIND_CALL,
  INSTR_KIND_SWITCH,
  INSTR_KIND_CGOTO,
  INSTR_KIND_RETURN
};

struct Fb_Hdr {
  char fb_ident[FB_NIDENT];	/* ident bytes */
  mUINT32 fb_version;		/* file version */
  mUINT32 opt_flags;		/* flags for determining compatibility
				   between the options passed to -fb_create
				   and fb_opt. */
  mUINT32 fb_profile_offset;	/* file offset for profile data */
  mUINT32 fb_pu_hdr_offset;	/* PU header file offset */
  mUINT32 fb_pu_hdr_ent_size;	/* PU header entry size */ 
  mUINT32 fb_pu_hdr_num;	/* Number of PU header entries */
  mUINT32 fb_str_table_offset;
  mUINT32 fb_str_table_size;
  PROFILE_PHASE phase_num;
  mUINT32 fb_dummy;		/* padding so that we don't read from
				   uninited memory when we 
				   write(file, &fb_hdr, sizeof(fb_hdr)) */
				   
  INT64 total_cycle_count;

  Fb_Hdr() : 
    fb_version(INSTR_CURRENT),
       fb_profile_offset(0),
       fb_pu_hdr_offset(0),
       fb_pu_hdr_ent_size(0),
       fb_pu_hdr_num(0),
       fb_str_table_offset(0),
       fb_str_table_size(0),
       phase_num(PROFILE_PHASE_NONE),
       fb_dummy(0),
       total_cycle_count(0)   
  {
    memset(fb_ident, 0, FB_NIDENT);
  }

  Fb_Hdr(Fb_Hdr& x) 
  {
    memcpy((void *)fb_ident,(void *)x.fb_ident, FB_NIDENT);
    fb_version = x.fb_version;
    fb_profile_offset = x.fb_profile_offset;
    fb_pu_hdr_offset = x.fb_pu_hdr_offset;
    fb_pu_hdr_ent_size = x.fb_pu_hdr_ent_size;
    fb_pu_hdr_num = x.fb_pu_hdr_num;
    fb_str_table_offset = x.fb_str_table_offset;
    fb_str_table_size = x.fb_str_table_size;
    phase_num = x.phase_num;
    total_cycle_count = x.total_cycle_count;
  }

  void Byteswap();
}; 

struct Pu_Hdr {
  INT32 pu_checksum;
  mUINT32 pu_opt_flags;
  mUINT32 pu_name_index;
  mUINT32 pu_file_offset;
  mUINT32 pu_cycle_count;
  mUINT32 pu_inv_offset;
  mUINT32 pu_num_inv_entries;
  mUINT32 pu_br_offset;
  mUINT32 pu_num_br_entries;
  mUINT32 pu_switch_offset;
  mUINT32 pu_switch_target_offset;	// # of targets for each swtich stmt
  mUINT32 pu_num_switch_entries;
  mUINT32 pu_cgoto_offset;
  mUINT32 pu_cgoto_target_offset;	// # of targets for each compgoto
  mUINT32 pu_num_cgoto_entries;
  mUINT32 pu_loop_offset;
  mUINT32 pu_num_loop_entries;
  mUINT32 pu_scircuit_offset;
  mUINT32 pu_num_scircuit_entries;
  mUINT32 pu_call_offset;
  mUINT32 pu_num_call_entries;
  mUINT32 pu_return_offset;
  mUINT32 pu_num_return_entries;
  mUINT32 pu_low_pc;

  Pu_Hdr() 
  {
    memset(this, 0, sizeof(*this));
  }

  void Byteswap();
};

#endif /* profile_com_INCLUDED */
