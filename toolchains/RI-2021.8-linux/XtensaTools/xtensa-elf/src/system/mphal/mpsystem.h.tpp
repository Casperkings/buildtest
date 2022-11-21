/*  System wide parameters (aka "MP HAL")  */

/*  (this file was generated -- do not edit)  */

/* Copyright (c) 2005-`(localtime)[5]+1900` Tensilica Inc.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

; #use Xtensa::AddressLayout;
; my @cores = $layout->cores;
; my %configs = ();
; foreach my $core (@cores) {$configs{$core->config->name} = 1;}
; my $n_configs = 0;
; foreach my $cfg (sort keys %configs) { $configs{$cfg} = $n_configs++; }
#define XMP_NUM_CORES	` scalar @cores`
#define XMP_NUM_CONFIGS	` scalar keys %configs`
#define XMP_SYS_NAME	"`$layout->system_name`"
#define XMP_SYS_NAME_ID	`$layout->system_name`
; my $max_dcache_linesize = 4;
; foreach my $i (0 .. $#cores) {
;   my $core = $cores[$i];
;   my $pr = $cores[$i]->config->pr;
;#printf STDERR "### core %s: dcacheopt=%s dcacheV=%s dcacheS=%s dcache=%s\n", $core->name, $pr->dcacheOption(), $pr->dcacheV(), $pr->dcacheS(), $pr->dcache();
;   my $lineSize = $pr->dcacheOption() ? ($pr->dcacheV() ? $pr->dcacheV()->lineSize() : $pr->dcacheS() ? $pr->dcacheS()->lineSize() : $pr->dcache()->lineSize()) : 0;
;   if ($lineSize > $max_dcache_linesize) {
;     $max_dcache_linesize = $lineSize
;   }
; }
#define XMP_MAX_DCACHE_LINESIZE        `$max_dcache_linesize`
;#
;#
; sub show_localmem_addrs {
;   my ($i, $locname, $locmems_ref) = @_;
;   my @locmems = (defined($locmems_ref) ? @$locmems_ref : ());
;   my $n = 0;
;   my $linkmap = $layout->find_addressable($cores[$i]->name.".linkmap");
;   foreach my $locmem (@locmems) {
#define XMCORE_`$i`_`$locname.$n`_VADDR		` sprintf("0x%x", $locmem->baseVAddr)`	/* vaddr local to that core */
;     my ($m) = $linkmap->findmaps($locmem->baseVAddr);
;     if (!defined($m)) {
/* WARNING: could not find `$locname.$n` at that address in the default linkmap */
;     } else {
;	my $mem = $m->mem;
;	my @vaddrs;
;	@vaddrs = $layout->{commonmap}->addrs_of_space($mem, 0, $mem->sizem1)
;		if defined($layout->{commonmap});
;       if (@vaddrs == 0) {
/* No global virtual address found for `$locname.$n` */
;	} else {
;	  my $vaddr = shift @vaddrs;
;	  if (@vaddrs) {
/* Multiple global virtual addresses found for `$locname.$n`, picking first one
   (others are: ` join(", ", map(sprintf("0x%x",$_), @vaddrs))`). */
;	  }
#define XMCORE_`$i`_`$locname.$n`_GLOBAL_VADDR	` sprintf("0x%x", $vaddr)`
;	}
;     }
;     $n++;
;   }
; }
;#
;#
; foreach my $i (0 .. $#cores) {
;   my $core = $cores[$i];
;   my $pr = $core->config->pr;


/***  Core `$i`  ***/

/*#define XMCORE_`$i`_PRID			0x%X */
#define XMCORE_`$i`_CORE_NAME		"`$core->name`"
#define XMCORE_`$i`_CORE_NAME_ID		`$core->name`
#define XMCORE_`$i`_CONFIG_NAME		"`$core->config->name`"
#define XMCORE_`$i`_CONFIG_NAME_ID		`$core->config->name`
#define XMCORE_`$i`_CONFIG_INDEX		`$configs{$core->config->name}`

/*
 *  These config-specific parameters are already in the compile-time HAL of
 *  each config.  Here we provide them for all configs of a system.
 */
#define XMCORE_`$i`_HAVE_BIG_ENDIAN	`$pr->core->memoryOrder eq "BigEndian" ? 1 : 0`
#define XMCORE_`$i`_MAX_INSTRUCTION_SIZE	`$pr->core->maxInstructionSize`
#define XMCORE_`$i`_HAVE_DEBUG		`$pr->debugOption`
#define XMCORE_`$i`_DEBUG_LEVEL		`$pr->debug->interruptLevel`
/*#define XMCORE_`$i`_HAVE_XEA2		`$pr->core->newExceptionArch`*/
#define XMCORE_`$i`_HAVE_XEAX		`$pr->core->externalExceptionArch`
; my $selected_vecbase = 0;
; if ($pr->vectors->relocatableVectorOption) {
;   $selected_vecbase = $pr->vectors->SW_stationaryVectorBaseSelect
;			? $pr->vectors->stationaryVectorBase0
;			: $pr->vectors->stationaryVectorBase1;
; }
#define XMCORE_`$i`_RESET_VECTOR_VADDR	` sprintf("0x%X",($pr->core->exceptionArch eq 'XEA3' ? $selected_vecbase : $pr->vectors->vectorList(0)->baseAddress))`

;   show_localmem_addrs($i, "DATARAM", $pr->ref_dataRams);
;   show_localmem_addrs($i, "DATAROM", $pr->ref_dataRoms);
;   show_localmem_addrs($i, "XLMI",    $pr->ref_dataPorts);
;   show_localmem_addrs($i, "URAM",    $pr->ref_unifiedRams);
;   show_localmem_addrs($i, "INSTRAM", $pr->ref_instRams);
;   show_localmem_addrs($i, "INSTROM", $pr->ref_instRoms);
;   #
; }

/*  Macro for creating processor-indexed arrays of parameters:  */
; print "#define XMCORE_ARRAY_P(param)";
; foreach my $i (0 .. $#cores) {
;   printf "%s\t\\\n\t\tXMCORE_%d_ ## param", ($i ? "," : ""), $i;
; }


;sub printWarning {
; my ($msg) = @_;

; $msg =~ s/\n/\n         /g;	# indent subsequent lines
;  print STDERR "WARNING: $msg\n";
;}


; my $L2cc = $cores[0]->config->pr->l2cc;
; if (defined($L2cc)) {
;    #use Data::Dumper;
;    my $l2ramstart = 0;
;    my $l2ramsize = 0;
;    foreach my $ad ($layout->addressables_deporder) {
;       #printf STDERR "XXX %s\n", $ad->name;
;       if ($ad->name eq "cc") { 
;          foreach my $ae ($ad->map->a) { 
;             #printf STDERR "YYY %s\n", $ae->name;
;             if ($ae->can("name") && ($ae->name eq "l2ram")) {
;                #printf STDERR "ZZZ %d %d\n", $ae->addr, $ae->sizem1;
;                $l2ramstart = $ae->addr;
;                $l2ramsize = $ae->sizem1 + 1;
;                }
;             }
;        }
;   } 
;   my $l2_ramfrac;
;   my $l2_cachefrac;

;   my $L2TotalSize = defined ($L2cc) ? $L2cc->cache->size : 0;
;   my $l2_ram_only = defined($L2cc->l2RAMonly) && $L2cc->l2RAMonly;
;   if ($L2TotalSize < $l2ramsize)
;	{
;		die "Error: $l2ramsize bytes of L2Ram requested in system description, but only $L2TotalSize is configured.\n";
;	}
;   if ($l2_ram_only) {
;       if ($L2TotalSize > $l2ramsize) {
;	printWarning("L2 is configured as ram only, and only $l2ramsize bytes were requested in the system description, but $L2TotalSize is available ( %d bytes wasted).\n",
;	$L2TotalSize - $l2ramsize);
;    }
;	$l2_ramfrac = 16;
;	$l2_cachefrac = 0;
;   }
; else
;{
;	if ($l2ramsize == 0) 
;	{
;	   printf STDERR "no L2 ram requested, configuring as all L2 cache\n";
;	   $l2_ramfrac = 0;
;	   $l2_cachefrac = 16;
;	}
;	elsif ($l2ramsize < ($L2TotalSize/8)) {
;	   printWarning("Requested L2RAM: $l2ramsize isn't available, choosing closest larger value.\n");
;	   $l2_ramfrac = 2;
;	   $l2_cachefrac = 14;
;	} elsif ($l2ramsize == ($L2TotalSize / 8)) {
;	   $l2_ramfrac = 2;
;	   $l2_cachefrac = 14;
;	} elsif ($l2ramsize < ($L2TotalSize/4)) {
;	   printWarning("Requested L2RAM: $l2ramsize isn't available, choosing closest larger value.\n");
;	   $l2_ramfrac = 4;
;	   $l2_cachefrac = 12;
;	} elsif ($l2ramsize == ($L2TotalSize/4)) {
;	   $l2_ramfrac = 4;
;	   $l2_cachefrac = 12;
;	}
;	elsif ($l2ramsize < ($L2TotalSize/2)) {
;	   printWarning("Requested L2RAM: $l2ramsize isn't available, choosing closest larger value.\n");
;		$l2_ramfrac = 8;
;		$l2_cachefrac = 8;
;	} elsif ($l2ramsize == ($L2TotalSize/2)) {
;	$l2_ramfrac = 8;
;		$l2_cachefrac = 8;
;	} elsif ($l2ramsize < ($L2TotalSize)) {
;			printWarning("Requested L2RAM: $l2ramsize isn't available, choosing closest larger value.\n");
;			$l2_ramfrac = 16;
;			$l2_cachefrac = 0;
;			} elsif ($l2ramsize == ($L2TotalSize)) {
;			$l2_ramfrac = 16;
;			$l2_cachefrac = 0;
;			}
;	}
/*
	L2 Total Size: `$L2TotalSize`
	L2 Ram Requested: `$l2ramsize`
	L2 Ram Base Address: `$l2ramstart`
*/

#define XMCHAL_L2RAM_START `$l2ramstart` 
#define XMCHAL_L2RAM_FRAC  `$l2_ramfrac`
#define XMCHAL_L2CACHE_FRAC `$l2_cachefrac` 
;}
