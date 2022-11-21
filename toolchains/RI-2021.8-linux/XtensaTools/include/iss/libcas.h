/* Copyright (c) 2003-2012 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.
*/

/* NOTE THIS FILE MUST BE COMPILABLE BY A C COMPILER AS WELL */

#ifndef LIBCAS_H
#define LIBCAS_H

#include "xtensa-isa.h"

#if defined __cplusplus
#  define EXTERN extern "C"
#else
#  define EXTERN extern
#endif

#ifndef XTCORE_DLLEXPORT
#define XTCORE_DLLEXPORT
#endif

#define LIBCAS_VERSION 79903

struct CycleCore;
struct XTCORE;
struct NXCORE;
struct dll_iss_callbacks;


struct dll_data_prefix {
    struct CycleCore *core;
};

typedef struct dll_data dll_data_t;
typedef dll_data_t *dll_data_ptr_t;

typedef unsigned (*dll_get_size_t)(void);
typedef unsigned (*dll_init_data_t)(dll_data_ptr_t,
                                    struct CycleCore *,
                                    struct dll_iss_callbacks *);
typedef void (*dll_enable_events_t)(dll_data_ptr_t, int);
typedef void (*dll_reset_states_t)(dll_data_ptr_t);
typedef void (*dll_advance_t)(dll_data_ptr_t);
typedef void (*dll_partial_advance_t)(dll_data_ptr_t, int);
typedef void (*dll_reset_cycle_advanced_t)(dll_data_ptr_t);
typedef void (*dll_kill_t)(dll_data_ptr_t, int start_stage, int end_stage);
typedef int (*dll_instruction_get_length_t)(dll_data_ptr_t,unsigned short); 
typedef int (*dll_export_state_stall_t)(dll_data_ptr_t, int stage); 

typedef int (*dll_stall_fcn)(dll_data_ptr_t, xtensa_insnbuf insn);
typedef struct dll_stall_struct 
{
    char *opcode_name; 
    dll_stall_fcn stall_function;
} dll_stall_t;

typedef struct dll_slot_stall_struct 
{
    char *format_name;
    char *slot_name;
    int slot_position;
    const dll_stall_t *stall_functions;
} dll_slot_stall_t;
typedef const dll_slot_stall_t *(*dll_get_stall_t)(void);

typedef void (*dll_issue_fcn)(dll_data_ptr_t, xtensa_insnbuf insn);
typedef struct dll_issue_struct 
{
    char *opcode_name; 
    dll_issue_fcn issue_function;
} dll_issue_t;

typedef struct dll_slot_issue_struct 
{
    char *format_name;
    char *slot_name;
    int slot_position;
    const dll_issue_t *issue_functions;
} dll_slot_issue_t;
typedef const dll_slot_issue_t *(*dll_get_issue_t)(void);

typedef void (*dll_stage_fcn)(dll_data_ptr_t, xtensa_insnbuf insn, int gstall);
typedef struct dll_semantic_struct {
    char *opcode_name; 
    unsigned num_stages;
    const dll_stage_fcn *stage_functions;
} dll_semantic_t;

typedef void (*dll_etie_module_fcn)(dll_data_ptr_t, int gstall);
typedef void (*dll_etie_tcm_fcn)(dll_data_ptr_t);
typedef int (*dll_etie_axi_slave_post_fcn)(dll_data_ptr_t, int address, int byte_width, int size, long long byte_en, unsigned *data, int block_num);
typedef int (*dll_etie_axi_master_post_fcn)(dll_data_ptr_t, int response, int status, unsigned *data, int block_num);

typedef struct dll_etie_module_struct {
    char *module_name; 
    unsigned num_stages;
    const dll_etie_module_fcn *stage_functions;
    const dll_etie_tcm_fcn tcm_request_function;
    const dll_etie_axi_slave_post_fcn *axi_slave_post_functions;
    const dll_etie_axi_master_post_fcn *axi_master_post_functions;
} dll_etie_module_t;

typedef struct dll_slot_semantic_struct {
    char *format_name;
    char *slot_name;
    int slot_position;
    const dll_semantic_t *semantic_functions;
} dll_slot_semantic_t;

typedef const dll_slot_semantic_t *(*dll_get_stage_t)(void);
typedef const dll_etie_module_t *(*dll_get_etie_hardware_t)(void);

typedef 
void (*dll_state_use_value_fcn)(dll_data_ptr_t, int stage, unsigned *buf);
typedef
int  (*dll_state_stage_value_fcn)(dll_data_ptr_t, int stage, unsigned *buf);
typedef
int  (*dll_state_set_stage_value_fcn)(dll_data_ptr_t, int stage, unsigned *buf);
typedef
void (*dll_state_commit_value_fcn)(dll_data_ptr_t, unsigned *buf);
typedef
void (*dll_state_set_commit_value_fcn)(dll_data_ptr_t, unsigned *buf);

typedef struct dll_state_struct {
    char *name;
    int width;
    dll_state_use_value_fcn        use_value;
    dll_state_stage_value_fcn      stage_value;
    dll_state_commit_value_fcn     commit_value;
    dll_state_set_stage_value_fcn  set_stage_value;
    dll_state_set_commit_value_fcn set_commit_value;
} dll_state_t;
typedef const dll_state_t *(*dll_get_state_table_t)(void);

typedef
int  (*dll_regfile_stage_value_fcn)(dll_data_ptr_t, int stage, 
                                    int index, unsigned *buf);
typedef
int  (*dll_regfile_set_stage_value_fcn)(dll_data_ptr_t, int stage, 
                                        int index, unsigned *buf);
typedef
void (*dll_regfile_commit_value_fcn)(dll_data_ptr_t, 
                                     int index, unsigned *buf);
typedef
void (*dll_regfile_set_commit_value_fcn)(dll_data_ptr_t, 
                                         int index, unsigned *buf);
typedef struct dll_regfile_struct {
    char *name;
    char *shortname;
    int width;
    int depth;
    dll_regfile_stage_value_fcn      stage_value;
    dll_regfile_commit_value_fcn     commit_value;
    dll_regfile_set_stage_value_fcn  set_stage_value;
    dll_regfile_set_commit_value_fcn set_commit_value;
} dll_regfile_t;
typedef const dll_regfile_t *(*dll_get_regfile_table_t)(void);

typedef struct dll_ext_regfile_struct {
    const char *name;
    const char *shortname;
    int width;
    int depth;
    int vector;
} dll_ext_regfile_t;
typedef const dll_ext_regfile_t *(*dll_get_ext_regfile_table_t)(void);

typedef void (*dll_exception_handler_fcn)(dll_data_ptr_t);
typedef struct dll_exception_struct {
    char *name;
    dll_exception_handler_fcn handler;
} dll_exception_t;
typedef const dll_exception_t *(*dll_get_exceptions_t)(void);

#define CORE_SIGNAL_INT unsigned long long
typedef CORE_SIGNAL_INT (*dll_core_signal_handler_fcn)(struct CycleCore *);
typedef struct dll_core_signal_struct {
    char *name;
    dll_core_signal_handler_fcn handler;
} dll_core_signal_t;
typedef const dll_core_signal_t *(*dll_get_core_signals_t)(void);

typedef void
(*dll_interface_function)(void *link, void *core, void *device, unsigned *data);

typedef struct dll_interface_table_entry {
    char *name;
    unsigned stage;
} dll_interface_table_entry_t;

#define TIE_PORT_GROUP_ExportState   0x00
#define TIE_PORT_GROUP_ImportWire    0x01
#define TIE_PORT_GROUP_InputQueue    0x02 
#define TIE_PORT_GROUP_OutputQueue   0x03
#define TIE_PORT_GROUP_Lookup        0x04
#define TIE_PORT_GROUP_LookupMemory 0x104
#define TIE_PORT_GROUP_ExtRegfile    0x05
#define TIE_PORT_GROUP_InputPort    0x06
#define TIE_PORT_GROUP_OutputPort    0x07

typedef int (*dll_tie_port_group_pop)(dll_data_ptr_t, unsigned *dst);
typedef int (*dll_tie_port_group_push)(dll_data_ptr_t, const unsigned *src);
typedef unsigned (*dll_tie_port_group_maxcount)(dll_data_ptr_t);

typedef const char **(*dll_get_shared_functions_t)(void);

typedef struct dll_queue_info {
    const char *name;
    int dir;
    int vector;
    int width;
} dll_queue_info_t;
typedef const dll_queue_info_t *(*dll_get_queue_info_t)(void);

typedef struct dll_lookup_struct {
    const char *name;
    int output_width;
    int def_stage;
    int input_width;
    int use_stage;
    int rdy;
} dll_lookup_t;
typedef const dll_lookup_t *(*dll_get_lookup_table_t)(void);

typedef struct dll_system_port_struct {
    const char *name;
    int dir; // 0: output, 1: input
    int width;
} dll_system_port_t;
typedef const dll_system_port_t *(*dll_get_system_port_info_t)(void);

typedef struct dll_etie_itf_struct {
    const char *name;
    unsigned kind;
} dll_etie_itf_t;
typedef const dll_etie_itf_t *(*dll_get_etie_itf_info_t)(void);

EXTERN XTCORE_DLLEXPORT void
nx_queue_DATA_use(struct NXCORE *nxcore, const char *name);

EXTERN XTCORE_DLLEXPORT void
nx_queue_NOTRDY_use(struct NXCORE *nxcore, const char *name);

EXTERN XTCORE_DLLEXPORT void
nx_queue_KILL(struct NXCORE *nxcore, const char *name, unsigned *value);

EXTERN XTCORE_DLLEXPORT void
nx_queue_NOTRDY(struct NXCORE *nxcore, const char *name, unsigned *value);

EXTERN XTCORE_DLLEXPORT void
nx_queue_DATA(struct NXCORE *nxcore, const char *name, unsigned *value);

EXTERN XTCORE_DLLEXPORT void
nx_GSEnable_1_interface(struct NXCORE *nxcore, unsigned *data);

EXTERN XTCORE_DLLEXPORT void
nx_lookup_req(struct NXCORE *nxcore, const char *name);

EXTERN XTCORE_DLLEXPORT void
nx_lookup_def(struct NXCORE *nxcore, const char *name, unsigned *out);

EXTERN XTCORE_DLLEXPORT void
nx_lookup_use(struct NXCORE *nxcore, const char *name, unsigned *in);

/* ISS functions that are called from libcas */
typedef void (*iss_cycle_core_cb)(struct CycleCore *core,
                                  unsigned *value);

typedef void (*iss_link_state)(struct CycleCore *core,
                               const char *name,
                               void *info);

typedef void (*iss_link_symbol)(struct CycleCore *core,
                                const char *name,
                                void *sym,
                                void *info);

typedef void (*iss_link_interface)(struct CycleCore *cas_core,
                                   const char *name,
                                   int in,
                                   int width,
                                   dll_interface_function *func,
                                   void **link,
                                   void **core,
                                   void **device);

typedef void (*iss_bind_interface)(struct CycleCore *cas_core,
                                   const char *name,
                                   dll_interface_function f,
                                   void *core,
                                   void *device);

typedef void (*iss_add_tie_port_group)(struct CycleCore *core,
                                       const char *name,
			               unsigned kind,
                                       unsigned latency,
			               dll_data_ptr_t data,
			               dll_tie_port_group_maxcount max_fn,
			               dll_tie_port_group_pop pop_fn,
			               dll_tie_port_group_push push_fn);

typedef void (*iss_add_wire)(struct CycleCore *core,
                             int slot,
                             const char *block_name, 
                             char *wire_name,
                             int width,
                             unsigned *data);

typedef void (*iss_update_wire)(struct CycleCore *core,
                                unsigned *data,
                                unsigned words);

typedef int (*iss_export_state_stall)(struct CycleCore *core,
                                      int stage);

typedef unsigned (*iss_iterative_insn)(struct CycleCore *core,
                                       unsigned stage,
                                       unsigned opctl,
                                       unsigned opnd1,
                                       unsigned opnd2);

typedef int (*iss_iterative_stall_count)(struct CycleCore *core,
                                         unsigned opctl,
                                         unsigned opnd1,
                                         unsigned opnd2);

typedef void (*iss_raise_exception)(struct CycleCore *core,
                                    unsigned stage,
                                    unsigned exc);

typedef int (*iss_set_iterative_stall)(struct CycleCore *core,
                                       unsigned stage);

typedef void (*iss_regfile_event)(struct CycleCore *core,
                                  int stage,
                                  const char *name,
                                  int idx,
                                  unsigned width,
                                  const unsigned *value,
                                  int update);

typedef void (*iss_state_event)(struct CycleCore *core,
                                int stage,
                                const char *name,
                                unsigned width,
                                const unsigned *value,
                                int update);

typedef void (*iss_tie_port_event)(struct CycleCore *core,
                                   int stage,
                                   const char *name,
                                   const char *type,
                                   unsigned width,
                                   const unsigned *value);

typedef void (*iss_tie_print_event)(struct CycleCore *core,
                                    int stage,
                                    unsigned slot,
                                    unsigned order,
                                    const char *tie_printf_output);

typedef void (*iss_etie_print_event)(struct CycleCore *core,
                                     const char *etie_printf_output);

typedef void (*iss_set_interrupt)(struct CycleCore *core,
                                  unsigned idx);

typedef void (*iss_update_etie)(struct CycleCore *core);

typedef void (*iss_ext_regfile_addr)(struct CycleCore *core,
                                    const char *name,
                                    int addr);

typedef void (*iss_ext_regfile_rdwr)(struct CycleCore *core,
                                     const char *name,
                                     unsigned *data);

typedef void (*iss_ext_regfile_use)(struct CycleCore *core,
                                    const char *name);

typedef void (*iss_external_regfile_access)(struct CycleCore *core,
                                            unsigned *data,
                                            unsigned slot,
                                            unsigned idx,
                                            unsigned width);

typedef void (*iss_schedule_tie_port_func)(struct CycleCore *core,
                                           void *func,
                                           void *info,
                                           int stage,
                                           int end_of_cycle);

typedef void (*iss_cycle_core_schedule)(struct CycleCore *core,
                                        void *func,
                                        void *info);

typedef void (*iss_etie_tcm_request)(struct XTCORE *xtc, int, int, unsigned, 
                                     unsigned, unsigned, unsigned *,
                                     unsigned *, unsigned *, unsigned *);

typedef void (*iss_xtcore_process_stage)(struct XTCORE *xtc,
                                         int stage);

typedef int (*iss_xtcore_signal)(struct XTCORE *xtc);

typedef int (*iss_xtcore_stage_killed)(struct XTCORE *xtc,
                                       unsigned m);

typedef void (*iss_xtcore_interface)(struct XTCORE *xtc,
                                     unsigned *value);

typedef void (*iss_invoke_interface)(struct CycleCore *cas_core,
        const char* name, void *link, unsigned *data);
        
struct dll_iss_callbacks {
    iss_link_state link_state;
    iss_link_state link_regfile;
    iss_link_state link_ext_regfile;
    iss_link_state link_shared_function;
    iss_link_symbol link_symbol;
    iss_link_interface link_interface;
    iss_bind_interface bind_interface;
    iss_invoke_interface invoke_interface;
    iss_add_tie_port_group add_tie_port_group;
    iss_add_wire add_wire;
    iss_update_wire update_wire;
    iss_export_state_stall export_state_stall;
    iss_iterative_insn iterative_insn;
    iss_iterative_stall_count iterative_stall_count;
    iss_set_iterative_stall set_iterative_stall;
    iss_raise_exception raise_exception;
    iss_regfile_event regfile_event;
    iss_state_event state_event;
    iss_tie_port_event tie_port_event;
    iss_tie_print_event tie_print_event;
    iss_etie_print_event etie_print_event;
    iss_set_interrupt set_interrupt;
    iss_update_etie update_etie;
    iss_ext_regfile_addr ext_regfile_set_def;
    iss_ext_regfile_addr ext_regfile_read_req;
    iss_ext_regfile_rdwr ext_regfile_sample_read_data;
    iss_ext_regfile_rdwr ext_regfile_def;
    iss_ext_regfile_use ext_regfile_use;
    iss_external_regfile_access external_regfile_access;
    iss_schedule_tie_port_func schedule_tie_port_func;
    iss_cycle_core_schedule schedule_for_start_of_free_cycle;
    iss_cycle_core_schedule schedule_for_end_of_global_cycle;
    iss_cycle_core_schedule schedule_for_end_of_stall_cycle;
    iss_cycle_core_schedule schedule_for_end_of_free_cycle;

    iss_xtcore_process_stage process_stage;
    iss_xtcore_process_stage set_tie_stall_eval;
    iss_etie_tcm_request etie_tcm_request;
    iss_xtcore_signal global_stall;
    iss_xtcore_signal interrupt_stall_m;
    iss_xtcore_signal killpipe_w;
    iss_xtcore_stage_killed stage_killed;

    iss_xtcore_interface AlternateResetVector_interface;
    iss_xtcore_interface ArithmeticException_interface;
    iss_xtcore_interface BlockPrefetchLength_interface;
    iss_xtcore_interface BranchTaken_interface;
    iss_xtcore_interface BranchTarget_interface;
    iss_xtcore_interface BusErrorType_interface;
    iss_xtcore_interface DBreakNum_interface;
    iss_xtcore_interface DRamAttr_0_interface;
    iss_xtcore_interface DRamAttr_1_interface;
    iss_xtcore_interface DRamWrAttr_0_interface;
    iss_xtcore_interface DRamWrAttr_1_interface;
    iss_xtcore_interface DTlbMissProc_interface;
    iss_xtcore_interface ERRead_interface;
    iss_xtcore_interface ERWrite_interface;
    iss_xtcore_interface EXCLRES_interface;
    iss_xtcore_interface EXCLRES_next_interface;
    iss_xtcore_interface ExceptionPC_interface;
    iss_xtcore_interface ExceptionVAddr_interface;
    iss_xtcore_interface ExceptionVector_interface;
    iss_xtcore_interface GSControl_0_interface;
    iss_xtcore_interface GSControl_1_interface;
    iss_xtcore_interface GSEnable_0_interface;
    iss_xtcore_interface GSEnable_1_interface;
    iss_xtcore_interface GSVAddrOffset_0_interface;
    iss_xtcore_interface GSVAddrOffset_1_interface;
    iss_xtcore_interface ITlbMissProc_interface;
    iss_xtcore_interface InvalidOperand_interface;
    iss_xtcore_interface LCOUNT_interface;
    iss_xtcore_interface LSBytes_0_interface;
    iss_xtcore_interface LSBytes_1_interface;
    iss_xtcore_interface LoadByteDisable_0_interface;
    iss_xtcore_interface LoadByteDisable_1_interface;
    iss_xtcore_interface Load_0_interface;
    iss_xtcore_interface Load_1_interface;
    iss_xtcore_interface MEMCTLOut_interface;
    iss_xtcore_interface MMUDataIn_interface;
    iss_xtcore_interface MMUDataOut_interface;
    iss_xtcore_interface MMUVAddr0_interface;
    iss_xtcore_interface MMUVAddr1_interface;
    iss_xtcore_interface MMUVAddrStatus_interface;
    iss_xtcore_interface MemDataIn128_0_interface;
    iss_xtcore_interface MemDataIn128_1_interface;
    iss_xtcore_interface MemDataIn16_0_interface;
    iss_xtcore_interface MemDataIn16_1_interface;
    iss_xtcore_interface MemDataIn256_0_interface;
    iss_xtcore_interface MemDataIn256_1_interface;
    iss_xtcore_interface MemDataIn32_0_interface;
    iss_xtcore_interface MemDataIn32_1_interface;
    iss_xtcore_interface MemDataIn512_0_interface;
    iss_xtcore_interface MemDataIn512_1_interface;
    iss_xtcore_interface MemDataIn64_0_interface;
    iss_xtcore_interface MemDataIn64_1_interface;
    iss_xtcore_interface MemDataIn8_0_interface;
    iss_xtcore_interface MemDataIn8_1_interface;
    iss_xtcore_interface MemDataMPSYNC_interface;
    iss_xtcore_interface MemDataOut128_0_interface;
    iss_xtcore_interface MemDataOut128_1_interface;
    iss_xtcore_interface MemDataOut16_0_interface;
    iss_xtcore_interface MemDataOut16_1_interface;
    iss_xtcore_interface MemDataOut256_0_interface;
    iss_xtcore_interface MemDataOut256_1_interface;
    iss_xtcore_interface MemDataOut32_0_interface;
    iss_xtcore_interface MemDataOut32_1_interface;
    iss_xtcore_interface MemDataOut512_0_interface;
    iss_xtcore_interface MemDataOut512_1_interface;
    iss_xtcore_interface MemDataOut64_0_interface;
    iss_xtcore_interface MemDataOut64_1_interface;
    iss_xtcore_interface MemDataOut8_0_interface;
    iss_xtcore_interface MemDataOut8_1_interface;
    iss_xtcore_interface MemErrAddr_interface;
    iss_xtcore_interface MemErrCastout_interface;
    iss_xtcore_interface MemErrIFData_interface;
    iss_xtcore_interface MemErrLSData0_interface;
    iss_xtcore_interface MemErrLSData1_interface;
    iss_xtcore_interface MemErrLSData2_interface;
    iss_xtcore_interface MemErrLSData3_interface;
    iss_xtcore_interface MemErrMemory_interface;
    iss_xtcore_interface MemErrTestIFData_interface;
    iss_xtcore_interface MemErrTestLSData0_interface;
    iss_xtcore_interface MemErrTestLSData1_interface;
    iss_xtcore_interface MemErrTestLSData2_interface;
    iss_xtcore_interface MemErrTestLSData3_interface;
    iss_xtcore_interface MemErrType_interface;
    iss_xtcore_interface MemErrWay_interface;
    iss_xtcore_interface MemErrWord_interface;
    iss_xtcore_interface NextOCDEnabled_interface;
    iss_xtcore_interface NextPC_interface;
    iss_xtcore_interface OCDEnabled_interface;
    iss_xtcore_interface PC_interface;
    iss_xtcore_interface PIFAttribute_0_interface;
    iss_xtcore_interface PIFAttribute_1_interface;
    iss_xtcore_interface RERAddr_interface;
    iss_xtcore_interface RERBus_interface;
    iss_xtcore_interface ROMPatchPC_interface;
    iss_xtcore_interface RSRBus_interface;
    iss_xtcore_interface RSRTraceBus_interface;
    iss_xtcore_interface ReplayNextInstruction_interface;
    iss_xtcore_interface RotateAmount_0_interface;
    iss_xtcore_interface RotateAmount_1_interface;
    iss_xtcore_interface SRAddr_interface;
    iss_xtcore_interface SRWrite_interface;
    iss_xtcore_interface ScatterData_0_interface;
    iss_xtcore_interface ScatterData_1_interface;
    iss_xtcore_interface SignExtendFrom_0_interface;
    iss_xtcore_interface SignExtendFrom_1_interface;
    iss_xtcore_interface SignExtendTo_0_interface;
    iss_xtcore_interface SignExtendTo_1_interface;
    iss_xtcore_interface StartHWDTLBRefill_interface;
    iss_xtcore_interface StartHWITLBRefill_interface;
    iss_xtcore_interface StatVectorSel_interface;
    iss_xtcore_interface StoreByteDisable_0_interface;
    iss_xtcore_interface StoreByteDisable_1_interface;
    iss_xtcore_interface StoreDisablesWords_0_interface;
    iss_xtcore_interface StoreDisablesWords_1_interface;
    iss_xtcore_interface StoreHasByteDisable_0_interface;
    iss_xtcore_interface StoreHasByteDisable_1_interface;
    iss_xtcore_interface StoreWordDisable_0_interface;
    iss_xtcore_interface StoreWordDisable_1_interface;
    iss_xtcore_interface Store_0_interface;
    iss_xtcore_interface Store_1_interface;
    iss_xtcore_interface VAddr_0_interface;
    iss_xtcore_interface VAddr_1_interface;
    iss_xtcore_interface WERAddr_interface;
    iss_xtcore_interface WERBus_interface;
    iss_xtcore_interface WSRBus_interface;
    iss_xtcore_interface WSRTraceBus_interface;

    iss_cycle_core_cb DBREAK_core_signal;
    iss_cycle_core_cb MEMCTL_core_signal;
    iss_cycle_core_cb MPUENB_core_signal;
    iss_cycle_core_cb IBREAKA_core_signal;
    iss_cycle_core_cb APB0CFG_core_signal;
    iss_cycle_core_cb OPMODEECCFENCE_core_signal;
    // RNX core signals 
    iss_cycle_core_cb MMEMCTL_core_signal;
    iss_cycle_core_cb MSECCFG_core_signal;
    iss_cycle_core_cb MMMIOBASE_core_signal;
    iss_cycle_core_cb MAPBBASE_core_signal;
};

#endif

/*
 * Local Variables:
 * mode:c++
 * c-basic-offset:4
 * End:
 */
