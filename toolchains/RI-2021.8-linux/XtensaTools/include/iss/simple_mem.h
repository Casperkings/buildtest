/* Interface for a fast Simple Memory Model. */

/* Copyright (c) 2004-2012 by Tensilica Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of Tensilica Inc.
 * They may not be modified, copied, reproduced, distributed, or disclosed to
 * third parties in any manner, medium, or form, in whole or in part, without
 * the prior written consent of Tensilica Inc.
 */
#ifndef SIMPLE_MEM_STATE_H
#define SIMPLE_MEM_STATE_H

/* When compiling in the trace, set 
   USE_PIFWIDTH_#
   USE_{LE_BE, LE_LE, BE_LE, BE_BE}
   USE_UNALIGNED
   USE_READ_{8,16,32}  USE_WRITE_{8,16,32}
   to reduce compile times
*/

typedef unsigned u32;
typedef unsigned long long u64;

/* encodings for cache attributes.  Only used when simulating caches. */
#define CA_IFETCH_INVALID 0
#define CA_IFETCH_BYPASS 1
#define CA_IFETCH_CACHED 2

#define CA_DLOAD_INVALID 0
#define CA_DLOAD_BYPASS 1
#define CA_DLOAD_NOALLOC 2
#define CA_DLOAD_ISOLATED 3
#define CA_DLOAD_CACHED 4

#define CA_DSTORE_INVALID 0
#define CA_DSTORE_BYPASS 1
#define CA_DSTORE_ISOLATED 2
#define CA_DSTORE_WRITE_THRU_NOALLOC 3
#define CA_DSTORE_WRITE_THRU_ALLOC 4
#define CA_DSTORE_WRITE_BACK 5

/* This is for functions that read 1, 2, 4 bytes and optionally swap.
   The content_size and the access width ARE NOT passed, but
   offset < content size and the offset must be aligned to
   the access width.  There are only 5 read and write functions
   that are needed this way.
 */

typedef void (*fast_load_fn)(void * /*callback data*/,
			     u32 * /*dst*/,
			     u64 /*local_address*/,
			     u32 /*size*/);

typedef void (*fast_store_fn)(void * /*callback data*/,
			      u64 /*local_address*/,
			      u32 /*size*/,
			      const u32 * /*src*/);

/* These are the same as above with an additional custom data argument */

typedef void (*custom_load_fn)(void * /*callback data*/,
                               u32 * /*dst*/,
                               u64 /*local_address*/,
                               u32 /*size*/,
                               const u32 * /*custom data*/);

typedef void (*custom_store_fn)(void * /*callback data*/,
                                u64 /*local_address*/,
                                u32 /*size*/,
                                const u32 * /*src*/,
                                const u32 * /*custom data*/);


struct raw_mem_block {
  struct raw_mem_block *next;
  unsigned char *contents;
  unsigned content_size;
  unsigned high_mask;

  unsigned vaddress;
  unsigned paddress;

  unsigned master; /* 0:None, 1:Data, 2:Inst, 3:iDMA */
  unsigned ring; /* 0 or 1, used with MPU */

  unsigned ldst_eca; /* ISA encoded memory attribute 0..15 */

  int has_custom_callbacks;

  int is_ca_executable;
  int is_ca_readable;
  int is_ca_writable;
  
  int is_mem_readable;
  int is_mem_writable;

  /* The ones that are usually used */
  int is_readable;
  int is_writable;
  int is_executable;

  int has_read_watchpoint;
  int has_write_watchpoint;

  int is_l32_readable; /* iram/rom are readable with l32{i,i.n,r} */
  int is_s32_writable; /* iram is writable with s32i */

  int is_apb; /* is accessing APB port */
  int is_secure; /* is accessing secure region of port */
  int is_mmio; /* is accessing MMIO region, RNX only */

  int is_turbo_readable; /* is_readable and no watchpoints */
  int is_turbo_writable; /* is_writable and no watchpoints */
  int is_turbo_l32_readable; /* is_l32_readable and no watchpoints */

  void *port; /* local memory PORT */

  /* User fast-access callbacks used if the contents is NULL */
  fast_load_fn fast_load;
  fast_store_fn fast_store;
  void *callback_data;
  unsigned paddress_offset;
  
  unsigned mem_byte_xor_bits;
  /* xor with address to to read 1 byte offset. This is also used with 2, 4
   * byte offsets because the bottom bits are truncated.
   * mem_byte_xor_bits ==
   *   (is_mem_ms_word ? (mem_word_size-4) : 0)  + (is_mem_ms_byte ? 3 : 0)
   */

  unsigned core_word_xor_bits;
  /* For big endian cores, swap word when reading wide values
   * core_word_xor_bits == is_core_ms_byte ? core_word_size-1 : 0
   */

  unsigned is_loaded_byte_swap; 
  /* when reading 2 or 4 words, swap the bytes when this is true.
   * host_load_xor == host_big_endian ? 3 : 0;
   * is_loaded_byte_swap == 
   *   ((mem_byte_xor_bits ^ host_load_xor ^ core_word_xor_bits) & 3) == 3
   */
};


/* This is the fast version of the memory map. We will be probably be
   compiling code to do the lookup.  */
   
struct fast_memory_map {
  struct raw_mem_block **memblocks; /* memblocks. non-overlapping */
  struct raw_mem_block ***l1_memblocks; /* Direct 2-level lookup */
  struct raw_mem_block **l1_sentinal;
  unsigned l1_shift;
  unsigned l1_mask;
  unsigned l1_entry_count;
  unsigned l2_shift;
  unsigned l2_mask;
  unsigned l2_entry_count;

  unsigned memblock_count;
  unsigned memblock_alloced; /* number allocated */
};


/* There are two different ways to do endian swapping.
   
   In the first, the simulated memory is always kept in
   host endian order.  This has the advantage that 
   simulated loads and stores of aligned quantities 
   are just regular loads and stores.  
   32 bit loads and stores are host loads and stores.
   16 bit aligned loads and stores twiddle the address.
      vaddress = (vaddress ^ 3) -1
   8 bit aligned loads and stores twiddle the address.
      vaddress = (vaddress ^ 3)
   Unaligned 32 and 16 bit values require loading
      two words, shift and mask to store into memory
      and reload.
   
   In the second, the simulated memory is kept in simulation
   order.  This has the advantage that a host load and store
   of the right address can always be used, but values must be
   swapped after load or before store.

   In either case, the XTCORE_BASE::peek() must return memory in core
   order XTCORE_BASE::copyToHost() returns memory in host order.
   
*/
      
#if NOINLINE_COMPILE

  /* This is used so O0 compiles that do not inline these
     do not need to compile them all the time. */

  struct raw_mem_block *
  memory_map_lookup(const struct fast_memory_map *mem_map, unsigned vaddress);

  unsigned memblock_read_u8_vaddress(const struct raw_mem_block *m,
				    unsigned offset);
  unsigned memblock_read_u16_vaddress(const struct raw_mem_block *m,
				     unsigned offset);
  unsigned memblock_read_u32_vaddress(const struct raw_mem_block *m,
				     unsigned offset);
  void memblock_read_wide_vaddress(const struct raw_mem_block *m,
				  unsigned *dst,
				  unsigned size, /* 8, 16, ... */
				  unsigned vaddress);

  void memblock_write_u8_vaddress(const struct raw_mem_block *m,
				unsigned vaddress,
				unsigned value);
  void memblock_write_u16_vaddress(const struct raw_mem_block *m,
				  unsigned vaddress,
				  unsigned value);
  void memblock_write_u32_vaddress(const struct raw_mem_block *m,
				  unsigned vaddress,
				  unsigned value);
  void memblock_write_wide_vaddress(const struct raw_mem_block *m,
				    unsigned vaddress,
				    unsigned size,
				    const unsigned *src);

#else  /* NOINLINE_COMPILE */

#ifdef __cplusplus
extern "C" {
#endif

  /* Inline aggressively for -O2 support */
  inline struct raw_mem_block *
  memory_map_lookup(const struct fast_memory_map *mem_map,
		    unsigned address) {
    /* level 1 is 14 bits. Level 2 is 9 bits when min size is 512.
       l1 is DENSE, but may refere to a canonical not-allocated marker.
       l2 is may currently have NULL values, but should be converted to
       be dense later
     */
    unsigned l1 = (address >> mem_map->l1_shift);
    unsigned l2 = (address >> mem_map->l2_shift) & mem_map->l2_mask;
    struct raw_mem_block *m = mem_map->l1_memblocks[l1][l2];
    return m;
  }

  inline unsigned memblock_read_u8(const struct raw_mem_block *m,
				   unsigned offset) {
    unsigned char *contents = m->contents;
    if (contents) {
      unsigned swizzle_offset = offset ^ m->mem_byte_xor_bits;
      return (unsigned)contents[swizzle_offset];
    } else {
      u32 v;
      m->fast_load(m->callback_data, &v,
		   offset + m->paddress - m->paddress_offset,
		   1);
      return v;
    }
  }

  inline unsigned memblock_read_u8_vaddress(const struct raw_mem_block *m,
					    unsigned vaddress) {
    return memblock_read_u8(m, vaddress - m->vaddress);
  }

  inline unsigned memblock_read_u16(const struct raw_mem_block *m,
				    unsigned offset) {
    unsigned short *contents = (unsigned short *)m->contents;
    if (m->contents) {
      unsigned swizzle_offset = offset ^ m->mem_byte_xor_bits;
      unsigned value = contents[swizzle_offset>>1];
      if (m->is_loaded_byte_swap) {
	value = ((value << 8) | value >> 8) & 0xffff;
      }
      return value;
    } else {
      u32 v;
      m->fast_load(m->callback_data, &v,
		   offset + m->paddress - m->paddress_offset,
		   2);
      return v;
    }
  }

  inline unsigned memblock_read_u16_vaddress(const struct raw_mem_block *m,
					    unsigned vaddress) {
    return memblock_read_u16(m, vaddress - m->vaddress);
  }

  inline unsigned memblock_read_u32(const struct raw_mem_block *m,
				    unsigned offset) {
    unsigned *contents = (unsigned *)m->contents;
    if (contents) {
      unsigned swizzle_offset = offset ^ m->mem_byte_xor_bits;
      unsigned value = contents[swizzle_offset>>2];
      if (m->is_loaded_byte_swap) {
	value = ((value << 24) | ((value >> 8) &   0xff00)
		 | ((value << 8) & 0xff0000) | (value >> 24));
      }
      return value;
    } else {
      u32 v;
      m->fast_load(m->callback_data, &v,
		   offset + m->paddress - m->paddress_offset,
		   4);
      return v;
    }
  }

  inline unsigned memblock_read_u32_vaddress(const struct raw_mem_block *m,
					    unsigned vaddress) {
    return memblock_read_u32(m, vaddress - m->vaddress);
  }

  inline unsigned memblock_read_small(const struct raw_mem_block *m,
				      unsigned offset,
				      unsigned size /* 1, 2, 4 */
				      ) {
    /* This is a complete function using no function pointers.
       This can be sped up by using function pointers to functions
       that compile in size, is_host_mem_byte_swap and possibly mem_xor_bits. 
    */
    /* assert: (offset & -size) == offset */
    if (size == 1) {
      return memblock_read_u8(m, offset);
    } else if (size == 2) {
      return memblock_read_u16(m, offset);
    }
    return memblock_read_u32(m, offset);
  }

  inline void memblock_read_wide(const struct raw_mem_block *m,
				 unsigned *dst,
				 unsigned size, /* 8, 16, ... */
				 unsigned offset) {
    if (m->contents) {
      unsigned i;
      unsigned num_words = size >>2;
      unsigned xbits = (m->core_word_xor_bits & (size-1)) >> 2;
      /* assert: size >= 4 */
      /* assert: (offset & -size) == offset */
      for (i = 0; i < num_words; ++i) {
	unsigned j = i ^ xbits;
	dst[j] = memblock_read_u32(m, offset + (i<<2));
      }
    } else {
      m->fast_load(m->callback_data, dst,
		   offset + m->paddress - m->paddress_offset,
		   size);
    }
  }

  inline void memblock_read_wide_vaddress(const struct raw_mem_block *m,
					 unsigned *dst,
					 unsigned size, /* 8, 16, ... */
					 unsigned vaddress) {
    return memblock_read_wide(m, dst, size, vaddress - m->vaddress);
  }

  inline void memblock_read(const struct raw_mem_block *m,
			    unsigned offset,
			    unsigned size, /* 1, 2, 4, 8, 16, ... */
			    unsigned *dst) {
    if (m->contents) {
      /* assert: (offset & -size) == offset */
      if (size <= 4) {
	dst[0] = memblock_read_small(m, offset, size);
      } else {
	memblock_read_wide(m, dst, size, offset);
      }
    } else {
      m->fast_load(m->callback_data, dst,
		   offset + m->paddress - m->paddress_offset,
		   size);
    }
  }

  inline void memblock_write_u8(const struct raw_mem_block *m,
			        unsigned offset,
			        unsigned value) {
    unsigned char *contents = m->contents;
    if (contents) {
      /* assert: offset < m->content_size */
      unsigned swizzle_offset = offset ^ m->mem_byte_xor_bits;
      m->contents[swizzle_offset] = (unsigned char)value;
    } else {
      m->fast_store(m->callback_data,
		    offset + m->paddress - m->paddress_offset,
		    1, &value);
    }
  }

  inline void memblock_write_u8_vaddress(const struct raw_mem_block *m,
				         unsigned vaddress,
				         unsigned value) {
    memblock_write_u8(m, vaddress - m->vaddress, value);
  }

  inline void memblock_write_u16(const struct raw_mem_block *m,
				 unsigned offset,
				 unsigned value) {
    unsigned short *contents = (unsigned short *)m->contents;
    if (m->contents) {
      /* assert: offset < m->content_size */
      /* assert: (offset & -2) == offset */
      unsigned swizzle_offset = offset ^ m->mem_byte_xor_bits;
      if (m->is_loaded_byte_swap) {
	value = ((value & 0xff) << 8) | ((value >> 8) & 0xff);
      }
      contents[swizzle_offset>>1] = (unsigned short)value;
    } else {
      m->fast_store(m->callback_data,
		    offset + m->paddress - m->paddress_offset,
		    2, &value);
    }
  }

  inline void memblock_write_u16_vaddress(const struct raw_mem_block *m,
					  unsigned vaddress,
					  unsigned value) {
    memblock_write_u16(m, vaddress - m->vaddress, value);
  }

  inline void memblock_write_u32(const struct raw_mem_block *m,
				 unsigned offset,
				 unsigned value) {
    unsigned *contents = (unsigned *)m->contents;
    if (contents) {
      /* assert: (offset & -4) == offset */
      unsigned swizzle_offset = offset ^ m->mem_byte_xor_bits;
      if (m->is_loaded_byte_swap) {
	value = ((value << 24) | ((value >> 8) &   0xff00)
		 | ((value << 8) & 0xff0000) | (value >> 24));
      }
      contents[swizzle_offset>>2] = value;
    } else {
      m->fast_store(m->callback_data,
		    offset + m->paddress - m->paddress_offset,
		    4, &value);
    }
  }

  inline void memblock_write_u32_vaddress(const struct raw_mem_block *m,
					unsigned vaddress,
					unsigned value) {
    memblock_write_u32(m, vaddress - m->vaddress, value);
  }

  inline void memblock_write_small(const struct raw_mem_block *m,
				   unsigned offset,
				   unsigned size, /* 1, 2, 4 */
				   unsigned value) {
    /* This is a complete function using no function pointers.
       This can be sped up by using function pointers. */
    /* assert: offset < m->content_size */
    /* assert: (offset & -size) == offset */
    if (size == 1) {
      memblock_write_u8(m, offset, value);
    } else if (size == 2) {
      memblock_write_u16(m, offset, value);
    } else {
      /* assert: size == 4 */
      memblock_write_u32(m, offset, value);
    }
  }

  inline void memblock_write_wide(const struct raw_mem_block *m,
				  unsigned offset,
				  unsigned size, /* 8, 16, ... */
				  const unsigned *src) {
    if (m->contents) {
      unsigned i;
      unsigned num_words = size >> 2;
      unsigned xbits = (m->core_word_xor_bits & (size-1)) >> 2;
      /* assert: offset < m->content_size */
      /* assert: size >= 4 */
      /* assert: (offset & -size) == offset */
      for (i = 0; i < num_words; ++i) {
	unsigned j = i ^ xbits;
	unsigned value = src[j];
	memblock_write_u32(m, offset + (i<<2), value);
      }
    } else {
      m->fast_store(m->callback_data,
		    offset + m->paddress - m->paddress_offset,
		    size, src);
    }
  }

  inline void memblock_write_wide_vaddress(const struct raw_mem_block *m,
					  unsigned vaddress,
					  unsigned size, /* 8, 16, ... */
					  const unsigned *src) {
    memblock_write_wide(m, vaddress-m->vaddress, size, src);
  }

  inline void memblock_write(const struct raw_mem_block *m,
			     unsigned offset,
			     unsigned size, /* 1, 2, 4, 8, 16, ... */
			     const unsigned *src) {
    /* assert: offset < m->content_size */
    /* assert: (offset & -size) == offset */
    if (size <= 4) {
      memblock_write_small(m, offset, size, src[0]);
    }
    else {
      memblock_write_wide(m, offset, size, src);
    }
  }


  /*
   * *******************************
   *   Swapping versions
   *   with core endian order
   * *******************************
   */

#if 0
  /* Endianness.  Iss4 stores memories in Core endian word order
     but in host endian byte order.  However, all internal buffers
     keep core words in little endian word order and host endian
     byte order.  

     For configs with unaligned accesses, access wraps within the pif
     width.
     In these examples, 0 is always the least significant byte.
     LE host LE 32 bit core:    0 1 2 3
     LE host BE 32 bit core:    0 1 2 3
     BE host LE 32 bit core:    3 2 1 0
     BE host BE 32 bit core:    3 2 1 0

     LE host LE 64 bit core:    0 1 2 3 / 4 5 6 7
     LE host BE 64 bit core:    4 5 6 7 / 0 1 2 3
     BE host LE 64 bit core:    3 2 1 0 / 7 6 5 4
     BE host BE 64 bit core:    7 6 5 4 / 3 2 1 0

     accessing a byte at vaddr:
     LE host LE 32 bit core:    mem[vaddr]
     LE host BE 32 bit core:    mem[vaddr^3]
     BE host LE 32 bit core:    mem[vaddr^3]
     BE host BE 32 bit core:    mem[vaddr]

     LE host LE 64 bit core:    mem[vaddr]
     LE host BE 64 bit core:    mem[vaddr^7]
     BE host LE 64 bit core:    mem[vaddr^3]
     BE host BE 64 bit core:    mem[vaddr^4]
     
     swizzle word: vaddr ^ (N-4)
     swizzle byte: vaddr ^ 3
     LE host LE core:    no swizzle
     LE host BE core:    swizzle word ^ swizzle byte
     BE host LE core:    swizzle byte
     BE host BE core:    swizzle word

     config allow unaligned access:
     addr = addr ^ (core_be ? (N-4) : 0) ^ (core_be != host_be) ? 3;

     Reading two bytes aligned:
     LE host LE 64 bit core:    mem[vaddr]
     LE host BE 64 bit core:    mem[vaddr^6]
     BE host LE 64 bit core:    mem[vaddr^2]
     BE host BE 64 bit core:    mem[vaddr^4]
     addr = addr ^ (core_be ? (N-4) : 0) ^ (core_be != host_be) ? 2;

     Reading four bytes aligned:
     LE host LE 64 bit core:    mem[vaddr]
     LE host BE 64 bit core:    mem[vaddr^4]
     BE host LE 64 bit core:    mem[vaddr]
     BE host BE 64 bit core:    mem[vaddr^4]
     addr = addr ^ (core_be ? (N-4) : 0)

     Reading 8 bytes aligned:
     LE host LE 64 bit core:    result[0] = mem[vaddr], 
                                  result[1] = mem[vaddr+4]
     LE host BE 64 bit core:    result[0] = mem[(vaddr+4)^4], 
                                  result[1] = mem[vaddr^4]
     BE host LE 64 bit core:    result[0] = mem[vaddr+4],
                                  result[1] = mem[vaddr]
     BE host BE 64 bit core:    result[0] = mem[vaddr^4],
                                  result[1] = mem[(vaddr+4)^4]
     
     Unaligned accesses:  3 options.
          config might throw exception.  i.e. if no exception,
	  always load aligned.
	  force alignment by removing bottom bits. lop off bottom bits
	  read shifted in the load buffer.

     Read shifted:
          For 2 byte values, read byte 0, byte 1 and put them together.
	      read at vaddr and (vaddr & ~(N-1)) | (vaddr+1)&(N-1)
	  For 4 byte values, read word 0, word 1 and put them together.
	  For 8 byte values, read 2-4 byte values.
  */
#endif

  /* 
   * *************************
   *   End of swapping code
   * *************************
   */

  /* This is only valid for virtually addressed memblocks */
  inline int memblock_contains(const struct raw_mem_block *m,
			       unsigned vaddress) {
    return (vaddress & m->high_mask) == m->vaddress;
  }
  inline int memblock_contains_physical(const struct raw_mem_block *m,
					unsigned paddress) {
    return (paddress & m->high_mask) == m->paddress;
  }

#ifdef __cplusplus
}
#endif

#endif /* NOINLINE_COMPILE */

#endif /* SIMPLE_VM_STATE_H */
