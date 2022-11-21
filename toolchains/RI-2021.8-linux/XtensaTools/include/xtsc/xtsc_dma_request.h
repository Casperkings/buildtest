#ifndef _XTSC_DMA_REQUEST_H_
#define _XTSC_DMA_REQUEST_H_

// Copyright (c) 2005-2017 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Cadence Design Systems, Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Cadence Design Systems, Inc.

/**
 * @file
 *
 *  Structures for defining DMA Programming Registers.
 *
 *  This file is suitable for including in both host simulator code (XTSC) and Xtensa
 *  target code.
 */


#if !defined(XTSC_COMP_API)
#define XTSC_COMP_API 
#endif

#ifndef __XTENSA__
namespace xtsc_component {
#endif


/**
 * This struct is plain old data (POD) used to define a DMA request.
 *
 * The complete DMA is specified by one xtsc_dma_request register set and one or more
 * xtsc_dma_descriptor register sets (as specified by the num_descriptors register).
 * Each entry in this struct defines the value that should be written into its
 * corresponding DMA control register at the DMA registers base address plus the
 * BYTE_OFFSET shown.
 *
 * Write a 32-bit value between 1 and 255 to the num_descriptors register to start the
 * DMA.
 *
 * The DMA control register base address is defined by the xtsc_dma_engine_parms
 * parameter "reg_base_address".
 *
 * Note: The num_descriptors register should be written last because the DMA engine will
 *       start running as soon as the num_descriptors register is written with a non-zero
 *       value.
 *
 * Note: The num_descriptors register should be written using a 32-bit write and the
 *       value written should be between 1 and 255.  The DMA engine detects big or
 *       little endian based on the location in the 32-bit value of the non-zero byte.
 *
 * Note: notify_address8 must be aligned to the PIF bus width.
 *
 * Note: notify_address8 must be physical (not virtual).
 *
 * Note: If desired, target code can wait for turboxim_event_id using the
 *       xt_iss_event_wait() method - see xtsc::xtsc_fire_turboxim_event_id().
 *
 * Note: When xtsc_dma_engine_parms "start_at_index_1" is true (the default), the DMA
 *       engine starts a DMA request at descriptor index 1 regardless of the value in
 *       the done_descriptor field.  If "start_at_index_1" is set false then the DMA
 *       engine will use done_descriptor+1 as the index of the first descriptor to
 *       process (if done_descriptor contains either 0 or 255, then the first descriptor
 *       index to be processed with be 1).
 *
 * Note: As each descriptor is completed, the DMA engine writes its index to the
 *       done_descriptor field (this occurs regardless of the xtsc_dma_engine_parms
 *       "start_at_index_1" setting).  A descriptor may be safely reprogrammed for a 
 *       future DMA once its index is written by the DMA engine to the done_descriptor
 *       field.
 *
 * @see xtsc_dma_descriptor
 * @see xtsc_dma_engine
 * @see xtsc::xtsc_fire_turboxim_event_id();
 */
struct XTSC_COMP_API xtsc_dma_request {
  unsigned int  num_descriptors;        ///< Number of dma descriptors in this dma request.            BYTE_OFFSET=0x00.
  unsigned int  notify_address8;        ///< Address to write to to signal dma request is done.        BYTE_OFFSET=0x04.
  unsigned int  notify_value;           ///< Value to write to notify_address8.                        BYTE_OFFSET=0x08.
  unsigned int  turboxim_event_id;      ///< Optional TurboXim event ID to be notified (if non-zero).  BYTE_OFFSET=0x0C.
  unsigned int  done_descriptor;        ///< The index of the last completed descriptor.               BYTE_OFFSET=0x10.
};

typedef struct xtsc_dma_request xtsc_dma_request;



/**
 * This struct is plain old data (POD) used to define each descriptor of a DMA request.
 *
 * DMA descriptors are numbered using index N where N ranges from 1 to num_descriptors.
 *
 * Each entry in this struct defines the value that should be written into its
 * corresponding DMA control register at the DMA registers base address plus the
 * BYTE_OFFSET shown.
 *
 * The DMA control register base address is defined by the xtsc_dma_engine_parms
 * parameter "reg_base_address".
 *
 * Note: If num_transfers is 1 then single READ|WRITE PIF requests are used to perform
 *       the DMA.  If num_transfers is 2|4|8|16 then BLOCK_READ|BLOCK_WRITE PIF requests
 *       are used to perform the DMA.
 *
 * Note: source_address8, destination_address8, and size8 must each be evenly divisible
 *       by num_transfers * (PIF bus width).
 *
 * Note: source_address8 and destination_address8 must be physical (not virtual).
 *
 * Note: Each byte address in a sequence of num_tranfers * (PIF bus width) byte
 *       addresses starting with source_address8 and ending with source_address8+size8-1
 *       must map to the same source device.
 *
 * Note: Each byte address in a sequence of num_tranfers * (PIF bus width) byte
 *       addresses starting with destination_address8 and ending with
 *       destination_address8+size8-1 must map to the same destination device.
 *
 * @see xtsc_dma_request
 * @see xtsc_dma_engine
 */
struct XTSC_COMP_API xtsc_dma_descriptor {
  unsigned int  source_address8;        ///< Source starting byte address.                             BYTE_OFFSET=0x100*N+0x00.
  unsigned int  destination_address8;   ///< Destination starting byte address.                        BYTE_OFFSET=0x100*N+0x04.
  unsigned int  size8;                  ///< Total number of bytes to be transferred.                  BYTE_OFFSET=0x100*N+0x08.
  unsigned int  num_transfers;          ///< Number of transfers in each request (1|2|4|8|16).         BYTE_OFFSET=0x100*N+0x0c.
};

typedef struct xtsc_dma_descriptor xtsc_dma_descriptor;


#ifndef __XTENSA__
}  // namespace xtsc_component
#endif

#endif  // _XTSC_DMA_REQUEST_H_
