/* Copyright (c) 2005-2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems, Inc. They may not be modified, copied, reproduced, 
 * distributed, or disclosed to third parties in any manner, medium, or form, 
 * in whole or in part, without the prior written consent of Cadence Design 
 * Systems, Inc.*/

#ifndef __XMP_SUBSYSTEM_DEFS_H__
#define __XMP_SUBSYSTEM_DEFS_H__

/* 
 * To port XIPC to a custom subsystem provide definitions of the following
 * for your target subsystem.
 */

/* Max data cache line size across all processors. Assume 4 if no DCACHE */
#define XMP_MAX_DCACHE_LINESIZE 4

#endif /* __XMP_SUBSYSTEM_DEFS_H__ */
