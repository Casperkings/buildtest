// Copyright (c) 2007-2009 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Tensilica Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Tensilica Inc.

;#
;# This file is copied to swtools-{machine}-{os}/src/system/xtsc-run/SYSTEM.cosim.systemc.on.top.inc.tpp
;# by running "make install-src" in builds-{machine}-{os}/_swtools_/xbuild/Software
;#
;sub hx {
;  my ($nibbles, $value) = @_;
;  sprintf("0x%0". $nibbles. "X", $value);
;}
;#
;#
; stash_obj_isa($sys, "MultiCoreSystem") or die "system must be of type MultiCoreSystem";
; use Xtensa::AddressLayout;	# HACK - until Stash implements class autoload
; my $system = $sys->name;
; my @cores = @{$sys->cores};
; my $core0 = $cores[0];
; my $big_endian = (($core0->sys->pr->xt->core->memoryOrder eq "BigEndian") ? "true" : "false");
; my $pif_byte_width = $core0->sys->pr->xt->core->pifReadDataBits / 8;
; my $num_cores = @cores;
; my $coherent = 1;
; my $byte_width = 4;
; my $num_transfers = 4;
; my @BInterruptSize;
; my $shared_interrupts = 32;
;#
;###  List memories:
;#
; my %memories;
; foreach my $m ($sys->cc->ref_map->a) {
;   my $name = $m->mem->name;
;   next if $name eq "inbound";	# FIXME: GROSS HACK: skip connections from CC to inbound PIFs for now
;   die "memory $name mapped multiple times on PIF or CC"
;	if exists($memories{$name});
;   $memories{$name} = $m;
; }
; my @memnames = sort keys %memories;
; my $num_memories = @memnames;
; #  First ROM (by name order):
; my ($default_memory_port) = grep( $memories{$memnames[$_]}->mem->is('writable'),
;					0 .. $#memnames );
; $default_memory_port = $#memnames unless defined($default_memory_port);	# if no ROM, pick last RAM
;#
;#
; my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
; $year = 1900 + $year;
; $mon = $mon + 1;
; $min = 100 + $min;
; $min = substr($min, 1);
; my $ampm = "AM";
; if ($hour >= 12) {
;   $ampm = "PM";
;   if ($hour > 12) {
;     $hour = $hour - 12;
;   }
; }
; my $date = "$year/$mon/$mday $hour:$min $ampm";
;#
;#
// Generated on `$date`

//  Instructions:
//  This xtsc-run include file can be used as is if you only need to specify target program
//  names and argv arguments.  To use this file as is:
//  In your main xtsc-run --include file, define the program for each core and optionally
//  define the argv arguments for each core program's main() function then #include this
//  file.  For example:

/*
;#
; foreach my $i (0 .. $#cores) {
;   my $core = $cores[$i];
#define `$core->name`_PROGRAM=`$core->name`.out
#define `$core->name`_ARGV=`$i`
; }
#include "`$system`.inc"
*/

//  If you need to make other changes, for example to enable core debugging or change
//  various module parameters, then Tensilica recommends that you copy this file and 
//  all *.def files to a new location and modify the copy.


#ifeq ($(XTSC_RUN_COSIM_VENDOR),synopsys)
#define READMEMH_FILE_NAME=
#define TOP=sYsTeMcToP
#else
#define READMEMH_FILE_NAME=<none>
#define TOP=sc_main
#endif

#ifeq ($(XTSC_RUN_COSIM_VENDOR_VERSION),Y-2006.06-SP1-9)
#define TOP=simv
#endif

;#
;#
; foreach my $i (0 .. $#cores) {
;   my $core = $cores[$i];
;   $byte_width = $core->sys->pr->xt->core->pifReadDataBytes;
;   if (!defined($core->sys->pr->dcache) || !$core->sys->pr->dcache->coherence) {
;     $coherent = 0;
;   } else {
;     my $line_size = $core->sys->pr->dcache->lineSize;
;     $num_transfers = ($byte_width && $line_size ? ($line_size / $byte_width) : $num_transfers);
;     $BInterruptSize[$i] = 0;
;     my @interrupts = $core->sys->pr->xt->interrupts->interrupt;
;     foreach my $j (0 .. $#interrupts) {
;       my $type = $interrupts[$j]->type;
;       if (($type eq "ExtLevel") || ($type eq "ExtEdge") || ($type eq "NMI")) {
;         $BInterruptSize[$i] += 1;
;       }
;     }
;     if ($BInterruptSize[$i] < $shared_interrupts) { $shared_interrupts = $BInterruptSize[$i]; }
;   }

#ifndef `$core->name`_ARGV
#define `$core->name`_ARGV=
#endif
; }
;#
;#
; if ($coherent) {
;   open(FH, ">MPSCORE_RunStall.def");
;   print(FH "// Generated on $date" . "\n");
;   foreach my $i (0 .. $#cores) {
;     if ($i == 0) {
;       print(FH "input MPSCORE 16 0" . "\n");
;     } else {
;       print(FH "output RunStall$i 1 0" . "\n");
;       print(FH "assign RunStall$i = MPSCORE[$i]" . "\n");
;     }
;   }
;   close(FH);

// Configure and create xtsc_wire_logic MPSCORE_RunStall
--set_logic_parm=definition_file=$(XTSC_SCRIPT_FILE_PATH)/MPSCORE_RunStall.def
--create_logic=MPSCORE_RunStall

// Configure and create xtsc_interrupt_distributor distributor
--set_distributor_parm=allow_bad_address=false
--set_distributor_parm=num_interrupts=`$shared_interrupts - 2`
// TODO: get syscfgid 
--set_distributor_parm=syscfgid=0xdeadbeef
--set_distributor_parm=num_ports=`$num_cores`
--create_distributor=distributor

// Configure and create xtsc_cohctrl cohctrl
--set_cohctrl_parm=byte_width=`$byte_width`
--set_cohctrl_parm=num_transfers=`$num_transfers`
--set_cohctrl_parm=num_clients=`$num_cores`
--create_cohctrl=cohctrl
; } else {

// Configure and create xtsc_arbiter arbiter
--set_arbiter_parm=num_masters=`$num_cores`
--create_arbiter=arbiter
; }
;#
;#
; open(FH, ">routing.def");
; if ($num_memories > 1) {
;   print(FH "// Generated on $date" . "\n");
;   print(FH "//<PortNum>  <LowAddr>   <HighAddr>  [<NewBaseAddr>]" . "\n");

// Configure, create, and connect xtsc_router router
--set_router_parm=num_slaves=`$num_memories`
--set_router_parm=routing_table=$(XTSC_SCRIPT_FILE_PATH)/routing.def
--set_router_parm=default_port_num=`$default_memory_port`
--create_router=router
;   if ($coherent) {
--connect_cohctrl_router=cohctrl,router
;   } else {
--connect_arbiter_router=arbiter,router
;   }
; }
;#
;#
; my $memory_index = 0;
; foreach my $key (@memnames) {
;   my $m = $memories{$key};

// Configure, create, and connect xtsc_tlm2pin_memory_transactor t2p_`$key`
--set_tlm2pin_parm=big_endian=`$big_endian`
--set_tlm2pin_parm=write_responses=false
--set_tlm2pin_parm=cosim=true
--set_tlm2pin_parm=shadow_memory=false
--set_tlm2pin_parm=dso_name=$(EXAMPLE_DIR)/dso/$(XTSC_RUN_COSIM_VENDOR)/lib_`$core0->name`_`$key`.so
--set_tlm2pin_parm=dso_cookie=$(TOP).`$key`
--set_tlm2pin_parm=byte_width=`$pif_byte_width`
--set_tlm2pin_parm=start_byte_address=` hx(8,$m->addr)`
--create_tlm2pin=t2p_`$key`
;   if ($num_memories == 1) {
;     if ($coherent) {
--connect_cohctrl_tlm2pin=cohctrl,t2p_`$key`
;     } else {
--connect_arbiter_tlm2pin=arbiter,t2p_`$key`
;     }
;   } else {
--connect_router_tlm2pin=router,`$memory_index`,t2p_`$key`
;     print(FH "      $memory_index      " . hx(8,$m->addr) . "  " . hx(8,$m->addr + $m->sizem1) . "\n");
;   }
;   $memory_index += 1;

// Configure, create, and connect to proxy `$key`
--set_proxy_parm=trace_file=&waveforms
--set_proxy_parm=module_name=`$core0->name`_`$key`
--set_proxy_parm=readmemh_file_name=$(READMEMH_FILE_NAME)
--set_proxy_parm=verilog_file=$(XTSC_SCRIPT_FILE_PATH)/`$core0->name`_`$key`.sv
--connect_tlm2pin_proxy=t2p_`$key`,`$key`
--connect_clock_proxy=CLK,`$key`
; }
; close(FH);
;#
;#
; foreach my $i (0 .. $#cores) {
;   my $core = $cores[$i];
;   my $p = $core->pconfig;

// Configure, create, and connect xtsc_core `$core->name`
--xtensa_core=`$core->name`
--set_core_parm=ProcessorID=`$i`
;   if ($p->relocatable_vectors) {
--set_core_parm=SimStaticVectorSelect=`$p->static_vector_select_sw`
;   }
--set_core_parm=SimTargetProgramDelayLoad=true
--set_core_parm=SimTargetProgram=$(`$core->name`_PROGRAM)
--core_argv=$(`$core->name`_ARGV)
--create_core=`$core->name`
;   if ($coherent) {
--connect_core_cohctrl=`$core->name`,`$i`,cohctrl
;     if ($i == 0) {
--connect_core_logic=`$core->name`,MPSCORE,MPSCORE,MPSCORE_RunStall
;     } else {
--connect_logic_core=MPSCORE_RunStall,RunStall`$i`,RunStall,`$core->name`
;     }
--connect_core_distributor=`$core->name`,`$i`,distributor
;     if ($BInterruptSize[$i] == $shared_interrupts) {
--connect_distributor_core=distributor,`$i`,`$core->name`
;     } else {
;       my $logic_name = "BInterrupt_" . $core->name;
;       open(FH, ">$logic_name.def");
;       print(FH "// Generated on $date" . "\n");
;       print(FH "input PROCINT $shared_interrupts" . "\n");
;       print(FH "output BInterrupt " . $BInterruptSize[$i] . " 0" . "\n");
;       print(FH "iterator i 0 " . ($shared_interrupts - 1) . "\n");
;       print(FH "assign BInterrupt[i] = PROCINT[i]" . "\n");
;       close(FH);

// Configure, create, and connect xtsc_wire_logic `$core->name`_BInterrupt
--set_logic_parm=definition_file=$(XTSC_SCRIPT_FILE_PATH)/`$logic_name`.def
--create_logic=`$logic_name`
--connect_distributor_logic=distributor,PROCINT`$i`,PROCINT,`$logic_name`
--connect_logic_core=`$logic_name`,BInterrupt,BInterrupt,`$core->name`
;     }
;   } else {
--connect_core_arbiter=`$core->name`,pif,`$i`,arbiter
;   }
; }
;#
;#
