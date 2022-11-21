#  cc_memories.txt - generated list of memories connected to the CC (what if there's no CC?)
#
; my $year = 1900 + (localtime)[5];
# Copyright (c) 2007-`$year` by Tensilica Inc.  ALL RIGHTS RESERVED.
# These coded instructions, statements, and computer programs are the
# copyrighted works and confidential proprietary information of Tensilica Inc.
# They may not be modified, copied, reproduced, distributed, or disclosed to
# third parties in any manner, medium, or form, in whole or in part, without
# the prior written consent of Tensilica Inc.

#  System memories:
; foreach my $mem ($layout->addressables) {
;     next if $mem->is_addrspace or $mem->is('device') or $mem->name =~ /^(.*\.)?[id]r[ao]m\d$/;
;#` sprintf("%s_addr = 0x%x", $mem->name, ... address as seen from where? not all systems have a CC ... )` 
` sprintf("%s_size = 0x%x", $mem->name, $mem->sizem1+1)` 
;#` sprintf("%s_flags = %s", $mem->name, $mem->{...})` 
; }

