/* Copyright (c) 2003-2012 Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.
*/

/* ADDING API FUNCTIONS
// ===================================
// If you add an API function to mp.h, you need to add the name of the function
// to the exported_symbols file that lists all exported symbols.
*/

#ifndef MP_INCLUDED
#define MP_INCLUDED

#include <stdio.h>

/* API for Multi-processor Simulation */

/*

Introduction
============

This file defines basic MP simulation objects such as processors,
memories, peripheral devices and connectors.  To create an MP
simulation, write a main program that creates a set of MP simulation
objects and connect the together, and start the simulation with a call
of XTMP_stepSystem.

MP Simulation Objects
=====================

Though we don't explicitly use C++, we do have a type hierarchy which
allows us to write functions that take many types of objects, not
just processors, or memories.  Most functions dynamically check the
type of the objects passed to them.  The following lists the types of
objects defined in this file.

XTMP_component: the superclass of all MP simulation objects, i.e. all
MP simulation objects belong to this object type.

XTMP_core: an xtensa core processor.  To create one of these you must
first create a configuration object by reading a configuration object.

XTMP_memory: a memory address with 32-bit addresses and uses pages to
represent a potential sparse data storage.

XTMP_device: a peripheral device that you can customize.  See
XTMP_deviceFunctions object.

XTMP_lock: a special device that implements multiprocessor 
synchronization locks.  Each lock has a PIF-width range of addresses 
mapped to it.  A write of a non-zero value x to an address within this 
range causes the writing processor to stall if the lock has a non-zero 
value.  The stalled processor resumes when the lock becomes zero and 
the lock is set to x.  A write of zero from any processor clears the 
lock.  The lock device does not try to track which processor cleared 
the lock or set it to a non-zero value.  We recommend that the 
processors write their PRID into the lock. 

XTMP_queue: a peripheral device that implements a FIFO queue
that can be connected to cores.

XTMP_connector: Connectors route processor read/write transactions to 
memories and devices. Notice that you can treat an XTMP_connector 
object as an XTMP_component as well. 

Connecting MP Simulation Objects
================================

To connect simulation objects, you first create a XTMP_connector,
then connect all your simulation objects to the connector with 
XTMP_addMapEntry().  The connector is attached to the core using
XTMP_connectToCore.

*/

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN extern
#if !__bool_true_false_are_defined
#define true  1
#define false 0
typedef unsigned char bool;
#endif
#endif

#ifdef _WIN32
typedef unsigned __int64 u64;
typedef unsigned __int64 XTMP_time;
typedef          __int64 XTMP_timeDelta;
#define PCT_LLD "%I64d"
#define PCT_LLU "%I64u"
#else
typedef unsigned long long u64;
typedef unsigned long long XTMP_time;
typedef signed   long long XTMP_timeDelta;
#define PCT_LLD "%lld"
#define PCT_LLU "%llu"
#endif


/*
// type definitions for addresses etc.
*/
typedef unsigned long long XTMP_address;
typedef unsigned int u32;
typedef unsigned short int u16;
typedef unsigned char u8;
#define OPAQUE_TYPE(t) struct t ## _struct; typedef struct t ## _struct *t
#define XTMP_ADDRESS_MAX 0xffffffffffffffffULL

typedef enum {
  XTMP_READ	  = 0x00,
  XTMP_BLOCKREAD  = 0x10,
  XTMP_BURSTREAD  = 0x30,
  XTMP_RCW        = 0x50,
  XTMP_WRITE      = 0x80,
  XTMP_BLOCKWRITE = 0x90,
  XTMP_BURSTWRITE = 0xb0,
  XTMP_SNOOP      = 0x60 /* This transaction type is reserved for future use */
} XTMP_transactionType;

typedef enum {
  XTMP_OK            = 0,
  XTMP_ERR_ADDR      = 1,
  XTMP_ERR_DATA      = 2,
  XTMP_ERR_ADDR_DATA = 3,
  XTMP_ERR           = XTMP_ERR_ADDR_DATA,
  XTMP_NACC          = 4,
  XTMP_EXOK          = 5 /* Used for AXI exclusive transactions */
} XTMP_status;

// AXI4 compatible burst types
typedef enum {
  XTMP_BURST_FIXED = 0,
  XTMP_BURST_INCR = 1,
  XTMP_BURST_WRAP = 2
} XTMP_burstType;

typedef enum {
  XTMP_MASTER_NONE = 0,
  XTMP_MASTER_DATA = 1,
  XTMP_MASTER_INST = 2,
  XTMP_MASTER_IDMA = 3,
  XTMP_MASTER_IDMA_CH0 = 4,
  XTMP_MASTER_IDMA_CH1 = 5,
  XTMP_MASTER_IDMA_CH2 = 6,
  XTMP_MASTER_IDMA_CH3 = 7
} XTMP_masterType;

#define XTMP_error(status) (((status) & XTMP_ERR) && (status != XTMP_EXOK))

typedef enum {
  XTMP_PRI_LOW         = 0,
  XTMP_PRI_MEDIUM_LOW  = 1,
  XTMP_PRI_MEDIUM_HIGH = 2,
  XTMP_PRI_HIGH        = 3,
  // AXI4 compatible QOS priorities
  XTMP_QOS_0           = 0,
  XTMP_QOS_1           = 1,
  XTMP_QOS_2           = 2,
  XTMP_QOS_3           = 3,
  XTMP_QOS_4           = 4,
  XTMP_QOS_5           = 5,
  XTMP_QOS_6           = 6,
  XTMP_QOS_7           = 7,
  XTMP_QOS_8           = 8,
  XTMP_QOS_9           = 9,
  XTMP_QOS_10          = 10,
  XTMP_QOS_11          = 11,
  XTMP_QOS_12          = 12,
  XTMP_QOS_13          = 13,
  XTMP_QOS_14          = 14,
  XTMP_QOS_15          = 15,
} XTMP_priority;

typedef enum {
  XTMP_SNOOP_NOSNOOP            = 0,
  XTMP_SNOOP_READNOSNOOP        = 0,
  XTMP_SNOOP_WRITENOSNOOP       = 0,
  XTMP_SNOOP_READONCE           = 0,
  XTMP_SNOOP_WRITEUNIQUE        = 0,
  XTMP_SNOOP_READSHARED         = 1,
  XTMP_SNOOP_WRITELINEUNIQUE    = 1,
  XTMP_SNOOP_READCLEAN          = 2,
  XTMP_SNOOP_WRITECLEAN         = 2,
  XTMP_SNOOP_READNOTSHAREDDIRTY = 3,
  XTMP_SNOOP_WRITEBACK          = 3,
  XTMP_SNOOP_EVICT              = 4,
  XTMP_SNOOP_WRITEEVICT         = 5,
  XTMP_SNOOP_INVALID6           = 6,
  XTMP_SNOOP_READUNIQUE         = 7,
  XTMP_SNOOP_CLEANSHARED        = 8,
  XTMP_SNOOP_CLEANINVALID       = 9,
  XTMP_SNOOP_INVALIDA           = 10,
  XTMP_SNOOP_CLEANUNIQUE        = 11,
  XTMP_SNOOP_MAKEUNIQUE         = 12,
  XTMP_SNOOP_MAKEINVALID        = 13,
  XTMP_SNOOP_DVMCOMPLETE        = 14,
  XTMP_SNOOP_DVPMESSAGE         = 15
} XTMP_snoopCommand;

typedef enum {
  XTMP_CYCLE_ACCURATE = 0,
  XTMP_FUNCTIONAL     = 1
} XTMP_simMode;

typedef enum {
  XTMP_SIM_MODE_READY = 0,
  XTMP_SIM_MODE_ERROR = 1,
  XTMP_SIM_MODE_PENDING = 2
} XTMP_simModeResponse;

typedef enum {
  XTMP_FAST_ACCESS_NONE = 0,
  XTMP_FAST_ACCESS_RAW = 1,
  XTMP_FAST_ACCESS_DENY = 2,
  XTMP_FAST_ACCESS_CALLBACKS = 3,
  XTMP_FAST_ACCESS_PEEKPOKE = 4,
  XTMP_FAST_ACCESS_CUSTOM_CALLBACKS = 5
} XTMP_fastAccessKind;


/*
// The XTMP_deviceXfer data structure is the generic device transfer 
// record used for both requests and responses on the inbound and 
// outbound channels of the PIF and internal ports.  A new transfer
// record is allocated with XTMP_deviceXferNew and can be deallocated
// with XTMP_deviceXferFree.  An existing transfer record can be
// cleared for reuse (a new post) with XTMP_deviceXferReset.
//
// The get and set accessors enumerated below are the public interface
// and will ensure your code continues to work in future releases.
//
*/
typedef struct _XTMP_deviceXfer XTMP_deviceXfer;

EXTERN XTMP_deviceXfer* XTMP_deviceXferNew(XTMP_transactionType type,
					   XTMP_address address,
					   u32 size,
					   u32* data);
EXTERN void XTMP_deviceXferFree(XTMP_deviceXfer* xfer);
EXTERN void XTMP_deviceXferReset(XTMP_deviceXfer* xfer,
				 XTMP_transactionType type,
				 XTMP_address address,
				 u32 size,
				 u32* data);

/*
 * Is the transaction a request or a response?
 */
EXTERN bool XTMP_xferIsRequest(XTMP_deviceXfer* xfer);
EXTERN bool XTMP_xferIsResponse(XTMP_deviceXfer* xfer);

EXTERN void XTMP_xferSetRequest(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetResponse(XTMP_deviceXfer* xfer);

/*
 * Get and set the transaction type.
 */
EXTERN XTMP_transactionType XTMP_xferGetType(XTMP_deviceXfer* xfer);
EXTERN void                 XTMP_xferSetType(XTMP_deviceXfer* xfer,
                                             XTMP_transactionType type);

/*
 * Get and set the burst type.
 */
EXTERN XTMP_burstType XTMP_xferGetBurstType(XTMP_deviceXfer* xfer);
EXTERN void           XTMP_xferSetBurstType(XTMP_deviceXfer* xfer,
                                            XTMP_burstType burst_type);

/*
 * Get and set the transaction address.
 */
EXTERN XTMP_address XTMP_xferGetAddress(XTMP_deviceXfer* xfer);
EXTERN void         XTMP_xferSetAddress(XTMP_deviceXfer* xfer,
                                        XTMP_address addr);

/*
 * Get and set the size associated with the transaction --
 * the number of bytes to be read or written.
 */
EXTERN u32  XTMP_xferGetSize(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetSize(XTMP_deviceXfer* xfer, u32 size);

/*
 * Get and set the size associated with the transaction --
 * the number of bytes to be read or written.
 */
EXTERN u8  XTMP_xferGetBurstSize(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetBurstSize(XTMP_deviceXfer* xfer, u8 burst_size);

/*
 * Get or set the pointer to the data associated with the transaction.
 */
EXTERN u32* XTMP_xferGetData(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetData(XTMP_deviceXfer* xfer, u32* data);

/*
 * Get or set the transaction status.
 */
EXTERN XTMP_status XTMP_xferGetStatus(XTMP_deviceXfer* xfer);
EXTERN void        XTMP_xferSetStatus(XTMP_deviceXfer* xfer, XTMP_status status);

/*
 * Get or set the PC associated with the transaction.
 */
EXTERN u32  XTMP_xferGetPC(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetPC(XTMP_deviceXfer* xfer, u32 pc);

/*
 * Get the unique ID associated with the transaction.
 * XTMP_xferSetId is provided only for completeness; a unique ID is
 * assigned when an xfer is created, and should not be set explicitly.
 */
EXTERN u64  XTMP_xferGetId(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetId(XTMP_deviceXfer* xfer, u64 id);

/*
 * Get the load/store element number associated with a load request.
 */
EXTERN u32  XTMP_xferGetLoadStoreElement(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetLoadStoreElement(XTMP_deviceXfer* xfer, u32 lsElement);

/*
 * Get or set the byte enables, used for partial word transactions.
 * There is one bit for each byte.  The least significant bit
 * corresponds to byte 0 in a little-endian configuration.
 */
EXTERN u64  XTMP_xferGetByteEnables(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetByteEnables(XTMP_deviceXfer* xfer, u64 byteEnables);
EXTERN bool XTMP_xferUseByteEnables(XTMP_deviceXfer* xfer);

/*
 * Get or set the indication whether this read or block-read
 * transaction is for an instruction fetch.
 */
EXTERN bool XTMP_xferIsFetch(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetFetch(XTMP_deviceXfer* xfer, bool ifetch);

/*
 * Get or set the transfer number associated with a block request.
 */
EXTERN u32  XTMP_xferGetTransferNumber(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetTransferNumber(XTMP_deviceXfer* xfer, u32 blockNumber);

/*
 * Get or set the indication whether the transaction is the
 * last sub-transfer of a (multi-word) block transaction.
 */
EXTERN bool XTMP_xferIsLastData(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetLastData(XTMP_deviceXfer* xfer, bool lastData);

/*
 * Get or set the request priority (for inbound PIF transactions)
 */ 
EXTERN XTMP_priority XTMP_xferGetPriority(XTMP_deviceXfer* xfer);
EXTERN void          XTMP_xferSetPriority(XTMP_deviceXfer* xfer,
                                          XTMP_priority priority);

/*
 * The following routine is to implement ReadCondWrite callbacks.
 * It gets or sets the data that is to be used to compare to the data
 * read from the address specified for the transaction. A write will
 * be performed if the comparison is successful.
 */
EXTERN u32  XTMP_xferGetCompareData(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetCompareData(XTMP_deviceXfer* xfer, u32 scompare);

/*
 * Get or set the pointer to the user data associated with the transaction.
 * This is for use not by ISS, but by the application.
 */
EXTERN void* XTMP_xferGetUserData(XTMP_deviceXfer* xfer);
EXTERN void  XTMP_xferSetUserData(XTMP_deviceXfer* xfer, void* data);

/*
 * Get or set the transaction PIF attribute.
 */
EXTERN u32  XTMP_xferGetPIFAttribute(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetPIFAttribute(XTMP_deviceXfer* xfer, u32 pifAttribute);

/*
 * Get or set the Data RAM attribute, which corresponds to.
 * 160-bit wide DRamAttr and DRamWrAttr TIE interfaces.
 * NULL return value means that the attribute has not been set.
 */
EXTERN u32* XTMP_xferGetDataRamAttribute(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetDataRamAttribute(XTMP_deviceXfer* xfer,
                                         u32* dramAttribute);

/*
 * Get or set the indication whether this transaction comes from
 * the micro DMA engine.
 */
EXTERN bool XTMP_xferIsMicroDMA(XTMP_deviceXfer *xfer);
EXTERN void XTMP_xferSetMicroDMA(XTMP_deviceXfer *xfer, bool udma);

/*
 * Get or set the channel number for the micro DMA transactions.
 */
EXTERN u32  XTMP_xferGetMicroDMAChannel(XTMP_deviceXfer *xfer);
EXTERN void XTMP_xferSetMicroDMAChannel(XTMP_deviceXfer *xfer, u32 udma_ch);

/*
 * Get or set the indication whether this is an AXI exclusive transaction.
 */
EXTERN bool XTMP_xferIsExclusive(XTMP_deviceXfer *xfer);
EXTERN void XTMP_xferSetExclusive(XTMP_deviceXfer *xfer, bool excl);

/*
 * Get or set the indication whether the PassDirty bit is set an AXI response.
 */
EXTERN bool XTMP_xferPassDirty(XTMP_deviceXfer *xfer);
EXTERN void XTMP_xferSetPassDirty(XTMP_deviceXfer *xfer, bool pass_dirty);

/*
 * Get or set the indication whether the IsShared bit is set in an AXI response.
 */
EXTERN bool XTMP_xferIsShared(XTMP_deviceXfer *xfer);
EXTERN void XTMP_xferSetIsShared(XTMP_deviceXfer *xfer, bool is_shared);

/*
 * Get or set the AXI Cache bits for an AXI transaction.
 */
EXTERN u8   XTMP_xferGetAXICache(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetAXICache(XTMP_deviceXfer* xfer, u8 ax_cache);

/*
 * Get or set the AXI Domain bits for an AXI transaction.
 */
EXTERN u8   XTMP_xferGetAXIDomain(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetAXIDomain(XTMP_deviceXfer* xfer, u8 axi_domain);

/*
 * Get or set the AXI Snoop bits for an AXI transaction.
 */
EXTERN XTMP_snoopCommand   XTMP_xferGetAXISnoop(XTMP_deviceXfer* xfer);
EXTERN void                XTMP_xferSetAXISnoop(XTMP_deviceXfer* xfer, XTMP_snoopCommand axi_snoop);

/*
 * Get or set the AXI Master ID for an AXI transaction.
 */
EXTERN u64  XTMP_xferGetAXIId(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetAXIId(XTMP_deviceXfer* xfer, u64 axiId);

/*
 * Get or set the AXI Prot bits for an AXI transaction.
 */
EXTERN u8   XTMP_xferGetAXIProt(XTMP_deviceXfer* xfer);
EXTERN void XTMP_xferSetAXIProt(XTMP_deviceXfer* xfer, u8 axi_prot);


/* Helper functions to simplify processing of block transactions */

/*
 * Determine whether this transaction is the first sub-transfer
 * of a (multi-word) block transaction.
 */
EXTERN bool XTMP_xferIsFirstInBlock(XTMP_deviceXfer* xfer);

/*
 * Get a pointer to the block transfer data associated with the transaction.
 */
EXTERN u32* XTMP_xferGetBlockTransferData(XTMP_deviceXfer* xfer,
					  u32 blockNumber);

/*
// MP Simulation Objects.
*/
OPAQUE_TYPE(XTMP_params);	/* a parameter object */
typedef void* XTMP_component;	/* superclass of all XTMP components */
OPAQUE_TYPE(XTMP_core);		/* An xtensa core */
OPAQUE_TYPE(XTMP_memory);	/* A memory device */
OPAQUE_TYPE(XTMP_device);	/* bfm peripheral, new version */
OPAQUE_TYPE(XTMP_lock);		/* lock device */
OPAQUE_TYPE(XTMP_connector);	/* to connect multiple devices to a core */
OPAQUE_TYPE(XTMP_queue);	/* a queue */
OPAQUE_TYPE(XTMP_event);	/* a primitive to suspend and resume threads */
OPAQUE_TYPE(XTMP_register);	/* a reference object for register values */
OPAQUE_TYPE(XTMP_userThread);	/* user threads */
OPAQUE_TYPE(XTMP_tiePort);	/* TIE port: export/import/queue/lookup ports */
OPAQUE_TYPE(XTMP_tieGroup);     /* TIE port group: 
				 * State Export, Import Wire,
				 * Input Queue, Output Queue, Lookup */

OPAQUE_TYPE(XTMP_fastAccessRequest);   /* fast access request */

typedef struct _XTMP_deviceFunctions XTMP_deviceFunctions;

/*
// Get the core or device that initiated a transaction request.
// Return NULL if the initiator is not an XTMP_core/XTMP_device.
*/
EXTERN XTMP_core   XTMP_xferGetSourceCore(XTMP_deviceXfer* xfer);
EXTERN XTMP_device XTMP_xferGetSourceDevice(XTMP_deviceXfer* xfer);


/* Object creation
//
// For devices and memories with start and size parameters: size must equal
// some power of two, and size must divide start, i.e. all device and memories
// must start on a "size" boundary.
*/

/*
// Before you can create a processor you need to read parameter files into
// a XTMP_params object.  We provide two functions to do this.
//
// XTMP_paramsNew: uses the value of XTENSA_SYSTEM to find the parameter
// file.
//
// XTMP_paramsNewFromPath: uses the path you specify via systemDirs to find
// the parameter files
//
*/
EXTERN XTMP_params XTMP_paramsNew( const char *paramsFileName /*ReadOnly*/,
				   char *tdkFiles[]     /*ReadOnly*/);
/*
// paramsFileName: the name of the parameter file for a core. 
// tdkFiles: the parameter files of the TIE you created locally.
//    TIE you entered on the web site is already part of your core.
*/
EXTERN XTMP_params XTMP_paramsNewFromPath( char *systemDirs[]   /*ReadOnly*/,
					   const char *paramsFileName /*ReadOnly*/,
					   char *tdkFiles[]     /*ReadOnly*/);
/*
// paramsFileName and tdkFiles: same as XTMP_paramsNew.
// systemDirs: an array of names of directories in which to search for
//    the parameter for your core.  The last entry in the array should
//    be NULL.
*/

EXTERN XTMP_core     XTMP_coreNew(const char *name /*ReadOnly*/,
				  XTMP_params cnf, 
				  char *args[] /*ReadOnly*/);
/*
// name: The name you use to identify the core.  The simulator will use this
//    name to refer to the core in error messages.
// cnf: an XTMP_params object returned by either XTMP_paramsNew or
//    XTMP_paramsNewFromPath.
// args: A null-terminated array of arguments for the simulator that will
//    simulate the target program on this core.  It is not the list of
//    arguments for the target program itself.				  
*/


/*
// Set the clock factor for a specifed core.  The clock factor is the
// number of cycles of the simulation clock that must elapse prior to
// the core being cycled.  A clock factor of 4 will cause the core to
// be stepped every fourth cycle of the simulation clock.
//
// If the clock factor for a core is not set by callling this function,
// the default value is 1.
//
// This function can only be called prior to the starting the simulation.
*/
EXTERN void
XTMP_setClockFactor( XTMP_core core, unsigned clock_factor );
/*
// core:         the core to set the clock factor for.
// clock_factor: a non-zero integer value for the clock factor.
*/


EXTERN XTMP_memory XTMP_memoryNew( const char *name,
                                   u32 widthInBytes,
                                   bool isBigEndian,
                                   u32 sizeInBytes );
EXTERN XTMP_memory XTMP_pifMemoryNew( const char *name,
                                      u32 widthInBytes, 
                                      bool isBigEndian,
                                      u32 sizeInBytes );
EXTERN XTMP_memory XTMP_localMemoryNew( const char *name,
                                        u32 widthInBytes,
                                        bool isBigEndian,
                                        u32 sizeInBytes );
/*
// name: The name you use to identify the memory.  The simulator uses this
//       name to refer to the memory in error messages.
// widthInBytes: the width of the memory in bytes
// isBigEndian: the endianness of the memory
// sizeInBytes: the size of the memory in bytes.  
*/

/* Set the read-only attribute for a memory. */
EXTERN void XTMP_memorySetReadOnly( XTMP_memory memory, bool value );
/*
// memory: The memory under consideration.
// value: if true, the memory is set to be read-only.
*/

/* Create a new device with this function. */
EXTERN XTMP_device XTMP_deviceNew(const char *name /*ReadOnly*/,
				  XTMP_deviceFunctions *instance,
				  void* deviceData,
				  unsigned dataWidthInBytes,
				  u32 sizeInBytes);

/*
// name: The name used to identify the device.
// instance: The data structure used to represent this instance of the device.
//    This should be filled in prior to the call.
// dataWidthInBytes: The width of the PIF in bytes.
// sizeInBytes: the size of the memory region mapped to this device.
*/

EXTERN XTMP_lock   XTMP_lockNew  ( const char *name/*ReadOnly*/, 
				      unsigned dataWidthInBytes,
				      bool isBigEndian,
				      unsigned nlocks);
/*
// name: The name used to identify the device.
// dataWidthInBytes: The width of the PIF in bytes.
// isBigEndian: The endianness of the PIF.
// nlocks: the number of locks you want to create.  The region of memory
//    mapped to the lock devices will begin at "start" and end at
//    "start+nlocks*dataWidthInBytes-1".
*/

/* Set the blocking attribute for a lock object. */
EXTERN void XTMP_lockSetBlockingMode( XTMP_lock lock, bool blocking );
/*
// lock: The lock object under consideration.
// blocking: if true, a process attempting to acquire a lock in use
//           blocks until the lock becomes free.
*/


/*
// Create connectors: width is in bytes.
// You cannot create a connector with width less than 4 bytes.
*/

EXTERN XTMP_connector
XTMP_connectorNew(const char *name, unsigned dataWidthInBytes, u32 sizeInBytes);


/* Connecting components together
// ==============================
// Use XTMP_addMapEntry to connect any component to a XTMP_connector.
// This function also requires that you provide an address mapping.
*/ 

EXTERN void XTMP_addMapEntryRange( XTMP_connector connector,
				   XTMP_component component,
				   XTMP_address start,
				   u32 size,
				   XTMP_address xlate );

EXTERN void XTMP_addMapEntry( XTMP_connector connector,
			      XTMP_component component,
			      XTMP_address start,
			      XTMP_address xlate );
/*
// connector: a connector object.
// component: the component object to be connected and mapped.
// core: the Xtensa core whose map into which the map entry should be added.
// start: the start of the core's address space that should map to the
//    component.  The mapped segment begins at "start".
// size: size of the mapped segment.  It ends at "start"+"size"-1.
//    With XTMP_addMapEntry, this parameter is implicitly the size of
//    the component.
// xlate: the start of the address space on translation.  It should be
//    the same as "start" if no translation is desried.
//
// *NOTE* regions mapped to internal memory ports cannot be overriden with
// this function.
*/


/* Internal Rams, Roms, Ports
// ==========================
//
// You can configure an Xtensa processor to have zero or more of each 
// kind of internal memories and port: InstructionRAM, InstructionROM, 
// DataRAM, DataROM, XLMI, UnifiedRAM.  The maximum number for each port
// type is specified in the Databook.  You can use these functions to 
// override the configured addresses of the internal memories/ports 
// but you will get a warning.  If you want the hardware to reflect 
// the overridden values, then you will need to go the configuration 
// GUI and reconfigure your Xtensa. 
*/
typedef enum
{
  XTMP_PT_FIRST = 0,
  XTMP_PT_IRAM  = 0,
  XTMP_PT_INSTRUCTION_RAM = XTMP_PT_IRAM,
  XTMP_PT_IROM,
  XTMP_PT_INSTRUCTION_ROM = XTMP_PT_IROM,
  XTMP_PT_DRAM,
  XTMP_PT_DATA_RAM        = XTMP_PT_DRAM,
  XTMP_PT_DROM,
  XTMP_PT_DATA_ROM        = XTMP_PT_DROM,
  XTMP_PT_XLMI,
  XTMP_PT_DATA_PORT       = XTMP_PT_XLMI,
  XTMP_PT_URAM,
  XTMP_PT_UNIFIED_RAM     = XTMP_PT_URAM,
  XTMP_PT_PIF,
  XTMP_PT_LAST
} XTMP_portType;


EXTERN void XTMP_connectToCore( XTMP_core core,
			        XTMP_portType portType,
			        unsigned portNumber,
			        XTMP_component component,
			        XTMP_address address );

/*
// core: the core to which to connect the component.
// portType: the port type of the port to connect to.
// component: the component object to connect to the core.
*/

/* Loading programs
 * return 0 or false upon failure, true or nozero on
 * success.
 */
EXTERN bool XTMP_loadProgram(XTMP_core core, 
                             const char *programFileName,
			     char *programArgs[]);
/*
// core: the core into which the program should be loaded.
// programName: The name of the file containing the executable
//    to load into the core.  
// programArgs: a null-terminated array of strings to pass to the program
//    as its argv[] array.
*/


typedef enum {
  XTMP_DEVICE_OK    = 0,         /* transaction posted successfully */
  XTMP_DEVICE_ERROR = 1,         /* error */
  XTMP_DEVICE_RESPONSE_NACC = 2  /* response not accepted */
} XTMP_deviceStatus;


/* XTMP_deviceFunctions: a structure that represents a device instance
// ====================================================================
//
// You should create one of these for each device instance.  Because
// each of the functions can identify which instance is being
// activated via its first argument, i.e. "instance", you can share
// transaction functions between different instances.
*/

struct _XTMP_deviceFunctions {

  /* Function called when a transfer record is posted to the device.
  // A device is not required to enqueue xfers, but that will be
  // the typical behavior.
  */
  XTMP_deviceStatus (*post)(void *deviceData,
			    XTMP_deviceXfer *xfer);
  /*
  // post parameters:
  //
  // deviceData: the device data identifying the instance that
  //  	should perform the post.
  // xfer: the transfer record holding the request type, address, etc.
  */

  /* Function called to read data from the device with no side effects. */
  XTMP_deviceStatus (*peek)(void *deviceData,
			    u32 *dst,
			    XTMP_address addr,
			    u32 size);
  /*
  // peek parameters:
  //
  // deviceData: the device data identifying the instance that
  //  	should perform the peek.
  // dst: points to a buffer of length PIF width bytes, or equivalently
  //    a of "PIF width bytes"/4 u32 elements.  Think of the dst buffer
  //    as eventually holding (after the read) PIF width bytes of data,
  //    starting at the floor(addr/"PIF Width bytes") * "PIF width bytes".
  // addr: the address to read from.
  // size: the size of this request, 1, 2, 4, .... Pifwidth
  */

  /* Function called to write data to the device with no side effects. */
  XTMP_deviceStatus (*poke)(void *deviceData,
			    XTMP_address addr,
			    u32 size,
			    const u32 *src);
  /*
  // poke parameters:
  //
  // deviceData: the device data identifying the instance that
  //  	should perform the poke.
  // addr: the address to read from.
  // size: the size of the request, 1, 2, 4, .... PIFwidth
  // src: pointer to buffer containing write data.
  */

  /* 
  // With TurboXim simulation engine, there are additional callbacks 
  // logically associated with a device. They are added to the device with:
  // 
  // XTMP_setDeviceFastAccessHandler() - fast-access request handling
  // XTMP_setDeviceSimModeCallBacks()  - simulation mode switching
  */
};


/*
// Post a transaction request from a source to a target device.
*/ 
EXTERN XTMP_deviceStatus
XTMP_post(XTMP_component target, XTMP_component source, XTMP_deviceXfer *xfer);
/*
// target: The device or core that is to be the recipient.
// source: The device or core that is the sender of the transfer record.
//         To directly forward a request, source should be NULL.
// xfer: The transaction information.
*/


/*
// Post a response to a trasaction request.
*/ 
EXTERN XTMP_deviceStatus
XTMP_respond(XTMP_deviceXfer *xfer, XTMP_status status);
/*
// xfer:   The transaction information to send back at the end
//         at the end of the current cycle.
// status: The transaction status.
*/


/*
// Peek from, or poke to, a device.
*/ 
EXTERN XTMP_deviceStatus 
XTMP_peek(XTMP_component component, u32 *dst, XTMP_address addr, u32 size);

EXTERN XTMP_deviceStatus
XTMP_poke(XTMP_component component, XTMP_address addr, u32 size, const u32 *src);


/* Device callbacks for TurboXim fast functional simulation
// --------------------------------------------------------
*/

typedef XTMP_deviceStatus
(*XTMP_deviceFastAccessHandler)(void *deviceData,
                                XTMP_fastAccessRequest request,
                                XTMP_address addr);
/* Fast-access request handler
// ---------------------------
// deviceData: a handle for the device data structure
//
// request: an opaque structure with information about the fast-access request
//          You can query it with the following accessor functions:
//            XTMP_getFastAccessRequestComponent()
//            XTMP_getFastAccessRequestAddress()
//            XTMP_isFastAccessRequestBigEndian()
//            XTMP_getFastAccessRequestByteWidth()
//
// addr: the address for which the fast access is requested
// 
// Use the same 'request' handle to select the fast-access method:
// 
//   XTMP_setFastAccessRaw()
//     Direct (raw) fast access to memory -- the most efficient implementation.
// 
//   XTMP_setFastAccessCallBacks()
//     Fast access through read and write callback functions:
//     XTMP_fastAccessRead and XTMP_fastAccessWrite.
// 
//   XTMP_setFastAccessCustomCallBacks()
//     Fast access through read and write callback functions with custom data:
//     XTMP_fastAccessCustomRead and XTMP_fastAccessCustomWrite.
// 
//   XTMP_setFastAccessPeekPoke()
//     Fast access with the peek and poke callbacks.
//     This is the default for a device that has not registered
//     a fast-access handler function.
// 
//   XTMP_denyFastAccess()
//     Fast access denied -- future accesses to the specified address range
//     will use posts for data accesses and peeks for instruction fetches.
//                            
//    You should always call one of the above functions when returning
//    XTMP_DEVICE_OK from the fast-access handler.
//  
//    Returning XTMP_DEVICE_ERROR is similar to denying fast access,
//    but the next access to the same address will request fast access again,
//    which slows down the simulation.
*/

typedef XTMP_simModeResponse
(*XTMP_devicePrepareToSwitchSimMode)(void *deviceData, 
                                     XTMP_simMode mode,
                                     XTMP_event readyToSwitchEvent);
/* Prepare to switch simulation mode
// ---------------------------------
// A callback function that prepares the device for switching
// between the fast functional and cycle-accurate simulation. 
//
// deviceData: a handle for the device data structure
// 
// mode: the simulation mode to switch to
// 
// readyToSwitchEvent: currently unused (always NULL)
// 
// If the device is still processing an outstanding request that
// prevents from switching, it should return XTMP_SIM_MODE_PENDING.
// After all the devices return XTMP_SIM_MODE_READY, the switch will
// be performed by invoking the XTMP_deviceSwitchSimMode callback.
*/

typedef XTMP_deviceStatus
(*XTMP_deviceSwitchSimMode)(void *deviceData, XTMP_simMode mode);

/* Switch simulation mode
// ----------------------
// A callback function that switches the device between
// fast functional and cycle-accurate simulation.
// 
// deviceData: a handle for the device data structure
// 
// mode: the simulation mode to switch to
// 
// This can be called any time before the simulation begins.
// After that, for devices that might not be able to switch on every cycle,
// this should only be called in the  switching protocol that invokes
// the XTMP_devicePrepareToSwitchSimMode callback until all the devices are
// ready to switch.
*/



/* Register the fast-access handler function for the device.
// This should be done immediately following XTMP_deviceNew().
*/
EXTERN void
XTMP_setDeviceFastAccessHandler(XTMP_device device,
                                XTMP_deviceFastAccessHandler handler);

/* Register the callbacks for simulation mode switching.
// This is only needed for devices that may behave differently
// in the cycle-accurate mode and the fast functional mode.
// It should be done immediately following XTMP_deviceNew().
*/
EXTERN void
XTMP_setDeviceSimModeCallBacks(XTMP_device device,
                               XTMP_devicePrepareToSwitchSimMode prepare,
                               XTMP_deviceSwitchSimMode switchSimMode);

/* 
// Peek and poke callbacks with master information
*/
typedef XTMP_deviceStatus
(*XTMP_masterPeekCallBack)(void *deviceData,
                           u32 *dst,
                           XTMP_address addr,
                           u32 size,
                           XTMP_masterType master);

typedef XTMP_deviceStatus
(*XTMP_masterPokeCallBack)(void *deviceData,
                           XTMP_address addr,
                           u32 size,
                           const u32 *src,
                           XTMP_masterType master);
/*
// Register master peek and poke callbacks.
*/
EXTERN void
XTMP_setMasterPeekPokeCallBacks(XTMP_device device,
                                XTMP_masterPeekCallBack masterPeek,
                                XTMP_masterPokeCallBack masterPoke);

/* Response flow control
// ---------------------
// A callback function invoked during response processing
// to check if the sender is ready to accept the response.
//
// deviceData: a handle for the device data structure of the sender
// xfer: the transfer record used in the response
*/
typedef bool
(*XTMP_acceptResponseCallBack)(void *deviceData, XTMP_deviceXfer *xfer);

/* Register the response-ready callback for the device.
// This should be done immediately following XTMP_deviceNew().
*/
EXTERN void
XTMP_setAcceptResponseCallBack(XTMP_device device,
                               XTMP_acceptResponseCallBack callBack);


/* Raw fast access
// ---------------
// Allow raw memory access to the aligned block of addresses that contains
// the specified 'address'. 
//    
// If the data is stored in a byte array by target address, swizzle is 0.
// If the data is stored optimally in an array of u32's on a little-endian
// host, swizzle is 0 for a little-endian target, 3 for a big-endian target.
// 
// Optimal storage on a 32 bit machine allows a 4-byte load to generate a
// value in the endianness of the target. This avoids swapping endianness
// on 4 byte loads.
// 
// Returns false on failure:
//   If the address is not in the specified block
//   If the blockStart is not aligned to a power of 2 greatern then swizzle
*/
EXTERN bool
XTMP_setFastAccessRaw(XTMP_fastAccessRequest request,
		      XTMP_address address,
		      XTMP_address blockStart,
		      XTMP_address blockEnd,
		      u32 *rawData,
		      u32 swizzle);

/* Compute the swizzle value from the memory storage.
//   Write the values 0 to 15 into the memory addresses 0 to 15.
//   Pass the memory storage in buf.
//   Returns 0xffffffff if no swizzle matches.
*/
EXTERN u32
XTMP_computeFastAccessSwizzle(const u32 *buf);


/* Callback fast access
// --------------------
*/

/* Fast-access read callback.
// Data should be returned in HOST-byte-endian, little-word-first order.
*/
typedef void
(*XTMP_fastAccessRead)(void *callBackArg,
                       u32 *dst,
                       XTMP_address address,
                       u32 size);

/* Fast-access write callback.
// Data is passed in HOST-byte-endian, little-word-first order.
*/
typedef void
(*XTMP_fastAccessWrite)(void *callBackArg,
                        XTMP_address address,
                        u32 size,
                        const u32 *src);

/* Allow fast access via the function callbacks to the aligned block
// of addresses that contains the specified address. 
// 
// Returns false on failure, when the address is not in the specified block.
// If fastAccessRead  is NULL, the block is not readable.
// If fastAccessWrite is NULL, the block is not writable.
*/
EXTERN bool
XTMP_setFastAccessCallBacks(XTMP_fastAccessRequest request,
			    XTMP_address address,
			    XTMP_address blockStart,
			    XTMP_address blockEnd,
			    XTMP_fastAccessRead fastAccessRead,
			    XTMP_fastAccessWrite fastAccessWrite,
			    void *callBackArg);

/* Fast-access read callback with custom data.
// Data should be returned in HOST-byte-endian, little-word-first order.
*/
typedef void
(*XTMP_fastAccessCustomRead)(void *callBackArg,
                             u32 *dst,
                             XTMP_address address,
                             u32 size,
                             const u32 *customData);

/* Fast-access write callback with custom data.
// Data is passed in HOST-byte-endian, little-word-first order.
*/
typedef void
(*XTMP_fastAccessCustomWrite)(void *callBackArg,
                              XTMP_address address,
                              u32 size,
                              const u32 *src,
                              const u32 *customData);

/* Allow fast access via the function callbacks with custom user data
// to the aligned block of addresses that contains the specified address. 
// 
// Returns false on failure, when the address is not in the specified block.
// If fastAccessRead  is NULL, the block is not readable.
// If fastAccessWrite is NULL, the block is not writable.
*/
EXTERN bool
XTMP_setFastAccessCustomCallBacks(XTMP_fastAccessRequest request,
			          XTMP_address address,
			          XTMP_address blockStart,
			          XTMP_address blockEnd,
			          XTMP_fastAccessCustomRead fastAccessRead,
			          XTMP_fastAccessCustomWrite fastAccessWrite,
			          void *callBackArg);

/* Peek/poke fast access
// ---------------------
// Allow fast access with peek and poke callbacks.
// Returns false on failure, when the  address is not in the specified block.
*/
EXTERN bool
XTMP_setFastAccessPeekPoke(XTMP_fastAccessRequest request,
			   XTMP_address address,
			   XTMP_address blockStart,
			   XTMP_address blockEnd);


/* Deny fast access to the aligned block of addresses that contains
// the requested address. The core will access these addresses with
// the standard XTMP post protocol.
// Returns false if address is not in the specified block.
*/
EXTERN bool
XTMP_denyFastAccess(XTMP_fastAccessRequest request,
                    XTMP_address address,
		    XTMP_address blockStart,
                    XTMP_address blockEnd);

/* Do not allow fast read access */
EXTERN void
XTMP_denyFastAccessRead(XTMP_fastAccessRequest request);

/* Do not allow fast write access */
EXTERN void
XTMP_denyFastAccessWrite(XTMP_fastAccessRequest request);

/* Built-in fast-access handler for denying all fast access requests.
// Pass this function as the 'handler' argument in 
// XTMP_setDeviceFastAccessHandler to force all fast accesses to be denied.
*/
EXTERN XTMP_deviceStatus 
XTMP_denyFastAccessHandler(void *deviceData,
			   XTMP_fastAccessRequest request,
			   XTMP_address addr);

/* Remove a range of addresses from a fast access response
// to avoid any overlap. The range may wrap around 0.
// This can be used to restrict a response to a block by removing all
// addresses outside of the block.
// The localAddress is the address used in the call the 
// XTMP_requestFastAccess
// Returns false if the range includes the local address of the response.
*/
EXTERN bool
XTMP_removeFastAccessBlock(XTMP_fastAccessRequest request,
			   XTMP_address localAddress,
			   XTMP_address blockStart,
			   XTMP_address blockEnd);

/* Revoke all fast access granted to the core. When invoked from
// a callback from the core, it may not take effect immediately.
*/
EXTERN void
XTMP_revokeFastAccess(XTMP_core core);


/* XTMP_fastAccessRequest
// ----------------------
*/

/* Create a fast-access request from the 'source' device */
EXTERN XTMP_fastAccessRequest 
XTMP_fastAccessRequestNew(XTMP_component source,
			  XTMP_address requestAddress,
			  bool requestIsBigEndian,
			  u32 requestByteWidth);

/* Deallocate a fast-access request */
EXTERN void
XTMP_fastAccessRequestFree(XTMP_fastAccessRequest request);

/* Fast-access request accessors */
EXTERN XTMP_component
XTMP_getFastAccessRequestComponent(XTMP_fastAccessRequest request);

EXTERN XTMP_address
XTMP_getFastAccessRequestAddress(XTMP_fastAccessRequest request);

EXTERN XTMP_masterType
XTMP_getFastAccessRequestMaster(XTMP_fastAccessRequest request);

EXTERN bool
XTMP_isFastAccessRequestBigEndian(XTMP_fastAccessRequest request);

EXTERN u32
XTMP_getFastAccessRequestByteWidth(XTMP_fastAccessRequest request);

/* Send a fast-access request to the 'target' device */
EXTERN XTMP_deviceStatus
XTMP_requestFastAccess(XTMP_component target,
		       XTMP_fastAccessRequest request,
		       XTMP_address addr);

/* Perform a fast-access read */
EXTERN XTMP_deviceStatus
XTMP_readFastAccess(XTMP_component target,
		    XTMP_fastAccessRequest request,
		    u32 *dst,
		    XTMP_address addr,
		    u32 size);

/* Perform a fast-access write */
EXTERN XTMP_deviceStatus
XTMP_writeFastAccess(XTMP_component target,
		     XTMP_fastAccessRequest request,
		     XTMP_address addr,
		     u32 size,
		     const u32 *src);

/* Functions for querying the response from a fast-access request
// --------------------------------------------------------------
 */

/* Get the response address and the enclosing block */
EXTERN void
XTMP_getFastAccessResponseBlock(XTMP_fastAccessRequest request,
				XTMP_address *responseAddress,
				XTMP_address *responseBlockStart,
				XTMP_address *responseBlockEnd);

/* Get the response block localized based on the 'localAddress' */
EXTERN void
XTMP_getFastAccessLocalBlock(XTMP_fastAccessRequest request,
			     XTMP_address localAddress,
			     XTMP_address *localBlockStart,
			     XTMP_address *localBlockEnd);

/* Get the fast-access type: raw, callbacks, peek/poke, deny, none */
EXTERN XTMP_fastAccessKind
XTMP_getFastAccessKind(XTMP_fastAccessRequest request);

/* Does the fast-access response allow reading or writing */
EXTERN void
XTMP_getFastAccessInfo(XTMP_fastAccessRequest request,
		       bool *isWritable,
		       bool *isReadable);

/* Get the fast-access raw data pointer and swizzle */
EXTERN bool
XTMP_getFastAccessRawInfo(XTMP_fastAccessRequest request,
			  u32 **orig_rawData,
			  u32  *swizzle);

/* Get the fast-access read and write callback functions */
EXTERN bool
XTMP_getFastAccessCallBackInfo(XTMP_fastAccessRequest request,
			       XTMP_fastAccessRead *fastAccessRead,
			       XTMP_fastAccessWrite *fastAccessWrite,
			       void **callBackArg);

/* Get the fast-access read and write callback functions with custom data */
EXTERN bool
XTMP_getFastAccessCustomCallBackInfo(XTMP_fastAccessRequest request,
			             XTMP_fastAccessCustomRead *fastAccessRead,
			             XTMP_fastAccessCustomWrite *fastAccessWrite,
			             void **callBackArg);


/* Simulation mode switching
// -------------------------
//
// To switch all devices into or out of the functional mode:
//   
// Before simulation begins, just invoke XTMP_switchSimMode().
//   
// Once simulation has begun a two phase prepare/switch protocol is required:
// 
// 1) Request the switch for all devices that must switch.  For
//    devices that are not ready, they should respond with
//    XTMP_SIM_MODE_PENDING.  When a not ready device becomes ready
//    to switch, if the event passed was not NULL, it should invoke
//    XTMP_fireEvent() on the token.  If one or more devices
//    initially indicated that they were not ready, once any device
//    signals ready, perform this step again.
//
//    Devices can continue to pass on passive requests throughout the
//    switch preparation, but they should not generate new requests
//    while a switch is pending.
//
// 2) The switcher should wait at least one cycle, then use
//    XTMP_switchSimMode() to switch all devices at the same time.
//     
// In systems where memory stores can be outstanding after the
// core believes they are complete, you can choose whether the
// memories should participate in the switching protocol. If they
// do not, the switching device should wait an appropriate 
// number of cycles after all switching devices are ready before
// switching.  This will ensure that all outstanding stores have
// completed before the system switches to functional mode.
*/

EXTERN XTMP_simModeResponse 
XTMP_prepareToSwitchSimMode(XTMP_component component,
                            XTMP_simMode mode,
			    XTMP_event readyToSwitchEvent);

EXTERN XTMP_deviceStatus
XTMP_switchSimMode(XTMP_component component, XTMP_simMode);

EXTERN XTMP_simMode
XTMP_getSimMode(XTMP_component component);

EXTERN bool
XTMP_isSimModeSwitchPending(XTMP_component component);


/* Relaxed simulaton out-of-order cycle limit
// ------------------------------------------
// A device should never go more than "relaxedCycleLimit" out of
// order when it is in the functional mode. In the cycle-accurate mode,
// this should have no effect. 
// This parameter should be set before simulation begins.
*/
EXTERN void
XTMP_setRelaxedSimulationCycleLimit(u32 relaxedCycleLimit);

EXTERN u32
XTMP_getRelaxedSimulationCycleLimit(void);


/* Core-specific relaxed simulaton out-of-order cycle limit
// ---------------------------------------------------------
// A core should never go more than its local "relaxedCycleLimit"
// out of order when it is in the functional mode and this parameter is set.
// In the cycle-accurate mode, this should have no effect.
// This parameter is expressed in terms of system clock periods and
// can be changed dynamically at simulation time.
*/
EXTERN void
XTMP_setCoreRelaxedSimulationCycleLimit(XTMP_core core, u32 relaxedCycleLimit);

EXTERN u32
XTMP_getCoreRelaxedSimulationCycleLimit(XTMP_core core);


/* XTMP_queue is a simple FIFO queue model that can connect
// two cores, one with a TIE input queue and the other with
// a TIE output queue interface.
*/
EXTERN XTMP_queue
XTMP_queueNew(const char *name, unsigned bitWidth, unsigned queueDepth);

/* Connect two cores with an XTMP_queue.
// The producer core writes to the queue via a TIE output queue;
// the consumer core reads from the queue via a TIE input queue.
*/
EXTERN bool
XTMP_connectQueue(XTMP_queue queue,
                  XTMP_core producer,
                  const char *producerTieOutputQueueName,
                  XTMP_core consumer,
                  const char *consumerTieInputQueueName);


/*
 * Controlling the simulation
 * ==========================
 *
 *
 * XTMP_wait
 * ------------
 * You can suspend the current thread for a specified number of cycles 
 * with XTMP_wait.  You can only call XTMP_wait from a context of a 
 * thread.  XTMP_wait outside of a thread will corrupt your simulation 
 * and yield unpredictable results. 
 * 
 * Calling XTMP_wait with an argument of zero has a special connotation. 
 * The argument of zero represents an infinitesimal delay, which is an 
 * arbitrariy small but positive unit of time.  The current thread gets 
 * re-scheduled to wake up after all pending threads have had a chance to 
 * run.  Simulation time is not advanced.
 * 
 * CPU threads (the post callback function) can call XTMP_wait only with 
 * an argument of zero.  Stalling a CPU would prevent it from carrying 
 * out the actions it needs to perform from cycle to cycle.  Similarly, 
 * ticker functions need to be invoked at every clock cycle and cannot 
 * stall themselves by calling XTMP_wait with a positive argument. 
 * 
 * You can't use XTMP_wait from peek and poke, because it doesn't make 
 * sense to stall the debugger. 
 * 
 * XTMP_stepSystem
 * ---------------
 * Advance the simulation by ncycles.
 *
 * XTMP_disable
 * ------------
 * Disable a cpu from running with each clock.
 *
 * XTMP_enable
 * -----------
 * Enable a cpu so that it will run with the clock.
 *
 * XTMP_stepCore
 * -------------
 * Step a single cpu and the rest of the system, while disabling all other
 * cpus.
 *
 * XTMP_reset
 * ----------
 * Reset the cpu.  Note the cpu's when created should already be in reset
 * but if you want to reset the cpu again later in the simulation.
 *
 * XTMP_printSummary
 * -----------------
 * Print processor execution summary.
 */
EXTERN void        XTMP_wait(unsigned ncycles);
EXTERN void        XTMP_stepSystem( int ncycles );
#define XTMP_start XTMP_stepSystem /* deprecated name */
EXTERN void        XTMP_disable( XTMP_core c);
EXTERN void        XTMP_enable( XTMP_core c );
EXTERN void        XTMP_stepCore( XTMP_core c, unsigned ncycles );
#define XTMP_step  XTMP_stepCore /* deprecated name */
EXTERN void        XTMP_setInterrupt( XTMP_core c, unsigned interruptNumber,
				      bool boolValue );
EXTERN void        XTMP_setExternalInterrupt( XTMP_core c,
					      unsigned interruptNumber,
					      bool boolValue );
EXTERN void        XTMP_reset( XTMP_core c);
EXTERN const char* XTMP_getComponentName(XTMP_component c);
EXTERN void        XTMP_printSummary(XTMP_core c, bool useStdError);
EXTERN const char* XTMP_getSummary(XTMP_core c);
EXTERN int         XTMP_getTargetExitCode(XTMP_core c);

EXTERN bool XTMP_getLocalMemoryAddress( XTMP_params params,
                                        XTMP_portType ty,
                                        unsigned port_num,
                                        XTMP_address* address );
EXTERN bool XTMP_getLocalMemorySize( XTMP_params params,
                                     XTMP_portType ty,
                                     unsigned port_num,
                                     u32* size );
EXTERN bool XTMP_getLocalMemoryBusy( XTMP_params params,
                                     XTMP_portType ty,
                                     unsigned port_num,
                                     bool* busy );
EXTERN bool XTMP_getLocalMemoryInbound( XTMP_params params,
                                        XTMP_portType ty,
                                        unsigned port_num,
                                        bool* inbound );

EXTERN bool XTMP_getSysRamAddress( XTMP_params params, XTMP_address* address );
EXTERN bool XTMP_getSysRomAddress( XTMP_params params, XTMP_address* address );
EXTERN bool XTMP_getSysRamSize( XTMP_params params, u32* size );
EXTERN bool XTMP_getSysRomSize( XTMP_params params, u32* size );

EXTERN void XTMP_setReadDelay( XTMP_memory mem, unsigned latency );
EXTERN void XTMP_setWriteDelay( XTMP_memory mem, unsigned latency );
EXTERN void XTMP_setBlockReadDelays( XTMP_memory mem,
                                     unsigned first,
                                     unsigned repeat);
EXTERN void XTMP_setBlockWriteDelays( XTMP_memory mem,
                                      unsigned first,
                                      unsigned repeat);

/* Utility functions */
EXTERN XTMP_time XTMP_clockTime(void);
EXTERN u32 XTMP_numberOfThreads(void);

/*
// XTMP_byteWidth returns the maximum of the core PIF width
// and the iDMA bus width, 0 if they not configured. 
// For configurations where the iDMA bus width is greater than the
// core PIF width, this is the width that should be used for devices
// connected to XTMP_PT_PIF.
*/
EXTERN unsigned XTMP_byteWidth( XTMP_core c );

/*
// XTMP_pifWidth returns the core PIF width (which is the
// same as the inbound PIF width), 0 if not configured. 
*/
EXTERN unsigned XTMP_pifWidth( XTMP_core c );

/*
// XTMP_loadStoreWidth returns the local data memory interface width.
*/
EXTERN unsigned XTMP_loadStoreWidth( XTMP_core c );

/*
// XTMP_instFetchWidth returns the local instruction memory interface width.
*/
EXTERN unsigned XTMP_instFetchWidth( XTMP_core c );

EXTERN bool XTMP_isBigEndian( XTMP_core c );
EXTERN bool XTMP_isBigEndianFromParams( XTMP_params p );
EXTERN bool XTMP_hasCoreExited( XTMP_core c );
EXTERN bool XTMP_haveAllCoresExited(void);

EXTERN void XTMP_dumpCoreStatus(void);

/*
// Register a callback to receive notification when
// an XLMI load has retired or been flushed.
*/
typedef void (*XTMP_loadRetiredCallBack)( XTMP_core core, 
                                          u32 lsUnit,
                                          bool flush, 
                                          void* callBackArg );
/* Returns the previous callback value */
EXTERN XTMP_loadRetiredCallBack
XTMP_setLoadRetiredCallBack( XTMP_core core,
                             XTMP_loadRetiredCallBack callBack,
                             void* callBackArg );
/*
// core:        the core performing the load.
// lsUnit:      the load/store unit performing the load.
// flush:       an indicator if the load is flushed (true) or retired (false).
// callBackArg: an argument available to the callback identical to the value
//              provided when it was registered.
*/

/*
// Register a callback to receive notification when
// a processor enters or leaves the wait mode.
*/
typedef void (*XTMP_waitiCallBack)( XTMP_core core,
                                    bool waitMode,
                                    void* callBackArg );
/* Returns the previous callback value */
EXTERN XTMP_waitiCallBack
XTMP_setWaitiCallBack( XTMP_core core, 
                       XTMP_waitiCallBack callBack, 
                       void* callBackArg );
/*
// core:        the core entering or leaving the wait mode.
// waitMode:    true when entering the wait mode (WAITI instruction),
//              false when leaving the wait mode (an interrupt).
// callBackArg: an argument available to the callback identical to the value
//              provided when it was registered.
*/

/*
// Register a callback to receive notification when the lock signal
// associated with the MP Sync (S32C1I) instruction changes state.
*/
typedef void (*XTMP_dataRamLockCallBack)( XTMP_core core,
				          u32 lsUnit,
				          u32 portNumber,
				          bool value, 
				          void* callBackArg );
/* Returns the previous callback value */
EXTERN XTMP_dataRamLockCallBack
XTMP_setDataRamLockCallBack( XTMP_core core,
                             XTMP_dataRamLockCallBack callBack,
                             void* callBackArg );
/*
// core:        the core performing S32C1I to a Data RAM.
// lsUnit:      the load/store unit performing S32C1I.
// portNumber:  The index and bank of the Data RAM being locked.
//              - the first byte is the index of the Data RAM: 0 or 1.
//              - the second byte is the bank index of the access
//                (it be between 0 and num_banks-1, and 0 for unbanked DRAMS)
//              - For example, portNumber=0x21 is bank 2 and Data RAM 1.
// value:       the new signal value: true means locked, false means unlocked.
// callBackArg: an argument available to the callback identical to the value
//              provided when it was registered.
*/

/*
// Register a callback to receive notification when
// a TX processor enters or leaves the halt mode.
*/
typedef void (*XTMP_coreHaltedCallBack)( XTMP_core core,
                                         bool coreHalted,
                                         void* callBackArg );
/* Returns the previous callback value */
EXTERN XTMP_coreHaltedCallBack
XTMP_setCoreHaltedCallBack( XTMP_core core, 
                            XTMP_coreHaltedCallBack callBack, 
                            void* callBackArg );
/*
// core:        the core entering or leaving the halt mode.
// coreHalted:  true when entering the halt mode,
//              false when leaving the halt mode.
// callBackArg: an argument available to the callback identical to the value
//              provided when it was registered.
*/

/*
// Register a callback to receive notification when
// a TX processor CoreStatus changes.
*/
typedef void (*XTMP_coreStatusCallBack)( XTMP_core core,
                                         u32  coreStatus,
                                         void* callBackArg );
/* Returns the previous callback value */
EXTERN XTMP_coreStatusCallBack
XTMP_setCoreStatusCallBack( XTMP_core core, 
                            XTMP_coreStatusCallBack callBack, 
                            void* callBackArg );
/*
// core:        the core for which the CoreStatus signal is monitored.
// coreStatus:  the CoreStatus signal value.
// callBackArg: an argument available to the callback identical to the value
//              provided when it was registered.
*/

/*
// Register a callback to receive notification when
// the PArithmeticException output signal changes.
*/
typedef void (*XTMP_arithmeticExceptionCallBack)( XTMP_core core,
                                                  bool value,
                                                  void* callBackArg );
/* Returns the previous callback value */
EXTERN XTMP_arithmeticExceptionCallBack
XTMP_setArithmeticExceptionCallBack( XTMP_core core, 
                                     XTMP_arithmeticExceptionCallBack callBack, 
                                     void* callBackArg );
/*
// core:        the core for which the PArithmeticException signal is monitored.
// value:       the new signal value.
// callBackArg: an argument available to the callback identical to the value
//              provided when it was registered.
*/

/*
// Register a callback to receive notification when
// a processor finishes an iDMA stream that has trigger configured in it.
*/
typedef void (*XTMP_triggerOutIDMA_CallBack)( XTMP_core core,
					      bool value,
                                              u32 index,
					      void* callBackArg );
/* Returns the previous callback value */
EXTERN XTMP_triggerOutIDMA_CallBack
XTMP_setTriggerOutIDMA_CallBack( XTMP_core core, 
				 XTMP_triggerOutIDMA_CallBack callBack, 
				 void* callBackArg );

/*
// Register a callback to receive notification when
// a processor finishes a coherent Read and triggers the RACK signal.
*/
typedef void (*XTMP_AXI_RACK_CallBack)( XTMP_core core,
                                        bool value,
                                        void* callBackArg );
/* Returns the previous callback value */
EXTERN XTMP_AXI_RACK_CallBack
XTMP_set_AXI_RACK_CallBack( XTMP_core core,
                            XTMP_AXI_RACK_CallBack callBack,
                            void* callBackArg );

/*
// Register a callback to receive notification when
// a processor's WWDT notifies earlyResetReq signal.
*/
typedef void (*XTMP_wwdtEarlyResetReqCallBack)( XTMP_core core,
                                                bool value,
                                                void* callBackArg );
/* Returns the previous callback value */
EXTERN XTMP_wwdtEarlyResetReqCallBack
XTMP_setWWDTEarlyResetReqCallBack( XTMP_core core,
                                   XTMP_wwdtEarlyResetReqCallBack callBack,
                                   void* callBackArg );
/*
// core:        the core having a WWDT component.
// value:       the value of output signal (earlyResetReq).
// callBackArg: an argument available to the callback identical to the value
//              provided when it was registered.
*/

/*
  // Register a callback to receive notification when
  // a processor's WWDT notifies resetReq signal.
  */
typedef void (*XTMP_wwdtResetReqCallBack)( XTMP_core core,
                                           bool value,
                                           void* callBackArg );
/* Returns the previous callback value */
EXTERN XTMP_wwdtResetReqCallBack
XTMP_setWWDTResetReqCallBack( XTMP_core core,
                              XTMP_wwdtResetReqCallBack callBack,
                              void* callBackArg );
/*
// core:        the core having a WWDT component.
// value:       the value of output signal (resetReq).
// callBackArg: an argument available to the callback identical to the value
//              provided when it was registered.
*/

/*
// Register a callback to receive notification when
// a processor's WWDT notifies HeartBeat signal.
*/
typedef void (*XTMP_wwdtHeartBeatCallBack)( XTMP_core core,
                                            bool value,
                                            void* callBackArg );
/* Returns the previous callback value */
EXTERN XTMP_wwdtHeartBeatCallBack
XTMP_setWWDTHeartBeatCallBack( XTMP_core core,
                               XTMP_wwdtHeartBeatCallBack callBack,
                               void* callBackArg );
/*
// core:        the core having a WWDT component.
// value:       the value of output signal (HearBeat).
// callBackArg: an argument available to the callback identical to the value
//              provided when it was registered.
*/

/*
  // Register a callback to receive notification when
  // a processor's WWDT notifies faultInfo signal.
  */
typedef void (*XTMP_wwdtFaultInfoCallBack)( XTMP_core core,
                                            u8 value,
                                            void* callBackArg );
/* Returns the previous callback value */
EXTERN XTMP_wwdtFaultInfoCallBack
XTMP_setWWDTFaultInfoCallBack( XTMP_core core,
                               XTMP_wwdtFaultInfoCallBack callBack,
                               void* callBackArg );
/*
// core:        the core having a WWDT component.
// value:       the value of output signal (faultInfo).
// callBackArg: an argument available to the callback identical to the value
//              provided when it was registered.
*/

/*
// Register a callback to be invoked for sim mode switching when
// an Xtensa processor executes a SYS_sim_mode_switch SIMCALL.
// Note: The callback must return without yielding to the XTMP or
//       SystemC kernel (no calls are allowed to methods such as
//       XTMP_wait(), XTMP_waitOnEvent(), or SystemC wait()).
// Note: This callback will not be called if the requested mode
//       is already the current mode.
*/
typedef bool (*XTMP_simModeSwitchCallBack)( XTMP_simMode mode,
                                            void* callBackArg );
EXTERN void
XTMP_setSimModeSwitchCallBack( XTMP_core core, 
                               XTMP_simModeSwitchCallBack callBack, 
                               void* callBackArg );
/*
// core:        the core initiating the simulation mode switch
// mode:        the simulation mode to switch to
// callBackArg: an argument available to the callback identical to
//              the value provided when it was registered.
*/

/*
// Freeze the core pipeline after it advances at the beginning of the 
// next cycle.  It will advance no further until the stall is released 
// by calling this routine with a value of false. 
*/
EXTERN void XTMP_setRunStall(XTMP_core cpu, bool val);
/*
// cpu: the core under consideration.
// val: the value to set the stall signal to.
*/

/*
// If the relocatable vector option is configured, this function can
// be used to choose between the two static vector base addresses.
// NOTE: In hardware, the StatVectorSel pin is latched when coming out
// reset. Therefore, you should call this function only just prior to
// calling XTMP_reset().
*/
EXTERN void XTMP_setStatVectorSel( XTMP_core cpu, unsigned select );
/*
// cpu:    the core under consideration.
// select: the value for the StatVectorSel input pin (0 or 1).
*/

/*
// If the relocatable vector option is configured, this function can
// be used to drive the value of the alternate vector base address
// (when StatVectorSel is 1).
// NOTE: In hardware, the AltResetVec pins are latched when coming out
// reset. Therefore, you should call this function only just prior to
// calling XTMP_reset().
*/
EXTERN void XTMP_setAltResetVec( XTMP_core cpu, XTMP_address alt_reset_vec );
/*
// cpu:           the core under consideration.
// alt_reset_vec: the value for the AltResetVec input pins
*/

/*
// Set the trigger in signal for the iDMA
*/
EXTERN void XTMP_setTriggerInIDMA( XTMP_core cpu, u32 index );

/*
  // Set the trigger in signal for the iDMA
  */
EXTERN void XTMP_setWWDTExtClear( XTMP_core cpu, bool val );

/*
  // Set the trigger in signal for the iDMA
  */
EXTERN void XTMP_setWWDTExtCounterDis( XTMP_core cpu, bool val );

/* Debugging related functions.
//
// XTMP_setWaitForDebugger: if called with v!=0, the core will wait
// for a connection from a debugger before proceeding with instruction
// execution.  If called with v==0, the core will use polling to
// detect a debugger connection as it executes instructions.  Note that
// the rest of the system can still execute while the core waits for
// a connection.  If you don't want this to happen, then you need
// to call XTMP_setBlockingDebugIO on the core.
//
// XTMP_getDebuggerPort returns zero upon failure.
*/
EXTERN void XTMP_setSynchronizedSimulation( bool v );
EXTERN unsigned XTMP_enableDebug( XTMP_core c, unsigned portNumber);
EXTERN void XTMP_setWaitForDebugger( XTMP_core c, bool v);
EXTERN void XTMP_setBlockingDebugIO( XTMP_core c, bool v);
EXTERN void XTMP_setDebugPollInterval( XTMP_core c, unsigned p );
EXTERN unsigned XTMP_getDebuggerPort( XTMP_core c );
EXTERN void XTMP_setAsynchronousDebugConnection( XTMP_core c, bool v );

typedef void (*XTMP_debuggerCallBack)(void *);

/* Returns the previous callback value */
EXTERN XTMP_debuggerCallBack
XTMP_setSoftBreakOutCallBack(XTMP_core c, 
			     XTMP_debuggerCallBack callBack,
			     void *callBackArg);

/* Returns the previous callback value */
EXTERN XTMP_debuggerCallBack
XTMP_setDebuggerResumeCallBack(XTMP_core c, 
			       XTMP_debuggerCallBack callBack,
			       void *callBackArg);

EXTERN void XTMP_softBreakInInterrupt( XTMP_core c );

/* Returns the previous callback value */
EXTERN XTMP_debuggerCallBack
XTMP_setDebuggerConnectCallBack(XTMP_core c, 
			     XTMP_debuggerCallBack callBack,
			     void *callBackArg);

/* Returns the previous callback value */
EXTERN XTMP_debuggerCallBack
XTMP_setDebuggerDisconnectCallBack(XTMP_core c, 
			     XTMP_debuggerCallBack callBack,
			     void *callBackArg);

/* Returns true if debugging is enabled on the specified core */
EXTERN bool XTMP_isDebuggable(XTMP_core c);

/* Change the order of the xt-gdb socket check and core run sub-steps */
EXTERN void XTMP_setCoordinatedDebugging(XTMP_core c, bool value);

/*
// Give the cycle count as tracked by each core.
*/
EXTERN XTMP_time XTMP_coreTime( XTMP_core c );

/*
// Tracing functions
*/
EXTERN void XTMP_setTraceFile( XTMP_core c, const char *fileName );
EXTERN void XTMP_closeTraceFile( XTMP_core c );
EXTERN void XTMP_setTraceLevel( XTMP_core c, unsigned l );

/*
// Print out information about the XTMP cores in the simulation in
// a form that can be interpreted by Xplorer.
*/
EXTERN char* XTMP_getCoreInformation(void);


/* Set the limit on how much memory is allocated by the cores on behalf
// of their target programs.
*/
EXTERN void XTMP_setMemoryLimit(unsigned megabytes);

/* Functions for accessing register values
//
// XTMP_getRegisterCount --- get the number of register the core has.  The
// count include inaccessible registers which you cannot access, see the
// lookup functions.  Inaccessible registers exists but this interface will
// not allow you to reference them.
//
// XTMP_getRegisterByIndex -- get a register by index where index ranges
// from 0 to XTMP_getRegisteCount(core)-1.  The function will returns
// NULL for inaccessible registers.
// 
// XTMP_getRegisterByName --- obtain an object for referring to a
// register by looking it up by name; this object works only for the specified
// core.  The function returns NULL for inaccessible registers or if no
// register with the given name exists.
//
// XTMP_getRegisterValue --- get the value of the register.  Returns the
// the bit length of the register.  It sets value to point to an internal
// buffer of the value.  The buffer will retain the same value until the
// next call to XTMP_getRegisterValue for the same register.  The caller
// may write into this buffer and pass it to XTMP_setRegisterValue.
//
// XTMP_copyRegisterValue --- copy the value of the register into 
// caller-provided buffer.  Caller must ensure buffer has the correct size.
// Returns the bit-length of the register.
//
// XTMP_setRegisterValue --- set a register with value provided.
//
// XTMP_getAllRegisters --- get all the register values.
//
// XTMP_setAllRegisters --- set all the register values from an array
// of values retrieved with XTMP_getAllRegisters.  This could become part
// of a save/restore scheme.
//
// The functions below return a register value as an array of 32-bit words
// independent of Xtensa byte ordering, i.e. the functions will convert each
// word into host ordering.  However, if a register's bit length does not
// evenly divide into 32-bit words, then either the first word or last word
// will contain extra bits, for big-endian or little-endian Xtensa
// respectively.
*/

EXTERN unsigned XTMP_getRegisterCount(XTMP_core cpu);
EXTERN XTMP_register XTMP_getRegisterByIndex(XTMP_core cpu, unsigned index);
#define XTMP_lookupRegisterByIndex XTMP_getRegisterByIndex /* deprecated name */
EXTERN XTMP_register XTMP_getRegisterByName( XTMP_core cpu, const char * );
#define XTMP_lookupRegisterByName XTMP_getRegisterByName /* deprecated name */
EXTERN unsigned XTMP_getRegisterValue( XTMP_register r, u32 **value );
EXTERN unsigned XTMP_copyRegisterValue( XTMP_register r, u32 *value );
EXTERN bool XTMP_getRegisterStageValue( XTMP_register reg, unsigned stage,
					   unsigned **value );
EXTERN bool XTMP_copyRegisterStageValue( XTMP_register reg, unsigned stage,
                                         u32 *value );
EXTERN void XTMP_setRegisterValue( XTMP_register r, const u32 *value );
EXTERN bool XTMP_setRegisterStageValue( XTMP_register r, unsigned stage, 
                                        const u32 *value );
EXTERN void XTMP_getAllRegisters( XTMP_core cpu, u32 **value, unsigned *nWords );
EXTERN void XTMP_setAllRegisters( XTMP_core cpu, u32 *value );
EXTERN const char* XTMP_getRegisterName( XTMP_register r );
EXTERN unsigned XTMP_getRegisterBitWidth( XTMP_register r );


/*
// Retrieve the number of the commit (W) pipeline stage:
// 3 on a 5-stage pipeline; 4 on a 7-stage pipeline.
// Stage R = 0, stage E = 1, etc.
*/
EXTERN unsigned XTMP_getCommitStage( XTMP_core cpu );
/*
// cpu: the core under consideration.
*/


/*
// Retrieve the number of the last stage in the processor pipeline.
// Stage R = 0, stage E = 1, etc.
*/
EXTERN unsigned XTMP_getLastStage( XTMP_core cpu );
/*
// cpu: the core under consideration.
*/


/*
// Retrieve the PC of the instruction in a given stage.
// If 0xffffffff is returned, the stage contains a bubble and
// not a valid instruction.  Stage R = 0, stage E = 1, etc.
*/
EXTERN u32 XTMP_getStagePC( XTMP_core cpu, unsigned stageNumber );
/*
// cpu: the core under consideration.
// stageNumber: the stage whose PC is desired.
*/


/*
// Cause the core to start fetching instructions from a new address.
*/
EXTERN bool XTMP_redirectPC( XTMP_core cpu, u32 newPC );
/*
// cpu: the core under consideration.
// newPC: the address to begin fetching from.
*/

/* Peek and Poke memory from the perspective of a specific core.
//
// XTMP_peekPhysical --- peek into the physical address space of a core.
//
// XTMP_pokePhysical --- poke the physical memory of a core.
//
// XTMP_peekVirtual --- peek into the virtual address space of a core.
// Note that this will use whatever virtual mapping in place at that time 
// of the call.
//
// XTMP_pokeVirtual --- poke the virtual memory of a core.  Same comments
// about MMU apply as before.
//
// Current limitation:  Addr must align to word boundary (i.e. 4-byte boundary)
// and size is multiple of words.  Otherwise, you get a warning and it
// truncates the address to the next lower word boundary and size to the
// lower word count.
//
// The functions return XTMP_DEVICE_OK upon success.
*/
EXTERN XTMP_deviceStatus XTMP_peekPhysical( XTMP_core c, u32 *buffer,
					    XTMP_address addr,
					    u32 size);
EXTERN XTMP_deviceStatus XTMP_pokePhysical( XTMP_core c, u32 *buffer,
					    XTMP_address addr,
					    u32 size);
EXTERN XTMP_deviceStatus XTMP_peekVirtual( XTMP_core c, u32 *buffer,
					   XTMP_address addr,
					   u32 size);
EXTERN XTMP_deviceStatus XTMP_pokeVirtual( XTMP_core c, u32 *buffer,
					   XTMP_address addr,
					   u32 size);

/*
// User Thread API
// ---------------
// Once a users thread has been created, its thread function is called 
// at the next cycle by the scheduler.  A thread function will execute 
// until it relinquishes control by calling XTMP_wait() or 
// XTMP_waitOnEvent() to suspend itself, thus releasing control back 
// to the scheduler.  A user thread is terminated when its thread 
// function returns.  You can use the same thread function for 
// different user threads, though you will need to be careful about 
// static local variables. 
//
// For example, the following thread function runs every clock cycle.
//
// void threadFunction( void* d)
// {
//   myData *data = (myData *) d;
//   while( data->enabled ) {
//     do_whatever();
//     XTMP_wait(1);
//   }
// }   
//
// If you want something that will just run at the 10th and
// the 30th cycle, and then never again:
//
// void threadFunction( void* d)
// {
//   myData *data = (myData *) d;
//   XTMP_wait(10);
//   do_whatever1();
//   XTMP_wait(20);
//   do_whatever2();
// }   
*/
typedef void (*XTMP_userThreadFunction)(void* userThreadData);
EXTERN XTMP_userThread XTMP_userThreadNew( const char *name,
					   XTMP_userThreadFunction
					   userThreadFunc,
					   void* data );
/* Clients
// -------
// Clients are loadable modules that process events generated by the
// core ISS.  Functionality such as tracing and profiling are implemented
// using clients.
//
// XTMP_loadClient loads a single client
// XTMP_loadClientFile loads clients specified in the given file
// XTMP_loadClientFromPath loads a single client from the specified directory
// XTMP_sendClientCommand sends a single command to a client
// 
// All three functions return true(1) upon success, false(0) upon failure.
*/
EXTERN bool
XTMP_loadClient( XTMP_core c, const char *client );
#define XTMP_clientLoad XTMP_loadClient /* deprecated name */

EXTERN bool
XTMP_loadClientFile( XTMP_core c, const char *filename );

EXTERN bool
XTMP_loadClientFromPath( XTMP_core c, const char *client, const char *dir );
#define XTMP_clientLoadFromPath XTMP_loadClientFromPath /* deprecated name */

EXTERN bool
XTMP_sendClientCommand( XTMP_core c, const char *command );
#define XTMP_clientCommandSend XTMP_sendClientCommand /* deprecated name */


/* Events
// ------
// Events are a synchronization mechanism between threads.  A thread
// can choose to suspend itself, waiting for an event to mature.  When
// the event is triggered, all threads waiting on that event begin
// participating in the simulation once more, starting with the next
// delta cycle.
*/

/* 
// Create a new event.
*/ 
EXTERN XTMP_event XTMP_eventNew( void );

/* 
// Deallocate an event.
*/ 
EXTERN void XTMP_eventFree( XTMP_event event );

/* 
// Suspend the current thread until the event e is triggered.
*/ 
EXTERN void XTMP_waitOnEvent( XTMP_event event );

/* 
// Trigger any threads waiting on event e.
// They will run in the next delta.
*/ 
EXTERN void XTMP_fireEvent( XTMP_event event );
#define XTMP_eventFire XTMP_fireEvent /* deprecated name */

/*
// Release functional-mode core(s) from the event stall
// associated with eventId
*/
EXTERN void XTMP_fireEventId( unsigned eventId );


/* XTMP_setEventDriven
// -------------------
// Informs the simulator that all queues and lookups connected to the core
// support event-driven stalls. This allows the core to avoid polling queue
// or lookup interfaces, which can result in a significant speedups of
// multi-processor simulations when cores are in functional simulation mode.
// To support event-driven simulation, a device connected to a 
// TIE queue or lookup interface must fire the event returned by
// XTMP_getTieStallRecheckEvent when its state changes:
// from empty to not empty (for a TIE input queue interface),
// from full to not full (for a TIE output queue interface), 
// from not ready to ready (for a TIE lookup interface).
// Currently, for cores that have timer interrupts, this has no effect.
*/

EXTERN void XTMP_setEventDriven(XTMP_core c, bool isEventDriven);

EXTERN bool XTMP_getEventDriven(XTMP_core c);

/*
// The stall recheck event should be fired by an event driven queue or
// lookup to allow the core to recheck its stall in event-driven mode.
*/
EXTERN XTMP_event 
XTMP_getTieStallRecheckEvent(XTMP_core c, const char *name);

/* This is an event that is fired when the core exits simulation */
EXTERN XTMP_event
XTMP_getCoreExitedEvent(XTMP_core c);

/* This is an event that is fired when all cores exit simulation */
EXTERN XTMP_event
XTMP_getAllCoresExitedEvent(void);


/* TIE Ports
// ---------
*/ 

/* The data is stored little word first with host endian byte order */
typedef void (*XTMP_tiePortCallBack)( XTMP_tiePort port,
                                      XTMP_core core,
                                      void *tiePortData,
                                      u32 *data );
/* 
// Returns true(1) upon success, false(0) upon failure.
*/
EXTERN bool XTMP_connectToTiePort( XTMP_core core, 
                                   void *tiePortData,
                                   const char *tiePortName,
                                   XTMP_tiePortCallBack callBack );

typedef enum {
  XTMP_TIE_PORT_IN  = 0, /* input port */
  XTMP_TIE_PORT_OUT = 1	 /* output port */
} XTMP_tiePortDirection;
#define XTMP_tiePortType XTMP_tiePortDirection /* deprecated name */

EXTERN unsigned
XTMP_getTiePortCount( XTMP_core core );

EXTERN XTMP_tiePort
XTMP_getTiePortByIndex( XTMP_core core, unsigned idx );

EXTERN XTMP_tiePort
XTMP_getTiePortByName( XTMP_core core, const char *name );

EXTERN const char *
XTMP_getTiePortName( XTMP_tiePort port );

EXTERN unsigned
XTMP_getTiePortBitWidth( XTMP_tiePort port );

EXTERN XTMP_tiePortDirection
XTMP_getTiePortDirection( XTMP_tiePort port );
#define XTMP_getTiePortType XTMP_getTiePortDirection /* deprecated name */

EXTERN XTMP_tieGroup
XTMP_getTiePortGroup( XTMP_tiePort port );

/* TIE port suffixes are:
   for State Export: ""
   for Import Wire: ""
   for Input Queue: "", "_PopReq", "_Empty"
   for Output Queue: "_PushReq", "_Full"
   for Lookup: "_Out_Req", "_In", "_Rdy"
   for External Register File: "_Write", "_WriteAddr", "_WriteData", 
                               "_ReadCheck", "_ReadCheckAddr", "_ReadStall", 
                               "_Read", "_ReadAddr", "_ReadData"
*/

/* TIE Port Groups
// ---------------
*/

typedef enum {
  XTMP_TIE_STATE_EXPORT = 0,
  XTMP_TIE_IMPORT_WIRE  = 1,
  XTMP_TIE_INPUT_QUEUE  = 2,
  XTMP_TIE_OUTPUT_QUEUE = 3,
  XTMP_TIE_LOOKUP       = 4,
  XTMP_TIE_EXT_REGFILE  = 5,
  XTMP_TIE_INPUT_PORT   = 6,
  XTMP_TIE_OUTPUT_PORT  = 7
} XTMP_tieGroupType;


EXTERN unsigned
XTMP_getTieGroupCount( XTMP_core core );

EXTERN XTMP_tieGroup
XTMP_getTieGroupByIndex( XTMP_core core, unsigned idx );

EXTERN XTMP_tieGroup
XTMP_getTieGroupByName( XTMP_core core, const char *name );

EXTERN const char *
XTMP_getTieGroupName( XTMP_tieGroup group );

EXTERN XTMP_tieGroupType
XTMP_getTieGroupType( XTMP_tieGroup group );

EXTERN unsigned
XTMP_getTieGroupPortCount( XTMP_tieGroup group );

EXTERN XTMP_tiePort
XTMP_getTieGroupPortByIndex( XTMP_tieGroup group, unsigned idx );

EXTERN bool
XTMP_isETieInterface(XTMP_core c, XTMP_tieGroupType group_type, 
                             const char *group_name);

EXTERN unsigned
XTMP_getTieLookupLatency(XTMP_tieGroup tpg);

EXTERN u32
XTMP_getInstructionWidth(XTMP_core cpu, u32 pc);

EXTERN u32
XTMP_disassemble(XTMP_core cpu, u32 pc, char *buf, u32 bufSize);


/* User Simcalls
// -------------
*/
typedef int (*XTMP_simcallCallBack)( XTMP_core core,
				     void *callBackArg,
				     int requestCode,
				     int arg1,
				     int arg2,
				     int arg3,
				     int arg4,
				     int arg5,
				     int arg6 );

EXTERN void
XTMP_setSimcallCallBack( XTMP_core core,
			 XTMP_simcallCallBack callBack,
			 void *callBackArg );


/* This section defines the interface between mp_driver.cxx and the XTMP
// system.
*/
typedef struct _XTMP_driverThreadInfo {
  void *handle;
  unsigned exitFlag;
} XTMP_threadInfo;
typedef void (*XTMP_threadCallBack)(XTMP_threadInfo *);


/*
// Extraction and Insertion macros 
// -------------------------------
//
// Their functionality can be deduced from their names using the following
// template:
//
// XTMP_extract<dataBitWidth><BEorLE><bitSize>(srcBuf, addrOrOffset)
//
// is a macro that returns a value of <bitSize> bits extracted from the 
// data buffer of size <dataBitWidth> bits and of <BEorLE> byte ordering
// stored in *srcBuf.  The argument addrOrOffset can be either an address or
// offset and specifies the location of the extracted bits within the pif
// buffer.  If it is an address, the macro will convert it to an offset by 
// masking it with (dataBitWidth/8-1).
// 
// XTMP_insert<dataBitWidth><BEorLE><bitSize>(dstBuf, addrOrOffset, value)
//
// Insert a value of <bitSize> bits into the data buffer of size <dataBitWidth> 
// bits and of <BEorLE> byte ordering stored in *dstBuf.  The location of
// the insertion is specified by addrOrOffset.  If it is an address, the
// macro will convert it to an offset by masking it with (dataBitWidth/8-1).
//
// Note: in the above you can use either an address or an offset to specify
// the location within the data buffer.  This works because a legal offset into
// a data buffer has the range 0..dataBitWidth/8-1.  The masking operation
// by both macros is a no-op when addrOrOffset is in that range.  If you
// specify an address for addrOrOffset, it is assumed that the the data buffer
// corresponds to the contents stored at 
// addrOrOffset & ~(dataBitWidth/8-1).
//
// You CANNOT use these macros to extract values that span multiple
// u32 words of the data buffer.   That is, if you extract a 16-bit quantity
// that spans two words of the data buffer you would only get the least
// significant byte on little-endian and the most significant byte on
// big-endian.
*/


/*
// Helper macro
// XTMP_bsLE: compute shift amount to access a byte in a word for little-endian
// XTMP_bsBE: compute shift amount to access byte in a big-endian word.
*/
#define XTMP_bsLE( addrOrOffset )		\
    (((addrOrOffset)&3)<<3)
#define XTMP_bsBE( addrOrOffset )		\
    ((3-((addrOrOffset)&3))<<3)

#define XTMP_extract32LE8(src,addrOrOffset )	\
    ((((u32*)(src))[0]>>XTMP_bsLE(addrOrOffset))&0xff)
#define XTMP_extract32LE16(src,addrOrOffset)	\
    ((((u32*)(src))[0]>>XTMP_bsLE(addrOrOffset))&0xffff)
#define XTMP_extract32LE24(src,addrOrOffset)	\
    ((((u32*)(src))[0]>>XTMP_bsLE(addrOrOffset))&0xffffff)
#define XTMP_extract32LE32(src,addrOrOffset)	\
    (((u32*)(src))[0])

#define XTMP_extract64LE8(src,addrOrOffset )	\
    XTMP_extract32LE8(&((u32*)(src))[((addrOrOffset)&4)>>2],(addrOrOffset))
#define XTMP_extract64LE16(src, addrOrOffset )	\
    XTMP_extract32LE16(&((u32*)(src))[((addrOrOffset)&4)>>2],(addrOrOffset))
#define XTMP_extract64LE24(src, addrOrOffset )	\
    XTMP_extract32LE24(&((u32*)(src))[((addrOrOffset)&4)>>2],(addrOrOffset))
#define XTMP_extract64LE32(src, addrOrOffset )	\
    (((u32*)(src))[((addrOrOffset)&4)>>2])

#define XTMP_extract128LE8(src,addrOrOffset)			 \
    XTMP_extract32LE8(&((u32*)(src))[((addrOrOffset)&0xc)>>2],(addrOrOffset))
#define XTMP_extract128LE16(src, addrOrOffset )			  \
    XTMP_extract32LE16(&((u32*)(src))[((addrOrOffset)&0xc)>>2],(addrOrOffset))
#define XTMP_extract128LE24(src, addrOrOffset )			  \
    XTMP_extract32LE24(&((u32*)(src))[((addrOrOffset)&0xc)>>2],(addrOrOffset))
#define XTMP_extract128LE32(src, addrOrOffset )	\
    (((u32*)(src))[((addrOrOffset)&0xc)>>2])

#define XTMP_extract256LE8(src,addrOrOffset)			 \
    XTMP_extract32LE8(&((u32*)(src))[((addrOrOffset)&0x1c)>>2],(addrOrOffset))
#define XTMP_extract256LE16(src, addrOrOffset )			  \
    XTMP_extract32LE16(&((u32*)(src))[((addrOrOffset)&0x1c)>>2],(addrOrOffset))
#define XTMP_extract256LE32(src, addrOrOffset )	\
    (((u32*)(src))[((addrOrOffset)&0x1c)>>2])

#define XTMP_extract512LE8(src,addrOrOffset)			 \
    XTMP_extract32LE8(&((u32*)(src))[((addrOrOffset)&0x3c)>>2],(addrOrOffset))
#define XTMP_extract512LE16(src, addrOrOffset )			  \
    XTMP_extract32LE16(&((u32*)(src))[((addrOrOffset)&0x3c)>>2],(addrOrOffset))
#define XTMP_extract512LE32(src, addrOrOffset )	\
    (((u32*)(src))[((addrOrOffset)&0x3c)>>2])

#define XTMP_extract32BE8(src,addrOrOffset )		\
    ((((u32*)(src))[0]>>XTMP_bsBE(addrOrOffset))&0xff)
#define XTMP_extract32BE16(src, addrOrOffset )		\
    (((((u32*)(src))[0]<<XTMP_bsLE(addrOrOffset))&0xffff0000)>>16)
#define XTMP_extract32BE24(src, addrOrOffset )		\
    (((((u32*)(src))[0]<<XTMP_bsLE(addrOrOffset))&0xffffff00)>>8)
#define XTMP_extract32BE32(src, addrOrOffset )		\
    (((u32*)(src))[0])

#define XTMP_extract64BE8(src,addrOrOffset )				  \
    XTMP_extract32BE8(&((u32*)(src))[1-(((addrOrOffset)&4)>>2)],addrOrOffset)
#define XTMP_extract64BE16(src, addrOrOffset )				   \
    XTMP_extract32BE16(&((u32*)(src))[1-(((addrOrOffset)&4)>>2)],addrOrOffset)
#define XTMP_extract64BE24(src, addrOrOffset )				   \
    XTMP_extract32BE24(&((u32*)(src))[1-(((addrOrOffset)&4)>>2)],addrOrOffset)
#define XTMP_extract64BE32(src, addrOrOffset )	\
    (((u32*)(src))[1-(((addrOrOffset)&4)>>2)])

#define XTMP_extract128BE8(src,addrOrOffset )			   \
    XTMP_extract32BE8(&((u32*)(src))[3-(((addrOrOffset)&0xc)>>2)],addrOrOffset)
#define XTMP_extract128BE16(src, addrOrOffset )			    \
    XTMP_extract32BE16(&((u32*)(src))[3-(((addrOrOffset)&0xc)>>2)],addrOrOffset)
#define XTMP_extract128BE24(src, addrOrOffset )			    \
    XTMP_extract32BE24(&((u32*)(src))[3-(((addrOrOffset)&0xc)>>2)],addrOrOffset)
#define XTMP_extract128BE32(src, addrOrOffset )	\
    (((u32*)(src))[3-(((addrOrOffset)&0xc)>>2)])

#define XTMP_extract256BE8(src,addrOrOffset )			   \
    XTMP_extract32BE8(&((u32*)(src))[7-(((addrOrOffset)&0x1c)>>2)],addrOrOffset)
#define XTMP_extract256BE16(src, addrOrOffset )			    \
    XTMP_extract32BE16(&((u32*)(src))[7-(((addrOrOffset)&0x1c)>>2)],addrOrOffset)
#define XTMP_extract256BE32(src, addrOrOffset )	\
    (((u32*)(src))[7-(((addrOrOffset)&0x1c)>>2)])

#define XTMP_extract512BE8(src,addrOrOffset )			   \
    XTMP_extract32BE8(&((u32*)(src))[15-(((addrOrOffset)&0x3c)>>2)],addrOrOffset)
#define XTMP_extract512BE16(src, addrOrOffset )			    \
    XTMP_extract32BE16(&((u32*)(src))[15-(((addrOrOffset)&0x3c)>>2)],addrOrOffset)
#define XTMP_extract512BE32(src, addrOrOffset )	\
    (((u32*)(src))[15-(((addrOrOffset)&0x3c)>>2)])

#define XTMP_insert32LE8(dst,addrOrOffset, value )		\
    *((u32*)(dst)) = ((*((u32*)(dst)))&(~(0xff<<XTMP_bsLE(addrOrOffset)))) |	\
             (((value)&0xff)<<XTMP_bsLE(addrOrOffset))
#define XTMP_insert32LE16(dst,addrOrOffset, value )		\
    *((u32*)(dst)) = ((*((u32*)(dst)))&(~(0xffff<<XTMP_bsLE(addrOrOffset)))) |	\
             (((value)&0xffff)<<XTMP_bsLE(addrOrOffset))
#define XTMP_insert32LE24(dst,addrOrOffset, value )		\
    *((u32*)(dst)) = ((*((u32*)(dst)))&(~(0xffffff<<XTMP_bsLE(addrOrOffset)))) |	\
             (((value)&0xffffff)<<XTMP_bsLE(addrOrOffset))
#define XTMP_insert32LE32(dst,addrOrOffset, value )		\
    *((u32*)(dst)) = (value)

#define XTMP_insert64LE8(dst,addrOrOffset, value )			      \
     XTMP_insert32LE8(&((u32*)(dst))[((addrOrOffset)&4)>>2],(addrOrOffset),value)
#define XTMP_insert64LE16(dst,addrOrOffset, value )		       \
     XTMP_insert32LE16(&((u32*)(dst))[((addrOrOffset)&4)>>2],(addrOrOffset),value)
#define XTMP_insert64LE24(dst,addrOrOffset, value )		       \
     XTMP_insert32LE24(&((u32*)(dst))[((addrOrOffset)&4)>>2],(addrOrOffset),value)
#define XTMP_insert64LE32(dst,addrOrOffset, value )	\
     ((u32*)(dst))[((addrOrOffset)&4)>>2] = value

#define XTMP_insert128LE8(dst,addrOrOffset, value )		       \
     XTMP_insert32LE8(&((u32*)(dst))[((addrOrOffset)&0xc)>>2],(addrOrOffset),value)
#define XTMP_insert128LE16(dst,addrOrOffset, value )			\
     XTMP_insert32LE16(&((u32*)(dst))[((addrOrOffset)&0xc)>>2],(addrOrOffset),value)
#define XTMP_insert128LE24(dst,addrOrOffset, value )			\
     XTMP_insert32LE24(&((u32*)(dst))[((addrOrOffset)&0xc)>>2],(addrOrOffset),value)
#define XTMP_insert128LE32(dst,addrOrOffset, value )	\
     ((u32*)(dst))[((addrOrOffset)&0xc)>>2] = value

#define XTMP_insert256LE8(dst,addrOrOffset, value )		       \
     XTMP_insert32LE8(&((u32*)(dst))[((addrOrOffset)&0x1c)>>2],(addrOrOffset),value)
#define XTMP_insert256LE16(dst,addrOrOffset, value )			\
     XTMP_insert32LE16(&((u32*)(dst))[((addrOrOffset)&0x1c)>>2],(addrOrOffset),value)
#define XTMP_insert256LE32(dst,addrOrOffset, value )	\
     ((u32*)(dst))[((addrOrOffset)&0x1c)>>2] = value

#define XTMP_insert512LE8(dst,addrOrOffset, value )		       \
     XTMP_insert32LE8(&((u32*)(dst))[((addrOrOffset)&0x3c)>>2],(addrOrOffset),value)
#define XTMP_insert512LE16(dst,addrOrOffset, value )			\
     XTMP_insert32LE16(&((u32*)(dst))[((addrOrOffset)&0x3c)>>2],(addrOrOffset),value)
#define XTMP_insert512LE32(dst,addrOrOffset, value )	\
     ((u32*)(dst))[((addrOrOffset)&0x3c)>>2] = value

#define XTMP_insert32BE8(dst,addrOrOffset, value )		\
    *((u32*)(dst)) = ((*((u32*)(dst)))&(~(0xff<<XTMP_bsBE(addrOrOffset)))) |	\
             (((value)&0xff)<<XTMP_bsBE(addrOrOffset))
#define XTMP_insert32BE16(dst,addrOrOffset, value )		\
    *((u32*)(dst)) = ((*((u32*)(dst)))&(~(0xffff0000>>XTMP_bsLE(addrOrOffset)))) |	\
             ((((value)&0xffff)<<16)>>XTMP_bsLE(addrOrOffset))
#define XTMP_insert32BE24(dst,addrOrOffset, value )		\
    *((u32*)(dst)) = ((*((u32*)(dst)))&(~(0xffffff00>>XTMP_bsLE(addrOrOffset)))) |	\
             ((((value)&0xffffff)<<8)>>XTMP_bsLE(addrOrOffset))
#define XTMP_insert32BE32(dst,addrOrOffset, value )		\
    *((u32*)(dst)) = (value)

#define XTMP_insert64BE8(dst,addrOrOffset, value )			  \
     XTMP_insert32BE8(&((u32*)(dst))[1-(((addrOrOffset)&4)>>2)],(addrOrOffset),value)
#define XTMP_insert64BE16(dst,addrOrOffset, value )		   \
     XTMP_insert32BE16(&((u32*)(dst))[1-(((addrOrOffset)&4)>>2)],(addrOrOffset),value)
#define XTMP_insert64BE24(dst,addrOrOffset, value )		   \
     XTMP_insert32BE24(&((u32*)(dst))[1-(((addrOrOffset)&4)>>2)],(addrOrOffset),value)
#define XTMP_insert64BE32(dst,addrOrOffset, value ) \
     ((u32*)(dst))[1-(((addrOrOffset)&4)>>2)] = value

#define XTMP_insert128BE8(dst,addrOrOffset, value )			   \
     XTMP_insert32BE8(&((u32*)(dst))[3-(((addrOrOffset)&0xc)>>2)],(addrOrOffset),value)
#define XTMP_insert128BE16(dst,addrOrOffset, value )			    \
     XTMP_insert32BE16(&((u32*)(dst))[3-(((addrOrOffset)&0xc)>>2)],(addrOrOffset),value)
#define XTMP_insert128BE24(dst,addrOrOffset, value )			    \
     XTMP_insert32BE24(&((u32*)(dst))[3-(((addrOrOffset)&0xc)>>2)],(addrOrOffset),value)
#define XTMP_insert128BE32(dst,addrOrOffset, value )	\
     ((u32*)(dst))[3-(((addrOrOffset)&0xc)>>2)] = value

#define XTMP_insert256BE8(dst,addrOrOffset, value )			   \
     XTMP_insert32BE8(&((u32*)(dst))[7-(((addrOrOffset)&0x1c)>>2)],(addrOrOffset),value)
#define XTMP_insert256BE16(dst,addrOrOffset, value )			    \
     XTMP_insert32BE16(&((u32*)(dst))[7-(((addrOrOffset)&0x1c)>>2)],(addrOrOffset),value)
#define XTMP_insert256BE32(dst,addrOrOffset, value )	\
     ((u32*)(dst))[7-(((addrOrOffset)&0x1c)>>2)] = value

#define XTMP_insert512BE8(dst,addrOrOffset, value )			   \
     XTMP_insert32BE8(&((u32*)(dst))[15-(((addrOrOffset)&0x3c)>>2)],(addrOrOffset),value)
#define XTMP_insert512BE16(dst,addrOrOffset, value )			    \
     XTMP_insert32BE16(&((u32*)(dst))[15-(((addrOrOffset)&0x3c)>>2)],(addrOrOffset),value)
#define XTMP_insert512BE32(dst,addrOrOffset, value )	\
     ((u32*)(dst))[15-(((addrOrOffset)&0x3c)>>2)] = value

/* DEPRECATED: General extraction functions */
EXTERN u32 XTMP_extract64LE16general( u32 *src, u32 addrOrOffset );
EXTERN u32 XTMP_extract64LE32general( u32 *src, u32 addrOrOffset );
EXTERN u32 XTMP_extract128LE16general( u32 *src, u32 addrOrOffset );
EXTERN u32 XTMP_extract128LE32general( u32 *src, u32 addrOrOffset );
EXTERN u32 XTMP_extract64BE16general( u32 *src, u32 addrOrOffset );
EXTERN u32 XTMP_extract64BE32general( u32 *src, u32 addrOrOffset );
EXTERN u32 XTMP_extract128BE16general( u32 *src, u32 addrOrOffset );
EXTERN u32 XTMP_extract128BE32general( u32 *src, u32 addrOrOffset );


/* DEPRECATED: General insertion functions */
EXTERN void XTMP_insert64LE16general( u32 *dst, u32 addrOrOffset, u32 value );
EXTERN void XTMP_insert64LE32general( u32 *dst, u32 addrOrOffset, u32 value);
EXTERN void XTMP_insert128LE16general( u32 *dst, u32 addrOrOffset, u32 value );
EXTERN void XTMP_insert128LE32general( u32 *dst, u32 addrOrOffset, u32 value );
EXTERN void XTMP_insert64BE16general( u32 *dst, u32 addrOrOffset, u32 value );
EXTERN void XTMP_insert64BE32general( u32 *dst, u32 addrOrOffset, u32 value );
EXTERN void XTMP_insert128BE16general( u32 *dst, u32 addrOrOffset, u32 value );
EXTERN void XTMP_insert128BE32general( u32 *dst, u32 addrOrOffset, u32 value );


/* 
// Insertion and extraction functions with parameters
// for size, buffer width and buffer endianness.
// These are not as efficient as the macros for a known width,
// buffer size and endianness.
*/

/* 
// Extract aligned u8, u16, or u32 from a source buffer
// numBytes must be 1, 2, or 4
// srcByteWidth must be 4, 8, or 16
// addrOrOffset must be aligned to numBytes
*/
EXTERN u32 XTMP_extract(const u32 *src, u32 addrOrOffset,
			u32 numBytes, u32 srcByteWidth, bool isBigEndian);

/* 
// Insert aligned u8, u16, or u32 into a destination buffer
// numBytes must be 1, 2, or 4
// dstByteWidth must be 4, 8, or 16
// addrOrOffset must be aligned to numBytes
*/
EXTERN void XTMP_insert(u32 *dst, u32 addrOrOffset, u32 value,
			u32 numBytes, u32 dstByteWidth, bool isBigEndian);

/*
// Determine whether the byte corresponding to addrOrOffset is enabled,
// for a memory access on an interface with the given width and endianness.
// The byteEnables value comes from calling XTMP_xferGetByteEnables().
*/
EXTERN bool XTMP_isByteEnabled(u64 byteEnables, u32 addrOrOffset,
			       u32 byteWidth, bool isBigEndian);


/*
// DEVELOPER's note
// ----------------
// Version Numbers are used to ensured that the driver uses
// a version XTMP_driverTable compatible with this release of XTMP.
// So you must change XTMP_VERSION if you make an incompatible change
// to XTMP_driverTable.
//
// none:     pre-T1040.2
// 20020504: T1040.2
// 20030320: Xtensa Tools 6.0
// 20061015: Xtensa Tools 7.0
*/
#define XTMP_VERSION 20061015

typedef struct _XTMP_driverTable 
{
  unsigned version;
  XTMP_event preAdvanceTimeEvent, postAdvanceTimeEvent;
  void (*initialize)(void);
  void *(*threadNew)( 
		     const char *name,
		     XTMP_threadCallBack callBack,
		     XTMP_threadInfo *ti
		     );

  void *(*cpuThreadNew)(
			const char *name,
			XTMP_time (*stepCPU)(XTMP_threadInfo *),
			void (*syncCPU)(XTMP_threadInfo *,XTMP_timeDelta),
			XTMP_threadInfo *ti
			);
  void (*start) (int ncycles);
  void (*stop) (void);
  void (*wait) (unsigned ncycles);
  XTMP_event (*eventNew)(void);
  void (*eventFree)(XTMP_event event);
  void (*waitOnEvent)(XTMP_event event);
  void (*fireEvent)(XTMP_event event);
  bool (*hasEventFired)(XTMP_event event);
  XTMP_time (*time)(void);
  u32 (*numberOfThreads)(void);
} XTMP_driverTable;

EXTERN int XTMP_main(int argc, char **argv);
EXTERN void XTMP_initialize(XTMP_driverTable *);
EXTERN void XTMP_cleanup(void);

EXTERN void XTMP_startOfCycleProcessing(void);
EXTERN void XTMP_endOfCycleProcessing(void);
EXTERN XTMP_event XTMP_getSchedulerEvent(void);







/*
/////////////////////////////////////////////////////////////
// The following interfaces are for internal or future use //
// and may change without notice.                          //
/////////////////////////////////////////////////////////////
*/

/**
 * This function returns the number of committed store-buffer entries
 * in a load/store unit of a core running in cycle-accurate simulation mode.
 *
 * @param   core        The core of interest.
 *
 * @param   lsUnit      Zero-based index of the load/store unit of interest.
 */
EXTERN unsigned XTMP_getStoreBufferCount(XTMP_core core, unsigned lsUnit);

#define XTMP_SB_PENDING      0U
#define XTMP_SB_DISPATCHED   1U
#define XTMP_SB_DISPATCHING  2U

/**
 * This function retrieves the information about a store-buffer entry
 * specified by a triplet (core, lsUnit, sbIndex). The information is
 * returned through output parameters sbState, address, byteEnable,
 * and dramAttribute.
 *
 * @param   core        The core of interest.
 *
 * @param   lsUnit      Zero-based index of the load/store unit of interest.
 *
 * @param   sbIndex     Zero-based index of the store-buffer entry of interest.
 *                      Must be less than XTMP_getStoreBufferCount(core,lsUnit);
 *
 * @param   sbState     If non-NULL, it is set to indicate the state of the
 *                      specified store-buffer entry: 
 *                      - XTMP_SB_PENDING
 *                      - XTMP_SB_DISPATCHING
 *                      - XTMP_SB_DISPATCHED
 *
 * @param   address     If non-NULL, it is set to indicate the address for
 *                      the specified store-buffer entry. The address is
 *                      aligned to the load/store access width.
 *
 * @param   byteEnable  If non-NULL, it is set to indicate the byte enables for
 *                      the specified store-buffer entry.
 *
 * @param   dramAttribute If non-NULL, it is set to indicate the DRamWrAttr
 *                        interface value for the specified store-buffer entry.
 *
 * @returns true on success, false on failure. 
 *          Output parameters are set only when the return value is true.
 */
EXTERN bool XTMP_getStoreBufferInfo(XTMP_core core,
                                    unsigned lsUnit,
                                    unsigned sbIndex,
                                    unsigned *sbState,
                                    XTMP_address *address,
                                    u32 *byteEnable,
                                    u32 *dramAttribute);

/**
 * This function retrieves the information about a store operation in
 * the pipeline specified by a triplet (core, lsUnit, stageNumber).
 * The information is returned through output parameters address, byteEnable,
 * and dramAttribute.
 *
 * @param   core        The core of interest.
 *
 * @param   lsUnit      Zero-based index of the load/store unit of interest.
 *
 * @param   stageNumber The number of the pipeline stage of interest.
 *                      Must be 1 (E), 2 (L), or 3 (M) for the 7-stage pipe;
 *
 * @param   address     If non-NULL, it is set to indicate the address for
 *                      the specified pipeline store operation. The address
 *                      is aligned to the load/store access width.
 *
 * @param   byteEnable  If non-NULL, it is set to indicate the byte enables for
 *                      the specified pipeline store operation.
 *
 * @param   dramAttribute If non-NULL, it is set to indicate the DRamWrAttr
 *                        interface value for the specified pipeline store
 *                        operation.
 *
 * @returns true if a valid store operation is found, false otherwise. 
 *          Output parameters are set only when the return value is true.
 */
EXTERN bool XTMP_getStageStoreInfo(XTMP_core core,
                                   unsigned lsUnit,
                                   unsigned stageNumber,
                                   XTMP_address *address,
                                   u32 *byteEnable,
                                   u32 *dramAttrbute);



/*
// Allow core clock factor changes after the start of simulation.
*/
EXTERN void XTMP_enableDynamicClockFactors();


typedef bool (*XTMP_instructionTraceCallBack)(void *, unsigned, unsigned);

EXTERN void
XTMP_setInstructionTraceCallBack(XTMP_core core,
                                 XTMP_instructionTraceCallBack callBack,
                                 void *callBackArg);

typedef bool (*XTMP_branchTraceCallBack)(void *, unsigned, unsigned, bool, bool);

EXTERN void
XTMP_setBranchTraceCallBack(XTMP_core core,
                            XTMP_branchTraceCallBack callBack,
                            void *callBackArg);

EXTERN void
XTMP_setExternalWatchpoint(XTMP_core core,
                           XTMP_address vaddr, /* watched region should not */
                           unsigned size, /* cross load-store width boundary */
                           unsigned accessType, /* 1=Write, 2=Read, 3=Access */
                           bool set); /* true to set, false to clear */

EXTERN void
XTMP_setExternalBreakpoint(XTMP_core core,
                           XTMP_address pc, /* breakpoint PC */
                           bool set);       /* true to set, false to clear */

/*
// Drive BootLoadStall signal for TX
*/
EXTERN void XTMP_setBootLoadStall(XTMP_core core, bool val);

typedef enum {
  XTMP_COHERENCE_NONE = 0, 
  XTMP_COHERENCE_SHARED = 1,
  XTMP_COHERENCE_EXCLUSIVE = 2,
  XTMP_COHERENCE_INVALID = 3
} XTMP_coherenceState;


/*
// Get or set the cache-coherence indication of the transaction.
*/
EXTERN XTMP_coherenceState XTMP_xferGetCoherenceState(XTMP_deviceXfer *xfer);
EXTERN void                XTMP_xferSetCoherenceState(XTMP_deviceXfer *xfer,
                                                      XTMP_coherenceState coh);
/*
// Get or set the virtual address index bits in cache-coherent transactions.
*/
EXTERN XTMP_address XTMP_xferGetVirtualAddressIndex(XTMP_deviceXfer *xfer);
EXTERN void         XTMP_xferSetVirtualAddressIndex(XTMP_deviceXfer *xfer,
                                                    XTMP_address vaddrIndex);
/*
// Get or set the indication that a snoop response is returning data.
*/
EXTERN bool XTMP_xferIsSnoopResponseWithData(XTMP_deviceXfer *xfer);
EXTERN void XTMP_xferSetSnoopResponseWithData(XTMP_deviceXfer *xfer, bool v);

/* 
// Cache-coherent peek and poke callbacks
*/
typedef XTMP_status
(*XTMP_coherentPeekCallBack)(void *deviceData,
                             u32 *dst,
                             XTMP_address vaddr,
                             XTMP_address paddr,
                             u32 size);
typedef XTMP_status
(*XTMP_coherentPokeCallBack)(void *deviceData,
                             XTMP_address vaddr,
                             XTMP_address paddr,
                             u32 size,
                             const u32 *src);
/*
// Register cache-coherent peek and poke callbacks.
*/
EXTERN void
XTMP_setCoherentPeekPokeCallBacks(XTMP_device device,
                                  XTMP_coherentPeekCallBack ccPeek,
                                  XTMP_coherentPokeCallBack ccPoke);
/*
// Cache-coherent peek and poke functions
*/
EXTERN XTMP_status
XTMP_peekCoherent(XTMP_component comp,
                  u32 *dst,
                  XTMP_address vaddr,
                  XTMP_address paddr,
                  u32 size);
EXTERN XTMP_status
XTMP_pokeCoherent(XTMP_component comp,
                  XTMP_address vaddr,
                  XTMP_address paddr,
                  u32 size,
                  const u32 *src);
/*
// Create a cache-coherence controller and connect the specified cores
// and the shared-memory device to it.
*/
EXTERN XTMP_device
XTMP_coherenceControllerNew(const char *name,
                            unsigned numCores,
                            XTMP_core cores[],
                            XTMP_component pifDevice);
/*
// Create an interrupt distributor for routing interrupts among cores.
*/
EXTERN void
XTMP_interruptDistributorNew(const char *name,
                             unsigned numCores,
                             XTMP_core cores[],
                             unsigned intraClusterInts,
                             unsigned sharedExternalInts);

/*
// Generate .siminfo file used to communicated information to Xtensa Xplorer.
// You may need this only if you are not calling XTMP_stepSystem.
*/
EXTERN void XTMP_updateSiminfo();

/*
// Access to Xtensa configuration options.
*/
EXTERN bool XTMP_getBooleanOption(XTMP_core core, const char *option);
EXTERN u32  XTMP_getIntegerOption(XTMP_core core, const char *option);

/*
// Access to summary event counts.
*/
EXTERN u64 XTMP_getSummaryCount(XTMP_core core, const char *what);

/* 
// Virtual -> Physical address translation
// ---------------------------------------
// Peek DTLB/ITLB to translate a virtual address to a physical address.
// If the core does not have DTLB/ITLB, then hit is set to true and
// vaddr is returned. If the address is not in the DTLB/ITLB, then
// hit is set to false and vaddr is returned. Otherwise, hit is set
// to true and the translated (physical) address will be returned.
*/
EXTERN XTMP_address
XTMP_peekDTLB(XTMP_core core, XTMP_address vaddr, bool *hit);
EXTERN XTMP_address 
XTMP_peekITLB(XTMP_core core, XTMP_address vaddr, bool *hit);

/*
// Debugging trace functions. 
*/
EXTERN void XTMP_setDebugTraceFile(XTMP_core core, const char *fileName);
EXTERN void XTMP_setDebugTraceLevel(XTMP_core core, unsigned level);
















/*
///////////////////////////////////////////////////////////////////////
// The following interfaces are NOT supported by the current version //
// of Xtensa Tools. They are reserved for internal use only.         //
///////////////////////////////////////////////////////////////////////
*/

/// Text logger handle
typedef void *XTMP_logTextLogger;

/// Generic (text or binary) logger handle
typedef void *XTMP_logLogger;


/// Log levels.
typedef enum {
  XTMP_LOG_OFF_LOG_LEVEL         = 60000,
  XTMP_LOG_FATAL_LOG_LEVEL       = 50000,
  XTMP_LOG_ERROR_LOG_LEVEL       = 40000,
  XTMP_LOG_WARN_LOG_LEVEL        = 30000,
  XTMP_LOG_NOTE_LOG_LEVEL        = 25000,
  XTMP_LOG_INFO_LOG_LEVEL        = 20000,
  XTMP_LOG_VERBOSE_LOG_LEVEL     = 15000,
  XTMP_LOG_DEBUG_LOG_LEVEL       = 10000,
  XTMP_LOG_TRACE_LOG_LEVEL       = 0,
  XTMP_LOG_ALL_LOG_LEVEL         = XTMP_LOG_TRACE_LOG_LEVEL
} XTMP_logLevel;


/**
 * C-function to configure (or re-configure) the text logger hierarchy.
 *
 * @param textEnabled           If true, text logging is enabled.  If false,
 *                              text logging is disabled.
 *
 * @param textFileName          Name of configuration file to use to configure
 *                              the text logger hierarachy.  If null or empty,
 *                              the text logger hierarchy is configured to send
 *                              logging events at LOG_LEVEL_NOTE or higher to
 *                              the console and to discard other events.
 *
 * @return true if function succeeded, false if function failed.
 */
EXTERN bool XTMP_logConfigure(bool        textEnabled,
                              const char *textFileName);



/**
 * C-function to get a handle to the named text logger.
 *
 * @param       name            The name of the text logger whose handle is
 *                              desired.
 */
EXTERN XTMP_logTextLogger XTMP_logGetTextLogger(const char *name);



/**
 * C-function to set the log level of the named logger.
 *
 * @param       logger          A handle to the text or binary logger whose
 *                              log level is to be set.
 *
 * @param       logLevel        The new log level.
 *
 * @return true if function succeeded, false if function failed.
 */
EXTERN bool XTMP_logSetLogLevel(XTMP_logLogger logger, XTMP_logLevel logLevel);



/**
 * C-function to get the log level of the named logger.
 *
 * @param       logger          A handle to the text or binary logger whose
 *                              log level is desired.
 *
 * @return the log level of the specified text or binary logger.
 */
EXTERN int XTMP_logGetLogLevel(XTMP_logLogger logger);



/**
 * C-function to determined if the specified text or binary logger is enabled
 * for the specified log level.
 *
 * @param       logger          A handle to a text or binary logger.
 *
 * @param       logLevel        The log level to compare against the logger's
 *                              log level.
 *
 * @return true if the logger is enabled for logging at logLevel, otherwise
 *         false.
 */
EXTERN bool XTMP_logIsEnabledFor(XTMP_logLogger logger, XTMP_logLevel logLevel);



/**
 * C-function to conditionally generate a logging event for the specified text
 * logger if the logger's log level is less then or equal to the specified log
 * level.
 *
 * @param       logger          A handle to a text logger.
 *
 * @param       logLevel        The log level to compare against the logger's
 *                              log level.
 *
 * @param       format          A c-string specifying the format of the text
 *                              logging event. The format of format is the same
 *                              as printf() in the standard C library.
 *
 * @param       ...             A variable number of parameters as specified in
 *                              format to be written to the logging event.
 *
 * @return true if function succeeded, false if function failed.
 */
EXTERN bool XTMP_logText(XTMP_logTextLogger     logger,
                         XTMP_logLevel          logLevel,
                         const char            *format,
                         ...);


/**
 * C-function to determine if text logging is enabled.
 */
EXTERN bool XTMP_logIsTextLoggingEnabled(void);


/**
 * C-function to set whether text logging is enabled.
 */
EXTERN bool XTMP_logSetTextLoggingEnabled(bool enable);


#endif
