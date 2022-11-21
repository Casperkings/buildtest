#  sys-defs.mk - generated Makefile information about the (sub)system
#
; my $year = 1900 + (localtime)[5];
# Copyright (c) 2007-`$year` by Tensilica Inc.  ALL RIGHTS RESERVED.
# These coded instructions, statements, and computer programs are the
# copyrighted works and confidential proprietary information of Tensilica Inc.
# They may not be modified, copied, reproduced, distributed, or disclosed to
# third parties in any manner, medium, or form, in whole or in part, without
# the prior written consent of Tensilica Inc.

#  Core instances, in PRID order (starting with 0, no gaps).
; my @cores = $layout->cores;
NCORES = ` scalar(@cores)`
CORES = ` join(' ',map($_->name, @cores))`

#  Core instances that share their reset vector with another (not in any order):
; my @shared_reset = grep($_->[0] eq 'ResetVector', @{$layout->{shared_sets}});
SHARED_RESET_CORES = ` join(' ',map($_->name, map(@{$_->[3]}, @shared_reset)))`
; my %sets = ();
; foreach my $sharedset (@{$layout->{shared_sets}}) {
;   push @{$sets{$sharedset->[0]}}, $sharedset;
; }
; foreach my $sharedvec (sort keys %sets) {
;   my $set = $sets{$sharedvec};
SHARED_CORES_`$sharedvec` = ` join(' ',map($_->name, map(@{$_->[3]}, @$set)))`
; }

#  Core configs, in alphabetical order.
; my %configs = ();
; foreach my $core (@cores) {$configs{$core->config->name} = 1;}
NCONFIGS = ` scalar(keys %configs)`
CONFIGS = ` join(' ',sort keys %configs)`

#  Core to use when working with shared elements

SHARED_CONFIG = `$cores[0]->name`

#  Configs for each core.
; foreach my $core (@cores) {
CORE_`$core->name`_CONFIG = `$core->config->name`
; }
;#
;#  core_config = $(CORE_$(1)_CONFIG)
;#  coren_config = $(CORE_$(word $(1),$(CORES))_CONFIG)
;#  $(call core_config,<name>)
;#  $(call coren_config,<number>)

#  Per-core derived/constructed/system info:
; foreach my $core (@cores) {
;   my $sw = $core->swoptions;
;#   my $sym = $core->name."._memmap_vecbase_reset";
CORE_`$core->name`_VECBASE   = ` defined($sw->{vecbase}) ? sprintf("0x%08x",$sw->{vecbase}) : ""`
;#CORE_`$core->name`_VECBASE_SYMBOL = ` defined($layout->{symbols}->{$sym}) ? sprintf("0x%08x",$layout->{symbols}->{$sym}) : ""`
CORE_`$core->name`_VECSELECT = ` defined($sw->{vecselect}) ? $sw->{vecselect} : ""`
CORE_`$core->name`_VECRESET_PINS = ` defined($sw->{vecreset_pins}) ? sprintf("0x%08x", $sw->{vecreset_pins}) : ""`
;#CORE_`$core->name`_RESET_TABLE_VADDR = ` defined($core->shared_reset_table_vaddr) ? sprintf("0x%08x", $core->shared_reset_table_vaddr) : ""`
; }

#  Set of linkmaps created
; my @linkmaps = @{$layout->linkmaps};
NLINKMAPS = ` scalar(@linkmaps)`
LINKMAPS = ` join(' ',map($layout->linkmap_name($_), @linkmaps))`

#  Per-linkmap info:
; foreach my $linkmap (@linkmaps) {
;   my $name = $layout->linkmap_name($linkmap);
;   #my $name = $linkmap->name // "oops";
LINKMAP_`$name`_FULL_IMAGE = `$linkmap->{full_image} ? 1 : 0`
LINKMAP_`$name`_CORES = ` join(" ", map($_->name, @{$linkmap->cores}))`
; }


#  end.
