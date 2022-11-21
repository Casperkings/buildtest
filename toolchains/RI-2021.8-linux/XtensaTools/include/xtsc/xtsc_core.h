#ifndef _XTSC_CORE_H_
#define _XTSC_CORE_H_

// Copyright (c) 2005-2017 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Cadence Design Systems, Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Cadence Design Systems, Inc.

/**
 * @file 
 */


#include <xtsc/xtsc.h>
#include <xtsc/xtsc_parms.h>
#include <xtsc/xtsc_request_if.h>
#include <xtsc/xtsc_respond_if.h>
#include <xtsc/xtsc_tx_xfer_if.h>
#include <xtsc/xtsc_queue_push_if.h>
#include <xtsc/xtsc_queue_pop_if.h>
#include <xtsc/xtsc_wire_write_if.h>
#include <xtsc/xtsc_wire_read_if.h>
#include <xtsc/xtsc_ext_regfile_if.h>
#include <xtsc/xtsc_lookup_if.h>
#include <string>
#include <set>
#include <vector>
#include <cstring>


namespace xtsc {


class xtsc_core_intf;
class xtsc_core_parts;
class xtsc_tie_lookup_driver;
class xtsc_output_queue_driver;
class xtsc_input_queue_driver;
class xtsc_interrupt_distributor;
class xtsc_tx_loader;




/**
 * Constructor parameters for a xtsc_core object.
 *
 * This class contains the parameters needed to construct an xtsc_core object.
 * When the xtsc_core_parms constructor is called, it populates the xtsc_core_parms
 * object with the three parameters passed in to it (i.e. "XTENSA_CORE, "XTENSA_SYSTEM",
 * and "XTENSA_PARAMS) and with numerous additional Xtensa parameters obtained from the 
 * core's configuration database in the Xtensa registry.  All the names and values of
 * these additional parameters can be seen using the xtsc_parms::dump() method.  For
 * example:
 * \verbatim
     xtsc_core_parms core_parms("le_32");
     core_parms.dump();
   \endverbatim
 * Most of the parameters in the core's configuration database are read-only in XTSC.
 * That is, the only way to change them for an XTSC simulation is by building a new 
 * core configuration.  Here is an alphabetical list of read-only parameters:
 * 
 *  \verbatim
   Name                         Type    Description
   ------------------           ----    -----------------------------------------------
  

   "AltResetVec"                u32     Initial value of the AltResetVec input.
                                        Default value is same as "StaticVectorBase1".

   "APBMasterBaseAddress"       u32     The base address served by the APB master port.

   "APBMasterByteSize"          u32     The aperture size served by the APB master port.
                                        Zero if the config has no APB master port.

   "BInterruptMap"              u32     This value expresses the mapping between the
                                        individual interrupt lines in the BInterrupt
                                        input and their corresponding bit position in
                                        the INTENABLE and INTERRUPT registers.  When
                                        expressed in binary and read from lsb to msb
                                        each bit with a value of 1 corresponds to an
                                        interrupt line in the BInterrupt input vector.
                                        The first 1 bit encountered in the
                                        "BInterruptMap" value defines the interrupt
                                        number (index into INTENABLE and INTERRUPT) of
                                        BInterrupt[0].  The second 1 bit encountered
                                        defines the interrupt number of BInterrupt[1],
                                        etc.  For example, a "BInterruptMap" value of
                                        0x1a8448b0 has the binary value shown below and
                                        defines the mapping shown.

                            3         2         1
                           10987654321098765432109876543210 (INTENABLE/INTERRUPT index)
          0x1a8448b0 = 32'b00011010100001000100100010110000
                              98 7 6    5   4  3   2 10     (BInterupt index)

             BInterrupt[0] <=> INTENABLE/INTERRUPT[4]
             BInterrupt[1] <=> INTENABLE/INTERRUPT[5]
             BInterrupt[2] <=> INTENABLE/INTERRUPT[7]
             BInterrupt[3] <=> INTENABLE/INTERRUPT[11]
             BInterrupt[4] <=> INTENABLE/INTERRUPT[14]
             BInterrupt[5] <=> INTENABLE/INTERRUPT[18]
             BInterrupt[6] <=> INTENABLE/INTERRUPT[23]
             BInterrupt[7] <=> INTENABLE/INTERRUPT[25]
             BInterrupt[8] <=> INTENABLE/INTERRUPT[27]
             BInterrupt[9] <=> INTENABLE/INTERRUPT[28]

                                        The mapping from BInterrupt index to interrupt
                                        number can also be obtained from
                                        xtsc_core::get_interrupt_number().
                                        The mapping from interrupt number to BInterrupt
                                        index can also be obtained from
                                        xtsc_core::get_binterrupt_index().
                                        Note: This parameter only shows the first 32
                                        interrupts for NX configs with more than 32
                                        interrupts.  See "InterruptTypes".

   "BInterruptSize"             u32     The number of bits in the BInterrupt input.

   "BootLoader"                 bool    True if config is a TX Xtensa core with the
                                        BootLoader interface.

   "CombineMasterAXIPorts"      bool    True if AXI master instruction and data ports
                                        are combined.

   "CombineSlaveAXIPorts"       bool    True if AXI slave instruction and data ports
                                        are combined.

   "DataCacheIsWriteback"       bool    True if the data cache is a write-back cache.

   "DataCacheIsCoherent"        bool    True if the config includes the coherent data
                                        cache option.

   "DataErrorWordWidth"         u32     Granularity of ECC/parity error correction in
                                        bytes.  Parity adds 1 bit of error correction
                                        per granule.  ECC adds 5 bits per granule when
                                        granularity is 1 byte and 7 bits per granule
                                        when granularity is 4 bytes.

   "DataMasterByteWidth"        u32     Width in bytes of DataMaster interface.

   "DataMasterType"             u32     DataMaster interface type.  Encoding:
                                        0 = None
                                        1 = Plain AXI4
                                        2 = ACE-Lite
                                        3 = Full ACE

   "DataRAM0BaseAddress"        u32     Byte base address of DRAM0.

   "DataRAM0ByteSize"           u32     Size in bytes of DRAM0.

   "DataRAM0HasBusy"            bool    True if DRAM0 has a busy interface.

   "DataRAM0HasECC"             bool    True if DRAM0 has error correction.

   "DataRAM0HasInbound"         bool    True if DRAM0 supports inbound PIF access.

   "DataRAM0HasParity"          bool    True if DRAM0 has parity checking.

   "DataRAM0HasRCW"             bool    True if architecture supports S32C1I to DRAM0.

   "DataRAM1BaseAddress"        u32     Byte base address of DRAM1.

   "DataRAM1ByteSize"           u32     Size in bytes of DRAM1.

   "DataRAM1HasBusy"            bool    True if DRAM1 has a busy interface.

   "DataRAM1HasECC"             bool    True if DRAM1 has error correction.

   "DataRAM1HasInbound"         bool    True if DRAM1 supports inbound PIF access.

   "DataRAM1HasParity"          bool    True if DRAM1 has parity checking.

   "DataRAM1HasRCW"             bool    True if architecture supports S32C1I to DRAM1.

   "DataRAMBanks"               u32     Number of banks in DataRAM0/DataRAM1 (0|1|2|4).
                                        If "DataRAMCount" is 0, then "DataRAMBanks" will
                                        be 0. From the programmer's view (32-bit address
                                        space), banks are addressed using the
                                        lowest-order address bits above the byte-lane
                                        bits.

   "DataRAMSubBanks"            u32     Number of subbanks in each DataRAM0/DataRAM1
                                        bank (0-16).  If "DataRAMCount" is 0, then
                                        "DataRAMSubBanks" will be 0. From the
                                        programmer's view (32-bit address space),
                                        subbanks are addressed using from 1-4 of the
                                        high-order byte-lane bits.

   "DataRAMAttributeWidth"      u32     Bit width of DRAM0/DRAM1 attribute interfaces.

   "DataRAMCount"               u32     Number of data RAMs (0|1|2).

   "DataRAMHasSplitRW"          bool    True if config has split read/write ports for
                                        DRAM0/DRAM1.  False if config has unified
                                        read/write ports for DRAM0/DRAM1 or if
                                        "DataRAMCount" is zero.

   "DataROM0BaseAddress"        u32     Byte base address of DROM0.

   "DataROM0ByteSize"           u32     Size in bytes of DROM0.

   "DataROM0HasBusy"            bool    True if DROM0 has a busy interface.

   "DataROM0HasInbound"         bool    True if DROM0 supports inbound PIF access.

   "DataROMBanks"               u32     Number of banks in DataROM0 (0|1|2|4).
                                        If "DataROMCount" is 0, then "DataROMBanks" will
                                        be 0. From the programmer's view (32-bit address
                                        space), banks are addressed using the
                                        lowest-order address bits above the byte-lane
                                        bits.

   "DataROMCount"               u32     Number of data ROMs (0|1).

   "DataSlaveByteWidth"         u32     Width in bytes of DataSlave interface.

   "DataSlaveType"              u32     DataSlave interface type.  See "DataMasterType"
                                        for encoding.

   "HasAltResetVec"             bool    True if config has the AltResetVec input (from
                                        the "External Reset Vector" option).

   "HasArithmeticException"     bool    True if config as the PArithmeticException
                                        interface.

   "HasCBox"                    bool    True if config has the CBox option.

   "HasDataCache"               bool    True if config has a data cache.

   "HasHaltOnException"         bool    True if config has Halt Exception Architecture.

   "HasInboundPIF"              bool    True if config has inbound PIF.

   "HasInstCache"               bool    True if config has an instruction cache.

   "HasL2CC"                    bool    True if config has an L2CC.

   "HasPIF"                     bool    True if config has a PIF.

   "HasPIFReqAttribute"         bool    True if config has PIF with the PIF Attribute
                                        interface.

   "HasPIFReqDomain"            bool    True if config has PIF with the PIF Request
                                        Domain interface.

   "HasPIFWriteResponse"        bool    True if the config has PIF write responses
                                        configured.  Note: xtsc_core always requires
                                        write responses regardless of the value of this
                                        parameter.

   "HasPIFCriticalWordFirst"    bool    True if the config has PIF critical word first
                                        configured.

   "HasPIFArbitraryByteEnables" bool    True if the config has PIF arbitrary byte
                                        enables configured.

   "HasProcessorID"             bool    True if config has a processor ID.

   "HWMicroArchLatest"          u32     Hardware version number of this config.

   "iDMAByteWidth"              u32     Width in bytes of the PIF iDMA interface.

   "iDMAChannels"               u32     Number of iDMA channels (0|1|2).

   "iDMACount"                  u32     Number of iDMA ports (0|1).

   "InstallPrefix"              char*   The absolute path to the XtensaTools directory.

   "InstFetchByteWidth"         u32     Width in bytes of instruction fetch interface.

   "InstMasterByteWidth"        u32     Width in bytes of InstMaster interface.

   "InstMasterType"             u32     InstMaster interface type.  See "DataMasterType"
                                        for encoding.

   "InstRAM0BaseAddress"        u32     Byte base address of IRAM0.

   "InstRAM0ByteSize"           u32     Size in bytes of IRAM0.

   "InstRAM0HasBusy"            bool    True if IRAM0 has a busy interface.

   "InstRAM0HasECC"             bool    True if IRAM0 has error correction.

   "InstRAM0HasInbound"         bool    True if IRAM0 supports inbound PIF access.

   "InstRAM0HasParity"          bool    True if IRAM0 has parity checking.

   "InstRAM1BaseAddress"        u32     Byte base address of IRAM1.

   "InstRAM1ByteSize"           u32     Size in bytes of IRAM1.

   "InstRAM1HasBusy"            bool    True if IRAM1 has a busy interface.

   "InstRAM1HasECC"             bool    True if IRAM1 has error correction.

   "InstRAM1HasInbound"         bool    True if IRAM1 supports inbound PIF access.

   "InstRAM1HasParity"          bool    True if IRAM1 has parity checking.

   "InstRAMCount"               u32     Number of instruction RAMs (0|1|2).

   "InstROM0BaseAddress"        u32     Byte base address of IROM0.

   "InstROM0ByteSize"           u32     Size in bytes of IROM0.

   "InstROM0HasBusy"            bool    True if IROM0 has a busy interface.

   "InstROM0HasInbound"         bool    True if IROM0 supports inbound PIF access.

   "InstROMCount"               u32     Number of instruction ROMs (0|1).

   "InstSlaveByteWidth"         u32     Width in bytes of InstSlave interface.

   "InstSlaveType"              u32     InstSlave interface type.  See "DataMasterType"
                                        for encoding.

   "InterruptCount"             u32     The total number of interrupts (both internal
                                        and external).

   "InterruptExtEdgeMask"       u32     Each 1 bit in this mask indicates that the 
                                        corresponding bit in "BInterruptMap" represents
                                        a edge-triggered external interrupt.
                                        Note: This parameter only shows the first 32
                                        interrupts for NX configs with more than 32
                                        interrupts.  See "InterruptTypes".

   "InterruptExtLevelMask"      u32     Each 1 bit in this mask indicates that the 
                                        corresponding bit in "BInterruptMap" represents
                                        a level-sensitive external interrupt.
                                        Note: This parameter only shows the first 32
                                        interrupts for NX configs with more than 32
                                        interrupts.  See "InterruptTypes".

   "InterruptNMIMask"           u32     A 1 bit in this mask indicates that the 
                                        corresponding bit in "BInterruptMap" represents
                                        a non-maskable interrupt.

   "InterruptTypes"             char*   For NX configs, this parameter's value has 1
                                        byte for each of the "InterruptCount" number of
                                        interrupts.  
                                          Byte  Meaning
                                           E    Edge-triggered external interrupt
                                           L    Level-sensitive external interrupt
                                           S    Software interrupt
                                           T    Timer interrupt
                                           I    Internal interrupt
                                        This parameter's value is NULL for non-NX
                                        configs.

   "IsBigEndian"                bool    True if config is big endian.

   "IsLittleEndian"             bool    True if config is little endian.

   "IsNX"                       bool    True if this is an NX config.

   "IsNXUnifiedLS"              bool    True if this is an NX VisionQ7 config.

   "IsPreconfiguredCore"        bool    True if this is a preconfigured core.

   "L2CCByteSize"               u32     Size in bytes of L2CC internal memory.

   "L2CCDataAccessByteWidth"    u32     Width in bytes of L2CC data core interface.

   "L2CCInstAccessByteWidth"    u32     Width in bytes of L2CC inst core interface.

   "L2CCLineBytes"              u32     The size of L2CC cache line in bytes.

   "L2CCMasterAccessByteWidth"  u32     Width in bytes of L2CC master interface.

   "L2CCMemoryLatency"          u32     L2CC internal memory latency.

   "L2CCRegBaseAddress"         u32     Byte base address of L2CC registers.

   "L2CCSlaveAccessByteWidth"   u32     Width in bytes of L2CC slave interface.

   "L2CCSlaveCount"             u32     Number of L2CC AXI slave ports (0|1).

   "L2CCTCMBaseAddress"         u32     Byte base address of L2CC L2RAM.

   "L2CCWayCount"               u32     The number of L2CC cache ways.

   "L2CCIndexLock"              u32     Indicates if L2CC supports cache line locking.

   "L2CCErrorInterrupt"         u32     L2Err interrupt count (0|1).

   "L2CCPerfCounterCount"       u32     The number of L2CC performance counters.

   "L2CCSupportMulticore"       u32     Indicates if L2CC supports multicore configs.

   "LoadStoreByteWidth"         u32     Width in bytes of load/store interface.

   "LoadStoreUnitCount"         u32     Number of load/store units (1|2|3).

   "LocalMemoryLatency"         u32     Local memory latency (1|2|3).

   "MasterExclAccess"           bool    True if config supports exclusive transactions
                                        on PIF.

   "MaxInstructionSize"         u32     Maximum width in bytes of any instruction.

   "PIFByteWidth"               u32     Width in bytes of the PIF.  Also see
                                        "SystemByteWidth".

   "PIFInboundBufferEntries"    u32     The number of entries in the PIF inbound buffer.

   "RelocatableVectors"         bool    True if config has the relocatable vectors
                                        option.

   "ResetVectorOffset"          u32     If "RelocatableVectors" is true this is the
                                        reset vector offset to be added to
                                        "StaticVectorBase0" or "StaticVectorBase1"
                                        (depending upon "SimStaticVectorSelect" and
                                        "StaticVectorSelect").
                                        If "RelocatableVectors" is false this is the
                                        reset vector address.

   "SlaveExclAccess"            bool    True if config supports exclusive transactions
                                        on inbound PIF.

   "StaticVectorBase0"          u32     Static vector base 0.

   "StaticVectorBase1"          u32     Static vector base 1.

   "StaticVectorSelect"         u32     This read-only parameter shows the default value
                                        for the hardware configuration.  This value
                                        tells the tools (linker and ISS) what value to
                                        use by default:
                                        A value of 0 means to use "StaticVectorBase0".
                                        A value of 1 means to use "StaticVectorBase1".
                                        If "SimStaticVectorSelect" is 0xFFFFFFFF, the
                                        default, and assuming the "StatVectorSel" system
                                        input is not being driven and the
                                        xtsc_core::set_static_vector_select() method has
                                        not been called, then the ISS inside xtsc_core
                                        will use either "StaticVectorBase0" or
                                        "StaticVectorBase1" according to the the value
                                        shown in this read-only parameter.

   "SystemByteWidth"            u32     Width in bytes of the system interface (PIF|AXI).

   "SystemRAMBaseAddress"       u32     Byte base address of system RAM.

   "SystemRAMByteSize"          u32     Size in bytes of system RAM.

   "SystemROMBaseAddress"       u32     Byte base address of system ROM.

   "SystemROMByteSize"          u32     Size in bytes of system ROM.

   "UnifiedRAM0BaseAddress"     u32     Byte base address of URAM0.

   "UnifiedRAM0ByteSize"        u32     Size in bytes of URAM0.

   "UnifiedRAM0HasBusy"         bool    True if URAM0 has a busy interface.

   "UnifiedRAM0HasInbound"      bool    True if URAM0 supports inbound PIF access.

   "UnifiedRAMCount"            u32     Number of unitified RAMs (0|1).

   "XLMI0BaseAddress"           u32     Byte base address of XLMI0.

   "XLMI0ByteSize"              u32     Size in bytes of XLMI0.

   "XLMI0HasBusy"               bool    True if XLMI0 has a busy interface.

   "XLMI0HasInbound"            bool    True if XLMI0 supports inbound PIF access.

   "XLMICount"                  u32     Number of XLMIs (0|1).

   "XTENSA_CORE"                char*   The config name.

   "XTENSA_PARAMS"              char**  Pointer to a c-string array of size 1 or 2.
                                        If XTENSA_PARAMS was passed in to the 
                                        xtsc_core_parms constructor or defined in the
                                        environment, then the array is of size 2 and the
                                        first entry in the array is the XTENSA_PARAMS
                                        value and the second entry is NULL.  Otherwise,
                                        the array is of size 1 and the only entry is
                                        NULL.

   "XTENSA_SYSTEM"              char**  Pointer to a c-string array of size 2.  The 
                                        first entry in the array is the value of 
                                        XTENSA_SYSTEM either passed in to the 
                                        xtsc_core_parms constructor or obtained
                                        from the environment.  The second entry in the
                                        array is NULL.

   \endverbatim
 * The following parameters are obtained from the core's configuration database in
 * the Xtensa registry, but, assuming "IsPreconfiguredCore" is false and the feature
 * exists in the config, the parameter values can be changed in XTSC (using the
 * appropriate xtsc_parms::set() method) to allow expermentation.
 * For example:
 * \verbatim
     xtsc_core_parms core_parms("le_32");
     core_parms.set("DataCacheByteSize", 4096);
   \endverbatim
 * Warning:  Changing these values in XTSC is for experimentation purposes only 
 *           and will NOT change the hardware. 
 * 
 * Note:  Except for "ProcessorID" the following parameters are not writeable for 
 *        Diamond Standard processors.
 * 
 *  \verbatim
   Name                         Type    Description
   ------------------           ----    -----------------------------------------------
  
   "ProcessorID"                u32     The processor ID.  This defines the intial
                                        value of the PRID register.  After elaboration,
                                        the PRID register can be changed using the 
                                        "PRID" system-level input.  See the 
                                        get_system_input_wire method.  Also see the 
                                        setup_debug() method.

   "WriteBufferEntries"         u32     The number of write buffer entries.

   "DataCacheByteSize"          u32     The size of the data cache in bytes.

   "DataCacheLineByteSize"      u32     The size of a data cache line in bytes.

   "DataCacheWays"              u32     The number of data cache ways.

   "iDMAPIFBurst"               bool    If true, PIF BURST_READ and BURST_WRITE will be
                                        used for multi-transfer transactions on the PIF
                                        iDMA interface.  If false, then BLOCK_READ and
                                        BLOCK_WRITE will be used.
                                        Default = false.

   "InstCacheByteSize"          u32     The size of the instruction cache in bytes.

   "InstCacheLineByteSize"      u32     The size of an instruction cache line in bytes.

   "InstCacheWays"              u32     The number of instruction cache ways.

    \endverbatim
 * 
 * The following parameters are not obtained from the core's configuration database in
 * the Xtensa registry.  Most of the following parameters are used to control some
 * aspect of the simulation and are not related to hardware.
 *
 * 
 *  \verbatim
   Name                         Type    Description
   ---------------------------  ------  ------------------------------------------------
  
   "SimAllowResetStragglers"    bool    Starting in RG-2015.0, when xtsc_core is reset
                                        it discards all outstanding requests and if it
                                        later receives a response to a discarded request
                                        it will throw an exception.  If you are using
                                        simulation memory/interconnect models that do
                                        not fully support reset, then you may need to
                                        set this parameter to true to prevent xtsc_core
                                        from discarding these requests until the
                                        response comes (at which time both the request
                                        and response will be discarded).
                                        Note: Setting this parameter true does not apply
                                        to the iDMA port (if any).
                                        Default value = false.

   "SimBootLoaderArbitrationPhase" u32  The phase of the clock at which arbitration is
                                        performed between boot loader and Xtensa access
                                        to local memories.  This parameter is ignored
                                        unless "BootLoader" is true and it is only used
                                        when a local memory interface has busy.  It is
                                        expressed in terms of the SystemC time
                                        resolution (from sc_get_time_resolution()).  A
                                        value of 0xFFFFFFFF means to use a phase of
                                        one-half of this core's clock period minus one.
                                        This corresponds to arbitrating at one SystemC
                                        time resolution prior to negedge clock (which
                                        allows a potential external arbiter to operate
                                        at the typical time of negedge clock and still
                                        have both sets of arbitration occur in the same
                                        clock cycle).  This parameter will need to be
                                        adjusted if the boot loader drives the Xfer
                                        interface later then this in the clock cycle.
                                        The arbitration phase should be strictly greater
                                        then Phase B because the Xtensa accesses always
                                        occur at Phase B.  In addition, the arbitration
                                        phase must be strictly less than the core's
                                        clock period and may not fall on or between
                                        phase A and phase B.
                                        Default = 0xFFFFFFFF (just prior to negedge
                                                  clock).

   "SimBResetAll"               bool    Set this parameter to true to cause xtsc_reset()
                                        to be called (with an argument of true) when the
                                        BReset input goes high on this core.  This
                                        causes all XTSC components to be reset.
                                        Default = false.

   "SimClients"                 char**  Call xtsc_core::load_client() for each char* in
                                        the array except the last one (which must be
                                        NULL).

   "SimClientFile"              char*   Call xtsc_core::load_client_file() with the 
                                        named file.

   "SimClockFactor"             u32     The clock period of this core expressed in terms
                                        of the system clock period (available using the
                                        xtsc_get_system_clock_period() method).  A value
                                        of 1, the default, means this core's clock 
                                        period is equal to the system clock period.  A
                                        value of 2 means this core's clock period is
                                        twice as long as the system clock period.  And
                                        so on.
                                        See xtsc_core::set_clock_phase_delta_factors();
                                        Default value = 1.
                                        Note: Changing "SimClockFactor" can slow down
                                        simulation and should not be done except in
                                        the case of a multi-core system in which at
                                        least two different core frequencies are needed.
                                        If there is only one core or if all cores run at
                                        the same frequency then xtsc_initialize_parms
                                        "system_clock_factor" should be used to adjust
                                        frequency in lieu of "SimClockFactor".

   "SimDebug"                   bool    If true, automatically enable debugging on this
                                        core by calling the enable_debug() method with
                                        arguments obtained from the "SimDebugWait",
                                        "SimDebugSynchronized", and
                                        "SimDebugStartingPort" parameters.  If false, do
                                        not automatically call enable_debug() for this
                                        core (of course, you can still manually call
                                        enable_debug() or one of the other debug setup
                                        methods from your sc_main program).
                                        Default value = false.

   "SimDebugWait"               bool    If "SimDebug" is true, then this parameter
                                        specifies the value to pass in as the wait
                                        argument to the enable_debug() call.
                                        If "SimDebug" is false, then this parameter
                                        is ignored.
                                        Default value = true.

   "SimDebugSynchronized"       bool    If "SimDebug" is true, then this parameter
                                        specifies the value to pass in as the
                                        synchronized argument to the enable_debug()
                                        call.  If "SimDebug" is false, then this
                                        parameter is ignored.
                                        Default value = true (Verilog co-simulation)
                                        Default value = false (All others)

   "SimDebugStartingPort"       u32     If "SimDebug" is true, then this parameter
                                        specifies the value to pass in as the
                                        starting_port argument to the enable_debug()
                                        call.
                                        If "SimDebug" is false, then this parameter
                                        is ignored.
                                        Default value = 0.

   "SimDebugAsynchronousIPC"    bool    If false, then the communication between the
                                        target debugger (xt-gdb/Xplorer) and the ISS is
                                        always done synchronously (as part of stepping
                                        the ISS).  This means the SystemC clock must
                                        be ticking for any debugger-ISS communication to
                                        take place.  If true, then most debugger-ISS
                                        communication can take place even when the
                                        SystemC clock is stopped.  Some debugger
                                        commands cannot be handled asynchronously (e.g.
                                        the target program cannot be halted by the
                                        debugger unless the SystemC clock is ticking).
                                        See "SimDebugPollInterval".
                                        Default value = false (Verilog co-simulation)
                                        Default value = true (All others)

   "SimDebugPollInterval"       u32     This specifies the interval (in units of ISS
                                        clock cycles) between two consecutive checks for
                                        communication from the attached target debugger
                                        (xt-gdb/Xplorer).  A value of 0 means to use the
                                        ISS default value of 1000 cycles when 
                                        "SimDebugAsynchronousIPC" is true and to use a
                                        value of 1 when "SimDebugAsynchronousIPC" is
                                        false.
                                        Default value = 0 (that is, use the ISS default
                                        value of 1000 or a value of 1 depending on
                                        "SimDebugAsynchronousIPC").

   "SimDumpAllRegisters"        bool    If true, the dump_all_registers() method is
                                        called at the end of construction.
                                        Default = false.

   "SimFakeL2CC"                bool    If true and this is an L2CC config and no
                                        xtsc_l2cc is instantiated then xtsc_core will
                                        try to fake the presence of the L2CC status
                                        register enough to keep XTOS from polling
                                        endlessly.  This parameter has no effect on
                                        non-L2CC configs or if an xtsc_l2cc is
                                        instantiated in the system.
                                        Default = true.

   "SimExitLocation"            char*   The optional value to pass to the ISS using the
                                        --exit_location option.  The value should be a
                                        10 character string representing a 32-bit value
                                        in hexadecimal (for example, 0x60000000).  See
                                        the --exit_location command-line option in the
                                        ISS User's Guide.
                                        Default value = NULL (do not pass
                                        --exit_location to the ISS).

   "SimFullLocalMemAddress"     bool    If true, use the full local memory address in
                                        xtsc_request objects (this corresponds to a
                                        programmer's view).  If false, use a 0-based
                                        address for each local memory except XLMI (this
                                        corresponds to a hardware view).  This parameter
                                        does not apply to XLMI or PIF|AXI|iDMA which
                                        always get the full memory address.
                                        Default value = true.

   "SimLoadFile"                char*   Call xtsc_core::load_file() with the named file
                                        during the start_of_simulation() callback.  If
                                        desired, multiple files to be loaded can be
                                        specified by separating them with a comma.

   "SimLogDisassembly"          bool    If true, log each instruction as it commits at
                                        INFO_LOG_LEVEL.  Logging can also be turned on
                                        and off during simulation.
                                        See xtsc_core::log_disassembly().
                                        Default = false.

   "SimLogPeekPoke"             bool    If false, xtsc_core does not log its calls to
                                        the peek/poke functions of attached memories.
                                        Set this parameter to true to cause xtsc_core to
                                        log them at VERBOSE_LOG_LEVEL.
                                        Default = false.

   "SimLogIDMAChannel"          bool    If true, xtsc_core logs channel number for each
                                        iDMA transaction. Set this parameter to true to
                                        cause xtsc_core to log them at VERBOSE_LOG_LEVEL.
                                        Default = false.

   "SimMasterPeekPoke"          bool    If false, xtsc_core sends all nb_peek/nb_poke
                                        calls to PIF or AXI data master port. If true,
                                        a peek/poke call from iDMA master goes to iDMA
                                        master port. For AXI, a peek call from instruction
                                        fetch goes to AXI instruction master port.
                                        Default = false.

   "SimScriptFile"              char*   Optional name of an xtsc_script_file to provide
                                        Lua code including Lua function definitions in a
                                        #lua_beg/#lua_end block followed by Lua function
                                        declarations using the format shown below.
                                        Note: The Lua code can access the XTSC command
                                        facility using the xtsc.cmd() function; however,
                                        SystemC time consuming commands (e.g. "sc wait"
                                        and "sc waitall") must not be used.
                                        Default = NULL
    Format:
        #lua_beg
        -- Lua code goes here
        #lua_end
        lua_function simcall    <LuaSimcallFunction>
        lua_function breakpoint <LuaBreakpointFunction>
    Where:
        <LuaSimcallFunction> is the name of a Lua function defined in the #lua_beg/
            #lua_end block which takes 7 integer arguments (the core's zero-based
            instantiation number followed by the 6 simcall arguments) and returns 1
            integer value.
        <LuaBreakpointFunction> is the name of a Lua function defined in the #lua_beg/
            #lua_end block which takes 2 integer arguments (the core's zero-based
            instantiation number followed by the current PC) and returns the value 0.

   "SimMonitorInterfaces"       char**  A NULL-terminated array of TIE and system I/O
                                        interface names or name patterns that should
                                        have an event notified whenever there is
                                        activity on that interface.  An interface
                                        pattern has one or more asterisks and matching
                                        is performed using the xtsc_pattern_match() API.
                                        Once enabled, the event name or names can be
                                        obtained at an XTSC command prompt like this:
                                           cmd: xtsc xtsc_event_dump
                                        Default value = NULL.

   "SimNameForISS"              char*   The ISS instance name is used to identify the
                                        core instance in Xtensa Xplorer for debugging
                                        and profiling.  By default, the ISS instance
                                        name is formed from the xtsc_core instance base
                                        name (that is, without hierarchy) plus a period
                                        and the zero-based instantiation number.
                                        Although typically not required, this parameter
                                        can be used to specify a different ISS instance
                                        name; however, this is not recommended and
                                        should only be done with caution because
                                        duplicate or illegal ISS instance names can
                                        cause Xtensa Xplorer debugging and profiling
                                        to fail.
                                        Note:  ISS instance names must be legal file
                                        system names.
                                        Default value = NULL (use default ISS name).

   "SimNeverExits"              bool    All XTSC simulations that include at least one
                                        core have a SystemC thread that waits for all
                                        cores to exit and then checks a flag to
                                        determine if sc_stop() should be called.  If
                                        this parameter is set to true, then this core
                                        will NOT be considered when notifing the "all
                                        cores have exited" event.
                                        See xtsc_core::get_all_cores_exited_event()
                                        See xtsc_core::get_stop_after_all_cores_exit()
                                        See "SimStopOnExit"
                                        Default = false.

   "SimNoDebugFlush"            bool    If true, disables flushing of the processor
                                        pipeline when a breakpoint or a watchpoint is
                                        encountered.
                                        Default = false.

   "SimNoZeroBSS"               bool    If true, disables zero initialization of the
                                        .bss segment by the simulator during target
                                        program loading.
                                        Default = false.

   "SimPIFFakeWriteResponses"   bool    The Xtensa ISS requires write responses on all
                                        configs (even configs that don't have write
                                        responses configured).  Starting in RG-2015.0
                                        and RF-2015.3 on configs that don't have write
                                        responses configured, xtsc_core will by default
                                        internally generate "fake" write responses for
                                        PIF and iDMA requests and send them to the ISS
                                        (in this case xtsc_core will also discard any
                                        incoming PIF|iDMA write responses).  This
                                        parameter can be set to false to prevent
                                        xtsc_core from generating fake write responses,
                                        in which case the external system must send
                                        them.  This parameter applies to PIF and iDMA
                                        (if present).
                                        Default = true on configs without write
                                        responses configured, otherwise false.

   "SimPinLevelInterfaces"      char**  A NULL-terminated array of TIE and system I/O
                                        interfaces that the user will connect to at the
                                        pin level instead of at the transaction level
                                        or a single asterisk to indicate that all TIE
                                        and system I/O interfaces should be pin level.
                                        It is also possible to specify all interfaces of
                                        the same kind using one (or more) of the
                                        following:
                                          *system_input
                                          *system_output
                                          *ext_regfile
                                          *lookup
                                          *input_queue
                                          *output_queue
                                          *import_wire
                                          *export_state
                                        TIE interfaces are identified using the name
                                        from the TIE file (the name that comes after the
                                        queue, ext_regfile, lookup, import_wire, or state
                                        keyword). System I/O interfaces are identified using
                                        the name as it appears in the Xtensa microprocessor
                                        data book.  Connection to these pin-level
                                        interfaces is performed using the xtsc_core
                                        methods get_input_pin() and get_output_pin().
                                        Any of the above interfaces and wild-cards can
                                        appear with a minus sign in front of them to
                                        indicate that interface or group of interfaces
                                        should be removed from the pin-level list (i.e.
                                        they should be transaction level).  For example,
                                        the following could be used to tell xtsc-run
                                        that all interfaces are to be pin-level except
                                        the system level inputs excluding RunStall:
            xtsc-run -set_core_parm=SimPinLevelInterfaces=*,-*system_input,RunStall ...
                                        Note: Depending on your shell, you may need to
                                        escape or quote any asterisk entered on the 
                                        command line to prevent it being interpreted
                                        as the file globbing character.
                                        Note: xtsc_core does not directly present pin-
                                        level memory interfaces.  Instead, for local and
                                        PIF memory interfaces, the user should use an
                                        xtsc_component::xtsc_tlm2pin_memory_transactor
                                        to convert the TLM memory interface presented by
                                        xtsc_core to a pin-level interface.  For the
                                        inbound PIF interface, an
                                        xtsc_component::xtsc_pin2tlm_memory_transactor
                                        is used.  For tracing, see the "SimVcdHandle"
                                        parameter.
                                        Note: Pin-level TIE lookups are not compatible
                                        with TurboXim/fast-access mode.

   "SimPrefetch"                char*   The optional value to pass to the ISS using the
                                        --prefetch option.  The value should be a 4
                                        character string representing a 8-bit value in
                                        hexadecimal (for example, 0x44).  See --prefetch
                                        command-line option in the ISS User's Guide.
                                        Default value = NULL (do not pass --prefetch to
                                        the ISS).

   "SimReference"               bool    If a TIE instruction is described both in a
                                        reference/operation section and in a semantic
                                        section, use the reference/operation description
                                        instead of the semantic description.
                                        Default value = false.

   "SimRunStall"                bool    If true, set_stall(true) will be called during
                                        end_of_elaboration().  This parameter is just
                                        for convenience to allow starting a core
                                        in the stalled state without having to hook up
                                        something to the RunStall input or calling the
                                        xtsc_core::set_stall() method.
                                        Default = false.

   "SimStackSize"               u32     A non-zero value means to call sc_module::
                                        set_stack_size() with the specified value for
                                        iss_thread, the SystemC thread containing the
                                        ISS.  If 0, set_stack_size() is not called for
                                        iss_thread.
                                        Default = 0x100000 (1MB).

   "SimStaticVectorSelect"      u32     A value of 0xFFFFFFFF means to use the value
                                        of "StaticVectorSelect" (obtained from the
                                        hardware configuration).  0 or 1 means to use
                                        "StaticVectorBase0" or "StaticVectorBase1",
                                        respectively, regardless of "StaticVectorSelect".
                                        Use of this parameter is an alternative to
                                        driving the StatVectorSel input or manually
                                        calling the xtsc_core::set_static_vector_select()
                                        method.  If the StatVectorSel input is being
                                        driven then this parameter should normally be
                                        left at its default value.
                                        Legal values are 0|1|0xFFFFFFFF.
                                        Default value = 0xFFFFFFFF.

   "SimStopOnExit"              bool    If true, sc_stop() will be called by this core
                                        when the target program running on this core
                                        calls exit() regardless of the state of other
                                        cores in the system.
                                        See xtsc_core::set_stop_after_all_cores_exit()
                                        See "SimNeverExits"
                                        Default = false.

   "SimTargetArgs"              char**  A NULL-terminated array of command-line
                                        arguments to pass to the target program.  If
                                        NULL, then the xtsc_core::load_program() call,
                                        if any, defines which arguments to pass to the
                                        target program during the end_of_elaboration()
                                        callback.  If not NULL, then this parameter
                                        specifies which arguments to pass to the target
                                        program during the end_of_elaboration() callback
                                        and overrides the target program arguments
                                        specified in the xtsc_core::load_program() call,
                                        if any.
                                        Default value = NULL (use the target program
                                        arguments from load_program()).

   "SimTargetInput"             char *  The name of the file from which target stdin is
                                        read.  If NULL, target stdin is read from host
                                        stdin.
                                        Default value = NULL.

   "SimTargetOutput"            char *  The name of the file to which target stdout is
                                        written.  If NULL, target stdout is written to
                                        host stdout.
                                        Default value = NULL.

   "SimTargetProgram"           char *  The name of the target program.  If NULL, then
                                        the xtsc_core::load_program() call, if any,
                                        defines which target program is loaded during
                                        the end_of_elaboration() callback.  If not NULL,
                                        then this parameter specifies the target program
                                        to load either during the end_of_elaboration()
                                        callback (if "SimTargetProgramDelayLoad" is
                                        false) or during the first delta cycle of
                                        simulation (if "SimTargetProgramDelayLoad" is
                                        true).  If not NULL, then this parameter
                                        overrides any xtsc_core::load_program() call
                                        made prior to the end_of_elaboration() call.  If
                                        desired, multiple files to be loaded can be
                                        specified by separating them with a comma.
                                        Default value = NULL (use load_program()).
                                        Also see "SimTargetProgramReload".

   "SimTargetProgramDelayLoad"  bool    If false, then the program specified by the
                                        "SimTargetProgram" parameter is loaded duing the
                                        end_of_elaboration() callback.  Otherwise, the
                                        program specified by the "SimTargetProgram"
                                        parameter is loaded during the first delta cycle
                                        of simulation.  This parameter only applies if
                                        "SimTargetProgram" is specified.
                                        Default value = true (Verilog co-simulation)
                                        Default value = false (All others)

   "SimTargetProgramReload"     bool    If false, then the program specified by the
                                        "SimTargetProgram" parameter is loaded only as
                                        specified by the "SimTargetProgramDelayLoad"
                                        parameter.  If true, then the program will also
                                        be reloaded one SystemC time resolution after 
                                        xtsc_core::reset() is called.  This parameter
                                        only applies if "SimTargetProgram" is specified.
                                        Default value = true (Verilog co-simulation)
                                        Default value = false (All others)

   "SimVcdHandle"               void *  Pointer to a SystemC VCD object (sc_trace_file*)
                                        in which to trace all registers named by the
                                        "SimTraceRegisters" parameter.  In addition, if
                                        "SimTraceInterfaces" is true, then all pin-level
                                        interfaces specified by "SimPinLevelInterfaces"
                                        parameter will be traced.
                                        Default value = NULL (no tracing).

   "SimTraceInterfaces"         bool    If true and if "SimVcdHandle" is non-null and
                                        non-empty then the interfaces specified by the
                                        "SimPinLevelInterfaces" parameter will be added
                                        to the SystemC VCD object specified by the
                                        "SimVcdHandle" parameter.
                                        Default true.

   "SimTraceRegisters"          char**  A NULL-terminated array of core registers to be
                                        added to the SystemC VCD object specified by the
                                        "SimVcdHandle" parameter or a single asterisk to
                                        indicate that all core registers (from the
                                        xtsc_core::get_register_set() method) should be
                                        traced.  When a single asterisk is used the
                                        traced registers will include the program
                                        counter.  Alternately, "PC" can be specified as
                                        one of the array entries to add the program
                                        counter to the list of traced registers.
                                        See enable_register_tracing().
                                        Default value = NULL (no registers are traced).

   "SimTurbo"                   bool    Enable TurboXim/fast-access mode (instruction-
                                        accurate but not cycle-accurate) from the start
                                        of simulation.  See xtsc_switch_sim_mode()
                                        for more fine grain control of TurboXim/fast-
                                        access mode.
                                        Default value is as set by the "turbo"
                                        xtsc_initialize_parms parameter.
                                        Note:  Direct setting of this parameter is
                                        deprecated.  Use the "turbo" parameter of
                                        xtsc_initialize_parms instead.

   "SimTurboCache"              char*   The optional name of a file which TurboXim
                                        should use to cache simulation context for this
                                        core.  If TurboXim is allowed to cache
                                        simulation context it can speed up subsequent
                                        simulation runs.
                                        Default value = NULL (simulation context is not
                                        cached).

   "SimTurboMaxRelaxedCycles"   u32     This specifies the maximum total amount that
                                        this core  may run ahead of actual simulation
                                        time without yielding to the SystemC kernel
                                        when operating in the functional mode of
                                        TurboXim. This amount is expressed in terms
                                        of system clock periods.
                                        See xtsc_get_system_clock_period().
                                        A value of 0xFFFFFFFF means to use the global
                                        value of "turbo_max_relaxed_cycles" parameter
                                        as set in xtsc_initialize_parms
                                        construction time.
                                        Default = 0xFFFFFFFF.

   "SimUsingPifMux"             bool    Although not recommended, you can set this
                                        parameter to true when cosimulating with
                                        PifMux.v to work around its violating the PIF
                                        protocol.
                                        Default = false.

    \endverbatim
 *
 *
 * @see xtsc_core
 * @see xtsc_parms
 * @see xtsc_initialize_parms
 */
class XTSC_API xtsc_core_parms : public xtsc_parms {
public:


  /**
   * Constructor for an xtsc_core_parms object.
   *
   * @param     XTENSA_CORE     The name of the core configuration.  If NULL (the
   *                            default) or empty, the core configuration name will be
   *                            obtained from the XTENSA_CORE environment variable if it
   *                            exists.  If the XTENSA_CORE argument is NULL or empty
   *                            and the XTENSA_CORE environment variable does not exist,
   *                            then a configuration name of "default" will be used.
   *
   * @param     XTENSA_SYSTEM   The Xtensa registry path.  If NULL (the default), the
   *                            registry path will be obtained from the XTENSA_SYSTEM
   *                            environment variable (which, in this case, must exist).
   *                            If desired, multiple directories may be specified using
   *                            a semi-colon separated list.  
   *
   * @param     XTENSA_PARAMS   The TDK directory for user TIE extensions.  If NULL (the
   *                            default), the TDK directory will be obtained from the
   *                            XTENSA_PARAMS environment variable (if that environment
   *                            variable exists).  If the XTENSA_PARAMS argument is the
   *                            empty string (""), then no user TIE will be applied to
   *                            this core regardless of the contents of the
   *                            XTENSA_PARAMS environment variable.
   *
   * Note: XTENSA_SYSTEM here is a c-string (char *) but it will get stored as a
   *       c-string array (char**) in xtsc_parms::m_c_str_array_map using the name
   *       "XTENSA_SYSTEM".
   *
   */
  xtsc_core_parms(const char  *XTENSA_CORE   = NULL,
                  const char  *XTENSA_SYSTEM = NULL,
                  const char  *XTENSA_PARAMS = NULL
                 );


  /// Return what kind of xtsc_parms this is (our C++ type)
  virtual const char* kind() const { return "xtsc_core_parms"; }


private:
  void ctor_helper(const char *XTENSA_CORE, const char *XTENSA_SYSTEM, const char *XTENSA_PARAMS);
};







/**
 * A Cadence-Tensilica Xtensa core Instruction Set Simulator (ISS).
 *
 * This class encapsulates an Xtensa core Instruction Set Simulator (ISS) as a SystemC
 * module.  It can be operated in cycle-accurate mode (the default) or in instruction-
 * accurate (TurboXim) mode, or a combination of both.  It includes methods for such
 * things as loading the core's program, loading simulation clients, probing the core's
 * state, querying the core's construction parameters, and setting up the core for
 * debugger control.
 *
 * It also includes methods to allow TLM-level (Transaction Level Model/Modeling) port
 * binding to take place to all the local memory ports, the PIF|AXI|iDMA, all user-
 * defined TIE interfaces (ext_regfiles, lookups, input queues, output queues, import 
 * wires, and export states) and certain system-level inputs and outputs.  Due to the
 * configurable nature of the Xtensa core interface, the port binding is done somewhat
 * differently than for a typical SystemC module.  See the documentation comments
 * associated with the "How_to_do_..." dummy Readme variables for information on port
 * binding with each of the various Xtensa interfaces (memory ports, system-level I/O,
 * and TIE interfaces).
 *
 * For information pertaining to memory interface request/response protocols, see the
 * documentation comments associated with the Information_on_memory_interface_protocols
 * dummy Readme variable.
 *
 * Here is a block diagram of the TLM ports of an xtsc_core:
 * @image html  Example_xtsc_core.jpg
 * @image latex Example_xtsc_core.eps "xtsc_core" width=10cm
 *
 * @see How_to_do_port_binding
 * @see Information_on_memory_interface_protocols
 */
class XTSC_API xtsc_core : public sc_core::sc_module, public xtsc_module, public xtsc_command_handler_interface {
public:


  /**
   * Instructions for doing port binding in XTSC.
   *
   * The XTSC generic connection mechanism, when available, is the recommended approach
   * to perform port binding in XTSC.  This approach is available as long as both
   * modules implement the xtsc_connection_interface (which all modules in the XTSC core
   * and component libraries do).  The XTSC generic connection mechanism is primarily
   * documented under xtsc_connect(), xtsc::xtsc_port_type, and
   * xtsc_connection_interface; however, a brief summary with some tips, examples, and
   * links is given here.
   *
   * A typical invocation of the xtsc_connect() method looks like this:
   *  \verbatim
         xtsc_connect(core0, "dram0", "slave_ports", mem_dram0);
      \endverbatim
   * Where core0 and mem_dram0 are instances of xtsc_core and 
   * xtsc_component::xtsc_memory, respectively, and where "dram0" and "slave_ports" are
   * port tables of core0 and mem_dram0, respectively.  A port table is a list of one or
   * more elementary ports.  The port tables passed to xtsc_connect() must be "conjugate
   * pairs" (that is, each elementary port in one port table must be capable of being
   * connected to the corresponding elementary port in the other port table).
   *  
   * You can access the xtsc_connect() method from xtsc-run using the --connect command.
   * For example:
   *  \verbatim
         xtsc-run ... --connect=core0,dram0,slave_ports,mem_dram0  ...
      \endverbatim
   *
   * Regardless of whether you are building your system using xtsc-run or sc_main, the
   * xtsc-run program can be used to get a list of port tables and elementary ports that
   * an XTSC module has once an instance of that module is created.  Here are some
   * examples:
   *  \verbatim
        xtsc-run -memory_port=dram0 -create_memory -show_ports -quit
        xtsc-run -set_arbiter_parm=num_masters=2 -create_arbiter -show_ports -quit
        xtsc-run -set_router_parm=num_slaves=2 -create_router -show_ports -quit
        xtsc-run -create_queue -show_ports -quit
        xtsc-run -create_xtsc_udma -show_ports -quit
        xtsc-run -create_xtsc_cache -show_ports -quit
        xtsc-run -create_wire -show_ports -quit
        xtsc-run -set_lookup_parm=address_bit_width=1 -set_lookup_parm=data_bit_width=1 \
                 -create_lookup=lut -show_ports -quit
      \endverbatim
   *
   * You can get more information about xtsc-run from the XTSC User's Guide (xtsc_ug.pdf)
   * or from xtsc-run itself using the --man command.  For example:
   *  \verbatim
         xtsc-run --man | less
      \endverbatim
   *
   * Many examples of using xtsc_connect() and xtsc-run --connect can be found in the
   * sc_main.cpp files (for xtsc_connect()) and in the *.inc files (for xtsc-run) in the
   * XTSC examples of your config installation.  Here is an example location on Linux
   * for the sample_config:
   *  \verbatim
      /xplorer/RF-2015.2/XtDevTools/install/builds/RF-2015.2-linux/sample_config/examples/XTSC
      \endverbatim
   * And here is an example location on MS Windows:
   *  \verbatim
      C:\xplorer\RF-2015.2\XtDevTools\install\builds\RF-2015.2-win32\sample_config\examples\XTSC
      \endverbatim
   *
   * When xtsc_connect() and the xtsc-run --connect command are not available (because
   * one or both of the modules does not implement the xtsc_connection_interface), then
   * manual SystemC port binding must be done (typically in your sc_main.cpp code);
   * however, this is complicated somewhat by the fact that the ports (sc_port<T> and
   * sc_export<T> objects) are not named members of xtsc_core.  Because of this, you
   * must use an xtsc_core member function to get each desired port object so that port
   * binding can be done.  Which member function is used depends upon which Xtensa
   * interface type is involved.  See the applicable How_to_do_XXX_binding links below.
   *
   * @see How_to_do_memory_port_binding
   * @see How_to_do_tx_xfer_port_binding
   * @see How_to_do_tie_lookup_binding
   * @see How_to_do_tie_queue_binding
   * @see How_to_do_tie_import_wire_binding
   * @see How_to_do_tie_export_state_binding
   * @see How_to_do_system_input_wire_binding
   * @see How_to_do_system_output_wire_binding
   * @see How_to_do_output_pin_binding;
   * @see How_to_do_input_pin_binding;
   *
   * @see xtsc_connect()
   * @see xtsc_port_type
   * @see xtsc_connection_interface
   */
  Readme How_to_do_port_binding;


  /**
   * Instructions for doing memory port binding.
   * 
   * The XTSC generic connection mechanism, when available, is the recommended approach
   * to perform port binding in XTSC.  This approach is available as long as both
   * modules implement the xtsc_connection_interface.  The XTSC generic connection
   * mechanism is primarily documented under xtsc_core::How_to_do_port_binding,
   * xtsc_connect(), xtsc::xtsc_port_type, and xtsc_connection_interface and is only
   * briefly mentioned here.
   *
   * Port binding of non-cache memory ports (both local memories and PIF|AXI|iDMA)
   * can also be accomplished by calling the get_request_port() and get_respond_export()
   * methods to get references to the appropriate sc_port and sc_export objects and then
   * using these references to do port binding in the callee's code.
   *
   * There are two versions of each of these latter two methods.  The primary version
   * takes a memory port name and the secondary version takes a xtsc_core::memory_port
   * enum.
   *
   * The following table shows the legal memory port names.  Although only lower-case
   * names are shown, the memory port name arguments in some XTSC methods are
   * case-insensitive.  
   *
   * <B>Important Exception:</B> the xtsc_connection_interface and related methods, most
   * importantly, the xtsc_connect() method, are all case-sensitive,
   *
   * When using the following table for the get_request_port() and get_respond_export()
   * methods, each row represents one memory port.  Multiple names on a single row are
   * aliases for the same memory port.  For example, "dram" refers to the same memory
   * port as "dram0".
   *
   * When using the following table for the xtsc_connect() method (or the xtsc-run
   * --connect command) only the Second Column and last column are used (where the last
   * column is determined with respect to the row whose Second Column is being used).
   * The Second Column shows the top-level port table name used to refer to all ports of
   * that memory interface and the last column shows the port table names of the memory
   * ports that memory interface potentially has (it may have fewer ports).  For
   * example, if the following xtsc_connect() call is made and core0 has a dual-ported
   * DataRAM0 interface and mem_dram0 has been properly constructed to also be
   * dual-ported, then the xtsc_connect() method will connect both the "dram0p0" and
   * "dram0p1" ports.
   *  \verbatim
         xtsc_connect(core0, "dram0", "slave_ports", mem_dram0);
      \endverbatim
   * 
   * The xtsc-run --connect command has the same arguments and order as the
   * xtsc_connect() method except that quotation marks and spaces are not used.  For
   * example:
   *  \verbatim
         xtsc-run ... --connect=core0,dram0,slave_ports,mem_dram0 ...
      \endverbatim
   * <B><CENTER>
   *  Table of port table names and memory port names (Second Column and last column):
   * </CENTER></B>
   *  \verbatim
             Second
             Column
             ---------
      "dram" "dram0"   "dramls0" "dram0ls0" "dram0b0" "dram0p0"
                       "dramls1" "dram0ls1" "dram0b1" "dram0p1"
                                            "dram0b2" "dram0p2"
                                            "dram0b3" "dram0p3"
             "dram0rw"                                          "dram0ls0rd"
                                                                "dram0ls0wr"
                                                                "dram0ls1rd"
                                                                "dram0ls1wr"
                                                                "dram0ls2rd"
                                                                "dram0ls2wr"
                                                                "dram0dmard"
                                                                "dram0dmawr"
             "dram0bs"                                          "dram0b0s00"
                                                                "dram0b0s01"
                                                                "dram0b0s02"
                                                                "dram0b0s03"
                                                                "dram0b0s04"
                                                                "dram0b0s05"
                                                                "dram0b0s06"
                                                                "dram0b0s07"
                                                                "dram0b0s08"
                                                                "dram0b0s09"
                                                                "dram0b0s10"
                                                                "dram0b0s11"
                                                                "dram0b0s12"
                                                                "dram0b0s13"
                                                                "dram0b0s14"
                                                                "dram0b0s15"
                                                                "dram0b0s16"
                                                                "dram0b0s17"
                                                                "dram0b0s18"
                                                                "dram0b0s19"
                                                                "dram0b0s20"
                                                                "dram0b0s21"
                                                                "dram0b0s22"
                                                                "dram0b0s23"
                                                                "dram0b0s24"
                                                                "dram0b0s25"
                                                                "dram0b0s26"
                                                                "dram0b0s27"
                                                                "dram0b0s28"
                                                                "dram0b0s29"
                                                                "dram0b0s30"
                                                                "dram0b0s31"
                                                                "dram0b1s00"
                                                                "dram0b1s01"
                                                                "dram0b1s02"
                                                                "dram0b1s03"
                                                                "dram0b1s04"
                                                                "dram0b1s05"
                                                                "dram0b1s06"
                                                                "dram0b1s07"
                                                                "dram0b1s08"
                                                                "dram0b1s09"
                                                                "dram0b1s10"
                                                                "dram0b1s11"
                                                                "dram0b1s12"
                                                                "dram0b1s13"
                                                                "dram0b1s14"
                                                                "dram0b1s15"
                                                                "dram0b1s16"
                                                                "dram0b1s17"
                                                                "dram0b1s18"
                                                                "dram0b1s19"
                                                                "dram0b1s20"
                                                                "dram0b1s21"
                                                                "dram0b1s22"
                                                                "dram0b1s23"
                                                                "dram0b1s24"
                                                                "dram0b1s25"
                                                                "dram0b1s26"
                                                                "dram0b1s27"
                                                                "dram0b1s28"
                                                                "dram0b1s29"
                                                                "dram0b1s30"
                                                                "dram0b1s31"
                                                                "dram0b2s00"
                                                                "dram0b2s01"
                                                                "dram0b2s02"
                                                                "dram0b2s03"
                                                                "dram0b2s04"
                                                                "dram0b2s05"
                                                                "dram0b2s06"
                                                                "dram0b2s07"
                                                                "dram0b2s08"
                                                                "dram0b2s09"
                                                                "dram0b2s10"
                                                                "dram0b2s11"
                                                                "dram0b2s12"
                                                                "dram0b2s13"
                                                                "dram0b2s14"
                                                                "dram0b2s15"
                                                                "dram0b2s16"
                                                                "dram0b2s17"
                                                                "dram0b2s18"
                                                                "dram0b2s19"
                                                                "dram0b2s20"
                                                                "dram0b2s21"
                                                                "dram0b2s22"
                                                                "dram0b2s23"
                                                                "dram0b2s24"
                                                                "dram0b2s25"
                                                                "dram0b2s26"
                                                                "dram0b2s27"
                                                                "dram0b2s28"
                                                                "dram0b2s29"
                                                                "dram0b2s30"
                                                                "dram0b2s31"
                                                                "dram0b3s00"
                                                                "dram0b3s01"
                                                                "dram0b3s02"
                                                                "dram0b3s03"
                                                                "dram0b3s04"
                                                                "dram0b3s05"
                                                                "dram0b3s06"
                                                                "dram0b3s07"
                                                                "dram0b3s08"
                                                                "dram0b3s09"
                                                                "dram0b3s10"
                                                                "dram0b3s11"
                                                                "dram0b3s12"
                                                                "dram0b3s13"
                                                                "dram0b3s14"
                                                                "dram0b3s15"
                                                                "dram0b3s16"
                                                                "dram0b3s17"
                                                                "dram0b3s18"
                                                                "dram0b3s19"
                                                                "dram0b3s20"
                                                                "dram0b3s21"
                                                                "dram0b3s22"
                                                                "dram0b3s23"
                                                                "dram0b3s24"
                                                                "dram0b3s25"
                                                                "dram0b3s26"
                                                                "dram0b3s27"
                                                                "dram0b3s28"
                                                                "dram0b3s29"
                                                                "dram0b3s30"
                                                                "dram0b3s31"

             "dram1"             "dram1ls0" "dram1b0" "dram1p0"
                                 "dram1ls1" "dram1b1" "dram1p1"
                                            "dram1b2" "dram1p2"
                                            "dram1b3" "dram1p3"
             "dram1rw"                                          "dram1ls0rd"
                                                                "dram1ls0wr"
                                                                "dram1ls1rd"
                                                                "dram1ls1wr"
                                                                "dram1ls2rd"
                                                                "dram1ls2wr"
                                                                "dram1dmard"
                                                                "dram1dmawr"
             "dram1bs"                                          "dram1b0s00"
                                                                "dram1b0s01"
                                                                "dram1b0s02"
                                                                "dram1b0s03"
                                                                "dram1b0s04"
                                                                "dram1b0s05"
                                                                "dram1b0s06"
                                                                "dram1b0s07"
                                                                "dram1b0s08"
                                                                "dram1b0s09"
                                                                "dram1b0s10"
                                                                "dram1b0s11"
                                                                "dram1b0s12"
                                                                "dram1b0s13"
                                                                "dram1b0s14"
                                                                "dram1b0s15"
                                                                "dram1b0s16"
                                                                "dram1b0s17"
                                                                "dram1b0s18"
                                                                "dram1b0s19"
                                                                "dram1b0s20"
                                                                "dram1b0s21"
                                                                "dram1b0s22"
                                                                "dram1b0s23"
                                                                "dram1b0s24"
                                                                "dram1b0s25"
                                                                "dram1b0s26"
                                                                "dram1b0s27"
                                                                "dram1b0s28"
                                                                "dram1b0s29"
                                                                "dram1b0s30"
                                                                "dram1b0s31"
                                                                "dram1b1s00"
                                                                "dram1b1s01"
                                                                "dram1b1s02"
                                                                "dram1b1s03"
                                                                "dram1b1s04"
                                                                "dram1b1s05"
                                                                "dram1b1s06"
                                                                "dram1b1s07"
                                                                "dram1b1s08"
                                                                "dram1b1s09"
                                                                "dram1b1s10"
                                                                "dram1b1s11"
                                                                "dram1b1s12"
                                                                "dram1b1s13"
                                                                "dram1b1s14"
                                                                "dram1b1s15"
                                                                "dram1b1s16"
                                                                "dram1b1s17"
                                                                "dram1b1s18"
                                                                "dram1b1s19"
                                                                "dram1b1s20"
                                                                "dram1b1s21"
                                                                "dram1b1s22"
                                                                "dram1b1s23"
                                                                "dram1b1s24"
                                                                "dram1b1s25"
                                                                "dram1b1s26"
                                                                "dram1b1s27"
                                                                "dram1b1s28"
                                                                "dram1b1s29"
                                                                "dram1b1s30"
                                                                "dram1b1s31"
                                                                "dram1b2s00"
                                                                "dram1b2s01"
                                                                "dram1b2s02"
                                                                "dram1b2s03"
                                                                "dram1b2s04"
                                                                "dram1b2s05"
                                                                "dram1b2s06"
                                                                "dram1b2s07"
                                                                "dram1b2s08"
                                                                "dram1b2s09"
                                                                "dram1b2s10"
                                                                "dram1b2s11"
                                                                "dram1b2s12"
                                                                "dram1b2s13"
                                                                "dram1b2s14"
                                                                "dram1b2s15"
                                                                "dram1b2s16"
                                                                "dram1b2s17"
                                                                "dram1b2s18"
                                                                "dram1b2s19"
                                                                "dram1b2s20"
                                                                "dram1b2s21"
                                                                "dram1b2s22"
                                                                "dram1b2s23"
                                                                "dram1b2s24"
                                                                "dram1b2s25"
                                                                "dram1b2s26"
                                                                "dram1b2s27"
                                                                "dram1b2s28"
                                                                "dram1b2s29"
                                                                "dram1b2s30"
                                                                "dram1b2s31"
                                                                "dram1b3s00"
                                                                "dram1b3s01"
                                                                "dram1b3s02"
                                                                "dram1b3s03"
                                                                "dram1b3s04"
                                                                "dram1b3s05"
                                                                "dram1b3s06"
                                                                "dram1b3s07"
                                                                "dram1b3s08"
                                                                "dram1b3s09"
                                                                "dram1b3s10"
                                                                "dram1b3s11"
                                                                "dram1b3s12"
                                                                "dram1b3s13"
                                                                "dram1b3s14"
                                                                "dram1b3s15"
                                                                "dram1b3s16"
                                                                "dram1b3s17"
                                                                "dram1b3s18"
                                                                "dram1b3s19"
                                                                "dram1b3s20"
                                                                "dram1b3s21"
                                                                "dram1b3s22"
                                                                "dram1b3s23"
                                                                "dram1b3s24"
                                                                "dram1b3s25"
                                                                "dram1b3s26"
                                                                "dram1b3s27"
                                                                "dram1b3s28"
                                                                "dram1b3s29"
                                                                "dram1b3s30"
                                                                "dram1b3s31"

      "drom" "drom0"   "dromls0" "drom0ls0" "drom0b0" "drom0p0"
                       "dromls1" "drom0ls1" "drom0b1" "drom0p1"
                                            "drom0b2" "drom0p2"
                                            "drom0b3" "drom0p3"

      "iram" "iram0"

             "iram1"

      "irom" "irom0"

      "uram" "uram0"

      "xlmi" "xlmi0"   "xlmils0" "xlmi0ls0"           "xlmi0p0"
                       "xlmils1" "xlmi0ls1"           "xlmi0p1"

             "pif"

             "datamaster"                                       "datamaster_rd"
                                                                "datamaster_wr"

             "instmaster"                                       "instmaster_rd"

             "aximaster"                                        "aximaster_rd"
                                                                "aximaster_wr"

             "axi"                                              "datamaster_rd"
                                                                "datamaster_wr"
                                                                "instmaster_rd"
                                                                "aximaster_rd"
                                                                "aximaster_wr"

             "idma0"

             "idma"                                             "idma_rd"
                                                                "idma_wr"

             "sys"                                              "pif"
                                                                "idma0"

             "sys"                                              "axi"
                                                                "idma"

      \endverbatim
   *
   * For inbound ports (snoop, inbound_pif, dataslave_rd, dataslavewr, instslave_rd,
   * instslave_wr, axislave_rd and axislave_wr), the get_request_export() and 
   * get_respond_port() methods are used to get references to the appropriate 
   * sc_export and sc_port objects.
   *
   * Here is a diagram showing the connections of an example system with two xtsc_core
   * objects with PIF interfaces and two xtsc_component::xtsc_memory objects (each of
   * which is single-ported) followed by the port-binding code using the
   * get_request_port() and get_respond_export() methods:
   *  \verbatim
                                RSP                          
       +<---------------------------------------------------+
       |                                                    |
       |      +----------+               +----------+       |
       |   +--|          |--+         +--|          |--+    |
       |   |  |          |  |   REQ   |  |          |  |    |
       +-->|  |  core0   |  |-------->|  | memory0  |  |--->+
           |  |          |  |         |  |          |  |   
           +--|          |--+         +--|          |--+       
              +----------+               +----------+          
                                                               
              +----------+               +----------+          
           +--|          |--+         +--|          |--+       
           |  |          |  |   REQ   |  |          |  |        
       +-->|  |  core1   |  |-------->|  | memory1  |  |--->+  
       |   |  |          |  |         |  |          |  |    |  
       |   +--|          |--+         +--|          |--+    |  
       |      +----------+               +----------+       |  
       |                                                    |
       |                        RSP                         |
       +<---------------------------------------------------+


        core0.get_request_port("pif")(*memory0.m_request_exports[0]);
        core1.get_request_port("pif")(*memory1.m_request_exports[0]);
        (*memory0.m_respond_ports[0])(core0.get_respond_export("pif"));
        (*memory1.m_respond_ports[0])(core1.get_respond_export("pif"));
      \endverbatim
   *
   * Using the recommended XTSC generic connection mechanism, the above 4-line code
   * snippet would be replaced with the following 2 calls to xtsc_connect():
   *  \verbatim
        xtsc_connect(core0, "pif", "slave_ports", memory0);
        xtsc_connect(core1, "pif", "slave_ports", memory1);
      \endverbatim
   *
   * Note:
   * If a local memory port is left unbound, then the Xtensa ISS will use an internal
   * model for that memory.  If PIF or AXI is left unbound, then any attempt by the
   * Xtensa program to access that memory space will result in a simulator exception
   * (not to be confused with an Xtensa exception) being thrown.
   *
   * Note:
   * The Xtensa ISS uses an internal model for instruction and data caches.  Connecting
   * external models to the instruction and data cache interfaces is not supported.
   *
   * @see How_to_do_port_binding
   * @see xtsc_request_if
   * @see xtsc_respond_if
   * @see get_request_port
   * @see get_respond_export
   * @see get_request_export
   * @see get_respond_port
   * @see xtsc_connect()
   * @see xtsc_port_type
   * @see xtsc_connection_interface
   * @see Information_on_memory_interface_protocols
   */
  Readme How_to_do_memory_port_binding;


  /**
   * Method to get the sc_port for binding the memory request channel from this
   * xtsc_core memory interface master to a memory interface slave.
   *
   * @param memory_port_name    The memory port name (case-insensitive).
   *
   * @see How_to_do_memory_port_binding for more information and for a list of valid
   *      memory port names.
   */
  sc_core::sc_port<xtsc_request_if, NSPP>& get_request_port(const char *memory_port_name) const;


  /**
   * Method to get the sc_export for binding the memory response channel from a memory
   * interface slave to this xtsc_core memory interface master.
   *
   * @param memory_port_name    The memory port name (case-insensitive).
   *
   * @see How_to_do_memory_port_binding for more information and for a list of valid
   *      memory port names.
   */
  sc_core::sc_export<xtsc_respond_if>& get_respond_export(const char *memory_port_name) const;


  /**
   * Method to get the sc_export of the named xtsc_core memory interface slave for
   * binding the memory request channel of an external memory interface master to.
   *
   * @param inbound_port_name   One of "snoop", "inbound_pif", "dataslave_rd",
   *                            "dataslave_wr", "instslave_rd", "instslave_wr",
   *                            "axislave_rd", or "axislave_wr".
   *
   * @see How_to_do_memory_port_binding
   */
  sc_core::sc_export<xtsc_request_if>& get_request_export(const char *inbound_port_name = "inbound_pif") const;


  /**
   * Method to get the sc_port of the named xtsc_core memory interface slave for binding
   * to the memory response channel of an external memory interface master.
   *
   * @param inbound_port_name   One of "snoop", "inbound_pif", "dataslave_rd",
   *                            "dataslave_wr", "instslave_rd", "instslave_wr",
   *                            "axislave_rd", or "axislave_wr".
   *
   * @see How_to_do_memory_port_binding
   */
  sc_core::sc_port<xtsc_respond_if, NSPP>& get_respond_port(const char *inbound_port_name = "inbound_pif") const;


#ifndef DOXYGEN_SKIP
  // Deprecated
  sc_core::sc_port<xtsc_retire_if, NSPP>& get_retire_port(const char *memory_port_name = "xlmi0") const;
#endif // DOXYGEN_SKIP


  /**
   * Instructions for doing XFER port binding (the BootLoader interface).
   *
   * Port binding of XFER ports can be accomplished by calling the get_tx_xfer_port()
   * method of the upstream TX core and the get_tx_xfer_export() method of the downstream
   * TX core to get references to the appropriate sc_port and sc_export objects and then
   * using these references to do port binding in the callee's code (typically, sc_main).
   *
   * To start the chain, the m_tx_xfer_port of an xtsc_tx_loader should be bound to the
   * export returned by the get_tx_xfer_export() method of the first TX in the chain.
   *
   * To terminate the chain the port returned by the get_tx_xfer_port() of the last TX
   * in the chain should be bound to the m_tx_xfer_export of the xtsc_tx_loader.
   *
   * Here is some example port-binding code for binding an xtsc_tx_loader, bootloader,
   * to the upstream TX, core0, thence to the downstream TX, core1, then terminating
   * back at bootloader (this is shown here for documentation purposes; the recommended 
   * technique is to use the appropriate connect() method):
   *  \verbatim
        bootloader.m_tx_xfer_port(core0.get_tx_xfer_export());
        core0.get_tx_xfer_port()(core1.get_tx_xfer_export());
        core1.get_tx_xfer_port()(bootloader.m_tx_xfer_export);
      \endverbatim
   *
   * @see How_to_do_port_binding
   * @see xtsc_tx_xfer_if
   * @see get_tx_xfer_port
   * @see get_tx_xfer_export
   * @see xtsc_tx_loader
   */
  Readme How_to_do_tx_xfer_port_binding;


  /**
   * Method to get the sc_port for binding the output xtsc_tx_xfer_if of this xtsc_core to the
   * downstream xtsc_core in the TX chain or to the boot loader if this is the last/only
   * TX in the chain.
   *
   * @throws xtsc_exception if xtsc_core_parms "BootLoader" is false
   * @see How_to_do_tx_xfer_port_binding 
   */
  sc_core::sc_port<xtsc_tx_xfer_if, NSPP>& get_tx_xfer_port() const;


  /**
   * Method to get the sc_export for binding the output xtsc_tx_xfer_if of the upstream
   * xtsc_core in the TX chain or the boot loader if this is the first/only TX in the
   * chain to the input xtsc_tx_xfer_if of this xtsc_core.
   *
   * @throws xtsc_exception if xtsc_core_parms "BootLoader" is false
   * @see How_to_do_tx_xfer_port_binding
   */
  sc_core::sc_export<xtsc_tx_xfer_if>& get_tx_xfer_export() const;


  /**
   * Instructions for doing TIE ext_regfile port binding.
   *
   * The XTSC generic connection mechanism, when available, is the recommended approach
   * to perform port binding in XTSC.  This approach is available as long as both
   * modules implement the xtsc_connection_interface.  The XTSC generic connection
   * mechanism is primarily documented under xtsc_core::How_to_do_port_binding,
   * xtsc_connect(), xtsc::xtsc_port_type, and xtsc_connection_interface and is only
   * briefly mentioned here.
   *
   * Port binding of TIE ext_regfiles can also be accomplished by calling the get_ext_regfile()
   * method with the ext_regfile name to get a reference to the appropriate sc_port and then
   * using it to do port binding in the callee's code.
   *
   * Here is a code snippet showing the instantiation of an xtsc_component::xtsc_ext_regfile
   * and connecting it to the TIE ext_regfile named "erf" of an xtsc_core object named core0
   * \verbatim
      // Get the address and data bit widths and whether the ext_regfile has a ready
      u32  address_width = core0.get_ext_regfile_address_bit_width("erf");
      u32  data_width    = core0.get_ext_regfile_data_bit_width("erf");

      // Create the parameters for an xtsc_ext_regfile 
      xtsc_ext_regfile_parms ext_regfile_parms(address_width, data_width);

      // Instantiate the xtsc_ext_regfile
      xtsc_ext_regfile ext_regfile_table("erf", ext_regfile_parms);

      // Connect the xtsc_ext_regfile to the xtsc_core
      core0.get_ext_regfile("erf")(ext_regfile_table.m_ext_regfile);
      \endverbatim
   *
   * Using the recommended XTSC generic connection mechanism, the last line above would
   * be replaced by:
   * \verbatim
      xtsc_connect(core0, "erf", "", ext_regfile_table);
      \endverbatim
   *
   * In the above call to xtsc_connect(), the third parameter is the empty string which
   * means to use the default port (which in this case is "ext_regfile").
   *
   * @see How_to_do_port_binding
   * @see xtsc_ext_regfile_if
   * @see get_ext_regfile
   * @see xtsc_component::xtsc_ext_regfile_parms
   * @see xtsc_component::xtsc_ext_regfile
   * @see has_ext_regfile
   * @see get_ext_regfile_address_bit_width
   * @see get_ext_regfile_data_bit_width
   * @see xtsc_connect()
   * @see xtsc_port_type
   * @see xtsc_connection_interface
   */
  Readme How_to_do_tie_ext_regfile_binding;


  /**
   * Method to get a reference to the sc_port for the named TIE ext_regfile.
   *
   * @param     ext_regfile_name   The ext_regfile name should be the name provided
   *                               after the "ext_regfile" keyword of the user TIE 
   *                               file.  Specifically, ext_regfile_name should not
   *                               include the "TIE_" prefix or any of the
   *                               suffixes ("_Write", "_WriteAddr", "_WriteData",
   *                               "_ReadCheck", "_ReadCheckAddr", "_ReadStall",
   *                               "_Read", "ReadAddr", or "_ReadData").
   *
   * @see How_to_do_tie_ext_regfile_binding
   */
  sc_core::sc_port<xtsc_ext_regfile_if, NSPP>& get_ext_regfile(const char *ext_regfile_name) const;


  /**
   * Instructions for doing TIE lookup port binding.
   *
   * The XTSC generic connection mechanism, when available, is the recommended approach
   * to perform port binding in XTSC.  This approach is available as long as both
   * modules implement the xtsc_connection_interface.  The XTSC generic connection
   * mechanism is primarily documented under xtsc_core::How_to_do_port_binding,
   * xtsc_connect(), xtsc::xtsc_port_type, and xtsc_connection_interface and is only
   * briefly mentioned here.
   *
   * Port binding of TIE lookups can also be accomplished by calling the get_lookup()
   * method with the lookup name to get a reference to the appropriate sc_port and then
   * using it to do port binding in the callee's code.
   *
   * Here is a code snippet showing the instantiation of an xtsc_component::xtsc_lookup
   * and connecting it to the TIE lookup named "lut" of an xtsc_core object named core0
   * (it assumes there is a lookup table file named "lut.rom"):
   * \verbatim
      // Get the address and data bit widths and whether the lookup has a ready
      u32  address_width = core0.get_lookup_address_bit_width("lut");
      u32  data_width    = core0.get_lookup_data_bit_width("lut");
      u32  latency       = core0.get_lookup_latency("lut");
      bool has_ready     = core0.has_lookup_ready("lut");

      // Create the parameters for an xtsc_lookup 
      xtsc_lookup_parms lookup_parms(address_width,
                                     data_width,
                                     has_ready,
                                     "lut.rom",
                                     "0xDEADBEEF");
      lookup_parms.set("latency", latency);

      // Instantiate the xtsc_lookup
      xtsc_lookup lookup_table("lut", lookup_parms);

      // Connect the xtsc_lookup to the xtsc_core
      core0.get_lookup("lut")(lookup_table.m_lookup);
      \endverbatim
   *
   * Using the recommended XTSC generic connection mechanism, the last line above would
   * be replaced by:
   * \verbatim
      xtsc_connect(core0, "lut", "", lookup_table);
      \endverbatim
   *
   * In the above call to xtsc_connect(), the third parameter is the empty string which
   * means to use the default port (which in this case is "lookup").
   *
   * The xtsc_lookup_parms class has a convenience constructor that takes a reference to
   * an xtsc_core object and the name of a TIE lookup and uses them to find the address and
   * data bit widths, the latency, and whether or not the lookup has a ready interface.  
   * See xtsc_component::xtsc_lookup_parms::xtsc_lookup_parms.
   *
   * @see How_to_do_port_binding
   * @see xtsc_lookup_if
   * @see get_lookup
   * @see xtsc_component::xtsc_lookup_parms
   * @see xtsc_component::xtsc_lookup
   * @see has_lookup
   * @see has_lookup_ready
   * @see get_lookup_address_bit_width
   * @see get_lookup_data_bit_width
   * @see xtsc_connect()
   * @see xtsc_port_type
   * @see xtsc_connection_interface
   */
  Readme How_to_do_tie_lookup_binding;


  /**
   * Method to get a reference to the sc_port for the named TIE lookup.
   *
   * @param     lookup_name     The lookup name should be the name provided
   *                            after the "lookup" keyword of the user TIE 
   *                            file.  Specifically, lookup_name should not
   *                            include the "TIE_" prefix or any of the
   *                            suffixes ("_Out", "_In", "_Out_Req", or "_Rdy").
   *
   * @see How_to_do_tie_lookup_binding
   */
  sc_core::sc_port<xtsc_lookup_if, NSPP>& get_lookup(const char *lookup_name) const;


  /**
   * Instructions for doing TIE queue port binding.
   *
   * The XTSC generic connection mechanism, when available, is the recommended approach
   * to perform port binding in XTSC.  This approach is available as long as both
   * modules implement the xtsc_connection_interface.  The XTSC generic connection
   * mechanism is primarily documented under xtsc_core::How_to_do_port_binding,
   * xtsc_connect(), xtsc::xtsc_port_type, and xtsc_connection_interface and is only
   * briefly mentioned here.
   *
   * Port binding of TIE input or output queues can also be accomplished by calling the
   * appropriate get_output_queue() or get_input_queue() method with the queue_name
   * to get a reference to the appropriate sc_port and then using it to do port
   * binding in the callee's code.
   *
   * Here is a code snippet showing the instantiation of an xtsc_component::xtsc_queue
   * named queue_0_1 and connecting it to the sc_port of a TIE ouput queue named "OUTQ1"
   * of an xtsc_core object named core0 and to the sc_port of a TIE input queue named
   * "INQ1" of an xtsc_core object named core1:
   * \verbatim
      // Get queue bit width
      u32 bit_width = core0.get_tie_bit_width("OUTQ1");

      // Configuration parameters for an xtsc_queue
      xtsc_queue_parms queue_parms(bit_width, 2);

      // Construct the queue
      xtsc_queue queue_0_1("queue_0_1", queue_parms);

      // Connect the queue to the xtsc_core objects
      core0.get_output_queue("OUTQ1")(queue_0_1.m_producer);
      core1.get_input_queue ("INQ1" )(queue_0_1.m_consumer);
      \endverbatim
   *
   * Using the recommended XTSC generic connection mechanism, the last 2 lines above
   * would be replaced with the following 2 calls to xtsc_connect():
   *  \verbatim
        xtsc_connect(core0, "OUTQ1", "queue_push", queue_0_1);
        xtsc_connect(core1, "INQ1",  "queue_pop",  queue_0_1);
      \endverbatim
   *
   * The xtsc_queue_parms class has a convenience constructor that takes a reference to
   * an xtsc_core object and the name of a TIE queue and uses them to find the queue bit
   * width.  See xtsc_component::xtsc_queue_parms::xtsc_queue_parms().
   *
   * @see How_to_do_port_binding
   * @see xtsc_queue_push_if
   * @see xtsc_queue_pop_if
   * @see get_output_queue
   * @see get_input_queue
   * @see xtsc_component::xtsc_queue_parms::xtsc_queue_parms()
   * @see xtsc_component::xtsc_queue
   * @see xtsc_connect()
   * @see xtsc_port_type
   * @see xtsc_connection_interface
   */
  Readme How_to_do_tie_queue_binding;


  /**
   * Method to get a reference to the sc_port for the named output queue.
   *
   * @param     queue_name      The queue_name should be the base queue name
   *                            as provided after the "queue" keyword of the
   *                            user TIE file.  Specifically, queue_name
   *                            should not include the "TIE_" prefix or any
   *                            of the suffixes ("_Empty", "_PopReq", "_Full",
   *                            or "_PushReq").
   *
   * @see How_to_do_tie_queue_binding
   */
  sc_core::sc_port<xtsc_queue_push_if, NSPP>& get_output_queue(const char *queue_name) const;


  /**
   * Method to get a reference to the sc_port for the named input queue.
   *
   * @param     queue_name      The queue_name should be the base queue name
   *                            as provided after the "queue" keyword of the
   *                            user TIE file.  Specifically, queue_name
   *                            should not include the "TIE_" prefix or any
   *                            of the suffixes ("_Empty", "_PopReq", "_Full",
   *                            or "_PushReq").
   *
   * @see How_to_do_tie_queue_binding
   */
  sc_core::sc_port<xtsc_queue_pop_if, NSPP>& get_input_queue(const char *queue_name) const;


  /**
   * Instructions for doing TIE import wire port binding.
   *
   * The XTSC generic connection mechanism, when available, is the recommended approach
   * to perform port binding in XTSC.  This approach is available as long as both
   * modules implement the xtsc_connection_interface.  The XTSC generic connection
   * mechanism is primarily documented under xtsc_core::How_to_do_port_binding,
   * xtsc_connect(), xtsc::xtsc_port_type, and xtsc_connection_interface and is only
   * briefly mentioned here.
   *
   * As an alternative approach, the get_import_wire() method can be called by the user
   * to get a reference to the sc_port<xtsc_wire_read_if, NSPP> object that is the
   * xtsc_core's input port for the import wire specified by wire_name.  
   *
   * One technique would be for the user's code to instantiate an
   * xtsc_component::xtsc_wire object of the appropriate size, call get_import_wire(),
   * and then bind the returned sc_port<xtsc_wire_read_if, NSPP> to the
   * xtsc_component::xtsc_wire channel using normal SystemC port binding.
   *
   * @see How_to_do_port_binding
   * @see xtsc_wire_read_if
   * @see get_import_wire
   * @see get_export_state
   * @see xtsc_component::xtsc_wire
   * @see How_to_do_tie_export_state_binding for a code example.
   * @see xtsc_connect()
   * @see xtsc_port_type
   * @see xtsc_connection_interface
   */
  Readme How_to_do_tie_import_wire_binding;


  /**
   * Method to get a reference to the sc_port for the named TIE import wire.
   *
   * @param     wire_name       The base import wire name as provided after 
   *                            the "import_wire" keyword of the user TIE file.
   *                            Specifically, wire_name should not include the 
   *                            "TIE_" prefix.
   *
   * @see How_to_do_tie_import_wire_binding
   */
  sc_core::sc_port<xtsc_wire_read_if, NSPP>& get_import_wire(const char *wire_name) const;


  /**
   * Instructions for doing TIE export state port binding.
   *
   * The XTSC generic connection mechanism, when available, is the recommended approach
   * to perform port binding in XTSC.  This approach is available as long as both
   * modules implement the xtsc_connection_interface.  The XTSC generic connection
   * mechanism is primarily documented under xtsc_core::How_to_do_port_binding,
   * xtsc_connect(), xtsc::xtsc_port_type, and xtsc_connection_interface and is only
   * briefly mentioned here.
   *
   * As an alternative approach, the get_export_state() method can be called by the user
   * to get a reference to the sc_port<xtsc_wire_write_if, NSPP> object that is the
   * xtsc_core's output port for the export state specified by state_name.  
   *
   * One technique would be for the user's code to instantiate an
   * xtsc_component::xtsc_wire object of the appropriate size, call get_export_state(),
   * and then bind the returned sc_port<xtsc_wire_write_if, NSPP> to the
   * xtsc_component::xtsc_wire channel using normal SystemC port binding.  For example,
   * the following code snippet connects a 50-bit xtsc_component::xtsc_wire from the
   * "status" export state of core0 to the "control" import wire of core1.
   *
   *  \verbatim
      // Configuration parameters for an xtsc_wire
      xtsc_wire_parms wire_parms(50);

      // Construct the wire
      xtsc_wire core0_to_core1("core0_to_core1", wire_parms);

      // Connect the wire 
      core0.get_export_state("status" )(core0_to_core1);
      core1.get_import_wire ("control")(core0_to_core1);
      \endverbatim
   *
   * Using the recommended XTSC generic connection mechanism, the last 2 lines above
   * would be replaced with the following 2 calls to xtsc_connect():
   *  \verbatim
        xtsc_connect(core0, "status",  "wire_write", core0_to_core1);
        xtsc_connect(core1, "control", "wire_read",  core0_to_core1);
      \endverbatim
   *
   * The xtsc_wire_parms class has a convenience constructor that takes a reference to
   * an xtsc_core object and the name of a TIE export state or import wire or
   * system-level output and uses them to find the wire bit width.
   * See xtsc_component::xtsc_wire_parms::xtsc_wire_parms().
   *
   * @see How_to_do_port_binding
   * @see xtsc_wire_write_if
   * @see get_export_state
   * @see get_import_wire
   * @see xtsc_component::xtsc_wire_parms::xtsc_wire_parms()
   * @see xtsc_component::xtsc_wire
   * @see xtsc_connect()
   * @see xtsc_port_type
   * @see xtsc_connection_interface
   */
  Readme How_to_do_tie_export_state_binding;


  /**
   * Method to get a reference to the sc_port for the named TIE export state.
   *
   * @param     state_name      The base state name as provided after the "state" 
   *                            keyword of the user TIE file.  Specifically,
   *                            state_name should not include the "TIE_" prefix.
   *
   * @see How_to_do_tie_export_state_binding
   */
  sc_core::sc_port<xtsc_wire_write_if, NSPP>& get_export_state(const char *state_name) const;


  /**
   * Instructions for doing output pin binding (Pin-level).
   *
   * The XTSC generic connection mechanism, when available, is the recommended approach
   * to perform port binding in XTSC.  This approach is available as long as both
   * modules implement the xtsc_connection_interface.  The XTSC generic connection
   * mechanism is primarily documented under xtsc_core::How_to_do_port_binding,
   * xtsc_connect(), xtsc::xtsc_port_type, and xtsc_connection_interface and is only
   * briefly mentioned here.
   *
   * As an alternative approach, the get_output_pin() method can be called by the user to
   * get a reference to the sc_out<sc_bv_base> object that is the xtsc_core's pin-level
   * output port for the TIE or system output pin specified by output_pin_name.  
   *
   * Here are some of the things a system output pin can be connected to using an
   * instance of sc_signal<sc_bv_base> or the xtsc_signal_sc_bv_base convenience class:
   *    - A Verilog module
   *    - A pin-level import wire of an xtsc_core 
   *    - A system input pin of an xtsc_core 
   *
   * Here is an example code snippet showing how to connect a pin-level output of an
   * xtsc_core (PWaitMode) to an sc_signal<sc_bv_base>:
   *  \verbatim
      char *pins[] = { "PWaitMode", NULL };
      core_parms.set("SimPinLevelInterfaces", pins);
      xtsc_core core("core", core_parms);
      sc_length_context lc(1);
      sc_signal<sc_bv_base> PWaitMode("PWaitMode");
      core.get_output_pin("PWaitMode")(PWaitMode);
      \endverbatim
   *
   * @see How_to_do_port_binding
   * @see get_pin_bit_width 
   * @see get_output_pin
   * @see xtsc_signal_sc_bv_base
   * @see xtsc_component::xtsc_wire_source
   * @see How_to_do_input_pin_binding;
   * @see xtsc_connect()
   * @see xtsc_port_type
   * @see xtsc_connection_interface
   */
  Readme How_to_do_output_pin_binding;


  /**
   * Method to get a reference to the sc_out<sc_bv_base> object for the named TIE or
   * system output pin.
   *
   * @param     output_pin_name         For TIE output pins, this is the Verilog name as
   *                                    generated by the TIE compiler.  This name always
   *                                    begins with the "TIE_" prefix and, for TIE
   *                                    ext_regfiles, lookups and queues, has the suffixes
   *                                    specified in the TIE Reference Manual.  For system
   *                                    output pins, this is the name as specified in the
   *                                    Xtensa microprocessor data book.  The TIE
   *                                    construct name (the name without the "TIE_"
   *                                    prefix and without the signal-specific suffix)
   *                                    or the system output name must have been
   *                                    specified in the xtsc_core_parms
   *                                    "SimPinLevelInterfaces" parameters.
   *
   * @see How_to_do_output_pin_binding;
   */
  sc_core::sc_out<sc_dt::sc_bv_base>& get_output_pin(const char *output_pin_name) const;


  /**
   * Instructions for doing input pin binding (Pin-level).
   *
   * The XTSC generic connection mechanism, when available, is the recommended approach
   * to perform port binding in XTSC.  This approach is available as long as both
   * modules implement the xtsc_connection_interface.  The XTSC generic connection
   * mechanism is primarily documented under xtsc_core::How_to_do_port_binding,
   * xtsc_connect(), xtsc::xtsc_port_type, and xtsc_connection_interface and is only
   * briefly mentioned here.
   *
   * As an alternative approach, the get_input_pin() method can be called by the user to
   * get a reference to the sc_in<sc_bv_base> object that is the xtsc_core's pin-level
   * input port for the TIE or system input pin specified by input_pin_name.  
   *
   * Here are some of the things a system input pin can be connected to using an
   * instance of sc_signal<sc_bv_base> or the xtsc_signal_sc_bv_base convenience class:
   *    - A Verilog module
   *    - An xtsc_wire_source pin-level output port
   *    - A pin-level export state of an xtsc_core 
   *    - A system output pin of an xtsc_core 
   *
   * Here is an example code snippet showing how to connect a pin-level output of an
   * xtsc_wire_source to a pin-level input (BInterrupt) of an xtsc_core using the
   * xtsc_signal_sc_bv_base convenience class:
   *  \verbatim
      char *pins[] = { "BInterrupt", NULL };
      core_parms.set("SimPinLevelInterfaces", pins);
      xtsc_core core("core", core_parms);

      u32 num_bits = core.get_pin_bit_width("BInterrupt");

      source_parms.set("bit_width", num_bits);
      source_parms.set("pin_level", true);
      xtsc_wire_source source("source", source_parms);

      xtsc_signal_sc_bv_base BInterrupt("BInterrupt", num_bits);

      source.get_output_pin("m_pin")(BInterrupt);
      core.get_input_pin("BInterrupt")(BInterrupt);
      \endverbatim
   *
   * @see How_to_do_port_binding
   * @see get_pin_bit_width 
   * @see get_input_pin
   * @see xtsc_signal_sc_bv_base
   * @see xtsc_component::xtsc_wire_source
   * @see How_to_do_output_pin_binding;
   * @see xtsc_connect()
   * @see xtsc_port_type
   * @see xtsc_connection_interface
   */
  Readme How_to_do_input_pin_binding;


  /**
   * Method to get a reference to the sc_in<sc_bv_base> object for the named TIE or
   * system input pin.
   *
   * @param     input_pin_name          For TIE input pins, this is the Verilog name as
   *                                    generated by the TIE compiler.  This name always
   *                                    begins with the "TIE_" prefix and, for TIE
   *                                    ext_regfiles, lookups and queues, has the suffixes
   *                                    specified in the TIE Reference Manual.  For system
   *                                    input pins, this is the name as specified in the
   *                                    Xtensa microprocessor data book.  The TIE
   *                                    construct name (the name without the "TIE_"
   *                                    prefix and without the signal-specific suffix)
   *                                    or the system input name must have been
   *                                    specified in the xtsc_core_parms
   *                                    "SimPinLevelInterfaces" parameters.
   * @see How_to_do_input_pin_binding;
   */
  sc_core::sc_in<sc_dt::sc_bv_base>& get_input_pin(const char *input_pin_name) const;


  /**
   * Instructions for doing system input wire port binding (TLM).
   *
   * The XTSC generic connection mechanism, when available, is the recommended approach
   * to perform port binding in XTSC.  This approach is available as long as both
   * modules implement the xtsc_connection_interface.  The XTSC generic connection
   * mechanism is primarily documented under xtsc_core::How_to_do_port_binding,
   * xtsc_connect(), xtsc::xtsc_port_type, and xtsc_connection_interface and is only
   * briefly mentioned here.
   *
   * As an alternative approach, the get_system_input_wire() method can be called by the
   * user to get a reference to the sc_export<xtsc_wire_write_if> object that is the
   * xtsc_core's TLM input port for the system input wire specified by wire_name.  
   *
   * Some of the things a system input wire can be bound to are:
   *    - An xtsc_component::xtsc_mmio output port
   *    - An xtsc_component::xtsc_wire_logic output port
   *    - An xtsc_component::xtsc_wire_source output port
   *    - An export state of an xtsc_core 
   *    - A system output wire of an xtsc_core 
   *
   * One example usage would be for the user's code to instantiate an
   * xtsc_component::xtsc_wire_source object of the appropriate size, call
   * get_system_input_wire(), and then bind the returned sc_export<xtsc_wire_write_if>
   * with the xtsc_component::xtsc_wire_source::m_write member (which is of type
   * sc_port<xtsc_wire_write_if, NSPP>) using normal SystemC port binding as illustrated
   * below:
   *  \verbatim
      xtsc_core                 core("core", core_parms);
      u32                       num_bits = core.get_sysio_bit_width("BInterrupt");
      xtsc_wire_source_parms    source_parms(num_bits, "int_driver.vec");
      xtsc_wire_source          int_driver("int_driver", source_parms);
      int_driver.m_write(core.get_system_input_wire("BInterrupt"));
      \endverbatim
   *
   * Using the recommended XTSC generic connection mechanism, the last line above
   * would be replaced with the following call to xtsc_connect():
   *  \verbatim
        xtsc_connect(int_driver, "m_write", "BInterrupt", core);
      \endverbatim
   *
   * @see How_to_do_port_binding
   * @see xtsc_wire_write_if
   * @see get_sysio_bit_width
   * @see get_system_input_wire
   * @see get_system_output_wire
   * @see xtsc_component::xtsc_mmio
   * @see xtsc_component::xtsc_wire_logic
   * @see xtsc_component::xtsc_wire_source
   * @see xtsc_connect()
   * @see xtsc_port_type
   * @see xtsc_connection_interface
   */
  Readme How_to_do_system_input_wire_binding;


  /**
   * Method to get a reference to the sc_export for the named system input wire.
   *
   * @param     wire_name       The system input name as it appears in the Xtensa
   *                            microprocessor data book.
   *                            The possible input wire names are:
   *  \verbatim
                                        "AltResetVec"     (LX)
                                        "BInterrupt"
                                        "BReset"
                                        "PRID"
                                        "RunStall"        (LX)
                                        "RunStallOnReset" (NX)
                                        "StatVectorSel"
                                        "TrigIn_iDMA"
                                        "XferReset"       (TX)
      \endverbatim
   *
   * Note:   The initial value of the PRID register can be set using the "ProcessorID"
   *         parameter of xtsc_core_parms.
   *
   * Note:   In addition to the composite "BInterrupt" wire, the interrupt wires are
   *         available individually.  For LX configs the inputs are named
   *         "BInterrupt00", "BInterrupt01", and so on (the last 2 digits of the name
   *         are the decimal value of the BInterrupt index).  For NX configs the inputs
   *         are named "BInterrupt000", "BInterrupt001", and so on (the last 3 digits of
   *         the name are the decimal value of the BInterrupt index).
   *
   * @see How_to_do_system_input_wire_binding
   */
  sc_core::sc_export<xtsc_wire_write_if>& get_system_input_wire(const char *wire_name) const;


#ifndef DOXYGEN_SKIP
  // Deprecated: use get_system_input_wire()
  inline sc_core::sc_export<xtsc_wire_write_if>& get_input_wire(const char *wire_name) const {
    return get_system_input_wire(wire_name);
  }
#endif // DOXYGEN_SKIP


  /**
   * Instructions for doing system output wire port binding.
   *
   * The XTSC generic connection mechanism, when available, is the recommended approach
   * to perform port binding in XTSC.  This approach is available as long as both
   * modules implement the xtsc_connection_interface.  The XTSC generic connection
   * mechanism is primarily documented under xtsc_core::How_to_do_port_binding,
   * xtsc_connect(), xtsc::xtsc_port_type, and xtsc_connection_interface and is only
   * briefly mentioned here.
   *
   * As an alternative approach, the get_system_output_wire() method can be called by the
   * user to get a reference to the sc_port<xtsc_wire_write_if, NSPP> object that is the
   * xtsc_core's output port for the system output wire specified by wire_name.  
   *
   * Some of the things a system output wire can be bound to are:
   *    - An xtsc_component::xtsc_wire
   *    - An xtsc_component::xtsc_mmio input port
   *    - An xtsc_component::xtsc_wire_logic input port
   *    - A system input wire of an xtsc_core 
   *
   * One example would be for the user's code to instantiate an
   * xtsc_component::xtsc_wire object of the appropriate size, call
   * get_system_output_wire(), and then bind the returned
   * sc_port<xtsc_wire_write_if, NSPP> to the xtsc_component::xtsc_wire channel using
   * normal SystemC port binding: 
   *  \verbatim
      // Configuration parameters for an xtsc_wire
      xtsc_wire_parms wire_parms(1);

      // Construct the wire
      xtsc_wire PWaitMode("PWaitMode", wire_parms);

      // Connect the wire 
      core.get_system_output_wire("PWaitMode")(PWaitMode);
      \endverbatim
   *
   * Using the recommended XTSC generic connection mechanism, the last line above
   * would be replaced with the following call to xtsc_connect():
   *  \verbatim
        xtsc_connect(core, "PWaitMode", "wire_write", PWaitMode);
      \endverbatim
   *
   * See xtsc_core::How_to_do_tie_export_state_binding for ways to simplify the above
   * code using the convenience xtsc_component::xtsc_wire_parms::xtsc_wire_parms()
   * constructor.
   *
   * @see How_to_do_port_binding
   * @see xtsc_wire_write_if
   * @see get_sysio_bit_width
   * @see get_system_output_wire
   * @see get_system_input_wire
   * @see xtsc_component::xtsc_wire_parms::xtsc_wire_parms()
   * @see xtsc_component::xtsc_wire
   * @see How_to_do_tie_export_state_binding
   * @see xtsc_connect()
   * @see xtsc_port_type
   * @see xtsc_connection_interface
   */
  Readme How_to_do_system_output_wire_binding;


  /**
   * Method to get a reference to the sc_port for the named system output wire.
   *
   * @param     wire_name       The system output wire name as it appears in the Xtensa
   *                            microprocessor data book.  The possible system output
   *                            wire names are:
   *  \verbatim
                                        "CoreStatus"            (TX only)
                                        "CoreHalted"            (TX only)
                                        "DmaHighPriority"
                                        "PArithmeticException"
                                        "PWaitMode"
                                        "TrigOut_iDMA"
      \endverbatim
   *
   * Note: The DmaHighPriority output exists but is never driven as a TLM output.
   *       Instead, for TLM modelling use xtsc::xtsc_request::get_priority().  The
   *       output is driven, however, when modelled at pin-level (see
   *       "SimPinLevelInterfaces" in xtsc_core_parms).
   *
   * @see How_to_do_system_output_wire_binding
   */
  sc_core::sc_port<xtsc_wire_write_if, NSPP>& get_system_output_wire(const char *wire_name) const;


#ifndef DOXYGEN_SKIP
  // Deprecated: use get_system_output_wire()
  inline sc_core::sc_port<xtsc_wire_write_if, NSPP>& get_output_wire(const char *wire_name) const {
    return get_system_output_wire(wire_name);
  }
#endif // DOXYGEN_SKIP


  /**
   * Information on memory interface protocols.
   *
   * The purpose of these comments is to describe the protocols required when an
   * xtsc_core is communicating through one of the memory interfaces.  This section may
   * be of interest to the general XTSC user but is intended especially for developers
   * of modules that will connect to one or more of the memory interfaces.
   *
   * The memory interface types are:
   * -# Data RAM                        (DRAM)
   * -# Data ROM                        (DROM)
   * -# Instruction RAM                 (IRAM)
   * -# Instruction ROM                 (IROM)
   * -# Unified RAM                     (URAM)
   * -# Xtensa Local Memory Interface   (XLMI)
   * -# PIF-based System Interface      (PIF|iDMA)
   * -# AXI-base System Interface       (AXI|iDMA)
   * -# APB-base System Interface       (APB)
   * -# PIF-based Inbound Interface     (IPIF)
   * -# AXI-based Inbound Interface     (IAXI)
   * -# Inbound Snoop                   (snoop)         [Reserved for future use]
   *
   * The first 6 types above (DRAM, DROM, IRAM, IROM, URAM, and XLMI) are sometimes
   * referred to as the "local" memory interfaces.
   *
   * The xtsc_core class uses the xtsc_request_if and xtsc_respond_if interfaces classes
   * for communications through each of the memory interfaces.  The primary methods are
   * xtsc_request_if::nb_request() and xtsc_respond_if::nb_respond() which move a
   * payload object of class xtsc_request and xtsc_response, respectively.  This type of 
   * scheme is sometimes referred to as a "split transaction" protocol because each 
   * complete transaction (for example, a read transaction or a write transaction) is
   * split into two phases: a request phase (using the xtsc_request_if::nb_request()
   * method) and a response phase (using the xtsc_response_if::nb_respond() method).
   *
   * Note: Data object ownership: For all Xtensa TLM interface methods, the caller
   *       module owns the data object being passed by the interface and the callee
   *       module may make no assumptions about the data object's validity after the
   *       callee module returns from the interface method call.  If the callee module
   *       will need information that is stored in the data object after returning from
   *       the interface method call, then the callee module must make a copy of that
   *       information prior to returning from the interface method call.  As an
   *       example, if a memory module needs access to information in the xtsc_request
   *       object passed in to it via the xtsc_request_if::nb_request() call, then the
   *       memory module must copy that information prior to returning from the
   *       xtsc_request_if::nb_request() call.
   *
   * The xtsc_request class defines 8 request transaction types in xtsc_request::type_t
   * that are supported by xtsc_core.  The 8 types are READ, BLOCK_READ, BURST_READ,
   * RCW, WRITE, BLOCK_WRITE, BURST_WRITE, and SNOOP.  
   *
   * Note: Although, xtsc_core neither generates nor accepts BURST_WRITE and BURST_READ
   *       transactions on its PIF interfaces, it does accept 16-byte WRITE transactions
   *       on the inbound PIF interface with byte enables of 0x0FFF, 0xFFF0, and 0x0FF0.
   *
   * The following table shows which of the 8 request transaction types supported by
   * xtsc_core are allowed on a particular memory interface type:
   *
   *  \verbatim
              READ  BLOCK_READ  BURST_READ  RCW   WRITE  BLOCK_WRITE  BURST_WRITE  SNOOP
     DRAM     Yes       No          No      No     Yes       No           No        No
     DROM     Yes       No          No      No     No        No           No        No
     IRAM     Yes       No          No      No     Yes       No           No        No
     IROM     Yes       No          No      No     No        No           No        No
     URAM     Yes       No          No      No     Yes       No           No        No
     XLMI     Yes       No          No      No     Yes       No           No        No
  PIF|LX-iDMA Yes       Yes-1,6     Yes-6   Yes-3  Yes       Yes-2,6      Yes-6     No
  AXI|NX-iDMA No        No          Yes     No     No        No           Yes       No
     APB      Yes       No          No      No     Yes       No           No        No
     IPIF-5   Yes       Yes-4       No      Yes-4  Yes       Yes-4        No        No
     IAXI-5   No        No          Yes-4   No     No        No           Yes-4     No
     snoop    No        No          No      No     No        No           No        Yes
      \endverbatim
   *
   * Footnote 1:  The Xtensa core will only generate BLOCK_READ transactions for
   *              uncached read requests when the load width is wider than the PIF data 
   *              width and for cache line misses on processor configurations which have
   *              a cache line that is wider than the PIF data bus.
   *
   * Footnote 2:  The Xtensa core will only generate BLOCK_WRITE transactions for cache
   *              castouts and this will only occur on processor configurations which
   *              have a write-back cache whose cache line is wider than the PIF data
   *              bus.
   *
   * Footnote 3:  The Xtensa core will only generate RCW transactions for the S32C1I
   *              instruction and this can only occur on processors which have the
   *              Xtensa LX synchronization option.
   *
   * Footnote 4:  The Xtensa core will split BLOCK_READ, BURST_READ, RCW, BLOCK_WRITE,
   *              and BURST_WRITE requests received on an inbound interface into
   *              individual READ and WRITE requests as required before re-issuing them
   *              to the appropriate IRAM, DRAM, or XLMI memory.
   *
   * Footnote 5:  Inbound requests can target IRAM, DRAM, or XLMI if the processor was
   *              configured to allow inbound requests to that interface.  Inbound
   *              requests can never target IROM or DROM.
   *
   * Footnote 6:  Xtensa LX iDMA hardware prior to the RG-2017.8 release uses BLOCK_READ
   *              and BLOCK_WRITE transactions on its iDMA PIF port; while Xtensa LX
   *              iDMA hardware from RG-2017.8 and later uses BURST_READ and BURST_WRITE
   *              transactions on its iDMA PIF port.  Xtensa LX does not generate
   *              BURST_READ or BURST_WRITE transactions on its main (non-iDMA) PIF
   *              interface.  See "iDMAPIFBurst" in xtsc_core_parms.  A BURST_WRITE
   *              sequence consists of N consecutive BURST_WRITE request transfers
   *              (2<=N<=16).  A BURST_READ sequence consists of a single BURST_READ
   *              request for N response transfers (2<=N<=16).
   *
   * A BLOCK_WRITE sequence consists of 2, 4, 8, or 16 consecutive BLOCK_WRITE request
   * transfers.
   *
   * Each request on a memory interface should occur in a different clock cycle (that
   * is you should not submit two requests on the same memory interface in the same
   * clock cycle).
   *
   * Each non-RSP_NACC response on a memory interface should occur in a different clock
   * cycle (that is you should not submit two non-RSP_NACC responses on the same memory
   * interface in the same clock cycle).
   *
   * The xtsc_response class defines 5 response status values (in
   * xtsc_response::status_t).  The 5 status values are RSP_OK, RSP_ADDRESS_ERROR,
   * RSP_DATA_ERROR, RSP_ADDRESS_DATA_ERROR, and RSP_NACC.
   *
   * The xtsc_response::RSP_OK response status is used to indicate a successfully
   * completed READ, RCW, WRITE, or BLOCK_WRITE request.  It is also used with each
   * successful BLOCK_READ response.  If the response status is RSP_OK to a READ, RCW,
   * or BLOCK_READ request, then the response will also contain a data payload
   * (available using the xtsc_response.get_buffer() method).  An RSP_OK response status
   * to an RCW request does NOT mean that the write took place; to determine if the
   * write took place you must compare the data payload returned with the response to
   * the data value sent in the first RCW request.
   *
   * Note: The Xtensa Instruction Set Simulator (ISS) requires a response after each
   *       WRITE transaction on all memory interface and after the last transfer of a
   *       sequence of BLOCK_WRITE transfers on the PIF and iDMA interfaces.  This rule
   *       applies even to configurations that don't have the write response option.
   *       The "SimPIFFakeWriteResponses" parameter controls whether or not xtsc_core
   *       will automatically supply write responses for requests on the PIF and iDMA
   *       interfaces on configurations without the write response option.
   *
   * The xtsc_response::RSP_NACC response status models the busy interface of a local
   * memory interface and it models the ReqRdy interface on the PIF|iDMA and inbound PIF
   * interfaces, respectively.  See below for special timing requirements of RSP_NACC.
   *
   * The xtsc_response::RSP_NACC response status is never used on an APB interface.  The
   * PREADY APB hardware signal is construed to be high during the clock period in which
   * nb_respond() is called and it is construed to be low at all other times.
   *
   * The xtsc_response::NOTRDY response status (which has the same value as RSP_NACC)
   * models the READY interface being deasserted (low) on the AXI and IAXI interfaces.
   *
   * An error response status (xtsc_response::RSP_ADDRESS_ERROR,
   * xtsc_response::RSP_DATA_ERROR, and xtsc_response::RSP_ADDRESS_DATA_ERROR) typically
   * results in an exception being raised on the processor that issued the request.
   *
   * The nb_respond() return value is used to model the PORespRdy/PIRespRdy interface on
   * the PIF and inbound PIF interfaces, respectively.  For the local memory interfaces,
   * xtsc_core always returns true to the nb_respond() call.
   * 
   * Port Associated With Each Xtensa TLM Call of xtsc_request_if and xtsc_debug_if:
   * 
   * The xtsc_core model always sends xtsc_request_if::nb_request() calls out the port
   * associated with the hardware operation being modeled.  For the
   * xtsc_debug_if::nb_peek(), xtsc_debug_if::nb_poke, and xtsc_debug_if::nb_fast_access
   * calls associated with multi-ported, non-banked local data memories there is some
   * ambiguity in which port to use because the port is associated with the hardware
   * origin of the call (e.g. which Ld/St unit) and there is no fixed concept of the
   * "hardware origin" of the xtsc_debug_if calls.  In these cases, xtsc_core sends the
   * call out the lowest numbered port as shown here:
   *  \verbatim
      Memory Interface     Non-Split R/W    Split R/W
      dram0                "dram0ls0"       "dram0ls0rd"
      dram1                "dram1ls0"       "dram1ls0rd"
      drom0                "drom0ls0"
      xlmi                 "xlmi0ls0"
      \endverbatim
   *
   * ISS Timing Requirements:
   *
   * Note:  In the following discussion, a deadline of Phase A means the nb_respond()
   *        call must occur PRIOR to Phase A.  See set_clock_phase_delta_factors() for
   *        more information about clock phases.
   *
   * ISS Timing Requirements for local memories:
   * The deadline for an xtsc_response::RSP_OK response from a local memory for a
   * request that was made in clock cycle N is Phase A of clock cycle N+1 for a 5-stage
   * pipeline and is Phase A of clock cycle N+2 for a 7-stage pipeline.  The deadline
   * for an xtsc_response::RSP_NACC (to model the Busy signal) for a request that was
   * made in clock cycle N is Phase A of clock cycle N+1 for both 5-stage and 7-stage
   * pipelines.
   *
   * ISS Timing Requirements for system memory interfaces:
   * The deadline for a xtsc_response::RSP_NACC or xtsc_response::NOTRDY on a system
   * memory interface for a request that was made at Phase B of clock cycle N is Phase A
   * of clock cycle N+1 regardless of the number of pipeline stages.  Responses with a
   * status_t of other than xtsc_response::RSP_NACC and xtsc_response::NOTRDY do not
   * have a deadline.
   *
   * The above ISS timing requirements are more lenient then the real hardware timing 
   * requirements in the following ways:
   *  
   *  1. An early response is accepted by the ISS as though it occurred at the proper
   *     time.  For example, local memory hardware timing requirements for a read
   *     request which occured in clock cycle N dictate that the response occur in clock
   *     cycle N+1 (5-stage pipeline) or clock cycle N+2 (7-stage pipeline).  However,
   *     the ISS will accept the response in clock cycle N.  Of course, the response can
   *     never precede the request.
   *
   *  2. Although hardware timing requirements dictate that a xtsc_response::RSP_NACC or
   *     xtsc_response::NOTRDY to a PIF|AXI|iDMA request occur in the same clock cycle
   *     (say clock cycle N) as the request, the ISS will accept a slightly late
   *     xtsc_response::RSP_NACC or xtsc_response::NOTRDY as long as it occurs prior to
   *     Phase A of clock cycle N+1.
   *
   * @see xtsc_request_if::nb_request
   * @see xtsc_respond_if::nb_respond
   * @see xtsc_request
   * @see xtsc_response
   * @see set_clock_phase_delta_factors
   * @see How_to_do_memory_port_binding
   */
  Readme Information_on_memory_interface_protocols;


  // SystemC needs this.
  SC_HAS_PROCESS(xtsc_core);


  /// Our C++ type (SystemC uses this)
  virtual const char* kind() const { return "xtsc_core"; }


  /**
   * Constructor for an xtsc_core.
   *
   * @param module_name   Name of the xtsc_core sc_module.
   *
   * @param core_parms    The configuration parameters.
   *
   * @see xtsc_core_parms
   */
  xtsc_core(sc_core::sc_module_name module_name, const xtsc_core_parms &core_parms);


  ~xtsc_core();


  /**
   * Method to change the clock period.
   *
   * @param     clock_period_factor     Specifies the new length of this core's clock
   *                                    period expressed in terms of the SystemC time
   *                                    resolution (from sc_get_time_resolution()).
   *                                    The clock_period_factor must be an integer
   *                                    multiple of the XTSC system clock factor so that
   *                                    the resulting clock period will be an integer
   *                                    multiple of the XTSC system clock period.
   *
   * Note: Changing the clock period of a module should generally only be attempted
   *       when that module is quiescent (for example, when a core is in waiti).
   *       Changing the clock period while a core has a TIE lookup pending or is
   *       receiving a sequence of either BLOCK_READ|BURST_READ responses on its
   *       PIF|AXI|iDMA interface or BLOCK_WRITE|BURST_WRITE requests on its inbound PIF
   *       or Data/Inst slave interfaces can potentially cause simulation failures.
   *
   * Note: Changing the clock period is not supported for SystemC-Verilog cosimulation.
   *
   * Note: Because of these issues, changing the clock period is a feature which must be
   *       explicitly enabled by setting the "enable_dynamic_clock_period" parameter of
   *       xtsc_initialize_parms to true.
   *
   * @see xtsc_get_system_clock_factor
   * @see xtsc_get_system_clock_period
   * @see the xtsc_initialize_parms parameter "enable_dynamic_clock_period"
   */
  void change_clock_period(u32 clock_period_factor);


  /**
   * Method to get the clock period factor (typically 1000).
   *
   * If change_clock_period() has been called, then this method returns the value of the
   * argument of the most recent previous call to change_clock_period().  Otherwise, it
   * returns a value equal to the value set by xtsc_core_parms "SimClockFactor" multiplied
   * by the value returned by the xtsc_get_system_clock_factor() function.
   *
   * @see change_clock_period
   * @see get_clock_factor
   * @see xtsc_get_system_clock_factor
   */
  u32 get_clock_period_factor() const;


  /**
   * Method to get the clock factor (typically 1).
   *
   * If change_clock_period() has been called, then this method returns a value equal to 
   * the argument of the most recent previous call to change_clock_period() divided by
   * the value returned by the xtsc_get_system_clock_factor() function.  Otherwise, it
   * returns the value set by xtsc_core_parms "SimClockFactor".
   *
   * @see change_clock_period
   * @see get_clock_period_factor
   * @see xtsc_get_system_clock_factor
   */
  u32 get_clock_factor() const;


  /// For xtsc_connection_interface
  virtual u32 get_bit_width(const std::string& port_name, u32 interface_num = 0) const;


  /// For xtsc_connection_interface
  virtual sc_core::sc_object *get_port(const std::string& port_name);


  /// For xtsc_connection_interface
  virtual std::string get_default_port_name() const;


  /// For xtsc_connection_interface
  virtual xtsc_port_table get_port_table(const std::string& port_table_name) const;


  /**
   *  Type used to identify a memory port on the Xtensa core.
   *
   *  Note: By contract:
   *  \verbatim
      - LS1 ports are always 1 greater than LS0 ports.
      - LS2 ports are always 1 greater than LS1 ports.
      - Similarly for P3, P2, P1, and P0 ports.
      - Subbanks of a bank always have consecutively numbered enum values.
      - Corresponding subbanks of adjacent banks are always separated in enum value
        by MEM_SUBBANK_DELTA (whose value may change in future releases).
      - For split R/W ports the write port (MEM_DR*M*WR) is always
        MEM_SPLIT_WR_DELTA greater then its corresponding read port (MEM_DR*M*RD).
      \endverbatim
   */
  enum memory_port {

    MEM_DRAM0P0 = 0,      ///<  Data RAM 0 Bank 0 or L/S 0
    MEM_DRAM0P1,          ///<  Data RAM 0 Bank 1 or L/S 1
    MEM_DRAM0P2,          ///<  Data RAM 0 Bank 2
    MEM_DRAM0P3,          ///<  Data RAM 0 Bank 3

    MEM_DRAM0LS0RD,       ///<  Data RAM 0 L/S 0 Read  port
    MEM_DRAM0LS1RD,       ///<  Data RAM 0 L/S 1 Read  port
    MEM_DRAM0LS2RD,       ///<  Data RAM 0 L/S 2 Read  port
    MEM_DRAM0DMARD,       ///<  Data RAM 0 DMA   Read  port

    MEM_DRAM0LS0WR,       ///<  Data RAM 0 L/S 0 Write port
    MEM_DRAM0LS1WR,       ///<  Data RAM 0 L/S 1 Write port
    MEM_DRAM0LS2WR,       ///<  Data RAM 0 L/S 2 Write port
    MEM_DRAM0DMAWR,       ///<  Data RAM 0 DMA   Write port

    MEM_DRAM0B0S00,       ///<  Data RAM 0 Bank 0 SubBank  0
    MEM_DRAM0B0S01,       ///<  Data RAM 0 Bank 0 SubBank  1
    MEM_DRAM0B0S02,       ///<  Data RAM 0 Bank 0 SubBank  2
    MEM_DRAM0B0S03,       ///<  Data RAM 0 Bank 0 SubBank  3
    MEM_DRAM0B0S04,       ///<  Data RAM 0 Bank 0 SubBank  4
    MEM_DRAM0B0S05,       ///<  Data RAM 0 Bank 0 SubBank  5
    MEM_DRAM0B0S06,       ///<  Data RAM 0 Bank 0 SubBank  6
    MEM_DRAM0B0S07,       ///<  Data RAM 0 Bank 0 SubBank  7
    MEM_DRAM0B0S08,       ///<  Data RAM 0 Bank 0 SubBank  8
    MEM_DRAM0B0S09,       ///<  Data RAM 0 Bank 0 SubBank  9
    MEM_DRAM0B0S10,       ///<  Data RAM 0 Bank 0 SubBank 10
    MEM_DRAM0B0S11,       ///<  Data RAM 0 Bank 0 SubBank 11
    MEM_DRAM0B0S12,       ///<  Data RAM 0 Bank 0 SubBank 12
    MEM_DRAM0B0S13,       ///<  Data RAM 0 Bank 0 SubBank 13
    MEM_DRAM0B0S14,       ///<  Data RAM 0 Bank 0 SubBank 14
    MEM_DRAM0B0S15,       ///<  Data RAM 0 Bank 0 SubBank 15
    MEM_DRAM0B0S16,       ///<  Data RAM 0 Bank 0 SubBank 16
    MEM_DRAM0B0S17,       ///<  Data RAM 0 Bank 0 SubBank 17
    MEM_DRAM0B0S18,       ///<  Data RAM 0 Bank 0 SubBank 18
    MEM_DRAM0B0S19,       ///<  Data RAM 0 Bank 0 SubBank 19
    MEM_DRAM0B0S20,       ///<  Data RAM 0 Bank 0 SubBank 20
    MEM_DRAM0B0S21,       ///<  Data RAM 0 Bank 0 SubBank 21
    MEM_DRAM0B0S22,       ///<  Data RAM 0 Bank 0 SubBank 22
    MEM_DRAM0B0S23,       ///<  Data RAM 0 Bank 0 SubBank 23
    MEM_DRAM0B0S24,       ///<  Data RAM 0 Bank 0 SubBank 24
    MEM_DRAM0B0S25,       ///<  Data RAM 0 Bank 0 SubBank 25
    MEM_DRAM0B0S26,       ///<  Data RAM 0 Bank 0 SubBank 26
    MEM_DRAM0B0S27,       ///<  Data RAM 0 Bank 0 SubBank 27
    MEM_DRAM0B0S28,       ///<  Data RAM 0 Bank 0 SubBank 28
    MEM_DRAM0B0S29,       ///<  Data RAM 0 Bank 0 SubBank 29
    MEM_DRAM0B0S30,       ///<  Data RAM 0 Bank 0 SubBank 30
    MEM_DRAM0B0S31,       ///<  Data RAM 0 Bank 0 SubBank 31
    MEM_DRAM0B1S00,       ///<  Data RAM 0 Bank 1 SubBank  0
    MEM_DRAM0B1S01,       ///<  Data RAM 0 Bank 1 SubBank  1
    MEM_DRAM0B1S02,       ///<  Data RAM 0 Bank 1 SubBank  2
    MEM_DRAM0B1S03,       ///<  Data RAM 0 Bank 1 SubBank  3
    MEM_DRAM0B1S04,       ///<  Data RAM 0 Bank 1 SubBank  4
    MEM_DRAM0B1S05,       ///<  Data RAM 0 Bank 1 SubBank  5
    MEM_DRAM0B1S06,       ///<  Data RAM 0 Bank 1 SubBank  6
    MEM_DRAM0B1S07,       ///<  Data RAM 0 Bank 1 SubBank  7
    MEM_DRAM0B1S08,       ///<  Data RAM 0 Bank 1 SubBank  8
    MEM_DRAM0B1S09,       ///<  Data RAM 0 Bank 1 SubBank  9
    MEM_DRAM0B1S10,       ///<  Data RAM 0 Bank 1 SubBank 10
    MEM_DRAM0B1S11,       ///<  Data RAM 0 Bank 1 SubBank 11
    MEM_DRAM0B1S12,       ///<  Data RAM 0 Bank 1 SubBank 12
    MEM_DRAM0B1S13,       ///<  Data RAM 0 Bank 1 SubBank 13
    MEM_DRAM0B1S14,       ///<  Data RAM 0 Bank 1 SubBank 14
    MEM_DRAM0B1S15,       ///<  Data RAM 0 Bank 1 SubBank 15
    MEM_DRAM0B1S16,       ///<  Data RAM 0 Bank 1 SubBank 16
    MEM_DRAM0B1S17,       ///<  Data RAM 0 Bank 1 SubBank 17
    MEM_DRAM0B1S18,       ///<  Data RAM 0 Bank 1 SubBank 18
    MEM_DRAM0B1S19,       ///<  Data RAM 0 Bank 1 SubBank 19
    MEM_DRAM0B1S20,       ///<  Data RAM 0 Bank 1 SubBank 20
    MEM_DRAM0B1S21,       ///<  Data RAM 0 Bank 1 SubBank 21
    MEM_DRAM0B1S22,       ///<  Data RAM 0 Bank 1 SubBank 22
    MEM_DRAM0B1S23,       ///<  Data RAM 0 Bank 1 SubBank 23
    MEM_DRAM0B1S24,       ///<  Data RAM 0 Bank 1 SubBank 24
    MEM_DRAM0B1S25,       ///<  Data RAM 0 Bank 1 SubBank 25
    MEM_DRAM0B1S26,       ///<  Data RAM 0 Bank 1 SubBank 26
    MEM_DRAM0B1S27,       ///<  Data RAM 0 Bank 1 SubBank 27
    MEM_DRAM0B1S28,       ///<  Data RAM 0 Bank 1 SubBank 28
    MEM_DRAM0B1S29,       ///<  Data RAM 0 Bank 1 SubBank 29
    MEM_DRAM0B1S30,       ///<  Data RAM 0 Bank 1 SubBank 30
    MEM_DRAM0B1S31,       ///<  Data RAM 0 Bank 1 SubBank 31
    MEM_DRAM0B2S00,       ///<  Data RAM 0 Bank 2 SubBank  0
    MEM_DRAM0B2S01,       ///<  Data RAM 0 Bank 2 SubBank  1
    MEM_DRAM0B2S02,       ///<  Data RAM 0 Bank 2 SubBank  2
    MEM_DRAM0B2S03,       ///<  Data RAM 0 Bank 2 SubBank  3
    MEM_DRAM0B2S04,       ///<  Data RAM 0 Bank 2 SubBank  4
    MEM_DRAM0B2S05,       ///<  Data RAM 0 Bank 2 SubBank  5
    MEM_DRAM0B2S06,       ///<  Data RAM 0 Bank 2 SubBank  6
    MEM_DRAM0B2S07,       ///<  Data RAM 0 Bank 2 SubBank  7
    MEM_DRAM0B2S08,       ///<  Data RAM 0 Bank 2 SubBank  8
    MEM_DRAM0B2S09,       ///<  Data RAM 0 Bank 2 SubBank  9
    MEM_DRAM0B2S10,       ///<  Data RAM 0 Bank 2 SubBank 10
    MEM_DRAM0B2S11,       ///<  Data RAM 0 Bank 2 SubBank 11
    MEM_DRAM0B2S12,       ///<  Data RAM 0 Bank 2 SubBank 12
    MEM_DRAM0B2S13,       ///<  Data RAM 0 Bank 2 SubBank 13
    MEM_DRAM0B2S14,       ///<  Data RAM 0 Bank 2 SubBank 14
    MEM_DRAM0B2S15,       ///<  Data RAM 0 Bank 2 SubBank 15
    MEM_DRAM0B2S16,       ///<  Data RAM 0 Bank 2 SubBank 16
    MEM_DRAM0B2S17,       ///<  Data RAM 0 Bank 2 SubBank 17
    MEM_DRAM0B2S18,       ///<  Data RAM 0 Bank 2 SubBank 18
    MEM_DRAM0B2S19,       ///<  Data RAM 0 Bank 2 SubBank 19
    MEM_DRAM0B2S20,       ///<  Data RAM 0 Bank 2 SubBank 20
    MEM_DRAM0B2S21,       ///<  Data RAM 0 Bank 2 SubBank 21
    MEM_DRAM0B2S22,       ///<  Data RAM 0 Bank 2 SubBank 22
    MEM_DRAM0B2S23,       ///<  Data RAM 0 Bank 2 SubBank 23
    MEM_DRAM0B2S24,       ///<  Data RAM 0 Bank 2 SubBank 24
    MEM_DRAM0B2S25,       ///<  Data RAM 0 Bank 2 SubBank 25
    MEM_DRAM0B2S26,       ///<  Data RAM 0 Bank 2 SubBank 26
    MEM_DRAM0B2S27,       ///<  Data RAM 0 Bank 2 SubBank 27
    MEM_DRAM0B2S28,       ///<  Data RAM 0 Bank 2 SubBank 28
    MEM_DRAM0B2S29,       ///<  Data RAM 0 Bank 2 SubBank 29
    MEM_DRAM0B2S30,       ///<  Data RAM 0 Bank 2 SubBank 30
    MEM_DRAM0B2S31,       ///<  Data RAM 0 Bank 2 SubBank 31
    MEM_DRAM0B3S00,       ///<  Data RAM 0 Bank 3 SubBank  0
    MEM_DRAM0B3S01,       ///<  Data RAM 0 Bank 3 SubBank  1
    MEM_DRAM0B3S02,       ///<  Data RAM 0 Bank 3 SubBank  2
    MEM_DRAM0B3S03,       ///<  Data RAM 0 Bank 3 SubBank  3
    MEM_DRAM0B3S04,       ///<  Data RAM 0 Bank 3 SubBank  4
    MEM_DRAM0B3S05,       ///<  Data RAM 0 Bank 3 SubBank  5
    MEM_DRAM0B3S06,       ///<  Data RAM 0 Bank 3 SubBank  6
    MEM_DRAM0B3S07,       ///<  Data RAM 0 Bank 3 SubBank  7
    MEM_DRAM0B3S08,       ///<  Data RAM 0 Bank 3 SubBank  8
    MEM_DRAM0B3S09,       ///<  Data RAM 0 Bank 3 SubBank  9
    MEM_DRAM0B3S10,       ///<  Data RAM 0 Bank 3 SubBank 10
    MEM_DRAM0B3S11,       ///<  Data RAM 0 Bank 3 SubBank 11
    MEM_DRAM0B3S12,       ///<  Data RAM 0 Bank 3 SubBank 12
    MEM_DRAM0B3S13,       ///<  Data RAM 0 Bank 3 SubBank 13
    MEM_DRAM0B3S14,       ///<  Data RAM 0 Bank 3 SubBank 14
    MEM_DRAM0B3S15,       ///<  Data RAM 0 Bank 3 SubBank 15
    MEM_DRAM0B3S16,       ///<  Data RAM 0 Bank 3 SubBank 16
    MEM_DRAM0B3S17,       ///<  Data RAM 0 Bank 3 SubBank 17
    MEM_DRAM0B3S18,       ///<  Data RAM 0 Bank 3 SubBank 18
    MEM_DRAM0B3S19,       ///<  Data RAM 0 Bank 3 SubBank 19
    MEM_DRAM0B3S20,       ///<  Data RAM 0 Bank 3 SubBank 20
    MEM_DRAM0B3S21,       ///<  Data RAM 0 Bank 3 SubBank 21
    MEM_DRAM0B3S22,       ///<  Data RAM 0 Bank 3 SubBank 22
    MEM_DRAM0B3S23,       ///<  Data RAM 0 Bank 3 SubBank 23
    MEM_DRAM0B3S24,       ///<  Data RAM 0 Bank 3 SubBank 24
    MEM_DRAM0B3S25,       ///<  Data RAM 0 Bank 3 SubBank 25
    MEM_DRAM0B3S26,       ///<  Data RAM 0 Bank 3 SubBank 26
    MEM_DRAM0B3S27,       ///<  Data RAM 0 Bank 3 SubBank 27
    MEM_DRAM0B3S28,       ///<  Data RAM 0 Bank 3 SubBank 28
    MEM_DRAM0B3S29,       ///<  Data RAM 0 Bank 3 SubBank 29
    MEM_DRAM0B3S30,       ///<  Data RAM 0 Bank 3 SubBank 30
    MEM_DRAM0B3S31,       ///<  Data RAM 0 Bank 3 SubBank 31

    MEM_DRAM1P0,          ///<  Data RAM 1 Bank 0 or L/S 0
    MEM_DRAM1P1,          ///<  Data RAM 1 Bank 1 or L/S 1
    MEM_DRAM1P2,          ///<  Data RAM 1 Bank 2
    MEM_DRAM1P3,          ///<  Data RAM 1 Bank 3

    MEM_DRAM1LS0RD,       ///<  Data RAM 1 L/S 0 Read  port
    MEM_DRAM1LS1RD,       ///<  Data RAM 1 L/S 1 Read  port
    MEM_DRAM1LS2RD,       ///<  Data RAM 1 L/S 2 Read  port
    MEM_DRAM1DMARD,       ///<  Data RAM 1 DMA   Read  port

    MEM_DRAM1LS0WR,       ///<  Data RAM 1 L/S 0 Write port
    MEM_DRAM1LS1WR,       ///<  Data RAM 1 L/S 1 Write port
    MEM_DRAM1LS2WR,       ///<  Data RAM 1 L/S 2 Write port
    MEM_DRAM1DMAWR,       ///<  Data RAM 1 DMA   Write port

    MEM_DRAM1B0S00,       ///<  Data RAM 1 Bank 0 SubBank  0
    MEM_DRAM1B0S01,       ///<  Data RAM 1 Bank 0 SubBank  1
    MEM_DRAM1B0S02,       ///<  Data RAM 1 Bank 0 SubBank  2
    MEM_DRAM1B0S03,       ///<  Data RAM 1 Bank 0 SubBank  3
    MEM_DRAM1B0S04,       ///<  Data RAM 1 Bank 0 SubBank  4
    MEM_DRAM1B0S05,       ///<  Data RAM 1 Bank 0 SubBank  5
    MEM_DRAM1B0S06,       ///<  Data RAM 1 Bank 0 SubBank  6
    MEM_DRAM1B0S07,       ///<  Data RAM 1 Bank 0 SubBank  7
    MEM_DRAM1B0S08,       ///<  Data RAM 1 Bank 0 SubBank  8
    MEM_DRAM1B0S09,       ///<  Data RAM 1 Bank 0 SubBank  9
    MEM_DRAM1B0S10,       ///<  Data RAM 1 Bank 0 SubBank 10
    MEM_DRAM1B0S11,       ///<  Data RAM 1 Bank 0 SubBank 11
    MEM_DRAM1B0S12,       ///<  Data RAM 1 Bank 0 SubBank 12
    MEM_DRAM1B0S13,       ///<  Data RAM 1 Bank 0 SubBank 13
    MEM_DRAM1B0S14,       ///<  Data RAM 1 Bank 0 SubBank 14
    MEM_DRAM1B0S15,       ///<  Data RAM 1 Bank 0 SubBank 15
    MEM_DRAM1B0S16,       ///<  Data RAM 1 Bank 0 SubBank 16
    MEM_DRAM1B0S17,       ///<  Data RAM 1 Bank 0 SubBank 17
    MEM_DRAM1B0S18,       ///<  Data RAM 1 Bank 0 SubBank 18
    MEM_DRAM1B0S19,       ///<  Data RAM 1 Bank 0 SubBank 19
    MEM_DRAM1B0S20,       ///<  Data RAM 1 Bank 0 SubBank 20
    MEM_DRAM1B0S21,       ///<  Data RAM 1 Bank 0 SubBank 21
    MEM_DRAM1B0S22,       ///<  Data RAM 1 Bank 0 SubBank 22
    MEM_DRAM1B0S23,       ///<  Data RAM 1 Bank 0 SubBank 23
    MEM_DRAM1B0S24,       ///<  Data RAM 1 Bank 0 SubBank 24
    MEM_DRAM1B0S25,       ///<  Data RAM 1 Bank 0 SubBank 25
    MEM_DRAM1B0S26,       ///<  Data RAM 1 Bank 0 SubBank 26
    MEM_DRAM1B0S27,       ///<  Data RAM 1 Bank 0 SubBank 27
    MEM_DRAM1B0S28,       ///<  Data RAM 1 Bank 0 SubBank 28
    MEM_DRAM1B0S29,       ///<  Data RAM 1 Bank 0 SubBank 29
    MEM_DRAM1B0S30,       ///<  Data RAM 1 Bank 0 SubBank 30
    MEM_DRAM1B0S31,       ///<  Data RAM 1 Bank 0 SubBank 31
    MEM_DRAM1B1S00,       ///<  Data RAM 1 Bank 1 SubBank  0
    MEM_DRAM1B1S01,       ///<  Data RAM 1 Bank 1 SubBank  1
    MEM_DRAM1B1S02,       ///<  Data RAM 1 Bank 1 SubBank  2
    MEM_DRAM1B1S03,       ///<  Data RAM 1 Bank 1 SubBank  3
    MEM_DRAM1B1S04,       ///<  Data RAM 1 Bank 1 SubBank  4
    MEM_DRAM1B1S05,       ///<  Data RAM 1 Bank 1 SubBank  5
    MEM_DRAM1B1S06,       ///<  Data RAM 1 Bank 1 SubBank  6
    MEM_DRAM1B1S07,       ///<  Data RAM 1 Bank 1 SubBank  7
    MEM_DRAM1B1S08,       ///<  Data RAM 1 Bank 1 SubBank  8
    MEM_DRAM1B1S09,       ///<  Data RAM 1 Bank 1 SubBank  9
    MEM_DRAM1B1S10,       ///<  Data RAM 1 Bank 1 SubBank 10
    MEM_DRAM1B1S11,       ///<  Data RAM 1 Bank 1 SubBank 11
    MEM_DRAM1B1S12,       ///<  Data RAM 1 Bank 1 SubBank 12
    MEM_DRAM1B1S13,       ///<  Data RAM 1 Bank 1 SubBank 13
    MEM_DRAM1B1S14,       ///<  Data RAM 1 Bank 1 SubBank 14
    MEM_DRAM1B1S15,       ///<  Data RAM 1 Bank 1 SubBank 15
    MEM_DRAM1B1S16,       ///<  Data RAM 1 Bank 1 SubBank 16
    MEM_DRAM1B1S17,       ///<  Data RAM 1 Bank 1 SubBank 17
    MEM_DRAM1B1S18,       ///<  Data RAM 1 Bank 1 SubBank 18
    MEM_DRAM1B1S19,       ///<  Data RAM 1 Bank 1 SubBank 19
    MEM_DRAM1B1S20,       ///<  Data RAM 1 Bank 1 SubBank 20
    MEM_DRAM1B1S21,       ///<  Data RAM 1 Bank 1 SubBank 21
    MEM_DRAM1B1S22,       ///<  Data RAM 1 Bank 1 SubBank 22
    MEM_DRAM1B1S23,       ///<  Data RAM 1 Bank 1 SubBank 23
    MEM_DRAM1B1S24,       ///<  Data RAM 1 Bank 1 SubBank 24
    MEM_DRAM1B1S25,       ///<  Data RAM 1 Bank 1 SubBank 25
    MEM_DRAM1B1S26,       ///<  Data RAM 1 Bank 1 SubBank 26
    MEM_DRAM1B1S27,       ///<  Data RAM 1 Bank 1 SubBank 27
    MEM_DRAM1B1S28,       ///<  Data RAM 1 Bank 1 SubBank 28
    MEM_DRAM1B1S29,       ///<  Data RAM 1 Bank 1 SubBank 29
    MEM_DRAM1B1S30,       ///<  Data RAM 1 Bank 1 SubBank 30
    MEM_DRAM1B1S31,       ///<  Data RAM 1 Bank 1 SubBank 31
    MEM_DRAM1B2S00,       ///<  Data RAM 1 Bank 2 SubBank  0
    MEM_DRAM1B2S01,       ///<  Data RAM 1 Bank 2 SubBank  1
    MEM_DRAM1B2S02,       ///<  Data RAM 1 Bank 2 SubBank  2
    MEM_DRAM1B2S03,       ///<  Data RAM 1 Bank 2 SubBank  3
    MEM_DRAM1B2S04,       ///<  Data RAM 1 Bank 2 SubBank  4
    MEM_DRAM1B2S05,       ///<  Data RAM 1 Bank 2 SubBank  5
    MEM_DRAM1B2S06,       ///<  Data RAM 1 Bank 2 SubBank  6
    MEM_DRAM1B2S07,       ///<  Data RAM 1 Bank 2 SubBank  7
    MEM_DRAM1B2S08,       ///<  Data RAM 1 Bank 2 SubBank  8
    MEM_DRAM1B2S09,       ///<  Data RAM 1 Bank 2 SubBank  9
    MEM_DRAM1B2S10,       ///<  Data RAM 1 Bank 2 SubBank 10
    MEM_DRAM1B2S11,       ///<  Data RAM 1 Bank 2 SubBank 11
    MEM_DRAM1B2S12,       ///<  Data RAM 1 Bank 2 SubBank 12
    MEM_DRAM1B2S13,       ///<  Data RAM 1 Bank 2 SubBank 13
    MEM_DRAM1B2S14,       ///<  Data RAM 1 Bank 2 SubBank 14
    MEM_DRAM1B2S15,       ///<  Data RAM 1 Bank 2 SubBank 15
    MEM_DRAM1B2S16,       ///<  Data RAM 1 Bank 2 SubBank 16
    MEM_DRAM1B2S17,       ///<  Data RAM 1 Bank 2 SubBank 17
    MEM_DRAM1B2S18,       ///<  Data RAM 1 Bank 2 SubBank 18
    MEM_DRAM1B2S19,       ///<  Data RAM 1 Bank 2 SubBank 19
    MEM_DRAM1B2S20,       ///<  Data RAM 1 Bank 2 SubBank 20
    MEM_DRAM1B2S21,       ///<  Data RAM 1 Bank 2 SubBank 21
    MEM_DRAM1B2S22,       ///<  Data RAM 1 Bank 2 SubBank 22
    MEM_DRAM1B2S23,       ///<  Data RAM 1 Bank 2 SubBank 23
    MEM_DRAM1B2S24,       ///<  Data RAM 1 Bank 2 SubBank 24
    MEM_DRAM1B2S25,       ///<  Data RAM 1 Bank 2 SubBank 25
    MEM_DRAM1B2S26,       ///<  Data RAM 1 Bank 2 SubBank 26
    MEM_DRAM1B2S27,       ///<  Data RAM 1 Bank 2 SubBank 27
    MEM_DRAM1B2S28,       ///<  Data RAM 1 Bank 2 SubBank 28
    MEM_DRAM1B2S29,       ///<  Data RAM 1 Bank 2 SubBank 29
    MEM_DRAM1B2S30,       ///<  Data RAM 1 Bank 2 SubBank 30
    MEM_DRAM1B2S31,       ///<  Data RAM 1 Bank 2 SubBank 31
    MEM_DRAM1B3S00,       ///<  Data RAM 1 Bank 3 SubBank  0
    MEM_DRAM1B3S01,       ///<  Data RAM 1 Bank 3 SubBank  1
    MEM_DRAM1B3S02,       ///<  Data RAM 1 Bank 3 SubBank  2
    MEM_DRAM1B3S03,       ///<  Data RAM 1 Bank 3 SubBank  3
    MEM_DRAM1B3S04,       ///<  Data RAM 1 Bank 3 SubBank  4
    MEM_DRAM1B3S05,       ///<  Data RAM 1 Bank 3 SubBank  5
    MEM_DRAM1B3S06,       ///<  Data RAM 1 Bank 3 SubBank  6
    MEM_DRAM1B3S07,       ///<  Data RAM 1 Bank 3 SubBank  7
    MEM_DRAM1B3S08,       ///<  Data RAM 1 Bank 3 SubBank  8
    MEM_DRAM1B3S09,       ///<  Data RAM 1 Bank 3 SubBank  9
    MEM_DRAM1B3S10,       ///<  Data RAM 1 Bank 3 SubBank 10
    MEM_DRAM1B3S11,       ///<  Data RAM 1 Bank 3 SubBank 11
    MEM_DRAM1B3S12,       ///<  Data RAM 1 Bank 3 SubBank 12
    MEM_DRAM1B3S13,       ///<  Data RAM 1 Bank 3 SubBank 13
    MEM_DRAM1B3S14,       ///<  Data RAM 1 Bank 3 SubBank 14
    MEM_DRAM1B3S15,       ///<  Data RAM 1 Bank 3 SubBank 15
    MEM_DRAM1B3S16,       ///<  Data RAM 1 Bank 3 SubBank 16
    MEM_DRAM1B3S17,       ///<  Data RAM 1 Bank 3 SubBank 17
    MEM_DRAM1B3S18,       ///<  Data RAM 1 Bank 3 SubBank 18
    MEM_DRAM1B3S19,       ///<  Data RAM 1 Bank 3 SubBank 19
    MEM_DRAM1B3S20,       ///<  Data RAM 1 Bank 3 SubBank 20
    MEM_DRAM1B3S21,       ///<  Data RAM 1 Bank 3 SubBank 21
    MEM_DRAM1B3S22,       ///<  Data RAM 1 Bank 3 SubBank 22
    MEM_DRAM1B3S23,       ///<  Data RAM 1 Bank 3 SubBank 23
    MEM_DRAM1B3S24,       ///<  Data RAM 1 Bank 3 SubBank 24
    MEM_DRAM1B3S25,       ///<  Data RAM 1 Bank 3 SubBank 25
    MEM_DRAM1B3S26,       ///<  Data RAM 1 Bank 3 SubBank 26
    MEM_DRAM1B3S27,       ///<  Data RAM 1 Bank 3 SubBank 27
    MEM_DRAM1B3S28,       ///<  Data RAM 1 Bank 3 SubBank 28
    MEM_DRAM1B3S29,       ///<  Data RAM 1 Bank 3 SubBank 29
    MEM_DRAM1B3S30,       ///<  Data RAM 1 Bank 3 SubBank 30
    MEM_DRAM1B3S31,       ///<  Data RAM 1 Bank 3 SubBank 31

    MEM_DROM0P0,          ///<  Data ROM   Bank 0 or L/S 0
    MEM_DROM0P1,          ///<  Data ROM   Bank 1 or L/S 1
    MEM_DROM0P2,          ///<  Data ROM   Bank 2 or L/S 2
    MEM_DROM0P3,          ///<  Data ROM   Bank 3

    MEM_IRAM0,            ///<  Instruction RAM 0

    MEM_IRAM1,            ///<  Instruction RAM 1

    MEM_IROM0,            ///<  Instruction ROM

    MEM_URAM0,            ///<  Unified RAM

    MEM_XLMI0P0,          ///<  XLMI memory/device L/S 0
    MEM_XLMI0P1,          ///<  XLMI memory/device L/S 1

    MEM_PIF,              ///<  PIF 

    MEM_IDMA0,            ///<  LX PIF iDMA

    MEM_DATAMASTER_RD,    ///<  DataMaster ACE-Lite Read  address (AR*) and Read data (R*) channels (Load)
    MEM_DATAMASTER_WR,    ///<  DataMaster ACE-Lite Write address (AW*),   Write data (W*), and Write response (B*) channels (Store)

    MEM_INSTMASTER_RD,    ///<  InstMaster ACE-Lite Read  address (AR*) and Read data (R*) channels (IFetch)

    MEM_AXIMASTER_RD,     ///<  For merged AXI I/D port: Master ACE-Lite Read  address (AR*) and Read data (R*) channels (Load)
    MEM_AXIMASTER_WR,     ///<  For merged AXI I/D port: Master ACE-Lite Write address (AW*),   Write data (W*), and Write response (B*) channels (Store)

    MEM_IDMA_RD,          ///<  NX iDMA ACE-Lite Read  address (AR*) and Read data (R*) channels (Load)
    MEM_IDMA_WR,          ///<  NX iDMA ACE-Lite Write address (AW*),   Write data (W*), and Write response (B*) channels (Store)

    MEM_APBMASTER,        ///<  NX APB master port

    // For bookkeeping:
    MEM_COUNT,
    MEM_FIRST           = 0,
    MEM_LAST            = MEM_COUNT - 1,
    MEM_DRAM0_LAST_SB   = MEM_DRAM0B3S31,
    MEM_DRAM1_LAST_SB   = MEM_DRAM1B3S31,
    MEM_UNDEFINED       = 0xFFFFFFFF,

    // For backward compatibility:
    MEM_DRAM0LS0        = MEM_DRAM0P0,
    MEM_DRAM0LS1        = MEM_DRAM0P1,
    MEM_DRAM1LS0        = MEM_DRAM1P0,
    MEM_DRAM1LS1        = MEM_DRAM1P1,
    MEM_DROM0LS0        = MEM_DROM0P0,
    MEM_DROM0LS1        = MEM_DROM0P1,
    MEM_XLMI0LS0        = MEM_XLMI0P0,
    MEM_XLMI0LS1        = MEM_XLMI0P1

  };

  // For bookkeeping:
  static const int MEM_SPLIT_WR_DELTA  = MEM_DRAM0LS0WR - MEM_DRAM0LS0RD;
  static const int MEM_SPLIT_DMA_DELTA = MEM_DRAM0DMARD - MEM_DRAM0LS0RD;
  static const int MEM_SUBBANK_DELTA   = MEM_DRAM0B1S00 - MEM_DRAM0B0S00;


  /**
   *  Type used to identify an inbound memory port on the Xtensa core.
   */
  enum inbound_port {
    INB_SNOOP     = 0,    ///<  Snoop port
    INB_PIF,              ///<  Inbound PIF
    INB_DATASLAVE_RD,     ///<  Inbound to DataRAM: DataSlave ACE-Lite Read  address (AR*) and Read data (R*) channels
    INB_DATASLAVE_WR,     ///<  Inbound to DataRAM: DataSlave ACE-Lite Write address (AW*),   Write data (W*), and Write response (B*) channels
    INB_INSTSLAVE_RD,     ///<  Inbound to InstRAM: InstSlave ACE-Lite Read  address (AR*) and Read data (R*) channels
    INB_INSTSLAVE_WR,     ///<  Inbound to InstRAM: InstSlave ACE-Lite Write address (AW*),   Write data (W*), and Write response (B*) channels 
    INB_AXISLAVE_RD,      ///<  Inbound to Inst/DataRAM: Slave ACE-Lite Read  address (AR*) and Read data (R*) channels
    INB_AXISLAVE_WR,      ///<  Inbound to Inst/DataRAM: Slave ACE-Lite Write address (AW*),   Write data (W*), and Write response (B*) channels
    // For bookkeeping:
    INB_COUNT,
    INB_FIRST     = 0,
    INB_LAST      = INB_COUNT - 1,
    INB_UNDEFINED = 0xFFFFFFFF,
  };


  /**
   * This function returns the memory_port number corresponding
   * to the requested memory port name.
   *
   * @param memory_port_name      The name of the memory port.
   * @param core                  Pointer to xtsc_core.
   *
   * @see How_to_do_memory_port_binding for a list of valid names.
   */
  static memory_port get_memory_port(const std::string& memory_port_name, const xtsc_core *core = NULL);

  /**
   * This function returns a c-string name corresponding to the
   * requested memory port type.
   *
   * @param     port_num          The port number whose name is desired.
   *
   * @param     ignore_multi_port If false the LD/ST unit or bank (and subbank), if any,
   *                              are included as part of the returned name (e.g.
   *                              "dram0p0" or "dram0ls0rd" or "dram0b0s00").  
   *                              If true, the LD/ST unit or bank (and subbank) do not
   *                              appear in the returned name (e.g. "dram0" or "dram0rw"
   *                              or "dram0bs", respectively).
   */
  static const char *get_memory_port_name(memory_port port_num, bool ignore_multi_port = false);


  /**
   * Convenience method to determine whether the memory_port specified by port_num is a
   * subbanked DataRAM0 port type.
   *
   * @param     port_num        The memory_port number of interest
   *
   * @returns true if (port_num is between MEM_DRAM0B0S00 and MEM_DRAM0B3S31 inclusive)
   *            otherwise returns false.
   */
  static bool is_subbanked_dram0(memory_port port_num);


  /**
   * Convenience method to determine whether the memory_port specified by port_num is a
   * subbanked DataRAM1 port type.
   *
   * @param     port_num        The memory_port number of interest
   *
   * @returns true if (port_num is between MEM_DRAM1B0S00 and MEM_DRAM1B3S31 inclusive)
   *            otherwise returns false.
   */
  static bool is_subbanked_dram1(memory_port port_num);


  /**
   * Convenience method to determine whether the memory_port specified by port_num is a
   * subbanked DataRAM port type.
   *
   * @param     port_num        The memory_port number of interest
   *
   * @returns true if (port_num is between MEM_DRAM0B0S00 and MEM_DRAM0B3S31 inclusive)
   *            OR if (port_num is between MEM_DRAM1B0S00 and MEM_DRAM1B3S31 inclusive)
   *            otherwise returns false.
   */
  static bool is_subbanked_dram(memory_port port_num);


  /**
   * Convenience method to determine whether the memory_port specified by port_num is a
   * dual-type port served by the specified LD/ST unit.
   *
   * @param     port_num        The memory_port number of interest
   *
   * @param     ls_unit         The LD/ST unit of interest.
   *
   * @returns true if (ls_unit is 0 AND port_num is MEM_DRAM0LS0 | MEM_DRAM1LS0 |
   *                                                MEM_DROM0LS0 | MEM_XLMI0LS0)
   *            OR if (ls_unit is 1 AND port_num is MEM_DRAM0LS1 | MEM_DRAM1LS1 |
   *                                                MEM_DROM0LS1 | MEM_XLMI0LS1)
   *            otherwise returns false.
   *
   * Note: This method is deprecated in favor of is_multi_port_zero().  In some cases
   *       the non-static get_multi_port_count() method may also be needed.
   */
  static bool is_ls_dual_port(memory_port port_num, u32 ls_unit);


  /**
   * Method to get the sc_port for binding the memory request channel from this
   * xtsc_core memory interface master to a memory interface slave.
   *
   * @param mem_port            The desired memory port.
   *
   * @see How_to_do_memory_port_binding for more information.
   */
  sc_core::sc_port<xtsc_request_if, NSPP>& get_request_port(memory_port mem_port) const;


  /**
   * Method to get the sc_export for binding the memory response channel from a memory
   * interface slave to this xtsc_core memory interface master.
   *
   * @param mem_port            The desired memory port.
   *
   * @see How_to_do_memory_port_binding for more information.
   */
  sc_core::sc_export<xtsc_respond_if>& get_respond_export(memory_port mem_port) const;


  /**
   * This function returns the inbound_port number corresponding to the requested
   * inbound port name.
   *
   * @param inbound_port_name   One of "snoop", "inbound_pif", "dataslave_rd",
   *                            "dataslave_wr", "instslave_rd", "instslave_wr",
   *                            "axislave_rd", or "axislave_wr".
   */
  static inbound_port get_inbound_port(const std::string& inbound_port_name);


  /**
   * This function returns a std::string name corresponding to the requested inbound
   * port type.
   *
   * @param     inb_port          The inbound port number whose name is desired.
   *
   * @param     ignore_multi_port If false the full inbound port name is returned.  If 
   *                              true the "_rd"/"_wr" suffix is not included for AXI
   *                              slave ports.
   */
  static const std::string& get_inbound_port_name(inbound_port inb_port, bool ignore_multi_port = false);


  /**
   * Implementation of the xtsc::xtsc_command_handler_interface.
   *
   * This implementation supports the following commands:
   *  \verbatim
        breakpoint_interrupt
          Call xtsc_core::breakpoint_interrupt() for this core.

        change_clock_period <ClockPeriodFactor>
          Call xtsc_core::change_clock_period(<ClockPeriodFactor>).  Return previous
          <ClockPeriodFactor> for this core.

        dasm [<PC> [<NumInstructions>]]
          Return address and disassembly of <NumInstructions> instructions (default 1)
          starting at <PC>.  If <PC> is omitted or is 'pc', then current PC address is
          used.  If <PC> is 'next' then disassembly continues where it left off before.

        disassemble [<PC>]
          Return buffer from calling xtsc_core::disassemble(buffer, <PC>) for this core.
          If <PC> is omitted, then current PC address is used.

        dump_counters
          Call xtsc_core::get_summary_count() for each counter of this core.

        dump_filtered_request [<MemoryPortName>]
          Return the buffer from calling xtsc_core::dump_filtered_request(<MemoryPortName>).

        dump_filtered_response [<MemoryPortName>]
          Return the buffer from calling xtsc_core::dump_filtered_response(<MemoryPortName>).

        dump_inbound_interfaces
          Return buffer from calling xtsc_core::dump_inbound_interfaces() for this core.

        dump_interface_values <InterfaceName> ...
          Return buffer from calling xtsc_core::dump_interface_values(<InterfaceName>)
          repeatedly for each <InterfaceName> for this core.

        dump_memory_interfaces
          Return buffer from calling xtsc_core::dump_memory_interfaces() for this core.

        dump_parameters
          Return buffer from calling xtsc_parms::dump() for the xtsc_core_parms used to
          construct this core.

        dump_registers
          Return buffer from calling xtsc_core::dump_all_registers() for this core.

        dump_core_interfaces_by_type
          Return buffer from calling xtsc_core::dump_core_interfaces_by_type() for this
          core (includes memories).

        dump_tie_interfaces
          Return buffer from calling xtsc_core::dump_tie_interfaces() for this core.

        dump_tie_interfaces_by_type
          Return buffer from calling xtsc_core::dump_tie_interfaces_by_type() for this
          core.

        enable_clock 0|1
          Call xtsc_core::enable_clock(0|1).  Return previous setting for this core.

        enable_debug [<Wait> [<Sync> [<StartPort>]]]
          Returns xtsc_core::enable_debug(<Wait>, <Sync>, true, <StartPort>).  <Wait>
          and <Sync> can be 0|1 (default is 0).

        get_alt_reset_vec
          Return xtsc_core::get_alt_reset_vec() for this core.

        get_clock_factor
          Return xtsc_core::get_clock_factor() for this core.

        get_clock_period_factor
          Return xtsc_core::get_clock_period_factor() for this core.

        get_commit_stage
          Return xtsc_core::get_commit_stage() for this core.

        get_cycle_count
          Return xtsc_core::get_cycle_count() for this core.

        get_debug_poll_interval
          Return xtsc_core::get_debug_poll_interval() for this core.

        get_debugger_port
          Return xtsc_core::get_debugger_port() for this core.

        get_exit_code
          Return xtsc_core::get_exit_code() for this core.

        get_instr_width [<PC>]
          Return xtsc_core::get_instr_width(<PC>) for this core.  If <PC> is omitted,
          then current PC address is used.

        get_last_stage
          Return xtsc_core::get_last_stage() for this core.

        get_parameter_exists <ParameterName>
          Return xtsc_parms::exists(<ParameterName>) for the xtsc_core_parms used to
          construct this core.

        get_parameter_value <ParameterName>
          Return xtsc_parms::dump_value(<ParameterName>) for the xtsc_core_parms used to
          construct this core.

        get_pc [<Stage>]
          Return xtsc_core::get_pc(<Stage>) for this core.  If <Stage> is omitted, then
          search backward from W stage for a meaningful PC.

        get_register_bit_width <RegisterName>
          Return xtsc_core::get_register_bit_width(<RegisterName>) for this core.

        get_register_value <RegisterName>
          Return xtsc_core::get_register_value(<RegisterName>) for this core.

        get_reset_vector
          Return xtsc_core::get_reset_vector() for this core.

        get_simcall_arg <Arg>
          Return xtsc_core::get_simcall_arg(<Arg>) for this core.

        get_stall
          Return xtsc_core::get_stall() for this core.

        get_static_vector_select
          Return xtsc_core::get_static_vector_select() for this core.

        get_summary_count <CounterName>
          Return xtsc_core::get_summary_count(<CounterName>) for this core.

        get_trigin_idma [<Index>]
          Return xtsc_core::get_trigin_idma(<Index>) for this core.  Default <Index> is 0.

        get_turbo_max_relaxed_cycles
          Return xtsc_core::get_turbo_max_relaxed_cycles() for this core.

        has_exited
          Return xtsc_core::has_exited() for this core.

        has_register <RegisterName>
          Return xtsc_core::has_register(<RegisterName>) for this core.

        is_clock_enabled
          Return xtsc_core::is_clock_enabled() for this core.

        is_debugging_enabled
          Return xtsc_core::is_debugging_enabled() for this core.

        is_fast_functional_mode
          Return xtsc_core::is_fast_functional_mode() for this core.

        load_client <ClientPackage>
          Call xtsc_core::load_client(<ClientPackage>) for this core.  <ClientPackage>
          is everything after 'load_client '.

        load_file <FileName>
          Call xtsc_core::load_file(<FileName>) for this core.

        load_program <ProgramName> [<arg>]...
          Call xtsc_core::load_program(<ProgramName>, <Arguments>) for this core.
          <Arguments> is a NULL-terminated c-str array from [<arg>]...).

        log_disassembly 0|1
          Call xtsc_core::log_disassembly(0|1) for this core.

        nb_peek <MemoryPortName> <StartAddress> <NumBytes>
          Call nb_peek(<StartAddress>, <NumBytes>, buffer) on the port specfied by
          <MemoryPortName> and display buffer.

        nb_poke <MemoryPortName> <StartAddress> <NumBytes> <Byte1> <Byte2> ... <ByteN>
          Call nb_poke(<StartAddress>, <NumBytes>, buffer) on the port specfied by
          <MemoryPortName> after putting <Byte1> ... <ByteN> in buffer.

        peek_physical <StartAddress> <NumBytes>
          Peek <NumBytes> of memory starting at <StartAddress> (a physical address).

        peek_virtual <StartAddress> <NumBytes>
          Peek <NumBytes> of memory starting at <StartAddress> (a virtual address).

        poke_physical <StartAddress> <NumBytes> <Byte1> <Byte2> ... <ByteN>
          Poke <NumBytes> (=N) of memory starting at <StartAddress> (a physical
          address).

        poke_virtual <StartAddress> <NumBytes> <Byte1> <Byte2> ... <ByteN>
          Poke <NumBytes> (=N) of memory starting at <StartAddress> (a virtual address).

        reset
          Call xtsc_core::reset() for this core.

        revoke_fast_access
          Call xtsc_core::revoke_fast_access() for this core.

        send_client_command <Command>
          Call xtsc_core::send_client_command(<Command>) for this core.  <Command> is
          everything after 'send_client_command '.

        set_debug_poll_interval <PollInterval>
          Call xtsc_core::set_debug_poll_interval(<PollInterval>) for this core.

        set_alt_reset_vec <Address>
          Call xtsc_core::set_alt_reset_vec(<Address>) for this core.

        set_interrupt <Interrupt> <Set>
          Call xtsc_core::set_interrupt(<Interrupt>, <Set>) for this core.  <Set> may be
          0|1.

        set_pc <PC>
          Call xtsc_core::set_pc(<PC>) for this core.

        set_register_value <RegisterName> <Value>
          Call xtsc_core::set_register_value(<RegisterName>, <Value>) for this core.

        set_simcall_return_value <Value>
          Call xtsc_core::set_simcall_return_value(<Value>) for this core.

        set_stall <Stall>
          Call xtsc_core::set_stall(<Stall>) for this core.  Return the previous value.
          <Stall> may be 0|1.

        set_static_vector_select <Value>
          Call xtsc_core::set_static_vector_select(<Value>) for this core.  <Value> may
          be 0|1.

        set_trigin_idma <High> [<Index>]
          Call xtsc_core::set_trigin_idma(<High>, <Index>) for this core.  Return the
          previous value.  <High> may be 0|1.  Default <Index> is 0.

        set_turbo_max_relaxed_cycles <Value>
          Call xtsc_core::set_turbo_max_relaxed_cycles(<Value>) for this core.

        summary
          Call xtsc_core::summary() for this core.

        translate_virtual <VirtualAddress>
          Return xtsc_core::translate_virtual(<VirtualAddress>) for this core.

        watchfilter_add <MemoryPortName> <FilterName> <EventName>
          Calls xtsc_core::watchfilter_add(<MemoryPortName>, <FilterName>, <Event>) and
          returns the watchfilter number.  <EventName> can be a hyphen (-) to mean the
          last event created by the xtsc_event_create command.

        watchfilter_dump [<MemoryPortName>]
          Return buffer from calling xtsc_core::watchfilter_dump(<MemoryPortName>).

        watchfilter_remove <Watchfilter> | *
          Return xtsc_core::watchfilter_remove(<Watchfilter>).  An * removes all
          watchfilters.
      \endverbatim
   */
  virtual void execute(const std::string&               cmd_line,
                       const std::vector<std::string>&  words,
                       const std::vector<std::string>&  words_lc,
                       std::ostream&                    result);


  /**
   * Connect to the inbound pif port of another xtsc_core.
   *
   * This method connects the specified memory port of this xtsc_core to the inbound pif
   * port of the xtsc_core specified by core.
   *
   * @param     core                    The xtsc_core whose inbound pif port is to be
   *                                    connected to.
   *
   * @param     memory_port_name        The memory port name of this xtsc_core that is
   *                                    to be connected to the inbound pif port of core.
   *
   * @see How_to_do_memory_port_binding for a list of valid names.
   */
  void connect(xtsc_core& core, const char *memory_port_name);


  /**
   * Connect an output of another xtsc_core to an input of this xtsc_core.
   *
   * This method connects a system output wire or TIE export state of another
   * xtsc_core to a system input wire of this xtsc_core.  This method may also be used
   * to chain TX Xtensa cores together.
   *
   * @param     core            The other xtsc_core.
   * 
   * @param     output_name     The system output wire or TIE export state of the other
   *                            xtsc_core.  If chaining TX cores, then this argument
   *                            must be "tx_xfer_out" and core is the upstream TX in the
   *                            chain..
   *
   * @param     input_name      The system input wire of this xtsc_core.  If chaining TX
   *                            cores, then this argument must be "tx_xfer_in" and this
   *                            xtsc_core is the downstream TX in the chain.
   */
  void connect(xtsc_core& core, const char *output_name, const char *input_name);


#ifndef DOXYGEN_SKIP
  /**
   * Connect an xtsc_interrupt_distributor to this xtsc_core.
   *
   * This method connects the specified PROCINT output of the specified
   * xtsc_interrupt_distributor to the specified system-level input wire of this
   * xtsc_core.
   *
   * @param     distributor     The xtsc_interrupt_distributor to connect to this
   *                            xtsc_core.
   * 
   * @param     port            The port index in distributor's PROCINT vector of
   *                            outputs.
   *
   * @param     input_name      The system-level input wire of this xtsc_core to connect
   *                            to.  Typically, and by default, this is "BInterrupt".
   *
   * @see xtsc_interrupt_distributor::connect() for connecting TIE interfaces
   *      "WMPINT_ADDR", "WMPINT_DATA", "WMPINT_TOGGLEEN", and "RMPINT".
   */
  void connect(xtsc_interrupt_distributor& distributor, u32 port, const char *input_name = "BInterrupt");
#endif // DOXYGEN_SKIP


  /**
   * Connect with an upstream xtsc_tx_loader.
   *
   * This method connects this core with the specified upstream xtsc_tx_loader.  Which
   * interfaces are connected depends on the iface argument.
   *
   * @param     loader          The xtsc_tx_loader to connect with.
   *
   * @param     iface           If iface is "tx_xfer_in", then the output XFER interface of
   *                            loader will be connected to the input XFER interface of
   *                            this core (so this core will be the first TX in the
   *                            chain).  If iface is not "tx_xfer_in", then the
   *                            \"pin_level\" parameter of loader must be false and
   *                            iface must name a 32-bit TLM TIE input queue interface
   *                            of this core, which will be connected to the TLM queue
   *                            pop interface (m_consumer) of loader.
   */
  void connect(xtsc_tx_loader& loader, const char *iface);


  /**
   * Dump the most recent previous xtsc_request that passed a xtsc_request watchfilter
   * on the specified outbound or inbound memory port (or on each memory port).
   *
   *
   * @param    os               The ostream object to which the watchfilters should be
   *                            dumped.
   *
   * @param    memory_port_name The memory port name.  If empty then the most recent
   *                            previous request passed on each memory port is dumped.
   *
   * @see watchfilter_add
   * @see xtsc_filter
   */
  void dump_filtered_request(std::ostream& os = std::cout, const std::string& memory_port_name = "");


  /**
   * Dump the most recent previous xtsc_response that passed a xtsc_response watchfilter
   * on the specified outbound or inbound memory port (or on each memory port).
   *
   *
   * @param    os               The ostream object to which the watchfilters should be
   *                            dumped.
   *
   * @param    memory_port_name The memory port name.  If empty then the most recent
   *                            previous response passed on each memory port is dumped.
   *
   * @see watchfilter_add
   * @see xtsc_filter
   */
  void dump_filtered_response(std::ostream& os = std::cout, const std::string& memory_port_name = "");


  /**
   * Add a watchfilter on peeks, pokes, requests, or responses to the specified outbound
   * or inbound memory port.
   *
   * @param    memory_port_name The memory port name.
   *
   * @param    filter_name      The filter instance name.  The actual xtsc_filter object
   *                            will be obtained via a call to xtsc_filter_get.  Its
   *                            kind must be one of "xtsc_peek", "xtsc_poke",
   *                            "xtsc_request", or "xtsc_response".
   *
   * @param    event            The sc_event to notify when a nb_peek, nb_poke,
   *                            nb_request, or nb_response (as appropriate) occurs whose
   *                            payload passes the filter.
   *
   * @returns the watchfilter number (use to remove the watchfilter).
   *
   * @see watchfilter_remove
   * @see xtsc_filter
   * @see xtsc_filter_get
   */
  u32 watchfilter_add(const std::string& memory_port_name, const std::string& filter_name, sc_core::sc_event& event);


  /**
   * Dump a list of all watchfilters applied to the specified outbound or inbound memory
   * port (or all memory ports) of this xtsc_core instance.
   *
   * @param    os               The ostream object to which the watchfilters should be
   *                            dumped.
   *
   * @param    memory_port_name The memory port name.  If empty then watchfilters for
   *                            all memory ports are dumped.
   *
   * @see watchfilter_add
   * @see xtsc_filter
   */
  void watchfilter_dump(std::ostream& os = std::cout, const std::string& memory_port_name = "");


  /**
   * Remove the specified watchfilter or all watchfilters.
   *
   * @param     watchfilter     The number returned from a previous call to
   *                            watchfilter_add.  A -1 (0xFFFFFFFF) means to remove all
   *                            watchfilters on this xtsc_core instance.
   *
   * @returns the number (count) of watchfilters removed.
   *
   * @see watchfilter_add
   * @see xtsc::xtsc_filter
   */
  u32 watchfilter_remove(u32 watchfilter);


  /**
   * Dump a lot of information about the core configuration.
   *
   * Dump the set of core configuration parameters and core interfaces to the specified
   * ostream.
   *
   * @param     os                      The ostream operator to dump the information to.
   *
   * @param     include_interfaces      If false, do not include core interfaces.
   */
  void dump_configuration(std::ostream& os = std::cout, bool include_interfaces = true) const;


  /**
   * This method dumps the status of all cores in a vector of cores.
   *
   * Dumps status information (pc and enabled|disabled|exited) for each core in a vector
   * of cores.  Enabled and disabled refer to the gated clock.
   *
   * @param     os              The ostream object to which the output is sent.  The
   *                            default is cout.
   *
   * @param     cores           The set of cores of interest.  The default is all cores
   *                            in the system.
   *
   * @see is_clock_enabled()
   * @see has_exited()
   */
  static void dump_status(std::ostream& os = std::cout, const std::vector<xtsc_core *>& cores = get_all_cores());


  /**
   * Set the flag that determines whether similation will be stopped (via a call to the
   * SystemC sc_stop() method) after the target program on the last running core (that
   * has "SimNeverExits" set to false--the default) exits.  The default value of the
   * flag is true.  That is, if this method is never called then sc_stop() will be
   * called after the target program on the last running core (that has "SimNeverExits"
   * set to false) exits.
   *
   * @param       stop    If true, sc_stop will be called as soon as the target program
   *                      on the last running core (that has "SimNeverExits" set to
   *                      false) exits.  If false, sc_stop will not be called.
   *
   * @returns the previous value of the flag
   * @see xtsc_core_parms "SimNeverExits"
   * @see xtsc_core_parms "SimStopOnExit"
   */
  static bool set_stop_after_all_cores_exit(bool stop);


  /**
   * Return the flag that determines whether similation will be stopped (via a call to
   * the SystemC sc_stop method) after the target program on the last running core
   * (that has "SimNeverExits" set to false--the default) exits.
   *
   * @returns the value of the flag.
   * @see xtsc_core_parms "SimNeverExits"
   * @see xtsc_core_parms "SimStopOnExit"
   */
  static bool get_stop_after_all_cores_exit();


  /**
   * Get the event that will be notified immediately after the target program on the
   * last running core (that has "SimNeverExits" set to false--the default) exits.
   *
   * @see xtsc_core_parms "SimNeverExits"
   * @see xtsc_core_parms "SimStopOnExit"
   */
  static sc_core::sc_event& get_all_cores_exited_event();


  /**
   * Get the event that will be notified when the target program of this core exits.
   */
  sc_core::sc_event& get_core_exited_event() const;


  /**
   * Get the event that will be notified when the target program of this core hits a
   * breakpoint or completes a debugger step command.
   */
  sc_core::sc_event& get_breakpoint_callback_event() const;


  /**
   * Get the event that will be notified when the attached debugger resumes execution of
   * the target program of this core.
   */
  sc_core::sc_event& get_debugger_resume_event() const;


  /**
   * Get the event that will be notified when the debugger connects to this core.
   */
  sc_core::sc_event& get_debugger_connect_event() const;


  /**
   * Get the event that will be notified when the debugger disconnects from this core.
   */
  sc_core::sc_event& get_debugger_disconnect_event() const;


  /**
   * Get the event that will be notified when the target program of this core makes a user simcall.
   *
   * @see set_simcall_callback
   */
  sc_core::sc_event& get_simcall_callback_event() const;


  /**
   * Get the specified argument from the most recent previous user simcall.
   *
   * @param     arg             The argument number to return.  Valid range is 1-6 (to
   *                            return arg1-arg6).
   *
   * @see simcall_callback
   * @see set_simcall_callback
   */
  int get_simcall_arg(u32 arg) const;


  /**
   * Set the value that will be returned by the default simcall callback.
   *
   * @param     value           The value that will be returned by the default simcall
   *                            callback.
   */
  void set_simcall_return_value(int value);


  /**
   * This method enables or disables all xtsc_core objects in the specified set of cores.
   * A core is enabled by turning its gated clock on and is disabled by turning its
   * gated clock off (using the enable_clock() method).
   *
   * @param     enable            If true (the default) all specified cores are enabled.
   *                              If false all specified cores are disabled.
   *
   * @param     cores             The set of cores to enable or disable.  The default is
   *                              all cores in the system.
   *
   * @see enable_clock()
   */
  static void enable_cores(bool enable = true, const std::vector<xtsc_core *>& cores = get_all_cores());


  /**
   * This method can be used to get a constant reference to the xtsc_core_parms 
   * used to construct this xtsc_core.  This xtsc_core_parms object can be
   * queried for construction parameter values; however, these values should not
   * be changed.
   *
   * @see xtsc_core_parms
   * @see xtsc_parms
   */
  const xtsc_core_parms& get_parms() const;


  /// Get data width of the specified memory interface in units of bytes.
  u32 get_memory_byte_width(memory_port mem_port) const;


  /// Get data width of the specified inbound memory interface in units of bytes.
  u32 get_inbound_byte_width(inbound_port inb_port) const;


  /// Get whether or not core is big endian (returns false if core is little endian)
  bool is_big_endian() const;


  /**
   * Get the instantiation order number of this core.
   *
   * Get the zero-based number indicating the instantiation order for this core.  The
   * first core instantiated has an instantiation order number of 0, the second core
   * instantiated has an instantiation order number of 1, and so on.
   */
  u32 get_instantiation_number() const;


  /**
   * Simcall callback function type.
   *
   * A function of this type can be registered with the core to receive simcall
   * callbacks generated by the target program executing on the core.
   *
   * @param     core            A reference to the xtsc_core whose target program
   *                            executed the simcall.
   *
   * @param     callback_arg    The value passed in to set_simcall_callback.
   *
   * @param     arg1            The 1st argument of the target simcall.
   *
   * @param     arg2            The 2nd argument of the target simcall.
   *
   * @param     arg3            The 3rd argument of the target simcall.
   *
   * @param     arg4            The 4th argument of the target simcall.
   *
   * @param     arg5            The 5th argument of the target simcall.
   *
   * @param     arg6            The 6th argument of the target simcall.
   *
   * @see set_simcall_callback
   */
  typedef int (*simcall_callback)(xtsc_core& core, void *callback_arg, 
                                  int arg1, int arg2, int arg3,
                                  int arg4, int arg5, int arg6);


  /**
   * Set the callback function for user simcalls.
   *
   * This method is called to register a callback function with the ISS that will be
   * called whenever the target program executes a user simcall.
   *
   * Steps to using user simcalls:
   *  \verbatim
      A.  In your host (simulator/sc_main) program:
          1.  Write the callback function in your host program.  Its signature must
              match the simcall_callback typedef. 
          2.  Register you callback function using this method.
      B.  In your target (Xtensa) program:
          1. Include the following file:
                #include <xtensa/sim.h>
          2. Call the xt_iss_simcall() function and pass it 6 arguments.  If you
             need fewer arguments, you can pass dummy values (e.g. 0) for the extra
             arguments to make a total of 6.  For example:
                xt_iss_simcall(value1, value2, 0, 0, 0, 0);
      \endverbatim
   *
   * @param     callback        The callback function to be called.
   *
   * @param     callback_arg    Any desired value.  This value will be passed as the
   *                            second argument to the callback function.
   *
   * @param     previous_arg    Pointer to a void* pointer in which to return
   *                            the previously registered callback argument.
   *
   * Clients calling this method should record the simcall_callback and previous_arg
   * returned by this method and chain a call to them in the registered callback.
   *
   * @see simcall_callback
   *
   * Note: See "lua_function simcall" under the "SimScriptFile" parameter in
   *       xtsc_core_parms for a way to service user simcalls using a Lua function
   *       instead of providing a C/C++ callback function using this method.
   *
   * Note: See the "simcall_csv_file" parameter in xtsc_initialize_parms for an
   *       automatic way to record simcall values into a comma-separated value (CSV)
   *       file using a built-in callback function.
   *
   * @returns the previous callback function.
   */
  simcall_callback set_simcall_callback(simcall_callback callback, void *callback_arg, void **previous_arg = NULL);


  /**
   * Simulation mode switch callback function type.
   *
   * A function of this type can be registered with the core to be called whenever
   * the target program executing on the core requests a simulation mode switch between
   * cycle-accurate and TurboXim fast functional.
   *
   * @param     sim_mode        The simulation mode to switch to.
   *
   * @param     callback_arg    The value passed in to set_sim_mode_switch_callback.
   *
   * @returns true if sim_mode is the current mode, else returns false.
   *
   * @see set_sim_mode_switch_callback
   */
  typedef bool (*sim_mode_switch_callback)(xtsc_sim_mode sim_mode, void *callback_arg);


  /**
   * Set a callback function to be used when a target program requests a simulation mode
   * switch.
   *
   * This method is called to register a callback function with the ISS that will be
   * called whenever the target program executing on this core requests a simulation
   * mode switch.  This callback will not be called if the requested mode is already
   * the current mode.
   *
   * Note: XTSC includes a built-in callback for switching.  Using this method overrides
   * XTSC's built-in callback.
   *
   * Note: In a multi-core simulation, the behavior is unspecified if multiple cores
   * attempt a simulation mode switch during overlapping simulation time intervals.
   *
   * Steps to using a custom simulation mode switch callback:
   *  \verbatim
      A.  In your host (simulator/sc_main) program:
          1.  Write the callback function in your host program.  Its signature must
              match the sim_mode_switch_callback typedef. 
          2.  Register you callback function using this method.
      B.  In your target (Xtensa) program:
          1. Include the following file:
                #include <xtensa/sim.h>
          2. Call the xt_iss_switch_mode(int mode) function with a mode of
             XT_ISS_FUNCTIONAL or XT_ISS_CYCLE_ACCURATE until it returns 0.
             For example:
                while (xt_iss_switch_mode(XT_ISS_FUNCTIONAL));
      \endverbatim
   *
   * @param     callback        The callback function to be called.
   *
   * @param     callback_arg    Any desired value.  This value will be passed as the
   *                            second argument to the callback function.
   *
   * @see sim_mode_switch_callback
   *
   */
  void set_sim_mode_switch_callback(sim_mode_switch_callback callback, void *callback_arg);


  /**
   *  Enable this core to be debugged by xt-gdb and Xtensa Xplorer.
   *
   * @param    wait             If true (the default), the ISS will wait for a debugger
   *                            to attach before releasing this processor core from
   *                            reset.  If false, this core will be released from reset
   *                            as soon as sc_start() is called.
   *
   * @param     synchronized    If enable_debug() is called one or more times with this
   *                            flag set to true, then all cores in the system run in
   *                            lock step so that if any one of them is waiting for the
   *                            debugger to attach or waiting at a breakpoint, then all
   *                            other cores will stall.  If you do not want to have
   *                            synchronized simulation then this flag must be set to
   *                            false (the default) on every call to enable_debug(). 
   *                            This parameter is ignored after the first call to
   *                            sc_start().
   *
   * Note: Starting with RC-2009, operating in synchronized mode stops SystemC
   *       simulation time from advancing (but delta cycles will occur) while any core
   *       is waiting for a debugger to attach or waiting at a breakpoint.
   *
   * @param    dummy            This parameter is ignored (in previous releases it 
   *                            was the enable_xplorer parameter, but enabling of
   *                            Xtensa Xplorer is handled automatically now).
   * 
   * @param    starting_port    xtsc_core needs a free port number to listen to while
   *                            waiting for xt-gdb to connect.  This specifies the
   *                            starting port number when looking for a free port.  It
   *                            is recommended you use a public port number (i.e. 1024
   *                            or greater) or use 0 (the default) which tells xtsc_core
   *                            to pick a port number for you.
   *
   * @returns the actual port number.
   * @throws xtsc_exception if a port could not be opened.
   * @see setup_debug()
   * @see setup_multicore_debug()
   *
   */
  u32 enable_debug(bool         wait            = true,
                   bool         synchronized    = false,
                   bool         dummy           = true,
                   u32          starting_port   = 0);


  /**
   * Setup debugging or profiling on this core based upon command line arguments.
   *
   * This is a convenience method to make it easy to enable debugging or profiling on
   * this core based upon command line arguments.  This method scans argv looking for
   * strings in the following format:
   *   -#  --xxdebug             or  -xxdebug
   *   -#  --xxdebug=<ProcNum>   or  -xxdebug=<ProcNum>
   *   -#  --xxdebug=sync        or  -xxdebug=sync
   *   -#  --xxprofile           or  -xxprofile
   *   -#  --xxprofile=<ProcNum> or  -xxprofile=<ProcNum>
   *
   * If the first format is found, then enable_debug() is called for this core.  If the
   * second format is found and if <ProcNum> matches processor_num, then enable_debug()
   * is called for this core.  If the third format is found then enable_debug() is
   * called for this core with the synchronized argument set to true (regardless of the
   * value of synchronized passed to this function).  If the fourth format is found, 
   * then load_client("profile --all") is called for this core.  If the fifth format is
   * found and if <ProcNum> matches processor_num, then load_client("profile --all") is
   * called for this core.
   *
   * @param     argc            The size of argv.  Typically, this is just the first
   *                            argument passed into sc_main.
   *
   * @param     argv            An array of c-strings.  Typically, this is just the
   *                            second argument passed into sc_main.
   *
   * @param     processor_num   The processor number to compare with the <ProcNum>
   *                            number from the --xxdebug=<ProcNum> argv argument.  
   *                            Two values of processor_num have special meaning.  If
   *                            processor_num is 0xFFFFFFFF (the default) it means to
   *                            use the instantiation order number of this xtsc_core
   *                            (see get_instantiation_number) to compare with
   *                            <ProcNum>.  If processor_num is 0xFFFFFFFE it means to
   *                            use the xtsc_core_parms value from "ProcessorID" to
   *                            compare with <ProcNum>.
   *
   * @param     wait            See enable_debug().
   *
   * @param     synchronized    See enable_debug().
   *
   * @param     dummy           Ignored, see enable_debug().
   * 
   * @param     starting_port   See enable_debug().
   *
   * @returns 1 if enable_debug was called for this core, otherwise returns 0.
   *
   * Note: Enabling both debugging and profiling in the same simulation run is not
   *       recommended because the timing and pipeline changes introduced by the
   *       debugging operation may invalidate the profiling data.
   *
   * @see enable_debug().
   * @see load_client().
   * @see setup_multicore_debug()
   * @see get_instantiation_number.
   */
  u32 setup_debug(int                   argc,
                  const char * const   *argv,
                  u32                   processor_num   = 0xFFFFFFFF,
                  bool                  wait            = true,
                  bool                  synchronized    = false,
                  bool                  dummy           = true,
                  u32                   starting_port   = 0);


  /**
   * Setup debugging or profiling on all cores in a list based upon command line
   * arguments.
   *
   * This is a convenience method to make it easy to enable debugging or profiling on
   * all cores in a list based upon command line arguments.  This method calls the
   * setup_debug() method for each core in the cores vector.
   *
   * @param     argc            See setup_debug().
   *
   * @param     argv            See setup_debug().
   *
   * @param     processor_num   See setup_debug().  If processor_num is 0xFFFFFFFF or
   *                            0xFFFFFFFE then it is passed along unchanged to 
   *                            setup_debug().  If processor_num is any other value,
   *                            then the index in the cores vector is passed to 
   *                            setup_debug as its processor_num argument.
   *
   * @param     wait            See enable_debug().
   *
   * @param     synchronized    See enable_debug().
   *
   * @param     dummy           Ignored, see enable_debug().
   * 
   * @param     starting_port   See enable_debug().
   *
   * @param     cores           The vector of xtsc_core pointers that you want
   *                            setup_debug() called for.  The default is all currently
   *                            existing xtsc_core objects.
   *
   * @returns the number of cores for which enable_debug was called.
   *
   * @see setup_debug().
   * @see enable_debug().
   * @see get_all_cores().
   */
  static u32 setup_multicore_debug(int                          argc,
                                   const char * const          *argv,
                                   u32                          processor_num   = 0,
                                   bool                         wait            = true,
                                   bool                         synchronized    = false,
                                   bool                         dummy           = true,
                                   u32                          starting_port   = 0,
                                   std::vector<xtsc_core*>      cores           = get_all_cores());


  /**
   * Set the debug poll interval.  This is the number of instruction cycles that the
   * ISS will execute before checking to see if the debugger has attached or is trying
   * to interrupt the simulator.
   *
   * @param     num_cycles      The number of instruction cycles between polls.
   *
   * @see the "SimDebugPollInterval" parameter
   */
  void set_debug_poll_interval(u32 num_cycles);


  /**
   * Get the debug poll interval.
   *
   * @returns the number of instruction cycles between polls.
   *
   * @see set_debug_poll_interval()
   */
  u32 get_debug_poll_interval() const;


  /// Returns true if synchronized debugging has been enabled.
  static bool is_debugging_synchronized();


  /// Returns true if debugging has been enabled on this core.
  bool is_debugging_enabled() const;


  /// Get the port number that the debugger should use to communicate with this core.
  u32 get_debugger_port() const;


  /// Debugger callback
  typedef void (*debugger_callback)(void *);


  /**
   * Set the callback function the simulator will call when a debugger connects to this
   * core.
   *
   * @param     callback        The callback function.
   * @param     arg             Argument to be passed to the callback function.
   *
   * Warning:  The callback occurs in the context of ipc_thread, a separate OS-level
   *           thread used to communicate with the debugger.  This OS-level thread runs
   *           independent of the OS-level thread in which the SystemC kernel runs and
   *           it is NOT safe to modify SystemC kernel state and most module state from
   *           the callback function.  In lieu of using a callback function, Cadence
   *           recommends that the get_debugger_connect_event() method be used to get an
   *           sc_event that will be safely notified when the debugger connects.
   *
   * @returns the previous callback function.
   */
  debugger_callback set_debugger_connect_callback(debugger_callback callback, void *arg);


  /**
   * Set the callback function the simulator will call when a debugger disconnects from
   * this core.
   *
   * @param     callback        The callback function.
   * @param     arg             Argument to be passed to the callback function.
   *
   * Warning:  The callback occurs in the context of ipc_thread, a separate OS-level
   *           thread used to communicate with the debugger.  This OS-level thread runs
   *           independent of the OS-level thread in which the SystemC kernel runs and
   *           it is NOT safe to modify SystemC kernel state and most module state from
   *           the callback function.  In lieu of using a callback function, Cadence
   *           recommends that the get_debugger_disconnect_event() method be used to get
   *           an sc_event that will be safely notified when the debugger disconnects.
   *
   * @returns the previous callback function.
   */
  debugger_callback set_debugger_disconnect_callback(debugger_callback callback, void *arg);


  /**
   * Set the callback function the simulator will call whenever this core encounters an
   * internal (xt-gdb/Xtensa Xplorer) breakpoint.
   *
   * @param     callback        The callback function.
   * @param     arg             Argument to be passed to the callback function.
   * @param     previous_arg    Pointer to a void* pointer in which to return
   *                            the previously registered callback argument.
   *
   * Clients calling this method should record the debugger_callback and previous_arg
   * returned by this method and chain a call to them in the registered callback.
   *
   * Note: Unlike the debugger resume callback, the callback function specfied here is
   *       called in the context of the OS-level thread of the SystemC kernel so
   *       SystemC and module methods and state can be safely accessed.
   *
   * Note: See "lua_function breakpoint" under the "SimScriptFile" parameter in
   *       xtsc_core_parms for a way to have a Lua function called when this core
   *       encouners an internal breakpoint instead of providing a C/C++ callback
   *       function using this method.
   *
   * Note: See the "breakpoint_csv_file" parameter in xtsc_initialize_parms for an
   *       automatic way to record breakpoint information into a comma-separated value
   *       (CSV) file using a built-in callback function.
   *
   * @returns the previous callback function.
   */
  debugger_callback set_breakpoint_callback(debugger_callback callback, void *arg, void **previous_arg = NULL);


  /**
   * Set the callback function the simulator will call whenever the debugger of this
   * core starts the core executing instructions.  From xt-gdb, this corresponds to the
   * continue, step, stepi, and stepc commands.
   *
   * @param     callback        The callback function.
   * @param     arg             Argument to be passed to the callback function.
   *
   * Warning:  The callback occurs in the context of ipc_thread, a separate OS-level
   *           thread used to communicate with the debugger.  This OS-level thread runs
   *           independent of the OS-level thread in which the SystemC kernel runs and
   *           it is NOT safe to modify SystemC kernel state and most module state from
   *           the callback function.  In lieu of using a callback function, Cadence
   *           recommends that the get_debugger_resume_event() method be used to get an
   *           sc_event that will be safely notified when the debugger resumes
   *           instruction execution.
   *
   * @returns the previous callback function.
   */
  debugger_callback set_debugger_resume_callback(debugger_callback callback, void *arg);


  /**
   * Cause the ISS for this core to break and pass control to its attached debugger.
   *
   * For example, when some other core has encountered a breakpoint and its breakpoint
   * callback function (set using set_breakpoint_callback) has been envoked by the
   * simulator, this method can be used from that callback to cause this core to also
   * break and pass control to its attached debugger.
   */
  void breakpoint_interrupt();


  /// Returns true if core program has called exit(), else returns false
  bool has_exited() const;


  /**
   * This method returns true if all cores in a vector of cores have exited; otherwise
   * it returns false.
   *
   * @param     cores           The set of cores of interest.  The default is all cores
   *                            in the system.
   *
   * @see has_exited()
   */
  static bool have_all_cores_exited(const std::vector<xtsc_core *>& cores = get_all_cores());


  /// Return the exit code of the core program
  int get_exit_code() const;


  /**
   * Get whether or not the specified outbound memory port exists.
   *
   * @param     mem_port        The desired outbound memory port.
   *
   * @returns true if the outbound memory port exists, otherwise returns false.
   * @see memory_port
   */
  bool has_memory_port(memory_port mem_port) const;


  /**
   * Get whether or not the named outbound memory port exists.
   *
   * @param memory_port_name    The outbound memory port name.
   *
   * @returns true if the outbound memory port exists, otherwise returns false.
   *
   * @see How_to_do_memory_port_binding for a list of valid names.
   */
  bool has_memory_port(const char *memory_port_name) const;


  /**
   * Get whether or not the specified inbound memory port exists.
   *
   * @param     inb_port        The desired inbound memory port.
   *
   * @returns true if the inbound memory exists, otherwise returns false.
   * @see inbound_port
   */
  bool has_inbound_port(inbound_port inb_port) const;


  /**
   * Get whether or not the named inbound memory port exists.
   *
   * @param inbound_port_name   One of "snoop", "inbound_pif", "dataslave_rd",
   *                            "dataslave_wr", "instslave_rd", "instslave_wr",
   *                            "axislave_rd", or "axislave_wr".
   *
   * @returns true if the inbound memory port exists, otherwise returns false.
   *
   * @see How_to_do_memory_port_binding 
   */
  bool has_inbound_port(const char *inbound_port_name) const;


  /**
   * Return true if inb_port is AXI protocol (INB_DATASLAVE_RD, INB_DATASLAVE_WR,
   * INB_INSTSLAVE_RD, INB_INSTSLAVE_WR, INB_AXISLAVE_RD, INB_AXISLAVE_WR).
   */
  static bool is_axi(inbound_port inb_port);


  /**
   * Return true if inb_port is a single-ported interface or if it is the first port of
   * a multi-ported interface.
   *
   * @param     inb_port        The desired inbound memory port. 
   */
  static bool is_multi_port_zero(inbound_port inb_port);


  /**
   * Return the zeroeth inbound memory port of the specified inbound memory port.
   *
   * @param     inb_port        The inbound memory port whose zeroeth memory port is
   *                            desired. 
   */
  static inbound_port get_multi_port_zero(inbound_port inb_port);


  /**
   * Return the number of ports the core has of the specified inbound memory port class.
   *
   * For example, calling this method with inb_port of either INB_DATASLAVE_RD or
   * INB_DATASLAVE_WR will return 2 if the core has a DataSlave interface and will
   * return 0 if it does not.
   *
   * @param     inb_port        A memory port of the desired memory port class.  
   */
  u32 get_multi_port_count(inbound_port inb_port) const;


  /**
   * Return the inbound_port enum of the nth port (0-based) of a multi-ported inbound
   * interface or the only (n=0) port of a single-ported inbound interface.
   *
   * For example, here is what this method will return when called with the values shown
   * on a config which has the inbound memory port specified by inb_port.
   * \verbatim
      inb_port          n       return
      ----------------  -       ----------------
      INB_PIF           0       INB_PIF
      INB_DATASLAVE_RD  0       INB_DATASLAVE_RD
      INB_DATASLAVE_RD  1       INB_DATASLAVE_WR
      INB_INSTSLAVE_RD  0       INB_INSTSLAVE_RD
      INB_INSTSLAVE_RD  1       INB_INSTSLAVE_WR
      INB_AXISLAVE_RD   0       INB_AXISLAVE_RD
      INB_AXISLAVE_RD   1       INB_AXISLAVE_WR
      \endverbatim
   *
   * @param     inb_port        The enum of the first port (port 0) of the desired
   *                            inbound multi-port or the enum of a single-ported
   *                            inbound interface.
   *
   * @param     n               The desired nth port.  If inb_port is a single-ported
   *                            inbound interface then n must be 0.  The valid range for
   *                            n is 0 to one less than the value returned by calling
   *                            get_multi_port_count(inb_port).
   *
   * @throws xtsc_exception if this core does not have the specified inb_port.
   */
  inbound_port get_nth_multi_port(inbound_port inb_port, u32 n) const;


  /**
   * Determine if core is dual-ported (hardware prior to RE-2012.0).
   *
   * If xlmi is false, this method returns true if the core has 2 LD/ST units and no
   * CBox.  If xlmi is true, this method returns true if the core has 2 LD/ST units.
   *
   * Note: This method always returns false on RE-2012.0 or later hardware configs
   *       ("HWMicroArchLatest" is 250000 or greater).
   *
   * Note: This method is deprecated in favor of the get_multi_port_count() method.
   */
  bool is_dual_ported(bool xlmi = false) const;


  /**
   * Return true if mem_port is MEM_PIF, MEM_DATAMASTER_RD, MEM_DATAMASTER_WR,
   * MEM_INSTMASTER_RD, MEM_AXIMASTER_RD, MEM_AXIMASTER_WR, MEM_IDMA0, MEM_IDMA_RD, MEM_IDMA_WR, or MEM_APBMASTER.
   */
  static bool is_system_port(memory_port mem_port);


  /**
   * Return true if mem_port is one of the ports comprising "axi" (that is,
   * MEM_DATAMASTER_RD, MEM_DATAMASTER_WR, MEM_INSTMASTER_RD, MEM_AXIMASTER_RD, MEM_AXIMASTER_WR).
   */
  static bool is_axi(memory_port mem_port);


  /**
   * Return true if mem_port is an AXI4 port (that is, MEM_DATAMASTER_RD,
   * MEM_DATAMASTER_WR, MEM_INSTMASTER_RD, MEM_AXIMASTER_RD, MEM_AXIMASTER_WR, MEM_IDMA_RD, or MEM_IDMA_WR).
   */
  static bool is_axi_or_idma(memory_port mem_port);


  /**
   * Return true if mem_port is an IDMA port (that is, MEM_IDMA0, MEM_IDMA_RD, or MEM_IDMA_WR).
   */
  static bool is_idma(memory_port mem_port);


  /**
   * Return true if mem_port is MEM_APBMASTER.
   */
  static bool is_apb(memory_port mem_port);


  /**
   * Return true if mem_port is an AXI4 or APB port.
   */
  static bool is_amba(memory_port mem_port);


  /// Return true if mem_port is MEM_XLMI0P0 or MEM_XLMI0P1
  static bool is_xlmi(memory_port mem_port);


  /**
   * Return true if mem_port is one of MEM_DRAM0LS0RD, MEM_DRAM0LS1RD, MEM_DRAM0LS2RD,
   * MEM_DRAM0DMARD, MEM_DRAM1LS0RD, MEM_DRAM1LS1RD, MEM_DRAM1LS2RD, or MEM_DRAM1DMARD.
   */
  static bool is_split_read(memory_port mem_port);


  /**
   * Return true if mem_port is one of MEM_DRAM0LS0WR, MEM_DRAM0LS1WR, MEM_DRAM0LS2WR,
   * MEM_DRAM0DMAWR, MEM_DRAM1LS0WR, MEM_DRAM1LS1WR, MEM_DRAM1LS2WR, or MEM_DRAM1DMAWR.
   */
  static bool is_split_write(memory_port mem_port);


  /**
   * Return true if is_split_read() or is_split_write() is true.
   */
  static bool is_split_read_write(memory_port mem_port);


  /**
   * Return true if mem_port is a single-ported interface or if it is the first port of
   * a multi-ported interface.
   *
   * @param     mem_port        The desired memory port. 
   */
  static bool is_multi_port_zero(memory_port mem_port);


  /**
   * Return the zeroeth memory port of the specified memory port.
   *
   * @param     mem_port        The memory port whose zeroeth memory port is desired. 
   */
  static memory_port get_multi_port_zero(memory_port mem_port);


  /**
   * Return the number of ports the core has of the specified memory port class.
   *
   * For example, for a non-subbanked, non-split RW interface, calling this method with
   * mem_port of MEM_DRAM0P0, MEM_DRAM0P1, MEM_DRAM0P2, or MEM_DRAM0P3 will return the
   * number of ports that the DataRAM0 interface has and calling this method with
   * mem_port of MEM_DRAM0LS0RD, MEM_DRAM0LS0WR, MEM_DRAM0LS1RD, MEM_DRAM0LS1WR, etc, or
   * with a mem_port of MEM_DRAM0B0S00, MEM_DRAM0B0S01, etc, will return 0.
   *
   * For an interface with subbanks, calling this method with mem_port of MEM_DRAM0P0,
   * MEM_DRAM0P1, MEM_DRAM0P2, or MEM_DRAM0P3 will return 0 while calling this method
   * with mem_port of MEM_DRAM0B0S00, MEM_DRAM0B0S01, etc, (and so on for banks 1, 2,
   * and 3) will return the number of banks times the number of subbanks per bank that
   * the DataRAM0 interface has.
   *
   * For a split RW interface, calling this method with mem_port of MEM_DRAM0P0,
   * MEM_DRAM0P1, MEM_DRAM0P2, or MEM_DRAM0P3 will return 0 while calling this method
   * with mem_port of MEM_DRAM0LS0RD, MEM_DRAM0LS0WR, MEM_DRAM0LS1RD, MEM_DRAM0LS1WR,
   * etc, will return the number of RD ports (equals number of WR ports) that the
   * DataRAM0 interface has (if count_split_rw is false) or it will return the sum of
   * the RD ports and WR ports (if count_split_rw is true).  If the core does not have
   * a DataRAM0 interface then calling this method with any of the above values of
   * mem_port will return 0.
   *
   * @param     mem_port        A memory port of the desired memory port class.  
   *
   * @param     count_split_rw  For split R/W ports, this method returns the number of
   *                            RD ports or the number of WR ports (the two numbers are
   *                            always equal) if count_split_rw is false.  If
   *                            count_split_rw is true, the sum of the RD and WR port
   *                            counts is returned.
   */
  u32 get_multi_port_count(memory_port mem_port, bool count_split_rw = true) const;


  /**
   * Return the memory_port enum of the nth port (0-based) of a multi-ported interface
   * or the only (n=0) port of a single-ported interface.
   *
   * This convenience method makes it easier to get each of the subbanked or split R/W
   * DataRAM0 or DataRAM1 ports that the config has when, for subbanked interfaces, it
   * may have less than the maximum number of subbanks and for split R/W interfaces it
   * may have less than 3 LD/ST units and may or may not have inbound PIF (DMA).
   *
   * For example, here is what this method will return when called with the values shown
   * on a config with 2 banks each with 4 subbanks on DataRAM0 (for a total of 8
   * multi-ports).
   * \verbatim
      mem_port          n       return
      --------------    -       --------------
      MEM_DRAM0B0S00    0       MEM_DRAM0B0S00
      MEM_DRAM0B0S00    1       MEM_DRAM0B0S01
      MEM_DRAM0B0S00    2       MEM_DRAM0B0S02
      MEM_DRAM0B0S00    3       MEM_DRAM0B0S03
      MEM_DRAM0B0S00    4       MEM_DRAM0B1S00
      MEM_DRAM0B0S00    5       MEM_DRAM0B1S01
      MEM_DRAM0B0S00    6       MEM_DRAM0B1S02
      MEM_DRAM0B0S00    7       MEM_DRAM0B1S03

      MEM_PIF           0       MEM_PIF
      MEM_IDMA0         0       MEM_IDMA0
      \endverbatim
   *
   * As another example, here is what this method will return when called with the
   * values shown on a config with split R/W ports, 2 LD/ST units, and inbound PIF to
   * DataRAM0 (for a total of 6 multi-ports) and with interleave_rw true.
   * \verbatim
      mem_port          n       return
      --------------    -       --------------
      MEM_DRAM0LS0RD    0       MEM_DRAM0LS0RD
      MEM_DRAM0LS0RD    1       MEM_DRAM0LS0WR
      MEM_DRAM0LS0RD    2       MEM_DRAM0LS1RD
      MEM_DRAM0LS0RD    3       MEM_DRAM0LS1WR
      MEM_DRAM0LS0RD    4       MEM_DRAM0DMARD
      MEM_DRAM0LS0RD    5       MEM_DRAM0DMAWR

      MEM_PIF           0       MEM_PIF
      \endverbatim
   *
   * And here is what this method will return when called with the values shown on the
   * same config as above but with interleave_rw false.
   * \verbatim
      mem_port          n       return
      --------------    -       --------------
      MEM_DRAM0LS0RD    0       MEM_DRAM0LS0RD
      MEM_DRAM0LS0RD    1       MEM_DRAM0LS1RD
      MEM_DRAM0LS0RD    2       MEM_DRAM0DMARD
      MEM_DRAM0LS0RD    3       MEM_DRAM0LS0WR
      MEM_DRAM0LS0RD    4       MEM_DRAM0LS1WR
      MEM_DRAM0LS0RD    5       MEM_DRAM0DMAWR
      \endverbatim
   *
   * @param     mem_port        The enum of the first port (port 0) of the desired
   *                            multi-port or the enum of a single-ported interface.
   *
   * @param     n               The desired nth port.  If mem_port is a single-ported
   *                            interface then n must be 0.  The valid range for n
   *                            is 0 to one less than the value returned by calling
   *                            get_multi_port_count(mem_port, true).
   *
   * @param     interleave_rw   Specifies whether each WR port of a split RW interface
   *                            should immediately follow its corresponding RD port
   *                            (interleave_rw true) or if they should all follow after
   *                            the last RD port (interleave_rw false).  For example,
   *                            if interleave_rw is true, MEM_DRAM0LS0RD is followed by
   *                            MEM_DRAM0LS0WR.  If interleave_rw is false (and assuming
   *                            at least 2 LD/ST units), MEM_DRAM0LS0RD is followed by
   *                            MEM_DRAM0LS1RD.  Similarly for DataRAM1.  This parameter
   *                            only applies if mem_port is a split RW Data RAM.
   *
   * Note: 3 LD/ST units is reserved for future use.
   *
   * @throws xtsc_exception if this core does not have the specified nth memory port.
   */
  memory_port get_nth_multi_port(memory_port mem_port, u32 n, bool interleave_rw = true) const;


  /**
   * Return true if the specified memory port has a busy/ready interface.
   *
   * @param     mem_port        The desired memory port. 
   *
   * @throws xtsc_exception if this core does not have the specified memory port.
   */
  bool has_busy(memory_port mem_port) const;


  /**
   * Get whether or not the specified local memory port exists and its starting byte
   * address.
   *
   * @param     mem_port        The desired local memory port. 
   *
   * @param     address8        A reference in which to return the starting byte
   *                            address.
   *
   * @returns true if the local memory exists, otherwise returns false.
   * @see memory_port
   */
  bool get_local_memory_starting_byte_address(memory_port mem_port, xtsc_address& address8) const;


  /**
   * Get whether or not system RAM memory exists and its starting byte address.
   *
   * @param     address8        A reference in which to return the starting byte
   *                            address.
   *
   * @returns true if system RAM memory exists, otherwise returns false.
   */
  bool get_system_ram_starting_byte_address(xtsc_address& address8) const;


  /**
   * Get whether or not system ROM memory exists and its starting byte address.
   *
   * @param     address8        A reference in which to return the starting byte
   *                            address.
   *
   * @returns true if system ROM memory exists, otherwise returns false.
   */
  bool get_system_rom_starting_byte_address(xtsc_address& address8) const;


  /**
   * Get whether or not the specified local memory port exists and its size in bytes.
   *
   * @param     mem_port        The desired local memory port.
   *
   * @param     size8           A reference in which to return the memory byte size.
   *
   * @returns true if the local memory port exists, otherwise returns false.
   * @see memory_port
   */
  bool get_local_memory_byte_size(memory_port mem_port, u32& size8) const;


  /**
   * Get whether or not system RAM memory exists and its size in bytes.
   *
   * @param     size8           A reference in which to return the memory byte size.
   *
   * @returns true if system RAM memory exists, otherwise returns false.
   */
  bool get_system_ram_byte_size(u32& size8) const;


  /**
   * Get whether or not system ROM memory exists and its size in bytes.
   *
   * @param     size8           A reference in which to return the memory byte size.
   *
   * @returns true if system ROM memory exists, otherwise returns false.
   */
  bool get_system_rom_byte_size(u32& size8) const;


  /// Get the cycle count of this core's gated clock
  u64 get_cycle_count() const;


  /**
   * This method is used to read memory using a physical address without 
   * disturbing the memory hardware, the processor hardware, or the bus 
   * hardware.  
   *
   * @param     address8        The byte address of the first byte to be
   *                            peeked.
   *
   * @param     size8           The number of bytes to peek.  Can be any
   *                            number of bytes as long as the address
   *                            range from address8 to address8+size8-1
   *                            maps to the same physical memory.
   *
   * @param     buffer          The byte array in which to return the
   *                            peeked data.  The byte at address8 is
   *                            returned in buffer[0], the byte at 
   *                            address8+1 is returned in buffer[1], and
   *                            so on up to the byte at address8+size8-1 
   *                            is returned in buffer[size8-1]. This format 
   *                            applies regardless of host and core 
   *                            endianess.  The caller is responsible for
   *                            allocating this buffer.
   *
   */
  void peek_physical(xtsc_address address8, u32 size8, u8 *buffer);


  /**
   * This method is used to read memory using a virtual address without 
   * disturbing the memory hardware, the processor hardware, or the bus 
   * hardware.  
   *
   * @param     address8        The byte address of the first byte to be
   *                            peeked.
   *
   * @param     size8           The number of bytes to peek.  Can be any
   *                            number of bytes as long as the address
   *                            range from address8 to address8+size8-1
   *                            maps to the same physical memory.
   *
   * @param     buffer          The byte array in which to return the
   *                            peeked data.  The byte at address8 is
   *                            returned in buffer[0], the byte at 
   *                            address8+1 is returned in buffer[1], and
   *                            so on up to the byte at address8+size8-1 
   *                            is returned in buffer[size8-1]. This format 
   *                            applies regardless of host and core 
   *                            endianess.  The caller is responsible for
   *                            allocating this buffer.
   */
  void peek_virtual(xtsc_address address8, u32 size8, u8 *buffer);


  /**
   * This method is used to write memory using a physical address without 
   * disturbing the memory controller hardware, the processor hardware, or 
   * the bus hardware.  
   *
   * @param     address8        The byte address of the first byte to be
   *                            poked.
   *
   * @param     size8           The number of bytes to poke.  Can be any
   *                            number of bytes as long as the address
   *                            range from address8 to address8+size8-1
   *                            maps to the same physical memory.
   *
   * @param     buffer          The byte array in which to obtain the
   *                            poked data.  The byte in buffer[0]
   *                            is poked into memory at address8, the
   *                            byte in buffer[1] is poked into memory
   *                            at address8+1, and so on up to the byte
   *                            in buffer[size8-1] is poked into memory
   *                            at address8+size8-1.  This format applies 
   *                            regardless of host and core endianess.
   */
  void poke_physical(xtsc_address address8, u32 size8, const u8 *buffer);


  /**
   * This method is used to write memory using a virtual address without 
   * disturbing the memory controller hardware, the processor hardware, or 
   * the bus hardware.  
   *
   * @param     address8        The byte address of the first byte to be
   *                            poked.
   *
   * @param     size8           The number of bytes to poke.  Can be any
   *                            number of bytes as long as the address
   *                            range from address8 to address8+size8-1
   *                            maps to the same physical memory.
   *
   * @param     buffer          The byte array in which to obtain the
   *                            poked data.  The byte in buffer[0]
   *                            is poked into memory at address8, the
   *                            byte in buffer[1] is poked into memory
   *                            at address8+1, and so on up to the byte
   *                            in buffer[size8-1] is poked into memory
   *                            at address8+size8-1.  This format applies 
   *                            regardless of host and core endianess.
   */
  void poke_virtual(xtsc_address address8, u32 size8, const u8 *buffer);


  /**
   * This method peeks the ITLB to translate the virtual address to a physical address.
   *
   * If the core does not have a ITLB, then hit will be set to true and address8 will be
   * returned.  If address8 is not in the ITLB, then hit will be set to false and
   * address8 will be returned.  Otherwise, hit will be set to true and the translated
   * (that is, physical) address will be returned.
   *
   * @param     address8        The virtual byte address to be translated.
   *
   * @param     hit             Set to true if the core does not have a ITLB or if the
   *                            ITLB contained a translation for address8.
   */
  xtsc_address peek_itlb(xtsc_address address8, bool& hit);


  /**
   * This method peeks the DTLB to translate the virtual address to a physical address.
   *
   * If the core does not have a DTLB, then hit will be set to true and address8 will be
   * returned.  If address8 is not in the DTLB, then hit will be set to false and
   * address8 will be returned.  Otherwise, hit will be set to true and the translated
   * (that is, physical) address will be returned.
   *
   * @param     address8        The virtual byte address to be translated.
   *
   * @param     hit             Set to true if the core does not have a DTLB or if the
   *                            DTLB contained a translation for address8.
   */
  xtsc_address peek_dtlb(xtsc_address address8, bool& hit);


  /**
   * This method translates the specified virtual address into a physical address.
   *
   * This convenience method first tries peek_itlb() and, if that misses, it then tries
   * peek_dtlb() and, if that misses, it returns address8.
   *
   * @param     address8        The virtual byte address to be translated.
   */
  xtsc_address translate_virtual(xtsc_address address8);


  /**
   * Assuming register tracing is setup, this method sets whether or not it is enabled.
   *
   * When register tracing is setup it is also automatically enabled from the start of
   * simulation.  If this is not desired, call this method with enable=false, prior to
   * starting simulation.
   *
   * Register tracing is setup by setting the "SimVcdHandle" and "SimTraceRegisters"
   * parameters.
   *
   * @param     enable          If true, register tracing, if setup, is enabled.  If
   *                            false, register tracing is disabled.
   *
   * @throws xtsc_exception if called with enable=true and register tracing has not be
   *         setup.
   *
   * @see is_register_tracing_enabled()
   */
  void enable_register_tracing(bool enable);


  /**
   * Return whether or not register tracing is enabled.
   *
   * @see enable_register_tracing()
   */
  bool is_register_tracing_enabled() const;


  /**
   * Assert or deassert the specified bit of this core's TrigIn_iDMA signal.
   *
   * @param     high    If true, the bit specified by index of the TrigIn_iDMA signal is
   *                    asserted.  If false, it is deasserted.
   *
   * @param     index   Index into the TrigIn_iDMA signal.  Must be 0 for LX iDMA.  Can
   *                    be 0|1 for NX iDMA.
   *
   * @throws xtsc_exception if called on a config without iDMA of if index is out of
   * range.
   *
   * @see get_trigin_idma()
   */
  void set_trigin_idma(bool high, u32 index = 0);


  /**
   * Return whether or not the specified bit of this core's TrigIn_iDMA signal is
   * asserted.
   *
   * @param     index   Index into the TrigIn_iDMA signal.  Must be 0 for LX iDMA.  Can
   *                    be 0|1 for NX iDMA.
   *
   * @returns true if signal is asserted, return false if signal is deasserted.
   *
   * @throws xtsc_exception if called on a config without iDMA of if index is out of
   * range.
   *
   * @see set_trigin_idma()
   */
  bool get_trigin_idma(u32 index = 0);


  /**
   * Set setting whether or not this core is in RunStall.
   *
   * This method allows stalling or releasing this core's pipeline in the same
   * manner as the RunStall system-level input.
   *
   * @param     stall   If true, causes the pipeline to stall after it advances
   *                    at the beginning of the next cycle.  The pipeline will
   *                    advance no further until the stall is released by calling 
   *                    this method with a value of false. 
   *
   * @see the "SimRunStall" parameter
   * @see get_stall()
   */
  void set_stall(bool stall);


  /**
   * Return whether or not this core is in RunStall.
   *
   * @returns true if core is stalled, return false if core is not stalled.
   *
   * @see set_stall()
   */
  bool get_stall();


  /**
   * Turns logging of disassembly on or off.
   *
   * If true, as each instruction commits, its program counter, machine code, and
   * disassembly will be logged at INFO_LOG_LEVEL.  If false, this logging will be
   * turned off.
   * 
   * @param     log             If true, logging of disassembly is turned on.  If false,
   *                            logging of disassembly is turned off.
   *
   * @see "SimLogDisassembly" in xtsc_core_parms
   */
  void log_disassembly(bool log);


  /**
   * Method to determine simulation mode.
   *
   * @returns true if this core is currently in fast functional mode (TurboXim), else 
   * returns false to indicate core is currently in cycle-accurate mode.
   */
  bool is_fast_functional_mode() const;


  /**
   * Disable or enable this core's gated clock.
   *
   * @param     enable          If true (the default), this core's gated clock is
   *                            enabled.  If false, this core's gated clock is disabled.
   *
   * @returns true if this core's gated clock was previously (prior to this API call)
   *         enabled, otherwise return false.
   *
   * @see is_clock_enabled()
   */
  bool enable_clock(bool enable = true);


  /**
   * Returns true if this core's gated clock is enabled; otherwise returns false.
   *
   * @see enable_clock()
   */
  bool is_clock_enabled();


  /**
   * Method to step one core while all other cores are disabled.
   *
   * This method disables the gated clock of all other core's in the system, enables
   * this core's gated clock, calls sc_start for the specified number of clock cycles,
   * then restores the gated clocks of all cores to the state they were in when this
   * method was called.
   *
   * Note: This method should only be called from places you can call sc_start (i.e.
   *       from sc_main, either directly or indirectly).
   *
   * @param     num_cycles      The number of clock cycles to step this core.
   *
   * @see enable_clock
   */
  void step(u32 num_cycles);


  /**
   * Set/clear the specified interrupt.
   *
   * @param     interrupt       The number of the desired interrupt.
   *
   * @param     set             If true, interrupt is set.
   *                            If false, interrupt is cleared.
   */
  void set_interrupt(u32 interrupt, bool set);


  /**
   * Reset the xtsc_core.
   */
  void reset(bool hard_reset = false);


  /**
   * Reset the XFER block of a TX core with the boot loader option.
   *
   * @param     reset           If true, XferReset is asserted.
   *                            If false, XferReset is de-asserted.
   *
   * @throws xtsc_exception if xtsc_core_parms "BootLoader" is false
   */
  void xfer_reset(bool reset);


  /**
   * Revoke all current fast access grants (TurboXim may subsequently re-request them).
   */
  void revoke_fast_access();


  /**
   * Print processor execution summary.
   *
   * @param     os              The ostream object on which to print the summary.
   *
   * Note:  Also see the summary client under the load_client() method.
   */
  void summary(std::ostream& os = std::cout) const;


  /**
   * Get the set of TLM TIE interfaces defined for this core.
   *
   * Get the set of TLM TIE interfaces (ext_regfiles, lookups, input queues, output
   * queues, import wires, and export states) defined for this core.  
   *
   * The TLM TIE names are as written in the user's TIE code and will never have the
   * "TIE_" prefix.
   */
  std::set<std::string> get_tie_interface_set() const;


  /// Get the set of TLM TIE ext_regfiles defined for this core.  
  std::set<std::string> get_ext_regfile_set() const;

  /// Get the set of TLM TIE lookups defined for this core.  
  std::set<std::string> get_lookup_set() const;


  /// Get the set of TLM TIE input queues defined for this core.  
  std::set<std::string> get_input_queue_set() const;


  /// Get the set of TLM TIE output queues defined for this core.  
  std::set<std::string> get_output_queue_set() const;


  /// Get the set of TLM TIE import wires defined for this core.  
  std::set<std::string> get_import_wire_set() const;


  /// Get the set of TLM TIE export states defined for this core.  
  std::set<std::string> get_export_state_set() const;


  /**
   *  Get the set of pins defined for this core.  
   *
   *  Get the set of TIE and system input and output pins defined for this core.  The
   *  pin-level TIE names always start with the "TIE_" prefix and the system input and
   *  output names will never start with this prefix.
   */
  std::set<std::string> get_pin_set() const;


  /**
   *  Get the set of input pins defined for this core.  
   *
   *  Get the set of TIE and system input pins defined for this core.  The pin-level TIE
   *  names always start with the "TIE_" prefix and the system input names will never
   *  start with this prefix.
   */
  std::set<std::string> get_input_pin_set() const;


  /**
   *  Get the set of output pins defined for this core.  
   *
   *  Get the set of TIE and system output pins defined for this core.  The pin-level
   *  TIE names always start with the "TIE_" prefix and the system output names will
   *  never start with this prefix.
   */
  std::set<std::string> get_output_pin_set() const;


  /// Get the set of pin-level TIE ext_regfiles defined for this core.  
  std::set<std::string> get_pin_level_ext_regfile_set() const;

  /// Get the set of pin-level TIE lookups defined for this core.  
  std::set<std::string> get_pin_level_lookup_set() const;


  /// Get the set of pin-level TIE input queues defined for this core.  
  std::set<std::string> get_pin_level_input_queue_set() const;


  /// Get the set of pin-level TIE output queues defined for this core.  
  std::set<std::string> get_pin_level_output_queue_set() const;


  /// Get the set of pin-level TIE import wires defined for this core.  
  std::set<std::string> get_pin_level_import_wire_set() const;


  /// Get the set of pin-level TIE export states defined for this core.  
  std::set<std::string> get_pin_level_export_state_set() const;


  /// Get the set of pin-level system inputs defined for this core.  
  std::set<std::string> get_pin_level_system_input_set() const;


  /// Get the set of pin-level system outputs defined for this core.  
  std::set<std::string> get_pin_level_system_output_set() const;


  /**
   * Dump the last value (or values) crossing the named interface.
   *
   * For TLM TIE interfaces, the values dump are:
   * \verbatim
      Tie External Register File
          <write_addr> <write_data> <read_check_addr> <read_stall>
          <read_addr> <read_data>

      Tie Lookup (<rdy> only if defined):
          <address> <value> <rdy>

      LX Tie Input Queue:
          <data> <empty> <ticket>

      LX Tie Output Queue:
          <data> <full> <ticket>

      NX Tie Input Queue:
          <data> <entries> <ticket>

      NX Tie Output Queue:
          <data> <spaces> <ticket>
      \endverbatim
   * 
   * @param     interface_name  The name of the TIE or system-level I/O TLM or pin
   *                            interface as shown in the output of
   *                            dump_core_interfaces_by_type().
   *
   * @param     os              The ostream operator to which the interface values
   *                            should be dumped.
   *
   * Note: You can arrange for an sc_event to be notified so you know when to call this
   *       method using the "SimMonitorInterfaces" parameter.
   *
   * @see dump_core_interfaces_by_type()
   * @see xtsc_core_parms "SimMonitorInterfaces"
   */
  void dump_interface_values(const char *interface_name, std::ostream& os = std::cout) const;


  /**
   * Dump a list of all core interfaces grouped by type.
   *
   * Dump a list of all TLM and pin-level core interfaces grouped by type (ext_regfiles,
   * lookups, input queues, output queues, import wires, export states, system input 
   * wires, system output wires, TIE/system input pins, and TIE/system output pins) to the
   * specified ostream object.  If include_memories is set to true, then the dump will
   * also include memory interfaces.
   *
   * The TIE pin-level names always have the "TIE_" prefix.  The TLM TIE names are as
   * written in the user's TIE code and never have the "TIE_" prefix.  In addition, no
   * system-level I/O name starts with "TIE_".
   *
   * @param     os                      The ostream operator to which the interfaces
   *                                    should be dumped.
   *
   * @param     include_memories        If true, memory interfaces are included in the
   *                                    dump; otherwise, they are not included.
   */
  void dump_core_interfaces_by_type(std::ostream& os = std::cout, bool include_memories = false) const;


  /**
   * Dump a list of core System I/O interfaces.
   *
   * Dump a list of all TLM and pin-level core System I/O interfaces to the
   * specified ostream object.
   *
   * @param     os                      The ostream operator to which the interfaces
   *                                    should be dumped.
   */
  void dump_sysio_interfaces(std::ostream& os = std::cout) const;


  /**
   * Dump a list of all core inbound (slave) memory interfaces.
   *
   * @param     os                      The ostream operator to which the interfaces
   *                                    should be dumped.
   */
  void dump_inbound_interfaces(std::ostream& os = std::cout) const;


  /**
   * Dump a list of all core memory interfaces.
   *
   * Dump a list of all core memory interfaces including their starting and ending
   * physical address.
   *
   * @param     os                      The ostream operator to which the interfaces
   *                                    should be dumped.
   */
  void dump_memory_interfaces(std::ostream& os = std::cout) const;


  /**
   * Dump a list of all TLM TIE interfaces.
   *
   * Dump a list of all TLM TIE interfaces to the specified ostream object.
   *
   * The TLM TIE names are as written in the user's TIE code and will never have the
   * "TIE_" prefix.
   */
  void dump_tie_interfaces(std::ostream& os = std::cout) const;


  /**
   * Dump a list of all TLM TIE interfaces grouped by type.
   *
   * Dump a list of all TLM TIE interfaces (ext_regfiles, lookups, input queues, 
   * output queues, import wires, and export states) grouped by type to the 
   * specified ostream object.
   *
   * The TLM TIE names are as written in the user's TIE code and will never have the
   * "TIE_" prefix.
   */
  void dump_tie_interfaces_by_type(std::ostream& os = std::cout) const;


  /**
   * Dump a list of TIE external register files to the specified ostream object.
   */
  void dump_ext_regfiles(std::ostream& os = std::cout) const;


  /**
   * Dump a list of TIE lookups.
   *
   * Dump a list of TIE lookups showing width of lookup result in bits,
   * name of lookup, width of lookup argument in bits, and whether there is 
   * a ready interface to the specified ostream object.
   */
  void dump_lookups(std::ostream& os = std::cout) const;


  /**
   * Dump a list of TIE input queues.
   *
   * Dump a list of TIE input queues showing name (as it appears in the
   * user's TIE code) and width in bits to the specified ostream object.
   */
  void dump_input_queues(std::ostream& os = std::cout) const;


  /**
   * Dump a list of TIE output queues.
   *
   * Dump a list of TIE output queues showing name (as it appears in the
   * user's TIE code) and width in bits to the specified ostream object.
   */
  void dump_output_queues(std::ostream& os = std::cout) const;


  /**
   * Dump a list of TIE import wires.
   *
   * Dump a list of TIE import wires showing name (as it appears in the
   * user's TIE code) and width in bits to the specified ostream object.
   */
  void dump_import_wires(std::ostream& os = std::cout) const;


  /**
   * Dump a list of TIE export states.
   *
   * Dump a list of TIE export states showing name (as it appears in the
   * user's TIE code) and width in bits to the specified ostream object.
   */
  void dump_export_states(std::ostream& os = std::cout) const;


  /**
   * Dump a list of all input pins.
   *
   * Dump a list of all TIE and system input pins showing name and width in bits to the
   * specified ostream object.
   *
   * The pin-level TIE names always have the "TIE_" prefix and the system input pins
   * will never have this prefix.
   */
  void dump_input_pins(std::ostream& os = std::cout) const;


  /**
   * Dump a list of all output pins.
   *
   * Dump a list of all TIE and system output pins showing name and width in bits to the
   * specified ostream object.
   *
   * The pin-level TIE names always have the "TIE_" prefix and the system input pins
   * will never have this prefix.
   */
  void dump_output_pins(std::ostream& os = std::cout) const;


  /**
   * Returns true if the named TLM TIE interface exists.
   *
   * Returns true if the named TLM TIE interface (ext_regfiles, lookup, 
   * input queue, output queue, import wire, or export state) exists.
   *
   * @param     tie_name        This is the name as it appears in the user's TIE code
   *                            after the ext_regfile, lookup, queue, import_wire, or
   *                            state keyword.
   *                            This name should not begin with the "TIE_" prefix.
   */
  bool has_tie_interface(const char *tie_name) const;


  /**
   * Returns true if the named TLM TIE external register file exists.
   *
   * @param     ext_regfile_name Port name as it appears in the user's TIE code after the
   *                             keyword.  This name should not begin with the
   *                             "TIE_" prefix.
   */
  bool has_ext_regfile(const char *ext_regfile_name) const;


  /**
   * Returns true if the named TLM TIE lookup exists.
   *
   * @param     lookup_name     Port name as it appears in the user's TIE code after the
   *                            lookup keyword.  This name should not begin with the
   *                            "TIE_" prefix.
   */
  bool has_lookup(const char *lookup_name) const;


  /**
   * Returns true if the named TLM TIE lookup has a ready interface.
   *
   * This method returns true if the named TIE lookup has a ready interface.  A TIE
   * lookup has a ready interface if the rdy keyword was specified in the lookup section
   * of the user's TIE code.  
   *
   * @param     lookup_name     Port name as it appears in the user's TIE code after the
   *                            lookup keyword.  This name should not begin with the
   *                            "TIE_" prefix.
   *
   * @Throws xtsc_exception if lookup_name is not found.
   */
  bool has_lookup_ready(const char *lookup_name) const;


  /**
   * Returns true if the named TLM TIE input queue exists.
   *
   * @param     queue_name      Port name as it appears in the user's TIE code after the
   *                            queue keyword.  This name should not begin with the
   *                            "TIE_" prefix.
   */
  bool has_input_queue(const char *queue_name) const;


  /**
   * Returns true if the named TLM TIE output queue exists.
   *
   * @param     queue_name      Port name as it appears in the user's TIE code after the
   *                            queue keyword.  This name should not begin with the
   *                            "TIE_" prefix.
   */
  bool has_output_queue(const char *queue_name) const;


  /**
   * Returns true if the named TLM TIE import wire exists.
   *
   * @param     wire_name       Port name as it appears in the user's TIE code after the
   *                            import_wire keyword.  This name should not begin with the
   *                            "TIE_" prefix.
   */
  bool has_import_wire(const char *wire_name) const;


  /**
   * Returns true if the named TLM TIE export state exists.
   *
   * @param     state_name      Port name as it appears in the user's TIE code after the
   *                            state keyword.  This name should not begin with the
   *                            "TIE_" prefix.
   */
  bool has_export_state(const char *state_name) const;


  /**
   * Returns true if the named TIE or system input or output pin exists.
   *
   * @param     pin_name        For TIE pins, this is the pin name as generated by the
   *                            TIE compiler which will always begin with the "TIE_"
   *                            prefix.  For system-level pins, this is the pin name as
   *                            specified in the Xtensa microprocessor data book.
   */
  bool has_pin(const char *pin_name) const;


  /**
   * Returns true if the named TIE or system input pin exists.
   *
   * @param     input_pin_name  For TIE input pins, this is the pin name as generated by
   *                            the TIE compiler which will always begin with the "TIE_"
   *                            prefix.  For system-level input pins, this is the pin
   *                            name as specified in the Xtensa microprocessor data
   *                            book.
   */
  bool has_input_pin(const char *input_pin_name) const;


  /**
   * Returns true if the named TIE or system output pin exists.
   *
   * @param     output_pin_name For TIE output pins, this is the pin name as generated
   *                            by the TIE compiler which will always begin with the
   *                            "TIE_" prefix.  For system-level output pins, this is
   *                            the pin name as specified in the Xtensa microprocessor
   *                            data book.
   */
  bool has_output_pin(const char *output_pin_name) const;


  /**
   * Get the width in bits of the specified pin.
   *
   * Get the width in bits of the specified TIE or system I/O pin.
   *
   * @param     pin_name        For TIE pins, this is the Verilog name as generated by
   *                            the TIE compiler.  This name always begins with the
   *                            "TIE_" prefix and, for TIE ext_regfiles,lookups and 
   *                            queues, has the suffixes specified in the TIE Reference
   *                            Manual.  For system I/O pins, this is the name as 
   *                            specified in the Xtensa microprocessor data book.
   *
   * @see get_sysio_bit_width for obtaining the width of system I/O TLM ports.
   * @see get_tie_bit_width for obtaining the width of TIE TLM ports.
   *
   * @Throws xtsc_exception if pin_name is not found.
   */
  u32 get_pin_bit_width(const char *pin_name) const;


  /**
   * Returns true if the named interface exists at pin-level.
   *
   * Returns true if the named interface (ext_regfile, lookup, input queue, output queue,
   * import wire, export state, or system-level input/output) exists at the pin-level.
   *
   * @param     interface_name  This is either the TIE name as it appears in the user's
   *                            TIE code after the ext_regfile, lookup, queue, import_wire,
   *                            or state keyword (this name should not begin with the "TIE_"
   *                            prefix) or it is the system-level input or output name
   *                            as specified in the Xtensa microprocessor data book.
   */
  bool has_pin_level_interface(const char *interface_name) const;


  /**
   * Returns true if the named pin-level system-level input/output interface exists.
   *
   * @param     sysio_name      This is the name as specified in the Xtensa
   *                            microprocessor data book.
   */
  bool has_pin_level_sysio_interface(const char *sysio_name) const;


  /**
   * Returns true if the named pin-level TIE interface exists.
   *
   * Returns true if the named pin-level TIE interface (ext_regfile, lookup, input queue, 
   * output queue, import wire, or export state) exists.
   *
   * @param     tie_name        This is the name as it appears in the user's TIE code
   *                            after the ext_regfile, lookup, queue, import_wire, or 
   *                            state keyword.
   *                            This name should not begin with the "TIE_" prefix.
   */
  bool has_pin_level_tie_interface(const char *tie_name) const;


  /**
   * Returns true if the named pin-level TIE external register file exists.
   *
   * @param     ext_regfile_name  Port name as it appears in the user's TIE code after the
   *                              ext_regfile keyword.  This name should not begin with the
   *                              "TIE_" prefix.
   */
  bool has_pin_level_ext_regfile(const char *ext_regfile_name) const;


  /**
   * Returns true if the named pin-level TIE lookup exists.
   *
   * @param     lookup_name     Port name as it appears in the user's TIE code after the
   *                            lookup keyword.  This name should not begin with the
   *                            "TIE_" prefix.
   */
  bool has_pin_level_lookup(const char *lookup_name) const;


  /**
   * Returns true if the named pin-level TIE input queue exists.
   *
   * @param     queue_name      Port name as it appears in the user's TIE code after the
   *                            queue keyword.  This name should not begin with the
   *                            "TIE_" prefix.
   */
  bool has_pin_level_input_queue(const char *queue_name) const;


  /**
   * Returns true if the named pin-level TIE output queue exists.
   *
   * @param     queue_name      Port name as it appears in the user's TIE code after the
   *                            queue keyword.  This name should not begin with the
   *                            "TIE_" prefix.
   */
  bool has_pin_level_output_queue(const char *queue_name) const;


  /**
   * Returns true if the named pin-level TIE import wire exists.
   *
   * @param     wire_name       Port name as it appears in the user's TIE code after the
   *                            import_wire keyword.  This name should not begin with the
   *                            "TIE_" prefix.
   */
  bool has_pin_level_import_wire(const char *wire_name) const;


  /**
   * Returns true if the named pin-level TIE export state exists.
   *
   * @param     state_name      Port name as it appears in the user's TIE code after the
   *                            state keyword.  This name should not begin with the
   *                            "TIE_" prefix.
   */
  bool has_pin_level_export_state(const char *state_name) const;


  /**  
   * Get the width in bits of the specified TLM TIE interface.
   *
   * Get the width in bits of the specified TLM TIE interface (ext_regfile, lookup, 
   * input queue, output queue, import wire, or export state).  
   *
   * Note: For TIE ext_regfiles, the width returned is the width of the ext_regfile
   *       data (corresponding to the TIE_xxx_WriteData/TIE_xxx_ReadData Verilog port).
   *       See get_ext_regfile_address_bit_width and get_ext_regfile_data_bit_width.
   *
   * Note: For TIE lookups, the width returned is the width of the lookup response data
   *       (corresponding to the TIE_xxx_In Verilog port).  See
   *       get_lookup_address_bit_width and get_lookup_data_bit_width.
   *
   * @param     tie_name        This is the name as it appears in the user's TIE code
   *                            after the state, import_wire, queue, or lookup keyword.
   *                            This name should not begin with the "TIE_" prefix.
   *
   * @see get_sysio_bit_width for obtaining the width of system I/O TLM ports.
   * @see get_pin_bit_width for obtaining the width of input/output pins.
   *
   * @Throws xtsc_exception if tie_name is not found.
   */
  u32 get_tie_bit_width(const char *tie_name) const;


  /**
   * Get the width in bits of the specified TLM TIE ext_regfile read/write 
   * request address.
   *
   * Get the width in bits of the specified TLM TIE ext_regfile request address
   * (corresponding to the width of the TIE_xxx_WriteAddr/TIE_xxx_ReadAddr 
   * Verilog port, where xxx = ext_regfile_name).
   *
   * @param     ext_regfile_name  Port name as it appears in the user's
   *                              TIE code after the ext_regfile keyword. This
   *                              name should not begin with the "TIE_"
   *                              prefix.
   *
   * @Throws xtsc_exception if ext_regfile_name is not found.
   */
  u32 get_ext_regfile_address_bit_width(const char *ext_regfile_name) const;


  /**
   * Get the width in bits of the specified TIE ext_regfile read/write data.
   *
   * Get the width in bits of the specified TIE ext_regfile response data
   * (corresponding to the width of the TIE_xxx_WriteData/TIE_xxx_ReadData 
   * Verilog port, where xxx = ext_regfile_name).
   *
   * @param     ext_regfile_name  Port name as it appears in the user's
   *                              TIE code after the ext_regfile keyword. This
   *                              name should not begin with the "TIE_"
   *                              prefix.
   *
   * @Throws xtsc_exception if ext_regfile_name is not found.
   */
  u32 get_ext_regfile_data_bit_width(const char *ext_regfile_name) const;


  /**
   * Get the width in bits of the specified TLM TIE lookup request address.
   *
   * Get the width in bits of the specified TLM TIE lookup request address
   * (corresponding to the width of the TIE_xxx_Out Verilog port, where xxx =
   * lookup_name).
   *
   * @param     lookup_name     Port name as it appears in the user's
   *                            TIE code after the lookup keyword.  This
   *                            name should not begin with the "TIE_"
   *                            prefix.
   *
   * @Throws xtsc_exception if lookup_name is not found.
   */
  u32 get_lookup_address_bit_width(const char *lookup_name) const;


  /**
   * Get the width in bits of the specified TIE lookup response data.
   *
   * Get the width in bits of the specified TIE lookup response data
   * (corresponding to the width of the TIE_xxx_In Verilog port, where xxx =
   * lookup_name).
   *
   * @param     lookup_name     Port name as it appears in the user's
   *                            TIE code after the lookup keyword.  This
   *                            name should not begin with the "TIE_"
   *                            prefix.
   *
   * @Throws xtsc_exception if lookup_name is not found.
   */
  u32 get_lookup_data_bit_width(const char *lookup_name) const;


  /**
   * Get the latency of the specified TIE lookup.
   *
   * Get the latency of the specified TIE lookup. Latency is defined as
   * def_stage minus use_stage (where def_stage and use_stage are specified
   * in the TIE lookup section).
   *
   * @param     lookup_name     Port name as it appears in the user's
   *                            TIE code after the lookup keyword.  This
   *                            name should not begin with the "TIE_"
   *                            prefix.
   *
   * @Throws xtsc_exception if lookup_name is not found.
   */
  u32 get_lookup_latency(const char *lookup_name) const;


  /**
   * Get the width in bits of the specified system-level I/O TLM port.
   *
   * Get the width in bits of the specified system-level input or output TLM port.
   *
   * @param     sysio_name      This is the system-level input or output name as it
   *                            appears in the Xtensa microprocessor data book.
   *                            sysio_name must refer to a TLM port.
   *
   * @see get_system_input_wire for a list of valid input names.
   * @see get_system_output_wire for a list of valid output names.
   *
   * @see get_tie_bit_width for obtaining the width of TIE TLM ports.
   * @see get_pin_bit_width for obtaining the width of input/output pins.
   *
   * @Throws xtsc_exception if sysio_name is not found.
   */
  u32 get_sysio_bit_width(const char *sysio_name) const;


#ifndef DOXYGEN_SKIP
  // Deprecated: use get_sysio_bit_width()
  inline u32 get_signal_bit_width(const char *sysio_name) const { return get_sysio_bit_width(sysio_name); }
#endif // DOXYGEN_SKIP


  /**
   * Get the set of system-level wires defined for this core.
   */
  std::set<std::string> get_sysio_wire_set() const;


#ifndef DOXYGEN_SKIP
  // Deprecated: use get_sysio_wire_set()
  inline std::set<std::string> get_signal_set() const { return get_sysio_wire_set(); }
#endif // DOXYGEN_SKIP


  /**
   * Get the set of system-level input wires defined for this core.
   */
  std::set<std::string> get_system_input_wire_set() const;


#ifndef DOXYGEN_SKIP
  // Deprecated: use get_system_input_wire_set()
  inline std::set<std::string> get_input_signal_set() const { return get_system_input_wire_set(); }
#endif // DOXYGEN_SKIP


  /**
   * Get the set of system-level output wires defined for this core.
   */
  std::set<std::string> get_system_output_wire_set() const;
  
  
#ifndef DOXYGEN_SKIP
  // Deprecated: use get_system_output_wire_set()
  inline std::set<std::string> get_output_signal_set() const { return get_system_output_wire_set(); }
#endif // DOXYGEN_SKIP


  /**
   * Dump a list of all system-level wires.
   *
   * Dump a list of all system-level wires showing name and width in bits to the
   * specified ostream object.
   */
  void dump_sysio_wires(std::ostream& os = std::cout) const;


#ifndef DOXYGEN_SKIP
  // Deprecated: use dump_sysio_wires()
  void dump_signals(std::ostream& os = std::cout) const { dump_sysio_wires(os); }
#endif // DOXYGEN_SKIP


  /**
   * Dump a list of all system-level input wires.
   *
   * Dump a list of all system-level input wires showing name and width in bits to the
   * specified ostream object.
   */
  void dump_system_input_wires(std::ostream& os = std::cout) const;


#ifndef DOXYGEN_SKIP
  // Deprecated: use dump_system_input_wires()
  void dump_input_signals(std::ostream& os = std::cout) const { dump_system_input_wires(os); }
#endif // DOXYGEN_SKIP


  /**
   * Dump a list of all system-level output wires.
   *
   * Dump a list of all system-level output wires showing name and width in bits to the
   * specified ostream object.
   */
  void dump_system_output_wires(std::ostream& os = std::cout) const;


#ifndef DOXYGEN_SKIP
  // Deprecated: use dump_system_output_wires()
  inline void dump_output_signals(std::ostream& os = std::cout) const { dump_system_output_wires(os); }
#endif // DOXYGEN_SKIP


  /**
   * Returns true if the named system-level input or output wire exists.
   *
   * @param     wire_name       This is the system-level input or output wire name as it
   *                            appears in the Xtensa microprocessor data book.
   */
  bool has_sysio_wire(const char *wire_name) const;
  
  
#ifndef DOXYGEN_SKIP
  // Deprecated: use has_sysio_wire()
  inline bool has_signal(const char *wire_name) const { return has_sysio_wire(wire_name); }
#endif // DOXYGEN_SKIP


  /**
   * Returns true if the named system-level input wire exists.
   *
   * @param     wire_name       This is the system-level input wire name as it appears
   *                            in the Xtensa microprocessor data book.
   */
  bool has_system_input_wire(const char *wire_name) const;

#ifndef DOXYGEN_SKIP
  // Deprecated: use has_system_input_wire()
  inline bool has_input_signal(const char *wire_name) const { return has_system_input_wire(wire_name); }
#endif // DOXYGEN_SKIP


  /**
   * Returns true if the named system-level output wire exists.
   *
   * @param     wire_name       This is the system-level output wire name as it appears
   *                            in the Xtensa microprocessor data book.
   */
  bool has_system_output_wire(const char *wire_name) const;


#ifndef DOXYGEN_SKIP
  // Deprecated: use has_system_output_wire()
  inline bool has_output_signal(const char *wire_name) const { return has_system_output_wire(wire_name); }
#endif // DOXYGEN_SKIP


  /**
   * Get the value of the named ISS counter.
   *
   * If the named counter is not used on this core then a value of 0 is returned.  If an
   * unrecognized counter name is passed in, an exception is thrown.
   *
   * @param     counter_name    The name of the counter whose value is desired.
   *
   * Counter names include:
   * \verbatim
     LX and NX:
      "Instructions"
      "TakenBranches"
      "ICacheReads"
      "ICacheRefills"
     LX only:
      "DCacheReads"
      "DCacheWrites"
      "DCacheRefills"
      "DCacheCastouts"
     NX only:
      "ScalarDCacheReads"
      "ScalarDCacheWrites"
      "ScalarDCacheRefills"
      "ScalarDCacheCastouts"
      "VectorDCacheReads"
      "VectorDCacheWrites"
      "VectorDCacheRefills"
      "VectorDCacheCastouts"
      \endverbatim
   *
   */
  u64 get_summary_count(const char *counter_name) const;


  /**
   * Get the number of the commit (W) stage of the pipeline.
   *
   * Pipeline stages are numbered as follows:
   *  \verbatim
       Stage number:            0 1 2 3 4
       5-stage pipeline:    P I R E M W
       7-stage pipeline:  P H I R E L M W

       (Stages before R are not numbered.  P is not counted as a stage.)

       P = Prefetch (not counted)
       H = (letter before I)
       I = Instruction fetch
       R = Register access
       E = Execute
       L = (letter before M)
       M = Memory access
       W = register Writeback (aka commit)
      \endverbatim
   */
  u32 get_commit_stage() const;


  /**
   * Get the number of the last stage of the pipeline.
   * @see get_commit_stage
   */
  u32 get_last_stage() const;


  /**
   * Get the byte address of the instruction (that is, the program counter or PC) in 
   * the given pipeline stage.
   *
   * @param     stage           The pipeline stage containing the instruction whose
   *                            address is desired.  If the special stage value of
   *                            0x80000000 is passed in then all stages are checked
   *                            in reverse order starting with the W (commit) stage
   *                            and going to the I stage (Inst Fetch = 0xFFFFFFFF)
   *                            until a stage without a bubble and without an invalid
   *                            instruction is found.  If no such stage is found,
   *                            then 0xFFFFFFFF is returned.
   *
   * @returns The byte address of the instruction in the given stage (if the given 
   * stage contains a bubble or an invalid instruction, 0xFFFFFFFF is returned).
   *
   * @see get_commit_stage
   */
  xtsc_address get_pc(u32 stage = 0x80000000) const;


  /**
   * Set the program counter (PC) to the specified address.
   *
   * @param     address8        The byte address to change the PC to.
   */
  void set_pc(xtsc_address address8);


  /**
   * Get the width (number of bytes) of the instruction pointed to by argument pc.
   *
   * @param     pc              The address of the instruction whose width is desired.
   *                            Default = 0xFFFFFFFF which means to use the current PC.
   *
   */
  u32 get_instr_width(xtsc_address pc = 0xFFFFFFFF);


  /**
   * Get a disassembly of the instruction pointed to by argument pc.
   *
   * @param     buffer          A reference to the string buffer in which to return the
   *                            disassembly.
   *
   * @param     pc              The address of the instruction to be disassembled.
   *                            Default = 0xFFFFFFFF which means to use the current PC.
   *
   * @returns the width (number of bytes) of the instruction
   */
  u32 disassemble(std::string &buffer, xtsc_address pc = 0xFFFFFFFF);


  /**
   * Determine if this core has the named register.
   *
   * @param     register_name   The name of the register.
   *
   * @returns true if the core has the named register, else return false.
   */
  bool has_register(const std::string& register_name) const;


  /**
   * Get the set of names of all registers in this core (including TIE state 
   * and TIE register files but not including the program counter).
   */
  std::set<std::string> get_register_set() const;


  /**
   * Get the value of the named register.
   *
   * @param     register_name   The name of the desired register.  
   *
   * @param     value           A reference to an sc_unsigned object in which to put the
   *                            register value.
   *
   * @Throws xtsc_exception if register_name is not found.
   *
   */
  void get_register_value(const std::string& register_name, sc_dt::sc_unsigned& value) const;


  /**
   * Get the value of the named register.
   *
   * @param     register_name   The name of the desired register.  
   *
   * @Throws xtsc_exception if register_name is not found.
   *
   * @returns the value in the register.
   */
  sc_dt::sc_unsigned get_register_value(const std::string& register_name) const;


  /**
   * Set the value of the named register.
   *
   * @param     register_name   The name of the desired register.  
   *
   * @param     value           The new value of the register.
   *
   * @Throws xtsc_exception if register_name is not found.
   */
  void set_register_value(const std::string& register_name, const sc_dt::sc_unsigned& value);


  /**
   * Get a map of all registers.
   *
   * Get a map of name-value pairs for all registers in this core (including TIE state
   * and TIE register files).  The first item of the map pair is the name of the
   * register.  The second item of the map pair is the value of the register as an
   * sc_unsigned object.
   */
  std::map<std::string, sc_dt::sc_unsigned> get_all_registers() const;


  /**
   * Set multiple registers.
   *
   * Use a map of name-value pairs to set multiple registers in this core (including
   * TIE state and TIE register files).
   *
   * @param     register_map    A map of name-value pairs.  The first item of the map
   *                            pair is the name of the register.  The second item of
   *                            the map pair is the value of the register as an
   *                            sc_unsigned object.  
   *
   * @Throws xtsc_exception if any name in the map is not a valid register name for this
   * core.
   */
  void set_multiple_registers(const std::map<std::string, sc_dt::sc_unsigned>& register_map);


  /**
   * Dump all registers.
   *
   * Dump information about all registers in this core (including TIE state
   * and TIE register files) to the specified ostream object.
   *
   * @param     os      The ostream object.
   */
  void dump_all_registers(std::ostream& os = std::cout) const;


  /**
   * Get the width in bits of the named register.
   *
   * @param     register_name   The name of the desired register.  
   *
   * @Throws xtsc_exception if register_name is not found.
   */
  u32 get_register_bit_width(const std::string& register_name) const;


  /**
   * Get the reset vector address.
   */
  xtsc_address get_reset_vector() const;


  /**
   * Get the latched value of the static vector select input.
   *
   * @Throws xtsc_exception if config does not have the relocatable vectors option.
   *
   * @returns the latched value of the static vector select input.
   */
  u32 get_static_vector_select() const;


  /**
   * Set the value of the static vector select input.
   *
   * @param     value           The new value of the static vector select input.  If the
   *                            core is already running this value will not take effect
   *                            until the core next comes out of reset.
   *
   * @Throws xtsc_exception if config does not have the relocatable vectors option.
   *
   */
  void set_static_vector_select(u32 value);


  /**
   * Get the latched value of the AltResetVec input.
   *
   * @Throws xtsc_exception if config does not have the AltResetVec input.
   *
   * @returns the latched value of the AltResetVec input.
   */
  u32 get_alt_reset_vec() const;


  /**
   * Set the value of the AltResetVec input.
   *
   * @param     address         The new value of the AltResetVec input.  If the
   *                            core is already running this value will not take effect
   *                            until the core next comes out of reset.
   *
   * @Throws xtsc_exception if config does not have the AltResetVec input.
   *
   */
  void set_alt_reset_vec(xtsc_address address);


  /**
   * Get the interrupt number given the BInterrupt index.
   *
   * @param     binterrupt_index        The BInterrupt index of the pin whose 
   *                                    corresponding interrupt number is desired.
   *
   * @Throws xtsc_exception if binterrupt_index is out of range.
   */
  u32 get_interrupt_number(u32 binterrupt_index) const;


  /**
   * Get the BInterrupt index given an external interrupt number.
   *
   * @param     interrupt_number        The external interrupt number whose 
   *                                    corresponding BInterrupt index is desired.
   *
   * @Throws xtsc_exception if interrupt_number is out of range or corresponds to
   * an internal interrupt.
   */
  u32 get_binterrupt_index(u32 interrupt_number) const;


  /**
   * Load the specified file into target memory.
   *
   * Load bytes read from the specified text file into target memory using calls to
   * poke_physical().  The text file should contain lines in the following format:
   * \verbatim
     ([@<Address>] <Byte>*)*
     
     1.  Numbers (<Address> or <Byte>) can be in decimal or hexadecimal.  Hexadecimal
         is specified using the C/C++ style '0x' prefix.
     2.  <Byte> cannot exceed 255 (0xFF).
     3.  If a <Byte> entry is not immediately preceeded in the file by an @<Address>
         entry, then its address is one greater than the preceeding <Byte> entry.
     4.  If the first <Byte> entry in the file is not preceeded by an @<Address> entry,
         then its address is zero.
     5.  Comments, extra whitespace, and blank lines are ignored.  See xtsc_script_file.
    
     Example text file contents:
        @0x3ffe0000
        0x00 0x01 0x02 0x03    // Poke  0 to 0x3ffe0000,  1 to 0x3ffe0001, etc.
        0x09 0x05 0x06 0x07    // Poke  9 to 0x3ffe0004,  5 to 0x3ffe0005, etc.
        @0x3fff0000 0x10 0x11  // Poke 16 to 0x3fff0000, 17 to 0x3fff0001
     \endverbatim
   *
   * The file can be created by hand or by passing the --xtsc option to xt-dumpelf.  For
   * example:
   *
   *     xt-dumpelf --xtsc --width=32 a.out > loadfile.txt
   *
   * If this method is called before simulation starts, then file_name is saved for later
   * loading during the SystemC start_of_simulation() callback.
   *
   * @param     file_name       The path and name of file to be loaded.  If desired
   *                            multiple files can be specfied by separating them with
   *                            a comma.
   *
   * @see xtsc_core_parms "SimLoadFile"
   * @see poke_physical
   */
  void load_file(const char *file_name);


  /**
   * Load the specified program into target memory.
   *
   * When this method is called, if sc_start() has not yet been called (i.e. during
   * elaboration), the program name and arguments are saved for later loading during the
   * SystemC end_of_elaboration() callback.  If sc_start() has been called (i.e. after
   * elaboration), then the program is loaded during the load_program call.
   *
   * @param     program_name    The path and name of the Xtensa executable to be loaded.
   *                            If desired multiple files to be loaded can be specfied
   *                            by separating them with a comma.
   *
   * @param     arguments       A pointer to a NULL-terminated array of c-strings.  Each
   *                            array entry except the last is an additional command-
   *                            line argument to be passed to the target program's main
   *                            function as part of argv[]:
   *                              int main(int argc, char *argv[]) 
   *                            The last array entry must be NULL.  If desired,
   *                            arguments itself can be NULL, in which case no
   *                            additional command-line arguments will be passed to the
   *                            target program's main() function (i.e. argc will be 1
   *                            and argv[0] will be the program name).
   * Note: The program name is always passed into main in argv[0] and should NOT be
   *       included in arguments[].  
   * Note: The "SimTargetProgram" parameter, if not NULL, overrides any call to this
   *       method that occurs prior to the first call to sc_start().
   */
  void load_program(const char *program_name, const char * const arguments[] = NULL);


  /**
   * Load the specified client to this core.
   *
   * See Chapter 5, Client Packages, of the Xtensa ISS UG.
   *
   * @param     client_package  The client package to load.
   *
   * Client packages include:
   * \verbatim
     "cycle_trace [<Arguments>] [<FileName>]"
       Valid <Arguments> include:
         --level 0|1|2
         --start <StartCycle>
         --stop <StopCycle>
     "ferret [--cmdfile=<FileName>]"
     "isa_profile [<Arguments>] [<FileName>]"
       Valid <Arguments> include:
         --disable
         --progname
         --formats
         --no-formats
         --slots
         --no-slots
         --unused-opcodes
     "loadbin <FileName>@<Address>"
     "pchistory <N>"
     "profile [<Arguments> ...] [<FileName>]"
       Valid <Arguments> include:
         --all
         --all-instructions
         --basic-blocks
         --branch-delay
         --combined
         --cycles
         --dcmiss
         --dcmiss-cycles
         --disable
         --force-suffix
         --icmiss
         --icmiss-cycles
         --instructions
         --interlock
         --uncached-ifetch
         --uncached-load
     "stackuse"
     "summary [--disable] [<FileName>]"
     "trace [<Arguments>] [<FileName>]"
       Valid <Arguments> include:
         --level 0|1|2|3|4|5|6
         --nopeek
         --peek
         --short
         --start <InstructionNumber>
         --stop <InstructionNumber>
         --tieprint
         --wtime
     \endverbatim
   *
   * @see xtsc_core_parms "SimClients" parameter
   * @see setup_debug() for information on using --xxprofile as a command line argument
   *      to cause this method to be called for the "profile --all" client.
   */
  void load_client(const char *client_package);


  /**
   * Load clients from a file.
   *
   * Load all clients in the specified file to this core.  See Chapter 5, Client
   * Packages, of the Xtensa ISS UG.
   *
   * @param     file_name       The name of a file containing a list of clients (one per
   *                            line) to load to this core.
   *
   * @see load_client
   * @see xtsc_core_parms "SimClientFile" parameter
   */
  void load_client_file(const char *file_name);


  /**
   * Send a client command.
   *
   * See Chapter 5, Client Packages, of the Xtensa ISS UG.
   *
   * @param     command         The client command to send.
   *
   * Client commands include:
   * \verbatim
     "isa_profile <DynamicCommand>"
       Where <DynamicCommand> can be:
         enable
         disable
         report <FileName>
         reset
     "pchistory state"
     "profile <DynamicCommand>"
       Where <DynamicCommand> can be:
         enable
         disable
         pcsamples <FileName>
     "stackuse state"
     "trace <DynamicCommand>"
       Where <DynamicCommand> can be:
         level 0|1|2|3|4|5|6
         start <InstructionNumber>
         stop <InstructionNumber>
     \endverbatim
   */
  void send_client_command(const char *command);


#ifndef DOXYGEN_SKIP
  // Internal/future use only.  May change or be deleted without notice.
  typedef bool (*turbo_instruction_trace_callback)(void *callback_arg, u32 pc, u32 length);
  void set_turbo_instruction_trace_callback(turbo_instruction_trace_callback callback, void *callback_arg);
  void set_external_watchpoint(xtsc_address address8, u32 size8, u32 access_type, bool set);
  // TurboXim only (cycle-accurate ISS ignores this call).  Target program must already be loaded to memory.
  void set_external_breakpoint(xtsc_address address8, bool set);
  typedef bool (*branch_trace_callback)(void *callback_arg, u32 pc, u32 target_pc, bool taken, bool conditional);
  void set_branch_trace_callback(branch_trace_callback callback, void *callback_arg);
#endif // DOXYGEN_SKIP


  /// Store buffer entry state (for internal/future use)
  enum xtsc_sb_state {
    XTSC_SB_PENDING     = 0,    ///< Store buffer entry pending
    XTSC_SB_DISPATCHING = 2,    ///< Store buffer entry dispatching
    XTSC_SB_DISPATCHED  = 1     ///< Store buffer entry dispatched
  };


  /**
   * Get the number of committed store-buffer entries.
   *
   * This method returns the number of committed store-buffer entries in the specified
   * load/store unit (cycle-accurate simulation mode only).
   *
   * @param     ls_unit         Zero-based index of the load/store unit of interest.
   *
   * Note: This method is for internal/future use and may change without notice.
   *
   * @returns the number of committed store-buffer entries.
   */
  u32 get_store_buffer_count(u32 ls_unit);


  /**
   * This method retrieves the information about the store-buffer entry
   * specified by input parameters ls_unit and sb_index. The information is
   * returned through output parameters p_sb_state, p_address, p_byte_enables,
   * and p_dram_attribute.
   *
   * @param     ls_unit         Zero-based index of the load/store unit of interest.
   *
   * @param     sb_index        Zero-based index of the store-buffer entry of interest.
   *                            Must be less than the value returned by 
   *                            get_store_buffer_count(ls_unit).
   *
   * @param     p_sb_state      If non-NULL, upon return will be set to indicate the
   *                            state of the specified store-buffer entry: 
   *                            - XTSC_SB_PENDING
   *                            - XTSC_SB_DISPATCHING
   *                            - XTSC_SB_DISPATCHED
   *
   * @param     p_address       If non-NULL, upon return will be set to indicate the
   *                            address for the specified store-buffer entry. The
   *                            address is aligned to the load/store access width.
   *
   * @param     p_byte_enables  If non-NULL, upon return will be set to indicate the
   *                            byte enables for the specified store-buffer entry.
   *
   * @param     p_dram_attribute If non-NULL, must point to a user-constructed
   *                            sc_unsigned of length 160 bits which upon return will be
   *                            set to indicate the DRamWrAttr interface value for the
   *                            specified store-buffer entry.
   *
   * Note: This method is for internal/future use and may change without notice.
   *
   * @returns true on success (output parameters are set), false on failure (output
   *          parameters are not set).
   */
  bool get_store_buffer_info(u32                 ls_unit,
                             u32                 sb_index,
                             xtsc_sb_state      *p_sb_state,
                             xtsc_address       *p_address,
                             xtsc_byte_enables  *p_byte_enables,
                             sc_dt::sc_unsigned *p_dram_attribute);


  /**
   * This method retrieves the information about a store operation in the pipeline
   * specified by input parameters ls_unit and stage.  The information is returned
   * through output parameters p_address, p_byte_enables, and p_dram_attribute.
   *
   * @param     ls_unit         Zero-based index of the load/store unit of interest.
   *
   * @param     stage           The number of the pipeline stage of interest.  Must be
   *                            1 (E), 2 (L), or 3 (M) for the 7-stage pipe;
   *
   * @param     p_address       If non-NULL, upon return will be set to indicate the
   *                            address for the specified pipeline store operation. The
   *                            address is aligned to the load/store access width.
   *
   * @param     p_byte_enables  If non-NULL, upon return will be set to indicate the
   *                            byte enables for the specified pipeline store operation.
   *
   * @param     p_dram_attribute If non-NULL, must point to a user-constructed
   *                            sc_unsigned of length 160 bits which upon return will be
   *                            set to indicate the DRamWrAttr interface value for the
   *                            specified pipeline store operation.
   *
   * Note: This method is for internal/future use and may change without notice.
   *
   * @returns true if a valid store operation is found, false otherwise. 
   *          Output parameters are set only when the return value is true.
   */
  bool get_stage_store_info(u32                 ls_unit,
                            u32                 stage,
                            xtsc_address       *p_address,
                            xtsc_byte_enables  *p_byte_enables,
                            sc_dt::sc_unsigned *p_dram_attribute);


  /**
   * Get a vector of all cores in the system.  The cores appear in the vector in order
   * of construction.
   */
  static std::vector<xtsc_core *> get_all_cores();

#if !defined(SCP)
#define SCP
#undef SCP
#endif

  /**
   * Set the relative timing of the 3 clock phases (A, B, and C) for all cores.
   *
   * If the default phase timing is not desired, this method can be used to set when the
   * 3 clock phases (A, B, and C) occur during a system clock period (the clock period
   * can be obtained using the xtsc_get_system_clock_period() method).  The first
   * posedge of system clock occurs at the time specified by the "posedge_offset_factor"
   * parameter of xtsc_initialize_parms and each subsequent posedge clock occurs one
   * system clock period later.
   *
   * The phases, relative to the posedge of the system clock, are set by specifying what
   * we will call a "phase delta factor" or PDF.
   *
   * The "phase delta" for a particular phase is defined as the time from the previous
   * phase (or posedge system clock in the case of phase A) to this phase.
   *
   * \verbatim
      
     posedge                                                                   posedge
  system clock                                                              system clock
        N             Phase   Phase                                   Phase      N+1   
        |               A       B                                       C         |
        |              0.2     0.3                                     0.9        |
        |               |       |                                       |         |
     <==============================================================================>
        |               |       |                                       |       
        |     Phase     | Phase |                Phase                  |       
        |     Delta     | Delta |                Delta                  |       
        |       A       |   B   |                  C                    |       
        |<------------->|<----->|<------------------------------------->|       
      
     \endverbatim
   * The "phase delta factor" for a particular phase is defined as the number to
   * multiply the SystemC time resolution by to get the "phase delta".
   *
   * The "SystemC time resolution" is a SystemC concept.  It can be obtained by calling
   * sc_get_time_resolution().
   *
   * The sum of the 3 "phase deltas" may not exceed the system clock period.
   *
   * The system clock is a conceptual thing, not an actual sc_clock.  Given the
   * system clock period (from xtsc_get_system_clock_period()) and the system clock
   * posedge offset (from xtsc_get_system_clock_posedge_offset()), one can compute what
   * phase of which system clock period any given SystemC simulation time (from
   * sc_time_stamp()) corresponds to.
   *
   * The events and deadlines that occur in each of the three phases are shown below
   * (the ordering is only BETWEEN clock phases--no ordering is implied WITHIN a clock
   * phase):
   * \verbatim
      Phase A: Deadline for nb_respond() calls (see xtsc_core::nb_respond).
               xtsc_core internally handles responses obtained from nb_respond() calls.
               nb_respond()             for non-RSP_NACC responses to inbound PIF/AXI
                                        requests
               nb_lock()                to DRAM (for inbound PIF RCW)
    
      Phase B: nb_request()             to memories
               nb_pop()                 to TIE input queues
               nb_can_push()            to TIE output queues
               nb_push()                to TIE output queues
               nb_write()               to TIE export states
               nb_send_address()        to TIE lookups
               nb_get_data()            to TIE lookups
               TIE_xxx_Out_Req          Pin-level TIE lookup request driven
               TIE_xxx_Out              Pin-level TIE lookup address driven
               TIE_xxx_In               Pin-level TIE lookup response data sampled
               TIE_xxx_PopReq           Pin-level TIE input queue pop request driven
               TIE_xxx                  Pin-level TIE output queue push data driven
               TIE_xxx_PushReq          Pin-level TIE output queue push request driven
               TIE_xxx_Full             Pin-level TIE output queue full signal sampled
               TIE_xxx                  Pin-level TIE export state driven
    
      Phase C: Deadline for inbound PIF/AXI nb_request() calls.
               nb_respond()             for RSP_NACC/NOTRDY responses to inbound PIF/AXI
                                        requests
               nb_can_pop()             to TIE input queues
               nb_num_available_entries() to TIE input queues
               nb_num_free_spaces()     to TIE output queues
               nb_read()                to TIE import wires
               nb_is_ready()            to TIE lookups
               TIE_xxx_Rdy              Pin-level TIE lookup ready signal sampled
               TIE_xxx_Empty            Pin-level TIE input queue empty signal sampled
               TIE_xxx                  Pin-level TIE input queue data sampled
               TIE_xxx_Entries          Pin-level TIE input/output queue entries sampled
               TIE_xxx                  Pin-level TIE import wire sampled

     \endverbatim
   *
   * @param     pdf_a           The "phase delta factor" for phase A.
   *
   * @param     pdf_b           The "phase delta factor" for phase B.
   *
   * @param     pdf_c           The "phase delta factor" for phase C.
   *
   *
   * Note:  The above discussion assumes the core's clock period is the same as the
   *        system clock period, that is, that "SimClockFactor" is 1.  For a core whose
   *        "SimClockFactor" is greater than 1, the core's phase A and phase B take
   *        place during the first system clock period of the core's clock period and
   *        the core's phase C takes place during the last system clock period of the
   *        core's clock period.  As an example, consider a core whose "SimClockFactor"
   *        is 3 running in a simulation with the system clock period equal to 1000 ps
   *        and with default clock phase delta factors.  In this case, the core's first
   *        clock cycle is from 0 ps to 3000 ps and its phase A, phase B, and phase C
   *        for its first clock cycle occur at 200 ps, 300 ps, and 2900 ps respectively.
   *        The core's second clock cycle is from 3000 to 6000 ps and its phase A, phase
   *        B, and phase C for its second clock cycle occur at 3200 ps, 3300 ps, and
   *        5900 ps respectively.  And so on. This assumes the "posedge_offset_factor"
   *        value of xtsc_initialize_parms is 0 (the default).  If a non-zero value for
   *        this parameter is specified then all the times would be increased by an
   *        amount equal to the "posedge_offset_factor" multiplied by the SystemC time
   *        resolution.
   *
   *
   * @see xtsc_initialize_parms
   * @see xtsc_respond_if::nb_respond
   * @see xtsc_set_system_clock_factor
   * @see xtsc_get_system_clock_factor
   * @see xtsc_get_system_clock_period
   * @see get_clock_phase_delta_factors
   * @see Information_on_memory_interface_protocols
   */
  static void set_clock_phase_delta_factors(u32 pdf_a, u32 pdf_b, u32 pdf_c);


  /**
   * Get the relative timing of the 3 clock phases (A, B, and C) for all cores.
   *
   * This method gets when the 3 clock phases (A, B, and C) occur during a system clock
   * cycle.
   *
   * @param     pdf_a           A reference to the object in which to return
   *                            the "phase delta factor" for phase A.
   *
   * @param     pdf_b           A reference to the object in which to return
   *                            the "phase delta factor" for phase B.
   *
   * @param     pdf_c           A reference to the object in which to return
   *                            the "phase delta factor" for phase C.
   *
   * @see set_clock_phase_delta_factors
   */
  static void get_clock_phase_delta_factors(u32& pdf_a, u32& pdf_b, u32& pdf_c);

  /**
   * Set max relaxed cycles for TurboXim.
   *
   * Sets this core's relaxed cycle limit.
   *
   * @param     cycle_limit     The max relaxed cycles expressed in terms of
   *                            system clock periods.
   *
   **/
  void set_turbo_max_relaxed_cycles(u32 cycle_limit);

  /**
   * Get the max relaxed cycles for TurboXim.
   *
   * @returns  This core's max relaxed cycles in terms of system clock periods.
   *
  **/
  u32 get_turbo_max_relaxed_cycles();

  /// Get the TextLogger for this component (e.g. to adjust its log level)
  log4xtensa::TextLogger& get_text_logger() { return m_text; }


  /// Get the BinaryLogger for this component (e.g. to adjust its log level)
  log4xtensa::BinaryLogger& get_binary_logger() { return m_binary; }


private:

  void before_end_of_elaboration();                     ///< Miscellaneous housekeeping before elaboration ends
  void end_of_elaboration();                            ///< Miscellaneous housekeeping after elaboration ends
  void start_of_simulation();                           ///< Miscellaneous housekeeping before simulation starts

  log4xtensa::TextLogger&       m_text;                 ///< For logging text messages
  log4xtensa::BinaryLogger&     m_binary;               ///< Our logging binary messages
  bool                          m_log_data_binary;      ///< True if transaction data should be logged by m_binary

  // Note: Keep m_p last in class
  xtsc_core_parts&              m_p;                    ///< Internal parts

  friend class xtsc_core_intf;
  friend class xtsc_core_parts;
  friend class xtsc_tie_ext_regfile_driver;
  friend class xtsc_tie_lookup_driver;
  friend class xtsc_output_queue_driver;
  friend class xtsc_input_queue_driver;
};



/**
 * Prefix operator++ used to iterate memory_port.
 * For example,
 * \verbatim
       xtsc_core::memory_port p;
       for (p=xtsc_core::MEM_FIRST; p<=xtsc_core::MEM_LAST; ++p) {
          ...
       }
   \endverbatim
 */
XTSC_API xtsc_core::memory_port& operator++(xtsc_core::memory_port& port);



/// Postfix operator++ used to iterate memory_port.
XTSC_API xtsc_core::memory_port& operator++(xtsc_core::memory_port& port, int);



/// Operator += for memory_port.
XTSC_API xtsc_core::memory_port& operator+=(xtsc_core::memory_port& port, int i);



/// Operator + for memory_port.
XTSC_API xtsc_core::memory_port  operator+ (xtsc_core::memory_port& port, int i);


#ifndef DOXYGEN_SKIP
#if defined(CWR_SYSTEMC)
XTSC_API void xtsc_register_coware_wrapper_end_of_elaboration(xtsc_core& core);
XTSC_API void xtsc_register_iss_thread_callbacks(xtsc_core &core, void *arg, void (*before_step_cpu)(void*), bool (*after_step_cpu)(void*));
XTSC_API void xtsc_core_vp_setup(xtsc_core& core);
#endif
#endif // DOXYGEN_SKIP

/**
 * Prefix operator++ used to iterate inbound_port.
 * For example,
 * \verbatim
       xtsc_core::inbound_port p;
       for (p=xtsc_core::INB_FIRST; p<=xtsc_core::INB_LAST; ++p) {
          ...
       }
   \endverbatim
 */
XTSC_API xtsc_core::inbound_port& operator++(xtsc_core::inbound_port& inb_port);



/// Postfix operator++ used to iterate inbound_port.
XTSC_API xtsc_core::inbound_port& operator++(xtsc_core::inbound_port& inb_port, int);



/// Operator += for inbound_port.
XTSC_API xtsc_core::inbound_port& operator+=(xtsc_core::inbound_port& inb_port, int i);



/// Operator + for inbound_port.
XTSC_API xtsc_core::inbound_port  operator+ (xtsc_core::inbound_port& inb_port, int i);



} // namespace xtsc


#endif  // _XTSC_CORE_H_
