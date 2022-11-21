/* Exception handling and frame unwind runtime interface routines.
   Copyright (C) 2001, 2003, 2004, 2006 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.  */

/* This file is an implementation of the ABI described at: 

   http://www.codesourcery.com/public/cxx-abi/abi-eh.html

   Adjusted for 32-bit efficiency.

  */

#ifndef _UNWIND_H
#define _UNWIND_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned _Unwind_Word __attribute__((__mode__(__word__)));
typedef signed _Unwind_Sword __attribute__((__mode__(__word__)));
typedef unsigned _Unwind_Ptr __attribute__((__mode__(__pointer__)));
typedef unsigned _Unwind_Internal_Ptr __attribute__((__mode__(__pointer__)));
typedef int _Unwind_Action;

typedef unsigned int _Unwind_Word;
typedef signed int _Unwind_Sword;
typedef _Unwind_Word _Unwind_Ptr;
typedef _Unwind_Word _Unwind_Internal_Ptr;
typedef _Unwind_Sword _Unwind_Action;

/* 1.2 Data Structures.  */

/* Reason Codes  */

typedef enum {
  _URC_NO_REASON = 0,
  _URC_FOREIGN_EXCEPTION_CAUGHT = 1,
  _URC_FATAL_PHASE2_ERROR = 2,
  _URC_FATAL_PHASE1_ERROR = 3,
  _URC_NORMAL_STOP = 4,
  _URC_END_OF_STACK = 5,
  _URC_HANDLER_FOUND = 6,
  _URC_INSTALL_CONTEXT = 7,
  _URC_CONTINUE_UNWIND = 8
} _Unwind_Reason_Code;


/* Exception Header  */
typedef void (*_Unwind_Exception_Cleanup_Fn)
(_Unwind_Reason_Code reason,
 struct _Unwind_Exception *exc);

struct _Unwind_Exception {
  _Unwind_Word			 exception_class;
  _Unwind_Exception_Cleanup_Fn   exception_cleanup;
  _Unwind_Word			 private_1;
  _Unwind_Word			 private_2;
} __attribute__((aligned));


/* Unwind Context  */
struct _Unwind_Context;


/* 1.3 Throwing an Exception  */

_Unwind_Reason_Code _Unwind_RaiseException(struct _Unwind_Exception *exception_object);

typedef _Unwind_Reason_Code (*_Unwind_Stop_Fn)(int version,
					       _Unwind_Action actions,
					       _Unwind_Word exceptionClass,
					       struct _Unwind_Exception *exceptionObject,
					       struct _Unwind_Context *context,
					       void *stop_parameter );

_Unwind_Reason_Code _Unwind_ForcedUnwind(struct _Unwind_Exception *exception_object,
					 _Unwind_Stop_Fn stop, void *stop_parameter );

void _Unwind_Resume (struct _Unwind_Exception *exception_object);


/* 1.4 Exception Object Management  */

void _Unwind_DeleteException(struct _Unwind_Exception *exception_object);


/* 1.5 Context Management  */

_Unwind_Word _Unwind_GetGR(struct _Unwind_Context *context, int index);

void _Unwind_SetGR(struct _Unwind_Context *context, int index, _Unwind_Word new_value);

_Unwind_Word _Unwind_GetIP(struct _Unwind_Context *context);

void _Unwind_SetIP(struct _Unwind_Context *context, _Unwind_Word new_value);

void * _Unwind_GetLanguageSpecificData(struct _Unwind_Context *context);

_Unwind_Word _Unwind_GetRegionStart(struct _Unwind_Context *context);


/* 1.6 Personality Routine and actions  */

typedef _Unwind_Reason_Code (*_Unwind_Personality_Fn)(int version,
						      _Unwind_Action actions,
						      _Unwind_Word exceptionClass,
						      struct _Unwind_Exception *exceptionObject,
						      struct _Unwind_Context *context);

 typedef int _Unwind_Action;
 static const _Unwind_Action _UA_SEARCH_PHASE = 1;
 static const _Unwind_Action _UA_CLEANUP_PHASE = 2;
 static const _Unwind_Action _UA_HANDLER_FRAME = 4;
 static const _Unwind_Action _UA_FORCE_UNWIND = 8;
 
#ifdef __cplusplus
}
#endif

#endif /* xtensa-unwind.h */
