;#
;# This file is copied to swtools-{machine}-{os}/src/system/xtsc-run/memory.lister.tpp
;# by running "make install-src" in builds-{machine}-{os}/_swtools_/xbuild/Software
;#
;# This script causes a *.sv file to be placed in the build/system/xtsc-run directory for
;# each memory in the cluster.  This *.sv files contains perl code to define the memory
;# parameters (config, name, start address, size, endian, bit width).  These *.sv files are
;# processed by the Makefile one-by-one and their contents are passed to tpp via the -e
;# option when tpp is envoked on CONFIG_MEMORY.sv.tpp to create individual SystemVerilog
;# memory models (*.sv files) in the package/xtsc-run directory.
;#
;# For example, for a cluster with config core0 and memories sysram and sysrom, files
;# core0_sysram.sv and core0_sysrom.sv are created in the build/system/xtsc-run directory.
;# These two files contain perl variables specifying the memory parameters which are
;# passed to tpp with the -e option to generate SystemVerilog memory models also named
;# core0_sysram.sv and core0_sysrom.sv that are placed in package/xtsc-run.
;#
;sub hx {
;  my ($nibbles, $value) = @_;
;  sprintf("0x%0". $nibbles. "X", $value);
;}
;#
;#
; stash_obj_isa($sys, "MultiCoreSystem") or die "system must be of type MultiCoreSystem";
; my $system = $sys->name;
; my $core = $sys->cores->[0];
; my $config = $core->name;
; my $little_endian = (($core->sys->pr->xt->core->memoryOrder eq "BigEndian") ? 0 : 1);
; my $pif_bit_width = $core->sys->pr->xt->core->pifReadDataBits;
; my %memories;
; foreach my $m ($sys->cc->map->a) {
;   my $name = $m->mem->name;
;   next if $name eq "inbound";	# FIXME: GROSS HACK: skip connections from CC to inbound PIFs for now
;   die "memory $name mapped multiple times on PIF or CC"
;	if exists($memories{$name});
;   $memories{$name} = 1;
;   #  Ignore $m->offset for now...
;   #  Ignore $m->sizem1 for now...
;   open(FH, ">" . $config . "_" . $name . ".sv");
;   print(FH "\$config_name='" . $config . "'; ");
;   print(FH "\$memory_name='" . $name . "'; ");
;   print(FH "\$memory_start=" . hx(8,$m->addr) . "; ");
;   print(FH "\$memory_byte_size=" . hx(8,$m->sizem1 + 1) . "; ");
;   print(FH "\$little_endian=" . $little_endian . "; ");
;   print(FH "\$bit_width=" . $pif_bit_width . "; ");
;   close(FH);
; }
;#
;#
;#
