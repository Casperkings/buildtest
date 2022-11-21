#include <xtensa/config/core-isa.h>
#include "xmem.h"
#include "xmem_bank.h"
#include "xlmem.h"
#include "xmem_misc.h"

// Number of blocks for the heap manager
#define XMEM_BANK_HEAP_MGR_NUM_BLOCKS (32)

#define XMEM_BANK_SIG_VALUE           (999)

// Statically reserve bank managers, one per thread
#ifdef XMEM_XOS
#define XMEM_MAX_BANK_MGRS            (4)
#else
#define XMEM_MAX_BANK_MGRS            (1)
#endif

// Internal Structure to represent a memory bank
typedef struct {
  void     *_start;         // Start of free space
  uint32_t  _size;          // Size of free memory in this bank
  uint32_t  _is_contiguous; // Does the next bank immediately follow this
} xmem_internal_bank_t;

// Structure to represent a bank memory manager. With an RTOS, there is
// one such structure per thread
typedef struct {
  void                    *_thread;            // Owning thread
  uint32_t                 _num_banks;         // Number of banks
  xmem_internal_bank_t    *_banks;             // Array of bank info per bank
  union {
    xmem_stack_mgr_t *_sm;
    xmem_heap_mgr_t  *_hm;
  } _mgr;                                      // Use heap or stack
  xmem_bank_alloc_policy_t _alloc_policy;      // Bank allocation policy
  void                    *_straddle_alloc;    // Record pointer that straddles
                                               // bank boundary
  int                      _banks_initialized; // Has the banks been initialized
  // For blocking/signaling threads waiting on bank to be available
  // Below 2 fields are useful if using a global bank manager
#ifdef XMEM_XOS
  XosMutex                 _xos_mutex;        
  XosCond                  _xos_cond;
#endif
} xmem_bank_mgr_t;

// Internal consistency check
#if (XMEM_BANK_CHECKPOINT_SIZE != XMEM_STACK_MGR_CHECKPOINT_SIZE)
  #error "XMEM_BANK_CHECKPOINT_SIZE and XMEM_STACK_MGR_CHECKPOINT_SIZE do not match"
#endif

// Statically reserve bank managers
static 
xmem_bank_mgr_t xmem_bank_mgrs[XMEM_MAX_BANK_MGRS] XMEM_BANK_PREFERRED_BANK;

/* Helper function to convert from xmem_status_t to xmem_bank_status_t */
static XMEM_INLINE xmem_bank_status_t xmem_bank_get_status(xmem_status_t s) {
  if (s == XMEM_OK)
    return XMEM_BANK_OK;
  else if (s == XMEM_ERR_INVALID_ARGS)
    return XMEM_BANK_ERR_INVALID_ARGS;
  else if (s == XMEM_ERR_ALLOC_FAILED)
    return XMEM_BANK_ERR_ALLOC_FAILED;
  else if (s == XMEM_ERR_ILLEGAL_ALIGN)
    return XMEM_BANK_ERR_ILLEGAL_ALIGN;
  else if (s == XMEM_ERR_PTR_OUT_OF_BOUNDS)
    return XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS;
  else
    return XMEM_BANK_ERR_INTERNAL;
}

/* Return an unused bank manager if available, else returns NULL */
static XMEM_INLINE xmem_bank_mgr_t *xmem_get_new_bank_mgr() {
#ifdef XMEM_XOS
  int i;
  for (i = 0; i < XMEM_MAX_BANK_MGRS; ++i) {
    if (!xmem_bank_mgrs[i]._banks_initialized) {
      XMEM_LOG("xmem_get_new_bank_mgr: Return free mgr %p at idx %d\n",
               &xmem_bank_mgrs[i], i);
      return &xmem_bank_mgrs[i];
    }
  }
  return NULL;
#else
  return &xmem_bank_mgrs[0];
#endif
}

/* Returns the current thread's bank manager. If the global manager is 
 * available return that, else return NULL.
 */
static XMEM_INLINE xmem_bank_mgr_t *xmem_get_bank_mgr() {
#ifdef XMEM_XOS
  int i;
  int global_idx = -1;
  for (i = 0; i < XMEM_MAX_BANK_MGRS; ++i) {
    if (!xmem_bank_mgrs[i]._banks_initialized) {
      continue;
    }
    // The global manager
    if (xmem_bank_mgrs[i]._thread == NULL) {
      global_idx = i;
      continue;
    }
    if (xmem_bank_mgrs[i]._thread == xos_thread_id()) {
      XMEM_LOG("xmem_get_bank_mgr: Return thread mgr %p for thread %p\n",
               &xmem_bank_mgrs[i], xos_thread_id());
      return &xmem_bank_mgrs[i];
    }
  }

  if (global_idx != -1) {
    XMEM_LOG("xmem_get_bank_mgr: Return global mgr %p at idx %d\n", 
             &xmem_bank_mgrs[global_idx], global_idx);
    return &xmem_bank_mgrs[global_idx];
  }

  return NULL;
#else
  return &xmem_bank_mgrs[0];
#endif
}

/* Set up the bank information and memory managers */
static xmem_bank_status_t 
xmem_setup_bank_managers(xmem_bank_mgr_t *xbm, 
                         xmem_bank_t *mem_banks, uint32_t num_banks, 
                         xmem_bank_alloc_policy_t alloc_policy, void *thread)
{
  unsigned int i;

  if (!xbm)
    return XMEM_BANK_ERR_UNINITIALIZED;

#ifdef XMEM_XOS
  // Setup mutex/cond only for global bank manager
  if (!thread) {
    int xr;
    xr = xos_cond_create(&xbm->_xos_cond);
    XMEM_CHECK(xr == XOS_OK, XMEM_BANK_ERR_UNINITIALIZED,
               "xmem_setup_bank_managers: Error initializing xos_cond\n");

    xr = xos_mutex_create(&xbm->_xos_mutex, XOS_MUTEX_WAIT_PRIORITY, 0);
    XMEM_CHECK(xr == XOS_OK, XMEM_BANK_ERR_UNINITIALIZED,
               "xmem_setup_bank_managers: Error initializing xos_mutex\n");
  }
#endif

  // Init the thread context
  xbm->_thread = thread;

  // Reserve space for bank and memory managers from the start of
  // lowest addressed bank
  xbm->_banks = (xmem_internal_bank_t *)mem_banks[0]._start;
  xbm->_num_banks = num_banks;
  xbm->_alloc_policy = alloc_policy;

  // Setup bank information for each memory bank
  for (i = 0; i < xbm->_num_banks; ++i) {
    xbm->_banks[i]._start = mem_banks[i]._start;
    xbm->_banks[i]._size = mem_banks[i]._size;
    xbm->_banks[i]._is_contiguous = 0;
  }

  // Check if banks are contigious and mark them so.
  for (i = 0; i < xbm->_num_banks-1; ++i) {
    XMEM_VERIFY_ASSERT((uintptr_t)xbm->_banks[i]._start <
                       (uintptr_t)xbm->_banks[i+1]._start,
                       "xmem_setup_bank_managers: Banks are expected to be "
                       "sorted in increasing order of start addresses\n");
    if ((uintptr_t)xbm->_banks[i]._start + xbm->_banks[i]._size == 
        (uintptr_t)xbm->_banks[i+1]._start) {
      xbm->_banks[i]._is_contiguous = 1;
    }
  }

#if XMEM_DEBUG
  XMEM_LOG("xmem_setup_bank_managers: Initial memory bank info:\n");
  for (i = 0; i < xbm->_num_banks; ++i) {
    XMEM_LOG("  Bank %d, start = %p, size = %u, is_contiguous: %s\n", i, 
             xbm->_banks[i]._start, xbm->_banks[i]._size,
             xbm->_banks[i]._is_contiguous == 1 ? "true" : "false");
  }
#endif

  XMEM_LOG("xmem_setup_bank_managers: Allocating %d bytes for memory bank info "
           "at %p\n", sizeof(xmem_internal_bank_t)*xbm->_num_banks,
           xbm->_banks);

  XMEM_CHECK(xbm->_banks[0]._size > 
             sizeof(xmem_internal_bank_t)*xbm->_num_banks,
              XMEM_BANK_ERR_INIT_BANKS_FAIL, 
              "xmem_setup_bank_managers: Expecting > %d bytes in bank-0\n", 
              sizeof(xmem_internal_bank_t)*xbm->_num_banks);

  // Adjust size and start of bank 0 to account for memory bank info
  xbm->_banks[0]._start = (void *)((uintptr_t)xbm->_banks[0]._start +
                                  sizeof(xmem_internal_bank_t)*xbm->_num_banks);
  xbm->_banks[0]._size -= sizeof(xmem_internal_bank_t)*xbm->_num_banks;

  if (alloc_policy == XMEM_BANK_STACK_ALLOC) {
    // Setup base of the stack memory managers for the banks.
    // It follows the local memory bank information
    xbm->_mgr._sm = (xmem_stack_mgr_t *)xbm->_banks[0]._start;

    XMEM_CHECK(xbm->_banks[0]._size > sizeof(xmem_stack_mgr_t)*xbm->_num_banks,
                XMEM_BANK_ERR_INIT_BANKS_FAIL, 
                "xmem_setup_bank_managers: Expecting > %d bytes in bank-0\n", 
                sizeof(xmem_internal_bank_t) * xbm->_num_banks);

    XMEM_LOG("xmem_setup_bank_managers: Allocating %d bytes for %d bank "
             "managers at %p\n", sizeof(xmem_stack_mgr_t)*xbm->_num_banks,
             xbm->_num_banks, xbm->_mgr._sm);

    // Adjust size and start of bank 0 to account for memory bank info
    xbm->_banks[0]._start = (void *)((uintptr_t)xbm->_banks[0]._start +
                                    sizeof(xmem_stack_mgr_t)*xbm->_num_banks);
    xbm->_banks[0]._size -= sizeof(xmem_stack_mgr_t)*xbm->_num_banks;

    // Align to load/store width
    uint32_t align = XCHAL_DATA_WIDTH;
    uint32_t p = xmem_find_msbit(align);
    uintptr_t start = (uintptr_t)xbm->_banks[0]._start;
    uintptr_t aligned_start = ((start + align - 1) >> p) << p;
    xbm->_banks[0]._start = (void *)aligned_start;
    xbm->_banks[0]._size -= (aligned_start - start);
    
    XMEM_LOG("xmem_setup_bank_managers: Reserving %d bytes in bank 0 for "
             "aligning to data width of %d bytes\n", aligned_start - start,
             XCHAL_DATA_WIDTH);
    
    // Initialize stack memory manager for bank 0
    XMEM_CHECK(xmem_stack_init(&xbm->_mgr._sm[0], 
                               xbm->_banks[0]._start, 
                               xbm->_banks[0]._size) == XMEM_OK, 
               XMEM_BANK_ERR_INIT_MGR, 
               "xmem_setup_bank_managers: Error initializing stack memory "
               "manager for bank 0\n");

    if (xbm->_num_banks > 1) {
      // Initialize stack memory manager for bank 1
      XMEM_CHECK(xmem_stack_init(&xbm->_mgr._sm[1],
                                 xbm->_banks[1]._start,
                                 xbm->_banks[1]._size) == XMEM_OK,
                 XMEM_BANK_ERR_INIT_MGR, 
                 "xmem_setup_bank_managers: Error initializing stack memory "
                 "manager for bank 1\n");
    }
  } else {
    // Setup base of the heap memory managers for the banks.
    // It follows the local memory bank information
    xbm->_mgr._hm = (xmem_heap_mgr_t *)xbm->_banks[0]._start;

    // Setup base of the heap memory manager headers for the banks.
    // It follows the local memory bank information
    char *xmem_bank_heap_mgr_header = (char *)
                                      ((uintptr_t)xbm->_mgr._hm + 
                                       sizeof(xmem_heap_mgr_t)*xbm->_num_banks);

    unsigned heap_mgr_header_size = 
        XMEM_HEAP_HEADER_SIZE(XMEM_BANK_HEAP_MGR_NUM_BLOCKS);

    XMEM_CHECK(xbm->_banks[0]._size > 
                 sizeof(xmem_heap_mgr_t)*xbm->_num_banks +
                 heap_mgr_header_size*xbm->_num_banks,
               XMEM_BANK_ERR_INIT_BANKS_FAIL,
               "xmem_setup_bank_managers: Expecting > %d bytes in bank-0\n", 
               sizeof(xmem_internal_bank_t)*xbm->_num_banks);

    XMEM_LOG("xmem_setup_bank_managers: Allocating %d bytes for %d bank "
             "managers at %p\n", sizeof(xmem_heap_mgr_t)*xbm->_num_banks,
             xbm->_num_banks, xbm->_mgr._hm);

    XMEM_LOG("xmem_setup_bank_managers: Allocating %d bytes for %d bank "
             "manager headers at %p\n", heap_mgr_header_size*xbm->_num_banks,
             xbm->_num_banks, xbm->_mgr._hm);

    // Adjust size and start of bank 0 to account for memory bank info
    xbm->_banks[0]._start = (void *)((uintptr_t)xbm->_banks[0]._start +
                                    sizeof(xmem_heap_mgr_t)*xbm->_num_banks +
                                    heap_mgr_header_size*xbm->_num_banks);
    xbm->_banks[0]._size -= sizeof(xmem_heap_mgr_t)*xbm->_num_banks +
                            heap_mgr_header_size*xbm->_num_banks;

    // Align to load/store width
    uint32_t align = XCHAL_DATA_WIDTH;
    uint32_t p = xmem_find_msbit(align);
    uintptr_t start = (uintptr_t)xbm->_banks[0]._start;
    uintptr_t aligned_start = ((start + align - 1) >> p) << p;
    xbm->_banks[0]._start = (void *)aligned_start;
    xbm->_banks[0]._size -= (aligned_start - start);
    
    XMEM_LOG("xmem_setup_bank_managers: Reserving %d bytes in bank 0 for "
             "aligning to data width of %d bytes\n", aligned_start - start,
             XCHAL_DATA_WIDTH);

    // Initialize heap memory manager for bank 0
    XMEM_CHECK(xmem_heap_init(&xbm->_mgr._hm[0],
                              xbm->_banks[0]._start, 
                              xbm->_banks[0]._size,
                              XMEM_BANK_HEAP_MGR_NUM_BLOCKS,
                              xmem_bank_heap_mgr_header) == XMEM_OK, 
               XMEM_BANK_ERR_INIT_MGR, 
               "xmem_setup_bank_managers: Error initializing heap memory "
               "manager for bank 0\n");

    if (xbm->_num_banks > 1) {
      // Initialize heap memory manager for bank 1
      XMEM_CHECK(xmem_heap_init(&xbm->_mgr._hm[1],
                                xbm->_banks[1]._start,
                                xbm->_banks[1]._size,
                                XMEM_BANK_HEAP_MGR_NUM_BLOCKS,
                                (char *)((uintptr_t)xmem_bank_heap_mgr_header +
                                         heap_mgr_header_size)) == XMEM_OK, 
                 XMEM_BANK_ERR_INIT_MGR, 
                 "xmem_setup_bank_managers: Error initializing heap memory "
                 "manager for bank 1\n");
    }
  }

#if XMEM_DEBUG
  XMEM_LOG("xmem_setup_bank_managers: Memory bank info after setup:\n");
  for (i = 0; i < xbm->_num_banks; ++i) {
    XMEM_LOG("  Bank %d, start = %p, size = %u, is_contiguous: %s\n", i, 
             xbm->_banks[i]._start, xbm->_banks[i]._size,
             xbm->_banks[i]._is_contiguous == 1 ? "true" : "false");
  }
#endif

  // Banks are now initialized
  xbm->_banks_initialized = 1;

  return XMEM_BANK_OK;
}

/* Initialize local memory banks and the stack/heap memory manager.
 * 
 * alloc_policy : Can be XMEM_BANK_STACK_ALLOC or XMEM_BANK_HEAP_ALLOC.
 * stack_size   : Stack size to reserve in highest addressed local memory bank
 *
 * Returns XMEM_BANK_OK if successful, sles returns one of 
 * XMEM_BANK_ERR_INIT_BANKS_FAIL, XMEM_BANK_ERR_INIT_MGR, 
 * XMEM_BANK_ERR_STACK_RESERVE_FAIL, XMEM_BANK_ERR_CONFIG_UNSUPPORTED.
 */
xmem_bank_status_t 
xmem_init_local_mem(xmem_bank_alloc_policy_t alloc_policy,
                    unsigned stack_size)
{
  xmem_bank_status_t status;

  xmem_bank_mgr_t *xbm = xmem_get_new_bank_mgr();

  // Initialize local memory banks
  unsigned num_banks = 2;
  xmem_bank_t local_mem_banks[2];
  status = xlmem_init_banks(local_mem_banks, &num_banks, stack_size);

  XMEM_CHECK(status == XMEM_BANK_OK || 
             status == XMEM_BANK_ERR_STACK_RESERVE_FAIL, 
             status,
             "xmem_init_local_mem: Error initializing local memory banks\n");
  XMEM_CHECK(num_banks <= 2, XMEM_BANK_ERR_INIT_BANKS_FAIL,
             "xmem_init_local_mem: No more than 2 banks supported\n");
  XMEM_CHECK(local_mem_banks[0]._size > sizeof(xmem_internal_bank_t) *
                                        num_banks,
              XMEM_BANK_ERR_INIT_BANKS_FAIL, 
              "Expecting > %d bytes in bank-0\n", sizeof(xmem_internal_bank_t) *
                                                  num_banks);

  xmem_bank_status_t xbs = xmem_setup_bank_managers(xbm,
                                                    local_mem_banks, 
                                                    num_banks, 
                                                    alloc_policy, 
                                                    NULL);

  // If setup went fine, return either XMEM_BANK_ERR_STACK_RESERVE_FAIL
  // or XMEM_BANK_OK (from xlmem_init_banks)
  if (xbs == XMEM_BANK_OK)
    return status;

  return xbs;
}

/* Initialize memory banks and the stack/heap memory manager from user
 * provided memory banks
 * 
 * alloc_policy : Can be XMEM_BANK_STACK_ALLOC or XMEM_BANK_HEAP_ALLOC.
 * memory_banks : User supplied array of banks
 * num_banks    : Number of banks
 *
 * Returns XMEM_BANK_OK if successful, sles returns one of 
 * XMEM_BANK_ERR_INVALID_ARGS, XMEM_BANK_ERR_INIT_BANKS_FAIL,
 * XMEM_BANK_ERR_INIT_MGR
 */
xmem_bank_status_t 
xmem_init_banks(xmem_bank_alloc_policy_t alloc_policy,
                xmem_bank_t *memory_banks, unsigned num_banks)
{
  XMEM_CHECK(num_banks, XMEM_BANK_ERR_INVALID_ARGS, 
             "xmem_init_banks: num_banks is 0\n");

  XMEM_CHECK(num_banks <= 2, XMEM_BANK_ERR_INVALID_ARGS, 
             "xmem_init_banks: No more than 2 banks supported\n");

  xmem_bank_mgr_t *xbm = xmem_get_new_bank_mgr();

  return xmem_setup_bank_managers(xbm, memory_banks, num_banks, 
                                  alloc_policy, NULL);
}

#ifdef XMEM_XOS
/* Initialize memory banks and the stack/heap memory manager from user
 * provided memory banks for the current thread context.
 * 
 * alloc_policy : Can be XMEM_BANK_STACK_ALLOC or XMEM_BANK_HEAP_ALLOC.
 * memory_banks : User supplied array of banks
 * num_banks    : Number of banks
 * num_banks    : Number of banks
 *
 * Returns XMEM_BANK_OK if successful, sles returns one of 
 * XMEM_BANK_ERR_INVALID_ARGS, XMEM_BANK_ERR_INIT_BANKS_FAIL,
 * XMEM_BANK_ERR_INIT_MGR
 */
xmem_bank_status_t 
xmem_thread_init_banks(xmem_bank_alloc_policy_t alloc_policy,
                       xmem_bank_t *memory_banks, unsigned num_banks)
{
  XMEM_CHECK(num_banks, XMEM_BANK_ERR_INVALID_ARGS, 
             "xmem_thread_init_banks: num_banks is 0\n");

  XMEM_CHECK(num_banks <= 2, XMEM_BANK_ERR_INVALID_ARGS, 
             "xmem_thread_init_banks: No more than 2 banks supported\n");

  xmem_bank_mgr_t *xbm = xmem_get_new_bank_mgr();

  return xmem_setup_bank_managers(xbm, memory_banks, num_banks, 
                                  alloc_policy, xos_thread_id());
}

/* Reset calling thread's bank manager. Allows it to be reused by
 * another thread 
 */
void xmem_thread_reset_banks()
{
  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();
  if (!xbm)
    return;
  if (xbm->_thread != xos_thread_id())
    return;
  xbm->_banks_initialized = 0;
}
#endif

/* Helper function to allocate from the heap memory manager */
static XMEM_INLINE void *
xmem_bank_heap_do_alloc(xmem_heap_mgr_t *mgr, size_t size, 
                    uint32_t align, xmem_bank_status_t *status)
{
  xmem_status_t xs;
  void *p = xmem_heap_alloc(mgr, size, align, &xs);
  xmem_bank_status_t xbs = xmem_bank_get_status(xs);
  if (status)
    *status = xbs;
  return p;
}

/* Allocate from the bank using the heap memory manager */
static XMEM_INLINE void *
xmem_bank_alloc_using_heap(xmem_bank_mgr_t *xbm, int bank, size_t size, 
                           uint32_t align, xmem_bank_status_t *status) {
  xmem_bank_status_t xbs = XMEM_BANK_ERR_ALLOC_FAILED;
  void *p = NULL;

  if (bank != -1) {
    // Allocating from specific bank
    XMEM_LOG("xmem_bank_alloc_using_heap: Allocating %d bytes in bank %d\n",
             size, bank);
    p = xmem_bank_heap_do_alloc(&xbm->_mgr._hm[bank], size, 
                                align, &xbs);
  } else {
    xmem_heap_mgr_t *mgr_b0 = &xbm->_mgr._hm[0];
    // Check if we can allocate from bank-0
    if ((p = xmem_bank_heap_do_alloc(mgr_b0, size, align, &xbs))) {
      XMEM_LOG("xmem_bank_alloc_using_heap: Allocating %d bytes in first "
               "available bank 0\n", size);
    } else if (xbm->_num_banks > 1) {
      /* Find free space at the end */
      size_t rem_b0 = xmem_heap_get_free_space(mgr_b0, align, 
                                               XMEM_HEAP_FREE_SPACE_END,
                                               NULL, NULL, NULL);
      XMEM_LOG("xmem_bank_alloc_using_heap: Remaining free space in "
               "bank 0 is %d bytes\n", rem_b0);
      xmem_heap_mgr_t *mgr_b1 = &xbm->_mgr._hm[1];
      // rem_b0 is what can be allocated from bank-0
      size_t to_be_allocated_b1 = size - rem_b0;
      size_t rem_b1 = xmem_heap_get_free_space(mgr_b1, 1, 
                                               XMEM_HEAP_FREE_SPACE_START, 
                                               NULL, NULL, NULL);
      XMEM_LOG("xmem_bank_alloc_using_heap: Remaining free space in "
               "bank 1 is %d bytes, to be allocated in bank 1 is %d bytes\n", 
               rem_b1, to_be_allocated_b1);
      if (rem_b0 && xbm->_banks[0]._is_contiguous &&
          to_be_allocated_b1 <= rem_b1) {
        // Allocating whatever is left of bank-0
        XMEM_LOG("xmem_bank_alloc_using_heap: Allocating %d bytes in "
                 "bank 0\n", rem_b0);
        p = xmem_bank_heap_do_alloc(mgr_b0, rem_b0, align, &xbs);
        XMEM_VERIFY_ASSERT(xbs == XMEM_BANK_OK, 
                           "xmem_bank_alloc_using_heap: Allocation of "
                           "in bank-0 not expected to fail!\n");

        XMEM_ASSERT(xbm->_straddle_alloc == NULL, "Expecting straddle pointer "
                    "to be null, but got %p\n", xbm->_straddle_alloc);
        // Mark straddle pointer
        xbm->_straddle_alloc = p;
  
        XMEM_LOG("xmem_bank_alloc_using_heap: Pointer %p is a "
                 "straddle alloc\n", p);

        // And remaining from bank-1
        XMEM_LOG("xmem_bank_alloc_using_heap: Allocating %d bytes in bank 1\n", 
                 to_be_allocated_b1);
        void *q = xmem_bank_heap_do_alloc(mgr_b1, to_be_allocated_b1, 1, &xbs);
        (void)q;
        XMEM_VERIFY_ASSERT(xbs == XMEM_BANK_OK, 
                           "xmem_bank_alloc_using_heap: Allocation of "
                           "in bank-1 not expected to fail!\n");
        XMEM_VERIFY_ASSERT(q == xbm->_banks[1]._start,
                           "xmem_bank_alloc_using_heap: Expecting allocation "
                           "at %p, but got %p\n", xbm->_banks[1]._start, q);
      } else if ((p = xmem_bank_heap_do_alloc(mgr_b1, size, align, &xbs))) {
         // Check if we can allocate from bank-1
        XMEM_LOG("xmem_bank_alloc_using_heap: Allocating %d bytes in bank 1\n",
                 size);
      }
    }
  }

  *status = xbs;
  return p;
}

/* Helper function to allocate from the stack memory manager */
static XMEM_INLINE void *
xmem_bank_stack_do_alloc(xmem_stack_mgr_t *mgr, size_t size, 
                         uint32_t align, xmem_bank_status_t *status)
{
  xmem_status_t xs;
  void *p = xmem_stack_alloc(mgr, size, align, &xs);
  xmem_bank_status_t xbs = xmem_bank_get_status(xs);
  if (status)
    *status = xbs;
  return p;
}

/* Allocate from the bank using the stack memory manager */
static XMEM_INLINE void *
xmem_bank_alloc_using_stack(xmem_bank_mgr_t *xbm, int bank, size_t size, 
                            uint32_t align, xmem_bank_status_t *status) {
  xmem_bank_status_t xbs = XMEM_BANK_ERR_ALLOC_FAILED;
  void *p = NULL;

  if (bank != -1) {
    // Allocating from specific bank
    XMEM_LOG("xmem_bank_alloc_using_stack: Allocating %d bytes in bank %d\n", 
             size, bank);
    p = xmem_bank_stack_do_alloc(&xbm->_mgr._sm[bank], size, 
                                 align, &xbs);
  } else {
    // Try from bank-0
    xmem_stack_mgr_t *mgr_b0 = &xbm->_mgr._sm[0];
    size_t rem_b0 = xmem_stack_get_free_space(mgr_b0, align, NULL, NULL, NULL);
    XMEM_LOG("xmem_bank_alloc_using_stack: Remaining free space in "
             "bank 0 is %d bytes\n", rem_b0);
    // If remaining size on bank-0 (after accounting for alignment) is >= size
    // allocate from bank-0
    if (rem_b0 >= size) {
      XMEM_LOG("xmem_bank_alloc_using_stack: Allocating %d bytes in bank 0\n", 
               size);
      p = xmem_bank_stack_do_alloc(mgr_b0, size, align, &xbs);
    } else if (xbm->_num_banks > 1) {
      xmem_stack_mgr_t *mgr_b1 = &xbm->_mgr._sm[1];
      // rem_b0 is what can be allocated from bank-0
      size_t to_be_allocated_b1 = size - rem_b0;
      size_t rem_b1 = xmem_stack_get_free_space(mgr_b1, 1, NULL, NULL, NULL);
      XMEM_LOG("xmem_bank_alloc_using_stack: Remaining free space in "
               "bank 1 is %d bytes, to be allocated in bank 1 is %d bytes\n", 
               rem_b1, to_be_allocated_b1);
      // Check if the banks are contiguous, if bank-1 is completely free
      // and has enough size
      if (rem_b0 && 
          xbm->_banks[0]._is_contiguous &&
          rem_b1 == xbm->_banks[1]._size &&
          to_be_allocated_b1 <= rem_b1) {
        // Allocating whatever is left of bank-0
        XMEM_LOG("xmem_bank_alloc_using_stack: Allocating %d bytes in bank 0\n",
                 rem_b0);
        p = xmem_bank_stack_do_alloc(mgr_b0, rem_b0, align, NULL);
        // And remaining from bank-1
        XMEM_LOG("xmem_bank_alloc_using_stack: Allocating %d bytes in bank 1\n",
                 to_be_allocated_b1);
        xmem_bank_stack_do_alloc(mgr_b1, to_be_allocated_b1, 1, &xbs);
      } else if (size <= rem_b1) {
        XMEM_LOG("xmem_bank_alloc_using_stack: Allocating %d bytes in bank 1\n",
                 size);
        p = xmem_bank_stack_do_alloc(mgr_b1, size, align, &xbs);
      }
    }
  }

  *status = xbs;
  return p;
}

/* Allocate memory from a bank of given size and alignment. 
 *
 * xbm    : Bank manager
 * bank   : Local memory bank to allocate memory from
 * size   : Size to allocate. Has to be non-zero.
 * align  : Requested alignment
 * status : Optional. If successful set to XMEM_OK, else set to one of
 *          XMEM_BANK_ERR_INVALID_ARGS,
 *          XMEM_BANK_ERR_ILLEGAL_ALIGN, XMEM_BANK_ERR_ALLOC_FAILED,
 *          XMEM_BANK_ERR_UNINITIALIZED
 */
static XMEM_INLINE void *
xmem_bank_alloc_internal(xmem_bank_mgr_t *xbm, int bank, size_t size, 
                         uint32_t align, xmem_bank_status_t *status)
{
  void *p = NULL;
  xmem_bank_status_t xbs;

  XMEM_CHECK_STATUS(xbm, xbs, XMEM_BANK_ERR_UNINITIALIZED, fail, 
                    "xmem_bank_alloc_internal: bank manager not initialized\n");

  XMEM_CHECK_STATUS(xbm->_banks_initialized, xbs, XMEM_BANK_ERR_UNINITIALIZED,
                    fail, "xmem_bank_alloc_internal: banks not initialized\n");

  XMEM_CHECK_STATUS(bank >= -1 && bank < (int)xbm->_num_banks,
                    xbs, XMEM_BANK_ERR_INVALID_ARGS, fail,
                    "xmem_bank_alloc_internal: bank has to be either "
                    "-1 or 0..%d\n", xbm->_num_banks-1);

  /* Expect alignment to be a power-of-2 */
  XMEM_CHECK_STATUS(align && !(align & (align-1)),
                    xbs, XMEM_BANK_ERR_ILLEGAL_ALIGN, fail,
                    "xmem_bank_alloc_internal: "
                    "alignment should be a power of 2\n");

  /* Size has to be > 0 */
  XMEM_CHECK_STATUS(size > 0, xbs, XMEM_BANK_ERR_INVALID_ARGS, fail,
                    "xmem_bank_alloc_internal: size has to be > 0\n");

  xmem_disable_preemption();

  if (xbm->_alloc_policy == XMEM_BANK_STACK_ALLOC)
    p = xmem_bank_alloc_using_stack(xbm, bank, size, align, &xbs);
  else
    p = xmem_bank_alloc_using_heap(xbm, bank, size, align, &xbs);

  xmem_enable_preemption();

fail:
  if (status)
    *status = xbs;

  XMEM_LOG("xmem_bank_alloc_internal: "
           "Allocating %d bytes, align %d, bank %d at %p\n", 
           size, align, bank, p);

  return p;
}

/* Allocate memory from a bank of given size and alignment. 
 *
 * bank   : Local memory bank to allocate memory from
 * size   : Size to allocate. Has to be non-zero.
 * align  : Requested alignment
 * status : Optional. If successful set to XMEM_OK, else set to one of
 *          XMEM_BANK_ERR_INVALID_ARGS,
 *          XMEM_BANK_ERR_ILLEGAL_ALIGN, XMEM_BANK_ERR_ALLOC_FAILED,
 *          XMEM_BANK_ERR_UNINITIALIZED
 */
void *xmem_bank_alloc(int bank, size_t size, uint32_t align, 
                      xmem_bank_status_t *status)
{
  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();
  return xmem_bank_alloc_internal(xbm, bank, size, align, status);
}

#ifdef XMEM_XOS
/* Allocate memory from a bank of given size and alignment. Blocks until
 * memory is available or timeout expires
 *
 * bank    : Local memory bank to allocate memory from
 * size    : Size to allocate. Has to be non-zero.
 * align   : Requested alignment
 * timeout : Wait for timeout cycles
 * status  : Optional. If successful set to XMEM_OK, else set to one of
 *           XMEM_BANK_ERR_INVALID_ARGS,
 *           XMEM_BANK_ERR_ILLEGAL_ALIGN, XMEM_BANK_ERR_ALLOC_FAILED,
 *           XMEM_BANK_ERR_UNINITIALIZED
 */
void *xmem_bank_alloc_wait(int bank, size_t size, uint32_t align, 
                           uint64_t timeout, xmem_bank_status_t *status)
{
  xmem_bank_status_t bs;
  int32_t ret;

  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();

  void *p = xmem_bank_alloc_internal(xbm, bank, size, align, &bs);
  if (bs != XMEM_BANK_ERR_ALLOC_FAILED) {
    if (status)
      *status = bs;
    return p;
  }

  ret = xos_mutex_lock(&xbm->_xos_mutex);
  XMEM_CHECK_STATUS(ret == XOS_OK, bs, XMEM_BANK_ERR_ALLOC_FAILED, fail,
                    "xmem_bank_alloc_wait: Failed to lock mutex\n");

  p = xmem_bank_alloc_internal(xbm, bank, size, align, &bs);

  ret = XMEM_BANK_SIG_VALUE;
  while (bs == XMEM_BANK_ERR_ALLOC_FAILED && (ret == XMEM_BANK_SIG_VALUE)) {
    XMEM_LOG("xmem_bank_alloc_wait: @%u: Blocking for alloc of %d bytes "
             "in bank %d\n", xmem_get_clock(), size, bank);
    // Wait until timeout
    ret = xos_cond_wait_mutex_timeout(&xbm->_xos_cond, &xbm->_xos_mutex, 
                                      timeout);
    XMEM_LOG("xmem_bank_alloc_wait: @%d: Cond ret: %d Retrying alloc of %d "
             "bytes in bank %d\n", xmem_get_clock(), ret, size, bank);
    p = xmem_bank_alloc_internal(xbm, bank, size, align, &bs);
  }
  xos_mutex_unlock(&xbm->_xos_mutex);

fail:
  if (status)
    *status = bs;

  return p;
}
#endif
  

/* Return the amount of free space in bank after taking into account the
 * alignment. 
 * 
 * bank             Bank number to query
 * align            The requested alignment
 * start_free_space If non-null, returns the location of the start of 
 *                  free space (after alignment is applied)
 * end_free_space   If non-null, returns end of allocatable space
 *
 * Return the free space and sets status to XMEM_BANK_OK if successful, else
 * returns one of XMEM_BANK_ERR_INVALID_ARGS, XMEM_BANK_ERR_UNINITIALIZED
 * or XMEM_BANK_ERR_ILLEGAL_ALIGN.
 */
size_t
xmem_bank_get_free_space(unsigned bank, uint32_t align, 
                         void **start_free_space, void **end_free_space,
                         xmem_bank_status_t *status)
{
  size_t r = 0;
  xmem_bank_status_t xbs;

  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();

  XMEM_CHECK_STATUS(xbm, xbs, XMEM_BANK_ERR_UNINITIALIZED, fail, 
                    "xmem_bank_get_free_space: bank manager not initialized\n");

  XMEM_CHECK_STATUS(xbm->_banks_initialized, xbs, XMEM_BANK_ERR_UNINITIALIZED,
                    fail, "xmem_bank_get_free: banks not initialized\n");

  XMEM_CHECK_STATUS(bank >= 0 && bank < xbm->_num_banks,
                    xbs, XMEM_BANK_ERR_INVALID_ARGS, fail,
                    "xmem_bank_get_free_space: bank has to be "
                    "between 0..%d\n", xbm->_num_banks-1);

  xmem_status_t xs;
  if (xbm->_alloc_policy == XMEM_BANK_STACK_ALLOC)
    r = xmem_stack_get_free_space(&xbm->_mgr._sm[bank], align, 
                                  start_free_space, end_free_space, &xs);
  else
    r = xmem_heap_get_free_space(&xbm->_mgr._hm[bank], align, 
                                 XMEM_HEAP_MAX_FREE_SPACE,
                                 start_free_space, end_free_space, &xs);

  xbs = xmem_bank_get_status(xs);
  
fail:
  if (status)
    *status = xbs;

  XMEM_LOG("xmem_bank_get_free_space: Start %p, end %p, size %d bytes\n",
           start_free_space ? *start_free_space : 0,
           end_free_space   ? *end_free_space : 0,
           r);

  return r;
}

/* Saves the bank allocation state 
 *
 * bank : Local memory bank number
 * s    : Object where state is saved to
 * 
 * Returns XMEM_BANK_OK if successful, else returns 
 * one of XMEM_BANK_ERR_UNSUPPORTED_ALLOC, XMEM_BANK_ERR_INVALID_ARGS,
 * or XMEM_BANK_ERR_UNINITIALIZED
 */
xmem_bank_status_t 
xmem_bank_checkpoint_save(unsigned bank, xmem_bank_checkpoint_t *s)
{
  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();

  XMEM_CHECK(xbm, XMEM_BANK_ERR_UNINITIALIZED, 
             "xmem_bank_checkpoint_save: bank manager not initialized\n");

  XMEM_CHECK(xbm->_banks_initialized, XMEM_BANK_ERR_UNINITIALIZED,
             "xmem_bank_checkpoint_save: banks not initialized\n");

  XMEM_CHECK(xbm->_alloc_policy == XMEM_BANK_STACK_ALLOC,
             XMEM_BANK_ERR_UNSUPPORTED_ALLOC,
             "xmem_bank_checkpoint_save: Checkpointing supported only for "
             "stack based memory allocator\n");

  XMEM_CHECK(bank >= 0 && bank < xbm->_num_banks,
             XMEM_BANK_ERR_INVALID_ARGS,
             "xmem_bank_checkpoint_save: bank has to be between 0..%d\n", 
             xbm->_num_banks-1);

  XMEM_CHECK(s != NULL, XMEM_BANK_ERR_INVALID_ARGS,
             "xmem_bank_checkpoint_save: s is null\n");

  xmem_status_t err = 
                xmem_stack_checkpoint_save(&xbm->_mgr._sm[bank],
                                           (xmem_stack_mgr_checkpoint_t *)s);
  return xmem_bank_get_status(err);
}

/* Restore the bank allocation state 
 *
 * bank : Local memory bank number
 * s    : Object where state is restored from
 * 
 * Returns XMEM_BANK_OK if successful, else returns 
 * one of XMEM_BANK_ERR_UNSUPPORTED_ALLOC, XMEM_ERR_INVALID_ARGS, or
 * XMEM_BANK_ERR_UNINITIALIZED
 */
xmem_bank_status_t 
xmem_bank_checkpoint_restore(unsigned bank, xmem_bank_checkpoint_t *s)
{
  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();

  XMEM_CHECK(xbm, XMEM_BANK_ERR_UNINITIALIZED, 
             "xmem_bank_checkpoint_restore: bank manager not initialized\n");

  XMEM_CHECK(xbm->_banks_initialized, XMEM_BANK_ERR_UNINITIALIZED,
             "xmem_bank_checkpoint_restore: banks not initialized\n");

  XMEM_CHECK(xbm->_alloc_policy == XMEM_BANK_STACK_ALLOC,
             XMEM_BANK_ERR_UNSUPPORTED_ALLOC,
             "xmem_bank_checkpoint_restore: Checkpointing supported only for "
             "stack based memory allocator\n");

  XMEM_CHECK(bank >= 0 && bank < xbm->_num_banks,
             XMEM_BANK_ERR_INVALID_ARGS,
             "xmem_bank_checkpoint_restore: bank has to be between 0..%d\n", 
             xbm->_num_banks-1);

  XMEM_CHECK(s != NULL, XMEM_BANK_ERR_INVALID_ARGS,
             "xmem_bank_checkpoint_restore: s is null\n");

  xmem_status_t err = 
                xmem_stack_checkpoint_restore(&xbm->_mgr._sm[bank],
                                              (xmem_stack_mgr_checkpoint_t *)s);
  return xmem_bank_get_status(err);
}

/* Reset the bank allocation state 
 *
 * bank: Local memory bank number
 * 
 * Returns XMEM_BANK_OK if successful, else returns XMEM_BANK_ERR_INVALID_ARGS,
 * or XMEM_BANK_ERR_UNINITIALIZED
 */
xmem_bank_status_t 
xmem_bank_reset(unsigned bank)
{
  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();

  XMEM_CHECK(xbm, XMEM_BANK_ERR_UNINITIALIZED, 
             "xmem_bank_reset: bank manager not initialized\n");

  XMEM_CHECK(xbm->_banks_initialized, XMEM_BANK_ERR_UNINITIALIZED,
             "xmem_bank_reset: banks not initialized\n");
  
  XMEM_CHECK(bank >= 0 && bank < xbm->_num_banks,
             XMEM_BANK_ERR_INVALID_ARGS,
             "xmem_bank_reset: bank has to be between 0..%d\n", 
             xbm->_num_banks-1);

  xmem_status_t err;

  if (xbm->_alloc_policy == XMEM_BANK_STACK_ALLOC) 
    err = xmem_stack_reset(&xbm->_mgr._sm[bank]);
  else
    err = xmem_heap_reset(&xbm->_mgr._hm[bank]);

  return xmem_bank_get_status(err);
}

/* Check if pointer is in bounds of the manager
 *
 * bank : Local memory bank number
 * p    : Pointer to check for bounds
 *
 * Returns XMEM_BANK_OK if in bounds, else returns one of 
 * XMEM_BANK_ERR_INVALID_ARGS, XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS, or
 * XMEM_BANK_ERR_UNINITIALIZED.
 */
xmem_bank_status_t 
xmem_bank_check_bounds(unsigned bank, void *p)
{
  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();

  XMEM_CHECK(xbm, XMEM_BANK_ERR_UNINITIALIZED, 
             "xmem_bank_check_bounds: bank manager not initialized\n");

  XMEM_CHECK(xbm->_banks_initialized, XMEM_BANK_ERR_UNINITIALIZED,
             "xmem_bank_check_bounds: banks not initialized\n");

  XMEM_CHECK(bank >= 0 && bank < xbm->_num_banks,
             XMEM_BANK_ERR_INVALID_ARGS,
             "xmem_bank_check_bounds: bank has to be between 0..%d\n", 
             xbm->_num_banks-1);

  xmem_status_t err;

  if (xbm->_alloc_policy == XMEM_BANK_STACK_ALLOC)
    err = xmem_stack_check_bounds(&xbm->_mgr._sm[bank], p);
  else
    err = xmem_heap_check_bounds(&xbm->_mgr._hm[bank], p);

  return xmem_bank_get_status(err);
}

/* Free and optionally clear memory allocated via the heap manager
 *
 * bank: bank to free memory from
 * p   : pointer to memory to be freed
 *
 * Returns XMEM_BANK_OK if successful, else returns 
 * XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS, XMEM_BANK_ERR_UNSUPPORTED_ALLOC, or
 * XMEM_BANK_ERR_INVALID_ARGS, XMEM_BANK_ERR_UNINITIALIZED.
 */
static XMEM_INLINE xmem_bank_status_t
xmem_bank_free_internal(xmem_bank_mgr_t *xbm, int bank, void *p, int clear)
{
  XMEM_CHECK(xbm, XMEM_BANK_ERR_UNINITIALIZED,
             "xmem_bank_free_internal: bank manager not initialized\n");

  XMEM_CHECK(xbm->_banks_initialized, XMEM_BANK_ERR_UNINITIALIZED,
             "xmem_bank_free_internal: banks not initialized\n");

  XMEM_CHECK(bank >= -1 && bank < (int)xbm->_num_banks,
             XMEM_BANK_ERR_INVALID_ARGS,
             "xmem_bank_free_internal: bank has to be between 0..%d\n", 
             xbm->_num_banks-1);

  XMEM_CHECK(xbm->_alloc_policy == XMEM_BANK_HEAP_ALLOC,
             XMEM_BANK_ERR_UNSUPPORTED_ALLOC,
             "xmem_bank_free_internal: Freeing supported only for heap based "
             "memory allocator\n");

  xmem_disable_preemption();

  xmem_status_t err;

  if (bank != -1) {
    if (clear)
      err = xmem_heap_free_with_clear(&xbm->_mgr._hm[bank], p);
    else
      err = xmem_heap_free(&xbm->_mgr._hm[bank], p);
  } else if (xbm->_num_banks == 1) {
    if (clear)
      err = xmem_heap_free_with_clear(&xbm->_mgr._hm[0], p);
    else
      err = xmem_heap_free(&xbm->_mgr._hm[0], p);
  } else {
    if (xbm->_straddle_alloc == p) {
      XMEM_LOG("xmem_bank_free_internal: "
               "Requesting free of straddle alloc %p\n", p);
      // If straddling, free from bank-0
      if (clear)
        err = xmem_heap_free_with_clear(&xbm->_mgr._hm[0], p);
      else
        err = xmem_heap_free(&xbm->_mgr._hm[0], p);
      XMEM_VERIFY_ASSERT(err != XMEM_ERR_PTR_OUT_OF_BOUNDS,
                         "xmem_bank_free_internal: "
                         "%p is out of bounds of bank 0\n", p);
      // And free start of bank-1
      if (clear)
        err = xmem_heap_free_with_clear(&xbm->_mgr._hm[1], 
                                        xbm->_banks[1]._start);
      else
        err = xmem_heap_free(&xbm->_mgr._hm[1], xbm->_banks[1]._start);
      XMEM_VERIFY_ASSERT(err != XMEM_ERR_PTR_OUT_OF_BOUNDS,
                         "xmem_bank_free_internal: "
                         "%p is out of bounds of bank 1\n", 
                         xbm->_banks[1]._start);
      // Reset straddle
      xbm->_straddle_alloc = NULL;
    } else {
      // Try both the banks
      if (clear)
        err = xmem_heap_free_with_clear(&xbm->_mgr._hm[0], p);
      else
        err = xmem_heap_free(&xbm->_mgr._hm[0], p);
      if (err == XMEM_ERR_PTR_OUT_OF_BOUNDS) {
        if (clear)
          err = xmem_heap_free_with_clear(&xbm->_mgr._hm[1], p);
        else
          err = xmem_heap_free(&xbm->_mgr._hm[1], p);
      }
      XMEM_VERIFY_ASSERT(err != XMEM_ERR_PTR_OUT_OF_BOUNDS,
                         "xmem_bank_free_internal: %p is out of bounds of bank "
                         "0 and 1\n", p);
    }
  }

  xmem_enable_preemption();

  XMEM_LOG("xmem_bank_free_with_clear: Freeing %p from bank %d\n", p, bank);

  return xmem_bank_get_status(err);
}

/* Free memory allocated via the heap manager
 *
 * bank: bank to free memory from
 * p   : pointer to memory to be freed
 *
 * Returns XMEM_BANK_OK if successful, else returns 
 * XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS, XMEM_BANK_ERR_UNSUPPORTED_ALLOC, or
 * XMEM_BANK_ERR_INVALID_ARGS, XMEM_BANK_ERR_UNINITIALIZED.
 */
xmem_bank_status_t
xmem_bank_free(int bank, void *p)
{
  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();
  return xmem_bank_free_internal(xbm, bank, p, 0);
}

#ifdef XMEM_XOS
/* Free memory allocated via the heap manager. Wakes up any threads blocked
 * on memory to be available.
 *
 * bank: bank to free memory from
 * p   : pointer to memory to be freed
 *
 * Returns XMEM_BANK_OK if successful, else returns 
 * XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS, XMEM_BANK_ERR_UNSUPPORTED_ALLOC, or
 * XMEM_BANK_ERR_INVALID_ARGS, XMEM_BANK_ERR_UNINITIALIZED.
 */
xmem_bank_status_t
xmem_bank_free_signal(int bank, void *p)
{
  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();

  xmem_bank_status_t bs = xmem_bank_free_internal(xbm, bank, p, 0);
  if (bs != XMEM_BANK_OK)
    return bs;

  xos_mutex_lock(&xbm->_xos_mutex);
  XMEM_LOG("xmem_bank_free_signal: @%u: Signaling bank free to "
           "waiting threads\n", xmem_get_clock());
  xos_cond_signal_one(&xbm->_xos_cond, XMEM_BANK_SIG_VALUE);
  xos_mutex_unlock(&xbm->_xos_mutex);

  return XMEM_BANK_OK;
}

/* Free memory and clear memory allocated via the heap manager. 
 * Wakes up any threads blocked on memory to be available.
 *
 * bank: bank to free memory from
 * p   : pointer to memory to be freed
 *
 * Returns XMEM_BANK_OK if successful, else returns 
 * XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS, XMEM_BANK_ERR_UNSUPPORTED_ALLOC, or
 * XMEM_BANK_ERR_INVALID_ARGS, XMEM_BANK_ERR_UNINITIALIZED.
 */
xmem_bank_status_t
xmem_bank_free_with_clear_signal(int bank, void *p)
{
  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();

  xmem_bank_status_t bs = xmem_bank_free_internal(xbm, bank, p, 1);
  if (bs != XMEM_BANK_OK)
    return bs;

  xos_mutex_lock(&xbm->_xos_mutex);
  XMEM_LOG("xmem_bank_free_signal: @%u: Signaling bank free to "
           "waiting threads\n", xmem_get_clock());
  xos_cond_signal_one(&xbm->_xos_cond, XMEM_BANK_SIG_VALUE);
  xos_mutex_unlock(&xbm->_xos_mutex);

  return XMEM_BANK_OK;
}
#endif

/* Free and clear memory allocated via the heap manager
 *
 * bank: bank to free memory from
 * p   : pointer to memory to be freed
 *
 * Returns XMEM_BANK_OK if successful, else returns 
 * XMEM_BANK_ERR_PTR_OUT_OF_BOUNDS, XMEM_BANK_ERR_UNSUPPORTED_ALLOC, or
 * XMEM_BANK_ERR_INVALID_ARGS, XMEM_BANK_ERR_UNINITIALIZED.
 */
xmem_bank_status_t
xmem_bank_free_with_clear(int bank, void *p)
{
  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();
  return xmem_bank_free_internal(xbm, bank, p, 1);
}

/* Returns the number of memory banks */
uint32_t 
xmem_bank_get_num_banks()
{
  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();
  if (!xbm)
    return 0;
  if (!xbm->_banks_initialized)
    return 0;
  return xbm->_num_banks;
}

/* Returns the bank allocation policy */
xmem_bank_alloc_policy_t 
xmem_bank_get_alloc_policy()
{
  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();
  if (!xbm)
    return (xmem_bank_alloc_policy_t)0;
  if (!xbm->_banks_initialized)
    return (xmem_bank_alloc_policy_t)0;
  return xbm->_alloc_policy;
}

/* Returns the free bytes in the bank */
ssize_t
xmem_bank_get_free_bytes(unsigned bank)
{
  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();

  XMEM_CHECK(xbm, -1,
             "xmem_bank_get_free_bytes: bank manager not initialized\n");

  XMEM_CHECK(xbm->_banks_initialized, -1,
             "xmem_bank_get_free_bytes: banks not initialized\n");

  XMEM_CHECK(bank >= 0 && bank < xbm->_num_banks, -1,
             "xmem_bank_get_free_bytes: bank has to be between 0..%d\n", 
             xbm->_num_banks-1);

  if (xbm->_alloc_policy == XMEM_BANK_STACK_ALLOC)
    return xmem_stack_get_free_bytes(&xbm->_mgr._sm[bank]);
  else
    return xmem_heap_get_free_bytes(&xbm->_mgr._hm[bank]);
}

/* Returns the allocated bytes in the bank */
ssize_t
xmem_bank_get_allocated_bytes(unsigned bank)
{
  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();

  XMEM_CHECK(xbm, -1,
             "xmem_bank_get_allocated_bytes: bank manager not initialized\n");

  XMEM_CHECK(xbm->_banks_initialized, -1,
             "xmem_bank_get_allocated_bytes: banks not initialized\n");

  XMEM_CHECK(bank >= 0 && bank < xbm->_num_banks, -1,
             "xmem_bank_get_allocated_bytes: bank has to be between 0..%d\n", 
             xbm->_num_banks-1);

  if (xbm->_alloc_policy == XMEM_BANK_STACK_ALLOC)
    return xmem_stack_get_allocated_bytes(&xbm->_mgr._sm[bank]);
  else
    return xmem_heap_get_allocated_bytes(&xbm->_mgr._hm[bank]);
}

/* Returns the unused bytes in the bank */
ssize_t
xmem_bank_get_unused_bytes(unsigned bank)
{
  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();

  XMEM_CHECK(xbm, -1,
             "xmem_bank_get_unused_bytes: bank manager not initialized\n");

  XMEM_CHECK(xbm->_banks_initialized, -1,
             "xmem_bank_get_unused_bytes: banks not initialized\n");

  XMEM_CHECK(bank >= 0 && bank < xbm->_num_banks, -1,
             "xmem_bank_get_unused_bytes: bank has to be between 0..%d\n", 
             xbm->_num_banks-1);

  if (xbm->_alloc_policy == XMEM_BANK_STACK_ALLOC)
    return xmem_stack_get_unused_bytes(&xbm->_mgr._sm[bank]);
  else
    return xmem_heap_get_unused_bytes(&xbm->_mgr._hm[bank]);
}

/* Returns 1 if the banks are contiguous, else returns 0 */
int xmem_banks_contiguous()
{
  unsigned i;

  xmem_bank_mgr_t *xbm = xmem_get_bank_mgr();

  XMEM_CHECK(xbm, 0, "xmem_banks_contiguous: bank manager not initialized\n");

  XMEM_CHECK(xbm->_banks_initialized, 0,
             "xmem_banks_contiguous: banks not initialized\n");

  if (xbm->_num_banks == 1)
    return 0;
  for (i = 0; i < xbm->_num_banks-1; ++i) {
    if (!xbm->_banks[i]._is_contiguous)
      return 0;
  }
  return 1;
}
