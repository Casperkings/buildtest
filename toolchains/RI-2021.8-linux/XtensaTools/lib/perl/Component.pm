# Copyright (c) 2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
package Component;
use Exporter 'import';
@EXPORT = qw(read_xld_raw);
@EXPORT_OK = qw(readfile writefile);
our $global_debug;
BEGIN {
    $global_debug = 0;
    $global_debug = 1 if ($ENV{"XTENSA_INTERNAL_DEBUG"} or "") =~ /^DEBUG/;
}
our $VERSION = "1.0";
use Object;
Class qw(HashObject);
use strict;
use Xtensa::YAML;
sub readfile {
    my ($filename) = @_;
    my $file;
    open(IN,"<$filename") or die "could not open '$filename' for reading: $!";
    defined(sysread(IN,$file,-s IN)) or die "could not read '$filename': $!";
    close(IN);
    return $file;
}
sub writefile {
    my ($filename, $data) = @_;
    open(OUT,">$filename") or die "Could not open '$filename' for writing: $!";
    print OUT $data;
    close(OUT) or die "Could not close '$filename' after writing: $!";
}
sub read_xld_raw {
    my ($filename, $what) = @_;
    my $yaml = readfile($filename);
    my $system = YAML_Load_objects($yaml);
    if (UNIVERSAL::isa($system, "ARRAY")) {
	$system = { constructs => $system };
    } elsif (!UNIVERSAL::isa($system, "HASH")) {
	return undef unless defined($what);
	die "$what: ERROR: system is neither a structure (perl hash) nor an array: $filename";
    }
    if (!UNIVERSAL::isa($system, "Component::System")) {
	if (ref($system) ne "HASH" and defined($what)) {
	    print STDERR "$what: WARNING: system is of type ".ref($system)." which is not a subclass of Component::System ; overriding\n";
	}
	bless($system, "Component::System");
    }
    return $system;
}
package Component::Object;
our $VERSION = "1.0";
our @ISA = qw(HashObject);
use Tie::OrderedHash;
use strict;
sub new {
    my ($class, $system, $name, $parent, $type) = @_;
defined($name) or die "new Component::Object missing name";
    my $obj = {
	name => $name,
	fullname => (defined($parent) ? $parent->fullname . "." : "").$name,
	parent => $parent,
	type => $type,
	system => $system,
	seen => undef,
    };
    bless ($obj, $class);	
    return $obj;		
}
sub dup {
    my ($orig_obj, $parent, $name) = @_;
defined($name) or defined($orig_obj->{name}) or die "dup Component::Object missing name";
    my $obj = { %$orig_obj };
    $obj->{parent} = $parent if defined($parent);
    $obj->{name} = $name if defined($name);
    $obj->{fullname} = (defined($parent) ? $parent->fullname . "." : "").($name // $obj->{name}),
    bless ($obj, ref($orig_obj));	
    return $obj;
}
sub sanity_check {
    my ($obj) = @_;
    return if defined($obj->{fullname}) and $obj->fullname =~ /\.types$/;	
    $obj->{type}  //= undef;
}
sub standard_members {
    my ($obj) = @_;
    return qw(name fullname parent type types system seen);
}
sub classify_type_types {
    my ($parent, $child, $name) = @_;
    return ( (exists($child->{ports}) or exists($child->{port}))
	     ? "Component::InterfaceType" : "Component::Instance" );
}
sub classify_members {
    my ($obj) = @_;
    my $in_types = (defined($obj->{fullname}) and $obj->{fullname} =~ /\.types$/);
    my %members;
    tie %members, 'Tie::OrderedHash';	
    foreach (sort keys %$obj) {
	$members{$_} = [$obj->{$_}, !/^(parent|type|system|seen)$/, !/^(seen)$/, $in_types ? \&classify_type_types : undef];
    }
    return %members;
}
sub find_subobjects_hash {
    my ($obj, $class) = @_;
    my %members = $obj->classify_members;
    my %standard = map { $_ => 1 } $obj->standard_members;
    my %hash;
    tie %hash, 'Tie::OrderedHash';	
    foreach my $member (keys %members) {
	my $m = $members{$member};
	my ($value, $ischild, $isobject, $typefn) = @$m;
	next unless $ischild and $isobject;
	next if $standard{$member};
	next if ref($value) eq "";
	if (ref($value) eq "ARRAY") {
	    foreach my $i (0 .. $#$value) {
		my $elem = $value->[$i];
		next unless $elem->isa($class);
		my $name = ($member =~ /\d$/) ? $member.".$i" : $member.$i;
		$hash{$name} = $elem;
	    }
	} elsif (ref($value) !~ /^[A-Z]+$/ and $value->isa($class)) {
	    $hash{$member} = $value;
	}
    }
    $obj->system->Info("Found ".scalar(%hash)." members of ".$obj->fullname." of type $class:");
    foreach (keys %hash) { $obj->system->Info("==> $_"); }
    return %hash;
}
sub find_subobjects {
    my ($obj, $class) = @_;
    my %hash = find_subobjects_hash($obj, $class);
    return values %hash;
}
sub assign_named_member {
    my ($obj, $orig_obj, $name, $value) = @_;
    my @part = split(/\./, $name);
    my $first = shift @part;
    if (!exists($orig_obj->{$first})) {
	$first =~ s/(\d+)$// or die "oops unsplittable";
	unshift @part, $1;
    }
    my $lvalue = \$obj->{$first};
    for (@part) {
	$lvalue = \$lvalue->[$_];
    }
    $$lvalue = $value;
}
sub base_type {
    my ($obj) = @_;
    my $type = ref($obj);
    return $type if $type eq "" or $type =~ /^[A-Z]+$/;
    my $next = $type;
    while (defined($next) and $next ne "Component::Object" and $next ne "Object" and $next ne "HashObject") {
	$type = $next;
	no strict 'refs';
	$next = ${$next."::ISA"}[0];
    }
    return $type;
}
sub lookup_type {
    my ($parent, $typename, $where) = @_;
    return $typename if UNIVERSAL::isa($typename, "Component::Type");
    my $system = $parent->system;
    ref($typename) eq "" or $system->Error("$where: type name is a ".ref($typename)." instead of a string");
    while (defined($parent)) {
	$system->Info("\? Lookup type $typename in ".$parent->fullname);
	my $types = $parent->{types} // {};	
	my $type = $types->{$typename};
	if (defined($type)) {
	    $type->{parent} //= $types;
	    $types->{parent} //= $parent;
	    $type->{fullname} //= $parent->fullname.".types.".$typename;
	    return $type;
	}
	$parent = $parent->parent;
    }
    $system->Error("$where: type name '$typename' not found");
}
sub deduce_type {
    my ($parent, $child, $name, $isobject, $typefn, $seen) = @_;
    return 0 if ref($child) eq "";		
    return 1 if UNIVERSAL::isa($child, "Component::Object");
    my $where = "type of $name in ".$parent->fullname;
    my $type;
    my $class;
    if (UNIVERSAL::isa($child, "HASH")) {
	my $type_name = $child->{type};
	my $type_family = $child->{type_family};
	if (defined($type_name)) {
	    if (ref($type_name) eq "") {
		if ($type_name eq "Interface") {
		    $class = "Component::InterfaceType";
		} elsif ($type_name eq "Component") {
		    $class = "Component::Instance";
		} else {
		    $type = $parent->lookup_type($type_name, $where);
		}
	    } else {
		$type = $type_name;
	    }
	} elsif (defined($type_family)) {
	    $parent->system->Info("*** PARSING $where: type_family=$type_family");
	    $type_family =~ m/^\w+(\.\w+)*$/
	    	or $parent->system->Error("$where: invalid type_family format (must be one or more dot-separated identifiers): $type_family");
	    my $family_class = "Component.".$type_family;
	    $family_class =~ s/\./::/g;
	    eval "require $family_class";
	    {
	      no strict 'refs';
	      $type = &{$family_class."::type_family_lookup"}($family_class, $parent->system, $child, $where);
	    }
	    defined($type) or $parent->system->Error("$where: lookup failed for type_family $type_family");
	}
    }
    if (!defined($class)) {
	if (defined($type)) {
	    if (defined($seen) and (!defined($type->{seen}) or $type->seen ne $seen)) {
		defined($type->{fullname}) or $parent->system->Error("$where: missing fullname for type");
		defined($type->{parent}) or $parent->system->Error("$where: missing parent for type");
		my $type_name = $type->{fullname};
		$type_name =~ s/.*\.//;
		$type->{parent}->deduce_type($type, $type_name, 1, \&classify_type_types, $seen)
		    or $parent->system->Error("$where: a type structure must be a subclass of Component::Object");
		$type->descendant_check($seen, $type->{parent}, $type->{fullname});
	    }
	    $child->{type} = $type;
	    $class = $type->instance_class;
	} elsif (defined($typefn) and defined($class = &$typefn(@_))) {
	} elsif (ref($child) eq "HASH" and $isobject) {
	    $class = "Component::Object";
	} elsif ($isobject and ref($child) ne "ARRAY") {
	    $parent->system->Error("".$parent->fullname.".$name: member of unexpected type ".ref($child)." is not a subclass of Component::Object");
	}
    }
    if (defined($class)) {
	bless ($child, $class);
	$parent->system->Info("==> bless ".$parent->fullname.".$name to $class");
    }
    return UNIVERSAL::isa($child, "Component::Object") ? 1 : 0;
}
sub descendant_check {
    my ($obj, $seen, $parent, $fullname) = @_;
    if (defined($obj->{seen}) and $obj->seen eq $seen) {
	if ($obj->parent ne $parent or $obj->fullname ne $fullname) {
	    $obj->system->Error("descendant_check: object with multiple parents, known as both ".$obj->fullname." and $fullname");
	}
	return;
    }
    $obj->set_parent($parent);
    $obj->set_fullname($fullname);
    $obj->set_system($parent->system) if defined($parent);
    $obj->set_seen($seen);
    $obj->sanity_check;
    my %members = $obj->classify_members;
    my @sorted_list = ( grep($_ eq 'types', keys %members), grep($_ ne 'types', keys %members) );
    while (@sorted_list) {
	my $member_name = shift @sorted_list;
	$member_name =~ /^\w+$/ and $member_name !~ /__/ and $member_name !~ /^_/
	    and $member_name !~ /_$/ and $member_name !~ /^\d$/ and $member_name ne ""
	    or $obj->system->Error("descendant_check: in object named '$fullname': invalid member name '$member_name'");
	my $m = $members{$member_name};
	my ($member, $ischild, $isobject, $typefn) = @$m;
	my $member_fullname = $fullname.".".$member_name;
	$obj->system->Info("+ $member_fullname: ischild=$ischild isobject=$isobject");
	next unless $ischild;		
	next if ref($member) eq "";	
	if (ref($member) eq "ARRAY") {
	    $obj->system->Info("++ array type");
	    my $where = $seen->{$member};
	    $where and $obj->system->Error("descendant_check: array ref ($member) seen multiple times: $member_fullname and $where");
	    $member_name =~ /\d$/ and $obj->system->Error("descendant_check: object '$fullname': array member name cannot end with a digit: '$member_name'");
	    $seen->{$member} = $member_fullname;
	    my $common_basetype;
	    foreach my $i (0 .. $#$member) {
		my $basetype;
		my $element = $member->[$i];
		my $suffix = $i;	
		my $element_name = $member_name.$suffix;
		exists($obj->{$element_name}) and $obj->system->Error("descendant_check: explicit member $element_name duplicates ${member_name} array entry $i (same name $element_name)");
		if ($obj->deduce_type($element, $element_name, $isobject, $typefn, $seen)) {
		    $basetype = $obj->base_type;
		    $element->descendant_check($seen, $obj, $member_fullname.$suffix);
		} else {
		    $basetype = "";
		}
		if ($i == 0) {
		    $common_basetype = $basetype;
		} elsif ($basetype ne $common_basetype) {
		    $obj->system->Error("descendant_check: elements 0 and $i of array $member_fullname have different base types '$common_basetype' and '$basetype'");
		}
	    }
	} else {
	    if ($obj->deduce_type($member, $member_name, $isobject, $typefn, $seen)) {
		$member->descendant_check($seen, $obj, $member_fullname);
	    } else {
		$obj->system->Info("++ deduced non-object type");
	    }
	}
	if ($fullname =~ /\.types$/) {
	    my %new_members = $obj->classify_members;
	    foreach my $mnew (keys %new_members) {
		next if exists($members{$mnew});
		unshift @sorted_list, $mnew;
		$members{$mnew} = $new_members{$mnew};
	    }
	}
    }
}
sub ref_check {
    my ($obj, $prevseen, $seen, $fullname) = @_;
    return if ref($obj) eq "";		
    my $oops = $seen->{$obj};
    $oops and die "ref_check: reference loop from $oops to $fullname";
    if (ref($obj) eq "ARRAY") {
	$seen->{$obj} = $fullname;	
	foreach my $i (0 .. $#$obj) {
	    my $element = $obj->[$i];
	    ref_check($element, $prevseen, $seen, $fullname.".".$i);
	}
	delete $seen->{$obj};		
	return;
    }
    return if $obj->{seen} eq $seen;
    $obj->{seen} eq $prevseen or die "ref_check: unexpected reference to unparented object $fullname";
    $obj->set_seen($seen);
    $seen->{$obj} = $fullname;		
    my %members = $obj->classify_members;
    foreach my $member (keys %members) {
	next if $member =~ /^(system|parent|seen)$/;
	my $m = $members{$member};
	my ($value, $ischild, $isobject, $typefn) = @$m;
	next unless $isobject;
	my $member_fullname = $fullname.".".$member;
	ref_check($value, $prevseen, $seen, $member_fullname);
    }
    delete $seen->{$obj};		
}
1;
package Component::System;
our $VERSION = "1.0";
our @ISA = qw(Component::Object);
use strict;
sub Error {
    my ($system, $msg, @args) = @_;
    $msg = sprintf($msg, @args);
    use Data::Dumper;
    $Data::Dumper::Sortkeys = 1;
    print STDERR "\nERROR: $msg\nSystem dump:\n".Dumper($system)."\n";
    die "ERROR: $msg\nStopped";
}
sub Warning {
    my ($system, $msg, @args) = @_;
    $msg = sprintf($msg, @args);
    print STDERR "WARNING: $msg\n";
}
sub Info {
    my ($system, $msg, @args) = @_;
    $msg = sprintf($msg, @args);
    print STDERR "INFO: $msg\n" if $global_debug;
}
sub standard_members {
    my ($system) = @_;
    return ($system->SUPER::standard_members,
    	qw(instance xtensa_tools_root xtensa_system constructs layout) );
}
sub classify_members {
    my ($system) = @_;
    my %members = $system->SUPER::classify_members;
    $members{layout} = [$system->{layout}, 0, 0];
    $members{constructs} = [$system->{constructs}, 0, 0];
    return %members;
}
sub sanity_check {
    my ($system) = @_;
    $system->SUPER::sanity_check;
    $system->{types} //= {};
    $system->{imports} //= [];
    $system->{system} = $system;
}
sub sanity_check_system {
    my ($system) = @_;
    $system->sanity_check;
    my $seen1 = {};	
    $system->descendant_check( $seen1, undef, $system->name );
    $system->ref_check( $seen1, {}, $system->name );
}
sub process_imports {
    my ($system) = @_;
    $system->import_types(glob($system->xtensa_tools_root."/lib/components/*/*type.xld"),
			  glob($system->xtensa_tools_root."/lib/components/interfaces/*.xld"));
    $system->resolve_types;
    $system->sanity_check;
    foreach my $import ($system->imports) {
	if (ref($import) eq "") {
	    my @files = glob($import);
	    @files or $system->Warning("system imports entry not found: $import");
	    $system->import_types(@files);
	} elsif(UNIVERSAL::isa($import, "HASH")) {
	    my $config = $import->{xtensa_core} // "";
$system->Error("import config $config -- not handled yet");
	} else {
	    $system->Error("unexpected entry in system imports list, of type: ".ref($import));
	}
    }
}
sub import_types {
    my ($system, @files) = @_;
    foreach my $file (@files) {
	my $sys = Component::read_xld_raw($file);
	my $types = $sys->{types} // {};
	foreach my $typename (keys %$types) {
	    my $type = $types->{$typename};
	    $system->{types}->{$typename} = $type;
	}
    }
}
sub resolve_types {
    my ($system) = @_;
    my $types = $system->{types} // {};
    foreach my $typename (keys %$types) {
	next if grep($typename, qw(fullname));
	my $type = $types->{$typename};
	$types->deduce_type($type, $typename, 1);
    }
}
sub types_equivalent {
    my ($system, $a, $b) = @_;
    my %classes;
    while (defined($a) and ref($a) ne "") {
	return 1 if $a eq $b;
	$classes{$a} and $system->Error("type with infinite superclass loop");
	$classes{$a} = 1;
	$a = $a->type;
    }
    while (defined($b) and ref($b) ne "") {
	return 1 if $classes{$b};
	$b = $b->type;
    }
    return 0;
}
sub expand_system {
    my ($system) = @_;
    if (!defined($system->type)) {
	$system->Warning("no type defined for system, nothing expanded");
	return undef;
    }
    my $type = $system->lookup_type($system->type, "system type");
    my $instance = $type->expand($system, "instance");
    $system->set_instance($instance);
    return $instance;
}
1;
package Component::Type;
our $VERSION = "1.0";
our @ISA = qw(Component::Object);
use strict;
sub new {
    my ($class, @args) = @_;
    my $type = $class->SUPER::new(@args);
    return $type;		
}
1;
package Component::InterfaceType;
our $VERSION = "1.0";
our @ISA = qw(Component::Object);
use strict;
sub instance_class { return "Component::InterfaceInstance"; }
1;
package Component::InterfaceInstance;
our $VERSION = "1.0";
our @ISA = qw(Component::Object);
use strict;
sub expand {
    my ($template, $parent, $name) = @_;
    my $system = $template->system;
    my %interfaces;
    my %map;		
    my $instance = Component::InterfaceInstance->new($system, $name, $parent, $template);
    my $what = "expand interface ".$template->fullname." to ".$instance->fullname;
    $system->Info($what);
    if (defined($template->type)) {
	$template->type->Component::Instance::expand_interfaces($instance, \%interfaces, \%map, "type of $name");
    }
    $template->Component::Instance::expand_interfaces($instance, \%interfaces, \%map, $name);
    return $instance;
}
1;
package Component::InterfaceInstanceType;
our $VERSION = "1.0";
our @ISA = qw(Component::Object);
use strict;
1;
package Component::Endpoint;
our $VERSION = "1.0";
our @ISA = qw(Component::Object);
use strict;
sub new {
    my ($class, @args) = @_;
    my $endpoint = $class->SUPER::new(@args);
    $endpoint->{interface} = undef;
    $endpoint->{array_name} = 0;
    return $endpoint;		
}
sub dup {
    my ($orig_endpoint, $parent, $name) = @_;
    my $endpoint = $orig_endpoint->SUPER::dup($parent, $name);
    return $endpoint;
}
package Component::Instance;
our $VERSION = "1.0";
our @ISA = qw(Component::Object);
use strict;
sub new {
    my ($class, @args) = @_;
    my $component = $class->SUPER::new(@args);
    $component->{connections} = [ ];
    return $component;		
}
sub dup {
    my ($component, $parent, $name) = @_;
    my $copy = $component->SUPER::dup($parent, $name);
    return $copy;
}
sub instance_class {
    return "Component::Instance";
}
sub sanity_check {
    my ($component) = @_;
    $component->SUPER::sanity_check;
    $component->{connections} //= [];
}
sub standard_members {
    my ($component) = @_;
    return ($component->SUPER::standard_members,
    	qw(connections) );
}
sub classify_members {
    my ($component) = @_;
    my %standard = map { $_ => 1 } $component->standard_members;
    my %members = $component->SUPER::classify_members;
    foreach (keys %members) {
	next if $standard{$_};
	$members{$_}[3] = \&Component::Object::classify_type_types;
    }
    return %members;
}
sub compile_glob_name {
    my ($name, $what) = @_;
    if ($name =~ /^\./ or $name =~ /\.$/ or $name =~ /\.\./) {
	die "$what: invalid name '$name': cannot start or end with a dot (.) or have two consecutive dots (..)";
    }
    my @parts = map(quotemeta($_), split(/\./, $name));
    foreach (@parts) { s/\\*+/[^.]*/g; }
    @parts = map(qr/$_/, @parts);
    return \@parts;
}
sub match_glob_name {
    my ($re_list, $name) = @_;
    my @parts = split(/\./, $name);
    my @remain = @$re_list;
    return undef if @parts > @remain;
    foreach my $n (@parts) {
	my $re = shift @remain;
	return undef unless $n =~ $re;
    }
    return \@remain;
}
sub match_interfaces {
    my ($system, $iface_comps, $is_components, $orig_endpoint, $re_list, $prefix, $internal, $what) = @_;
    my %arrays;
    my @keys = keys %$iface_comps;	
    foreach (@keys) {
	next unless /^(.*)_array$/;
	$arrays{$1} = 1 if match_glob_name($re_list, $1);
    }
    foreach (@keys) {
	next unless /^(.*?)\d+/;
	delete $iface_comps->{$_} if $arrays{$1};
    }
    my @interfaces;
    foreach my $iname (keys %$iface_comps) {
	my $name = $iname;
	my $is_array = ($name =~ s/_array$//);
	my $remain = match_glob_name($re_list, $name);
	if ($remain) {
	    my $iface_comp = $iface_comps->{$iname};
	    if (@$remain) {
		if (!$internal and !$is_components and ! $iface_comp->type->allow_subconnect) {
		    $system->Error("$what: subinterface $prefix$name does not allow connecting below it");
		}
		my %sub_interfaces = $iface_comp->find_subobjects_hash('Component::InterfaceInstance');
		push @interfaces, match_interfaces($system, \%sub_interfaces, 0, $orig_endpoint, $remain, $prefix.$name.".", $internal, $what." .");
	    } else {
		my $endpoint = $orig_endpoint->dup;
		$endpoint->set_name($prefix.$name);
		$endpoint->set_interface($iface_comp);
		$endpoint->set_internal($internal);
		$endpoint->set_array_name($is_array ? $name : undef);
		push @interfaces, $endpoint;
	    }
	}
    }
    $system->Info("$what: matched ".scalar(@interfaces)." interfaces: ".join(", ", map($_->name, @interfaces))) if @interfaces;
    return @interfaces;
}
sub lookup_endpoint {
    my ($component, $endpoint, $name, $portnum, $subcomponents, $interfaces) = @_;
    my @interfaces;
    my $target_name;
    if (ref($endpoint) eq "") {
	$target_name = $endpoint;
	$endpoint = Component::Endpoint->new($component->system, $name, $component);
	$endpoint->set_name($target_name);
    } else {
	if (!blessed($endpoint) or !$endpoint->isa('Component::Endpoint')) {
	    if (ref($endpoint) eq "HASH") {
		bless ($endpoint, 'Component::Endpoint');
	    } else {
		$component->system->Error("lookup_endpoint: component ".$component->fullname.": connection endpoint of unexpected type ".ref($endpoint));
	    }
	}
	if (defined($endpoint->name) and ref($endpoint->name) eq "") {
	    $target_name = $endpoint->name;
	} else {
	    $component->system->Error("lookup_endpoint: missing endpoint name; only names are handled for now");
	}
    }
    my $what = $component->fullname." connection endpoint $name ($target_name)";
    my $re_list = compile_glob_name($target_name, $what);
    push @interfaces, match_interfaces($component->system, $interfaces, 0, $endpoint, $re_list, "", 1, $what." to interface");
    push @interfaces, match_interfaces($component->system, $subcomponents, 1, $endpoint, $re_list, "", 0, $what." to component");
    @interfaces or $component->system->Error("$what: no sub-component or interface matching name '$target_name'");
    return @interfaces;
}
sub expand_components {
    my ($template, $instance, $components, $map, $what) = @_;
    my $herewhat = "$what from ".$template->fullname;
    $template->system->Info("$herewhat: expand components");
    my %orig_components = $template->find_subobjects_hash('Component::Instance');
    foreach my $cname (keys %orig_components) {		
	$template->system->Info("$herewhat: expand component $cname");
	my $orig_comp = $orig_components{$cname};
	my $comp = $orig_comp->expand($instance, $cname);
	$comp->set_original($orig_comp);
	$components->{$cname} = $comp;
	$map->{$orig_comp->fullname} = $comp;
	$instance->assign_named_member($template, $cname, $comp);
    }
}
sub expand_interfaces {
    my ($template, $instance, $interfaces, $map, $what) = @_;
    my $herewhat = "$what from ".$template->fullname;
    $template->system->Info("$herewhat: expand interfaces");
    my %orig_interfaces = $template->find_subobjects_hash('Component::InterfaceInstance');
    foreach my $iname (keys %orig_interfaces) {
	$template->system->Info("$herewhat: expand interface $iname");
	my $orig_iface = $orig_interfaces{$iname};
	my $intf = $orig_iface->expand($instance, $iname);
	$intf->set_original($orig_iface);
	$interfaces->{$iname} = $intf;
	$map->{$orig_iface->fullname} = $intf;
	$instance->assign_named_member($template, $iname, $intf);
    }
}
sub expand {
    my ($template, $parent, $name) = @_;
    my $system = $template->system;
    my (%components, %interfaces);
    my %map;		
    my $instance = Component::Instance->new($system, $name, $parent, $template);
    my $what = "expand component ".$template->fullname." to ".$instance->fullname;
    $system->Info($what);
    if (defined($template->type)) {
	$template->type->expand_components($instance, \%components, \%map, "type of $name");
	$template->type->expand_interfaces($instance, \%interfaces, \%map, "type of $name");
    }
    $template->expand_components($instance, \%components, \%map, $name);
    $template->expand_interfaces($instance, \%interfaces, \%map, $name);
    $instance->set_connections( [] );
    my @connections = $template->connections;
    foreach my $c (0 .. $#connections) {
	my $conn = $connections[$c];
	ref($conn) eq "ARRAY" and @$conn == 2 or $system->Error("$what: found a connection that is not an array of two entries");
	my ($a_endpoint, $b_endpoint) = @$conn;
	my @a_endpoints = $template->lookup_endpoint($a_endpoint, "connections${c}_0", 0, \%components, \%interfaces);
	my @b_endpoints = $template->lookup_endpoint($b_endpoint, "connections${c}_1", 1, \%components, \%interfaces);
	@a_endpoints == @b_endpoints or $system->Error("$what: unequal number of endpoints on both sides of a connection");
	for my $i (0 .. $#a_endpoints) {
	    my $a = $a_endpoints[$i];
	    my $b = $b_endpoints[$i];
	    my $ewhat = "$what: connecting ".$a->name." to ".$b->name;
	    $system->types_equivalent($a->interface->type, $b->interface->type)
		or $system->Error("$ewhat: incompatible types ".$a->interface->type->fullname." and ".$b->interface->type->fullname
			." --- for interfaces ".$a->interface->fullname." and ".$b->interface->fullname);
	    my @pair = ($a, $b);
	    foreach my $i (0 .. 1) {
		my $endpoint = $pair[$i];
		my $other = $pair[1 - $i];
		if ($endpoint->array_name) {
		    my $i_array = ($endpoint->interface->parent->{$endpoint->array_name} //= []);
		    my $max = $endpoint->interface->{array_max};
		    if (defined($max) and @$i_array >= $max) {
			$system->Error("$ewhat: ".$endpoint->name." array exceeding max of $max entries");
		    }
		    my $index = scalar(@$i_array);
		    my $new_iface = $endpoint->interface->dup($endpoint->interface->parent, $endpoint->array_name . $index);
		    delete $new_iface->{max};
		    push @$i_array, $new_iface;
		    $endpoint->set_interface($new_iface);
		    $endpoint->set_name($endpoint->name . $index);
		    $endpoint->set_array_name(undef);
		    $system->Info("created dynamic interface array entry %s\n", $endpoint->name);
		}
		my $refpoint = ($endpoint->internal ? \$endpoint->interface->{alias_to} : \$endpoint->interface->{connected_to});
		defined($$refpoint) and $system->Error("$ewhat: ".$endpoint->name." is already connected to ".($$refpoint)->fullname);
		$$refpoint = $other->interface;
	    }
	    $instance->push_connections(\@pair);
	}
    }
    return $instance;
}
1;
