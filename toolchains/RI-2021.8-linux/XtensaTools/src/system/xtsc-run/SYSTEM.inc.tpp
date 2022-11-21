// Copyright (c) 2007-2018 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of
// Tensilica Inc.  They may be adapted and modified by bona fide
// purchasers for internal use, but neither the original nor any adapted
// or modified version may be disclosed or distributed to third parties
// in any manner, medium, or form, in whole or in part, without the prior
// written consent of Tensilica Inc.

;#
;# This file is copied to swtools-{machine}-{os}/src/system/xtsc-run/SYSTEM.inc.tpp
;# by running "make install-src" in builds-{machine}-{os}/_swtools_/xbuild/Software
;#
;sub hx {
;  my ($nibbles, $value) = @_;
;  sprintf("0x%0". $nibbles. "X", $value);
;}
;sub xtsc_name {
;  my ($name) = @_;
;  $name =~ s/\./__/g;
;  return $name;
;}
;sub layout_name {
;  my ($name) = @_;
;  $name =~ s/__/\./g;
;  return $name;
;}
;#
;#
; #use Xtensa::AddressLayout;
; my $debug = 0;
; my @cores = $layout->cores;
; my $num_cores = @cores;
; my $coherent = 1;
; my $has_l2cc = 0;
; my $byte_width = 4;
; my $num_transfers = 4;
; my @BInterruptSize;
; my $shared_interrupts = 32;
;#
; my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
; my $date = sprintf("%04u-%02u-%02u %02u:%02u:%02u", 1900+$year, $mon+1, $mday, $hour, $min, $sec);
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
;   my $cname = xtsc_name($core->name);
#define `$cname`_PROGRAM=`$cname`.out
#define `$cname`_ARGV=`$i`
; }
#include "SYSTEM.inc"
*/

//  If you need to make other changes, for example to enable core debugging or change
//  various module parameters, then Tensilica recommends that you copy this file and 
//  all *.def files to a new location and modify the copy.


;#
;#
; foreach my $i (0 .. $#cores) {
;   my $core = $cores[$i];
;   my $cname = xtsc_name($core->name);
;   my $pr = $core->config->pr;
;   $byte_width = $pr->core->pifReadDataBytes;
;   #$byte_width = $pr->core->dataUsePIF ? $pr->core->pifReadDataBytes : $pr->...;
;   if (!defined($pr->dcache) || !$pr->dcache->coherence) {
;     $coherent = 0;
;     #$has_l2cc = defined($pr->l2cc) ? 1 : 0;
;   } else {
;     my $line_size = $pr->dcache->lineSize;
;     $num_transfers = ($byte_width && $line_size ? ($line_size / $byte_width) : $num_transfers);
;     $BInterruptSize[$i] = 0;
;     my @interrupts = $pr->interrupts->interrupt;
;     foreach my $j (0 .. $#interrupts) {
;       my $type = $interrupts[$j]->type;
;       if (($type eq "ExtLevel") || ($type eq "ExtEdge") || ($type eq "NMI")) {
;         $BInterruptSize[$i] += 1;
;       }
;     }
;     if ($BInterruptSize[$i] < $shared_interrupts) { $shared_interrupts = $BInterruptSize[$i]; }
;   }

#ifndef `$cname`_PROGRAM
#define `$cname`_PROGRAM=`$cname`.out
#endif
#ifndef `$cname`_ARGV
#define `$cname`_ARGV=
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
; } elsif ($has_l2cc) {
// Configure and create xtsc_l2cc l2cc
//--set_xtsc_l2cc_parm=num_cores=`$num_cores`
--create_xtsc_l2cc=l2cc

// Configure and create xtsc_memory l2pseudoregs to act as the L2's registers
--set_memory_parm=num_ports=1
--set_memory_parm=byte_width=`$byte_width`
;  my $l2regaddr = $pr->l2cc->controlRegAddr;
// at ` hx(8,$l2regaddr)`
--set_memory_parm=start_byte_address=0
--set_memory_parm=memory_byte_size=0x1000
--set_memory_parm=read_only=false
--create_memory=l2pseudoregs

;  my $routing_filel2 = "// Generated on $date\n"
;		    . "//<PortNum>  <LowAddr>   <HighAddr>  [<NewBaseAddr>]\n";
;  $routing_filel2 .= sprintf("     %2u      0x%08x  0x%08x  0x%08x\n", 0, $l2regaddr, $l2regaddr+0xfff, 0);
;  open(FH, ">routing-l2pseudoregs.def");
;  print(FH $routing_filel2);
;  close(FH);
// Configure and create xtsc_router named "router_l2pseudoregs" to route among l2pseudoregs and l2cc
//--set_router_parm=num_slaves=2
//--set_router_parm=routing_table=$(XTSC_SCRIPT_FILE_PATH)/routing-l2pseudoregs.def
//--set_router_parm=default_port_num=1
//--create_router=router_l2pseudoregs
; }
;#
;#
; my @memories = grep(!$_->is_addrspace && !$_->is('device'), $layout->addressables);
; foreach my $mem (@memories) {
;     my $name = xtsc_name($mem->name);
;     next if $name =~ /^(.*__)?([id]r[ao]m\d|null_space)$/;	# skip local memories (HACK - how do we connect these in XTSC?) and no-PIF null space

// Configure and create xtsc_memory `$name`
--set_memory_parm=num_ports=1
--set_memory_parm=byte_width=`$byte_width`
--set_memory_parm=start_byte_address=0
--set_memory_parm=memory_byte_size=` hx(8,$mem->sizem1+1)`
--set_memory_parm=read_only=`($mem->is('writable',1) ? "false" : "true")`
--create_memory=`$name`
; }
;
; #
; #  Go through connecting (master) components and list what's connected to what, in both directions.
; #
; #my @all_connections;		# list of $connection = [$src, $dst], each $src and $dst is { name=>X, port=>X, component=>X, ... }
; my %dst_connections;		# {$memname} = [connections...]		# incoming ports for each addressable's arbiter
; my %src_connections;		# {$spacename} = [connections...]	# outgoing ports for each space's router
;
; #  Sanity-check routine
; sub check_consistency {
;   foreach my $dstname (keys %dst_connections) {
;	foreach my $dstconn (@{$dst_connections{$dstname}}) {
;	    if (!grep($_ eq $dstconn, @{$src_connections{$dstconn->{src}->{name}}})) {
;		my ($src, $dst) = ($dstconn->{src}, $dstconn->{dst});
;		my $msg = "missing src connection for dst $dstname:  ".$src->{name}." ".$src->{xtsc_type}." (".($src->{port} // "single port").") to ".$dst->{name}." ".$dst->{xtsc_type}." (".($dst->{port} // "single port").")";
// ***ERROR*** `$msg`
;		#die $msg;
;	    }
;	}
;   }
;   foreach my $srcname (keys %src_connections) {
;	foreach my $srcconn (@{$src_connections{$srcname}}) {
;	    if (!grep($_ eq $srcconn, @{$dst_connections{$srcconn->{dst}->{name}}})) {
;		my ($src, $dst) = ($srcconn->{src}, $srcconn->{dst});
;		my $msg = "missing dst connection for src $srcname:  ".$src->{name}." ".$src->{xtsc_type}." (".($src->{port} // "single port").") to ".$dst->{name}." ".$dst->{xtsc_type}." (".($dst->{port} // "single port").")";
// ***ERROR*** `$msg`
;		#die $msg;
;	    }
;	}
;   }
; }
;
; #  First, go through all address spaces.
; #  Also identify single-destination address spaces that can be dropped, allowing what connects to
; #  them to connect directly to the destination.
; #
; my @spaces = grep($_->is_addrspace, $layout->addressables);
; foreach my $space (@spaces) {
;     my $spacename = xtsc_name($space->name);
;     next if $spacename =~ /^(.*__)?(linkmap|virtual|physical|inbound|null_space)$/;	# skip core-internal and software address spaces
;     next if $spacename =~ /^(.*__)?[id]r[ao]m\d_iface$/;	# skip local memories interfaces (HACK - how do we connect these in XTSC?)
;     #  Build up routing file out of space's mappings,
;     #  and build list of ports for each thing it connects to (assumed to be an arbiter).
;     my $src_name = "router_$spacename";
;     my $connections = $src_connections{$src_name} = [];	# array of connections, indexed by space port number
;     my $non_identity = 0;
;     foreach my $map ($space->map->a) {
;	  next if $map->mem->is('device');	# skip devices
;	  $non_identity++ if $map->addr != $map->offset or $map->sizem1 != $map->mem->sizem1;
;	  my $dst_type = ($map->mem->is_addrspace ? "router" : "memory");
;	  my $dst_name = ($map->mem->is_addrspace ? "router_" : "") . xtsc_name($map->mem->name);
;	  if (!grep($_->{dst}->{name} eq $dst_name, @$connections)) {
;	      my $mem_connections = ($dst_connections{$dst_name} //= []);
;	      my $connection = {src => {name => $src_name, xtsc_type => "router", # layout_name => $space->name,
;					component => $space},
;				dst => {name => $dst_name, xtsc_type => $dst_type, $dst_type eq "router" ? (port => undef):(),
;					component => $map->mem}};
;	      push @$connections, $connection;
;	      push @$mem_connections, $connection;
;	      #push @all_connections, $connection;
;#// Added `$src_name` to `$dst_name`
;#// -- src{`$src_name`} has ` scalar(@{$src_connections{$src_name}})` connections
;#// -- dst(`$dst_name`) has ` scalar(@{$dst_connections{$dst_name}})` connections
;		check_consistency();
;	  }
;     }
;     if (@$connections == 0) {
;	  #  This space can be omitted, goes nowhere.
;	  print STDERR "WARNING: address space $spacename is empty.\n";
;     } elsif (@$connections == 1 and $non_identity == 0) {
;	  #  This space can be omitted, goes straight through to addressable entity $memname
;	  #  with no address translation/filtering:
;#// Will bypass `$spacename` ...
;	  $connections->[0]->{src}->{bypass} = 1;
;     }
; }
;
; #  Second, go through all cores.
; #
; foreach my $i (0 .. $#cores) {
;   my $core = $cores[$i];
;   my $cname = xtsc_name($core->name);
;   my $pr = $core->config->pr;
;   if (!$coherent and $pr->core->dataUsePIF) {
;     my @ports = ($pr->uarchName eq 'Cairo' ? ("instmaster_rd", "datamaster_rd", "datamaster_wr") : ("pif"));
;     my $src_conns = ($src_connections{$cname} //= []);
;     foreach my $port (@ports) {
;	my $dstname = "router_${cname}__external";
;	my $dst_conns = ($dst_connections{$dstname} //= []);
;	my $connection = {src => {name => $cname, xtsc_type => "core",
;				  port => $port, component => $core},
;			  dst => {name => $dstname, xtsc_type => "router", port => undef,
;				  component => $layout->find_addressable($cname.".external")}};
;	push @$src_conns, $connection;
;	push @$dst_conns, $connection;
;#// Added `$cname` to `$dstname`
;	#push @all_connections, $connection;
;     }
;   }
;   check_consistency();
; }
;
;
; #  All connections are listed.
; #  Now optimize network a bit.
; #  Now that we know what connects to droppable single-entry address spaces,
; #  take them out of the connections lists.
; #
; my $rescan;
; do {
;   $rescan = 0;
;   foreach my $space (@spaces) {
;     my $src_name = "router_" . xtsc_name($space->name);
;     my $out_conns = $src_connections{$src_name} // [];
;     next unless @$out_conns;		# skip empty spaces
;     my $in_conns = $dst_connections{$src_name} // [];
;     if (@$in_conns == 0) {
;	  #  Nothing connects to this space, drop it completely.
;	  foreach $out_conn (@$out_conns) {
;	      my $dst_conn = $dst_connections{$out_conn->{dst}->{name}};
;	      @$dst_conn = grep($_ ne $out_conn, @$dst_conn);	# remove from destination
;	  }
;	  @$out_conns = ();		# drop this space
// Dropped `$src_name` (nothing goes to it) ...
;	  $rescan = 1;
;     } elsif (@$out_conns == 1 and $out_conns->[0]->{src}->{bypass}) {
;	  #  We can drop this space and redirect its input straight to its output.  Find what connects to it.
;	  my $out_conn = $out_conns->[0];
// Bypassed `$src_name` to `$out_conn->{dst}->{name}` ...
;	  foreach my $in_conn (@$in_conns) {
;		#  Reroute past $space :
;		$in_conn->{dst} = $out_conn->{dst};
//  ... from `$in_conn->{src}->{name}`
;	  }
;	  my $dst_conns = $dst_connections{$out_conn->{dst}->{name}};
;	  @$dst_conns = (grep($_ ne $out_conn, @$dst_conns), @$in_conns);
;	  #$dst_connections{$src_name} = [];
;	  @$out_conns = ();	# drop this space
;	  @$in_conns = ();	# drop this space
;     }
;     check_consistency();
;     next;
;
; ###   TODO FIXME: the following needs to be done in a more component independent way
;     next unless $src_name =~ /^router_(.*)__external$/ and !$coherent;
;     my $corename = $1;
;     #my $core = $layout->cores_byname->{$corename};
;     my ($core) = grep($_->name eq $corename, @cores);
;     defined($core) or die "could not find core $corename for space $src_name";
;     my $uarch = $core->config->pr->uarchName;
;#// core `$corename` uarch is `$uarch`
;     if ($uarch eq 'Cairo') {	  # needs 3 ports
;	  my $d_connections = $dst_connections{$memname};
;	  #  Add two more ports/connections:
;	  push @$d_connections, [ $out_conn->{src}, { %{$out_conn->{dst}}, port => scalar(@$d_connections) } ];
;	  push @$d_connections, [ $out_conn->{src}, { %{$out_conn->{dst}}, port => scalar(@$d_connections) } ];
;     }
;   }
; } while($rescan);
;

//
//  Create arbiters for components with more than one inbound connection.
//
; sub dump_conns {
;   my ($conns) = @_;
;   foreach my $conn (@$conns) {
;      my ($src, $dst) = ($conn->{src}, $conn->{dst});
//    `$src->{name}` `$src->{xtsc_type}` (`$src->{port} // (exists($src->{port}) ? "single port" : "unassigned")`) to `$dst->{name}` `$dst->{xtsc_type}` (`$dst->{port} // (exists($dst->{port}) ? "single port" : "unassigned")`)
;   }
; }
; sub dump_src {
// src connections are:
;   foreach my $srcname (sort keys %src_connections) {
//   `$srcname`:
;	dump_conns($src_connections{$srcname});
;   }
; }
; sub dump_dst {
// dst connections are:
;   foreach my $dstname (sort keys %dst_connections) {
//   `$dstname`:
;	dump_conns($dst_connections{$dstname});
;   }
; }
; sub debugdump {
;   dump_src() if $debug;
;   dump_dst() if $debug;
; }
; foreach my $name (sort keys %dst_connections) {
;#// dest `$name` ...
;     my $dst_conns = $dst_connections{$name};
;     if ($coherent and $name eq "router_cc") {
;	  #connect_to_addressable("cohctrl", "cohctrl", "cc");
;	  #next;
;     }
;     next if @$dst_conns < 2;
;     my $arbiter_name = $name; $arbiter_name =~ s/^router_//; $arbiter_name = "arbiter_$arbiter_name";
;     my %arbiter_comp = (name => $arbiter_name, xtsc_type => "arbiter");
;     #  No port number out of the arbiter, so port => undef.
;     my $conn_arbiter_to_dst = { src => {%arbiter_comp, port => undef}, dst => { %{$dst_conns->[0]->{dst}} } };
;     foreach my $dst_conn (@$dst_conns) {
;	  $dst_conn->{dst} = { %arbiter_comp };	# reroute to arbiter
;     }
;     $dst_connections{$arbiter_name} = [ @$dst_conns ];
;     $src_connections{$arbiter_name} = [ $conn_arbiter_to_dst ];

// Configure and create xtsc_arbiter `$arbiter_name`
--set_arbiter_parm=num_masters=` scalar(@$dst_conns)`
--create_arbiter=`$arbiter_name`
;     @$dst_conns = ($conn_arbiter_to_dst);
;     check_consistency();
;#;     if ($has_l2cc and $name eq "cc") {
;#--connect=arbiter_cc,,core0data_wr,l2cc
;#--set_arbiter_parm=num_masters=2
;#--create_arbiter=arbiter_ccout
;#--connect=l2cc,master0_rd,slave_port[0],arbiter_ccout
;#--connect=l2cc,master0_wr,slave_port[1],arbiter_ccout
;#;	  connect_to_addressable("arbiter", "arbiter_ccout", "cc");
;#;	  next;
;#;     }
;     debugdump();
; }
;
; #  Okay, now that connectivity has stabilized, assign port numbers where unassigned.
; #
; foreach my $src_name (keys %src_connections) {
;     my $port = 0;
;     foreach my $src_conn (@{$src_connections{$src_name}}) {
;	  $src_conn->{src}->{port} = $port++ unless exists($src_conn->{src}->{port});
;     }
; }
; foreach my $dst_name (keys %dst_connections) {
;     my $port = 0;
;     foreach my $dst_conn (@{$dst_connections{$dst_name}}) {
;	  $dst_conn->{dst}->{port} = $port++ unless exists($dst_conn->{dst}->{port});
;     }
; }
; debugdump();

//
//  Create routers for address spaces in use visible to XTSC.
//

; #   For each address space, create a router that corresponds to all mappings in that address space.
; #   In effect, the router acts as that address space.  Except that ...
; #   If an address space maps to only one mapping, AND there's no address translation, skip the router and connect directly.
; #   If an address space maps to nowhere, drop it.
; #
; foreach my $space (@spaces) {
;     my $src_name = "router_" . xtsc_name($space->name);
;     my $connections = $src_connections{$src_name} // [];
;     next unless @$connections;
;     my $routing_file = "// Generated on $date\n"
;		       . "//<PortNum>  <LowAddr>   <HighAddr>  [<NewBaseAddr>]\n";
;     foreach my $map ($space->map->a) {
;	  #next if $map->mem->is('device');	# skip devices
;	  my $memname = xtsc_name($map->mem->name);
;	  my @conns_to_mem = grep($_->{dst}->{name} eq $memname, @$connections);
;	  next unless @conns_to_mem;
;	  my $space_port = $conns_to_mem[0]->{src}->{port};
;	  $routing_file .= sprintf("     %2u      0x%08x  0x%08x  0x%08x\n", $space_port, $map->addr, $map->endaddr, $map->offset);
;     }
;     my $filename = "routing-".$space->name.".def";
;     open(FH, ">$filename");
;     print(FH $routing_file);
;     close(FH);

// Configure and create xtsc_router named "`$src_name`" for `$space->name` space
--set_router_parm=num_slaves=` scalar(@$connections)`
--set_router_parm=routing_table=$(XTSC_SCRIPT_FILE_PATH)/`$filename`
--set_router_parm=default_port_num=0xdeadbeef
--create_router=`$src_name`
; }
;
;
; #
; #  Create cores
; #
;
; foreach my $i (0 .. $#cores) {
;   my $core = $cores[$i];
;   my $cname = xtsc_name($core->name);
;   my $pr = $core->config->pr;

// Configure, create, and connect xtsc_core `$cname`
--xtensa_core=`$cname`
;   if ($pr->usePRID) {
--set_core_parm=ProcessorID=`$i`
;   }
;   if ($pr->vectors->relocatableVectorOption) {
--set_core_parm=SimStaticVectorSelect=`$pr->vectors->SW_stationaryVectorBaseSelect`
;   }
--core_program=$(`$cname`_PROGRAM)
--core_argv=$(`$cname`_ARGV)
--create_core=`$cname`
;   if ($coherent) {
--connect_core_cohctrl=`$cname`,`$i`,cohctrl
;     if ($i == 0) {
--connect_core_logic=`$cname`,MPSCORE,MPSCORE,MPSCORE_RunStall
;     } else {
--connect_logic_core=MPSCORE_RunStall,RunStall`$i`,RunStall,`$cname`
;     }
--connect_core_distributor=`$cname`,`$i`,distributor
;     if ($BInterruptSize[$i] == $shared_interrupts) {
--connect_distributor_core=distributor,`$i`,`$cname`
;     } else {
;       my $logic_name = "BInterrupt_" . $cname;
;       open(FH, ">$logic_name.def");
;       print(FH "// Generated on $date" . "\n");
;       print(FH "input PROCINT $shared_interrupts" . "\n");
;       print(FH "output BInterrupt " . $BInterruptSize[$i] . " 0" . "\n");
;       print(FH "iterator i 0 " . ($shared_interrupts - 1) . "\n");
;       print(FH "assign BInterrupt[i] = PROCINT[i]" . "\n");
;       close(FH);

// Configure, create, and connect xtsc_wire_logic `$cname`_BInterrupt
--set_logic_parm=definition_file=$(XTSC_SCRIPT_FILE_PATH)/`$logic_name`.def
--create_logic=`$logic_name`
--connect_distributor_logic=distributor,PROCINT`$i`,PROCINT,`$logic_name`
--connect_logic_core=`$logic_name`,BInterrupt,BInterrupt,`$cname`
;     }
;   } # else if core has external bus, connections were setup already above

; }
;#

//
//  Connect sources to their destinations...
//

; my @connect_lines;
; foreach my $srcname (sort keys %src_connections) {
;     my $connections = $src_connections{$srcname};
;     foreach my $connection (@$connections) {
;	  my $src = $connection->{src};
;	  my $dst = $connection->{dst};
;#// Connect `$src->{name}` (`$src->{port} // "(single port)"`) to `$dst->{name}` (`$dst->{port} // "(single port)"`)
;	  push @connect_lines, "--connect_".$src->{xtsc_type}."_".$dst->{xtsc_type}."=".$src->{name}.(defined($src->{port}) ? ",".$src->{port} : "").",".(defined($dst->{port}) ? $dst->{port}."," : "").$dst->{name};
;     }
; }
; foreach (sort @connect_lines) {
`$_`
; }

