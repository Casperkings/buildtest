/*
 * Copyright (c) 2003-2021 Cadence Design Systems, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

//-----------------------------------------------------------------------------
// Filename:    secmon-secure.h
//
// Contents:    Secure monitor function and object hooks for developers to 
//              optionally override.
//
// Usage:       #include in secure code only; not for use in nonsecure space.
//-----------------------------------------------------------------------------
#ifndef __SECMON_SECURE_H
#define __SECMON_SECURE_H

#include <xtensa/core-macros.h>
#include <xtensa/xtruntime.h>


//-----------------------------------------------------------------------------
// Secure MPU configuration settings.  
//
// Typically these are automatically generated based off the app-sim/app-board
// LSPs' copies of mpu_table.c, but alternately they can be overridden by 
// providing a copy via these object hooks.  
//
// When overriding, care must be taken to ensure all secure MPU entries are 
// configured as locked to avoid introducing a security vulnerability.  No
// checking of locked bits is performed on this table at run-time.
//-----------------------------------------------------------------------------
extern const struct xthal_MPU_entry 
__xt_mpu_init_table[] __attribute__((section(".ResetHandler.text")));


//-----------------------------------------------------------------------------
// Secure MPU configuration table size. Override when providing a custom table.
// See comments in __xt_mpu_init_table for further details.
//-----------------------------------------------------------------------------
extern const unsigned int
__xt_mpu_init_table_size __attribute__((section(".ResetHandler.text")));


//-----------------------------------------------------------------------------
// Secure MPU configuration table: number of secure entries. 
// Override when providing a custom table.
// See comments in __xt_mpu_init_table for further details.
//-----------------------------------------------------------------------------
extern const unsigned int
__xt_mpu_init_table_size_secure __attribute__((section(".ResetHandler.text")));


//-----------------------------------------------------------------------------
// Secure MPU cacheadrdis configuration.  
// See comments in __xt_mpu_init_table for further details.
//-----------------------------------------------------------------------------
extern const unsigned int
__xt_mpu_init_cacheadrdis __attribute__((section(".ResetHandler.text")));


//-----------------------------------------------------------------------------
// Secure MPU write-enable override hook. 
//-----------------------------------------------------------------------------
extern const unsigned int
__xt_mpu_nsm_write_enable __attribute__((section(".ResetHandler.text")));


//-----------------------------------------------------------------------------
// Secure initialization function hook.  Can be overridden.  Run prior to 
// unpacking nonsecure executable.
//-----------------------------------------------------------------------------
extern int32_t secmon_user_init(void);


//-----------------------------------------------------------------------------
// Nonsecure unpack function hook.  Can be overridden.  For ISS targets, 
// parameters contain command-line arguments.  Returns nonsecure entry point.
//-----------------------------------------------------------------------------
extern uint32_t secmon_unpack(int32_t argc, char **argv);


//-----------------------------------------------------------------------------
// Exception types that can be registered by a nonsecure application (via
// libsecmon.a).  Can be overridden or modified at your own risk.
//-----------------------------------------------------------------------------
extern uint8_t secmon_nonsecure_exceptions[XCHAL_EXCCAUSE_NUM];

#endif /* __SECMON_SECURE_H */

