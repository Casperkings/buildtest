# Copyright (c) 1999-2018 by Tensilica Inc.  ALL RIGHTS RESERVED.
use Object;
package main;
sub dprintf { wdprintf(0, @_); }
sub wprintf { wdprintf(1, @_); }
sub Error         { Xtensa::AddressLayout::Error(@_); }
sub InternalError { Xtensa::AddressLayout::InternalError(@_); }
sub wdprintf {
    my ($iswarn, $fh, $format, @args) = @_;
    printf STDERR $format, @args if $Xtensa::AddressLayout::global_debug or $iswarn;
    printf $fh $format, @args if $fh;
}
sub mymin {my $min = shift; foreach (@_) {$min = $_ if $_ < $min;} $min;}
sub mymax {my $max = shift; foreach (@_) {$max = $_ if $_ > $max;} $max;}
package Xtensa::AddressLayout;
our $global_debug;
BEGIN {
    $global_debug = 0;
    $global_debug = 1 if ($ENV{"XTENSA_INTERNAL_DEBUG"} or "") =~ /^DEBUG/;
    if ($global_debug) {
	my $diesub = sub {
	    print STDERR "SelfLoader::AUTOLOAD is $SelfLoader::AUTOLOAD\n" if (caller)[0] =~ /SelfLoader/;
	    local $@; require Carp;
	    if ($previous_sigdie) {
	      Carp::cluck(@_);
	      $SIG{__DIE__} = $previous_sigdie;
	      goto &$previous_sigdie;
	    } else {
	      Carp::confess(@_);
	    }
	  } if $global_debug;
	our $previous_sigdie = $SIG{__DIE__};
	$previous_sigdie = undef if defined($previous_sigdie) and $previous_sigdie eq $diesub;
	$SIG{__DIE__} = $diesub;
    }
}
our $VERSION = "1.0";
use Object;
Class qw(HashObject);
use strict;
use IO::Handle;
use Cwd;
use Xtensa::YAML;
use Component::XtensaCore;
sub new {
    my $class = shift;
    my $layout = {
	children => {},		
	symbols => {},		
	xtensa_tools_root => undef,	
	xtensa_system => undef,	
	cores_byname => {},	
	core_names => [],	
	core_set_names	=> {},	
	requested_linkmaps => [],	
	linkmaps => [],		
	core_configs => {},	
	linkmaps_byname => {},	
	modified => 0,		
	mems	=> {},		
	linkmap_constructs => new ArrayObject,	
	ordered_constructs => new ArrayObject,	
	verbose => 0,		
	logfile => undef,	
    };
    bless ($layout, $class);	
    return $layout;		
}
sub cleanup {
    my ($layout, $keep_configs) = @_;
    my $obj = { logfile => $layout->logfile };
    if (!$keep_configs) {
	$obj->{core_configs} = $layout->core_configs;
	foreach my $core (values %{$layout->cores_byname}) {
	    $core->cleanup;
	}
	$layout->set_core_configs({});
    }
    $layout->set_logfile(undef);
    return $obj;
}
sub reload {
    my ($layout, $restore_obj) = @_;
    if (defined($restore_obj)) {
	foreach my $configname (keys %{$restore_obj->{core_configs}}) {
	    $layout->core_configs->{$configname} = $restore_obj->{core_configs}->{$configname}
		if !defined($layout->core_configs->{$configname});
	}
	$layout->set_logfile($restore_obj->{logfile})
	    if !defined($layout->logfile);
    }
}
sub linkmap { InternalError("linkmap singular no more"); }
sub dprintf {
    my ($layout, $format, @args) = @_;
    $layout->display(0, $format, @args);
}
sub warn {
    my ($layout, $format, @args) = @_;
    $layout->display(1, "WARNING: $format\n", @args);
}
sub info {
    my ($layout, $format, @args) = @_;
    $layout->display(2, "INFO: $format\n", @args);
}
sub display {
    my ($layout, $iswarn, $format, @args) = @_;
    printf STDERR $format, @args if $Xtensa::AddressLayout::global_debug
	or $iswarn == 1 or ($iswarn == 2 and $layout->verbose);
    $layout->logfile->printf($format, @args) if defined($layout->logfile);
}
sub add_linkmap {
    my ($layout, $name, $linkmap) = @_;
    $layout->dprintf("add_linkmap(layout=$layout, name=$name, linkmap=$linkmap)\n");
    if (defined($linkmap->layout) and $linkmap->layout ne $layout) {
	Error("linkmap '$name' is already in another layout: ".$linkmap->layout);
    }
    $linkmap->set_layout($layout);
    my $linkmaps_byname = $layout->{linkmaps_byname};
    if (exists($linkmaps_byname->{$name})) {
	Error("linkmap named '$name' already present in layout");
    }
    $linkmaps_byname->{$name} = $linkmap;
    push @{$layout->{linkmaps}}, $linkmap;
}
sub clear_linkmaps {
    my ($layout) = @_;
    $layout->{linkmaps_byname} = {};
    $layout->{linkmaps} = [];
    foreach ($layout->cores) { $_->set_linkmap(undef); }
}
sub set_split {
    my ($a, $b) = @_;
    my %bhash = map {$_ => 1} @$b;
    return (
	[grep(!exists($bhash{$_}), @$a)],
	[grep( exists($bhash{$_}), @$a)]
	);
}
sub set_split3 {
    my ($a, $b) = @_;
    my %ahash = map {$_ => 1} @$a;
    my %bhash = map {$_ => 1} @$b;
    return (
	[grep(!exists($bhash{$_}), @$a)],
	[grep( exists($bhash{$_}), @$a)],
	[grep(!exists($ahash{$_}), @$b)]
	);
}
sub normalize_set_of_linkmaps {
    my ($layout, $linkmap_names) = @_;
    my ($ignored, $ordered, $unrecognized) = set_split3([map($layout->linkmap_name($_),@{$layout->linkmaps})], $linkmap_names);
    @$unrecognized and Error("unrecognized linkmap name(s) "
				.join(", ", map("'$_'", @$unrecognized)));
    return $ordered;
}
sub name_of_set_of_linkmaps {
    my ($layout, $names) = @_;
    $names = $layout->normalize_set_of_linkmaps($names);
    return "ALL" if @$names == @{$layout->linkmaps};
    return join("__", @$names);
}
sub names_of_linkmaps {
    my ($layout, @setnames) = @_;
    my %names;
    my @unrecognized;
    foreach my $setname (@setnames) {
	if ($setname eq "ALL") {
	    foreach (@{$layout->linkmaps}) { $names{$layout->linkmap_name($_)} = 1; }
	    next;
	}
	my @parts = split("__", $setname, -1);
	@parts = ("") if $setname eq "";	
	foreach my $name (@parts) {
	    if (defined($layout->{linkmaps_byname}->{$name})) {
		$names{$name} = 1;
	    } else {
		push @unrecognized, $name;
	    }
	}
    }
    return (normalize_set_of_linkmaps($layout, [keys %names]), \@unrecognized);
}
sub normalize_set_of_cores {
    my ($layout, $core_names) = @_;
    my ($ignored, $ordered, $unrecognized) = set_split3($layout->ref_core_names, $core_names);
    @$unrecognized and Error("unrecognized core or bus master name(s) "
				.join(", ", map("'$_'", @$unrecognized)));
    return $ordered;
}
sub name_of_set_of_cores {
    my ($layout, $names) = @_;
    $names = $layout->normalize_set_of_cores($names);
    return "ALL" if @$names == @{$layout->ref_core_names};
    my $master_sets = $layout->ref_core_set_names;
    while(1) {
	my $best_set = [];
	my $best_set_name = undef;
	SET: foreach my $setname (sort keys %$master_sets) {
	    my $set = $master_sets->{$setname};
	    next unless @$set > @$best_set;
	    foreach my $element (@$set) {
		next SET unless grep($_ eq $element, @$names);
	    }
	    $best_set = $set;
	    $best_set_name = $setname;
	}
	last unless @$best_set;
	@$names = map(($_ eq $best_set->[0] ? $best_set_name : $_), @$names);
	($names) = set_split($names, $best_set);
    }
    return join("__", @$names);
}
sub names_of_cores {
    my ($layout, @setnames) = @_;
    my %names;
    my @unrecognized;
    foreach my $setname (@setnames) {
	if ($setname eq "ALL") {
	    foreach (@{$layout->core_names}) { $names{$_} = 1; }
	    next;
	}
	my @parts = split("__", $setname, -1);
	@parts = ("") if $setname eq "";	
	foreach my $name (@parts) {
	    my $set = $layout->ref_core_set_names->{$name};
	    if (defined($set)) {
		foreach (@$set) { $names{$_} = 1; }
	    } elsif (grep($_ eq $name, @{$layout->ref_core_names})) {
		$names{$name} = 1;
	    } else {
		push @unrecognized, $name;
	    }
	}
    }
    return (normalize_set_of_cores($layout, [keys %names]), \@unrecognized);
}
sub name_of_all {
    my ($layout) = @_;
    return "ALL";
}
sub linkmap_name {
    my ($layout, $linkmap) = @_;
    my $linkmaps_byname = $layout->{linkmaps_byname};
    my ($name) = grep($linkmaps_byname->{$_} eq $linkmap, keys %$linkmaps_byname);
    defined($name) or $name = "__Unknown_Name__"; # Error("did not find name for linkmap $linkmap");
    return $name;
}
sub sharemap_cleanup {
    my ($layout) = @_;
    my $mems = $layout->ref_mems;
    foreach my $mem (keys %$mems) {
	my $mem_map = $mems->{$mem};
	@$mem_map = grep(scalar(@{$_->{share}->{set}}) != 0, @$mem_map);
	if (0 == @$mem_map) {
	    delete $mems->{$mem};
	}
    }
}
sub sharemap_find_mem_byte {
    my ($layout, $mem, $offset) = @_;
    my $mem_map = $layout->ref_mems->{$mem};
    if (defined($mem_map)) {
	foreach my $share_entry (@$mem_map) {
	    next if $share_entry->{addr} + $share_entry->{sizem1} < $offset;
	    if ($share_entry->{addr} <= $offset) {
		my $share_set = $share_entry->{share};
		my $offset_in_share_set = $share_entry->{offset} + ($offset - $share_entry->{addr});
		my $old_offset_in_share_set = $share_entry->{offset} + ($share_entry->{addr} - $offset);
$layout->dprintf("Found mem %s offset 0x%x in entry 0x%x..0x%x at offset 0x%x (not 0x%x) of share_set{sizem1=0x%x, [%s]}\n",
	$mem->name, $offset, $share_entry->{addr}, $share_entry->{addr} + $share_entry->{sizem1}, $offset_in_share_set, $old_offset_in_share_set, $share_set->{sizem1}, join(", ", map(sprintf("0x%x of %s", $_->{addr}, $_->{space}->name), @{$share_set->{set}})));
		if (0 == scalar(@{$share_set->{set}})) {
$layout->dprintf("sharemap_find_mem_byte: oops sharemap_cleanup\n");
		    $layout->sharemap_cleanup;
		    return $layout->sharemap_find_mem_byte($mem, $offset);
$layout->dprintf("sharemap_find_mem_byte: done\n");
		}
		return ($share_set, $share_entry, $offset_in_share_set);
	    }
	    return (undef, undef, $share_entry->{addr});
	}
    }
    return (undef, undef, undef);		
}
sub sharemap_add_share_entry {
    my ($layout, $mem, $addr, $sizem1, $shareset, $offset) = @_;
    my @sets = ();
    @sets = @{$layout->ref_mems->{$mem}} if exists($layout->ref_mems->{$mem});
    push @sets, { addr => $addr, sizem1 => $sizem1, share => $shareset, offset => $offset };
    @{$layout->ref_mems->{$mem}} = sort { $a->{addr} <=> $b->{addr} } @sets;
}
sub sharemap_process_mapping {
    my ($layout, $linkmap, $mapping) = @_;
    my $contig_sizem1 = 0;		
    my @mspans = $mapping->spans;
    my $span = $mspans[0];
    my ($share_set, $share_entry, $share_ofs)
	= $layout->sharemap_find_mem_byte($span->mem, $span->offset);
    if (defined($share_set)) {		
	while(1) {
	    my $entry_end = $share_entry->{addr} + $share_entry->{sizem1};
	    my $mm_end = $span->offset + $span->sizem1;
$layout->dprintf("sharemap_process_mapping: shared addr=0x%x span 0x%x..0x%x\n", $mapping->addr, $span->offset, $mm_end);
	    $contig_sizem1 += $span->sizem1;	
	    if ($entry_end < $mm_end) {	
		$contig_sizem1 -= ($mm_end - $entry_end);	
		last;
	    }
	    shift @mspans;
	    $span = undef;
	    last unless @mspans;		
	    $span = $mspans[0];
	    my ($next_share_set, $next_share_entry, $next_share_ofs)
		= $layout->sharemap_find_mem_byte($span->mem, $span->offset);
	    last unless defined($next_share_set) and $next_share_set eq $share_set
		    and $share_ofs + $contig_sizem1 + 1 == $next_share_ofs;
	    $share_entry = $next_share_entry;
	    $contig_sizem1++;
	}
	my ($left, $mid, $right) = $share_set->cut( $share_ofs, $contig_sizem1 );
	$share_set = $mid->[2] if defined($mid);
	defined($share_set) or InternalError("sharemap_process_mapping: missing shareset for mid portion");
    } else {
	$share_set = Xtensa::System::ShareSet->new($layout);
	while(1) {
	    my $mm_end = $span->offset + $span->sizem1;
$layout->dprintf("sharemap_process_mapping: new addr=0x%x span 0x%x..0x%x\n", $mapping->addr, $span->offset, $mm_end);
	    my $mm_new_sizem1 = $span->sizem1;		
	    if (defined($share_ofs)) {
		if ($share_ofs - 1 < $mm_end) {
		    $mm_new_sizem1 = ($share_ofs - 1) - $span->offset;
		}
	    }
	    $layout->sharemap_add_share_entry($span->mem, $span->offset, $mm_new_sizem1, $share_set,
					$contig_sizem1);
	    $contig_sizem1 += $mm_new_sizem1;		
	    last if $mm_new_sizem1 < $span->sizem1;	
	    shift @mspans;
	    $span = undef;
	    last unless @mspans;			
	    $span = $mspans[0];
	    my $next_share_set;
	    ($next_share_set, $share_entry, $share_ofs)
		= $layout->sharemap_find_mem_byte($span->mem, $span->offset);
	    last if defined($next_share_set);
	    $contig_sizem1++;
	}
	$share_set->set_sizem1($contig_sizem1);
$layout->dprintf("contig_sizem1 = 0x%x\n", $share_set->sizem1);
    }
    if ($contig_sizem1 < $mapping->sizem1) {
$layout->dprintf("sharemap_process_mapping: cut ending at 0x%x\n", $mapping->addr + $contig_sizem1);
	$mapping->cut(undef, $mapping->addr + $contig_sizem1, 0);
    }
    $share_set->add_share_space($linkmap, $mapping->addr);
    defined($mapping->{share}) and InternalError("sharemap_process_mapping: already added {share} ?");
    $mapping->{share} = $share_set;
}
sub sharemap_reset {
    my ($layout) = @_;
    foreach my $linkmap (@{$layout->{linkmaps}}) {
	foreach my $mapping ($linkmap->map->a) {
	    $mapping->{share} = undef;
	}
    }
    $layout->{mems} = {};
}
sub sharemap_process_linkmaps {
    my ($layout) = @_;
    $layout->set_modified(0);
    foreach my $linkmap (@{$layout->{linkmaps}}) {
	my $round = 0;
	while (1) {
	    my @mappings = grep( !defined($_->{share}), $linkmap->map->a );
	    last unless @mappings;
$round++; $layout->dprintf("add_linkmap: round $round: " . scalar(@mappings) . " of " . $linkmap->map->n . " remaining...\n");
	    my $mapping = shift @mappings;
	    $layout->sharemap_process_mapping($linkmap, $mapping);
	}
    }
}
sub sharemap_sort_by_set {
    my ($layout) = @_;
    my %share_sets = ();
    my $mems = $layout->{mems};
    foreach my $mem (keys %$mems) {
	my $what_shares_mem = $mems->{$mem};
	foreach my $share_entry (@$what_shares_mem) {
	    $share_sets{$share_entry->{share}} = $share_entry->{share};
	}
    }
    my %name_sets = ();
    my %name_addr_sets = ();		
    my %name_set_addrs = ();		
    foreach my $shareset (values %share_sets) {
	my %names = ();
	my %names_addr = ();
	foreach my $set (@{$shareset->{set}}) {
	    my $linkmap = $set->{space};
	    my ($name) = $layout->linkmap_name($linkmap);
	    $names{$name}++;
	    $names_addr{$set->{addr}}{$name}++;
	}
	my $setname = $layout->name_of_set_of_linkmaps([keys %names]);
	push @{$name_sets{$setname}}, $shareset;
	foreach my $sameaddr (sort keys %names_addr) {
	    my $sameaddr_names = $names_addr{$sameaddr};	
	    my $sameaddr_setname = $layout->name_of_set_of_linkmaps([keys %$sameaddr_names]);
	    exists($name_addr_sets{$sameaddr}{$sameaddr_setname})
		and InternalError("sharemap_sort_by_set: multiple share sets for same address $sameaddr and set $sameaddr_setname");
	    $name_addr_sets{$sameaddr}{$sameaddr_setname} = $shareset;
	    $name_set_addrs{$sameaddr_setname}{$sameaddr} = $shareset;
	}
    }
    return (\%name_sets, \%name_set_addrs, \%name_addr_sets );
}
sub shared_sets {
    my ($layout) = @_;
    if (scalar(keys %{$layout->{mems}}) == 0 || $layout->modified) {
	$layout->sharemap_reset;
	$layout->sharemap_process_linkmaps;
	if ($layout->modified) {
	    $layout->sharemap_process_linkmaps;
	    $layout->modified and InternalError("Xtensa::AddressLayout::shared_sets: modified flag still set after two processing runs");
	}
    }
    return $layout->sharemap_sort_by_set;
}
sub strtoint {
  my ($num, $errwhere) = @_;
  $num =~ s/^\s+//;	
  $num =~ s/\s+$//;
  return oct($num) if $num =~ /^0[0-7]*$/;
  return hex($num) if $num =~ /^0x[0-9a-f]+$/i;
  return $num      if $num =~ /^[1-9][0-9]*$/; 
  return undef unless defined($errwhere);
  Error("invalid syntax, integer expected, got '$num' in $errwhere\n"
	."Valid integer formats are [1-9][0-9]* (decimal), 0[0-7]* (octal), 0x[0-9a-f]+ (hex)");
}
sub construct_strtoint {
    my ($construct, $num, $what) = @_;
    return undef unless defined($num);
    my $n = strtoint($num);
    return $n if defined($n);
    construct_error($construct, "$what: invalid integer syntax '$num', expected one of:  [1-9][0-9]* (decimal), 0[0-7]* (octal), 0x[0-9a-f]+ (hex)");
}
sub find_addressable {
    my ($layout, $name) = @_;
    my $child = $layout->ref_children->{$name};
    return $child if defined($child) and $child->isa("Xtensa::System::Addressable");
    return undef;
}
sub element_name {
    my ($layout, $child) = @_;
    my $children = $layout->ref_children;
    my ($name) = grep($children->{$_} eq $child, keys %$children);
    return $name;
}
sub virtual {
    my ($layout, $namespace, $alternate) = @_;
    $namespace = defined($namespace) && $namespace ne "" ? $namespace."." : "";
    $alternate = defined($alternate) && $alternate ne "" ? ".".$alternate : "";
    $layout->find_addressable($namespace."virtual".$alternate);
}
sub physical {
    my ($layout, $namespace) = @_;
    $namespace = defined($namespace) && $namespace ne "" ? $namespace."." : "";
    $layout->find_addressable($namespace."physical");
}
sub add_addressable {
    my ($layout, $name, $addressable, $replace) = @_;
    if (!$replace and exists($layout->ref_children->{$name})) {
	Error("addressable named '$name' already exists in layout $layout");
    }
    $layout->ref_children->{$name} = $addressable;
    $addressable->{name} = $name;
    $addressable->{layout} = $layout;
    $addressable;
}
sub remove_addressable {
    my ($layout, $addressable) = @_;
    return unless defined($addressable);
    my $name = $addressable->{name};
    if (!defined($name)) {
	InternalError("attempt to remove addressable that has no name");
    }
    my $child = $layout->ref_children->{$name};
    if (defined($child) and $child ne $addressable) {
	InternalError("removing addressable named '$name': a different addressable is in the layout than that requested");
    }
    delete $layout->ref_children->{$name};
    delete $addressable->{layout};
}
sub find_core_index {
    my ($layout, $num_cores) = @_;
    $num_cores //= 1;
    my $next = 0;
    foreach my $core ($layout->cores) {
	return $next if $core->index - $next >= $num_cores;
	$next = $core->index + 1;
    }
    return $next;
}
sub add_core {
    my ($layout, $core, $replace) = @_;
    my $name = defined($core->name) ? $core->name : "(undefined)";
    if ($name eq "" and scalar(keys %{$layout->ref_cores_byname}) == 0) {
    } elsif ( $name !~ /^\w+(\.\w+)*$/	
      or $name =~ /^\d+$/		
      or $name =~ /^_/			
      or $name =~ /_$/			
      or $name =~ /__/			
      ) {
	Error("illegal core/master name '$name' - each name must consist of one or more identifiers separated by a period (.), no whitespace; each identifier must consist of alphanumerics or underscore and not be only a number nor start or end with an underscore nor contain consecutive underscores");
    }
    if (!$replace and grep("\L$_" eq "\L$name", values %{$layout->ref_cores_byname})) {
	Error("core named '$name' already exists in layout $layout");
    }
    if (exists($layout->ref_cores_byname->{""})) {
	Error("core with empty name is only allowed if it is the only core");
    }
    if (!defined($core->index)) {
	$core->set_index(scalar(keys %{$layout->ref_cores_byname}));
    }
    if (grep($_->index == $core->index, values %{$layout->ref_cores_byname})) {
	Error("core with index ".$core->index." already exists in layout $layout");
    }
    $layout->ref_cores_byname->{$name} = $core;
    @{$layout->ref_core_names} = map($_->name, $layout->cores);
    $core->{layout} = $layout;
    $core;
}
sub cores {
    my ($layout) = @_;
    sort { $a->index <=> $b->index } values %{$layout->ref_cores_byname};
}
sub addressables {
    my ($layout) = @_;
    my @children = map($layout->ref_children->{$_}, sort keys %{$layout->ref_children});
    return grep( $_->isa("Xtensa::System::Addressable"), @children );
}
sub addressables_deporder
{
    my ($layout) = @_;
    my @list = ();
    my %deps = ();
    foreach my $a ($layout->addressables) {
	$deps{$a->fullname} = { map(($_->fullname,1), $a->dependencies) };
    }
    $deps{'*root*'} = { map(($_->fullname,1), $layout->addressables) };
    while (keys %deps > 1) {
	my @nodeps = grep(keys %{$deps{$_}} == 0, keys %deps);
	if (!@nodeps) {
	    $layout->dprintf("\n**************************** LAYOUT $layout ...\n");
	    foreach my $ad ($layout->addressables) {
		$layout->dprintf(">>> " . $ad->format_self(1,1,{}) . "\n");
	    }
	    $layout->dprintf("**************************** END LAYOUT\n");
	    Error("cyclic dependency among the following addressable elements: " .
		  join(", ", sort keys %deps));
	}
	push @list, sort @nodeps;
	foreach my $a (@nodeps) { delete $deps{$a}; }
	foreach my $ad (keys %deps) {
	    foreach my $a (@nodeps) { delete ${$deps{$ad}}{$a}; }
	}
    }
    @list = map($layout->find_addressable($_), @list);	
    @list == ($layout->addressables) or InternalError("oops, internal error: wrong list size");
    @list;
}
sub format_addressables {
    my ($layout) = @_;
    my @lines = ();
    push @lines, "<addressables>";
    foreach my $ad ($layout->addressables_deporder) {
	push @lines, $ad->format_self(1, 1);
    }
    push @lines, "</addressables>";
    wantarray ? @lines : join("\n", @lines);
}
sub yaml_quote {
    my ($value) = @_;
    return "~" unless defined($value);
    return "{ ".join(", ", map("$_: ".yaml_quote($value->{$_}), sort keys %$value))." }"
	if ref($value) eq "HASH";
    return "[ ".join(", ", map(yaml_quote($_), @$value))." ]"
	if ref($value) eq "ARRAY";
    Error("don't know how to dump a ".ref($value)) if ref($value) ne "";
    return $value if $value =~ m|^[-a-zA-Z0-9_\+\.]+$|;
    return "'".$value."'" unless $value =~ m|'|;	# '
    Error("not sure how to quote << $value >>") if $value =~ m|"|;	# "
    return '"'.$value.'"'
}
sub format_xld {
    my ($constructs) = @_;
    my $out = "---\n";
    my @order = (
	[qw/construct name namespace optional description/],
	[qw/index config cores filename/],
	[qw/vecselect vecbase vecreset_pins/],
	[qw/space addrspace direct addressable offset/],
	[qw/contains alwaysto alwaysfrom memory memory_byaddr/],
	[qw/startaddr endaddr size sizem1/],
	[qw/minsize maxsize log2sized startalign endalign/],
	[qw/shared ordering lock lock_precedence/],
	[qw/requirement/],
	[qw/prefer/],
	[qw/attrs/],
	[qw/mapattrs/],
	[qw/keepsections sections/],
	[qw/assignments/],
    );
    my %maporder;
    foreach my $i (0 .. $#order) {
	my $list = $order[$i];
	foreach my $j (0 .. $#$list) {
	    $maporder{$list->[$j]} = [$i, $j, $i*100+$j];
	}
    }
    my $next = $#order;
    foreach my $c (@$constructs) {
	my $prefix = "- { ";
	foreach (keys %$c) { $next++; $maporder{$_} = [$next, 0, $next*100] unless exists($maporder{$_}); }
	my @params = sort { $maporder{$a}[2] <=> $maporder{$b}[2] } keys %$c;
	foreach my $i (0 .. $next) {
	    my @p = grep($maporder{$_}[0] == $i, @params);
	    next unless @p;
	    $out .= $prefix . join(", ", map("$_: ".yaml_quote($c->{$_}), @p)) . ",\n";
	    $prefix = "    ";
	}
	$out .= "  }\n";
    }
    return $out;
}
sub encode_key {
  my ($key) = @_;
  $key =~ s/(\W)/sprintf("%x:%x",(ord($1)>>4)&0xF,(ord($1)&0xF))/ge;
  return $key;
}
sub decode_key {
  my ($key) = @_;
  $key =~ s/([0-9a-fA-F])\:([0-9a-fA-F])/chr(oct("0x$1$2"))/ge;
  return $key;
}
sub parse_addressables {
    my ($layout, $namespace, $clear, @lines) = @_;
    %{$layout->ref_children} = () if $clear;	
    chomp(@lines);
    my $n = @lines;
    while (1) {
	@lines or Error("reading addressables: unexpected end of input, missing <addressables>");
	$_ = shift @lines;
	last if /^\s*\<addressables\>\s*$/;
    }
    while (1) {
	@lines or Error("reading addressables: unexpected end of input, missing </addressables>");
	return $layout->addressables if $lines[0] =~ /^\s*\<\/addressables\>\s*$/;
	if ($lines[0] =~ /^\s*\<memdev\s+/) {
	    Xtensa::System::Addressable->new_parse($layout, $namespace, \@lines);
	} elsif ($lines[0] =~ /^\s*\<space\s+/) {
	    Xtensa::System::AddressSpace->new_parse($layout, $namespace, \@lines);
	} elsif ($lines[0] =~ /^\s*\<segment\s+/) {
	    $layout->parse_segment(\@lines, $n + 1 - @lines);
	} else {
	    Error("reading addressables: unexpected line: ".$lines[0]);
	}
    }
}
sub  read_addressables {
    my ($layout, $namespace, $clear, $filename) = @_;
    open(XMAP,"<$filename") or return "could not read $filename: $!";
    $layout->parse_addressables($namespace, $clear, <XMAP>);
    close(XMAP) or return "could not close $filename: $!";
    return undef;
}
sub readfile {
    my ($filename, $whatdebug, $checkread) = @_;
    use FileHandle;
    my $handle = new FileHandle $filename, '<';
    if (! $handle) {
	return (undef, 1) if $checkread;
	Error("$whatdebug: cannot open '$filename' for reading");
    }
    ::dprintf(undef, "Reading $whatdebug from $filename ...\n");
    my @lines = $handle->getlines();
    $handle->close();
    foreach (@lines) { s|\r\n$|\n|g; }	
    return join('',@lines);
}
sub read_layout {
    my ($filename) = @_;
    my $yaml = readfile($filename, "address layout");
    my $debug_yaml = 0;
    my $debug_autoload = 0;
    my $load_code = 0;
    my $hash = YAML_Load_objects($yaml, 'autoload', undef, $load_code, $debug_yaml, $debug_autoload);
    my $layout = $hash->{layout};
    defined($layout) and ref($layout) eq "Xtensa::AddressLayout"
	or Error("YAML file does not contain an Xtensa::AddressLayout layout: $filename");
    return $layout;
}
sub read_xld {
    my ($layout, $filename, $namespace, $core, $searchpath, $includestack) = @_;
    $namespace = "" unless defined($namespace);
    my $what = $namespace ? " ($namespace)" : "";
    $filename =~ s|\\|/|g;	
    $filename =~ m|/$| and Error("including XLD file$what: filename cannot end with a directory separator: $filename");
    $what = "include $filename$what";
    $filename .= ".xld" unless $filename =~ m|\.[^/]*$|;
    my $rooted = ($filename =~ m|^([a-z]:)?/|i);
    if ($rooted) {
	my ($constructs, $err) = $layout->read_xld_file($filename, $what, $namespace, $searchpath, $includestack);
	return @$constructs unless $err;
	Error("$what: file not found");
    } else {
	my @paths = defined($searchpath) ? @$searchpath : (getcwd());
	if (!defined($core)) {
	    my @cores = $layout->cores;
	    $core = $cores[0] if @cores == 1;
	}
	if (defined($core)) {
	    push @paths, $core->config->xtensa_root . "/lib/components";
	}
	my $swtools = $layout->{xtensa_tools_root};
	if (defined($swtools)) {
	    push @paths, $swtools. "/lib/components";
	}
	if (!@paths) {
	    Error("$what: no search path and no core defined, nowhere to look for this file");
	}
	my @paths_with_includer = @paths;
	if (defined($includestack) and @$includestack) {
	    my $last = $includestack->[$#$includestack];
	    if ($last =~ m|^(.*)/[^/]*$|) {
		unshift @paths_with_includer, $1;
	    } 
	}
	foreach my $path (@paths_with_includer) {
	    my $fullpath = $path . "/" . $filename;
	    my ($constructs, $err) = $layout->read_xld_file($fullpath, $what, $namespace, \@paths, $includestack);
	    return @$constructs unless $err;
	}
	Error("$what: not found in search path:  ".join("  ", @paths));
    }
}
sub read_xld_file {
    my ($layout, $filename, $what, $namespace, $searchpath, $includestack) = @_;
    $includestack = [] unless defined($includestack);
    if (grep($_ eq $filename, @$includestack)) {
	Error("loop including file $filename\n".join("",map("     $_\n",@$includestack)));
    } elsif (@$includestack > 99) {
	Error("probable include file loop, reached depth 100\n".join("",map("     $_\n",@$includestack)));
    }
    push @$includestack, $filename;
    my ($file, $err) = readfile($filename, $what, 1);
    return (undef, 1) if $err;
    my $constructs = YAML_Load_objects($file);
    if (ref($constructs) eq "HASH") {
	$constructs = $constructs->{constructs};
    }
    ref($constructs) eq "ARRAY"
	or Error("$what does not contain a valid sequence of memory map constructs: $filename");
    foreach my $construct (@$constructs) {
	if (ref($construct) ne "HASH" or !exists($construct->{construct})) {
	    Error("$what does not contain valid memory map constructs: $filename");
	}
    }
    $layout->assign_constructs_to_namespace($namespace, $constructs);
    $layout->expand_include_constructs($constructs, undef, $searchpath, $includestack);
    return ($constructs, 0);
}
sub expand_include_constructs {
    my ($layout, $constructs, $namespace, $searchpath, $includestack) = @_;
    my @in = @$constructs;
    my @out;
    while (@in) {
	my $construct = shift @in;
	if ($construct->{construct} ne "include") {
	    push @out, $construct;
	    next;
	}
	my $filename = $construct->{filename};
	if (!defined($filename)) {
	    construct_error($construct, "the filename parameter is required");
	}
	construct_check_params($construct, qw/filename/);
	my $subnamespace = $construct->{namespace};
	if (defined($subnamespace) and $subnamespace ne "") {
	    if (defined($namespace) and $namespace ne "" and $subnamespace !~ /^\./) {
		$subnamespace = $namespace . "." . $subnamespace;
	    }
	} else {
	    $subnamespace = $namespace;
	}
	my $core = undef;
	my @included = $layout->read_xld($filename, $subnamespace, $core, $searchpath, $includestack);
	push @in, @included;
    }
    @$constructs = @out;
}
sub parse_segment {
    my ($layout, $lines, $lineno) = @_;
    $_ = shift @$lines;
#$layout->dprintf("########## Parsing SEGMENT <<$_>>\n");
    my $saveline = $_;
    s/^\s*\<segment\s+//;
    my $params = Xtensa::System::Attributes->parse_attributes(\$_);
    /^\/\>\s*$/ or Error("reading segment: bad end of line, expected '/>' at line $lineno: <<$_>>");
    foreach my $attr (keys %$params) {
      $attr =~ /^(_.*|lock|fixed)$/ or next;
      my $newattr = $attr;
      $newattr =~ s/^_//;
      $params->{attrs}->{$newattr} = $params->{$attr};
      delete $params->{$attr};
    }
    if (!exists($params->{construct})) {
      $params->{construct} = "segment";
    }
    $params->{_saveline} = $saveline;
    $params->{_lineno} = $lineno;
    $layout->linkmap_constructs->push($params);
}
sub construct_check_params {
    my ($construct, @allowed_params) = @_;
    my %check_p = %$construct;
    delete @check_p{@allowed_params};
    delete @check_p{qw/construct name namespace ordering optional description linkmap linkmaps shared _needsyms _layout _linkmap _saveline _lineno _step _steps _order/};
    if (keys %check_p) {
	my $msg = "unexpected construct parameter(s): ". join(", ", sort keys %check_p);
	if ($Xtensa::AddressLayout::global_debug) {
	    construct_error($construct, $msg);
	} else {
	    construct_warning($construct, $msg);
	}
    }
}
sub construct_add_section {
    my ($construct,
	$linkmap,		
	$seg,			
	$do_carved_lits,	
	$do_plain_lits,		
	) = @_;
    my ($section,$litsection);
    my ($secname,$secorder,$secminsz);
    my ($litname,$litorder,$litminsz,$litcreatesz);
    my $some_literals = 0;
    my $total_minsize = 0;
    my $sections = $construct->{sections};
    return (0, 0) unless defined($sections);
    ref($sections) eq "ARRAY"
	or construct_error($construct, "sections parameter must be an array of section definition strings");
    foreach my $secset (@$sections) {
	my ($section,$litsection,@oops) = split(",", $secset, 3);
	@oops and construct_error($construct, "unexpected comma after literal section in '$secset'");
	$some_literals = 1 if defined($litsection) and $some_literals == 0;
	my ($secname,$secorder,$secminsz,@oops2) = (split(":", $section), '', '', '');
	$secname ne '' or construct_error($construct, "missing section name");
	@oops2 > 3 and construct_error($construct, "unexpected colon after minimum size in '$section'");
#$construct->{_layout}->dprintf("########## Adding '%s' in 0x%x..0x%x of space %s\n", $secname, $seg->addr, $seg->endaddr, $linkmap->fullname);
	my $secorder_arg = ($secorder eq '') ? undef : $secorder;
	$secminsz = ($secminsz eq '' ? 0 : construct_strtoint($construct, $secminsz, "min size of section '$secname'"));
	$total_minsize += $secminsz;
	my $sec;
	if (defined($seg)) {
	    $sec = $linkmap->insert_section($seg, $secname, $secorder_arg, ($secminsz ? (minsize => $secminsz) : ()));
	    if (!defined($sec)) {
		construct_warn_or_error($construct, "failed to insert section $secname");
		next;
	    }
	}
	next unless defined($litsection);
	my ($litname,$litorder,$litminsz,$litcreatesz,@oops3) = split(":", $litsection);
	@oops3 and construct_error($construct, "unexpected colon after literal create size in '$litsection'");
	$litname = text_to_literal_name($secname) if !defined($litname) or $litname eq '';
	$litorder = undef if defined($litorder) and $litorder eq '';
	$litminsz = undef if defined($litminsz) and $litminsz eq '';
	$litcreatesz = undef if defined($litcreatesz) and $litcreatesz eq '';
	$litminsz = construct_strtoint($construct, $litminsz, "min size of literal section '$litname'");
	my $literal_is_optional = (defined($litminsz) and $litminsz == 0);
	my $carve_percent = (defined($litcreatesz) and $litcreatesz =~ s/\%$// and $litcreatesz ne "0");
	$litcreatesz = construct_strtoint($construct, $litcreatesz, "preferred available space (create size) for literal section '$litname'");
	if ($carve_percent and ($litcreatesz < 0 || $litcreatesz > 100)) {
	    construct_error($construct, "literal section create size, when expressed as percentage of main section's segment, must be in the range 0..100%");
	}
	next unless $do_carved_lits or $do_plain_lits;
	if (!defined($sec)) {
	    $sec = $linkmap->{sections_by_name}{Xtensa::AddressLayout::encode_key($secname)};
	    if (!defined($sec)) {
		my $msg = "section $secname not found in linkmap %s when adding its corresponding literal section";
		if ($literal_is_optional) {
		    construct_warning($construct, $msg, $linkmap->name);
		} else {
		    construct_warn_or_error($construct, $msg, $linkmap->name);
		}
		next;
	    }
	}
	my $secseg = $sec->segment;
	defined($secseg) or InternalError("section $secname has no segment");
	if ($carve_percent) {
	    $litcreatesz = ($secseg->sizem1 * 1.0 + 1.0) * ($litcreatesz / 100.0);
	    $litcreatesz = int($litcreatesz + 0.5);
	    $litcreatesz = 4 if $litcreatesz < 4;
	}
	$litcreatesz = undef unless defined($litcreatesz) and $litcreatesz > 0 and $do_carved_lits;
	next unless $litcreatesz or $do_plain_lits;
	if (!defined($linkmap->cores) or !@{$linkmap->cores}) {
	    construct_warning($construct, "literal section requested outside the context of a processor configuration; literal placement might not be optimal or correct");
	}
	my ($litseg, $created) = literal_for_text($linkmap, $linkmap->cores->[0]->config->pr, $secseg, $litminsz, $litcreatesz,
				($secorder ne "first"),
				(defined($sec) and $sec->orderTacking == 1) ? $sec : undef);
	$some_literals = 2 if $created;
	next unless $do_plain_lits;
	if (defined($litseg)) {
	    my $sec = Xtensa::Section->new($litname, $litorder, undef, minsize => $litminsz, createsize => $litcreatesz);
	    my $finalsec = $litseg->insertSection($sec);  
	    if ($sec ne $finalsec) {
		construct_warn_or_error($construct, "attempt to place literal section $litname in multiple segments (at 0x%x and 0x%x)", $finalsec->segment->addr, $litseg->addr);
		next;
	    }
	    construct_cut_mem_aliases($construct, $litseg, "literals for $secname", 1);
	} else {
	    my $msg1 = "could not find ".($litcreatesz ? "or cutout ":"")."space for ";
	    my $msg2 = "literal section $litname for section $secname at ".sprintf("0x%x", $secseg->addr);
	    if (defined($litminsz) and $litminsz > 0) {
		construct_warn_or_error($construct, $msg1."$litminsz bytes of ".$msg2);
	    } else {
		construct_info($construct, $msg1.$msg2);
	    }
	}
    }
    return ($some_literals, $total_minsize);
}
sub replaceone (&\@@) {
  my ($code, $aref, @newvalues) = @_;
  my $i = 0;
  foreach $_ (@$aref) {
    return splice (@$aref, $i, 1, @newvalues) if &$code;
    $i++;
  }
  return ();
}
sub replaceall (&\@@) {
  my ($code, $aref, @newvalues) = @_;
  my @indices = ();
  my $i = 0;
  foreach $_ (@$aref) {
    unshift(@indices, $i) if &$code;
    $i++;
  }
  my @result = ();
  foreach $i (@indices) {
    unshift @result, splice(@$aref, $i, 1, @newvalues);
  }
  return @result;
}
sub array_extract (&\@;\@\@\@\@\@\@\@\@\@\@\@\@) {
  my $code = shift;
  my @result;
  foreach my $aref (@_) {
    push @result, &replaceall ($code, $aref);
  }
  @result;
}
sub construct_cut_mem_aliases {
    my ($construct, $seg, $lockmsg, $lockout) = @_;
    $lockout = 2 unless defined($lockout) and $lockout >= 0 and $lockout <= 4;
    my $linkmap = $construct->{_linkmap};
    my $linkmaps = $construct->{linkmaps};
    $linkmaps //= [ $linkmap ];
    my $lmnames = join("+", map($_->name, @$linkmaps));
    my $layout = $construct->{_layout};
    my $name = $construct->{name};
    my $construct_type = $construct->{construct};
    $layout->dprintf( "\$             (%s) %s %s did 0x%x..0x%x (%s) (linkmaps=%s lockout=%d)\n",
		$lmnames, $construct_type, $name // "(unnamed)", $seg->addr, $seg->endaddr,
		$lockmsg, join(",", map($_->name, @$linkmaps)), $lockout );
    foreach my $other_linkmap (@{$layout->{linkmaps}}) {
	my $shared_linkmap = grep($_ eq $other_linkmap, @$linkmaps);
	my $this_linkmap = ($other_linkmap eq ($linkmap // $other_linkmap));
	my $cuts = $other_linkmap->cut_mem_aliases($seg, undef, 0, 0);
	foreach my $cutseg (@$cuts) {
	    my $other_addr = !($cutseg->addr <= $seg->endaddr && $cutseg->endaddr >= $seg->addr);
	    my $lock_it = [ 0,
			    ($other_addr && $this_linkmap),
			    ($other_addr && $shared_linkmap),
			    ($other_addr || !$shared_linkmap),
			    ($other_addr || !$this_linkmap) ]->[$lockout];
	    $layout->dprintf("\*             (%s) alias 0x%x..0x%x (%s) of %s (%s) %s\n",
	    		$lmnames, $cutseg->addr, $cutseg->endaddr, ($other_addr ? "other" : "same"),
			$other_linkmap->name, ($this_linkmap ? "this" : $shared_linkmap ? "shared" : "other"),
			$lock_it ? "marked for delete" : "cut only");
	    if ($lock_it) {
		if ($cutseg->sections->n) {
		    construct_error($construct, "assigning segment 0x%x..0x%x to linkmap %s for '%s', however its alias at 0x%x..0x%x in linkmap %s has sections in it:  %s", $seg->addr, $seg->endaddr, $linkmap->name, $lockmsg, $cutseg->addr, $cutseg->endaddr, $other_linkmap->name, join(", ", map($_->name, $cutseg->sections->a)));
		}
		$cutseg->set_attr( 'delete' => 1, 'lock' => "alias of $lockmsg" );
	    }
	}
    }
}
sub construct_translate_addrs {
    my ($construct, $addrspace, $space, $startaddr, $endaddr) = @_;
    my ($spacename, $addrspacename) = @{$construct}{qw/space addrspace/};
    my ($s_start, $s_end,  $s_min, $s_max, $attrs, $attrsmap) = ($startaddr, $endaddr,  0, $space->sizem1, {}, {});
    my ($s_minmap, $s_maxmap) = ($s_min, $s_max);
    if ($addrspace ne $space) {
	my $lo = defined($startaddr);
	my $testaddr = $lo ? $startaddr : $endaddr;
	my $addr;
	($addr, $s_min, $s_max, $s_minmap, $s_maxmap, $attrs, $attrsmap)
		= $addrspace->addr_span_in_space($testaddr, $space, 0, !$lo, $lo);
	if (!defined($addr)) {
	    my @s_addrs = $space->addrs_of_space($addrspace, ($startaddr // $endaddr), ($endaddr // $startaddr));
	    if (@s_addrs == 1) {
		$addr = $s_addrs[0]->[0];
		my $span = $s_addrs[0]->[1];
		$s_min = $span->addr;
		$s_max = $span->endaddr;
		$s_minmap = $s_min;
		$s_maxmap = $s_max;
		$attrsmap = {};		
		construct_info($construct, "addrspace %s (0x%x..0x%x) is a descendant of space %s (0x%x in 0x%x..0x%x)",
			    $addrspacename, ($startaddr // $endaddr), ($endaddr // $startaddr), ($spacename // $space->name // "(unnamed)"), $addr, $s_min, $s_max);
	    } elsif (@s_addrs) {
		construct_error($construct, "addrspace %s (0x%x..0x%x) is a descendant of space %s with multiple mappings (%s), and no selection mechanism is implemented",
			    $addrspacename, $startaddr, $endaddr, $spacename,
			    join(", ", map(sprintf("0x%x",$_->[0]), @s_addrs)));
	    }
	}
	if (!defined($addr)) {
	    construct_error($construct, "addrspace %s address 0x%x does not reach space %s",
			    $addrspacename, $testaddr, $spacename);
	}
	if (defined($startaddr)) {
	    $s_start = $addr;
	    my $highend = $startaddr + ($s_max - $addr);
	    if (defined($endaddr)) {
		if ($highend < $endaddr) {
		    construct_error($construct, "addrspace %s address range 0x%x..0x%x does not all reach space %s",
			    $addrspacename, $startaddr, $endaddr, $spacename);
		}
		$s_end = $addr + ($endaddr - $startaddr);
	    }
	} else {
	    $s_end = $addr;
	}
    }
    return ($s_start, $s_end, $s_min, $s_max, $s_minmap, $s_maxmap, $attrs, $attrsmap);
}
sub construct_size_per_min_align {
    my ($construct, $t_sizem1, $fixedaddr, $fixedname, $log2sized, $minsizem1) = @_;
    if ($t_sizem1 < $minsizem1) {
	return (undef, sprintf("available space 0x%x is less than minsize=0x%x", $t_sizem1+1, $minsizem1+1));
    }
    if ($log2sized) {
	my $aligned_size = ($t_sizem1 == ~0 ? 0 : (1 << log2($t_sizem1 + 1)));
	my $fixed_aligned = ($fixedaddr & -$fixedaddr);
	if ($fixed_aligned != 0 and $fixed_aligned - 1 < $minsizem1) {
	    my $errmsg = sprintf("cannot naturally align to a power-of-2 size: 0x%x byte alignment of $fixedname address is smaller than minsize=0x%x", $fixed_aligned, $minsizem1+1);
	    return (undef, $errmsg) if $log2sized == 1;
	    construct_info($construct, $errmsg);
	    return ($t_sizem1, undef);
	} elsif ($aligned_size != 0 and $aligned_size - 1 < $minsizem1) {
	    my $errmsg = sprintf("cannot naturally align to a power-of-2 size: no such size between minsize=0x%x and available size 0x%x", $minsizem1+1, $t_sizem1 + 1);
	    return (undef, $errmsg) if $log2sized == 1;
	    construct_info($construct, $errmsg);
	    return ($t_sizem1, undef);
	}
	$aligned_size = $fixed_aligned if $fixed_aligned != 0
				and ($aligned_size == 0 or $fixed_aligned < $aligned_size);
	if ($aligned_size == 0) {
	} else {
	    my $aligned_sizem1 = $aligned_size - 1;
	    my $subalign = $aligned_size;
	    while ($aligned_sizem1 < $minsizem1) {
		$aligned_sizem1 = ($t_sizem1 & -$subalign);
		$subalign = (1 << (log2($subalign) - 1));
	    }
	    $t_sizem1 = $aligned_sizem1;
	}
    }
    return ($t_sizem1, undef);
}
sub construct_find_addressable {
    my ($construct, $name, $what, $is_addrspace, $default) = @_;
    return $default unless defined($name);
    my $addressable = $construct->{_layout}->find_addressable($name);
    if (!defined($addressable)) {
	construct_error($construct, "%s named '%s' not found", $what, $name);
    }
    if ($is_addrspace and !$addressable->is_addrspace) {
	construct_error($construct, "%s '%s' is a memory or device, expected an address space", $what, $name);
    }
    if (!defined($addressable->sizem1)) {
	construct_error($construct, "%s '%s' does not yet have a size (use ordering to ensure it is placed and sized before being referenced?)", $what, $name);
    }
    return $addressable;
}
sub construct_addressed {
    my ($construct, $check_only) = @_;
    my ($layout, $linkmap, $linkmaps) = @{$construct}{qw/_layout _linkmap linkmaps/};
    my ($construct_type, $namespace, $name, $description, $step) = @{$construct}{qw/
	 construct        namespace   name   description  _step /};
    my ($memoryname, $memory_strict, $attrs, $mapattrs, $ignorelock, $addressable) = @{$construct}{qw/
	 memory       memory_strict   attrs   mapattrs   ignorelock   addressable/};
    my ($spacename,$addrspacename, $alwaysto,$alwaysin, $contains,  $partition) = @{$construct}{qw/
	 space      addrspace       alwaysto  alwaysin   contains    partition /};
    my @pertype = ($construct_type eq 'segment' ? qw/sections keepsections partition cores/
		 : $construct_type eq 'space'  ? qw/offset direct space alwaysin mapattrs alwaysto contains/
		 : $construct_type eq 'memory' ? qw/offset direct space alwaysin mapattrs/
		 : $construct_type eq 'map'    ? qw/offset direct space alwaysin addressable/
		 : qw// );
    construct_check_params($construct, qw/addrspace startaddr endaddr startalign endalign size sizem1 minsize maxsize log2sized prefer requirement memory memory_strict memory_byaddr attrs ignorelock assignments/, @pertype);
    my $ns = (defined($namespace) and $namespace ne "") ? $namespace."." : "";
    foreach ($name, $spacename, $addrspacename, $addressable, $memoryname, $memory_strict, $alwaysto, $alwaysin) {
	next unless defined($_);
	next if s/^\.//;	
	$_ = $ns.$_;		
    }
    my $startaddr =    construct_eval($construct, "startaddr",	undef, $check_only);
    my $endaddr =      construct_eval($construct, "endaddr",	undef, $check_only);
    my $startalign =   construct_eval($construct, "startalign",	1,     $check_only);
    my $endalign =     construct_eval($construct, "endalign",	1,     $check_only);
    my $size =         construct_eval($construct, "size",	undef, $check_only);
    my $sizem1 =       construct_eval($construct, "sizem1",	undef, $check_only);
    my $minsize =      construct_eval($construct, "minsize",	0,     0);
    my $maxsize =      construct_eval($construct, "maxsize",	undef, $check_only);
    my $log2sized =    construct_eval($construct, "log2sized",	undef, $check_only);
    my $offset =       construct_eval($construct, "offset",	0,     $check_only);
    my $memory_byaddr= construct_eval($construct,"memory_byaddr",undef,$check_only);
    my $keepsections = construct_eval($construct,"keepsections",0,     $check_only);
                       construct_eval($construct, "optional",	0,     $check_only);
    my $direct =       construct_eval($construct, "direct",	undef, 0);
    my $prefer =       construct_eval($construct, "prefer",	undef, 1);
    my $requirement =  construct_eval($construct, "requirement",undef, 1);
    my $need_space = (defined($startaddr) or defined($endaddr) or $startalign != 1 or $endalign != 1
	  or $minsize or defined($maxsize) or defined($log2sized) or defined($prefer));
    my $have_space = (defined($spacename) or $construct_type eq "segment");
    if (!$have_space) {
	if ($need_space and ($construct_type eq "memory" or $construct_type eq "space")) {
	    construct_error($construct, "the following parameters imply placement into a containing address space and require the space parameter:  startaddr, endaddr, startalign, endalign, minsize, maxsize, log2sized, prefer");
	}
    }
    my $specific_addr = (defined($startaddr) or defined($endaddr));
    if (defined($attrs)) {
	ref($attrs) eq "HASH" or construct_error($construct, "attrs parameter must be a set (hash) of attr=value pairs");
    } else {
	$attrs = {};
    }
    if (defined($partition)) {
	ref($partition) eq "HASH" or construct_error($construct, "partition parameter must be a set (hash) of linkmapname=EXPR[%%] pairs");
	foreach my $name (sort keys %$partition) {
	    my $portion = $partition->{$name};
	    my $is_percent = ($portion =~ s/\%$//);
	    my $portion_size = construct_eval_string($construct, $portion, undef, 0, "<partition> parameter (linkmap $name)");
	    if ($portion_size < 0) {
		construct_error($construct, "partition parameter:  partition portion cannot be negative (%s => %s = %s)", $name, $portion, $portion_size);
	    }
	    $layout->dprintf("construct partition.%s => %s = %s (is_percent=%u)\n", $name, $portion, $portion_size, $is_percent);
	    $partition->{$name} = ($portion_size + 0).($is_percent ? "%" : "");
	}
    }
    if (defined($size)) {
	if (defined($sizem1)) {
	    construct_error($construct, "cannot specify both size and sizem1");
	}
	if ($check_only) {
	    $sizem1 = 0;
	} else {
	    if ($size == 0) {
		construct_error($construct, "cannot specify a size of zero");
	    }
	    $sizem1 = $size - 1;
	}
    }
    my $size_explicit = defined($sizem1) ? 1 : 0;
    my $size_by_addrs = (defined($startaddr) and defined($endaddr)) ? 1 : 0;
    my $size_searched = (defined($maxsize) or ($minsize > 0 and $construct_type ne "segment")) ? 1 : 0;
    my $size_specs = $size_explicit + (defined($prefer)?0:$size_by_addrs) + $size_searched;
    if ($size_specs > 1) {
	construct_error($construct, "only one of the three ways of specifying size must be used:  size or sizem1; or, both startaddr and endaddr and no prefer specified; or, maxsize (or minsize>0 for constructs other than segment)");
    } elsif ($size_specs == 0) {
	$size_specs = $size_by_addrs;
	if ($size_specs == 0 and !$have_space) {
	    construct_error($construct, "size missing to construct an unmapped $construct_type");
	}
    }
    my $ask_cut = ($specific_addr or $size_specs);			
    if ($construct_type eq "map" and !defined($addressable) && !defined($spacename)) {
	construct_error($construct, "the map construct requires at least one of the space or addressable parameters (usually both, unless one can be inferred from the other via an earlier alwaysin or alwaysto parameter)");
    }
    if (($construct_type eq "space" or $construct_type eq "memory") and !defined($name)) {
	construct_error($construct, "the space and memory constructs require the name parameter");
    }
    if ($construct_type ne "segment" and (defined($direct) or (!$check_only and $offset != 0) or defined($mapattrs)) and !defined($spacename)) {
	construct_error($construct, "the direct, offset and mapattrs parameters require specifying the space parameter");
    }
    if (defined($mapattrs)) {
	ref($mapattrs) eq "HASH" or construct_error($construct, "mapattrs parameter must be a set (hash) of attr=value pairs");
    } else {
	$mapattrs = {};
	$mapattrs = $attrs if $construct_type eq "map";		
    }
    $direct = ($construct_type eq "memory" ? 0 : 1) unless defined($direct);
    if (defined($memoryname) and defined($memory_strict)) {
	construct_error($construct, "cannot specify both memory and memory_strict");
    }
    my (undef, $total_minsize) = construct_add_section($construct, $linkmap, undef, 0, 0, 0);
    $minsize += $total_minsize;
    if ($check_only) {
	evaluate_symbol_assignments($construct, undef, 1, 1);
	if ($construct_type eq 'segment') {
	    my $findcut_order = (defined($partition) ? 'partition'
				: $specific_addr ? ($size_specs ? 'cut_at' : 'cut_try')
				: $ask_cut ? 'cut_scan' : 'no_cut');	
	    construct_default_ordering($construct,
			['find/cut', 'cut-lit', 'place-lit'],		
	    		[$findcut_order, 'cut_scan', 'no_cut']);	
	} else {
	    my $map_order = ($specific_addr ? '_at' : '_scan');
	    if ($direct) {
		$map_order = "map".$map_order;
	    } else {
		$map_order = ($construct_type eq "space" ? "space" : "memory").$map_order;
	    }
	    if ($construct_type eq 'map') {
		construct_default_ordering($construct, ['map'], [$map_order]);
	    } else {
		construct_default_ordering($construct, ['create', 'map'], ['creation', $map_order]);
	    }
	}
	return;
    }
    my ($startaddr_implied, $endaddr_implied) = ("", "");
    if ($size_by_addrs) {
	$sizem1 = $endaddr - $startaddr unless $size_explicit;
    } elsif ($size_explicit) {
	if (defined($startaddr)) {
	    ~0 - $startaddr < $sizem1
		and construct_error($construct, "startaddr 0x%x + sizem1 0x%x exceed 0x%x", $startaddr, $sizem1, ~0);
	    $endaddr = $startaddr + $sizem1;
	    $endaddr_implied = " (implied by startaddr and size)";
	} elsif (defined($endaddr)) {
	    $endaddr < $sizem1
		and construct_error($construct, "sizem1 0x%x is larger than endaddr 0x%x", $sizem1, $endaddr);
	    $startaddr = $endaddr - $sizem1;
	    $startaddr_implied = " (implied by endaddr and size)";
	}
    }
    my $alloc_down = 0;
    if (defined($maxsize) and $maxsize < 0) {
	$alloc_down = 1;
	$maxsize = - $maxsize;
    }
    my $unaligned = (($startalign & ($startalign-1)) != 0 or ($endalign & ($endalign-1)) != 0);
    if (!defined($log2sized)) {
	$log2sized = ($construct_type eq "segment" or $unaligned
		      or (defined($maxsize) && ($maxsize & ($maxsize-1)) != 0)) ? 0 : 2;
    }
    if ($log2sized and $unaligned) {
	construct_error($construct, "log2sized is not compatible with non-power-of-2 startalign or endalign");
    }
    if ($startalign > 1 and defined($startaddr) and !defined($prefer)) {
	construct_error($construct, "startaddr$startaddr_implied and startalign cannot be both specified unless prefer is specified (the prefer parameter requests a search; startaddr is tried first, then a search is made if that fails and startalign is applied on the results on that search)");
    }
    if ($endalign > 1 and defined($endaddr) and !defined($prefer)) {
	construct_error($construct, "endaddr$endaddr_implied and endalign cannot be both specified unless prefer is specified (the prefer parameter requests a search; endaddr is tried first, then a search is made if that fails and endalign is applied on the results on that search)");
    }
    if (defined($maxsize) and $minsize > $maxsize) {
	construct_error($construct, "minsize (0x%x) cannot be greater than maxsize (0x%x)", $minsize, $maxsize);
    }
    if (defined($maxsize) and $maxsize < 1) {
	construct_error($construct, "maxsize must be at least 1");
    }
    my $minsizem1 = $minsize > 0 ? $minsize - 1 : 0;
    my $target;
    if ($construct_type eq "space" or $construct_type eq "memory") {
	if (!$have_space) {
	    $requirement = construct_eval($construct, "requirement", 1, 0, "(no search)");
	    if (!$requirement) {
		construct_warn_or_error($construct, "<requirement> test failed");
		return;
	    }
	}
	if ($step == 0) {
	    if ($construct_type eq "space") {
		$target = Xtensa::System::AddressSpace->new($contains, $sizem1, $attrs);
	    } else {
		$target = Xtensa::System::Addressable->new($sizem1, undef, $attrs);
	    }
	    $layout->add_addressable($name, $target);
	    construct_schedule_step($construct, 1);
	    return;
	}
	$addressable = $name;
	$target = $layout->find_addressable($name);
	defined($target) or construct_internal_error($construct, "$construct_type named '%s' disappeared?", $name);
	$construct_type ne "space" or $target->is_addrspace	
	    or construct_internal_error($construct, "space named '%s' is now a memory?", $name);
    } else {
	$target = construct_find_addressable($construct, $addressable, "addressable", 0);
    }
    my $space = construct_find_addressable($construct, $spacename, "space", 1);
    if ($construct_type eq "map") {
	if (!defined($target) && defined($space)) {
	    $target = $space->alwaysto;
	    if (!defined($target)) {
		construct_error($construct, "the map construct requires the addressable parameter (unless implied by the space parameter specifying an address space that was defined with an alwaysto parameter)");
	    }
	    $addressable = $target->name;
	} elsif (defined($target) && !defined($space)) {
	    $space = $target->alwaysin;
	    if (!defined($space)) {
		construct_error($construct, "the map construct requires the space parameter (unless implied by the addressable parameter specifying a memory or space that was defined with an alwaysin parameter)");
	    }
	    $spacename = $space->name;
	}
	$have_space = 1;
    }
    if ($construct_type eq "map" or $offset != 0) {
	if ($offset < 0 || $offset > $target->sizem1) {
	    construct_error($construct, "start of mapping (offset 0x%x) is outside the bounds 0..0x%x of addressable '%s'", $offset, $target->sizem1, $addressable);
	}
	if (defined($sizem1) and ($sizem1 > $target->sizem1 || $offset > $target->sizem1 - $sizem1)) {
	    construct_error($construct, "end of mapping (offset 0x%x + sizem1 0x%x) is outside the bounds 0..0x%x of addressable '%s'", $offset, $sizem1, $target->sizem1, $addressable);
	}
    }
    my $multiple_linkmaps = 0;
    if ($construct_type eq "segment") {
	$multiple_linkmaps = (!defined($linkmap) and defined($linkmaps) and @$linkmaps > 1);
	$linkmaps = [$linkmap] unless $multiple_linkmaps;
	if ($step == 1) {		
	    foreach my $lm (@$linkmaps) {
		construct_add_section($construct, $lm, undef, 1, 0);
	    }
	    construct_schedule_step($construct, 2);
	    return;
	}
	if ($step == 2) {		
	    foreach my $lm (@$linkmaps) {
		construct_add_section($construct, $lm, undef, 0, 1);
	    }
	    return;
	}
    }
    my $spacewhat;
    if (defined($spacename)) {
	$spacewhat = "space $spacename";
    } elsif ($construct_type eq "segment") {
	if ($multiple_linkmaps) {
	    $linkmap = $layout->intersect_linkmaps(undef, $linkmaps, undef,
	    	{lock=>1, delete=>1, partition=>1});	
	    $spacewhat = "intersection of linkmaps";
	    $layout->dprintf("INTERSECTION LINKMAP:\n%s\n", $linkmap->dump);
	} else {
	    $spacewhat = "linkmap";
	}
	$space = $linkmap;
    }
    my $addrspace = construct_find_addressable($construct, $addrspacename, "addrspace", ($construct_type ne "segment"), $space);
    my $to_space = construct_find_addressable($construct, $alwaysto, "alwaysto space", 1);
    my $in_space = construct_find_addressable($construct, $alwaysin, "alwaysin space", 1);
    $memoryname //= $memory_strict;
    my $memory = construct_find_addressable($construct, $memoryname, "memory/device", 0);;
    if (defined($memory_byaddr)) {
	defined($memory) and construct_error($construct, "cannot specify both memory and memory_byaddr");
	my $bytespace = $addrspace->extract(2, $memory_byaddr, $memory_byaddr);
	my ($m) = $bytespace->map->a;
	defined($m) or construct_error($construct, "memory/device at address 0x%x not found\nLooked in the following addrspace (%s):\n%s\n", $memory_byaddr, ($addrspace->name // "(unnamed)"), $addrspace->dump);
	my $a_memory = ($m->spans)[0]->mem;
	defined($a_memory) or construct_error($construct, "oops memory/device at address 0x%x not found", $memory_byaddr);
	my $memname = $layout->element_name($a_memory);
	if (defined($memory) and $memory ne $a_memory) {
	    construct_error($construct, "both memory and memory_byaddr parameters are specified but refer to different memories:  '%s', '%s'", $memoryname, ($memname // sprintf("(no name found for memory at 0x%x)", $memory_byaddr)));
	}
	$memory = $a_memory;
	$memname = "(no name found for $memory)" unless defined($memname);
	$layout->dprintf( "*             (%s) found memory %s (name=%s) at 0x%x\n",
		($spacename // $linkmap->name), $memname, $memory->name, $memory_byaddr);
    }
    if (defined($to_space)) {
	if (defined($target->alwaysto) and $target->alwaysto ne $to_space) {	
	    construct_internal_error($construct, "space %s alwaysto=%s: already has alwaysto of %s", $name, $alwaysto, $target->alwaysto->name);
	}
	$target->{alwaysto} = $to_space;
	if (grep(grep($_->mem ne $to_space, $_->spans), $target->map->a)) {
	    construct_error($construct, "space %s maps to addressable(s) other than alwaysto=%s", $name, $alwaysto);
	}
    }
    if (defined($in_space)) {
	my $the_name = ($construct_type eq "space" or $construct_type eq "memory") ? $name : $addressable;
	if (defined($target->alwaysin) and $target->alwaysin ne $in_space) {
	    construct_error($construct, "addressable %s alwaysin=%s: already has alwaysin of %s", $the_name, $alwaysin, $target->alwaysin->name);
	}
	$target->{alwaysin} = $in_space;
	push @{$in_space->{alwaysin_here}}, $target
	    unless grep($_ eq $target, @{$in_space->{alwaysin_here}});
	my @other_spaces = grep($_->is_addrspace && $_ ne $in_space, $layout->addressables);
	my @oops_spaces = grep(grep(grep($_->mem eq $target, $_->spans), $_->map->a), @other_spaces);
	if (@oops_spaces) {
	    construct_error($construct, "addressable %s alwaysin=%s is mapped in other address space(s) %s", $the_name, $alwaysin, join(", ", map($_->name, @oops_spaces)));
	}
    }
    if (!$have_space) {		
	evaluate_symbol_assignments($construct, undef, 0, 1);
	return;
    }
    my $mapping;
    my $cutout_mapping;
    my $override_lock = 0;
    my ($t_space, $t_fits, $t_attrs, $t_startalign, $t_endalign, $startmask, $endmask);
    my ($t_start, $t_end, $t_sizem1, $t_min, $t_max, $t_left, $t_mid, $t_right);
    if (defined($sizem1) and $construct_type ne "segment") {
	$space ne $target or construct_error($construct, "cannot insert address space into itself ($name)");
	if ($target->is_addrspace) {
	    my $testloop = $target->extract(1, undef, undef, $space);
	    my @loopmems = grep(grep($_->mem eq $space, $_->spans), $testloop->map->a);
	    if (@loopmems) {
		construct_error($construct, "cyclic address mapping through address spaces $name and $spacename (for example at address 0x%x of the latter)", $loopmems[0]->offset);
	    }
	}
    }
    if ($specific_addr) {
	my ($s_start, $s_end, $s_minmin, $s_maxmax, $s_min, $s_max, $attrswide, $attrsmap)
	    = construct_translate_addrs($construct, $addrspace, $space, $startaddr, $endaddr);
	($t_space, $t_start, $t_end, $t_min, $t_max, $t_fits, $t_left, $t_mid, $t_right, $t_attrs)
	= ($space, $s_start, $s_end, $s_min, $s_max, 0, 0, 0, 0, {});
	($t_startalign, $t_endalign, $startmask, $endmask) = ($startalign, $endalign, 0, 0);
	my $errmsg;
	if ($construct_type eq "segment") {
	    $mapping = $space->cuttable_map($t_start, $t_end);	
	    if (defined($mapping)) {
		my $mwhat = sprintf("segment at 0x%x..0x%x", $mapping->addr, $mapping->endaddr);
		($t_startalign, $errmsg) = combine_align($startalign, $mapping->has_attr('startalign',1),
						"startalign parameter", "startalign attribute of $mwhat");
		($t_endalign, $errmsg) = combine_align($endalign, $mapping->has_attr('endalign',1),
						"endalign parameter", "endalign attribute of $mwhat")
						if defined($t_startalign);
		if (!defined($t_startalign) or !defined($t_endalign)) {
		    $t_fits = 0;	
		} else {
		    $t_min = $mapping->addr;
		    $t_max = $mapping->endaddr;
		    $t_attrs = $mapping->attributes;
		    ($t_fits, $errmsg, $t_left, $t_mid, $t_right) = $mapping->cut_okay($t_start, $t_end, $minsize);
		    if ($t_fits and (($mapping->is('lock') and (!defined($ignorelock) or $mapping->is('lock') !~ m|$ignorelock|))
				     or $mapping->is('exception')
				 or $mapping->is('delete'))) {
			if ($mapping->is('lock') and $mapping->is('lock') =~ /^partition/ and $mapping->is('partition')
			    and !$mapping->is('exception') and !$mapping->is('device')
			    and !defined($prefer)) {
			    $override_lock = 1;
			} else {
			    $t_fits = 0;
			    my @why;
			    push @why, "locked (".$mapping->is('lock').")" if $mapping->is('lock');
			    push @why, "marked to take exception" if $mapping->is('exception');
			    push @why, "assigned to a device" if $mapping->is('device');
			    push @why, "marked for deletion" if $mapping->is('delete');
			    $errmsg = "specified address range is ".join(", ", @why);
			}
		    }
		}
	    } else {
		$errmsg = $size_by_addrs ? "no segment found spanning entire specified address range"
					 : "no segment found at specified address";
	    }
	} else {
	    if ($direct) {
		$t_fits = 1;		
	    } else {
		my ($t_addr, $t_mmin, $t_mmax, $maxattrs);
		my $ref_addr = $s_start // $s_end;
		($t_addr, $t_mmin, $t_mmax, $t_min, $t_max, $maxattrs, $t_attrs, $t_space)
		    = $space->addr_span_in_space($ref_addr, undef, 1, 0, 0);
		$t_fits = $t_space->is_addrspace;
		if (!$t_fits) {
		    $errmsg = $t_space->name . " is not an address space";
		} elsif (defined($s_start)) {
		    $t_start = $t_addr;
		    if (defined($s_end)) {
			my $t_sizm1 = $s_end - $s_start;
			if (~0 - $t_start >= $t_sizm1) {
			    $t_end = $t_start + $t_sizm1;
			} else {
			    $errmsg = "in bounds in space ".$space->name.", out of bounds in ".$t_space->name;
			    $t_fits = 0;
			}
		    }
		} else {
		    $t_end = $t_addr;
		}
		$layout->dprintf("*** fits in space %s in range 0x%x..0x%x (for 0x%x..0x%x)\n", $t_space->name, $t_min, $t_max, $t_start, $t_end) if $t_fits;
	    }
	    if ($t_fits) {
		($t_startalign, $startmask, $errmsg) = combine_align_mask($startalign, $t_space->has_attr('startmask'),
					"startalign parameter", "startmask attribute of ".$t_space->name);
		($t_endalign, $endmask, $errmsg) = combine_align_mask($endalign, $t_space->has_attr('endmask'),
					"endalign parameter", "endmask attribute of ".$t_space->name)
					if defined($t_startalign);
		$t_fits = 0 unless defined($t_startalign) and defined($t_endalign);
	    }
	    if ($t_fits) {
		my ($newmin, $newmax, $maps) = $t_space->findspan($t_start // $t_end, $t_end);
		if (@$maps == 0) {
		    $t_min = $newmin if $newmin > $t_min;
		    $t_max = $newmax if $newmax < $t_max;
		} else {
		    $t_fits = 0;
		    my ($map) = @$maps;
		    if (@$maps == 1 and defined($t_start) and defined($t_end)
				    and $map->addr <= $t_start and $map->endaddr >= $t_end
			    and ($map->is('background',0) or	
				 ($map->is('overridable',0) and $mapattrs->{overriding}))) {
			$cutout_mapping = $map;
			delete $mapattrs->{overriding};
construct_info($construct, "did override");
			$t_fits = 1;
		    } else {
			$errmsg = "existing mappings are in the way: " .join(", ", map(
				  sprintf("0x%x..0x%x%s to %s", $_->addr, $_->endaddr,
				      $_->name ? " (".$_->name.")" : "",
				      "(" . join(", ", map($_->mem->name, $_->spans)) . ")"),
				  @$maps));
		    }
		}
	    }
	}
	FITS: {
	    if ($t_fits) {
		$t_fits = 0;		
		my $t_ref = $t_start // $t_end;
		my $s_ref = $s_start // $s_end;
		if ($t_max - $t_ref > $s_max - $s_ref) { $t_max = $t_ref + ($s_max - $s_ref); }
		if ($t_ref - $t_min > $s_ref - $s_min) { $t_min = $t_ref - ($s_ref - $s_min); }
		$errmsg = "out of min/max bounds";
		last FITS if defined($t_start) and $t_start < $t_min;
		last FITS if defined($t_end) and $t_end > $t_max;
		$errmsg = undef;
		if (!defined($t_end)) {
		    if (defined($maxsize) and $t_max - $t_start > $maxsize - 1) {
			$t_max = $t_start + ($maxsize - 1);
		    }
		    $t_end = align_down($t_max, $t_endalign, $endmask);
		    if ($t_end != $t_max and $t_max - $t_end < $t_right) {
			$t_end = align_down($t_max - $t_right, $t_endalign, $endmask);
		    }
		    if (!defined($t_end)) {
			$errmsg = sprintf("endalign=0x%x brings available end address 0x%x below zero", $t_endalign, $t_max);
			last FITS;
		    }
		    ($t_sizem1, $errmsg) = construct_size_per_min_align($construct, $t_end - $t_start,
							$t_start, "start", $log2sized, $minsizem1);
		    last FITS if defined($errmsg);
		    $t_end = $t_start + $t_sizem1;
		} elsif (!defined($t_start)) {
		    if (defined($maxsize) and $t_end - $t_min > $maxsize - 1) {
			$t_min = $t_end - ($maxsize - 1);
		    }
		    $t_start = align_up($t_min, $t_startalign, $startmask);
		    if ($t_start != $t_min and $t_start - $t_min < $t_left) {
			$t_start = align_up($t_min + $t_left, $t_startalign, $startmask);
		    }
		    if (!defined($t_start)) {
			$errmsg = sprintf("startalign=0x%x makes available start address 0x%x wrap-around", $t_startalign, $t_min);
			last FITS;
		    }
		    ($t_sizem1, $errmsg) = construct_size_per_min_align($construct, $t_end - $t_start,
							($t_end == ~0 ? 0 : $t_end + 1), "end", $log2sized, $minsizem1);
		    last FITS if defined($errmsg);
		    $t_start = $t_end - $t_sizem1;
		} else {
		    $t_sizem1 = $t_end - $t_start;
		    if ($t_sizem1 < $minsizem1) {
			$errmsg = sprintf("available space 0x%x is less than minsize=0x%x", $t_sizem1+1, $minsizem1+1);
			last FITS;
		    }
		}
		$t_fits = 1;		
	    }
	}
	if ($t_fits and !defined($errmsg)) {
	    if (defined($requirement) and construct_eval_string($construct, $requirement, $mapping, 0, "<requirement> parameter") == 0) {
		$errmsg = "requirement parameter requirement not met";
	    } elsif ($construct_type eq "segment") {
		($t_fits, $errmsg) = $mapping->cut_okay($t_start, $t_end, $minsize);
	    }
	}
	my $where = " at ".tohex($t_start)."..".tohex($t_end)
		    ." (".tohex($t_min)."..".tohex($t_max)." available)"
		    ." of ".($t_space->is_addrspace ? "space" : "memory")." ".$t_space->name;
	$where .= " (".tohex($s_start)."..".tohex($s_end)." of $spacewhat)" if $t_space ne $space;
	$where .= " (".tohex($startaddr)."..".tohex($endaddr)." of addrspace ".$addrspace->name.")" if defined($addrspace) and $addrspace ne $space;
	if (!$t_fits or defined($errmsg)) {
	    my $msg = "failed to allocate $construct_type" . $where;
	    $msg .= ": $errmsg" if defined($errmsg);
	    if (defined($prefer)) {
		construct_info($construct, $msg.", now searching for available space");
	    } else {
		construct_warn_or_error($construct, $msg);
		return;		
	    }
	}
	construct_info($construct, "$construct_type ".($name // "(unnamed)")." ".($construct_type eq "segment" ? "cut" : "mapped").$where)
	    if $t_fits;
    } else {
	$mapping = undef;
	$t_fits = 0;
    }
    my @all_mappings;		
    if ($t_fits) {
	push @all_mappings, [1, $mapping, 0, $t_start, $t_end, $t_startalign, $t_endalign, $startmask, $endmask];
    } else {			
	my $search_map;
	if ($construct_type eq "segment") {
	    $search_map = $space;
	} else {
	    $search_map = $space->extract($direct ? -1 : 1);
	}
	my ($best_mapping, $best_preference, $best, $is_best_yet);
	MAPPING:
	foreach my $mapping ($search_map->map->a) {
	    next if ($mapping->is('lock') and (!defined($ignorelock) or $mapping->is('lock') !~ m|$ignorelock|))
		 or $mapping->is('exception')
		 or $mapping->is('device')
		 or $mapping->is('delete');	
	    next if $mapping->sizem1 < $minsizem1;
	    next if defined($requirement) and construct_eval_string($construct, $requirement, $mapping, 0, "<requirement> parameter") == 0;
	    my $preference = defined($prefer)
			   ? construct_eval_string($construct, $prefer, $mapping, 0, "<prefer> parameter")
			   : $mapping->sizem1;
	    if (!defined($best_preference) or $preference > $best_preference) {
		$is_best_yet = 1;
	    } elsif ($preference == $best_preference) {
		$is_best_yet = ($mapping->default_preferred($best_mapping) < 0);
	    } else {
		$is_best_yet = 0;
	    }
	    next unless $is_best_yet or defined($partition);
	    my ($t_startalign, $startmask) = ($startalign, 0);
	    my ($t_endalign,   $endmask)   = ($endalign,   0);
	    my $what = sprintf("selected address range 0x%x..0x%x of %s", $mapping->addr, $mapping->endaddr, $spacewhat);
	    my $erralign;
	    if ($construct_type eq "segment") {
		$t_min = $mapping->addr;
		$t_max = $mapping->endaddr;
		if (defined($memory)) {
		    next unless grep($_->mem eq $memory, $mapping->spans);
		    if (defined($memory_strict)) {	
			foreach my $span ($mapping->spans) {
			    if ($span->mem eq $memory) {
				$t_min = $span->addr;
				$t_max = $span->endaddr;
				$what = sprintf("0x%x..0x%x in %s", $t_min, $t_max, $what);
				last;
			    }
			}
		    }
		}
		($t_mid, $t_left, $t_right) = $mapping->sumSections;
		$t_mid -= ($t_left + $t_right);
	    } else {
		$mapping->is_spanned and construct_internal_error($construct, "expected single entry in mapping");
		$t_space = $mapping->mem;
		$t_min = $mapping->offset;
		$t_max = $t_min + $mapping->sizem1;
		($t_left, $t_mid, $t_right) = (0, 0, 0);
		if ($t_space ne $space) {
		    $what .= sprintf(" (leading to 0x%x..0x%x of space %s)", $t_min, $t_max, $t_space->name);
		}
		($t_startalign, $startmask, $erralign) = combine_align_mask($startalign, $t_space->has_attr('startmask'),
					"startalign parameter", "startmask attribute of ".$t_space->name);
		($t_endalign, $endmask, $erralign) = combine_align_mask($endalign, $t_space->has_attr('endmask'),
					"endalign parameter", "endmask attribute of ".$t_space->name)
					if defined($t_startalign);
	    }
	    if (!defined($t_startalign) or !defined($t_endalign)) {
		construct_info($construct, "$what: skipping segment: $erralign");
		next;
	    }
	    $t_min = align_up($t_min, $t_startalign, $startmask);
	    $t_max = align_down($t_max, $t_endalign, $endmask);
	    if ($t_min > $mapping->addr and $t_min - $mapping->addr < $t_left) {
		$t_min = align_up($mapping->addr + $t_left, $t_startalign, $startmask);
	    }
	    if ($t_max < $mapping->endaddr and $mapping->endaddr - $t_max < $t_right) {
		$t_max = align_down($mapping->endaddr - $t_right, $t_endalign, $endmask);
	    }
	    $t_sizem1 = $t_max - $t_min;
	    $t_sizem1 = $maxsize - 1 if defined($maxsize) and $t_sizem1 > $maxsize - 1;
	    if ($t_max < $t_min or $t_sizem1 < $minsizem1) {
		construct_info($construct, "$what: 0x%x does not meet minimum size 0x%x", $t_sizem1+1, $minsizem1+1);
		next;
	    }
	    $t_sizem1 = $sizem1 if $size_explicit;
	    $t_fits = 0;
	    if ($t_sizem1 == ~0) {		
		$t_fits = 1;
		$t_start = $t_min;
		$t_end = $t_max;
	    } elsif ($log2sized) {
		my $max = (1 << log2($t_sizem1+1));
		if ($alloc_down) {
		    $t_end = align_down($t_max, $max);
		    $t_fits = (defined($t_end) and $t_end >= $t_min and $t_end - $t_min >= $max - 1);
		    $t_start = $t_end - ($max - 1);
		} else {
		    $t_start = align_up($t_min, $max);
		    $t_fits = (defined($t_start) and $t_start <= $t_max and $t_max - $t_start >= $max - 1);
		    $t_end = $t_start + ($max - 1);
		}
		my $errmsg;
		($t_fits, $errmsg) = $mapping->cut_okay($t_start, $t_end, $minsize)
		    if $t_fits and $construct_type eq "segment";
		if (!$t_fits and $max > 1) {
		    $max >>= 1;		
		    if ($alloc_down) {
			$t_end = align_down($t_max, $max);
			$t_start = $t_end - ($max - 1);
		    } else {
			$t_start = align_up($t_min, $max);
			$t_end = $t_start + ($max - 1);
		    }
		    $t_fits = 1;
		}
		($t_fits, $errmsg) = $mapping->cut_okay($t_start, $t_end, $minsize)
		    if $t_fits and $construct_type eq "segment";
		my $sizm1 = $t_end - $t_start;
		CHECK: {
		    if (!$t_fits and defined($errmsg)) {
			construct_info($construct, "$what: cannot naturally align to a power-of-2 size: $errmsg");
			next CHECK;
		    } elsif (!$t_fits or $sizm1 < $minsizem1) {
			construct_info($construct, "$what: cannot naturally align to a power-of-2 size: no such aligned size between minsize=0x%x and available size 0x%x", $minsizem1 + 1, $t_sizem1 + 1);
			next CHECK;
		    } elsif ($max < $t_startalign or $max < $t_endalign) {
			construct_info($construct, "$what: cannot naturally align to power-of-2 size: size 0x%x (within available size 0x%x) fits but is smaller than startalign=0x%x or endalign=0x%x", $max, $t_sizem1, $t_startalign, $t_endalign);
			next CHECK;
		    } elsif ($size_explicit and $sizm1 != $sizem1) {
			construct_info($construct, "$what: cannot naturally align to power-of-2 size: explicit size 0x%x does not fit (0x%x does)", $sizem1 + 1, $sizm1 + 1);
			next CHECK;
		    }
		    last CHECK;
		} continue {
		    $t_fits = 0;
		    next MAPPING if $log2sized == 1;
		}
	    }
	    if (!$t_fits) {
		if ($alloc_down) {
		    $t_end = $t_max;
		    $t_start = $t_end - $t_sizem1;
		} else {
		    $t_start = $t_min;
		    $t_end = $t_start + $t_sizem1;
		}
		if ($construct_type eq "segment") {
		    my $errmsg;
		    ($t_fits, $errmsg) = $mapping->cut_okay($t_start, $t_end, $minsize);
		    if (!$t_fits) {
			construct_info($construct, "$what: $errmsg");
			next MAPPING;
		    }
		} else {
		    $t_fits = 1;
		}
	    }
	    my $found = [0, $mapping, $preference, $t_start, $t_end, $t_space,
			 $t_startalign, $t_endalign, $startmask, $endmask];
	    push @all_mappings, $found;
	    if ($is_best_yet) {
		$best = $found;
		$best_mapping = $mapping;
		$best_preference = $preference;
	    }
	}
	if (!defined($best)) {
	    construct_warn_or_error($construct, "no address range was found in $spacewhat that meets specified criteria");
	    return;
	}
	(undef, $mapping, undef, $t_start, $t_end, $t_space, $t_startalign, $t_endalign, $startmask, $endmask)
	    = @$best;
	$t_sizem1 = $t_end - $t_start;
    }
    if ($construct_type eq "segment") {
	my $do_cut = ($t_start != $mapping->addr or $t_end != $mapping->endaddr);
	if (defined($partition) and !$do_cut) {
	    foreach my $lmname (sort keys %$partition) {
		my ($lm) = grep(($_->name eq $lmname or $_->name eq $lmname.".linkmap"), @$linkmaps);
		defined($lm) or construct_internal_error($construct, "did not find linkmap %s", $lmname);
		my $portion = $partition->{$lmname};
		foreach my $amap (@all_mappings) {
		    my ($m_ataddr, $m_mapping, $m_pref, $m_start, $m_end, $m_space, $m_startalign, $m_endalign) = @$amap;
		    my ($map) = $lm->findmaps($m_start, $m_end);
		    defined($map) and $map->addr <= $m_start and $map->endaddr >= $m_end
			or construct_internal_error($construct, "common segment not found in linkmap %s, at 0x%x..0x%x", $lm->name, $m_start, $m_end);
		    my $m_portion = $map->{partition_portion};
		    if (defined($m_portion)) {
			construct_error($construct, "multiple partitions given (%s, %s) for linkmap %s at 0x%x..0x%x", $m_portion->[0], $portion, $lm->name, $m_start, $m_end);
		    }
		    my $is_percent = ($portion =~ m/^(.*)\%$/);
		    $layout->dprintf("partition: apportioning %s to 0x%x..0x%x of linkmap %s (align=%d..%d)\n", $portion, $map->addr, $map->endaddr, $lmname, ($m_startalign // 1), ($m_endalign // 1));
		    $map->{partition_portion} = [$portion, $is_percent, ($is_percent ? $1 : $portion),
						 ($m_startalign // 1), ($m_endalign // 1)];
		}
	    }
	}
	my $some_literals = 0;
	if (@$linkmaps > 1 and defined($construct->{sections}) and !defined($attrs->{intent})) {
	    construct_warning($construct, "placing sections (%s) in the same segment (0x%x..0x%x) across multiple linkmaps: this is usually only done when either (a) only one of the linkmaps is known to populate that section, or (b) contents of the sections will be identical across all linkmaps and the MP program image loading mechanism allows overlapping matching contents, or (c) this is done for some kind of overlay mechanism (to turn off this warning, set the 'intent' attribute in this construct to one of 'oneof', 'matching', or 'overlay')",  join(", ", @{$construct->{sections} // []}), $t_start, $t_end);
	}
	foreach my $lm (@$linkmaps) {
	    my $save_linkmap = $construct->{_linkmap};
	    $construct->{_linkmap} = $lm;
	    ($mapping) = $lm->findmaps($t_start, $t_end) if $multiple_linkmaps;
	    ::dprintf(undef, "-- mapping BEFORE: %s\n", $mapping->dump);
	    if ($do_cut) {
		construct_info($construct, "cut segment 0x%x..0x%x out of 0x%x..0x%x",
			$t_start, $t_end, $mapping->addr, $mapping->endaddr);
		$mapping = $mapping->cut($t_start, $t_end, $keepsections);
	    } else {
		construct_info($construct, "selected segment 0x%x..0x%x", $mapping->addr, $mapping->endaddr);
	    }
	    $mapping->del_attr(qw/delete partition lock/) if $override_lock;
	    my ($somelits, undef) = construct_add_section($construct, $lm, $mapping, 0, 0);
	    $some_literals += $somelits;
	    $mapping->set_attr(%$attrs);
	    $mapping->set_attr(startalign => $t_startalign) if $t_startalign > $mapping->has_attr('startalign', 1);
	    $mapping->set_attr(endalign   => $t_endalign)   if $t_endalign   > $mapping->has_attr('endalign', 1);
	    if (defined($partition) and $do_cut) {
		my $lmname = $lm->name;  $lmname =~ s/\.linkmap$//;
		my $portion = $partition->{$lmname} // $partition->{$lmname.".linkmap"};
		if (defined($portion)) {
		    my $m_portion = $mapping->{partition_portion};
		    if (defined($m_portion)) {
			construct_error($construct, "multiple partitions given (%s, %s) for linkmap %s at 0x%x..0x%x", $m_portion->[0], $portion, $lmname, $mapping->addr, $mapping->endaddr);
		    }
		    my $is_percent = ($portion =~ m/^(.*)\%$/);
		    $layout->dprintf("partition: apportioning %s to 0x%x..0x%x of linkmap %s (align=%d..%d)\n", $portion, $mapping->addr, $mapping->endaddr, $lmname, ($t_startalign // 1), ($t_endalign // 1));
		    $mapping->{partition_portion} = [$portion, $is_percent, ($is_percent ? $1 : $portion),
						     ($t_startalign // 1), ($t_endalign // 1)];
		}
	    }
	    ::dprintf(undef, "-- mapping AFTER:  %s\n", $mapping->dump);
	    $construct->{_linkmap} = $save_linkmap;
	}
	if ($do_cut or $size_specs or defined($construct->{sections}) or scalar(keys(%$attrs))) {
	    my $cutwhat = ($description // $name // "segment cut");
	    construct_cut_mem_aliases($construct, $mapping, $cutwhat, defined($partition) ? 0 : $size_specs ? 3 : 2);
	}
	construct_schedule_step($construct, 1) if $some_literals;
    } else {
	if (!defined($target->sizem1)) {
	    $target->sizem1($t_sizem1);
	    $offset == 0 or construct_error($construct, "a dynamically sized $construct_type can only have an offset of zero");
	}
	if (defined($t_space->alwaysto) and $t_space->alwaysto ne $target) {
	    construct_error($construct, "space %s is set to map alwaysto=%s but is being mapped to %s", $t_space->name, $t_space->alwaysto->name, $target->name);
	}
	if (defined($target->alwaysin) and $target->alwaysin ne $t_space) {
	    construct_error($construct, "addressable %s is set to be mapped alwaysin=%s but is being mapped in space %s", $target->name, $target->alwaysin->name, $t_space->name);
	}
	my $alignmask = $t_space->has_attr('alignmask');
	if (($t_start & $alignmask) != ($offset & $alignmask)) {
	    construct_error($construct, "mapping from 0x%x of address space %s to offset 0x%x of %s violates minimum alignment from this address space (bits 0x%x of address and offset must match)", $t_start, $t_space->name, $offset, $target->name, $alignmask);
	}
	my $startmask = $t_space->has_attr('startmask');
	my $endmask = $t_space->has_attr('endmask');
	if (($t_start & $startmask) != 0 or ($t_end & $endmask) != $endmask) {
	    construct_error($construct, "mapping from 0x%x..0x%x (to offset 0x%x of %s) violates startmask=0x%x (bits that must be 0 in mapping start address) and/or endmask=0x%x (bits that must be 1 in mapping end address) attributes of address space %s", $t_start, $t_end, $offset, $target->name, $startmask, $endmask, $t_space->name);
	}
	if (defined($cutout_mapping)) {
	    $cutout_mapping->cutout($t_start, $t_end);
	}
	$mapping = Xtensa::System::AddressMapping->new( $t_space, $t_start,
			$mapattrs, [[$target, $offset, $t_sizem1]] );
	$mapping->{name} = $name;		
	$mapping->coalesce_mems;
	$mapping->insert;
    }
    evaluate_symbol_assignments($construct, $mapping, 0, defined($mapping));
}
sub construct_core {
    my ($construct, $check_only) = @_;
    my ($layout, $namespace, $name, $configname, $num_cores, $hw_maps,$default_maps,$sw_sections) = @{$construct}{qw/
	_layout   namespace   name   config       num_cores   hw_maps  default_maps  sw_sections /};
    construct_check_params($construct, qw/requirement assignments config num_cores hw_maps default_maps sw_sections index vecbase vecselect vecreset_pins ignore_missing_vectors/);
    if (!defined($configname)) {
	construct_error($construct, "the config parameter is required");
    }
    $num_cores //= 1;
    if ($num_cores < 1 or $num_cores > 999) {
	construct_error($construct, "the num_cores parameter must be sane (at least 1; and more than 3 digits really seems high...): %s", $num_cores);
    }
    my $ns = (defined($namespace) and $namespace ne "") ? $namespace."." : "";
    $name //= "core#";
    my $namerooted = ($name =~s/^\.//);
    my $index         = construct_eval($construct, "index",         undef, $check_only);
    my $vecbase       = construct_eval($construct, "vecbase",       undef, $check_only);
    my $vecselect     = construct_eval($construct, "vecselect",     undef, $check_only);
    my $vecreset_pins = construct_eval($construct, "vecreset_pins", undef, $check_only);
    my $ignore_missing_vectors = construct_eval($construct, "ignore_missing_vectors", undef, $check_only);
    my $requirement   = construct_eval($construct, "requirement",   1,     $check_only);
    if ($check_only) {
	evaluate_symbol_assignments($construct, undef, 1, 1);
	construct_default_ordering($construct, ['create'], ['creation']);
	return;
    }
    if (!$requirement) {
	construct_warn_or_error($construct, "<requirement> test failed");
	return;
    }
    $index //= $layout->find_core_index($num_cores);
    my $nameindex = $index;
    my $len = 0;
    if ($name =~ /(\#+)[^\#]*$/) {
	$len = length($1);
    } elsif ($num_cores > 1 and $name =~ s/(0*)(\d+?)$//) {
	$len = length($1.$2);
	$nameindex = $2 + 0;
	$name .= "#";
    }
    my $config = Component::XtensaConfig->new($layout, $configname);
    foreach my $i (0 .. $num_cores - 1) {
	my $thisname = $name;
	if ($len > 0) {
	    $thisname =~ s/(\#+)([^\#]*)$/sprintf("%0${len}u", $nameindex).$2/e;
	} elsif ($num_cores > 1) {
	    $thisname .= $nameindex;
	}
	$thisname = $ns.$thisname unless $namerooted;	
	my $swoptions = { vecbase => $vecbase, vecselect => $vecselect, vecreset_pins => $vecreset_pins };
	my $core = Component::XtensaCore->new($layout, $thisname, $index, $config, $swoptions);
	$core->{hw_maps} = $hw_maps;
	$core->{default_maps} = $default_maps;
	$core->{sw_sections} = $sw_sections;
	$core->{ignore_missing_vectors} = $ignore_missing_vectors;
	$nameindex++;
	$index++;
    }
    evaluate_symbol_assignments($construct, undef, 0, 1);
}
sub construct_linkmap {
    my ($construct, $check_only) = @_;
    my ($layout, $namespace, $name, $corenames, $vectors, $span_okay) = @{$construct}{qw/
	_layout   namespace   name   cores       vectors   span_okay /};
    construct_check_params($construct, qw/requirement assignments cores full_image vectors span_okay/);
    if (!defined($corenames)) {
	construct_error($construct, "the cores parameter is required");
    }
    if (ref($corenames) ne "ARRAY" or !@$corenames) {
	construct_error($construct, "the cores parameter must be a non-empty array");
    }
    if (defined($vectors)) {
	if (ref($vectors) ne "ARRAY") {
	    construct_error($construct, "the vectors parameter must be an array");
	}
	if (array_extract { $_ eq "none" } @$vectors) {
	    @$vectors and construct_error($construct, "vectors: 'none' cannot be combined with other vector specifiers");
	} else {
	    foreach my $vecspec (@$vectors) {
		next unless $vecspec =~ m/^(.*\.)none$/;
		my $coreprefix = $1;
		my @coreprefixed = grep((length($_) >= length($coreprefix) && substr($_, 0, length($coreprefix)) eq $coreprefix), @$vectors);
		my @notnone = grep($_ ne $vecspec, @coreprefixed);
		@notnone and construct_error($construct, "vectors: $vecspec cannot be combined with other ${coreprefix}*: %s", join(", ", @notnone));
	    }
	}
    }
    if (!defined($name) and @$corenames > 1) {
	construct_error($construct, "the name parameter is required if multiple cores are specified");
    }
    my $ns = (defined($namespace) and $namespace ne "") ? $namespace."." : "";
    $name = $ns.$name unless $name =~ s/^\.//;	
    my $full_image  = construct_eval($construct, "full_image", undef, $check_only);
    my $requirement = construct_eval($construct, "requirement", 1, $check_only);
    if ($check_only) {
	evaluate_symbol_assignments($construct, undef, 1, 1);
	construct_default_ordering($construct, ['create'], ['linkmap']);
	return;
    }
    if (!$requirement) {
	construct_warn_or_error($construct, "<requirement> test failed");
	return;
    }
    my ($core_names, $oops) = $layout->names_of_cores(@$corenames);
    if (@$oops) {
	construct_warn_or_error($construct, "cores parameter: unrecognized core or set name(s): ".join(", ", @$oops));
	return;
    }
    my $cores = [ map($layout->cores_byname->{$_}, @$core_names) ];
    $name = $corenames if !defined($name);
    if (grep($_->{name} eq $name, @{$layout->{requested_linkmaps}})) {
	construct_warn_or_error($construct, "multiple linkmaps named '$name' requested");
	return;
    }
    my $request = {name => $name, cores => $cores};
    $request->{span_okay} = $span_okay if defined($span_okay);
    $request->{full_image} = $full_image if defined($full_image);
    $request->{vectors} = $vectors if defined($vectors);
    push @{$layout->{requested_linkmaps}}, $request;
    evaluate_symbol_assignments($construct, undef, 0, 1);
}
sub construct_named_set {
    my ($construct, $check_only) = @_;
    my ($layout, $namespace, $name, $corenames) = @{$construct}{qw/
	_layout   namespace   name   cores /};
    construct_check_params($construct, qw/cores/);
    if (!defined($corenames)) {
	construct_error($construct, "the cores parameter is required");
    }
    if (ref($corenames) ne "ARRAY" or !@$corenames) {
	construct_error($construct, "the cores parameter must be a non-empty array");
    }
    if (!defined($name)) {
	construct_error($construct, "the name parameter is required");
    }
    if ($check_only) {
	construct_default_ordering($construct, ['create'], ['creation']);
	return;
    }
    my ($core_names, $oops) = $layout->names_of_cores(@$corenames);
    if (@$oops) {
	construct_warn_or_error($construct, "cores parameter: unrecognized core or set name(s): ".join(", ", @$oops));
	return;
    }
    my ($existingname, $oops2) = $layout->names_of_cores($name);
    if (@$existingname) {
	construct_warn_or_error($construct, "name parameter: a set or core named '%s' already exists", $name);
	return;
    }
    $layout->ref_core_set_names->{$name} = $core_names;;
}
sub construct_partition {
    my ($construct, $check_only) = @_;
    construct_check_params($construct, qw//);
    if ($check_only) {
	construct_default_ordering($construct, ['partition'], ['partition']);
	return;
    }
    my $layout = $construct->{_layout};
    $layout->apply_partitions($construct);
}
my %construct_subs = (
    "space"    => \&construct_addressed,
    "memory"   => \&construct_addressed,
    "map"      => \&construct_addressed,
    "segment"  => \&construct_addressed,
    "_partition"=> \&construct_partition,
    "core"     => \&construct_core,
    "linkmap"  => \&construct_linkmap,
    "named_set"=> \&construct_named_set,
);
sub construct_full_symbols {
    my ($construct, $symbol) = @_;
    my $linkmaps = $construct->{linkmaps};
    my @syms;
    if ($symbol =~ /^\.(.*)$/) {
	@syms = ($1);		
    } elsif ($construct->{namespace}) {
	@syms = ($construct->{namespace}.".".$symbol);
    } elsif (defined($linkmaps) && @$linkmaps > 0) {
	foreach (@$linkmaps) {
	    my $name = $_->name;
	    $name =~ s/\.linkmap$//;
	    push @syms, $name.".".$symbol;
	}
    } else {
	@syms = ($symbol);
    }
    return @syms;
}
sub evaluate_symbol_assignments {
    my ($construct, $seg, $check_only, $condition) = @_;
    my $assignments = $construct->{assignments};
    return unless $condition and defined($assignments);
    if (ref($assignments) ne "HASH") {
	construct_error($construct, "assignments parameter must be a set (hash) of symbol=value pairs");
    }
    my $layout = $construct->{_layout};
    my $namespace = $construct->{namespace};
    my $linkmaps = $construct->{linkmaps};
    $linkmaps //= [ $construct->{_linkmap} ];
    my $lmnames = join("+", map($_->name, @$linkmaps)).",".($construct->{name} // "?");
    my @cores = map($layout->cores_byname->{$_}, @{ $construct->{cores} // [] });
    if (!@cores) {
	foreach my $core (map(@{$_->cores}, @$linkmaps)) {
	    push @cores, $core unless grep($_ eq $core, @cores);
	}
    }
    my $ns = (defined($namespace) and $namespace ne "") ? $namespace."." : "";
    my $symbols = $layout->{symbols};
    my $symbol_constructs = $layout->{symbol_constructs};
    foreach my $symbol (sort keys %$assignments) {
	my $value = $assignments->{$symbol};
	my $is_swoption = ($symbol =~ /^sw\.(\w+)$/ and @cores);
	my $swopt = $is_swoption ? $1 : "";
	my @syms = $is_swoption ? ($symbol) : construct_full_symbols($construct, $symbol);
	my $showsymbol = join(", ", @syms);
	my $new_value = construct_eval_string($construct, $value, $seg, $check_only, "assignment to <$showsymbol>");
	if ($check_only) {
	    foreach (@syms) { push @{$symbol_constructs->{$_}}, (); }	
	    next;
	}
	$layout->dprintf("... MULTIPLE Assignments to: $showsymbol\n") if @syms > 1;
	my $new_show = ($new_value =~ /^\d+$/) ? sprintf("0x%x", $new_value) : "'$new_value'";
	foreach my $fullsymbol (@syms) {
	    $layout->dprintf("... (%s) Assigning '%s' = '%s' (= '%s')%s\n", $lmnames, $fullsymbol, $value, $new_show,
			$is_swoption ? " on cores ".join(",", map($_->name, @cores)) : "");
	    if ($is_swoption) {
		foreach (@cores) { $_->swoptions->{$swopt} = $new_value; }
	    } else {
		my $old_value = $symbols->{$fullsymbol};
		$symbols->{$fullsymbol} = $new_value;
		if (defined($old_value)) {
		    if ($old_value ne $new_value) {
			construct_error($construct, "multiply defined symbol '$fullsymbol' changed value from '$old_value' to '$new_show'");
		    } else {
			construct_warning($construct, "multiply defined symbol '$fullsymbol' to the same value '$new_show'");
		    }
		}
	    }
	    if (defined($symbol_constructs->{$fullsymbol})) {
		my $constructs = delete $symbol_constructs->{$fullsymbol};
		$layout->dprintf("... (%s) Re-executing %d constructs for symbol '%s'\n",
				$lmnames, scalar(@$constructs), $fullsymbol);
		foreach my $waiting_construct (@$constructs) {
		    execute_construct($waiting_construct);
		}
	    }
	}
    }
}
sub execute_construct {
    my ($construct) = @_;
    my ($layout, $linkmap, $needsyms, $construct_type) = @{$construct}{qw/
	_layout  _linkmap  _needsyms   construct /};
    if (defined($needsyms)) {
	my $symbols = $layout->{symbols};
	my $symbol_constructs = $layout->{symbol_constructs};
	foreach my $symbol (sort keys %$needsyms) {
	    next if $symbol =~ /^[\@\$]/;		
	    my $symcons = $symbol_constructs->{$symbol};
	    next if $symbol =~ /^sw\./ and (!defined($symcons) or defined(($construct->{assignments} // {})->{$symbol}));
	    my @fullsymbols = construct_full_symbols($construct, $symbol);
	    foreach my $fullsymbol (@fullsymbols) {
		if (!exists($symbols->{$fullsymbol})) {
		    push @{ $symbol_constructs->{$fullsymbol} }, $construct;
		    my $linkmapname = defined($construct->{_linkmap}) ? $construct->{_linkmap}->name
				    : defined($construct->{linkmaps}) ? join("+",map($_->name,@{$construct->{linkmaps}}))
				    : "-";
		    $layout->dprintf("... (%s) Deferring construct (%s, %s) for symbol %s\n",
			    $linkmapname, $construct_type, ($construct->{name} // "(unnamed)"), $fullsymbol);
		    return;
		}
	    }
	}
    }
    my ($showpre,$showpost) = construct_synopsis($construct);
    $layout->dprintf("\$  EXECUTING  %s\n", $showpre.$showpost);
    my $sub = $construct_subs{$construct_type};
    my $err = call_sub($sub, $construct, 0);
    if ($err) {
	$construct->{_internal} = $err->[1];
	construct_error($construct, $err->[0]);
    }
#$layout->dprintf("##### DUMP:  %s #####\n", $linkmap->dump);
}
my %ordering_names = (
    creation => 10,		
    map_at => 20,		
    space_at => 25,		
    memory_at => 30,		
    map_scan => 40,		
    space_scan => 45,		
    memory_scan => 50,		
    linkmap => 60,		
    cut_at => 70,		
    partition => 75,		
    cut_try => 80,		
    default => 82,		
    cut_scan => 85,		
    no_cut => 90,		
    );
sub construct_parse_order {
    my ($construct, $order_string, $default, $what) = @_;
    my $head_delta = 0;
    $head_delta = -1 if $order_string =~ s/\.head$//;	
    my $is_delta = $order_string =~ s/^\+//;
    my $value = construct_eval_string($construct, $order_string, \%ordering_names, 0, $what);
    if ($value < 0 or $is_delta) {
	$value += $default;
	$value = 0 if $value < 0;
	$value = 99 if $value > 99;
    }
    return $value if $value >= 0 and $value <= 99;
    construct_error($construct, "$what, $order_string = $value which is outside valid range 0-99");
}
sub construct_default_ordering {
    my ($construct, $steps, $default_ordering) = @_;
    my $ordering = $construct->{ordering};
    $ordering = [$ordering] unless ref($ordering) eq "ARRAY";
    foreach my $i (0 .. $#$default_ordering) {
	my $what = "step ".($i+1)." of ".scalar(@$default_ordering);
	my $default = construct_parse_order($construct, $default_ordering->[$i], 0, "<ordering> default, $what");
	if (defined($ordering->[$i])) {
	    $ordering->[$i] = construct_parse_order($construct, $ordering->[$i], $default, "<ordering> parameter, $what");
	} else {
	    $ordering->[$i] = $default;
	}
    }
    $construct->{ordering} = $ordering;
    $construct->{_steps} = $steps;
    $construct->{_step} = 0;		
    $construct->{_order} = $ordering->[0];	
}
sub dump_value {
    my ($value, $depth) = @_;
    if (ref($value) eq "HASH" and $depth > 0) {
	return "{".join(", ", map("$_ = ".dump_value($value->{$_}, $depth-1), sort keys %$value))."}";
    } elsif (ref($value) eq "ARRAY" and $depth > 0) {
	return "[".join(", ", map(dump_value($_, $depth-1), @$value))."]";
    } else {
	return "(undef)" if !defined($value);
	return sprintf("0x%x", $value+0) if $value =~ /^[1-9]\d*$/;
	return $value;
    }
}
sub construct_synopsis {
    my ($construct) = @_;
    return ("", "") unless defined($construct);
    my ($construct_type, $name, $linkmap,$linkmaps, $saveline, $lineno, $ordering, $description, $step, $steps ) = @{$construct}{qw/
	 construct        name  _linkmap  linkmaps  _saveline  _lineno   ordering   description  _step  _steps /};
    my $showpre = "";
    $showpre .= "(";
    my $comma = ", ";
    if (defined($linkmap) and $linkmap->name ne "") {
	$showpre .= $linkmap->name;
    } elsif (defined($linkmaps) and @$linkmaps > 0) {
	$showpre .= join("+", map($_->name, @$linkmaps));
    } else {
	$comma = "";
    }
    $showpre .= $comma . ($name // "?");
    $showpre .= ") construct";
    $showpre .= " $construct_type" if defined($construct_type);
    $showpre .= " (step $step \[".$steps->[$step]."\] order ".$ordering->[$step].")"
	if defined($ordering) and defined($step);
    $showpre .= ": ";
    $showpre .= "$description: " if defined($description);
    my $showpost = "";
    if (defined($lineno) or defined($saveline)) {
	$showpost .= "\n   ";
	$showpost .= defined($lineno) ? "At line $lineno" : "Line";
	$showpost .= ": $saveline" if defined($saveline);
    }
    my @ksort = qw/name namespace linkmap shared cores space direct alwaysin addrspace startaddr endaddr startalign endalign size sizem minsize maxsize log2sized prefer requirement memory_byaddr memory memory_strict alwaysto addressable offset mapattrs attrs sections keepsections partition contains optional ignorelock assignments/;
    my %ksort = map { $ksort[$_] => sprintf("%03u",$_) } 0 .. $#ksort;
    foreach (sort {($ksort{$a}//$a) cmp ($ksort{$b}//$b)} keys %$construct) {
	next if /^_/ or /^construct|ordering|description|linkmaps/;
	$showpost .= "\n   $_ = ";
	my $value = $construct->{$_};
	$showpost .= dump_value($value, 2);
    }
    return ($showpre, $showpost);
}
{
  package Xtensa::System::Exception;
  our $check;
}
sub Error {
    ErrorReport(0, @_);
}
sub InternalError {
    ErrorReport(1, @_);
}
sub ErrorReport {
    my ($internal, $format, @args) = @_;
    my $msg = sprintf($format, @args);
    if ($Xtensa::System::Exception::check) {
	if ($Xtensa::AddressLayout::global_debug) {
	    local $@; require Carp;
	    Carp::cluck($msg);
	}
	die bless([$msg, $internal], "Xtensa::System::Exception");
    } else {
	die (($internal ? "INTERNAL " : "")."ERROR: $msg");
    }
}
sub construct_internal_error {
    my ($construct, $format, @args) = @_;
    $construct->{_internal} = 1;
    construct_error($construct, $format, @args);
}
sub construct_error {
    my ($construct, $format, @args) = @_;
    my ($showpre, $showpost) = construct_synopsis($construct);
    my $msg = $showpre . sprintf($format, @args) . $showpost;
    my $layout = $construct->{_layout};
    my $linkmap = $construct->{_linkmap};
    my $prefix = $construct->{_internal} ? "INTERNAL " : "";
    $layout->dprintf("${prefix}ERROR: %s\n", $msg) if defined($layout);
    $layout->dprintf("${prefix}ERROR: linkmap dump:\n%s", $linkmap->dump)
	if defined($layout) and defined($linkmap);
    die "${prefix}ERROR: $msg";
}
sub construct_warning {
    my ($construct, $format, @args) = @_;
    my ($showpre, $showpost) = construct_synopsis($construct);
    my $layout = $construct->{_layout};
    $layout->warn("%s%s%s", $showpre, sprintf($format, @args), $showpost);
}
sub construct_info {
    my ($construct, $format, @args) = @_;
    my ($showpre, $showpost) = construct_synopsis($construct);
    my $layout = $construct->{_layout};
    $layout->info("%s%s%s", $showpre, sprintf($format, @args), $showpost);
}
sub construct_warn_or_error {
    my ($construct, $format, @args) = @_;
    my $optional = $construct->{optional} // 0;
    if ($optional == 0) {
	construct_error($construct, $format, @args);
    } elsif ($optional == 2) {
	construct_info($construct, $format, @args);
    } else {
	construct_warning($construct, $format, @args);
    }
}
sub call_sub {
    my ($sub, @params) = @_;
    $Xtensa::System::Exception::check++;
    eval {
	&$sub(@params);
    };
    $Xtensa::System::Exception::check--;
    if ($@) {
	return $@ if ref($@) eq "Xtensa::System::Exception";
	die $@;
    }
    undef;
}
sub schedule_construct {
    my ($construct) = @_;
    my ($showpre,$showpost) = construct_synopsis($construct);
    my ($layout, $order) = @{$construct}{qw/_layout _order/};
    $layout->dprintf("\$  SCHEDULING %s\n", $showpre);
    defined($order) or construct_internal_error($construct, "construct did not set its scheduling order");
    my $ordered_constructs = $layout->ref_ordered_constructs;
    push @{$ordered_constructs->[99 - $order]}, $construct;
}
sub construct_schedule_step {
    my ($construct, $step) = @_;
    $construct->{_step} = $step;
    $construct->{_order} = $construct->{ordering}->[$step];
    construct_info($construct, "scheduling step $step order ".$construct->{_order} // "(oops)");
    schedule_construct($construct);
}
sub assign_constructs_to_linkmap {
    my ($linkmap, $constructs) = @_;
    foreach my $construct (@$constructs) {
	$construct->{_linkmap} = $linkmap;
    }
}
sub assign_constructs_to_namespace {
    my ($layout, $namespace, $constructs) = @_;
    return unless defined($namespace);
    my $renames = {};
    my $parameters;
    if (ref($namespace) eq "HASH") {
	$renames = $namespace;
	$namespace = delete $renames->{_namespace};
	$parameters = (delete $renames->{_parameters}) // ["space", "addrspace", "alwaysto", "alwaysin", "addressable"];
    }
    return if $namespace eq "" and scalar(keys %$renames) == 0;
    foreach my $construct (@$constructs) {
	if ($namespace ne "") {
	    my $prev_ns = ($construct->{namespace} // "");
	    if ($prev_ns !~ /^\./) {			
		$construct->{namespace} = $namespace . (($prev_ns ne "") ? ".$prev_ns" : "");
	    }
	    if ($construct->{construct} eq "segment"
		and !defined($construct->{linkmaps}) and !defined($construct->{shared}) and !defined($construct->{cores})) {
		$construct->{shared} = [$namespace];	
	    }
	}
	foreach my $parm (@$parameters) {
	    my $pvalue = $construct->{$parm};
	    if (defined($pvalue)) {
		my $replacement = $renames->{$pvalue};
		if (defined($replacement)) {
		    $construct->{$parm} = $replacement;
		}
	    }
	}
    }
}
sub construct_dup {
    my (@constructs) = @_;
    my @out;
    foreach my $construct (@constructs) {
	my $copy = { %$construct };		
	foreach my $p (keys %$copy) {
	    my $value = $copy->{$p};
	    if (ref($value) eq "ARRAY") {
		$copy->{$p} = [ @$value ];
	    } elsif (ref($value) eq "HASH") {
		$copy->{$p} = { %$value };
	    }
	}
	push @out, $copy;
    }
    @out;
}
sub apply_constructs {
    my ($layout, $constructs_ref, $software, $default_linkmap) = @_;
    my %named;
    foreach my $construct (@$constructs_ref) {
	$construct->{_layout} = $layout;
	if (my $name = $construct->{name}) {
	    $name = $construct->{namespace} . ".". $name if $construct->{namespace};
	    $named{$name} and construct_error($construct, "duplicate construct named '$name'");
	    $named{$name} = $construct;
	}
    }
    my @constructs;
    if ($software) {
	@constructs = grep( $_->{construct} =~ m/^segment|linkmap|_partition$/, @$constructs_ref );
	foreach my $construct (@constructs) {
	    next unless $construct->{construct} eq "segment";	
	    my $shared = $construct->{shared};
	    if (defined($shared)) {
		foreach my $linkmap_name (@$shared) {
		    my $linkmap = $layout->{linkmaps_byname}{$linkmap_name};
		    defined($linkmap) or construct_error($construct, "did not find linkmap '$linkmap_name' specified in 'shared' parameter");
		    push @{$construct->{linkmaps}}, $linkmap unless grep($_ eq $linkmap, @{$construct->{linkmaps}});
		}
	    }
	    my $corenames = $construct->{cores};
	    if (defined($corenames) and @$corenames) {
		my ($core_names, $oops) = $layout->names_of_cores(@$corenames);
		@$oops and construct_error($construct, "unrecognized core or set name(s) in 'cores' parameter: ".join(", ", @$oops));
		my %coresbyname = map { $_ => 1 } @$core_names;
		foreach my $linkmap (@{$layout->{linkmaps}}) {
		    next if grep(!exists($coresbyname{$_->name}), @{$linkmap->cores});	
		    push @{$construct->{linkmaps}}, $linkmap unless grep($_ eq $linkmap, @{$construct->{linkmaps}});
		}
	    }
	    my $partition = $construct->{partition};
	    if (defined($partition)) {
		foreach my $linkmap_name (keys %$partition) {
		    next if $linkmap_name eq "*";
		    my $linkmap = $layout->{linkmaps_byname}{$linkmap_name};
		    defined($linkmap) or construct_error($construct, "did not find linkmap '$linkmap_name' specified in 'partition' parameter");
		    push @{$construct->{linkmaps}}, $linkmap unless grep($_ eq $linkmap, @{$construct->{linkmaps}});
		}
	    }
	    if (!defined($construct->{_linkmap})) {
		my @linkmaps = @{$construct->{linkmaps} // []};
		if (@linkmaps == 1) {
		    $construct->{_linkmap} = $linkmaps[0];
		} elsif (@linkmaps) {
		} elsif (defined($default_linkmap)) {
		    $construct->{_linkmap} = $default_linkmap;
		} elsif (@{$layout->{linkmaps}} == 1) {
		    $construct->{_linkmap} = $layout->{linkmaps}->[0];
		} else {
		    construct_error($construct, "no linkmap specified for segment construct");
		}
	    }
	}
    } else {
	@constructs = grep( $_->{construct} !~ m/^segment|linkmap|_partition$/, @$constructs_ref );
    }
    my @constructs_copy = @constructs;
    @constructs = ();
    foreach my $construct (@constructs_copy) {
	my $memory = $construct->{memory};
	if (defined($memory) and $memory eq "*") {
	    if (exists($construct->{assignments})) {
		construct_error($construct, "cannot duplicate assignments with memory='*'");
	    }
	    my $linkmap = $construct->{_linkmap};
	    foreach my $mem ($linkmap->memories) {
		my $memname = $linkmap->uniqname($mem);
		if (!defined($layout->find_addressable($mem->name))) {
		    #$layout->dprintf("##** Skipping memory '$memname' (not found)\n");
		    next;
		}
		my ($newcons) = construct_dup($construct);	
		$newcons->{memory} = ".".$mem->name;
		if (exists($newcons->{sections})) {
		    foreach (@{$newcons->{sections}}) { s/MEMORY/$memname/g; }
		}
		if (defined($newcons->{description})) {
		    $newcons->{description} =~ s/MEMORY/$memname/g;
		}
		push @constructs, $newcons;
	    }
	} else {
	    push @constructs, $construct;
	}
    }
    #  $layout->dprintf("######### Layout Statements:\n");
    #  $layout->dprintf("######### Done\n");
    $layout->set_symbol_constructs({});	
    my $ordered_constructs = $layout->ref_ordered_constructs;
    @$ordered_constructs = ();		
    foreach my $construct (@constructs) {
	my $construct_type = $construct->{construct};
	defined($construct_type) or construct_error($construct, "missing construct name");
	my $sub = $construct_subs{$construct_type};
	defined($sub) or construct_error($construct, "unrecognized construct '$construct_type'");
my ($showpre,$showpost) = construct_synopsis($construct); $layout->dprintf("\$  CHECKING %s\n", $showpre);
	my $err = call_sub($sub, $construct, 1);
	if ($err) {
	    $construct->{_internal} = $err->[1];
	    construct_error($construct, $err->[0]);
	}
	schedule_construct($construct);
    }
    $layout->set_symbols({});		
    while (@$ordered_constructs) {
	my $construct_queue = $ordered_constructs->[$#$ordered_constructs];
	if (!defined($construct_queue) or ! @$construct_queue) {
	    pop @$ordered_constructs;
	    next;
	}
	my $construct = shift @$construct_queue;
	execute_construct($construct);
    }
    my @missing = sort keys %{$layout->{symbol_constructs}};
    if (@missing) {
	Error( "some construct(s) could not complete because the following symbols were not defined: "
		.join(", ", @missing)
		);
    }
#$layout->dprintf("##### DUMP after apply_constructs:  %s #####\n", $linkmap->dump);
}
sub check_type {
    my ($value, $force_string) = @_;
    return ($force_string ? "''" : "0") unless defined($value);
    my $is_string = ($force_string or ($value !~ /^(0[0-7]*|0x[0-9a-fA-F]+|[1-9][0-9]*|)$/));
    if ($is_string) {
	$value = hex_encode($value);
	return "'$value'";
    } else {
	$value = "0" if $value eq "";
	return $value;
    }
}
sub construct_eval_identifier {
    my ($construct, $seg, $ident, $check_only, $what, $orig_expr) = @_;
    my $layout = $construct->{_layout};
    return 0 if $ident =~ /^0+[kMGTP]?$/i or $ident =~ /^0x0+$/i;;	
    my $maxint = ~0;
    my $maxbits = ($maxint == 0xFFFFFFFF ? 32 : 64);
    if ($ident =~ /^0+([1-7][0-7]*)$/) {			
	construct_error($construct, "$what: octal constant overflow, must fit within $maxbits bits: $ident")
	    if length($1)*3 > $maxbits;
	return oct($ident);
    }
    if ($ident =~ /^0x0*([1-9a-f][0-9a-f]*)$/i) {		
	construct_error($construct, "$what: hex constant overflow, must fit within $maxbits bits: $ident")
	    if length($1)*4 > $maxbits;
	return hex($ident);
    }
    if ($ident =~ /^([1-9][0-9]*)([kMGTP]?)$/i) {	
	my ($n, $unit) = ($1, $2);
	my $bits;
	if (length($n) > length($maxint) or length($n) == length($maxint) && $n gt "".$maxint) {
	    $bits = 999;		
	} else {
	    $unit = "" unless defined($unit);
	    my $ubits = { ""=>0, "K"=>10, "M"=>20, "G"=>30, "T"=>40, "P"=>50 }->{"\U$unit"};
	    $bits = log2($n) + 1 + $ubits;
	    $n *= (1 << $ubits);
	}
	construct_error($construct, "$what: decimal constant overflow, must fit within $maxbits bits ($bits > $maxbits or $n > $maxint): $ident")
	    if $bits > $maxbits;
	return $n;
    }
    if (defined($seg) and ref($seg) eq "HASH" and exists($seg->{$ident})) {
	return check_type($seg->{$ident});
    }
    $construct->{_needsyms}{$ident} = 1 if $check_only;
    my $linkmap = $construct->{_linkmap};
    my $cores = $construct->{cores} ? [ map($layout->cores_byname->{$_}, @{$construct->{cores}}) ]
		: defined($linkmap) ? $linkmap->cores : [];
    if ($ident =~ m/^sw\.(\w+)$/) {
	@$cores or construct_error($construct, "$what: $ident: no cores targeted by this construct");
	my @oops = grep(!exists($_->swoptions->{$1}), @$cores);
	@oops and construct_error($construct, "$what: $ident: some cores don't support that option: ".join(", ", map($_->name, @oops)));
	my $swopt = $1;
	my @values = map($_->swoptions->{$swopt}, @$cores);
	my $value = shift @values;
	if (grep($_ ne $value, @values)) {
	    construct_error($construct, "$what: $ident: core software option '$swopt' does not have the same setting (".join(",",$value,@values).") for all cores (".join(",",map($_->name,@$cores)).") of linkmap ".$linkmap->name);
	}
	return check_type($value);
    }
    my $namespace = $construct->{namespace};
    my $ns = (defined($namespace) and $namespace ne "") ? $namespace."." : "";
    if ($ident =~ s/^\.//) {
    } elsif ($ident =~ /^[\@\$]/) {
    } elsif ($ident =~ /\./) {
	$ident = $ns.$ident;
    } else {
	$ident = $ns.$ident;		
    }
    my $value;
    my $symbols = $layout->{symbols};
    if (defined($symbols) and exists($symbols->{$ident})) {
	return check_type($symbols->{$ident});
    }
    my $is_seg = (defined($seg) and UNIVERSAL::isa($seg, "Xtensa::System::AddressMapping"));
    if ($ident =~ s/^\@//) {
	my $attrs_ref = $is_seg ? $seg->ref_attributes : {};
	my $value = exists($attrs_ref->{$ident}) ? $attrs_ref->{$ident} : undef;
	return check_type($value, ($ident eq "type"));
    }
    if ($ident =~ /^\$/ and $is_seg) {
	return $seg->addr     if $ident eq "\$start";
	return $seg->endaddr  if $ident eq "\$end";
	return $seg->sizem1+1 if $ident eq "\$size";
	return $seg->sizem1   if $ident eq "\$sizem1";
	return $seg->leftTacked?1:0   if $ident eq "\$leftTacked";
	return $seg->rightTacked?1:0  if $ident eq "\$rightTacked";
	my ($minsize, $minleft, $minright, $createsize, $minaddr, $maxendaddr) = $seg->sumSections;
	return $minsize    if $ident eq "\$minsize";
	return $minleft    if $ident eq "\$minleft";
	return $minright   if $ident eq "\$minright";
	return $createsize if $ident eq "\$createsize";
	return $minaddr    if $ident eq "\$minaddr";
	return $maxendaddr if $ident eq "\$maxendaddr";
    }
    return $ident if $check_only;
    construct_error($construct, "$what: invalid identifier or constant ($ident) in expression: $orig_expr");
}
sub int_op {
    my ($a, $op, $b, $c) = @_;
    $a += 0;
    $b += 0;
    if    ($op eq '!')	{return( $b ? 0 : 1 );}
    elsif ($op eq '~')	{return( ~$b ); }
    elsif ($op eq '*')	{return( $a * $b ); }
    elsif ($op eq '/')	{return( $a / $b ); }
    elsif ($op eq '&')	{return( $a & $b ); }
    elsif ($op eq '|')	{return( $a | $b ); }
    elsif ($op eq '+')	{return( $a + $b ); }
    elsif ($op eq '-')	{return( $a - $b ); }
    elsif ($op eq '>')	{return( ($a >  $b) ?1:0 ); }
    elsif ($op eq '<')	{return( ($a <  $b) ?1:0 ); }
    elsif ($op eq '>=')	{return( ($a >= $b) ?1:0 ); }
    elsif ($op eq '<=')	{return( ($a <= $b) ?1:0 ); }
    elsif ($op eq '==')	{return( ($a == $b) ?1:0 ); }
    elsif ($op eq '!=')	{return( ($a != $b) ?1:0 ); }
    elsif ($op eq '&&')	{return( ($a && $b) ?1:0 ); }
    elsif ($op eq '||')	{return( ($a || $b) ?1:0 ); }
    elsif ($op eq '?')	{return( $a ? $b : $c ); }
    return undef;
}
sub re_match {
    my ($str, $regexp) = @_;
    return 1 if $regexp eq "";
    return( ($str =~ m/$regexp/) ? 1 : 0 );
}
sub hex_encode {
    local $_ = shift;
    s/(.)/sprintf("%02x",ord($1))/ge;
    $_;
}
sub hex_decode {
    local $_ = shift;
    s/([0-9a-fA-F][0-9a-fA-F])/chr(oct("0x$1"))/ge;
    $_;
}
sub construct_eval_string {
    my ($construct, $expr, $seg, $check_only, $what) = @_;
    my $orig_expr = $expr;		
    if (!defined($expr)) {
	return if $check_only;
	construct_error($construct, "$what: missing expression");
    }
    if (ref($expr) ne "SCALAR" and ref($expr) ne "") {
	construct_error($construct, "$what: expression must be a string, found a ".ref($expr));
    }
    my $norm = "";
    while ($expr ne "") {
	if ($expr =~ s/^'([^']*)'//) {		
	    $norm .= "'".hex_encode($1)."'";	
	} elsif ($expr =~ s/^([^']+)//) {
	    local $_ = $1;
	    s/(\.\s*)+(\w+)/.$2/g;
	    s/(\w+)\s*\.(\w+)/$1.$2/g;
	    s/(\w+)\s+(\w+)/$1.$2/g;
	    s/\s+//g;
	    s/([\.\$\@]?\w+)(\.(\w+))*(?![\$\@a-zA-Z0-9_.])/construct_eval_identifier($construct, $seg, $&, $check_only, $what, $orig_expr)/eg;
	    $norm .= $_;
	} else {
	    construct_error($construct, "$what: unterminated quote in expression: $expr");
	}
    }
    $expr = $norm;
    my $moredone = 1;
    while ($moredone) {
	$moredone = 0;
	$moredone = 1 if $expr =~ s/\((-?\d+)\)/$1/g;
	$moredone = 1 if $expr =~ s/\(('\w*')\)/$1/g;
	next if $moredone;
	$moredone = 1 if $expr =~ s/\!(-?\d+)/int_op(0,'!',$1)/eg;
	$moredone = 1 if $expr =~ s/\!'(\w*)'/$1 ne ""?0:1/eg;
	$moredone = 1 if $expr =~ s/\~(-?\d+)/int_op(0,'~',$1)/eg;	
	next if $moredone;
	$moredone = 1 if $expr =~ s/(\d+)\*(-?\d+)/int_op($1,'*',$2)/eg;
	$moredone = 1 if $expr =~ s|(\d+)/(-?\d+)|int_op($1,'/',$2)|eg;
	next if $moredone;
	$moredone = 1 if $expr =~ s/(\d+)&(-?\d+)/int_op($1,'&',$2)/eg;
	next if $moredone;
	$moredone = 1 if $expr =~ s/(\d+)\|(-?\d+)/int_op($1,'|',$2)/eg;
	next if $moredone;
	$moredone = 1 if $expr =~ s/(\d+)([-\+])(-?\d+)/int_op($1,$2,$3)/e;
	next if $moredone;
	$moredone = 1 if $expr =~ s/(-?\d+)>(-?\d+)/int_op($1,'>',$2)/eg;
	$moredone = 1 if $expr =~ s/(-?\d+)<(-?\d+)/int_op($1,'<',$2)/eg;
	$moredone = 1 if $expr =~ s/(-?\d+)>=(-?\d+)/int_op($1,'>=',$2)/eg;
	$moredone = 1 if $expr =~ s/(-?\d+)<=(-?\d+)/int_op($1,'<=',$2)/eg;
	$moredone = 1 if $expr =~ s/(-?\d+)==(-?\d+)/int_op($1,'==',$2)/eg;
	$moredone = 1 if $expr =~ s/(-?\d+)\!=(-?\d+)/int_op($1,'!=',$2)/eg;
	next if $moredone;
	$moredone = 1 if $expr =~ s/'(\w*)'\+'(\w*)'/"'$1$2'"/eg;
	next if $moredone;
	$moredone = 1 if $expr =~ s/'(\w*)'>'(\w*)'/$1 gt $2 ?1:0/eg;
	$moredone = 1 if $expr =~ s/'(\w*)'<'(\w*)'/$1 lt $2 ?1:0/eg;
	$moredone = 1 if $expr =~ s/'(\w*)'>='(\w*)'/$1 ge $2 ?1:0/eg;
	$moredone = 1 if $expr =~ s/'(\w*)'<='(\w*)'/$1 le $2 ?1:0/eg;
	$moredone = 1 if $expr =~ s/'(\w*)'=='(\w*)'/$1 eq $2 ?1:0/eg;
	$moredone = 1 if $expr =~ s/'(\w*)'\!='(\w*)'/$1 ne $2 ?1:0/eg;
	$moredone = 1 if $expr =~ s/'(\w*)'=~'(\w*)'/re_match(hex_decode($1),hex_decode($2))/eg;
	$moredone = 1 if $expr =~ s/'(\w*)'\!~'(\w*)'/!re_match(hex_decode($1),hex_decode($2))/eg;
	$moredone = 1 if $expr =~ s/(-?\d+)=~'(\w*)'/re_match($1,hex_decode($2))/eg;
	$moredone = 1 if $expr =~ s/(-?\d+)\!~'(\w*)'/!re_match($1,hex_decode($2))/eg;
	next if $moredone;
	$moredone = 1 if $expr =~ s/(-?\d+)&&(-?\d+)/int_op($1,'&&',$2)/eg;
	next if $moredone;
	$moredone = 1 if $expr =~ s/(-?\d+)\|\|(-?\d+)/int_op($1,'||',$2)/eg;
	next if $moredone;
	$moredone = 1 if $expr =~ s/(-?\d+)\?(-?\d+):(-?\d+)/int_op($1,'?',$2,$3)/eg;
	$moredone = 1 if $expr =~ s/(-?\d+)\?'(\w*)':'(\w*)'/$1 ? $2 : $3/eg;
    }
    return if $check_only;
    $expr =~ m/^-?\d+$/
	or construct_error($construct, "$what: unrecognized expression (reduces to $expr): $orig_expr");
    return $expr;
}
sub construct_eval {
    my ($construct, $parameter, $default_value, $check_only, $what, $seg) = @_;
    $what = "<$parameter> parameter".($what // "");
    my $expr = $construct->{$parameter};
    return $default_value unless defined($expr);
    my $result = construct_eval_string($construct, $expr, $seg, $check_only, $what);
    return ($check_only ? $expr : $result);
}
sub all_attributes {
    my ($layout) = @_;
    my @all = $layout->addressables;
    foreach my $a ($layout->addressables) {
	push @all, $a->map->a if $a->is_addrspace;
    }
    @all;
}
sub log2 {
    my ($num) = @_;
    my $i = 0;
    while ($num > 1) {
	$num >>= 1;
	$i++;
    }
    $i;
}
sub align_up {
    my ($n, $align, $mask) = @_;
    my $orig = $n;
    if (defined($mask) and $n > 0) {
	$n = (($n - 1) | $mask);
	goto overflow if $n == ~0;
	$n++;
    }
    if (defined($align) and $align > 1) {
	my $alignment = (1 << log2($align));		
	my $offset = $align - $alignment;			
	my $increment = $alignment - 1 - $offset;
	my $would_overflow = (~0 - $n < $increment);	
	$n -= $alignment if $would_overflow;
	$n += $increment;
	$n &= -$alignment;
	goto overflow if $would_overflow and ~0 - $n < $alignment;	
	$n += $alignment if $would_overflow;
	goto overflow if ~0 - $n < $offset;		
	$n += $offset;
    }
    if ($n != $orig) {
	::dprintf(undef, "Aligning 0x%x up to 0x%x (align=0x%x mask=0x%x)\n", $orig, $n, $align // 1, $mask // 0);
    }
    return $n;
overflow:
    ::dprintf(undef, "Aligning 0x%x up OVERFLOWS (align=0x%x mask=0x%x)\n", $orig, $align // 1, $mask // 0);
    return undef;
}
sub align_down {
    my ($n, $align, $mask) = @_;
    my $orig = $n;
    if (defined($mask) and $n != ~0) {
	$n = ($n + 1) & ~$mask;
	goto underflow if $n == 0;
	$n--;
    }
    if (defined($align) and $align > 1) {
	my $alignment = (1 << log2($align));		
	my $offset = $align - $alignment;		
	if ($offset == 0) {
	    if ($n != ~0) {				
		$n++;
		$n &= -$alignment;
		goto underflow if $n == 0;		
		$n--;
	    }
	} else {
	    goto underflow if $n < $offset - 1;		
	    $n -= ($offset - 1);
	    $n &= -$alignment;
	    $n += ($offset - 1);
	}
    }
    if ($n != $orig) {
	::dprintf(undef, "Aligning 0x%x down to 0x%x (align=0x%x mask=0x%x)\n", $orig, $n, $align // 1, $mask // 0);
    }
    return $n;
underflow:
    ::dprintf(undef, "Aligning 0x%x down UNDERFLOWS (align=0x%x mask=0x%x)\n", $orig, $align // 1, $mask // 0);
    return undef;
}
sub combine_align {
    my ($a, $b, $whata, $whatb) = @_;
    return (($a // 1), $whata) unless defined($b);
    return ($b, $whatb) unless defined($a);
    my $a_align = (1 << log2($a));	
    my $a_offset = $a - $a_align;	
    my $b_align = (1 << log2($b));	
    my $b_offset = $b - $b_align;	
    if (($a_offset & ($b_align - 1)) != ($b_offset & ($a_align - 1))) {
	return (undef, sprintf("%s 0x%x and %s 0x%x have incompatible offsets", $whata, $a, $whatb, $b));
    }
    return ( (($a > $b) ? $a : $b),
	     sprintf("combined alignment (of %s 0x%x and %s 0x%x)", $whata, $a, $whatb, $b) );
}
sub combine_align_mask {
    my ($ab, $mask, $whatab, $whatmask) = @_;
    $ab //= 1;   $ab = 1 unless $ab;
    $mask //= 0;
    my $ab_align = (1 << log2($ab));	
    my $ab_offset = $ab - $ab_align;	
    if (($ab_offset & $mask) != 0) {
	return (undef, undef, sprintf("%s 0x%x has offset 0x%x incompatible with %s 0x%x", $whatab, $ab, $ab_offset, $whatmask, $mask));
    }
    my $overmask = ($mask | ($ab_align - 1));	
    $mask = ($overmask & ~$ab_offset);	
    if ($overmask == ~0) {
	$ab_align = (~0 - (~0 >> 1));	
    } else {
	$ab_align = (($overmask + 1) & ~$overmask);	
    }
    return (($ab_align + $ab_offset), $mask, undef);
}
sub tohex {
    my ($n) = @_;
    defined($n) ? sprintf("0x%x", $n) : "(undef)";
}
sub check_misvalign {
    my ($layout, $linkmap) = @_;
    foreach my $mapping ($linkmap->map->a) {
	my $paddr = $linkmap->linkspace->addr_in_space($mapping->addr, $mapping->endaddr, $linkmap->physical);
	my $valign = $mapping->is('valign');
	next unless defined($paddr) and defined($valign);
	if (($paddr & $valign) != ($mapping->addr & $valign)) {
	    my $identity = ($valign == $linkmap->sizem1 or $valign == ~0);
	    my $what = $identity ? "n-identity" : sprintf("misaligned (on bits 0x%x)", $valign);
	    $what .= " mapping to ". $mapping->mem->name;
	    if ($identity and $mapping->has_attr('local')) {
		$mapping->set_attr('delete' => 1, 'lock' => $what);
	    } else {
		Error("$what mapping from vaddr 0x%x to paddr 0x%x", $mapping->addr, $paddr);
	    }
	}
    }
}
sub setupVectorLayout {
    my ($core_name, $pr, $diags) = @_;
    return setupSharedVectorLayout([[$core_name, $pr]], $diags);
}
sub setupSharedVectorLayout {
    my ($cores_info, $diags, $included_vecs, $include_relocatables) = @_;
    @$cores_info or InternalError("setupSharedVectorLayout: no cores");
    my $first_core = $cores_info->[0];
    my $oncore = @$cores_info > 1 ? " on cores " . join(",", map($_->[0], @$cores_info))
	       : $first_core->[0] ? " on core ".$first_core->[0]
	       : "";
    my $pr = $first_core->[1];
    if (grep($_->[1] ne $pr, @$cores_info)) {
	Error("sharing vectors among cores of different configs isn't yet allowed (TODO)");
    }
    #::dprintf(undef, "### setupSharedVectorLayout$oncore\n");
    my @layout_constructs = ();		
    $diags //= 0;
    $included_vecs //= { map { $_->name => $_ } $pr->vectors->swVectorList };
    if ($pr->vectors->relocatableVectorOption and (!defined($include_relocatables) or @$include_relocatables)) {
	my ($vec_minofs, $vec_maxofs) = Xtensa::Processor::Vector::reloc_vector_offsets($pr);
	push @layout_constructs, {
			name => 'reloc_vecs',
			construct => 'segment',
			(defined($include_relocatables)
			 ? (shared => $include_relocatables)
			 : ()),
			startaddr => "sw.vecbase + ".sprintf("0x%x", $vec_minofs),	
			startalign => (1 << $pr->vectors->vecbase_alignment) + $vec_minofs,
			minsize => $pr->vectors()->reloc_vectors_max_size() - $vec_minofs,
			size => $pr->vectors()->reloc_vectors_max_size() - $vec_minofs,
			requirement => "\@executable && !\@device && !\@exception && !\@isolate && \@readable",
			ignorelock => "ResetVector",	# "stationary vector ResetVector"
			prefer  => "\@writable",
			description => "relocatable vectors$oncore",
			assignments => { _memmap_vecbase_reset => sprintf("\$start - 0x%x", $vec_minofs),
					 "sw.vecbase"          => sprintf("\$start - 0x%x", $vec_minofs),
					},
			};
    }
    foreach my $vecname (sort {$included_vecs->{$a}->baseAddress <=> $included_vecs->{$b}->baseAddress} keys %$included_vecs) {
	my $vec = $included_vecs->{$vecname};
	my $linkmapnames = [];
	my $cores = $cores_info;
	if (ref($vec) eq 'HashObject') {
	    $linkmapnames = $vec->{linkmapnames} // [];
	    $cores = [ map([$_->name, $_->config->pr], @{$vec->cores}) ] if $vec->{cores};
	}
	my @corenames = map($_->[0], @$cores);
	::dprintf(undef, "## VEC addr=%s0x%08x %s%s\n",
			$vec->group eq "relocatable" ? "VECBASE+" : "",
			$vec->group eq "relocatable" ? $vec->offset : $vec->baseAddress,
			$vecname,
			(@$linkmapnames ? " (in linkmap(s) ".join(", ", @$linkmapnames).")" : ""));
	my $ignore_lock;
	my @vec_sections = ("." . $vecname . ".text:first:" . $vec->size);
	if ($vecname eq 'WindowVectors') {
	} elsif ($vecname eq 'ResetVector') {
	    my @reset_handler_sections = (".ResetHandler.text:third");
	    unshift @reset_handler_sections, ".ResetHandler.literal:third"
		unless $pr->core->externalExceptionArch or $pr->core->exceptionArch eq "XEAHALT";	
	    if (@$cores > 1) {
		@vec_sections = (".SharedResetVector.text:first:" . $vec->size);
	    } elsif ($pr->is_xea3 || $pr->is_xea5) {
		@vec_sections = (".ResetVector.text:second", @reset_handler_sections);
		$ignore_lock = "DispatchVector";	
	    } else {
		$vec_sections[0] .= ",::0";	
		push @vec_sections, @reset_handler_sections;
	    }
	} elsif ($vecname eq 'ResetHandler') {	
	    push @layout_constructs, {
		    name => 'ResetHandler',
		    construct => 'segment',
		    cores => \@corenames,
		    shared => $linkmapnames,
		    memory_byaddr => '.SharedResetVector.vaddr',
		    requirement => "\@executable",		# FIXME add:   . ($mpu ? "" : " && !\@cached");
		    prefer => "(~0) - \$sizem1",	
		    description => "ResetVector in same memory as SharedResetVector".$oncore,
		    sections => [ ".ResetVector.text:text:".$vec->size,
			".ResetHandler.literal", ".ResetHandler.text",
			],
		    };
	    next;
	} elsif ($vecname eq 'DispatchVector') {
	    push @vec_sections, ".DispatchHandler.literal:third";
	    push @vec_sections, ".DispatchHandler.text:third";
	    $ignore_lock = "ResetVector";	
	} else {
	    $vec_sections[0] .= ",::4:10%";
	}
	my $what = ($pr->vectors->relocatableVectorOption ? $vec->group." " : "")."vector ".$vecname.$oncore
			. ($corenames[0] ? " shared among cores ".join(",", @corenames) : "");
	my $vec_vaddr_expr;
	if (! $pr->vectors->relocatableVectorOption) {
	    $vec_vaddr_expr = sprintf("0x%x", $vec->baseAddress);
	} elsif ($vec->group eq "relocatable") {
	    $vec_vaddr_expr = sprintf("sw.vecbase + 0x%x", $vec->offset);
	} else {
	    $vec_vaddr_expr = "(sw.vecselect ? ";
	    if ($pr->vectors->vecBase1FromPins) {
		$vec_vaddr_expr .= "sw.vecreset_pins";
	    } elsif ($diags) {
		$vec_vaddr_expr .= "sw.static_vecbase1";
	    } else {
		$vec_vaddr_expr .= sprintf("0x%x", $pr->vectors->stationaryVectorBase1);
	    }
	    if ($diags) {
		$vec_vaddr_expr .= " : sw.static_vecbase0";
	    } else {
		$vec_vaddr_expr .= sprintf(" : 0x%x", $pr->vectors->stationaryVectorBase0);
	    }
	    $vec_vaddr_expr .= sprintf(") + 0x%x", $vec->offset);
	}
	my $construct = {
		name => $vecname,
		construct => 'segment',
		(@$linkmapnames
		 ? (shared => $linkmapnames) : ()),
		(@corenames && $corenames[0] ne ""
		 ? (cores => \@corenames) : ()),
		startaddr => $vec_vaddr_expr,
		keepsections => 0,
		description => $what,
		sections => \@vec_sections,
	    };
	$construct->{ignorelock} = $ignore_lock if defined($ignore_lock);
	if (@$cores > 1 and $vecname eq 'ResetVector') {
	    $construct->{assignments} = { ".SharedResetVector.vaddr" => "\$start" };
	}
	$construct->{size} = sprintf("0x%x", $vec->size);
	$construct->{attrs}{lock} = $what;
	$construct->{attrs}{fixed} = 1;
	push @layout_constructs, $construct;
    }
    if (0 and ($pr->is_xea3 || $pr->is_xea5)) {
	if ($pr->relocatableITB == 0) {
	    push @layout_constructs, {
		    name => 'intr_table',
		    construct   => 'segment',
		    startaddr => $pr->interrupts->interruptTableBase,
		    description => "interrupt table",
		    attrs    => {'lock' => "interrupt table"},	# 'fixed'=>1
		    sections => [ ".intr.data:first:".($pr->interrupts->count * 8) ],
		    };
	}
	if ($pr->relocatableISB == 0) {
	    push @layout_constructs, {
		    name => 'intr_stack',
		    construct   => 'segment',
		    endaddr   => $pr->interrupts->interruptStackBase - 1,
		    description => "interrupt stack",
		    attrs    => {'lock' => "interrupt stack"},	# 'fixed'=>1
		    sections => [ ".intr.top:STACK" ],
		    };
	}
    }
    my @exe_local_rams = ($pr->instRams, $pr->unifiedRams);
    if (($pr->core->externalExceptionArch or $pr->core->exceptionArch eq "XEAHALT")
	  and @exe_local_rams
	  and !grep($_->name eq 'ResetVector', $pr->vectors->swVectorList)) {
	my $ram = $exe_local_rams[0];
	push @layout_constructs, {
    		name => 'tx_reset',
    		construct => 'segment',
		memory => $ram->name,
		startaddr => $ram->basePAddr,	
		description => "Reset Vector",
		sections => [ ".ResetVector.text:first" ],
		};
    }
    return @layout_constructs;
}
sub setupMemoryLayout {
    my ($core_name, $pr, $noSysSegs) = @_;
    #::dprintf(undef, "### setupMemoryLayout on core $core_name\n");
    my @layout_constructs = ();		
    push @layout_constructs, {
    		name => 'allmem_code',
    		construct => 'segment',
    		memory => "*",	
		requirement => "\@executable",
		optional => 2,
		description => "code section for MEMORY",
		sections => [ ".MEMORY.text,:::10%" ],
		};
    push @layout_constructs, {
    		name => 'allmem_rodata',
    		construct => 'segment',
    		memory => "*",	
		requirement => "\@data",
		optional => 2,
		description => "rodata section for MEMORY",
		sections => [ ".MEMORY.rodata" ],
		};
    push @layout_constructs, {
    		name => 'allmem_data',
    		construct => 'segment',
    		memory => "*",	
		requirement => "\@data && \@writable",
		optional => 2,
		description => "data sections for MEMORY",
		sections => [ ".MEMORY.data", ".MEMORY.bss" ],
		};
    if (0) {
	push @layout_constructs, {
    		name => 'allmem_literal',
    		construct => 'segment',
    		memory => "*",	
		requirement => "\@data && !\@executable",
		optional => 2,
		description => "obsolete literal section for MEMORY",
		sections => [ ".MEMORY.literal" ],
		};
    }
    push @layout_constructs, {
    		name => 'code',
    		construct => 'segment',
		requirement => "\@executable",
		prefer => "\@writable",
		description => "code segment",
		sections => [ ".text," ],
		};
    push @layout_constructs, {
    		name => 'data',
    		construct => 'segment',
		requirement => "\@data && \@writable",
		prefer => "\@type != 'dataPort'",
		optional => 1,
		description => "default data sections",
		sections => [ ".rodata",
			      ((defined($pr) && $pr->core->ExtL32R) ? (".lit4") : ()),
			      ".data",
			      "__llvm_prf_names",
			      ".bss",
			      ($noSysSegs ? () : ("STACK", "HEAP")) ],
		};
    if (!$noSysSegs) {
	push @layout_constructs, {
    		name => 'rom_store',
    		construct => 'segment',
		requirement => "\@data && !\@writable && \$size > 32",
		prefer => "\@executable",	
		optional => 2,
		description => "ROM store section",
		sections => [ ".rom.store" ],
		};
    }
    push @layout_constructs, {
    		name => 'bump_zero',
    		construct => 'segment',
		startaddr => undef, endaddr => 3,
		keepsections => 0,	
		optional => 2,
		description => "bumped from zero",
		ordering => '+5',	
		attrs => {delete => 1},
		requirement => "!\@lock && !\@fixed && !\@device && \$sizem1 >= 16 && !\$leftTacked",
		}
		unless $noSysSegs;
    return @layout_constructs;
}
sub setupCore {
    my ($layout, $core_name, $pr, $swoptions, $xtensa_root) = @_;
    my $config = Component::XtensaConfig->new_internal($layout, "default", undef, $pr, $xtensa_root);
    my $core = Component::XtensaCore->new($layout, $core_name, undef, $config, $swoptions);
    $core->{hw_maps} = "";
    $core->{default_maps} = "";
    $core->{sw_sections} = "";
    $core;
}
sub instantiateLayout {
    my ($layout, $name, $layout_constructs, %opts) = @_;
    push @$layout_constructs, {
		construct => "linkmap",
		name => $name,
		cores => [""],
		vectors => [],		
		%opts
		};
    $layout->instantiateSystemLayout(create_local_memories => 0,
				create_memories => 0,
				system_name => "single_core",
				constructs => $layout_constructs);
    $layout->instantiateLinkmaps(ignore_missing_vectors => 1,
				constructs => $layout_constructs);
    return $layout->linkmaps->[0];
}
sub is_ptp_mmu {
  my ($pr) = @_;
  my $mmu = $pr->xt->mmu;
  return 0 unless defined($mmu) and defined($mmu->itlb) and defined($mmu->dtlb);
  return ( $mmu->asidBits >= 2
	&& $mmu->ringCount >= 2 );
}
sub create_local_memories {
    my ($layout) = @_;
    foreach my $core ($layout->cores) {
	my $core_namespace_p = $core->name . ".";
	my $pspace = $layout->find_addressable($core_namespace_p.'physical');
	if (!defined($pspace)) {
	    $layout->warn("creating local memories: skipping '".$core->name."', no physical space present");
	    next;
	}
	foreach my $ifacemap ($pspace->map->a) {
	    my $iface = $ifacemap->mem;
	    if ($iface->has_attr('local')) {
		$iface->map->n and Error("was asked to create local memories, but interface ".$iface->name." already has something mapped to it");
		my $locmem = Xtensa::System::Addressable->new($iface->sizem1, undef, {});
		my $memname = $iface->name;		
		if ($memname =~ s/_iface$//) {		
		    $memname =~ s/^instRam/iram/;
		    $memname =~ s/^instRom/irom/;
		    $memname =~ s/^dataRam/dram/;
		    $memname =~ s/^dataRom/drom/;
		    $memname =~ s/^dataPort/dport/;
		    $memname =~ s/^unifiedRam/uram/;
		} else {
		    $memname .= "_mem";
		}
		$layout->add_addressable($memname, $locmem);
		$iface->add_mapping(0, [[$locmem, 0, $iface->sizem1]], {});
	    }
	}
    }
}
sub debug_show_shared_linkmaps {
    my ($layout, $anywhere_sets, $sameaddr_sets) = @_;
    my %both = (%$anywhere_sets, %$sameaddr_sets);
    foreach my $setname (sort keys %both) {
	my $addresses = $sameaddr_sets->{$setname};
	my ($lmap_names, $oops) = $layout->names_of_linkmaps($setname);
	if (@$lmap_names == 1) {
	    $layout->dprintf("### UNIQUE FOR $setname:\n");
	    if (defined($addresses)) {
		foreach my $addr (sort {$a <=> $b} keys %$addresses) {
		    my $shareset = $addresses->{$addr};
		    $layout->dprintf("   At addresses 0x%x .. 0x%x\n", $addr, $addr + $shareset->sizem1);
		}
	    }
	    next;
	}
	$layout->dprintf("### SHARED BY $setname:\n");
	if (defined($addresses)) {
	    $layout->dprintf("At the same address:\n");
	    my @addrs = sort {$a <=> $b} keys %$addresses;
	    $layout->dprintf("   (none)\n") unless @addrs;
	    foreach my $addr (@addrs) {
		my $shareset = $addresses->{$addr};
		$layout->dprintf("   At addresses 0x%x .. 0x%x\n", $addr, $addr + $shareset->sizem1);
	    }
	}
	if (exists($anywhere_sets->{$setname})) {
	    my $sharesets = $anywhere_sets->{$setname};
	    my @diff_sets = grep($addresses->{$_->{set}->[0]->{addr}} ne $_, @$sharesets);
	    $layout->dprintf("At mismatching addresses:\n");
	    $layout->dprintf("   (none)\n") unless @diff_sets;
	    foreach my $shareset (@diff_sets) {
		$layout->dprintf("   Segment of 0x%x bytes:\n", $shareset->sizem1+1);
		my %addresses = ();
		foreach my $entry (sort {$a->{addr} <=> $b->{addr}} @{$shareset->{set}}) {
		    my $name = $layout->linkmap_name($entry->{space});
		    my $addr = $entry->{addr};
		    push @{$addresses{$addr}}, $name;
		}
		foreach my $addr (sort {$a <=> $b} keys %addresses) {
		    my $cores = join(", ", @{$addresses{$addr}});
		    $layout->dprintf("      At addresses 0x%x .. 0x%x: core(s) %s\n", $addr, $addr + $shareset->sizem1, $cores);
		}
	    }
	}
    }
}
sub intersect_linkmaps {
    my ($layout, $name, $linkmaps, $cores, $ignored_attrs) = @_;
    my $ignored_attributes = {
	asid => 1, background => 1, bustype => 1, ca => 1, check => 1, "local" => 1, type => 1,
	startalign => 1, endalign => 1,	
	%{ $ignored_attrs // {} } };
    my @cores = @{$cores // []};
    foreach my $core (map(@{$_->cores}, @$linkmaps)) {
	push @cores, $core unless grep($_ eq $core, @cores);
    }
    defined($name) or $name = join("*", map($_->name, @$linkmaps));
    my @lmaps = @$linkmaps;			
    my $first = shift @lmaps;			
    my $sharedmap = $first->dup(0);		
    $sharedmap->{name} = $name;
    $sharedmap->{cores} = \@cores;
    $sharedmap->{linkspace} = undef;
    $sharedmap->{physical} = undef;
    $sharedmap->{symbols} = {};
    $sharedmap->{sections_by_name} = {};
    foreach my $linkmap (@lmaps) {		
	$sharedmap->intersect($linkmap, $ignored_attributes);
    }
    $sharedmap->check;
    return $sharedmap;
}
sub create_linkmap {
    my ($layout, $name, $cores, %options) = @_;
    my $ns = ($name eq "" ? "" : $name.".");
    my $linkmap = Xtensa::System::LinkMap->new($cores, $layout->virtual($name), $layout->physical($name), $name,
			    layout => $layout, %options);
    $layout->add_addressable($ns."linkmap", $linkmap, 1);
    $linkmap->check;
    $layout->add_linkmap($name, $linkmap);
    return $linkmap;
}
sub create_all_linkmaps {
    my ($layout, $create_shared_linkmaps, %options) = @_;
    $layout->clear_linkmaps;
    my $requests = [ @{$layout->requested_linkmaps} ];
    my %cores_requested;		
    my @linkmaps;
    foreach my $request (@$requests) {
	my %opts = %$request;
	my $cores = delete $opts{cores};
	my $name  = delete $opts{name};
	my $full_image = (delete $opts{full_image}) // 1;
	my $vectors    = delete $opts{vectors};
	foreach (@$cores) { $cores_requested{$_->name} = $_; }
	next unless @$cores == 1;	
	my $core = $cores->[0];
	$layout->dprintf("### Creating linkmap '%s' for core %s\n", $name, $core->name);
	my $linkmap = create_linkmap($layout, $name, $cores, %options, %opts);
	$linkmap->{full_image} = $full_image;
	$linkmap->{vectors} = $vectors;
	$core->set_linkmap($linkmap) unless defined($core->linkmap)
					and !($full_image and !$core->linkmap->{full_image});
	push @linkmaps, $linkmap;
    }
    foreach my $corename (sort keys %cores_requested) {
	my $core = $cores_requested{$corename};
	next if defined($core->linkmap);
	my $linkmap = create_linkmap($layout, $core->name, [$core], %options);
	$core->set_linkmap($linkmap);
    }
    $layout->dprintf("\n### INITIAL LINKMAPS:\n%s\n\n", scalar($layout->format_addressables));
    $layout->dprintf("### Created linkmaps, intersecting (computing shared_sets) ...\n");
    my ($anywhere_sets, $sameaddr_sets, $sameaddr_addrs) = $layout->shared_sets();
    debug_show_shared_linkmaps($layout, $anywhere_sets, $sameaddr_sets);
    my $allcores_set = $layout->name_of_all;
    if ($create_shared_linkmaps) {
	foreach my $sameaddr_set (sort keys %$sameaddr_sets) {
	    my ($lmap_names, $oops) = $layout->names_of_linkmaps($sameaddr_set);	
	    @$oops and InternalError("unrecognized linkmap set name '$sameaddr_set' from shared_sets: ".join(", ",@$oops));
	    next unless @$lmap_names > 1;
	    my $cores = [ map($layout->cores_byname->{$_}, @$lmap_names) ];
	    my $name = $sameaddr_set;
	    $name = "shared" if $name eq $allcores_set;	# special case linkmap name "shared" for all cores
	    $layout->dprintf("### Auto-requesting shared linkmap '%s' for cores %s\n", $name, join(", ", @$lmap_names));
	    push @$requests, { name => $name, cores => $cores };
	}
    }
    foreach my $request (@$requests) {
	my $name = $request->{name};
	my $cores = $request->{cores};
	next if @$cores == 1;		
	$layout->dprintf("### Creating shared linkmap '%s' for cores %s\n", $name, join(", ", map($_->name, @$cores)));
	my $sharedmap = $layout->intersect_linkmaps($name, [map($_->linkmap, @$cores)]);
	$sharedmap->map->n or Error("there is no shared memory at the same virtual address for all cores (".join(",",map($_->name,@$cores)).") of shared linkmap '$name'");
	$sharedmap->coalesce_memories(1) if $request->{span_okay};
	$layout->dprintf("\n### INTERSECT MAP for $name:\n%s\n\n",
		join("\n", map($_->format_self, $sharedmap->map->a)));
	$sharedmap->{full_image} = $request->{full_image} // 0;
	$sharedmap->{vectors} = $request->{vectors};
	$layout->add_addressable($name.".linkmap", $sharedmap, 1);
	$layout->add_linkmap($name, $sharedmap);
	if ($sharedmap->{full_image}) {
	    foreach my $core (@$cores) {
		$core->set_linkmap($sharedmap) unless defined($core->linkmap) and $core->linkmap->{full_image};
	    }
	}
	push @linkmaps, $sharedmap;
    }
    foreach (@linkmaps) {$_->check;}	
    my $commonmap;
    if (exists($sameaddr_sets->{$allcores_set})) {
	my $addresses = $sameaddr_sets->{$allcores_set};
	$layout->dprintf("### Found a potential CC: all cores share some common address space\n");
	$commonmap = $linkmaps[0]->dup;
	my $map = $commonmap->ref_map;
	my @outmap = ();
	foreach my $addr (sort {$a <=> $b} keys %$addresses) {
	    my $shareset = $addresses->{$addr};
	    my $endaddr = $addr + $shareset->sizem1;
	    $layout->dprintf("   At addresses 0x%x .. 0x%x\n", $addr, $endaddr);
	    shift @$map while @$map and $map->[0]->endaddr < $addr;
	    @$map == 0 and InternalError("missing mapping in first core");	
	    my $m = shift @$map;
	    $m->addr > $addr		
	      and InternalError("incomplete mapping (missed 0x%x) in first core at 0x%x..0x%x", $m->addr, $m->endaddr);
	    $m->truncate($addr, undef);
	    if ($m->endaddr > $endaddr) {
		my $remains = $m->dup;
		$remains->truncate($endaddr + 1, undef);
		$m->truncate(undef, $endaddr);
		unshift @$map, $remains;
	    }
	    push @outmap, $m;
	}
	@$map = @outmap;
	$commonmap->check;			
	$layout->dprintf("\n### INTERSECT MAP:\n%s\n\n", join("\n",map($_->format_self,$commonmap->map->a)));
	$layout->dprintf("### Got intersect\n");
    } else {
	$layout->dprintf("### No intersect\n");
    }
    $layout->sharemap_reset;
    return ($commonmap, \@linkmaps);
}
sub fitting_locations {
    my ($createmap, $size, $whichgb, $locations, $what) = @_;
    foreach my $seg ($createmap->map->a) {
	next if grep(!$_->mem->is_addrspace, $seg->spans);
	my $min = ($seg->addr == 0) ? 0
		  : (($seg->addr - 1) & -$size) + $size;
	my $max = ($seg->endaddr == 0xffffffff) ? 0xffffffff
		  : (($seg->endaddr + 1) & -$size);
	next if $max == 0;
	$max-- unless $max == 0xffffffff;
	next if $max < $min;
	if (defined($whichgb)) {	
	    $min = $whichgb if $whichgb >= $min and $whichgb < $max;
	}
	if ($min <= 0x40000000 and $max > 0x40000000 and $size <= 0x40000000) {
	    $locations->{0+0x40000000} += 0;		
	} elsif ($min <= 0x20000000 and $max > 0x20000000 and $size <= 0x20000000) {
	    $locations->{0+0x20000000} += 0;		
	} elsif ($min == 0 and $max > $size) {
	    $locations->{0+$size} += 0;			
	} else {
	    $locations->{0+$min} += 0;			
	}
    }
    if (scalar(keys(%$locations)) == 0) {
	Error("cannot find space for an aligned $what of 0x%x bytes", $size);
    }
    return sort {  $locations->{$a} <=> $locations->{$b}
		or ((($b & 0xc0000000) == $whichgb) <=> (($a & 0xc0000000) == $whichgb))
		or (($a == 0) <=> ($b == 0))
		or $a <=> $b
		} keys %$locations;
}
sub natur_aligned_sizes {
    my ($addr,$endaddr, $in_addr,$in_endaddr) = @_;
    my ($minsz,$maxsz) = (0,0);
    for (my $sizem1 = 0; $sizem1 < ~0; $sizem1 = $sizem1*2 + 1) {
	my $maddr = ($addr & ~$sizem1);
	my $mend = $maddr + $sizem1;
	next if $mend < $endaddr;		
	last if $maddr < $in_addr or $mend > $in_endaddr;	
	$minsz = $sizem1+1 if $minsz == 0;
	$maxsz = $sizem1+1;
    }
    return ($minsz, $maxsz);
}
sub add_sysmem {
    my ($layout, $createmap, $addr, $size, $name, $showname, $attrs) = @_;
    my $sysmem = Xtensa::System::Addressable->new($size - 1, undef, $attrs);
    $layout->add_addressable($name, $sysmem);
    $createmap->add_mapping($addr, [[$sysmem, 0, $size - 1]], {}, 1);	
    my $m = $createmap->cut_map($addr, $addr + ($size - 1), 0);
    if (defined($m)) {
	$createmap->cut_mem_aliases($m, undef, undef, 1);
    }
    $createmap->add_mapping($addr, [[$sysmem, 0, $size - 1]], {}, 0);	
    $layout->dprintf("### $showname created at 0x%x .. 0x%x\n", $addr, $addr + ($size - 1));
}
sub place_sysrom_over_reset_vector {
    my ($layout, $createmap) = @_;
    my $sysrom_minsz =  1 * 1024 * 1024;	
    my $sysrom_size =  16 * 1024 * 1024;	
    my $sysrom_maxsz = 64 * 1024 * 1024;	
    my $rom_addr = undef;
    my $rom_size = $sysrom_size;
    if (grep(is_ptp_mmu($_->config->pr), $layout->cores)) {
	$rom_addr = 0xFE000000;
	$rom_size = $sysrom_size;
    } else {
	$layout->dprintf("### Placing ROM at some static reset vector\n");
	foreach my $core ($layout->cores) {
	    my ($vec) = grep( $_->name eq "ResetVector", $core->config->pr->vectors->swVectorList );
	    next unless defined($vec);
	    my $vec_vaddr = $vec->addr($core->config->pr, $core->swoptions);
	    $layout->dprintf("** Core %s RESET VEC addr=0x%08x size=0x%x\n",
			$core->name, $vec_vaddr, $vec->size);
	    my $vecend = $vec_vaddr + ($vec->size - 1);
	    my $seg = $createmap->cuttable_map($vec_vaddr, $vecend);
	    next unless defined($seg) and $seg->mem->is_addrspace and !$seg->is_spanned;
	    my ($minsz, $maxsz) = natur_aligned_sizes($vec_vaddr, $vecend, $seg->addr, $seg->endaddr);
	    if ($minsz == 0 or $minsz > $sysrom_maxsz or $maxsz < $sysrom_minsz) {
		Error("cannot fit aligned ROM of 0x%x to 0x%x bytes around reset vector (at 0x%08x .. 0x%08x) in the available space (0x%08x .. 0x%08x)", $sysrom_minsz, $sysrom_maxsz, $rom_addr, $rom_addr + ($sysrom_size - 1), $seg->addr, $seg->endaddr);
	    }
	    $rom_size = $sysrom_size;
	    $rom_size = $maxsz if $maxsz < $sysrom_size;
	    $rom_size = $minsz if $minsz > $sysrom_size;
	    if ($rom_size != $sysrom_size) {
		$layout->warn("cannot fit aligned ROM of exactly 0x%x bytes around reset vector (at 0x%08x .. 0x%08x) in the available space (0x%08x .. 0x%08x): creating ROM of 0x%x bytes instead", $sysrom_size, $rom_addr, $rom_addr + ($sysrom_size - 1), $seg->addr, $seg->endaddr, $rom_size);
	    }
	    $rom_addr = ($vec_vaddr & -$rom_size);		
	    last;
	}
    }
    add_sysmem($layout, $createmap, $rom_addr, $rom_size, "srom", "system ROM", {writable=>0})
	    if defined($rom_addr);
    return ($rom_addr, $rom_size);
}
sub place_sysram {
    my ($layout, $createmap, $rom_addr) = @_;
    my $sysram_size = 128 * 1024 * 1024;	
    my $ram_addr;
    {
	$layout->dprintf("### Placing RAM where most static vectors are found\n");
	my %ramvecs = ();
	foreach my $core ($layout->cores) {
	    foreach my $vec ($core->config->pr->vectors->swVectorList) {
		next if $core->config->pr->vectors->relocatableVectorOption and $vec->group eq "relocatable";
		my $vec_vaddr = $vec->addr($core->config->pr, $core->swoptions);
		$layout->dprintf("** Core %s VEC addr=0x%08x size=0x%x %s\n",
			    $core->name, $vec_vaddr, $vec->size, $vec->name);
		my $seg = $createmap->cuttable_map($vec_vaddr, $vec_vaddr + ($vec->size - 1));
		next unless defined($seg);
		my $addr = ($vec_vaddr & -$sysram_size);
		next unless $seg->addr <= $addr and $addr + ($sysram_size-1) <= $seg->endaddr
			and $vec_vaddr + ($vec->size - 1) <= $addr + ($sysram_size - 1);
		$ramvecs{0+$addr}++;
	    }
	}
	my $romgb = defined($rom_addr) ? ($rom_addr & 0xc0000000) : 123;
	($ram_addr) = fitting_locations($createmap, $sysram_size, $romgb, \%ramvecs, "system RAM");
    }
    my $ram_size = $sysram_size;	
    add_sysmem($layout, $createmap, $ram_addr, $ram_size, "sram", "system RAM", {writable=>1});
    return ($ram_addr, $ram_size);
}
sub place_stray_vector_rams {
    my ($layout, $createmap) = @_;
    my $maxgap = 500 * 1024;	
    my %vecsegs = ();
    foreach my $core ($layout->cores) {
	foreach my $vec ($core->config->pr->vectors->swVectorList) {
	    next if $core->config->pr->vectors->relocatableVectorOption and $vec->group eq "relocatable";
	    my $vec_vaddr = $vec->addr($core->config->pr, $core->swoptions);
	    $layout->dprintf("** Core %s VEC addr=0x%08x size=0x%x %s\n",
			$core->name, $vec_vaddr, $vec->size, $vec->name);
	    my $seg = $createmap->cuttable_map($vec_vaddr, $vec_vaddr + ($vec->size - 1));
	    next unless defined($seg) and $seg->mem->is_addrspace and !$seg->is_spanned;
	    $vecsegs{$seg}[0] = $seg;
	    push @{$vecsegs{$seg}[1]}, { vec=>$vec, vaddr=>$vec_vaddr };
	}
    }
    my @little_mems;
    my $littles = 0;
    if (scalar(keys %vecsegs)) {
	$layout->warn("vectors are spread out, creating extra little system RAMs to contain them");
	foreach my $segindex (sort keys %vecsegs) {
	    my ($seg, $vecs) = @{$vecsegs{$segindex}};
	    my @v = sort { $a->{vaddr} <=> $b->{vaddr} } @$vecs;
	    my @memvecs = (shift @v);
	    foreach my $vecv (@v, {}) {
		my ($vec, $vaddr) = ($vecv->{vec}, $vecv->{vaddr});
		if (!defined($vec) or $vaddr - ($memvecs[-1]->{vaddr} + $memvecs[-1]->{vec}->size) >= $maxgap) {
		    my $addr = $memvecs[0]->{vaddr};
		    my $endaddr = $memvecs[-1]->{vaddr} + ($memvecs[-1]->{vec}->size - 1);
		    for (my $align = 16; $align < 2 * ($endaddr - $addr); $align *= 2) {
			my $newaddr = ($addr & -$align);
			my $newend = (($endaddr + $align) & -$align) - 1;
			$addr = $newaddr if $newaddr >= $seg->addr;
			$endaddr = $newend if $newend <= $seg->endaddr
			    and (!defined($vec) or $newend < $vaddr);
		    }
		    push @little_mems, [$addr, $endaddr];
		    my $is_rom = grep($_->{vec}->name eq 'ResetVector', @memvecs);
		    add_sysmem($layout, $createmap, $addr, $endaddr - $addr + 1,
				    ($is_rom ? "extrarom" : "extraram").$littles,
				    ($is_rom ? "Extra ROM " : "Extra RAM ").$littles,
				    {writable => ($is_rom ? 0 : 1)});
		    $littles++;
		    @memvecs = ();
		}
		push @memvecs, $vecv;
	    }
	}
    }
}
sub create_default_system_memories {
    my ($layout, $create_memories) = @_;
    return if defined($create_memories) and !$create_memories;
    $layout->dprintf("\n### SYSTEM BEFORE SYSTEM MEMORY CHECK:\n%s\n\n", scalar($layout->format_addressables));
    $layout->dprintf("### Creating temporary virtual spaces and core-mm and linkmaps for each core...\n");
    my @linkmaps;
    my @virtuals;
    foreach my $core ($layout->cores) {
	my $virtual = $layout->virtual($core->name);
	my $alt_virtual = $virtual->dup_nomap;
	my $vname = join(".",$core->name,"virtual.CHECK");
	$layout->add_addressable($vname, $alt_virtual);
	push @virtuals, $alt_virtual;
	my @mm_constructs = $layout->read_xld("xtensa-core/default-maps", { _namespace => $core->name, virtual => "virtual.CHECK" }, $core);
	$layout->apply_constructs(\@mm_constructs, 0);
	my $linkmap = Xtensa::System::LinkMap->new([$core], $alt_virtual, $layout->physical($core->name), $core->name,
			    layout => $layout, withspaces => 1);
	push @linkmaps, $linkmap;
	$linkmap->check;
	$layout->dprintf("### Created unnamed linkmap for core %s, %s with %d map entries.\n", $core->name, $vname, $linkmap->map->n);
	$layout->dprintf("%s\n", $linkmap->dump);
    }
    $layout->dprintf("### Created temporary linkmaps, intersecting...\n");
    my $commonmap = $layout->intersect_linkmaps("commonmap", \@linkmaps, [$layout->cores]);
    my $common_exists = ($commonmap->map->n > 0);
    my $common_populated = grep(!$_->mem->is_addrspace, map($_->spans, $commonmap->map->a));
    $layout->dprintf("### A common address space %s%s\n",
		$common_exists ? "exists, and is " : "does not exist",
		$common_exists ? ($common_populated ? "populated" : "empty") : "" );
    if (!defined($create_memories)) {
	$create_memories = ($common_exists && !$common_populated);
    }
    if (! $create_memories) {
	$layout->dprintf("### Using existing system memories\n");
    } else {
	if (!$common_exists) {
	    Error("for now, can only automatically create memories when all cores share a common address space; in this system they don't, so until memory creation gets more automated, you have to manually populate memories in this system");
	}
	if ($common_populated) {
	    Error("there is already some memory or device in the common address space, automatically creating memory in that case is not handled");
	}
	$layout->dprintf("### Preparing to create system memories\n");
	my $createmap = $commonmap->dup;	
	my ($rom_addr, $rom_size) = place_sysrom_over_reset_vector($layout, $createmap);
	my ($ram_addr, $ram_size) = place_sysram($layout, $createmap, $rom_addr);
	if (!defined($rom_addr)) {
	    ($rom_addr) = fitting_locations($createmap, $rom_size, ($ram_addr & 0xc0000000), { }, "system ROM");
	    add_sysmem($layout, $createmap, $rom_addr, $rom_size, "srom", "system ROM", {writable=>0});
	}
	place_stray_vector_rams($layout, $createmap);
    }
    foreach my $alt_virtual (@virtuals) {
	$layout->remove_addressable($alt_virtual);
    }
}
sub process_partition_requests {
    my ($layout, $constructs) = @_;
    $layout->dprintf("### Partition request gathering phase\n");
    my @partition_constructs = array_extract { $_->{construct} eq "segment" and defined($_->{partition}) } @$constructs;
    $layout->apply_constructs(\@partition_constructs, 1);
    $layout->dprintf("### Partition request gathering done\n");
}
sub apply_partitions {
    my ($layout, $construct) = @_;
    $layout->dprintf("### Partition assignment phase\n");
    foreach my $linkmap (@{$layout->linkmaps}) {
	my %portionmaps;
	foreach my $mapping ($linkmap->map->a) {
	    my $mportion = $mapping->{partition_portion};
	    push @{$portionmaps{$mportion}}, $mapping if defined($mportion);
	}
	foreach my $portionkey (keys %portionmaps) {
	    my $maps = $portionmaps{$portionkey};
	    next unless @$maps > 1;
	    my $totalsize = 0;
	    foreach (@$maps) { $totalsize += $_->sizem1 + 1; }
	    my $mportion = $maps->[0]->{partition_portion};
	    my ($portion, $is_percent, $amount, $startalign, $endalign) = @$mportion;
	    next if $is_percent;
	    foreach my $mapping (@$maps) {
		my $prorated = $amount * ($mapping->sizem1 + 1.0) / ($totalsize * 1.0);
		$prorated = int($prorated + 0.00001) unless $is_percent;
		$mapping->{partition_portion} = [$portion, $is_percent, $prorated, $startalign, $endalign];
	    }
	}
    }
    $layout->dprintf("### Computing shared_sets ...\n");
    my ($anywhere_sets, $sameaddr_sets, $sameaddr_addrs) = $layout->shared_sets();
    debug_show_shared_linkmaps($layout, $anywhere_sets, $sameaddr_sets);	
    foreach my $shareset (map(@$_, values %$anywhere_sets)) {
	my $sizem1 = $shareset->sizem1;
	my $set = $shareset->set;	
	my %addrs; foreach (@$set) { push @{ $addrs{$_->{addr}} }, $_->{space}; }
	next unless @$set > 1;		
	my %lmnames = map { $_->{space}->name => 1 } @$set;
	my @s_linkmaps = grep(exists($lmnames{$_->name}), @{$layout->{linkmaps}}); 
	next unless @s_linkmaps > 1;	
	my @sets = sort { scalar(@{$addrs{$b->{addr}}}) <=> scalar(@{$addrs{$a->{addr}}}) } @$set;
	my $sumbytes = 0;
	my $sumpercent = 0;
	my $maxalign = 4;		
	my @mapremain;
	my @inuse;
	my $zaddr;
	foreach my $linkmap (@s_linkmaps) {
	    my $portion; my $portion_addr;
	    my @lm_addrs = map($_->{addr}, grep($_->{space} eq $linkmap, @sets));
	    foreach my $addr (@lm_addrs) {
		my ($mapping) = $linkmap->findmaps($addr, $addr + $sizem1);
		push @inuse, sprintf("0x%x..0x%x in 0x%x..0x%x in %s (%s, %u sections)", $addr, $addr+$sizem1, $mapping->addr, $mapping->endaddr, $linkmap->name, $mapping->is('lock'), $mapping->sections->n) if $mapping->is('lock'); 
		my $mportion = $mapping->{partition_portion};
		if ($mportion) { $zaddr = $addr; }
		next unless defined($mportion);
		$maxalign = ::mymax($maxalign, $mportion->[3], $mportion->[4]);
		if (defined($portion) and $mportion->[0] ne $portion->[0]) {
		    Error("multiple partition portions in linkmap %s for the same memory at different addresses: %s at 0x%x..0x%x and %s at 0x%x..0x%x", $linkmap->name, $portion->[0], $portion_addr, $portion_addr+$sizem1, $mportion->[0], $addr, $addr+$sizem1);
		}
		$portion = $mportion;
		$portion_addr = $addr;
	    }
	    if (defined($portion)) {
		if ($portion->[1]) {
		    $sumpercent += $portion->[2];
		} else {
		    $sumbytes += $portion->[2] + ::mymax($portion->[3], $portion->[4]) - 1;
		}
		$linkmap->{_portion}{$zaddr} = [$portion->[2], $portion->[1]];
	    } else {
		push @mapremain, $linkmap;
	    }
	}
	if (@inuse) {
	    construct_warning($construct, "dropping partitioning of segments already in use: %s", join(", ", @inuse));
	    next;
	}
	if ($sumbytes < 0 or $sumbytes > 0 and $sumbytes-1 > $sizem1) {
	    Error("%u total partition bytes (plus %u%%) requested out of only %u bytes available at %s", $sumbytes, $sumpercent, $sizem1+1, join(", ", map(sprintf("0x%x..0x%x of %s", $_->{addr}, $_->{addr}+$sizem1, $_->{space}->name), @$set)));
	}
	my $bytespercent = $sumbytes * 100.0 / ($sizem1+1.0);
	my $totalpercent = $sumpercent + $bytespercent;
	my $nonbytepercent = 100.0 - $bytespercent;
	$layout->dprintf("### sumbytes=$sumbytes sizem1=$sizem1 bytespercent=$bytespercent sumpercent=$sumpercent totalpercent=$totalpercent nonbytepercent=$nonbytepercent\n");
	$nonbytepercent = 0 if $nonbytepercent < 0;
	my $autopercent = 0;
	if ($sumpercent > $nonbytepercent) {
	    $layout->warn("%u%% total partitions requested (%u%% + (%u of %u bytes)) out of 100%%, scaling down the %u%% to fit, at %s", $totalpercent, $sumpercent, $sumbytes, $sizem1+1, $sumpercent, join(", ", map(sprintf("0x%x..0x%x of %s", $_->{addr}, $_->{addr}+$sizem1, $_->{space}->name), @$set)));
	    foreach my $linkmap (@s_linkmaps) {
		my $portion = $linkmap->{_portion}{$_->{addr}};
		next unless defined($portion) and $portion->[1];
		$portion->[0] *= $nonbytepercent/$sumpercent;
	    }
	    $sumpercent = $nonbytepercent;
	    $totalpercent = 100.0;
	} elsif ($totalpercent < 100.0 and scalar(@mapremain) > 0) {
	    $autopercent = 100.0 - $totalpercent;
	    my $remain_percent = ($autopercent / scalar(@mapremain));
	    foreach my $linkmap (@mapremain) {
		$linkmap->{_portion}{$sets[0]->{addr}} = [$remain_percent, 1];
		$sumpercent += $remain_percent;
	    }
	    $totalpercent = 100.0;
	} 
	my $orig_addr = $sets[0]->{addr};	
	my $align_addr = $orig_addr;
	my @p_linkmaps = grep(defined($_->{_portion}{$orig_addr}), @s_linkmaps);
	$layout->dprintf("\n### Partitioning 0x%x..0x%x (per linkmap %s) - %u bytes and %f%% assigned (%f%% auto-assigned)\n", $orig_addr, $orig_addr+$sizem1, $sets[0]->{space}->name, $sumbytes, $sumpercent, $autopercent);
	my @parts;
	foreach my $i (0 .. $#p_linkmaps) {
	    my $linkmap = $p_linkmaps[$i];
	    my $portion = delete $linkmap->{_portion}{$orig_addr};
	    my $is_last = ($i == $#p_linkmaps);
	    my ($size, $is_percent) = @$portion;
	    $layout->dprintf("### Partitioning for linkmap %s, assigned %u%s\n", $linkmap->name, $size, $is_percent ? "%" : "");
	    my $offset = $align_addr - $orig_addr;
	    my $remaining = $sizem1 - $offset + 1;
	    last if $remaining < $maxalign;
	    if ($is_percent) {
		my $percent = $size;
		$size = int(($size * 1.0 / $sumpercent) * $remaining);
		$sumpercent -= $percent;
	    }
	    my $offset_end = ($offset + $size + $maxalign - 1) & -$maxalign;
	    $size = $offset_end - $offset;
	    $size = $remaining if $size > $remaining or ($is_last and $remaining - $size <= $maxalign);
	    next if $size <= 0;
	    $offset_end = $offset + ($size - 1);
	    push @parts, [$linkmap, $offset, $offset_end];
	    $layout->dprintf("### Partitioned offsets 0x%x..0x%x (%u bytes) to linkmap %s\n", $offset, $offset_end, $size, $linkmap->name);
	    $align_addr += $size;
	}
	my $offset = $align_addr - $orig_addr;
	my $remaining = $sizem1 - $offset + 1;
	$layout->dprintf("### Partitioned offsets 0x%x..0x%x (%u remaining bytes) as unassigned\n", $offset, $offset + $remaining - 1, $remaining);
	push @parts, [undef, $offset, $offset + ($remaining - 1)] if $remaining > 0;	
	foreach my $linkmap (@s_linkmaps) {
	    my @lm_addrs = map($_->{addr}, grep($_->{space} eq $linkmap, @sets));
	    foreach my $addr (@lm_addrs) {
		foreach my $part (@parts) {
		    my ($lm, $start, $end) = @$part;
		    my $lmname = defined($lm) ? $lm->name : "(unassigned)";
		    $layout->dprintf("### Partition: cutting 0x%x..0x%x (0x%x + offsets 0x%x..0x%x) of linkmap %s assigned to linkmap %s\n", $addr+$start, $addr+$end, $addr, $start, $end, $linkmap->name, $lmname);
		    my $cutmap = $linkmap->cut_map($addr + $start, $addr + $end, 0);
		    $cutmap->set_attr('partition' => $lmname);
		    if (!defined($lm) or $linkmap ne $lm) {
			if ($cutmap->sections->n) {
			    construct_internal_error($construct, "partitioning segment 0x%x..0x%x out of linkmap %s (it was assigned to linkmap %s), however it has sections in it:  %s", $cutmap->addr, $cutmap->endaddr, $linkmap->name, $lmname, join(", ", map($_->name, $cutmap->sections->a)));
			}
			$cutmap->set_attr('delete' => 1,  'lock' => "partition for linkmap ".$lmname);
		    }
		}
	    }
	}
    }
    foreach my $linkmap (@{$layout->{linkmaps}}) {
	foreach my $mapping ($linkmap->map->a) {
	    delete $mapping->{partition_portion};
	}
    }
    $layout->dprintf("### Partition assignment done\n");
}
sub find_shared_vectors {
    my ($layout) = @_;
    $layout->dprintf("### Placing static vectors\n");
    my @vecs = ();
    foreach my $core ($layout->cores) {
	my $linkmap = $core->linkmap;
	$linkmap->check;
	my $pr = $core->config->pr;
	my @sorted_vectors = sort {$a->addr($pr, $core->swoptions) <=> $b->addr($pr, $core->swoptions)} $pr->vectors->swVectorList;
	foreach my $vec (@sorted_vectors) {
	    next if $pr->vectors->relocatableVectorOption and $vec->group eq "relocatable";
	    my $vec_vaddr = $vec->addr($pr, $core->swoptions);
	    $layout->dprintf("** Core %s VEC addr=0x%08x vaddr=0x%08x %s\n",
			$core->name, $vec_vaddr, $vec_vaddr, $vec->name);
	    my $what = $vec->name." vector on core ".$core->name;
	    my $seg = $linkmap->cuttable_map($vec_vaddr, $vec_vaddr + $vec->size - 1, $what);
	    $seg->is_spanned and Error("vectors not yet supported on segments spanning multiple memories");
	    push @vecs, [$core, $vec, $what, $seg, $vec_vaddr];  
	}
    }
    $layout->dprintf("### Listing shared vectors\n");
    my @deferred_sections = ();
    my %shared_vectors = ();	
    my @shared_vector_sets = ();
    my ($warn_endian, $warn_prid);
    while (@vecs) {
	my $current = shift @vecs;
	my ($core, $vec, $what, $seg, $vec_vaddr) = @$current;
	my $vec_mem = $seg->mem;
	my $vec_memofs = $seg->offset;
	my $vec_end = $vec_vaddr + ($vec->size - 1);
	my @shared = ($current);
	my %sharedcores = ($core => 1);
	my @copyvecs = @vecs;	
	foreach my $v (@copyvecs) {
	    my ($vcore, $vvec, $vwhat, $vseg, $vvaddr) = @$v;
	    next unless $vec_mem eq $vseg->mem and $vec_memofs eq $vseg->offset;
	    my $vvec_end = $vvaddr + ($vvec->size - 1);
	    next if $vvaddr > $vec_end or $vvec_end < $vec_vaddr;
	    $vec_vaddr == $vvaddr
		or Error("vectors partially overlap: $what and $vwhat");
	    $vec->name eq $vvec->name
		or Error("mismatching vectors overlap: $what and $vwhat");
	    $warn_endian = 1 if $core->config->pr->xt->core->memoryOrder ne $vcore->config->pr->xt->core->memoryOrder;
	    $warn_prid = 1 unless $core->config->pr->xt->usePRID and $vcore->config->pr->xt->usePRID;
	    if ($vec->size != $vvec->size) {
		$vec_end = $vvec_end if $vvec_end > $vec_end;	
		$layout->warn("shared vectors have different sizes, using largest (", $vec_end - $vec_vaddr + 1, " bytes) for $what and $vwhat");
	    }
	    push @shared, $v;
	    @vecs = grep($_ ne $v, @vecs);
	    $sharedcores{$vcore} = 1;
	}
	$what = $vec->name;		
	if (@shared > 1) {
	    my @shared_cores = map($_->[0], @shared);
	    push @shared_vector_sets, [$vec->name, $vec_vaddr, $vec_end, \@shared_cores, $vec_mem];
	    my $whatshared;
	    if (@shared == scalar(@{[$layout->cores]})) {
		$whatshared = " shared among all cores";
	    } else {
		$whatshared = " shared among cores ".join(",", map($_->name, @shared_cores));
	    }
	    $what .= $whatshared;
	    if ($warn_endian) {
		$layout->warn("sharing vectors of cores of different endianness is not supported: $what");
	    }
	    if ($vec->name ne "ResetVector" and $vec->name ne "MemoryExceptionVector") {
		$layout->warn("sharing vectors other than reset or mem-error (not having relocatable vectors) is not supported: $what");
	    }
	    if ($warn_prid) {
		if ($vec->name eq "ResetVector") {
		    $layout->warn("sharing reset vectors without PRID isn't supported:  there is no default shared handler, reset vector will be empty unless you provide your own: $what");
		} else {
		    $layout->warn("sharing vectors without PRID configured isn't supported:  default handlers are not likely to work:  $what");
		}
	    }
	    $layout->dprintf("#### Adding section for $what\n");
	    foreach my $v (@shared) {
		my ($vcore, $vvec, $vwhat, $vseg, $vvaddr) = @$v;
		$shared_vectors{$vcore->name}{$vvec->name} = [$whatshared, \@shared_cores];
	    }
	}
    }
    return (\@shared_vector_sets, \%shared_vectors);
}
sub setup_shared_reset_tables {
    my ($layout, $shared_vector_sets) = @_;
    my @constructs;
    my $shared_reset_table_size = scalar(@{[$layout->cores]}) * 4;		
    my %shared_reset_mems = ();
    foreach my $shared_vec (@$shared_vector_sets) {
	my ($vec_name, $vec_vaddr, $vec_end, $vec_cores, $vec_mem) = @$shared_vec;
	next unless $vec_name eq "ResetVector";
	$shared_reset_mems{$vec_mem} = $vec_mem;
    }
    foreach my $vec_mem_index (sort keys %shared_reset_mems) {
	my $vec_mem = $shared_reset_mems{$vec_mem_index};	
	my @vec_cores = map(@{$_->[3]}, grep($_->[4] eq $vec_mem, @$shared_vector_sets));
	my $vec_coreset = $layout->normalize_set_of_cores( [map($_->name, @vec_cores)] );
	my $forcores = ((keys %shared_reset_mems) > 1 ? "in memory ".$vec_mem->name." " : "");
	$forcores .= "for cores ".join(",", map($_->name, @vec_cores));
	my $setname = $layout->name_of_set_of_cores($vec_coreset);
	my $mpu = scalar(grep($_->config->pr->xt->immu, @vec_cores));
if(0){
	push @constructs, {
			name => 'shared_reset_table_'.$setname,
			construct => 'segment',
			linkmaps => [ map($_->linkmap, @vec_cores) ],
			startalign => 4,
			memory_byaddr => '.SharedResetVector.vaddr',
			size => $shared_reset_table_size,
			requirement => ($mpu ? "1" : "!\@cached"),	
			prefer  => "\@shared",
			ordering => "-1",			
			description => "shared reset table $forcores",
			assignments => { "_ResetTable_base" => "\$start" },
			};
}else{
	push @constructs, {
			name => 'find_shared_reset_table_'.$setname,
			construct => 'segment',
			linkmaps => [ map($_->linkmap, @vec_cores) ],
			startalign => 4,
			memory_byaddr => '.SharedResetVector.vaddr',
			minsize => $shared_reset_table_size + 3,
			maxsize => -1 * ($shared_reset_table_size + 3),
			requirement => ($mpu ? "1" : "!\@cached"),	
			prefer  => "\@shared",			
			description => "space for shared reset table $forcores",
			assignments => { ".${setname}._ResetTable_base.segstart" => "\$start" },
			};
	push @constructs, {
			name => 'shared_reset_table_'.$setname,
			construct => 'segment',
			linkmaps => [ map($_->linkmap, @vec_cores) ],
			startaddr => sprintf("(.${setname}._ResetTable_base.segstart+3)&~3",),
			endaddr   => sprintf("(.${setname}._ResetTable_base.segstart+3)&~3+0x%x", $shared_reset_table_size - 1),
			description => "shared reset table $forcores",
			assignments => { "_ResetTable_base" => "\$start" },
			};
}
	my @cores = $layout->cores;
	foreach my $i (0 .. $#cores) {
	    my $core = $cores[$i];
	    next unless grep($_ eq $core, @vec_cores);
	    push @constructs, {
			name => 'shared_reset_table_entry_'.$core->name,
			construct => 'segment',
			_linkmap => $core->linkmap,
			startaddr => sprintf("_ResetTable_base+0x%x", $i*4),
			endaddr   => sprintf("_ResetTable_base+0x%x", $i*4+3),
			namespace => $core->name,
			description => "entry $i (for core ".$core->name.") of reset handler table $forcores",
			attrs => {lock => "entry $i (for core ".$core->name.") of reset handler table $forcores"},
			sections => [ ".ResetTable.rodata:first:4" ],
			};
	}
    }
    return @constructs;
}
sub instantiateSystemLayout {
    my ($layout, %options) = @_;
    $layout->{system_name} = ($options{system_name} // "unnamed_system");
    $layout->{logfile} = $options{extralog} // $layout->{logfile};	
    my %linkmap_options = %{$options{linkmap_options} // {}};
    $layout->dprintf( "### In instantiateSystemLayout\n" );
    my @constructs = @{$options{constructs} // []};
    my @option_constructs = array_extract { $_->{construct} eq "options" } @constructs;
    foreach my $c (@option_constructs) {
	$layout->{system_name} = $c->{system_name} if defined($c->{system_name});
	$options{create_local_memories} = $c->{create_local_memories} if defined($c->{create_local_memories});
	$options{create_memories} = $c->{create_memories} if defined($c->{create_memories});
	$options{create_shared_linkmaps} = $c->{create_shared_linkmaps} if defined($c->{create_shared_linkmaps});
	%linkmap_options = (%linkmap_options, %{$c->{linkmap_options}}) if defined($c->{linkmap_options});
    }
    my @core_constructs = array_extract { $_->{construct} eq "core" } @constructs;
    $layout->apply_constructs(\@core_constructs, 0);
    scalar(@{[$layout->cores]}) or Error("need at least one core!");
    foreach my $core ($layout->cores) {
	$layout->dprintf("############>> ".$core->name." ($core)\n");
	my $hw_maps = ($core->{hw_maps} // "xtensa-core/hw-maps");
	push @constructs, $layout->read_xld($hw_maps, $core->name, $core)
		unless $hw_maps eq "";
    }
    my @namedset_constructs = array_extract { $_->{construct} eq "named_set" } @constructs;
    $layout->apply_constructs(\@namedset_constructs, 0);
    $layout->apply_constructs(\@constructs, 0);
    if ($options{create_local_memories} // 1) {
	$layout->create_local_memories;
    }
    $layout->create_default_system_memories($options{create_memories});
    $layout->dprintf("\n### INITIAL SYSTEM:\n%s\n\n", scalar($layout->format_addressables));
    $layout->dprintf("### instantiateSystemLayout:  Done!\n");
}
sub instantiateLinkmaps {
    my ($layout, %options) = @_;
    $layout->{logfile} = $options{extralog} // $layout->{logfile};	
    my %linkmap_options = %{$options{linkmap_options} // {}};
    $layout->dprintf( "### In instantiateLinkmaps\n" );
    my @constructs = @{$options{constructs}};
    my @option_constructs = array_extract { $_->{construct} eq "options" } @constructs;
    foreach my $c (@option_constructs) {
	$options{create_shared_linkmaps} = $c->{create_shared_linkmaps} if defined($c->{create_shared_linkmaps});
	$options{ignore_missing_vectors} = $c->{ignore_missing_vectors} if defined($c->{ignore_missing_vectors});
	%linkmap_options = (%linkmap_options, %{$c->{linkmap_options}}) if defined($c->{linkmap_options});
    }
    my @mm_constructs;
    foreach my $core ($layout->cores) {
	my $default_maps = ($core->{default_maps} // "xtensa-core/default-maps");
	push @mm_constructs, $layout->read_xld($default_maps, $core->name, $core)
		unless $default_maps eq "";
    }
    $layout->apply_constructs(\@mm_constructs, 0);	
    push @constructs, @mm_constructs;			
    my @linkmap_constructs = array_extract { $_->{construct} eq "linkmap" } @constructs;
    $layout->apply_constructs(\@linkmap_constructs, 1);
    my $create_shared_linkmaps = ($options{create_shared_linkmaps} // 0);
    if (!@{$layout->{requested_linkmaps}}) {
	push @{$layout->{requested_linkmaps}},
	    map({name => $_->name, cores => [$_]}, $layout->cores);
	$create_shared_linkmaps = 1;
    }
    my ($commonmap, $linkmaps) = $layout->create_all_linkmaps($create_shared_linkmaps, %linkmap_options);
    $layout->{commonmap} = $commonmap;
    foreach my $linkmap (@$linkmaps) {
	my $map = $linkmap->ref_map;
	@$map = grep(!($_->is('local') && $_->is('delay')), @$map);
    }
    foreach my $linkmap (@$linkmaps) {
	my $cores = $linkmap->{cores};
	my $name = $linkmap->name;
	$name =~ s/\.linkmap$//;
	my $core = $cores->[0];		
	my $sw_sections = ($core->{sw_sections} // "xtensa-core/sw-sections");
	push @constructs, $layout->read_xld($sw_sections, $name, $core)
		unless $sw_sections eq "";
    }
    $layout->process_partition_requests(\@constructs);
    push @constructs, {construct => "_partition", name => "partition"};	
    my ($shared_vector_sets, $shared_vectors) = ([], {});
    ($shared_vector_sets, $shared_vectors) = $layout->find_shared_vectors
	if scalar(@{[$layout->cores]}) > 1;	
    $layout->{shared_sets} = $shared_vector_sets;	
    my %core_vecs;
    foreach my $core ($layout->cores) {
	my @vecs = $core->config->pr->vectors->swVectorList;
	if (defined(($shared_vectors->{$core->name} // {})->{"ResetVector"})) {
	    my ($resetvec) = grep($_->name eq "ResetVector", @vecs);
	    push @vecs, bless( { name => 'ResetHandler', group => '_nonvector_',
			     baseAddress => 0, offset => 0, size => $resetvec->size,
			   }, 'HashObject' );
	}
	$core_vecs{$core->name} = \@vecs;
    }
    my %core_vec_linkmaps;	
    my %linkmap_reloc_cores;
    foreach my $linkmap (@$linkmaps) {
	my $vectors = $linkmap->{vectors};
	next unless defined($vectors);
	my $lmname = $layout->linkmap_name($linkmap);
	foreach my $vname (@$vectors) {
	    my $vecname = $vname;
	    my @cores = @{$linkmap->cores};
	    if ($vname =~ /^(.*)\.([^\.]+)$/) {		
		my $corename = $1;
		$vecname = $2;
		@cores = grep($_->name eq $corename, @cores);
		@cores or Error("unknown core name '%s' in linkmap %s list of requested vectors (%s)",
			$corename, $lmname, $vname);
	    }
	    my $found = 0;
	    foreach my $core (@cores) {
		foreach my $vec (@{ $core_vecs{$core->name} }) {
		    next unless $vec->name eq $vecname or $vec->group eq $vecname;
		    $core_vec_linkmaps{$core->name}{$vec->name}{$lmname} = $linkmap;
		    if ($vec->group eq 'relocatable') {
			$core_vec_linkmaps{$core->name}{$vec->group}{$lmname} = $linkmap;
			$linkmap_reloc_cores{$lmname}{$core->name} = $core;
		    }
		    $found++;
		}
	    }
	    $found or $layout->warn("ignoring vector name '%s' (not a vector in core(s) %s) in linkmap %s list of requested vectors (%s)",
				$vecname, join(", ", map($_->name, @cores)), $lmname, $vname);
	}
    }
    foreach my $core ($layout->cores) {
	my $corename = $core->name;
	my $vec_linkmaps = ($core_vec_linkmaps{$corename} // {});
	my @missing_vecs;
	foreach my $vec (@{$core_vecs{$corename}}) {
	    my @linkmap_names = keys %{ $vec_linkmaps->{$vec->name} // {} };
	    next if @linkmap_names;
	    if (defined($core->linkmap) and !defined($core->linkmap->{vectors})) {
		my $lmname = $layout->linkmap_name($core->linkmap);
		$core_vec_linkmaps{$corename}{$vec->name}{$lmname} = $core->linkmap;
		if ($vec->group eq 'relocatable') {
		    $core_vec_linkmaps{$corename}{$vec->group}{$lmname} = $core->linkmap;
		    $linkmap_reloc_cores{$lmname}{$corename} = $core;
		}
	    } else {
		push @missing_vecs, $vec->name;		
	    }
	}
	my $ignore_missing_vecs = $core->{ignore_missing_vectors} //
				  ($options{ignore_missing_vectors} // 0);
	if (@missing_vecs and !$ignore_missing_vecs) {
	    $layout->warn("segments/sections for vector(s) %s of core %s are not requested to be placed in any linkmap; if intentional (such as because they are placed separately or not needed), silence this warning using ignore_missing_vectors:1 in either core %s or in global options",
		join(",",@missing_vecs), $corename, $corename);
	}
    }
    my @linkmap_scan = keys %linkmap_reloc_cores;
    while (my $lmname = shift @linkmap_scan) {
	my $relcores = $linkmap_reloc_cores{$lmname};
	my @corenames = keys %$relcores;
	next unless @corenames > 1;
	my %linkmaps;
	foreach my $corename (@corenames) {
	    %linkmaps = (%linkmaps, %{$core_vec_linkmaps{$corename}{'relocatable'}});
	}
	foreach my $corename (@corenames) {
	    foreach my $lmname2 (keys %linkmaps) {
		next if defined($linkmap_reloc_cores{$lmname2}{$corename});
		$layout->dprintf("Expanding reloc vectors of core $corename from linkmap $lmname to linkmap $lmname2\n");
		$linkmap_reloc_cores{$lmname2}{$corename} = $relcores->{$corename};
		$core_vec_linkmaps{$corename}{'relocatable'}{lmname2} = $linkmaps{$lmname2};
		push @linkmap_scan, $lmname2 unless grep($_ eq $lmname2 ,@linkmap_scan);
	    }
	}
    }
    my %seen;		
    foreach my $corename (sort keys %core_vec_linkmaps) {
	next if exists($seen{$corename}{'relocatable'});
	my $reloc_linkmapnames = $layout->normalize_set_of_linkmaps(
		[ keys %{ $core_vec_linkmaps{$corename}{'relocatable'} // {} } ]);
	next unless @$reloc_linkmapnames;
	my @reloc_cores = sort { $a->index <=> $b->index } values %{ $linkmap_reloc_cores{$reloc_linkmapnames->[0]} };
	next unless @reloc_cores > 1;		
	my %vecs;	
	foreach my $core (@reloc_cores) {
	    my $vec_linkmaps = ($core_vec_linkmaps{$core->name} // {});
	    foreach my $vec ($core->config->pr->vectors->swVectorList) {
		my $linkmapnames = $layout->normalize_set_of_linkmaps([ keys %{ $vec_linkmaps->{$vec->name} // {} } ]);
		next unless @$linkmapnames;	
		my @overlap = grep(($_ ne $vec->name
				    && $vecs{$_}->offset <= $vec->offset + ($vec->size - 1)
				    && $vecs{$_}->offset + ($vecs{$_}->size - 1) >= $vec->offset),
					sort keys %vecs);
		if (@overlap) {
		    my $ovname = $overlap[0];
		    my $ovconfig = $vecs{$ovname}{configname};
		    Error("relocatable vectors of cores %s are shared (mapped to the same location) because they were placed in the same linkmap(s) %s, however they are not compatible:  for example, vector %s of core %s overlaps with vector %s of core(s) %s",
			join(",", map($_->name, @reloc_cores)), join(" + ", @$reloc_linkmapnames),
			$vec->name, $core->name,
			$ovname, join(",", map($_->name, @{$vecs{$ovname}{cores}})));
		}
		my $vecinfo = $vecs{$vec->name};
		if (defined($vecinfo)) {
		    if ($vecinfo->offset != $vec->offset) {
			Error("relocatable vectors of cores %s are shared (mapped to the same location) because they were placed in the same linkmap(s) %s, however they are not compatible:  for example, vector %s of core %s and core(s) %s have different offsets 0x%x and 0x%x",
				join(",", map($_->name, @reloc_cores)), join(" + ", @$reloc_linkmapnames),
				$vec->name, $core->name, join(",", map($_->name, @{$vecinfo->{cores}})),
				$vec->offset, $vecinfo->offset);
		    }
		    $vecinfo->{size} = $vec->size if $vec->size > $vecinfo->size;
		    push @{$vecinfo->{cores}}, $core;
		    $vecinfo->{linkmapnames} = $layout->normalize_set_of_linkmaps([ @{$vecinfo->linkmapnames}, @$linkmapnames ]);
		} else {
		    $vecs{$vec->name} = bless( { offset => $vec->offset, size => $vec->size, group => 'relocatable',
						 baseAddress => $vec->offset,	
						 cores => [$core], linkmapnames => $linkmapnames },
					       'HashObject' );
		}
	    }
	}
	layout->info("Sharing relocatable vectors of cores %s at the same location, across linkmap(s) %s",
		join(", ", map($_->name, @reloc_cores)), join(", ", @$reloc_linkmapnames));
	foreach (@reloc_cores) { $seen{$_->name}{'relocatable'} = 1; }
	my @vector_constructs = setupSharedVectorLayout(
		[ map( [$_->name, $_->config->pr], @reloc_cores) ],
		0, \%vecs, $reloc_linkmapnames
		);
	push @constructs, @vector_constructs;
    }
    foreach my $shared_vec (@$shared_vector_sets) {
	my ($vecname, $vec_vaddr, $vec_end, $cores, $vec_mem) = @$shared_vec;
	my %linkmapset;
	foreach my $core (@$cores) {
	    $seen{$core->name}{$vecname} = 1;
	    %linkmapset = (%linkmapset, %{ $core_vec_linkmaps{$core->name}{$vecname} });
	}
	my $linkmapnames = $layout->normalize_set_of_linkmaps([keys %linkmapset]);
	if (!@$linkmapnames) {
	    $layout->info("Skipping vector %s shared among cores %s, requested not to be included in any linkmap",
			    $vecname, join(",", map($_->name, @$cores)));
	    next;
	}
	my ($onerealvec) = grep($_->name eq $vecname, $cores->[0]->config->pr->vectors->swVectorList);
	my $combined_vector = bless( { baseAddress => $vec_vaddr, offset => $onerealvec->offset,
					size => ($vec_end - $vec_vaddr) + 1, group => 'stationary',
					cores => $cores, linkmapnames => $linkmapnames },
				     'HashObject' );
	my @vector_constructs = setupSharedVectorLayout(
		[ map( [$_->name, $_->config->pr], @$cores) ],
		0, { $vecname => $combined_vector }, [] );
	push @constructs, @vector_constructs;
    }
    foreach my $core ($layout->cores) {
	my $corename = $core->name;
	my $vec_linkmaps = ($core_vec_linkmaps{$corename} // {});
	my $corevecs = $core_vecs{$corename};
	my %vecs;
	my $include_relocs = 0;
	foreach my $vecname (sort keys %$vec_linkmaps) {
	    next if $vecname eq "relocatable"; # or $vecname eq "stationary";
	    my ($vec) = grep($_->name eq $vecname, @$corevecs);
	    defined($vec) or InternalError("did not find vector $vecname in core $corename");
	    next if $seen{$corename}{$vecname};		
	    my $linkmapnames = $layout->normalize_set_of_linkmaps([keys %{ $vec_linkmaps->{$vecname} // {} }]);
	    $layout->dprintf("Issuing core %s vector %s to linkmaps %s\n", $corename, $vecname, join(", ", @$linkmapnames));
	    $include_relocs = 1 if $vec->group eq "relocatable";
	    $vecs{$vecname} = bless( { baseAddress => $vec->baseAddress, offset => $vec->offset,
	    				size => $vec->size, group => $vec->group,
					cores => [$core], linkmapnames => $linkmapnames },
				     'HashObject' );
	}
	my @vector_constructs = setupSharedVectorLayout( [[$corename, $core->config->pr]],
		0, \%vecs, $include_relocs ? undef : [] );
	$layout->assign_constructs_to_namespace($core->name, \@vector_constructs);
	push @constructs, @vector_constructs;
    }
    push @constructs, $layout->setup_shared_reset_tables($shared_vector_sets);
    $layout->apply_constructs(\@constructs, 1);
    foreach my $linkmap (@$linkmaps) {
	$layout->dprintf("\n#######  LINKMAP ".$linkmap->name." AFTER CUTOUT:\n%s\n", $linkmap->dump);
	$linkmap->cleanup_map();
	$layout->dprintf("#######  LINKMAP ".$linkmap->name." AFTER DELETE:\n%s\n", $linkmap->dump);
    }
    foreach my $sym (keys %{$layout->symbols}) {
	my $coresym = $sym;
	$coresym =~ s/^([^.]+)\.// or next;	
	my $name = $1;
	my $core = $layout->cores_byname->{$name};
	next unless defined($core);
	$core->linkmap->symbols->{$coresym} = $layout->symbols->{$sym};
    }
    $layout->dprintf("### instantiateLinkmaps:  Done!\n");
}
sub literal_preference {
    my ($m, $mtext, $pr) = @_;
    my $literals_in_iram = $ENV{"XTENSA_LITERALS_IN_IRAM"};
    return 0 unless $m->is('readable');		
    my $adjust = 0;
    if (!$m->is('data') and $m->is('executable')) {	
	if (defined($pr)) {
	    return 0 if $pr->is_xea1;
	    if (!(defined($literals_in_iram) and ($literals_in_iram eq "2"))) {
	        return 10 if $pr->TargetHW_EarliestVNum < 250000 and $pr->loadStoreUnitsCount >= 2;
	        return 10 unless $pr->core->L32RnoReplay;
	        return 10 if ($m->addr & 0xE0000000) != ($mtext->endaddr & 0xE0000000)
		       and defined($pr->mmu) and $pr->mmu->is_caxlt;
	    }
	}
	if (defined($literals_in_iram) and (($literals_in_iram eq "1") or ($literals_in_iram eq "2"))) {
	    $adjust = 1;
	     }
	else {
	   $adjust = -1;
	     }
	}
    return 20 if $m->is('type') eq 'dataPort';
    return 50+$adjust if $m->is('writable') == $mtext->is('writable')
		     and $m->is('local') == $mtext->is('local');
    return 40+$adjust if $m->is('writable');
    return 30+$adjust;
}
sub literal_for_text {
    my ($linkmap, $pr, $textSeg, $minsize, $create_size, $mix_ok, $textSec) = @_;
    my $created = 0;
    $create_size = 0 unless defined($create_size);
    if ($create_size) {
	$create_size = ($create_size + 3) & ~3;	
    }
    if (defined($minsize)) {
	$minsize = 4 if $minsize <= 0;
	$minsize = ($minsize + 3) & ~3;		
    } else {
	$minsize = 4;				
    }
    $create_size = $minsize if $create_size != 0 and $create_size < $minsize;
    my $text_endaddr = defined($textSec)
    	? $textSeg->addr + (defined($textSec->minsize) ? $textSec->minsize : 0) + 0x100 - 1
	: $textSeg->endaddr;
    my $minaddr = ($text_endaddr - 0x40000 + 1 + 3) & ~3;
    $minaddr = 0 if $minaddr < 0 or $minaddr > $textSeg->endaddr;	
    my $textpref = literal_preference($textSeg, $textSeg, $pr);
    my $litdebug = 0;
    print STDERR "NOTE: search lits for ", $textSeg->format_self, " (textSec=", (defined($textSec)?$textSec->name:"(none)"), ", minsize=$minsize, create_size=$create_size, textpref=$textpref, minaddr=", sprintf("0x%x", $minaddr), ")\n" if $litdebug;
    my $best = undef;
    my $bestpref = 0;
    my $bestcut = undef;
    my $bestmerge = undef;
    if ($mix_ok) {				
	$best = $textSeg;
	$bestpref = $textpref + 5;		
	$bestcut = $textSeg->addr;
    }
    my @map = $linkmap->map->a;			
    foreach my $i (0 .. $#map) {
	my $seg = $map[$i];
	next if $seg->is('lock') or $seg->is('exception') or $seg->is('device');	
	next if $seg->addr >= $textSeg->addr;		
	my $pref = literal_preference($seg, $textSeg, $pr);	
	next if $pref == 0;				
	next if $mix_ok and $pref <= $textpref;		
	my $inrange = $seg->endaddr - $minaddr + 1;
	$inrange = $seg->sizem1+1 if $inrange > $seg->sizem1+1;
	my ($seg_minsize, $seg_minleft, $seg_minright, $seg_createsize, $seg_minaddr, $seg_maxendaddr) = $seg->sumSections;
	my $available = $seg->sizem1 + 1;
	$available -= $seg_minsize; 
	$available = $inrange if $available > $inrange;
	my $minaddrav = ($seg->endaddr - $available +1+3) & ~3;	
	$available = $seg->endaddr - $minaddrav + 1;		
	if ($available <= 0) {			
	    print STDERR "NOTE: no avail bytes in segment ", $seg->format_self, " (seg_minsize=", $seg_minsize, ", inrange=$inrange, minaddr=", sprintf("0x%x", $minaddr), ")\n" if $litdebug;
	    next;
	}
	my $cutaddr;
	if ($inrange > $seg->sizem1) {		
	    $pref += 5;				
	    $cutaddr = $seg->addr;		
	} else {
	    next unless $create_size;
	    my $cutsize = $seg->endaddr - (($seg->endaddr - $create_size + 1) & ~0xF) + 1;
	    $cutsize = $available if $cutsize > $available;
	    $cutaddr = $seg->endaddr - $cutsize + 1;
	    $cutaddr &= ~3;			
	    $cutaddr = $minaddrav if $cutaddr < $minaddrav;	
	}
	my $mergenext = undef;
	if ($create_size and $i < $#map) {
	    my $next = $map[$i+1];
	    my ($next_minsize, $next_minleft, $next_minright, $next_createsize, $next_minaddr, $next_maxendaddr) = $seg->sumSections;
	    if (!$next->is('lock') and !$next->is('exception') and !$next->is('device')	
		and defined($next_minaddr)
		and $next_minaddr <= $cutaddr
		and $seg->map2one($next)
		and $seg->merge_ok_nomin($next)) {
		$available += $next->sizem1 + 1;
		$available -= $next_minsize; 
		$mergenext = $next;
	    }
	}
	if ($available < $minsize) {		
	    next;
	}
	next unless $create_size or $cutaddr == $seg->addr;	
	#printf STDERR "### 0x%x..0x%x in L32R range of 0x%x..0x%x"
	if ($pref > $bestpref					
	      or ($pref == $bestpref && $best ne $textSeg)) {	
	    $best = $seg;
	    $bestpref = $pref;
	    $bestcut = $cutaddr;
	    $bestmerge = $mergenext;
	}
    }
    if ($bestpref <= 0) {
	print STDERR "NOTE: nothing found\n" if $litdebug;
	return (undef, 0);		
    }
    if ($best eq $textSeg) {	
	print STDERR "NOTE: returning text seg\n" if $litdebug;
	return ($textSeg, 0);
    }
    if ($best->addr != $bestcut) {
	$best = $best->cut($bestcut, undef);
	$created = 1;
    }
    if (defined($bestmerge)) {
	print STDERR ">>>>  Merging $bestmerge into $best\n" if $litdebug;
	$best->merge($bestmerge) and InternalError("Oops didn't merge");
	$created = 1;
    }
    if ($best->addr == $bestcut) {	
	#printf STDERR "##### Cut aliases from 0x%x..0x%x\n", $best->addr, $best->endaddr;
    }
    print STDERR "NOTE: found ", $best->format_self, "\n" if $litdebug;
    return ($best, $created);
}
sub text_to_literal_name {
    local ($_) = @_;	
    s/\.text$/.literal/
    or
    s/^\.gnu\.linkonce\.t\./.gnu.linkonce.literal./
    or
    s/$/.literal/;
    return $_;
}
1;
package Xtensa::System::Attributes;
our $VERSION = "1.0";
our @ISA = qw(HashObject);
use strict;
our $debug = 0;
sub has_attr {
    my ($self, $attr, $default) = @_;
    $default = 0 unless defined($default);
    my $attrs_ref = $self->ref_attributes;
    return exists($attrs_ref->{$attr}) ? $attrs_ref->{$attr} : $default;
}
sub is { has_attr(@_); }
sub set_attr {
    my ($self, %attribs) = @_;
    while (my ($attr, $value) = each %attribs) {
	${$self->ref_attributes}{$attr} = $value;
    }
}
sub multi_set_attr {
    my ($selves, %attribs) = @_;
    foreach my $self (@$selves) {
	$self->set_attr(%attribs);
    }
}
sub del_attr {
    my ($self, @attrs) = @_;
    foreach my $attr (@attrs) {
	delete ${$self->ref_attributes}{$attr};
    }
}
sub is_accessible {
    my ($self) = @_;
    return 0 if $self->is('exception') or $self->is('isolate');
    return 1;
}
sub attr_diff {
    my ($self, $a, $b, $ignoreset) = @_;
    my %diff;
    foreach my $attr (sort keys %$a) {
	next if exists($ignoreset->{$attr});
	if (exists($$b{$attr})) {
	    my ($va,$vb) = ($$a{$attr}, $$b{$attr});
	    $diff{$attr} = "d" unless ($va // '-undef-') eq ($vb // '-undef-');
	} else {
	    $diff{$attr} = "a";
	}
    }
    foreach my $attr (sort keys %$b) {
	next if exists($ignoreset->{$attr});
	$diff{$attr} = "b" unless exists($$a{$attr});
    }
    return %diff;
}
sub attr_equivalent {
    my ($self, $a, $b, $ignoreset) = @_;
    my %diff = $self->attr_diff($a, $b, $ignoreset);
    return (@{[keys %diff]} ? 0 : 1);
    foreach my $attr (sort keys %$a) {
	next if exists($ignoreset->{$attr});
	return 0 unless exists($$b{$attr});		
	my ($va,$vb) = ($$a{$attr}, $$b{$attr});
	return 0 unless ($va // '-undef-') eq ($vb // '-undef-');	
    }
    foreach my $attr (sort keys %$b) {
	next if exists($ignoreset->{$attr});
	return 0 unless exists($$a{$attr});		
    }
    return 1;				
}
sub namesplit {
    my ($name) = @_;
    my @result;
    while ($name =~ s/((?>[^()|]+)?(\(((?>[^()]+)|(?-2))*\)))*(?>[^()|]+)?//) {
	push @result, $&;
	last unless $name =~ s/^|//;
    }
    $name ne "" and Error("oops namesplit failed, this was left:  {$name}");
    return [@result];
}
sub namejoin_adjacent {
    my ($namea, $nameb);
    my @a = namesplit($namea);
    my @b = namesplit($nameb);
    pop @a if scalar(@a) and scalar(@b) and $a[-1] eq $b[0];
    return join("|", @a, @b);
}
sub namejoin_contains {
    my ($namea, $nameb);
    $namea = "($namea)" if namesplit($namea) > 1;
    $nameb = "($nameb)" if namesplit($nameb) > 1;
    return $namea . "." . $nameb;
}
our %attr_handling = (
	readable   => [sub { $a & $b }],
	writable   => [sub { $a & $b }],
	executable => [sub { $a & $b }],
	data       => [sub { $a & $b }],
);
sub attr_combine {
    my ($self, $a, $b, $contains) = @_;
    my %attrs = %$a;
    foreach my $attr (sort keys %$b) {
	my $va = $attrs{$attr};		
	my $vb = $b->{$attr};		
	if (!defined($va)) {
	    $attrs{$attr} = $vb;
	    next;
	}
	next unless defined($vb);
	if ($contains) {
	    if ($attr eq "delay") {
		$attrs{$attr} += $vb;	
		next;
	    }
	    if ($attr eq "name") {
		$attrs{$attr} = namejoin_contains($va, $vb);
		next;
	    }
	}
	next unless $va ne $vb;		
	if ($attr =~ /^(executable|readable|writable|data)$/) {
	    $attrs{$attr} = 0;		
	} elsif ($attr =~ /^(exception|device)$/) {
	    $attrs{$attr} = 1;		
	} elsif ($attr eq "delay") {
	    $attrs{$attr} = $vb if $vb > $va;	
	} elsif ($attr eq "name") {
	    $attrs{$attr} = namejoin_adjacent($va, $vb);
	} else {
	    ::Error("don't know how to resolve conflicting values for attribute '$attr' ($va ".($contains ? "contains" : "adjacent to")." $vb)");
	}
	print STDERR "attr_combine: WARNING: conflicting values for attribute '$attr' ($va, $vb), setting to ".$attrs{$attr}."\n"
	    if $debug;
    }
    return \%attrs;
}
sub format_attributes {
    my ($self) = @_;
    my $keywords = "";
    foreach my $attr (sort keys %{$self->ref_attributes}) {
	my $value = $self->ref_attributes->{$attr};
print STDERR "*WARNING: ATTR $attr has undefined value\n" if $debug and !defined($value);
	next unless defined($value);
	next if $attr eq "background";	
    	$keywords .= " $attr='";
	$keywords .= ($value =~ /^\d/)
			? (($value >= 0 && $value < 10) ? $value : sprintf("0x%x",$value))
			: $value;
	$keywords .= "'";
    }
    $keywords;
}
sub parse_attributes {
    my ($self, $lineref) = @_;
    my %attrs = ();
    while ($$lineref =~ s/^(\w+)='([^']*)'\s+//) {
	my ($attr, $value) = ($1, $2);
	$value = Xtensa::AddressLayout::strtoint($value, "parse_attributes") if $value =~ /^\d/;
	$attrs{$attr} = $value;
    }
    \%attrs;
}
sub classical_format_attributes {
    my ($self) = @_;
    my @keywords = ();
    foreach my $attr (sort keys %{$self->ref_attributes}) {
	next if $attr =~ /^(asid|ca|type|check)$/;	
	next if $attr eq "background";	
	my $value = $self->ref_attributes->{$attr};
	next unless defined($value) and $value ne '0';
	if ($value eq '1') {
	    push @keywords, $attr;
	} else {
	    push @keywords, $attr . "=" . (($value =~ /^\d/)
		? (($value >= 0 && $value < 10) ? $value : sprintf("0x%x",$value))
		: $value);
	}
    }
    return join(" , ", @keywords);
}
sub cabits_to_attrs {
    my ($cattr, $fbits, $lbits, $sbits) = @_;
    my %attrs = ();
    my $excepts = 0;
    my $Exception = 0x001;	
    my $HitCache  = 0x002;	
    my $Allocate  = 0x004;	
    my $WriteThru = 0x008;	
    my $Isolate   = 0x010;	
    my $Guard     = 0x020;	
    $attrs{'executable'} = undef;
    $attrs{'readable'} = undef;
    $attrs{'writable'} = undef;
    $attrs{'cached'} = undef;
    $attrs{'isolate'} = undef;
    $attrs{'exception'} = undef;
    $attrs{'device'} = undef;
    $attrs{'ca'} = $cattr;
    if (($fbits & $Exception) != 0) {
	$attrs{'executable'} = 0;	# can't fetch if Exception
	$excepts++;
    } elsif (($fbits & $Isolate) != 0) {
	$attrs{'isolate'} = 1;
    } else {
	$attrs{'executable'} = 1;
	$attrs{'cached'} = 1 if ($fbits & $HitCache) != 0;
    }
    if (($lbits & $Exception) != 0) {
	$attrs{'readable'} = 0;		# can't load if Exception
	$excepts++;
    } elsif (($lbits & $Isolate) != 0) {
	$attrs{'isolate'} = 1;
    } else {
	$attrs{'readable'} = 1;
	$attrs{'cached'} = 1 if ($lbits & $HitCache) != 0;
    }
    if (($sbits & $Exception) != 0) {
	$attrs{'writable'} = 0;		# can't store if Exception
	$excepts++;
    } elsif (($sbits & $Isolate) != 0) {
	$attrs{'isolate'} = 1;
    } else {
	$attrs{'writable'} = 1;
	$attrs{'cached'} = 1 if ($sbits & $HitCache) != 0;
    }
    $attrs{'exception'} = 0 if $excepts == 0;
    $attrs{'exception'} = 1 if $excepts == 3;
    $attrs{'cached'} = 0 unless $attrs{'cached'} or $attrs{'isolate'} or $excepts == 3;
    $attrs{'isolate'} = 0 unless $attrs{'isolate'} or $excepts == 3;
    $attrs{'device'} = 1 if $attrs{'isolate'};	# in isolate mode, can't have memory properties
    \%attrs;
}
1;
package Xtensa::System::Addressable;
our $VERSION = "1.0";
our @ISA = qw(HashObject Xtensa::System::Attributes);
use strict;
sub new {
    my ($class, $sizem1, $target, $attributes) = @_;
    ref($attributes) eq "HASH" or ::InternalError("new Addressable $class: attributes must be a hash");
    my $addressable = {
	name		=> undef,
	sizem1		=> $sizem1,
	target		=> $target,
	attributes	=> $attributes,
	layout		=> undef,
	alwaysin	=> undef,
    };
    bless ($addressable, $class);	
    return $addressable;		
}
sub dup {
    my ($addressable) = @_;
    my $newself = { %$addressable };
    $newself->{attributes} = { %{$addressable->attributes} };	
    bless ($newself, ref($addressable));	
    return $newself;			
}
sub new_parse {
    my ($class, $layout, $namespace, $lines) = @_;
    $_ = shift @$lines;
    s/^\s*\<\w+\s+name='([^']+)'\s+sizem1='([^']+)'\s+//
	or ::Error("addressable: couldn't parse start of line: <<$_>>");
    my ($name, $sizem1) = ($1, $2);
    $sizem1 = Xtensa::AddressLayout::strtoint($sizem1, "Addressable::new_parse sizem1");
    my $attrs = $class->parse_attributes(\$_);
    /^\/\>\s*$/
	or ::Error("addressable: bad end of line, expected '/>' : <<$_>>");
    my $a = $class->new($sizem1, undef, $attrs);
    $layout->add_addressable($namespace.($namespace?".":"").$name, $a);	
    return $a;
}
sub fullname {
    my ($addressable) = @_;
    $addressable->name;
}
sub format_keywords {
    my ($addressable, $define_it) = @_;
    return sprintf("name='%s'", defined($addressable->fullname) ? $addressable->fullname : "(undef)")
	   . ($define_it ? sprintf(" sizem1='0x%08x'", $addressable->sizem1) : "");
}
sub format_self {
    my ($addressable, $define_it, $showmaps) = @_;
    return ("  <memdev ".$addressable->format_keywords(1).$addressable->format_attributes()." />");
}
sub is_addrspace {
    return 0;
}
sub dependencies {
    return ();
}
sub addr_span_in_space {
    my ($addressable, $addr, $space, $last_space, $leftspan, $rightspan) = @_;
    if (defined($space) ? ($space eq $addressable) : !$addressable->is_addrspace) {			
	return ($addr, 0, $addressable->sizem1, 0, $addressable->sizem1, {}, {}, $addressable);
    }
    return undef unless $addressable->is_addrspace;	
    my $maps = $addressable->ref_map;
    my $i;
    for ($i = 0; $i <= $#$maps; $i++) {
	last if $maps->[$i]->addr <= $addr and $addr <= $maps->[$i]->endaddr;
    }
    if ($i > $#$maps) {		# nothing mapped at that address?
	return undef if defined($space);
	return ($addr, 0, $addressable->sizem1, 0, $addressable->sizem1, {}, {}, $addressable);
    }
    my $map = $maps->[$i];
    my ($paddr, $startpaddr, $endpaddr, $startmapaddr, $endmapaddr, $attrs, $attrsmap, $pspace)
	= addr_span_in_map($addressable, $map, $addr, $space, $last_space, $leftspan, $rightspan);
    if (!defined($paddr)) {	
	InternalError("addr_span_in_map fails with undefined space") unless defined($space);
	return undef;
    }
    if (!$pspace->is_addrspace and $last_space) {
	return ($addr, 0, $addressable->sizem1, 0, $addressable->sizem1, {}, {}, $addressable);
    }
    if ($rightspan) {
	for (my $j = $i+1; $j <= $#$maps; $j++) {
	    my $jmap = $maps->[$j];
	    last unless $jmap->addr == $addr + ($endpaddr - $paddr) + 1;
	    my ($jaddr, $jstart, $jend, $jmstart, $jmend, $jattrs)
		= addr_span_in_map($addressable, $jmap, $jmap->addr, $pspace, $last_space, 0, 1);
	    last unless defined($jaddr);
	    ($jaddr == $jstart && $jaddr == $jmstart && $jaddr == $endpaddr + 1) or InternalError("addr_span_in_space got confused, these must all match (right): 0x%x, 0x%x, 0x%x, 0x%x", $jaddr, $jstart, $jmstart, $endpaddr+1);
	    $attrsmap = Xtensa::System::Attributes::attr_combine(undef, $attrsmap, $jattrs, 0);
	    $endpaddr = $jend;
	}
    }
    if ($leftspan) {
	for (my $j = $i-1; $j >= 0; $j--) {
	    my $jmap = $maps->[$j];
	    last unless $jmap->endaddr + 1 == $addr - ($paddr - $startpaddr);
	    my ($jaddr, $jstart, $jend, $jmstart, $jmend, $jattrs)
		= addr_span_in_map($addressable, $jmap, $jmap->endaddr, $pspace, $last_space, 1, 0);
	    ($jaddr == $jend && $jaddr == $jmend && $jaddr + 1 == $startpaddr) or InternalError("addr_span_in_space got confused, these must all match (left): 0x%x, 0x%x, 0x%x, 0x%x", $jaddr, $jend, $jmend, $startpaddr-1);
	    $attrsmap = Xtensa::System::Attributes::attr_combine(undef, $jattrs, $attrsmap, 0);
	    $startpaddr = $jstart;
	}
    }
    $attrs    = Xtensa::System::Attributes::attr_combine(undef, $addressable->attributes, $attrs, 1);
    $attrsmap = Xtensa::System::Attributes::attr_combine(undef, $addressable->attributes, $attrsmap, 1);
    return ($paddr, $startpaddr, $endpaddr, $startmapaddr, $endmapaddr, $attrs, $attrsmap, $pspace);
}
sub addr_span_in_map {
    my ($addressable, $map, $addr, $space, $last_space, $leftspan, $rightspan) = @_;
    my @spans = $map->spans;
    my ($i, $span);
    for ($i = 0; $i <= $#spans; $i++) {
	$span = $spans[$i];
	last if $span->addr <= $addr and $addr <= $span->endaddr;
    }
    InternalError("addr_span_in_map: addr 0x%x not in map 0x%x..0x%x", $addr, $map->addr, $map->endaddr)
	unless $i <= $#spans;
    my $mmaddr = $span->offset + ($addr - $span->addr);
    my ($paddr, $lowest, $highest, $maplo, $maphi, $attrs, $attrsmap, $pspace)
	= $span->mem->addr_span_in_space($mmaddr, $space, $last_space, $leftspan, $rightspan);
    if (!defined($paddr)) {	
	InternalError("addr_span_in_map fails with undefined space") unless defined($space);
	return undef;
    }
    my $ofslo = $addr - $span->addr;
    my $plo = ($paddr > $ofslo ? $paddr - $ofslo : 0);
    my $ofshi = $span->endaddr - $addr;
    my $phi = ($paddr < ~0 - $ofshi ? $paddr + $ofshi : ~0);
    $lowest = $plo unless $lowest > $plo;
    $highest = $phi unless $highest < $phi;
    $maplo = $plo unless $maplo > $plo;
    $maphi = $phi unless $maphi < $phi;
    if ($rightspan) {
	my $mend = $span->endaddr;
	for (my $j = $i+1; $j <= $#spans && $highest - $paddr == $mend - $addr; $j++) {
	    my $jspan = $spans[$j];
	    my ($jaddr, $jstart, $jend, $jmstart, $jmend, $jattrs)
		= $jspan->mem->addr_span_in_space($jspan->offset, $pspace, $last_space, 0, 1);
	    last unless defined($jaddr);
	    ($jaddr == $jstart && $jaddr == $jmstart && $jaddr == $highest + 1) or InternalError("addr_span_in_map got confused, these must all match (right): 0x%x, 0x%x, 0x%x, 0x%x", $jaddr, $jstart, $jmstart, $highest+1);
	    $attrsmap = Xtensa::System::Attributes::attr_combine(undef, $attrsmap, $jattrs, 0);
	    $highest = $jend;
	    $mend += $jspan->sizem1 + 1;
	}
    }
    if ($leftspan) {
	my $maddr = $span->addr;
	for (my $j = $i-1; $j >= 0 && $paddr - $lowest == $addr - $maddr; $j--) {
	    my $jspan = $spans[$j];
	    my ($jaddr, $jstart, $jend, $jmstart, $jmend, $jattrs)
		= $jspan->mem->addr_span_in_space($jspan->offset+$jspan->sizem1, $pspace, $last_space, 1, 0);
	    last unless defined($jaddr);
	    ($jaddr == $jend && $jaddr == $jmend && $jaddr == $lowest - 1) or InternalError("addr_span_in_map got confused, these must all match (left): 0x%x, 0x%x, 0x%x, 0x%x", $jaddr, $jend, $jmend, $lowest-1);
	    $attrsmap = Xtensa::System::Attributes::attr_combine(undef, $jattrs, $attrsmap, 0);
	    $lowest = $jstart;
	    $maddr -= $jspan->sizem1 + 1;
	}
    }
    $attrs    = Xtensa::System::Attributes::attr_combine(undef, $map->ref_attributes, $attrs, 1);
    $attrsmap = Xtensa::System::Attributes::attr_combine(undef, $map->ref_attributes, $attrsmap, 1);
    return ($paddr, $lowest, $highest, $maplo, $maphi, $attrs, $attrsmap, $pspace);
}
sub addr_in_space {
    my ($addressable, $addr, $endaddr, $space) = @_;
    return $addr if $space eq $addressable;		
    return undef unless $addressable->is_addrspace;	
    my $maps = $addressable->extract(0, $addr, $endaddr);
    return undef unless $maps->map->n;
    my $paddr = undef;
    my $next_paddr = undef;
    my $last_addr = $addr - 1;		
    my $next_addr = $addr;
    foreach my $map ($maps->map->a) {
	return undef unless $map->addr == $next_addr;	
	foreach my $span ($map->spans) {
	    my $m_paddr = $span->mem->addr_in_space($span->offset, $span->offset + $span->sizem1, $space);
	    return undef unless defined($m_paddr);
	    if (defined($next_paddr)) {
		next unless $m_paddr == $next_paddr;
	    } else {
		$paddr = $m_paddr;
	    }
	    $next_paddr = $m_paddr + $span->sizem1 + 1;	
	}
	$last_addr = $map->endaddr;
	$next_addr = $last_addr + 1;	
    }
    return undef unless $last_addr == $endaddr;		
    return $paddr;			
}
sub mymin {my $min = shift; foreach (@_) {$min = $_ if $_ < $min;} $min;}
sub mymax {my $max = shift; foreach (@_) {$max = $_ if $_ > $max;} $max;}
sub search_maps {
    my ($addressable, $func, $ancestry, $addr, $endaddr, @args) = @_;
    $addr = 0 unless defined($addr);
    $endaddr = $addressable->sizem1 unless defined($endaddr);
    $ancestry = [] unless defined($ancestry);
    my $ancestors = [$addressable, @$ancestry];
    if (grep($_ eq $addressable, @$ancestry)) {	
	::Error("(during search) recursive loop through: " .
	    join(", ", map($_->fullname, @$ancestors)));
    }
    my $res = &$func($addressable, $ancestors, $addr, $endaddr, @args);
    return ($res,$ancestors) if defined($res);
    if ($addressable->is_addrspace) {
	foreach my $mapping ($addressable->map->a) {
	    next if $mapping->endaddr < $addr or $mapping->addr > $endaddr;
	    my $sub_addr    = mymax($addr, $mapping->addr);
	    my $sub_endaddr = mymin($endaddr, $mapping->endaddr);
	    my $nextaddr = $mapping->addr;
	    foreach my $span ($mapping->spans) {
		my ($mstart, $mend) = ($nextaddr, $nextaddr + $span->sizem1);
		$nextaddr = $mend + 1;
		next if $sub_addr > $mend or $sub_endaddr < $mstart;
		my ($instart, $inend) = (mymax($mstart, $sub_addr), mymin($mend, $sub_endaddr));
		my @res = $span->mem->search_maps($func, $ancestors,
					    $span->offset + ($instart - $mstart),
					    $span->offset + ($inend - $mstart), @args);
		return @res if defined($res[0]);
	    }
	}
    }
    return (undef);
}
1;
package Xtensa::System::AddressMapping;
our $VERSION = "1.0";
our @ISA = qw(HashObject Xtensa::System::Attributes);
use strict;
sub new {
    my ($class, $space, $addr, $attributes, $mems) = @_;
    my $total_sizem1 = 0;
    my $subsequent = 0;
    my @omems = ();
    ::InternalError("new AddressMapping: need at least one map") unless @$mems;
    foreach my $m (@$mems) {
	$total_sizem1 < ~0 or ::Error("new AddressMapping $class: total size of submappings exceeds 0x%x", ~0);
	$total_sizem1 += $subsequent;
	my ($mem, $offset, $sizem1) = ();
	if (ref($m) eq 'ARRAY') {
	    ($mem, $offset, $sizem1) = @$m;
	} else {
	    $mem = $m->{mem};
	    $offset = $m->{offset} if defined($m->{offset});
	    $sizem1 = $m->{sizem1} if defined($m->{sizem1});
	}
	$mem->isa("Xtensa::System::Addressable")
	    or ::InternalError("new AddressMapping $class: mem must be Addressable (is a ".ref($mem).")");
	if (defined($space)) {
	    if (defined($space->alwaysto) and $space->alwaysto ne $mem) {
		::Error("new AddressMapping $class in space ".$space->name." must always map to ".$space->alwaysto->name." but is mapped to ".$mem->name);
	    }
	    if (defined($mem->alwaysin) and $space ne $mem->alwaysin) {
		::Error("new AddressMapping $class to memory ".$mem->name." must always map in space ".$mem->alwaysin->name." but is mapped in space ".$space->name);
	    }
	}
	$offset = 0 unless defined($offset);		
	$sizem1 = $mem->sizem1 - $offset unless defined($sizem1);	
	push @omems, { mem => $mem, offset => $offset, sizem1 => $sizem1 };
	$total_sizem1 < ~0 - $sizem1 or ::Error("new AddressMapping $class: total size of submappings exceeds 0x%x", ~0);
	$total_sizem1 += $sizem1;
	$subsequent = 1;
    }
    ref($attributes) eq "HASH" or ::InternalError("new AddressMapping $class: attributes must be a hash");
    my $mapping = {
	space		=> $space,		
	addr		=> $addr,
	sizem1		=> $total_sizem1,
	mem		=> $omems[0]->{mem},
	offset		=> $omems[0]->{offset},
	attributes	=> $attributes,
	sections	=> bless( [], "ArrayObject"),		
	paddr		=> undef,		
	share		=> undef,		
    };
    if (@omems > 1) {
	my $spanspace = Xtensa::System::SpanningSpace->new($space, {});
	$mapping->{mem} = $spanspace;
	$mapping->{offset} = $addr;
	my $saddr = $addr;
	foreach my $o (@omems) {
	    push @{$spanspace->ref_map},
		Xtensa::System::AddressMapping->new($space, $saddr, {}, [[$o->{mem}, $o->{offset}, $o->{sizem1}]]);
	}
    }
    bless ($mapping, $class);	
    return $mapping;		
}
sub dup {
    my ($mapping, $takesections, $space) = @_;
    my $copy = { %$mapping };
    bless($copy, ref($mapping));
    $copy->set_space($space) if defined($space);
    $copy->set_share(undef) unless $copy->space eq $mapping->space;
    $copy->set_attributes( { %{$mapping->ref_attributes} } );
    $copy->set_sections( bless([], "ArrayObject") );	
    if (!defined($takesections)) {
	foreach my $sec ($mapping->sections->a) { $sec->dup($copy); }
    } elsif ($takesections) {
	my @secs = $mapping->sections->a;
	foreach my $sec (@secs) {
	    $copy->insertSection($sec) if ($sec->orderTacking & $takesections) != 0;
	}
	(($takesections & 1) ? $mapping : $copy)->del_attr('startalign');
	(($takesections & 4) ? $mapping : $copy)->del_attr('endalign');
    } else {
    }
    if ($mapping->is_spanned) {
	$copy->set_mem($mapping->mem->dup);
    }
    return $copy;
}
sub insert {
    my ($mapping, $space) = @_;
    ::InternalError("wrong space") if defined($space) and $space ne $mapping->ref_space;
    @{$mapping->space->ref_map} = sort {$a->addr <=> $b->addr} (@{$mapping->space->ref_map}, $mapping);
}
sub remove {
    my ($mapping) = @_;
    grep($_ eq $mapping, @{$mapping->space->ref_map})		
	or ::InternalError("did not find self in ".$mapping->space->dump."\n==> ".join(", ", @{$mapping->space->ref_map}));
    @{$mapping->space->ref_map} = grep($_ ne $mapping, @{$mapping->space->ref_map});
    my $share = $mapping->{share};
    $mapping->{share} = undef;
    $share->remove($mapping->space, $mapping->addr) if defined($share);
}
sub endaddr {
    my ($mapping,@check) = @_;
    ::InternalError("AddressMapping: can't set endaddr") if @check;
    return $mapping->addr + $mapping->sizem1;
}
sub endpaddr {
    my ($mapping,@check) = @_;
    ::InternalError("AddressMapping: can't set endpaddr") if @check;
    return undef unless defined($mapping->paddr);
    return $mapping->paddr + $mapping->sizem1;
}
sub default_preferred {
    my ($a, $b) = @_;
    return (
	$b->sizem1 <=> $a->sizem1			
	or (defined($a->addr) ? $a->addr : 0) <=> (defined($b->addr) ? $b->addr : 0)
	or $a->mem->name cmp $b->mem->name
	or $a->offset    <=> $b->offset
    );
}
sub sumSections {
  my ($seg) = @_;
  my ($minsize, $minleft, $minright, $createsize, $minaddr, $maxendaddr) = (0, 0, 0, 0, undef, undef);
  foreach my $sec ($seg->sections->a) {
    if (defined($sec->{minsize})) {
	$minsize += $sec->minsize;
	$minleft  += $sec->minsize if $sec->orderLeftTacked;
	$minright += $sec->minsize if $sec->orderRightTacked;
    }
    $createsize += $sec->createsize if defined($sec->{createsize});
    if (defined($sec->{minaddr})) {
      $minaddr = $sec->minaddr if !defined($minaddr) or $minaddr < $sec->minaddr;
    }
    if (defined($sec->{maxendaddr})) {
      $maxendaddr = $sec->maxendaddr if !defined($maxendaddr) or $maxendaddr > $sec->maxendaddr;
    }
  }
  return ($minsize, $minleft, $minright, $createsize, $minaddr, $maxendaddr);
}
sub is_spanned {
    my ($seg) = @_;
    return $seg->mem->isa("Xtensa::System::SpanningSpace");
}
sub spans {
    my ($seg) = @_;
    return ($seg) unless $seg->is_spanned;
    my @segs = $seg->mem->map->a;
    if (defined($segs[0]->addr) and defined($seg->addr)) {
	$segs[0]->addr == $seg->addr and $segs[-1]->endaddr == $seg->endaddr	
	    or ::Error("improper span bounds for segment ".$seg->span_dump);
    }
    @segs > 1 or ::Error("unexpected spanning way without multiple entries for segment ".$seg->span_dump);
    return @segs;
}
sub span_dump {
    my ($seg) = @_;
    my $dump = sprintf("0x%x..0x%x ", $seg->addr, $seg->endaddr);
    return $dump."(not spanned)" unless $seg->is_spanned;
    $dump .= "spanning";
    foreach my $span ($seg->mem->map->a) {
	$dump .= sprintf(" %s[0x%x..0x%x]", $span->mem->fullname, $span->offset, $span->offset+$span->sizem1);
    }
    return $dump;
}
sub cut_okay {
    my ($seg, $t_start, $t_end, $minsize) = @_;
    my ($seg_minsize, $seg_minleft, $seg_minright) = $seg->sumSections;
    my $seg_minmid = $seg_minsize - $seg_minleft - $seg_minright;
    my $startcut = ($t_start // $seg->addr);
    my $endcut = ($t_end // $seg->endaddr);
    my $left_margin = $startcut - $seg->addr;	
    my $mid_margin = $endcut - $startcut + 1;	
    my $right_margin = $seg->endaddr - $endcut;	
    if ($left_margin != 0 and $left_margin < $seg_minleft) {
	return (0, sprintf("cannot cut segment starting at 0x%x, leaves only 0x%x of 0x%x bytes needed for start-of-segment sections", $startcut, $left_margin, $seg_minleft));
    }
    if ($right_margin != 0 and $right_margin < $seg_minright) {
	return (0, sprintf("cannot cut segment ending at 0x%x, leaves only 0x%x of 0x%x bytes needed for end-of-segment sections", $endcut, $right_margin, $seg_minright));
    }
    my $mid_needed = $minsize;
    my $mid_what = sprintf("minsize=0x%x", $minsize);
    if ($left_margin) {
	$left_margin -= $seg_minleft;
    } else {
	$mid_needed += $seg_minleft;	
	$mid_what .= sprintf(" and 0x%x bytes of start-of-segment sections", $seg_minleft);
    }
    if ($right_margin) {
	$right_margin -= $seg_minright;
    } else {
	$mid_needed += $seg_minright;	
	$mid_what .= sprintf(" and 0x%x bytes of end-of-segment sections", $seg_minright);
    }
    if ($mid_margin < $mid_needed) {
	return (0, sprintf("not enough space (0x%x bytes) in resulting segment for %s", $mid_margin, $mid_what));
    }
    $mid_margin -= $mid_needed;
    my $mid_mask = (($seg_minmid <= $left_margin) ? 1 : 0)
		 + (($seg_minmid <= $mid_margin)  ? 2 : 0)
		 + (($seg_minmid <= $right_margin)? 4 : 0);
    if (!$mid_mask) {
	return (0, sprintf("no space for 0x%x bytes of movable sections in any resulting segment pieces (0x%x, 0x%x, 0x%x bytes)", $seg_minmid, $left_margin, $mid_margin, $right_margin));
    }
    return ($mid_mask, undef, $seg_minleft, $seg_minmid, $seg_minright);
}
sub leftTacked {
  my ($seg) = @_;
  my ($sec) = ($seg->sections->a);	
  return 0 unless defined($sec);
  return $sec->orderLeftTacked;
}
sub rightTacked {
  my ($seg) = @_;
  my $secs = $seg->ref_sections;
  return 0 unless @$secs;
  my $sec = $secs->[$#$secs];		# get last section
  return $sec->orderRightTacked;
}
sub insertSection {
  my ($seg, $section, $must_where_requested) = @_;
  my $newspace = $seg->space;
  defined($newspace) or ::Error("adding section ".$section->name." to segment without an address space");
  ref($newspace->{sections_by_name}) eq "HASH" or ::Error("adding section ".$section->name.": segment linkmap's space $newspace has no {sections_by_name} as hash: space: ".$newspace->dump);
  my $oldseg = $section->segment;
  if (defined($oldseg)) {
      @{$oldseg->ref_sections} = grep($_ ne $section, $oldseg->sections->a);
      my $oldspace = $oldseg->space;
      defined($oldspace) or ::Error("moving section ".$section->name." from segment without an address space");
      delete $oldspace->{sections_by_name}{Xtensa::AddressLayout::encode_key($section->name)};
  }
  if ($section->orderUnique) {
      my @same_order = grep($_->order eq $section->order, $seg->sections->a);
      if (@same_order) {
	  ::Error("adding section ".$section->name." at ".sprintf("0x%X",$seg->addr).": cannot have multiple sections ordered '".$section->order."' in the same segment (already have ".$same_order[0]->name.")");
      }
  }
  if (exists($newspace->{sections_by_name}{Xtensa::AddressLayout::encode_key($section->name)})) {
      my $same = $newspace->{sections_by_name}{Xtensa::AddressLayout::encode_key($section->name)};
      if ($same->segment eq $seg and $same->order eq $section->order
	) {
	  return $same;
      }
      return $same unless $must_where_requested;
      print STDERR "##### Insertion LINKMAP:\n", $seg->space->dump, "\n";
      ::Error("adding section ".$section->name." in segment at ".sprintf("0x%X",$seg->addr).": that section is already present elsewhere in the linkmap");
  }
  $newspace->{sections_by_name}{Xtensa::AddressLayout::encode_key($section->name)} = $section;
  $section->segment($seg);
  my $orderIndex = $section->orderIndex;
  my @sections = $seg->sections->a;
  my $insertion = scalar(@sections);	
  for my $i (0 .. $#sections) {
    if ($sections[$i]->orderIndex > $orderIndex) {
      $insertion = $i;
      last;	
    }
  }
  splice(@{$seg->ref_sections}, $insertion, 0, $section);
  return $section;
}
sub dump {
    my ($mapping) = @_;
    join("", $mapping->format_self);
}
sub format_self {
    my ($mapping) = @_;
    my @spans = $mapping->spans;
    my @lines;
    my $line = sprintf("    <map addr='0x%08x'", $mapping->addr);
    if (@spans == 1) {
	push @lines, $line.sprintf(" sizem1='0x%08x' offset='0x%08x' addressable='%s'%s />",
			$mapping->sizem1, $mapping->offset, $mapping->mem->fullname,
			$mapping->format_attributes());
    } else {
	push @lines, $line . $mapping->format_attributes() . " >";
	foreach my $span (@spans) {
	    push @lines, sprintf("      <m sizem1='0x%08x' offset='0x%08x' addressable='%s' />",
			$span->sizem1, $span->offset, $span->mem->fullname);
	}
	push @lines, "    </map>";
    }
    foreach my $sec ($mapping->sections->a) {
	push @lines, sprintf("      <sec name='%s' order='%s'%s/>", $sec->name, $sec->order,
		join("", map(" $_='".$sec->{$_}."'", grep( !/^name|order|segment$/ && defined($sec->{$_}), keys %$sec))));
    }
    return @lines;
}
sub coalesce_mems {
    my ($mapping) = @_;
    my @spans = $mapping->spans;
    return unless @spans > 1;
    my @outspans = ();
    my $prev = shift @spans;	
    foreach my $span (@spans) {
	if ($prev->mem eq $span->mem
	    and $prev->offset + $prev->sizem1 + 1 == $span->offset) {
	    $prev->set_sizem1($prev->sizem1 + $span->sizem1 + 1);
	} else {
	    push @outspans, $prev;
	    $prev = $span;
	}
    }
    if (@outspans) {
	@{$mapping->mem->ref_map} = (@outspans, $prev);
    } else {
	$mapping->set_offset($prev->offset);
	$mapping->set_mem($prev->mem);
    }
}
sub truncate {
    my ($mapping, $addr, $endaddr) = @_;
    $mapping->space->modified;
    $addr = $mapping->addr unless defined($addr);
    $endaddr = $mapping->endaddr unless defined($endaddr);
    my $pre_msg = sub {			
	my $msg = sprintf("space %s: attempt to truncate mapping at 0x%x..0x%x ", ($mapping->space->name // "(unknown)"), $mapping->addr, $mapping->endaddr);
	my $lock = $mapping->is('lock');
	$msg .= "($lock) " if $lock;
	$msg;
    };
    my $range_msg = sub {
	sprintf("0x%x..0x%x", $addr, $endaddr);
    };
    ::InternalError(&$pre_msg."out of its range to ".&$range_msg)
	if $addr < $mapping->addr or $endaddr > $mapping->endaddr;
    ::InternalError(&$pre_msg."to an inversed range: addr 0x%x greater than endaddr 0x%x",
		$addr, $endaddr) if $addr > $endaddr;
    my ($seg_minsize, $seg_minleft, $seg_minright, $seg_createsize, $seg_minaddr, $seg_maxendaddr) = $mapping->sumSections;
    ::Error(&$pre_msg."to range ".&$range_msg." which is below its minimum size %u", $seg_minsize)
	if $endaddr - $addr < $seg_minsize - 1;
    if ($mapping->is_spanned) {
	my @spans = $mapping->spans;
	my $spanspace = $mapping->mem;
	while ($spans[0]->endaddr < $addr) {
	    shift @spans;
	}
	while ($spans[-1]->addr > $endaddr) {
	    pop @spans;
	}
	if (@spans > 1) {		
	    $mapping->set_offset($addr);
	    $spans[0]->set_sizem1($spans[0]->sizem1 - ($addr - $spans[0]->addr));
	    $spans[0]->set_offset($spans[0]->offset + ($addr - $spans[0]->addr));
	    $spans[0]->set_addr($addr);
	    $spans[-1]->set_sizem1($endaddr - $spans[-1]->addr);
	    @{$spanspace->ref_map} = @spans;
	} else {
	    $mapping->set_mem($spans[0]->mem);
	    $mapping->set_offset($mapping->offset + ($addr - $mapping->addr));
	}
    } else {
	$mapping->set_offset($mapping->offset + ($addr - $mapping->addr));
    }
    $mapping->del_attr('startalign') if $addr > $mapping->addr;
    $mapping->del_attr('endalign') if $endaddr < $mapping->endaddr;
    $mapping->set_addr($addr);
    $mapping->set_sizem1($endaddr - $addr);
    return $mapping;
}
sub cut {
    my ($mapping, $addr, $endaddr, $take_movable_sections) = @_;
    $addr    = undef unless defined($addr)    and    $addr >  $mapping->addr and   $addr <= $mapping->endaddr;
    $endaddr = undef unless defined($endaddr) and $endaddr >= $mapping->addr and $endaddr < $mapping->endaddr;
    my $largest;
    if ($take_movable_sections) {
	$largest = "mid";	
    } else {
	my ($begsize, $midsize, $endsize) = (0, $mapping->endaddr - $mapping->addr, 0);
	if (defined($addr)) {
	    $begsize = $addr - $mapping->addr;
	    $midsize -= $begsize;
	}
	if (defined($endaddr)) {
	    $endsize = $mapping->endaddr - $endaddr;
	    $midsize -= $endsize;
	}
	$largest = ($begsize > $midsize) ? ($begsize > $endsize ? "beg" : "end")
					 : ($midsize > $endsize ? "mid" : "end");
    }
    my ($m1, $m2);
    $m1 = $mapping->dup(($largest eq "beg" ? 2 : 0) | 1) if defined($addr);
    $m2 = $mapping->dup(($largest eq "end" ? 2 : 0) | 4) if defined($endaddr);
    $mapping->truncate($addr, $endaddr);
    if (defined($addr)) {
	$m1->truncate(undef, $addr - 1);
	$m1->insert;
    }
    if (defined($endaddr)) {
	$m2->truncate($endaddr + 1, undef);
	$m2->insert;
    }
    return $mapping;
}
sub cutout {
    my ($mapping, $addr, $endaddr) = @_;
#printf STDERR "#### Cutout 0x%x..0x%x from 0x%x..0x%x ($mapping)\n", $addr, $endaddr, $mapping->addr, $mapping->endaddr;
    my @result = ($mapping);
    if ($mapping->addr == $addr) {		
	if ($mapping->endaddr == $endaddr) {		
	    my ($seg_minsize, $seg_minleft, $seg_minright, $seg_createsize, $seg_minaddr, $seg_maxendaddr) = $mapping->sumSections;
	    my $lock = $mapping->is('lock');
	    ::Error("cannot completely cutout/remove mapping at 0x%x..0x%x%s with allocated size %u",
			$addr, $endaddr, ($lock?" ($lock)":""), $seg_minsize)
		if $seg_minsize > 0;
	    $mapping->remove;		
	    return ();
	} else {
	    $mapping->truncate( $endaddr + 1, undef );
	}
    } else {
	if ($mapping->endaddr != $endaddr) {
	    my $m2 = $mapping->dup(0);
	    $m2->truncate( $endaddr + 1, undef );
	    $m2->insert;		
	    push @result, $m2;
	}
	$mapping->truncate( undef, $addr - 1 );
    }
    return @result;
}
sub merge_ok_nomin {
    my ($a, $b) = @_;
    return 0 unless $a->endaddr + 1 == $b->addr;
    return 0 unless $a->attr_equivalent($a->ref_attributes, $b->ref_attributes);
    return 0 unless ($a->spans)[-1]->mem->is_addrspace == ($b->spans)[0]->mem->is_addrspace;
    return 1;		
}
sub merge_ok {
    my ($a, $b) = @_;
    my ($seg_minsize, $seg_minleft, $seg_minright, $seg_createsize, $seg_minaddr, $seg_maxendaddr) = $b->sumSections;
    return 0 if defined($seg_minaddr) and $a->addr < $seg_minaddr;
    return merge_ok_nomin($a, $b);
}
sub map2one {
    my ($a, $b) = @_;
    return (!$a->is_spanned and !$b->is_spanned
	    and $a->mem eq $b->mem
	    and $a->offset + $a->sizem1 + 1 == $b->offset) ? 1 : 0;
}
sub merge {
    my ($mapping, $after, $spanning_okay, $testfunc) = @_;
    return 1 unless $spanning_okay or map2one($mapping, $after);	
    $testfunc = \&merge_ok unless defined($testfunc);
    my $can_merge = $testfunc->($mapping, $after);
#::dprintf(undef, "#### can_merge 0x%x..0x%x with 0x%x..0x%x = %s\n", $mapping->addr, $mapping->endaddr, $after->addr, $after->endaddr, $can_merge);
    return 1 unless $can_merge;		
    if (defined($mapping->addr) and !(defined($after->addr) and $mapping->endaddr + 1 == $after->addr)) {
	#::dprintf(undef, "##WARNING## removing addr on merge addr/size 0x%x/%x with 0x%x/%x (paddrs 0x%x, 0x%x)\n",
	$mapping->addr(undef)
    }
    $after->remove;
    $mapping->set_attributes(
	$mapping->attr_combine($mapping->ref_attributes, $after->ref_attributes) );
    $after->set_attributes({});			
    my $mappingspan = $mapping->is_spanned;
    my $afterspan = $after->is_spanned;
    my $spanspace = $mappingspan ? $mapping->mem
		  : $afterspan ? $after->mem
		  : Xtensa::System::SpanningSpace->new($mapping->space, {});
    my @spanmaps;
    if ($mappingspan) {
	push @spanmaps, $spanspace->map->a;
    } else {
	my $submap = $mapping->dup(0, $spanspace);
	$submap->set_attributes( {} );	
	push @spanmaps, $submap;
    }
    if ($afterspan) {
	my @maps = $after->mem->map->a;
	foreach (@maps) { $_->set_space($spanspace); }
	push @spanmaps, @maps;
    } else {
	$after->set_space($spanspace);
	push @spanmaps, $after;
    }
    @{$spanspace->ref_map} = @spanmaps;
    $mapping->set_mem($spanspace);
    $mapping->set_offset($mapping->addr);	
    $mapping->sizem1($mapping->sizem1 + $after->sizem1 + 1);
    $mapping->coalesce_mems();
    my @secs = $after->sections->a;	
    foreach my $sec (@secs) {
      $mapping->insertSection($sec);
    }
    return 0;			
}
1;
package Xtensa::System::AddressSpace;
our $VERSION = "1.0";
our @ISA = qw(Xtensa::System::Addressable);
use strict;
our $debug = $Xtensa::AddressLayout::global_debug;
sub new {
    my ($class, $containswhat, $sizem1, $attributes) = @_;
    $sizem1 = 0xFFFFFFFF unless defined($sizem1);		
    $attributes = {} unless defined($attributes);
    my $space = $class->SUPER::new($sizem1, undef, $attributes);
    bless ($space, $class);	
    $containswhat = "mapping" unless defined($containswhat);
    $space->{contains} = $containswhat;
    $space->{map} = new ArrayObject;	
    $space->{alwaysto} = undef;
    $space->{alwaysin_here} = [];
    return $space;
}
sub dup_nomap {
    my ($space) = @_;
    my $newself = $space->SUPER::dup;
    $newself->{map} = new ArrayObject;
    return $newself;
}
sub dup {
    my ($space, $take_all_sections, $predeep_sub) = @_;
    my $newself = $space->SUPER::dup;
    if (defined($predeep_sub)) {
	local $_ = $newself;
	&$predeep_sub;
    }
    $newself->{map} = new ArrayObject;
    my $takesections = defined($take_all_sections) ? ($take_all_sections ? 7 : 0) : undef;
    $newself->map->set( map($_->dup($takesections,$newself), $space->map->a) );
    $space->modified;
    return $newself;
}
sub setmap {
    my $space = shift;
    foreach my $m (@_) {
	$m->{space} = $space;
    }
    @{$space->ref_map} = @_;
    $space->modified;
}
sub pushmap {
    my $space = shift;
    foreach my $m (@_) {
	$m->{space} = $space;
    }
    push @{$space->ref_map}, @_;
    $space->modified;
}
sub modified {
}
sub new_parse {
    my ($class, $layout, $namespace, $lines) = @_;
    my $namespace_p = ($namespace eq "" ? "" : $namespace.".");
    $_ = shift @$lines;
    s/^\s*\<(\w+)\s+name='([^']+)'\s+//
	or ::Error("reading address space: couldn't parse start of line: <<$_>>");
    my ($key, $name) = ($1, $2);
    my $done = 0;
    my $space = $layout->find_addressable($namespace_p.$name);
    if (/^\>\s*$/) {
	defined($space) or ::Error("reading address space: space $name referenced before definition");
    } else {
	s/^sizem1='([^']+)'\s+contains='([^']*)'\s+//
	    or ::Error("reading address space: couldn't parse further start of line: <<$_>>");
	my ($sizem1, $contains) = ($1, $2);
	$sizem1 = Xtensa::AddressLayout::strtoint($sizem1, "AddrSpace sizem1");
	defined($space) and ::Error("reading address space: space $name multiply defined");
	my $attrs = $class->parse_attributes(\$_);
	$done = s/^\///;
	/^\>\s*$/
	    or ::Error("reading address space: bad end of first line, expected '>' : <<$_>>");
	$space = $class->new($contains, $sizem1, $attrs);
	if ($contains eq "segment") {		
	    $space = Xtensa::System::LinkMap->new_from_space($space,undef,undef,undef, layout=>$layout);	
	}
	$layout->add_addressable($namespace_p.$name, $space);	
    }
    return $space if $done;	
    $space->modified;
    my $lastkey;
    my $lastmap = undef;
    while (1) {
	$_ = shift @$lines;
	if (/^\s*\<\/(\w+)\>\s*$/) {
	    $lastkey = $1;
	    last;
	}
	if (/^\s*\<sec((?:\s+\w+='[^']+')+)\s*\/\>\s*$/) {
	    my $keypairstr = $1;  my %keypairs = ();
	    while ($keypairstr =~ s/^\s+(\w+)='([^']+)'//) {
		$keypairs{$1} = $2;
	    }
	    my $secname = delete $keypairs{'name'};
	    my $secorder = delete $keypairs{'order'};
	    defined($secname) and defined($secorder) or ::Error("reading address space: missing name and/or order in map entry: $_");
	    exists($keypairs{'segment'}) and ::Error("reading address space: unexpected segment=... keypair in map entry: $_");
	    defined($lastmap) or ::Error("reading address space: unexpected section before map entry: $_");
	    Xtensa::Section->new($secname, $secorder, $lastmap, %keypairs);
	    next;
	}
	if (s/^\s*\<symbols\b//) {
	    while (s/\s*([a-zA-Z0-9_:]+)\s*=\s*'([^']*)'//) {
		$space->{symbols}->{Xtensa::AddressLayout::decode_key($1)} = $2;
	    }
	    /^\s*\/\>\s*$/
		or ::Error("reading address space: bad end of symbols line, expected '/>' : <<$_>>");
	    next;
	}
	s/^\s*\<map\s+//
	    or ::Error("reading address space: couldn't parse map entry: <<$_>>");
	my $mattrs = $class->parse_attributes(\$_);
	my $submaps = ! s|^/||;
	/^\>\s*$/
	    or ::Error("reading address space: bad end of map line, expected '>' : <<$_>>");
	my $addr = $$mattrs{'addr'};		delete $$mattrs{'addr'};
	my @mems = ();
	if ($submaps) {
	    while (1) {
		$_ = shift @$lines;
		last if /^\s*\<\/map\>\s*$/;
		s/^\s*\<m\s+sizem1='([^']+)'\s+offset='([^']+)'\s+addressable='([^']+)'\s*\/\>\s*$//
		    or ::Error("reading address space: couldn't parse sub-map entry: <<$_>>");
		my ($m_sizem1, $m_offset, $m_aname) = ($1, $2, $3);
		$m_sizem1 = Xtensa::AddressLayout::strtoint($m_sizem1, "AddrSpace submap sizem1");
		$m_offset = Xtensa::AddressLayout::strtoint($m_offset, "AddrSpace submap offset");
		my $ad = $layout->find_addressable($namespace_p.$m_aname);
		defined($ad) or ::Error("reading address space: have not yet seen '$m_aname', oops");
		push @mems, { mem => $ad, offset => $m_offset, sizem1 => $m_sizem1 };
	    }
	} else {
	    my $sizem1 = $$mattrs{'sizem1'};		delete $$mattrs{'sizem1'};
	    my $offset = $$mattrs{'offset'};		delete $$mattrs{'offset'};
	    my $aname = $$mattrs{'addressable'};	delete $$mattrs{'addressable'};
	    my $ad = $layout->find_addressable($namespace_p.$aname);
	    defined($ad) or ::Error("reading address space: haven't yet seen '$aname', oops");
	    @mems = ({ mem => $ad, offset => $offset, sizem1 => $sizem1 });
	}
	$lastmap = $space->add_mapping($addr, [@mems], $mattrs);
    }
    $lastkey eq $key
	or ::Error("reading address space: mismatching tags <$key> ... </$lastkey>");
    $space;
}
sub format_self {
    my ($space, $define_it, $showmaps) = @_;
    my @map = $space->map->a;
    $showmaps = 0 unless @map or $showmaps == 2;
    return unless $showmaps or $define_it;
    my @lines = ("  <space  ".$space->format_keywords($define_it)
	    .($define_it ? " contains='".$space->contains()."'".$space->format_attributes() : "")
	    .($showmaps ? " >" : " />"));
    if ($showmaps) {
	foreach my $m (@map) {
	    push @lines, $m->format_self();
	}
	push @lines, "  </space>";
    }
    @lines;
}
sub dump {
    my ($space) = @_;
    join("",map("$_\n",$space->format_self(1,1,{})));
}
sub debugdump {
    return "" unless $debug;
    my $space = shift;
    $space->dump(@_);
}
sub check {
    my $space = shift;
    foreach my $seg ($space->map->a) {
      my $whatmap = "check: mapping $seg (addr=".sprintf("0x%x..0x%x",$seg->addr, $seg->endaddr).") of space ".($space->name // $space);
      $seg->ref_space eq $space or ::InternalError("$whatmap: invalid ref_space ".$seg->ref_space);
      my $shareset = $seg->{share};
      if (defined($shareset)) {
	  my $layout = $shareset->layout;
	  if ($layout->modified) {
	  } else {
	      my $what = "$whatmap, shareset $shareset (layout ".$shareset->{layout}.")";
	      my $found = join("", map(sprintf("   0x%x => space %s\n", $_->{addr}, $_->{space}), @{$shareset->{set}}));
	      $seg->sizem1 == $shareset->sizem1 or ::InternalError("$what: shareset size ".$shareset->sizem1." does not match mapping size ".$seg->sizem1);
	      my @sets = grep($_->{space} eq $space, @{$shareset->{set}});
	      @sets or ::InternalError("$what: no shares for this space, found:\n".join("", map(sprintf("   0x%x => space %s\n", $_->{addr}, $_->{space}->name), @{$shareset->{set}})));
	      grep($_->{addr} == $seg->addr, @sets) or ::InternalError("$what: no shareset entry for this space matching addr 0x%x, found these addresses: %s", $seg->addr, join(", ", map(sprintf("0x%x", $_->{addr}), @sets)));
	  }
      } else {
      }
    }
}
sub is_addrspace {
    return 1;
}
sub dependencies {
    my ($space) = @_;
    return map(map($_->mem, $_->spans), $space->map->a);
}
sub new_mapping {
    my ($space, $addr, $attributes, $mems) = @_;
    return Xtensa::System::AddressMapping->new($space, $addr, $attributes, $mems);
}
sub add_mapping {
    my ($space, $addr, $mems, $attributes, $recurse_ok, $where) = @_;
    my $mapping = $space->new_mapping($addr, $attributes, $mems);
    $mapping->coalesce_mems();
    return $space->insert_mapping($mapping, $recurse_ok, $where);
}
sub insert_mapping {
    my ($space, $mapping, $recurse_ok, $where) = @_;
    my $addr    = $mapping->addr;
    my $sizem1  = $mapping->sizem1;
    $mapping->ref_space( $space );	
    $space->modified;
    $recurse_ok = 1 unless defined($recurse_ok);
    my $inwhat = sprintf("%s[0x%x..0x%x]", $space->fullname, $addr, $addr+$sizem1);
    $where = (defined($where) and $where ne "") ? $where.": " : "";
    my $wherenext = $where . " at $inwhat:";
    my $mapmsg = $where . "map $inwhat to " .
	join("+", map(sprintf( "%s[0x%x..0x%x]",
			       $_->mem->fullname, $_->offset, $_->offset+$_->sizem1), $mapping->spans));
    my $msg = "inserting ".$space->contains." into address space: $mapmsg";
    ::Error("$msg attributes (isolate and/or exception) prevent access into this space") unless $space->is_accessible;
    my $sub = sub {return ($_[0] eq $space) ? 1 : undef;};
    my ($err, $ancestors) = $mapping->mem->search_maps($sub);
    if (defined($err)) {
	::Error("$msg recursive loop (or mapping already exists) through: " .
	    join(", ", map($_->fullname, (@$ancestors, $space))));
    }
    my ($match, @matches) = $space->findmaps($addr, $addr + $sizem1);
    ::Error("$msg spans more than one existing ".$space->contains." "
    	.join("+",map("(".join(", ",map($_->mem->fullname,$_->spans)).")", ($match,@matches))))
	if @matches;
    if (defined($match)) {
	if ($match->addr <= $addr and $addr + $sizem1 <= $match->addr + $match->sizem1) {
	    if ($recurse_ok) {
		if (!$match->is_spanned) {		
		    if ($match->mem->is_addrspace) {
			$mapping->addr( $match->offset + ($addr - $match->addr) );
			return $match->mem->insert_mapping($mapping, 1, $wherenext);
		    }
		}
	    }
	    if ($match->is('overridable',0) or $match->is('background',0) 
		  or (!$match->is_spanned and $match->mem->name eq "null")) {
		print STDERR "### $msg overridden\n" if $debug;
		$match->cutout($addr, $addr + $sizem1);
		print STDERR "### done\n" if $debug;
		$match = undef;
	    }
	}
    }
#print STDERR "#####  DUMP:\n", map("** $_\n", $layout->format_addressables({})), "#####  END OF DUMP.\n" if defined($match);
    ::Error("$msg overlaps existing ".$space->contains." map(s) at "
    	.join(" + ",map(sprintf("[0x%x..0x%x]=>(",$_->addr,$_->endaddr).join(", ",map(sprintf("%s ofs=0x%x sz=0x%x", $_->mem->fullname, $_->offset, $_->sizem1+1),$_->spans)).")", ($match,@matches)))
	."\n   in code")
	if defined($match);
    print STDERR "### $mapmsg\n" if $debug;
    $mapping->insert;				
    return $mapping;
}
sub sort_by_preference {
    my $space = shift;
    Xtensa::System::LinkMap::sort_map_by_preference(undef, $space->map->a);
}
sub mymin {my $min = shift; foreach (@_) {$min = $_ if $_ < $min;} $min;}
sub mymax {my $max = shift; foreach (@_) {$max = $_ if $_ > $max;} $max;}
sub seg_intersect {
    my ($seg_a, $seg_b) = @_;
    my %amems;
    foreach my $span ($seg_a->spans) {
	push @{ $amems{$span->mem} }, $span;
    }
    foreach my $mem (keys %amems) {
	@{ $amems{$mem} } = sort { $a->offset <=> $b->offset } @{ $amems{$mem} };
    }
    my @results;		
    my $last;			
    foreach my $bspan ($seg_b->spans) {
	my $malist = $amems{$bspan->mem};
	next unless defined($malist);		
	my $bspan_end = $bspan->offset + $bspan->sizem1;
	foreach my $aspan (@$malist) {
	    my $aspan_end = $aspan->offset + $aspan->sizem1;
	    next if $aspan->offset > $bspan_end or $aspan_end < $bspan->offset;
	    my $mstart = mymax($aspan->offset, $bspan->offset);
	    my $mend   = mymin($aspan_end,     $bspan_end);
	    my $sizem1 = $mend - $mstart;
	    my $astart = $mstart - $aspan->offset + $aspan->addr;
	    my $bstart = $mstart - $bspan->offset + $bspan->addr;
	    if (defined($last) and $astart == $last->[0] + $last->[2] + 1 and $bstart == $last->[1] + $last->[2] + 1) {
		$last->[2] += $sizem1 + 1;	
	    } else {
		push @results, $last if defined($last);
		$last = [$astart, $bstart, $sizem1];	
	    }
	}
    }
    push @results, $last if defined($last);
    return @results;
}
sub cut_mem_aliases {
    my ($space, $mem_range, $except_mapping, $skiplocks, $cutout, @cut_attrs) = @_;
    $cutout = 1 unless defined($cutout);
$space->check;
    my ($exceptaddr, $exceptend) = (undef, undef);
    if (defined($except_mapping)) {
	if (ref($except_mapping) eq "ARRAY") {
	    ($exceptaddr, $exceptend) = @$except_mapping;
	    die "this never happens";
	} else {
	    ($exceptaddr, $exceptend) = ($except_mapping->addr, $except_mapping->endaddr);
	}
    }
    my $cuts = $cutout ? 0 : [];
    $space->modified;
    my @maps = $space->map->a;
    foreach my $cutmapping (@maps) {
	my @intersects = seg_intersect($mem_range, $cutmapping);
	foreach my $range (@intersects) {
	    my ($addrm, $cutaddr, $sizem1) = @$range;
	    my $cutend = $cutaddr + $sizem1;
	    next if defined($exceptaddr)
		   && $exceptaddr <= $cutend && $cutaddr <= $exceptend;
	    if (0) {
		printf STDERR "+OVERLAP+ at 0x%x..0x%x in mapping 0x%x..0x%x\n",
		  $cutaddr, $cutend, $cutmapping->addr, $cutmapping->endaddr;
	    }
	    my $lock = $cutmapping->is('lock');
	    if ($lock) {
		next if $skiplocks;
		my $msg = sprintf("cut_mem_aliases($cutout)%s: 0x%x..0x%x of segment at 0x%x..0x%x is already locked (locked for %s)",
		    (defined($exceptaddr) ? sprintf(" (except vaddr 0x%x..0x%x)", $exceptaddr, $exceptend) : ""),
		    $cutaddr, $cutend, $cutmapping->addr, $cutmapping->endaddr, $lock);
		$msg = "(".$space->name.") ".$msg if defined($space->name);
		::Error($msg) unless defined($skiplocks);
		::dprintf(undef, "$msg (skipped)\n");	
		next;
	    }
	    if ($cutout) {
		$cuts++;		
		my @mappings = $cutmapping->cutout($cutaddr, $cutend);
		$cutmapping = $mappings[-1] if @mappings;	
	    } else {
		my $m = $cutmapping->cut($cutaddr, $cutend);
		push @$cuts, $m;
	    }
	}
    }
    $cutout and @cut_attrs and ::InternalError("cut_mem_aliases: cannot specify both cutout and cut_attrs");
    Xtensa::System::Attributes::multi_set_attr($cuts, @cut_attrs) if @cut_attrs;
    return $cuts;
}
sub findmaps {
    my ($space, $addr, $endaddr) = @_;
    $endaddr = $addr unless defined($endaddr);
    return grep($_->addr <= $endaddr && $_->addr + $_->sizem1 >= $addr, $space->map->a);
}
sub findspan {
    my ($space, $addr, $endaddr) = @_;
    $endaddr = $addr unless defined($endaddr);
    my ($min, $max) = (0, $space->sizem1);
    my @maps;
    foreach my $map ($space->map->a) {
	if ($map->endaddr < $addr) {
	    $min = $map->endaddr + 1;
	} elsif ($map->addr <= $addr) {
	    $min = $map->addr;
	}
	if ($map->addr > $endaddr and $map->addr < $max) {
	    $max = $map->addr - 1;
	} elsif ($map->endaddr >= $endaddr and $map->endaddr < $max) {
	    $max = $map->endaddr;
	}
	push @maps, $map if $map->addr <= $endaddr and $map->endaddr >= $addr;
    }
    return ($min, $max, \@maps);
}
sub extract {
    my ($space, $flatten, $offset, $ofsend, $stopspace, $depth) = @_;
    $offset = 0 unless defined($offset);
    $ofsend = $space->sizem1 unless defined($ofsend);
    $flatten = 0 unless defined($flatten);
    my $edebug = 0;
    $depth or $depth = "+++";
    $space->layout->dprintf("$depth EXTRACT(0x%x, 0x%x, %d, %s)\n", $offset, $ofsend, $flatten,
	(defined($stopspace) ? $stopspace->{name} : "<nil>")) if $edebug;
    my $map = new Xtensa::System::AddressSpace;
    $map->{layout} = $space->{layout};
    my $map_self = ($flatten == 1 or $flatten < 0);
    $flatten = 0 if $flatten < 0;
    my $next_gap = $offset;
    foreach my $mapping ($space->map->a) {
	my $maddr = $mapping->addr;
	my $mend = $maddr + $mapping->sizem1;
	next unless $mend >= $offset;		
	last if $maddr > $ofsend;		
	if ($map_self and $maddr > $next_gap) {
	    my $m1gap = Xtensa::System::AddressMapping->new($map, $next_gap, {},
					[[$space, $next_gap, $maddr - 1 - $next_gap]]);
	    $map->pushmap($m1gap);
	}
	$next_gap = ($mend == ~0 ? undef : $mend + 1);
	my $m2 = $mapping->dup(0);		
	$m2->{origmapping} = $mapping;
	$m2->{origspace} = $space;
	$m2->truncate( $offset, undef ) if $maddr < $offset;	
	$m2->truncate( undef, $ofsend ) if $mend > $ofsend;	
	if (!$flatten) {
	    if (defined($stopspace) and (!$m2->is_spanned or $m2->mem ne $stopspace)) {
		$space->layout->dprintf("${depth} SKIP 0x%x..0x%x\n", $m2->addr, $m2->endaddr) if $edebug;
		next;
	    }
	    $map->pushmap($m2);
	    $space->layout->dprintf("${depth} + 0x%x..0x%x\n", $m2->addr, $m2->endaddr) if $edebug;
	    next;
	}
	my ($m_mem, $m_offset, $m_sizem1) = ($m2->mem, $m2->offset, $m2->sizem1);
	%{$m2->ref_attributes} =
	    %{Xtensa::System::Attributes::attr_combine(undef, $m2->ref_attributes, $m_mem->ref_attributes, 1)};
	if (! $m_mem->is_addrspace
	    or (defined($stopspace) and $m_mem eq $stopspace)) {
	    $map->pushmap($m2);
	    $space->layout->dprintf("${depth} + 0x%x..0x%x\n", $m2->addr, $m2->endaddr) if $edebug;
	    next;
	}
	$space->layout->dprintf("$depth Recurse EXTRACT under 0x%x..0x%x\n", $m2->addr, $m2->endaddr) if $edebug;
	my $submap = $m_mem->extract($flatten, $m_offset, $m_offset + $m_sizem1, $stopspace, $depth."+");
	foreach my $smap ($submap->map->a) {
	    $space->layout->dprintf("${depth} SMAP 0x%x..0x%x\n", $smap->addr, $smap->endaddr) if $edebug;
	    $smap->addr( $m2->addr + ($smap->addr - $m_offset) );
	    $space->layout->dprintf("${depth} + 0x%x..0x%x (sub map)\n", $smap->addr, $smap->endaddr) if $edebug;
	    %{$smap->ref_attributes} =
		%{Xtensa::System::Attributes::attr_combine(undef, $m2->ref_attributes, $smap->ref_attributes, 1)};
	    $map->pushmap($smap);
	}
    }
    if ($map_self and defined($next_gap) and $next_gap <= $ofsend) {
	my $m1gap = Xtensa::System::AddressMapping->new($map, $next_gap, {},
				    [[$space, $next_gap, $ofsend - $next_gap]]);
	$map->pushmap($m1gap);
    }
    if ($edebug and $map->map->n) {
	my ($x, @rest) = $map->map->a;
	foreach my $y (@rest) {
	    $y->addr > $x->endaddr
		or ::InternalError("${depth} out of order: 0x%x..0x%x followed by 0x%x..0x%x",
	    			$x->addr, $x->endaddr, $y->addr, $y->endaddr);
	    $x = $y;
	}
    }
    return $map;
}
sub coalesce_mappings {
    my ($space, $spanning_okay, $testfunc) = @_;
    return unless $space->map->n;
    my $i = 0;
    while ($i + 1 < $space->map->n) {
	$i += $space->ref_map->[$i]->merge( $space->ref_map->[$i+1], $spanning_okay, $testfunc );
    }
}
sub cuttable_map {
    my ($m, $free) = cuttable_or_free(@_);
    return $m;
}
sub cuttable_or_free {
    my ($space, $addr, $endaddr, $what) = @_;
    my ($astart, $aend) = ($addr, $endaddr);
    $astart = $aend unless defined($astart);
    $aend = $astart unless defined($aend);
    if (!defined($aend)) {
	return (undef, undef) unless defined($what);
	::InternalError("cuttable_map: $what: at least one of addr,endaddr must be defined");
    }
    my @maps = $space->findmaps($astart, $aend);
    if (@maps == 0) {
	return (undef, 1) unless defined($what);
	::Error("cuttable_map: $what: did not find at least one %s in range 0x%x..0x%x in %s space%s",
		$space->contains, $astart, $aend, $space->fullname, $space->debugdump);
    }
    if (@maps > 1) {
	return (undef, 0) unless defined($what);
	::Error(sprintf("cuttable_map: $what: multiple %ss (expected one) in range 0x%x..0x%x in %s space",
		$space->contains, $astart, $aend, $space->fullname));
    }
    my $m = $maps[0];			
    if ($m->addr > $astart or $m->endaddr < $aend) {
	return (undef, 0) unless defined($what);
	my $lock = $m->is('lock');
	::Error("cuttable_map: $what: %s found at 0x%x..0x%x%s does not cover whole range 0x%x..0x%x in %s space",
		$space->contains, $m->addr, $m->endaddr, ($lock ? " ($lock)" : ""), $astart, $aend, $space->fullname);
    }
    return ($m, 0);
}
sub requirements_okay {
    my ($mapping, $requirement_fns) = @_;
    foreach my $requirement_fn (@$requirement_fns) {
	local $_ = $mapping;
	&$requirement_fn or return 0;
    }
    return 1;
}
sub cut_map {
    my ($space, $addr, $endaddr, $takesections_or_trunc, $requirement_fns) = @_;
    my $m = $space->cuttable_map($addr, $endaddr);
    defined($m) or return undef;
    requirements_okay($m, $requirement_fns) or return undef;
#printf STDERR "#### ABOUT TO CUT 0x%x..0x%x from %s to %s\n%s", $addr,$endaddr,$space,$m->ref_space,$space->debugdump;
    $space->modified;
    return $m->truncate($addr, $endaddr) if $takesections_or_trunc == 2;		
    return $m->cut($addr, $endaddr, $takesections_or_trunc);
}
sub intersect {
    my ($space1, $space2, $ignored_attrs) = @_;
    $space1->layout->dprintf("Intersecting %s with %s\n", $space1->name, $space2->name // "(unnamed linkmap)");
    my @result = ();
    my @a = $space1->map->a;		
    my @b = $space2->map->a;
    while (@a and @b) {
	my ($a_adr, $a_end) = ($a[0]->addr, $a[0]->endaddr);
	my ($b_adr, $b_end) = ($b[0]->addr, $b[0]->endaddr);
	if ($a_end < $b_adr) { shift @a; next; }
	if ($b_end < $a_adr) { shift @b; next; }
	my $mm_adr = ::mymax($a_adr, $b_adr);
	my $mm_end = ::mymin($a_end, $b_end);
	::Error("intersecting multi-memory mappings is not supported")
	    if $a[0]->is_spanned or $b[0]->is_spanned;
	my %diff = $a[0]->attr_diff($a[0]->ref_attributes, $b[0]->ref_attributes, $ignored_attrs);
	my @diffkeys = keys %diff;
	if ($a[0]->mem eq $b[0]->mem
	    and $a[0]->offset == $b[0]->offset + ($a_adr - $b_adr)
	    and @diffkeys == 0) {
	    my $mm = $a[0];			
	    $mm->truncate( $mm_adr, $mm_end );	
	    push @result, $mm;		
	} else {
	    $space1->layout->dprintf("   => overlap at 0x%x..0x%x but differing %s\n",
	    	$mm_adr, $mm_end,
		($a[0]->mem ne $b[0]->mem ? "memories (".$a[0]->mem->name.", ".$b[0]->mem->name.")"
			: $a[0]->offset != $b[0]->offset + ($a_adr - $b_adr) ? "offsets of memory ".$a[0]->mem->name
			: "attributes: ".join(", ", map("$_=(".($a[0]->ref_attributes->{$_} // "(undef)")
							  .",".($b[0]->ref_attributes->{$_} // "(undef)").")", sort keys %diff))));
	}
	if ($a_end < $b_end) { shift @a; } else { shift @b; }
    }
    @{$space1->ref_map} = @result;
}
sub addrs_of_space {
    my ($space, $target_space, $offset, $ofsend, $minaddr, $maxaddr) = @_;
    $minaddr = 0 unless defined($minaddr);
    $maxaddr = $space->sizem1 unless defined($maxaddr);
    my @vaddrs = ();
    my $mapmem = $space->extract(1, $minaddr, $maxaddr, $target_space);
    return $space->addrs_of_space_in_map($target_space, $offset, $ofsend, $mapmem);
}
sub addrs_of_space_in_map {
    my ($space, $target_space, $offset, $ofsend, $mapmem) = @_;
    my @vaddrs = ();
    $mapmem->coalesce_mappings;
    foreach my $mapping ($mapmem->map->a) {
	foreach my $span ($mapping->spans) {
	    next unless $span->mem eq $target_space;
	    next unless $span->offset <= $offset
		and $span->offset + $span->sizem1 >= $ofsend;
	    push @vaddrs, [$span->addr + ($offset - $span->offset), $span];
	}
    }
    @vaddrs;
}
1;
package Xtensa::System::SpanningSpace;
our $VERSION = "1.0";
our @ISA = qw(Xtensa::System::AddressSpace);
use strict;
our $debug = $Xtensa::AddressLayout::global_debug;
sub new {
    my ($class, $containing_space, $attributes) = @_;
    $attributes = {} unless defined($attributes);
    my $space = $class->SUPER::new("submaps", $containing_space->sizem1, $attributes);
    bless ($space, $class);	
    return $space;
}
1;
package Xtensa::Section;
our $VERSION = "1.0";
our @ISA = qw(HashObject);
use strict;
sub new {
    my ($class, $name, $order, $segment, %misc) = @_;
    $order = default_order($name) unless defined($order);
    my $section = {
	name		=> $name,
	order		=> $order,
	segment		=> undef,
	%misc
    };
    bless ($section, (ref($class) || $class));	
    return $segment->insertSection($section) if defined($segment);
    return $section;		
}
sub dup {
    my ($section, $segment) = @_;
    my $new_self = { %$section };
    $new_self->{segment} = undef;
    bless ($new_self, (ref($section) || $section));	
    $segment->insertSection($new_self) if defined($segment);
    return $new_self;
}
sub orderInfo {
  my ($order) = @_;
  my %info = (	'first'   => [ 0, 1, 1],	
		'second'  => [ 1, 1, 1],	
		'third'   => [ 2, 0, 1],	
		'rodata'  => [ 4, 0, 2],	
		'literal' => [ 5, 0, 2],	
		'text'    => [ 6, 0, 2],	
		'data'    => [ 7, 0, 2],	
		'unknown' => [ 8, 0, 2],	
		'ROMSTORE'=> [ 9, 1, 2],	
		'bss'     => [10, 0, 2],	
		'HEAP'    => [11, 1, 2],	
		'STACK'   => [12, 1, 2],	
		'2ndlast' => [13, 0, 4],	
		'last'    => [14, 1, 4],	
	      );
  return $info{$order} if exists($info{$order});
  return [3, 0, 2];		
}
sub orderIndex {
  my ($section) = @_;
  my $info = orderInfo($section->order);
  return $info->[0];
}
sub orderUnique {
  my ($section) = @_;
  my $info = orderInfo($section->order);
  return $info->[1];
}
sub orderTacking {
  my ($section) = @_;
  my $info = orderInfo($section->order);
  return $info->[2];
}
sub orderLeftTacked {
  my ($section) = @_;
  my $info = orderInfo($section->order);
  return ($info->[2] == 1 and $info->[1] != 0);
}
sub orderRightTacked {
  my ($section) = @_;
  my $info = orderInfo($section->order);
  return ($info->[2] == 4 and $info->[1] != 0);
}
sub default_order {
  my ($name) = @_;      
  return 'rodata'   if $name =~ /\.rodata$/;
  return 'rodata'   if $name =~ /\.lit4$/;
  return 'literal'  if $name =~ /\.literal$/;
  return 'text'     if $name =~ /\.text$/;
  return 'data'     if $name =~ /\.data$/;
  return 'ROMSTORE' if $name eq '.rom.store';
  return 'bss'      if $name =~ /\.bss$/;
  return 'HEAP'     if $name eq 'HEAP';
  return 'STACK'    if $name eq 'STACK';
  return 'rodata'   if $name =~ /\.rodata\./;
  return 'rodata'   if $name =~ /\.lit4\./;
  return 'literal'  if $name =~ /\.literal\./;
  return 'text'     if $name =~ /\.text\./;
  return 'data'     if $name =~ /\.data\./;
  return 'bss'      if $name =~ /\.bss\./;
  return 'unknown';		
}
1;
package Xtensa::System::LinkMap;
our $VERSION = "1.0";
our @ISA = qw(Xtensa::System::AddressSpace);
use strict;
our $debug = 0;
sub new {
    my ($class, $cores, $linkspace, $physical, $namespace, %opts) = @_;
    my $linkmap = $class->SUPER::new("segment", $linkspace->sizem1, {} );
    $linkmap = $class->new_from_space($linkmap, $cores, $linkspace, $physical, %opts);
    my $map = $linkspace->extract($opts{withspaces} ? 1 : 2);
    $linkmap->pushmap($map->map->a);
    $map->ref_map(undef);	
    my $layout = $opts{layout};
    $layout->check_misvalign($linkmap);
    my $span_okay = $opts{span_okay} // 0;
    $linkmap->coalesce_mappings($span_okay);
    $linkmap->index_memories($namespace);
    return $linkmap;
}
sub new_from_space {
    my ($class, $space, $cores, $linkspace, $physical, %opts) = @_;
    bless ($space, $class);	
    $space->{layout} = undef;
    foreach my $opt (keys %opts) {
	$space->{$opt} = $opts{$opt};
    }
    $space->{cores} = $cores;		
    $space->{linkspace} = $linkspace;	
    $space->{physical} = $physical;	
    $space->{memories_by_name} = undef;
    $space->{sections_by_name} = {};
    $space->{symbols} = {} unless defined($space->{symbols});	
    $space->{symbol_constructs}   = {};	
    return $space;
}
sub dup {
    my ($linkmap, $take_all_sections) = @_;
    my $newmap = $linkmap->SUPER::dup( $take_all_sections, sub {
	$_->{sections_by_name} = { };		
    } );
    $newmap->{memories_by_name} = { %{$linkmap->{memories_by_name}} };
    return $newmap;
}
sub format_self {
    my ($linkmap, $define_it, $showmaps) = @_;
    my $symbols = $linkmap->{symbols};
    my @lines;
    if ($showmaps and keys %$symbols) {
	@lines = $linkmap->SUPER::format_self($define_it, 2);
	my $lastline = pop @lines;
	push @lines, "    <symbols " . join(" ",
		      map(Xtensa::AddressLayout::encode_key($_)."='".$symbols->{$_}."'", sort keys %$symbols)
		      )." />";
	push @lines, $lastline;
    } else {
	@lines = $linkmap->SUPER::format_self($define_it, $showmaps);
    }
    @lines;
}
sub modified {
    my ($linkmap) = @_;
    my $layout = $linkmap->{layout};
    if (defined($layout)) {
	$layout->set_modified(1);
    }
}
sub index_memories {
    my ($linkmap, $namespace) = @_;
    my %memories;
    foreach my $seg ($linkmap->map->a) {
	my @mems = map($_->mem, $seg->spans);
	foreach my $mem (@mems) {
	    my $fname = $mem->fullname;
	    my $uniqname = $fname;	
	    if (substr($uniqname, 0, length($namespace)+1) eq $namespace.".") {
		$uniqname = substr($uniqname, length($namespace)+1);
	    }
	    $fname =~ s/\./__/g;	
	    my $checkmem = $memories{$fname};
	    if (defined($checkmem) and $checkmem->[0] ne $mem) {
	      ::Error("multiple memories in this linkmap ($namespace) have the name '".$mem->fullname."'");
	    }
	    $memories{$fname} = [$mem, $uniqname];
	}
    }
    $linkmap->{memories_by_name} = \%memories;
}
sub memories {
    my ($linkmap) = @_;
    return map($linkmap->{memories_by_name}{$_}->[0], sort keys %{$linkmap->ref_memories_by_name});
}
sub uniqname {
    my ($linkmap, $mem) = @_;
    my $fname = $mem->fullname;
    $fname =~ s/\./__/g;	
    my $meminfo = $linkmap->ref_memories_by_name->{$fname};
    defined($meminfo) or ::Error("uniqname: memory '".$mem->fullname."' not in this linkmap's initial set of memories");
    my ($umem, $uname) = @$meminfo;
    return $uname;
}
sub sort_map_by_preference {
    my ($linkmap, @map) = @_;
    sort { $a->default_preferred($b) } @map;
}
sub cleanup_map {
    my ($linkmap) = @_;
    foreach my $mapping ($linkmap->map->a) {
	if ($mapping->is('delete') and $mapping->sections->n) {
	    ::InternalError("Segment 0x%x..0x%x of linkmap '%s' is marked for deletion%s but contains sections: %s",
	    	$mapping->addr, $mapping->endaddr, $linkmap->name,
		($mapping->is('lock') ? " (locked for ".$mapping->is('lock').")" : ""),
		join(", ", map($_->name, $mapping->sections->a)));
	}
    }
    $linkmap->setmap( grep(!$_->is('delete'), $linkmap->map->a) );
    LOOP: {
	my @sort_segs = $linkmap->sort_by_preference;
	foreach my $seg (@sort_segs) {
	    $linkmap->cut_mem_aliases($seg, $seg, 1, 1) and redo LOOP;
	}
    }
}
sub insert_section {
    my ($linkmap, $segment, $secname, $secorder, %misc) = @_;
    my $sec = Xtensa::Section->new($secname, $secorder, $segment, %misc);
    my $cuts = $linkmap->cut_mem_aliases($segment, $segment, 1, 0);
    foreach my $cutseg (@$cuts) {
	if ($cutseg->sections->n) {
	    ::InternalError("inserting section %s in segment 0x%x..0x%x of linkmap %s, however its alias at 0x%x..0x%x (same linkmap) has sections in it:  %s", $secname, $segment->addr, $segment->endaddr, $linkmap->name, $cutseg->addr, $cutseg->endaddr, join(", ", map($_->name, $cutseg->sections->a)));
	}
	$cutseg->set_attr('delete'=>1, 'lock'=>"alias to section $secname");
    }
    $sec;
}
sub build_classical_memories {
    my ($linkmap, $mapref) = @_;
    my $map = '';
    my %n;
    foreach my $m (@$mapref) {
	my $name = $m->memname;
	$n{$name} = 'A' unless exists($n{$name});
	$name .= "_" . (++$n{$name}) if $m->level != 1;
	my @segs = ();
	if (defined($m->addr)) {
	    @segs = grep($_->addr >= $m->addr && $_->endaddr <= $m->endaddr, $linkmap->map->a);
	} else {
	    @segs = grep(defined($_->paddr) &&
			 $_->paddr >= $m->paddr && $_->endpaddr <= $m->endpaddr, $linkmap->map->a);
	    my ($largest_seg) = sort {$b->sizem1 <=> $a->sizem1} @segs;
	    my $try_vaddr = $largest_seg->addr - ($largest_seg->paddr - $m->paddr);
	    if (defined($linkmap->linkspace->addr_in_space($try_vaddr, $try_vaddr + $m->sizem1, $m->mem))) {
		$m->addr($try_vaddr);		
	    }
	}
	my $typename = $m->is('type');
	$typename = ($m->is('device') ? "device" : $m->is('writable') ? "sysram" : "sysrom")
		if $typename eq '0' or !$m->is('local');
	$map .= sprintf("BEGIN %s\n", $name);
	$map .= sprintf("0x%08x", $m->addr) if defined($m->addr);
	$map .= sprintf(",0x%08x", $m->paddr) if defined($m->paddr)
				    and (!defined($m->addr) or $m->paddr != $m->addr);
	$map .= sprintf(": %s : %s : ", $typename, $name);
	$map .= sprintf("0x%08x", $m->sizem1+1) if defined($m->addr);
	my $psizem1 = $m->sizem1;	
	$map .= sprintf(",0x%08x", $psizem1+1) if defined($m->paddr)
				    and (!defined($m->addr) or $psizem1 != $m->sizem1);
	$m->del_attr('type', 'local');
	$map .= " : " . $m->classical_format_attributes . ";\n";
	my $segname_suffix = (($name =~ /\d$/) ? "_" : "") . ((@segs > 9) ? "%-2d" : "%d");
	my $n = 0;
	if (!$m->is('device')) {	
	    foreach my $seg (@segs) {
		$map .= sprintf " %s$segname_suffix :  %s : 0x%08x - 0x%08x : ",
			$name, $n, ($seg->is('fixed') ? 'F' : 'C'),
			$seg->addr, $seg->endaddr;
		my @secnames = map($_->name, $seg->sections->a);
		$map .= " STACK : " if grep($_ eq "STACK", @secnames);
		$map .= " HEAP : " if grep($_ eq "HEAP", @secnames);
		@secnames = grep (($_ ne "STACK" and $_ ne "HEAP"), @secnames);
		$map .= sprintf "%s ;\n", join(" ", @secnames);
		$n++;
	    }
	}
	$map .= sprintf "END %s\n\n", $name;
    }
    return $map;
}
sub build_classical_memmap {
    my ($linkmap, $namespace, $merge_memories) = @_;
    $merge_memories //= 0;		
    if (defined($linkmap->linkspace) and defined($linkmap->physical)) {
	foreach my $m ($linkmap->map->a) {
	    $m->{paddr} = $linkmap->linkspace->addr_in_space($m->addr, $m->endaddr, $linkmap->physical);
	}
    }
    my $copymap = $linkmap->dup(0);
#print STDERR "##### COPYMAP:\n", $copymap->dump, "\n";
#print STDERR "##### LINKMAP:\n", $linkmap->dump, "\n";
#print STDERR "##### COPYMAP map = ", $copymap->ref_map, "\n";
#print STDERR "##### LINKMAP map = ", $linkmap->ref_map, "\n";
    $copymap->setmap( grep(!$_->is('exception'), $copymap->map->a) );
    foreach my $m ($copymap->map->a) {
	$m->del_attr(qw(lock fixed asid ca check readable partition bustype overridable overriding));
	$m->del_attr('cached');
	$m->del_attr('data');
	$m->del_attr('condstore');
	$m->del_attr('delay');
	$m->del_attr('unalignable');	
	$m->del_attr('valign', 'startalign', 'endalign', 'startmask', 'endmask', 'alignmask');
    }
#print STDERR "##### COPYMAP1:\n", $copymap->dump, "\n";
#      print STDERR "######### Dump of copymap/linkmap:\n";
#      print STDERR "######### Done\n";
    sub merge_psame {
	my($a, $b) = @_;
	my $debug_merge = 0;
	::dprintf(undef, "** Attempt to merge by addr\n") if $debug_merge;
	return 0 unless defined($a->addr) and defined($b->addr);
	::dprintf(undef, "** Attempt to merge 0x%x..0x%x with 0x%x..0x%x\n", $a->addr, $a->endaddr, $b->addr, $b->endaddr) if $debug_merge;
	return 0 unless $a->attr_equivalent($a->ref_attributes, $b->ref_attributes);
	::dprintf(undef, "** Attempt:  attr okay\n") if $debug_merge;
	my ($ma, $mb) = (($a->spans)[-1], ($b->spans)[0]);
	return 0 unless $ma->mem eq $mb->mem;
	::dprintf(undef, "** Attempt:  same mem\n") if $debug_merge;
	return 0 unless $mb->offset - $ma->offset == $mb->addr - $ma->addr;	
	::dprintf(undef, "** Attempt:  mem offsets compatible\n") if $debug_merge;
	if ($a->addr + $a->sizem1 + 1 != $b->addr) {
	    my $delta = $b->addr - ($a->endaddr + 1);
	    $ma->sizem1($ma->sizem1 + $delta);
	    $a->sizem1($a->sizem1 + $delta) if $a ne $ma;
	    ::dprintf(undef, "** Attempt:  merged across 0x%x delta\n", $delta) if $debug_merge;
	}
	if (!defined($a->paddr) and !defined($b->paddr)) {
	    ::dprintf(undef, "** Attempt:  no paddr, success\n") if $debug_merge;
	    return 1;
	}
	::dprintf(undef, "** Attempt:  some paddr present\n") if $debug_merge;
	return 0 unless defined($a->paddr) and defined($b->paddr);
	::dprintf(undef, "** Attempt:  both paddr present\n") if $debug_merge;
	return 0 unless $mb->offset - $ma->offset == $mb->paddr - $ma->paddr;	
	::dprintf(undef, "** Attempt:  paddr compatible, success\n") if $debug_merge;
	return 1;
    }
    $copymap->coalesce_mappings(1, \&merge_psame);
#print STDERR "##### COPYMAP2:\n", $copymap->dump, "\n";
    my $pcopymap = $copymap;
    $pcopymap->setmap( sort {($a->paddr // $a->addr) <=> ($b->paddr // $b->addr)} $pcopymap->map->a );
    sub merge_psame2 {
	my($a, $b) = @_;
	my $debug_merge = 0;
	::dprintf(undef, "** P Attempt to merge %s with %s by paddr\n", defined($a->addr) ? sprintf("0x%x..0x%x", $a->addr, $a->endaddr) : "?", defined($b->addr) ? sprintf("0x%x..0x%x", $b->addr, $b->endaddr) : "?") if $debug_merge;
	return 0 unless $a->attr_equivalent($a->ref_attributes, $b->ref_attributes);
	::dprintf(undef, "** P Attempt:  attr okay\n") if $debug_merge;
	return 0 unless defined($a->paddr) and defined($b->paddr);
	::dprintf(undef, "** P Attempt:  both paddr present 0x%x..0x%x with 0x%x..0x%x\n", $a->paddr, $a->paddr + $a->sizem1, $b->paddr, $b->paddr + $b->sizem1) if $debug_merge;
	my ($ma, $mb) = (($a->spans)[-1], ($b->spans)[0]);
	return 0 unless $ma->mem eq $mb->mem;
	if ($a->paddr + $a->sizem1 + 1 == $b->paddr) {
	    ::dprintf(undef, "** P Attempt:  paddr in sequence, success\n") if $debug_merge;
	    return 1;
	}
	::dprintf(undef, "** P Attempt:  paddr out of sequence\n") if $debug_merge;
	return 0;
    }
    $pcopymap->coalesce_mappings(1, \&merge_psame2);
#print STDERR "##### COPYMAP3:\n", $pcopymap->dump, "\n";
   $pcopymap->setmap( $pcopymap->sort_map_by_preference($pcopymap->map->a) );
    my %primary = ();
    foreach my $m ($pcopymap->map->a) {
	my %mems = ();
	my @allmems = map($_->mem, $m->spans);
	foreach my $mmem (@allmems) {
	    my $name = $mmem->name;
	    if (substr($name, 0, length($namespace)+1) eq $namespace.".") {
		$name = substr($name, length($namespace)+1);
	    }
	    $name =~ s/\./_/g;		
	    $mems{$name}++;
	}
	$m->{memname} = join("__", sort keys %mems);
	$m->{level} = ++$primary{$m->{memname}};
    }
    $pcopymap->setmap( sort { (defined($a->addr) ? $a->addr : $a->paddr)
		   <=> (defined($b->addr) ? $b->addr : $b->paddr) } $pcopymap->map->a );
#print STDERR "##### COPYMAP4:\n", $pcopymap->dump, "\n";
    return $linkmap->build_classical_memories($pcopymap->ref_map);
}
sub print_classical_memmap {
    my ($linkmap, $namespace) = @_;
    print $linkmap->build_classical_memmap($namespace);
}
sub find (&@) {
  my ($code, @array) = @_;
  my $i = 0;
  foreach $_ (@array) {		
    return $i if &$code;
    $i++;
  }
  return undef;
}
1;
package Xtensa::System::ShareSet;
our $VERSION = "1.0";
our @ISA = qw(HashObject);
use strict;
our $debug = 0;
sub new {
    my ($class, $layout, $sizem1) = @_;
    my $shareset = {
	layout    => $layout,	
	sizem1	  => $sizem1,	
	set	  => [],	
    };
    bless ($shareset, $class);	
    return $shareset;		
}
sub _update_sharemap {
    my ($shareset, $offset, $newset) = @_;
    my $mems = $shareset->{layout}->ref_mems;
    foreach my $mem (keys %$mems) {
	my $mem_map = $mems->{$mem};
	my @mem_new = ();			
	foreach my $mmap (@$mem_map) {
	    next unless $mmap->{share} eq $shareset;
	    next if $mmap->{offset} >= $offset;	
	    my $mmap_end = $mmap->{offset} + $mmap->{sizem1};
	    next if $mmap_end < $offset;	
	    push @mem_new, { addr => $mmap->{addr} + $offset,
	    		     sizem1 => $mmap_end - $offset,
			     share => $newset,
			     offset => 0 };
	    $mmap->{sizem1} = ($offset - 1) - $mmap->{offset};
	}
	if (@mem_new) {
	    @$mem_map = sort { $a->{addr} <=> $b->{addr} } (@$mem_map, @mem_new);
	}
    }
}
sub cut {
    my ($shareset, $offset, $endofs, $except_linkmap, $except_addr, $except_endaddr) = @_;
    return (undef, undef, undef)
	    unless $offset > 0 && $offset <= $shareset->sizem1
		or $endofs > 0 && $endofs <= $shareset->sizem1;
    $offset <= $endofs or ::InternalError("Xtensa::System::ShareSet->cut: offset $offset > endofs $endofs");
    my $midsize = $endofs - $offset;
    my $fullsize = $shareset->sizem1;
    my $midset;
    if ($offset > 0) {
	my $leftset = $shareset;
	$leftset->set_sizem1($offset - 1);
	$midset = Xtensa::System::ShareSet->new($shareset->{layout}, $midsize);
	$shareset->_update_sharemap($offset, $midset);
    } else {
	$midset = $shareset;
	$midset->set_sizem1($midsize);
    }
    if ($endofs < $fullsize) {
	my $rightsize = $fullsize - $endofs;
	my $rightset = Xtensa::System::ShareSet->new($shareset->{layout}, $rightsize);
	$shareset->_update_sharemap($endofs, $rightset);
    }
}
sub add_share_space {
    my ($shareset, $linkmap, $vaddr) = @_;
    push @{$shareset->{set}}, { space => $linkmap, addr => $vaddr };
}
sub remove {
    my ($shareset, $space, $addr) = @_;
    @{$shareset->{set}} = grep( !($_->{space} eq $space and $_->{addr} == $addr),
				@{$shareset->{set}} );
}
1;
package Xtensa::Processor::Vector;
sub addr {
    my ($vec, $pr, $swoptions) = @_;
    if (!$pr->vectors->relocatableVectorOption) {
	return $vec->baseAddress;
    }
    my $base;
    if ($vec->group eq "relocatable") {
	my $vecbase = ($swoptions->{vecbase} // $pr->vectors->relocatableVectorBaseResetVal);
	$base = ($vecbase & ~((1 << $pr->vectors->vecbase_alignment) - 1));
    } else {
	my $vecselect = ($swoptions->{vecselect} // $pr->vectors->SW_stationaryVectorBaseSelect);
	if ($pr->vectors->vecBase1FromPins && $vecselect == 1) {
	    $base = ($swoptions->{vecreset_pins} // $pr->vectors->stationaryVectorBase1);
	} else {
	    $base = ($vecselect ? $swoptions->{static_vecbase1} : $swoptions->{static_vecbase0});
	    $base //= ($vecselect ? $pr->vectors->stationaryVectorBase1 : $pr->vectors->stationaryVectorBase0);
	}
    }
    return $base + $vec->offset;
}
sub reloc_vector_offsets {
    my ($pr) = @_;
    my ($minofs, $maxofs);
    foreach my $vec ($pr->vectors->swVectorList) {
	next unless $pr->vectors->relocatableVectorOption and $vec->group eq "relocatable";
	my $offset = $vec->offset;
	$minofs = $offset if !defined($minofs) || $offset < $minofs;
	$maxofs = $offset if !defined($maxofs) || $offset > $maxofs;
    }
    return ($minofs, $maxofs);
}
1;
