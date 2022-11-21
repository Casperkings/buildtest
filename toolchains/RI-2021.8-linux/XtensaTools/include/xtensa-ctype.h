/* Interface definition for configurable Xtensa ISA support
   to convert CTYPE values between memory- and register-
   layouts for the same CTYPE and between register-layouts values
   for different CTYPEs

   Copyright (c) 2010 Tensilica, Inc.  All Rights Reserved.

   This program is the copyrighted work of Tensilica Inc.  You may use
   it under the terms of version 2 or later of the GNU General Public
   License as published by the Free Software Foundation.  Other use is
   prohibited without the prior written consent of Tensilica Inc.

   This program is distributed WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  */

#ifndef XTENSA_LIBCTYPES_H
#define XTENSA_LIBCTYPES_H

#include <xtensa-isa.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum xtensa_ctype_status_enum
{
  xtensa_ctype_ok = 0,
  xtensa_ctype_internal_error
} xtensa_ctype_status;

#ifndef uint32
#define uint32 unsigned int
#endif

/* Returns TRUE, if there is a LOADI cstub function for CTYPE.
   In this case, CTYPE memory-layout value from *MV will be
   transformed to CTYPE register-layout value and copied to *RV.

   RV and MV must be aligned on CTYPE alignment.
*/
extern int xtensa_ctype_loadi (xtensa_ctype ctype, uint32 *rv, uint32 *mv );
    
/* Returns TRUE, if there is a STOREI cstub function for CTYPE.
   In this case, CTYPE register-layout value from *RV will be
   transformed to CTYPE memory-layout value and copied to *MV.

   RV and MV must be aligned on CTYPE alignment.
*/
extern int xtensa_ctype_storei (xtensa_ctype ctype, uint32 *rv, uint32 *mv );


/* Returns TRUE, if there is an RTOR cstub function to convert
   register-layout CT_FROM value to register-layout CT_TO value.
   In this case, CT_FROM register-layout value from *RF will be
   transformed to CT_TO register-layout value and copied to *RT.

   RF must be aligned on CT_FROM alignment.
   RT must be aligned on CT_TO alignment.
*/

extern int xtensa_ctype_rtor (xtensa_ctype ct_from,
			      xtensa_ctype ct_to,
			      uint32 *rf, uint32 *rt);

/* It needs to be called to initialize CTYPEs conversion library.
   Returns TRUE, if initialization was OK. The caller is responsible
   of providing the right LIBCTYPE dll pathname.
*/
extern int xtensa_ctype_library_init (const char *dll_name, char **error_msg_p);

#ifdef __cplusplus
}
#endif
#endif /* XTENSA_LIBCTYPES_H */
