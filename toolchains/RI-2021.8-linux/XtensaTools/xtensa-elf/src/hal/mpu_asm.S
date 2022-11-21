// 
// mpu_asm.S - Assembler routine(s) for the MPU
//

// Copyright (c) 2016 - 2018 Tensilica Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <xtensa/coreasm.h>
#include <xtensa/mpuasm.h>
#include <xtensa/cacheasm.h>

#if !XCHAL_HAVE_XEA5 // todo
	.begin	schedule	// turn on assembler scheduling for this file
#endif

#if !XCHAL_HAVE_XEA5 // todo
#if defined(__SPLIT__write_map_helper)

/*
  void  xthal_write_map_raw_helper(const struct xthal_MPU_entry* map, unsigned n);

  Updates the MPU with the MPU entries provided:
	map	pointer to array of MPU entries
	n	number of entries in array (0 <= n <= XCHAL_MPU_ENTRIES)

  The entries provided must have monotonically increasing addresses.
  This function otherwise orders its updates to ensure the MPU always has
  all its entries in monotonically increasing sequence.

  on entry
  	a2 		=>	vector of MPU entries to write
  	a3		=>	number of entries to write
  	a4-a8	=>	destroyed
*/

DECLFUNC (xthal_write_map_helper)
	abi_entry
#if XCHAL_MPU_LOCK
        mov a10, a9 // Force any window overflow to 
		    // happen here before we flush cache
#endif
	dcache_writeback_inv_all a5, a6, a7
	icache_invalidate_all a6, a7
	mpu_write_map a2, a3, a4, a5, a6, a7, a8, a9, a10
	isync
	abi_return
	endfunc

#endif

#if defined(__SPLIT__write_map_raw) ||\
    defined(__SPLIT__write_map_raw_nw)

/*
  void  xthal_write_map_raw( const struct xthal_MPU_entry* map, unsigned n);

  Updates the MPU with the MPU entries provided:
	map	pointer to array of MPU entries
	n	number of entries in array (0 <= n <= XCHAL_MPU_ENTRIES)

  The entries provided must have monotonically increasing addresses.
  This function otherwise orders its updates to ensure the MPU always has
  all its entries in monotonically increasing sequence.

  on entry
  	a2 		=>	vector of MPU entries to write
  	a3		=>	number of entries to write
  	a4-a8	=>	destroyed
*/

DECLFUNC (xthal_write_map_raw)
	abi_entry
	mpu_write_map a2, a3, a4, a5, a6, a7, a8, a9, a10
	abi_return
	endfunc

#endif

/*
	void xthal_read_map_raw(struct xthal_MPU_entry* map);

	Reads the current map from the MPU and puts it the vector
	pointed to by map.

	a2		=> 	vector of MPU entries where the current MPU state is copied
	a3-a4	=>	destroyed
*/

#if defined(__SPLIT__read_map_raw) ||\
    defined(__SPLIT__read_map_raw_nw)

DECLFUNC (xthal_read_map_raw)
	abi_entry
	mpu_read_map a2, a3, a4
	abi_return
	endfunc

#endif

	.end	schedule

#if defined(XCHAL_HAVE_MPU) && XCHAL_HAVE_MPU

#if defined(__SPLIT__xthal_make_noncacheable)
/*
	internal use only

	1) Flush and invalidate caches
	2) Update the memorytype and access rights of n consecutive entries

	xthal_make_noncacheable(int32_t entry, int32_t n, uint32_t new_mt);
*/
DECLFUNC (xthal_make_noncacheable)
	abi_entry
	.begin no-schedule
	dcache_writeback_inv_all a9, a10, a11
	icache_invalidate_all a9, a10
	_movi		a10, 0x1 // make sure that no mem access
	_movi		a9, 0xFFFFFFE3 // make sure that no mem access
	floop		a3, 1f
	rptlb0 		a8, a2	// Read from MPU to get vaddr
	and		a8, a8, a9 // Mask off reserved bits
	or		a8, a8, a10 // set enable
	or		a7, a4, a2 // combine mt/ar with entry number
	wptlb		a7, a8 // update MPU
	addi		a2, a2, 1
	floopend	a3, 1f
	isync
	.end no-schedule
	abi_return
	endfunc
#endif
#endif
#endif
