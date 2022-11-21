# Copyright (c) 2008-2018 by Tensilica Inc.  ALL RIGHTS RESERVED.
package Stash;
our $QUIET_HACK = 1;	
our $force_no_xml = 0;	
our $global_debug;
our $stash_debug;
BEGIN {
    $global_debug = (($ENV{"XTENSA_INTERNAL_DEBUG"} or "") =~ /DEBUG/) ? 1 : 0;
    $stash_debug  = (($ENV{"XTENSA_INTERNAL_DEBUG"} or "") =~ /STASH/) ? 1 : 0;
    $SIG{__DIE__} = sub { require Carp; Carp::confess(@_) } if $global_debug;
}
use Exporter 'import';
@EXPORT = qw(stash_xml_write stash_xml_read stash_dict_new stash_dict_import stash_lookup stash_obj_isa);
use Object;
use strict;
our $HMODEL_HASH = 0;
our $HMODEL_OBJECT = 1;
our $HMODEL_HASHOBJ = 2;
our $AMODEL_ARRAY = 0;
our $AMODEL_ARRAYOBJ = 1;
#	  <hash   n="default" ... type of unspecified hash entries ...	# default is unrestricted ("*")
#	  <array  n="type"  value="TYPE[,TYPE[,...]]"/>		# valid alternative for type="..." above
#	  <array  n="subtypes">				# valid alternative for subtypes="..." above
sub dumpit {
  my ($x,$lev,$ptr) = @_;
  return "<undef>" unless defined($x);
  defined($lev) or $lev = 0;
  return "..." if $lev > 3;
  my $t = (UNIVERSAL::isa($x,"UNIVERSAL") ? ref($x).":" : "");
  return $t.($ptr?$x:"")."{".join(",",map($_."=".dumpit($x->{$_},$lev+1,$ptr), sort keys %$x))."}"
    if UNIVERSAL::isa($x,"HASH");
  return $t.($ptr?$x:"")."[".join(",",map(dumpit($_,$lev+1,$ptr),@$x))."]"
    if UNIVERSAL::isa($x,"ARRAY");
  my $str = $x . "";
  $str =~ s/\\/\\\\/g;
  $str =~ s/\'/\\\'/g;
  return "$t'$x'";
}
sub package_loaded {
    my ($package_name) = @_;
    no strict 'refs';
    return( @{"${package_name}::ISA"} != 0 );
}
sub lookup_name {
  my ($obj, $objname, $name) = @_;
  my $whyundef = "is undef";
  defined($name) or return ($obj, undef);
  foreach my $coded_part (split(/\./, $name)) {
    defined($obj) or return (undef, "'$objname' $whyundef, cannot lookup '$coded_part' (of $name) in it");
    $whyundef = "is undef";
    $name ne "" or return (undef, "badly formed name '$name' at '$objname'");
    my $part = key_decode($coded_part);
    if ($part =~ /^\d+$/ and		
	(ref($obj) eq "ARRAY" or UNIVERSAL::isa($obj, "ArrayObject"))) {
      exists($obj->[$part]) or $whyundef = "not found";
      $obj = $obj->[$part];
      $objname .= ".$part";
    } elsif (UNIVERSAL::isa($obj, "ObjectClass")) {	
      my $oref = $obj->getref($part);
      if (defined($oref)) {
	$obj = $$oref;
      } else {
	$obj = undef;
	$whyundef = "not a member";
      }
      $objname .= ".$part";
    } elsif (UNIVERSAL::isa($obj, "HASH")) {
      exists($obj->{$part}) or $whyundef = "not found";
      $obj = $obj->{$part};
      $objname .= ".$part";
    } else {
      return (undef, "unrecognized or unexpected '$objname' (obj ref = ".ref($obj)."), cannot lookup '$coded_part' of '$name' in it");
    }
  }
  return ($obj, undef);
}
sub stash_lookup {
  my ($dict, $name) = @_;
  return (undef, undef) unless defined($name);
  return lookup_name($dict->{roots}, "(root)", $name);
}
sub stash_dict_empty {
  my ($name) = @_;
  my $dict = {};
  $dict->{name} = $name;
  $dict->{roots} = {};
  $dict->{roots}{types} = {};
  $dict->{types} = $dict->{roots}{types};
  $dict->{objs} = {};
  $dict->{skiptype} = {};
  $dict->{skipname} = {};
  stash_catalog_object($dict, $dict->{roots}, "", 1, undef, 0);
print STDERR ">>>> new dict $name: roots=".$dict->{roots}."\n" if $stash_debug;
print STDERR ">>>> new dict $name: types=".$dict->{types}."\n" if $stash_debug;
  return $dict;
}
my $base_dict;
sub stash_base_dict {
  if (!defined($base_dict)) {
    $base_dict = stash_dict_empty("basic types");
    %{$base_dict->{types}} = (
      hash   => { type => ["hash"],   basetype => "hash",   named=>1, name => "hash",  objname => "types.hash", flat => {}, flatorder => [], parents => [] },
      array  => { type => ["array"],  basetype => "array",  named=>1, name => "array", objname => "types.array", aflat => [], parents => [] },
      any    => { type => ["any"],    basetype => "any",    named=>1, name => "any", objname => "types.any", parents => [] },
      string => { type => ["string"], basetype => "string", named=>1, name => "string", objname => "types.string", parents => [] },
      u32    => { type => ["u32"],    basetype => "u32",    named=>1, name => "u32", objname => "types.u32", parents => [] },
      i32    => { type => ["i32"],    basetype => "i32",    named=>1, name => "i32", objname => "types.i32", parents => [] },
      u64    => { type => ["u64"],    basetype => "u64",    named=>1, name => "u64", objname => "types.u64", parents => [] },
      i64    => { type => ["i64"],    basetype => "i64",    named=>1, name => "i64", objname => "types.i64", parents => [] },
      float  => { type => ["float"],  basetype => "float",  named=>1, name => "float", objname => "types.float", parents => [] },
      type_tree => { type => ["hash"],basetype => "hash",   named=>1, name => "type_tree", objname => "types.type_tree", flat => {}, flatorder => [], parents => [] },
      typedef=> { type => ["hash"],
		  default => { type => ["annotation"] },
		  entries => {
		    type      => { type => ["array"], subtypes => ["string"] },
		    range     => { type => ["array"], subtypes => ["string"] },
		    entries   => { type => ["typedef_hash"] },
		    default   => { type => ["typedef"] },
		    subtypes  => { type => ["array"], subtypes => ["string"] },
		    sizerange => { type => ["array"], subtypes => ["string"] },
		    namemap   => { type => ["array"], subtypes => ["sigmap"] },
		    objmodel  => { type => ["i32"] },
		    parents   => "typedef_array",
		    flat      => { type => ["flat_hash"] },
		    flatorder => { type => ["array"] },
		    aflat     => { type => ["typedef_array"] },
		    name      => { type => ["string"] },
		    named     => { type => ["i32"] },
		    objname   => { type => ["string"] },
		    basetype  => { type => ["string"] },
		  } },
      typedef_hash => { type => ["hash"], default => { type => ["typedef"] } },
      flat_hash    => { type => ["hash"], default => "typedef_hash" },
      typedef_array => { type => ["array"], subtypes => ["typedef"] },
      sigmap => { type => ["hash"],
		  entries => {
		      name    => { type => ["string"] },
		      type    => { type => ["string"] },
		      signame => { type => ["string"] },
		  } },
      set_member => { type => ["i32"], range => [1] },
      set => { type => ["hash"], default => { type => ["set_member"] } },
      annotation => { type => ["string"] },
    );
print STDERR ">>>> base dict roots=".$base_dict->{roots}."\n" if $stash_debug;
print STDERR ">>>> base dict types=".$base_dict->{types}."\n" if $stash_debug;
    stash_catalog_object($base_dict, $base_dict->{roots}{types}, "types", 0, undef, 0);
    my $objinfo = $base_dict->{objs}{$base_dict->{types}{typedef}};
    $objinfo->{order} = ['type','default','entries'];
    $objinfo = $base_dict->{objs}{$base_dict->{types}{typedef}{entries}};
    $objinfo->{order} = ['type','range','entries','default','subtypes','sizerange','namemap'];
    $objinfo = $base_dict->{objs}{$base_dict->{types}{sigmap}};
    $objinfo->{order} = ['type','entries'];
    stash_catalog_object($base_dict, $base_dict->{roots}{types}, "types", 0,
			  dict_lookup_type($base_dict, "typedef_hash"), 0);
print STDERR ">>>> (bis) base dict roots=".$base_dict->{roots}."\n" if $stash_debug;
print STDERR ">>>> (bis) base dict types=".$base_dict->{types}."\n" if $stash_debug;
    dict_expand_all_types($base_dict);
  }
  return $base_dict;
}
sub stash_dict_new {
  my ($name) = @_;
  $name = "(unnamed)" unless defined($name);
  my $dict = stash_dict_empty($name);
  my $basedict = stash_base_dict();
  stash_dict_import($dict, $basedict);
  my $types_type = dict_lookup_type($dict, "typedef_hash");
  defined($types_type) or die "couldn't find typedef_hash for dict $name";
  stash_catalog_object($dict, $dict->{roots}{types}, "types", 1, $types_type, 1);
  my $objinfo  = $dict->{objs}{$dict->{types}};
  my $bobjinfo = $dict->{objs}{$basedict->{types}};
defined($objinfo) or print STDERR ">}}>> $name: types objinfo doesn't exist!\n" if $stash_debug;
  push @{$objinfo->{order}}, @{$bobjinfo->{order}};
print STDERR ">>>> dict '$name': types=".$dict->{types}.", roots.types=".$dict->{roots}{types}.", types type=$types_type, objinfo type=".$objinfo->{type}."\n" if $stash_debug;
  return $dict;
}
sub stash_dict_import {
  my ($dict, $idict, $withroots, $merge) = @_;
  $withroots = 1 unless defined($withroots);
  $merge = 0 unless defined($merge);
  my $what = "importing dictionary '".$idict->{name}."' into '".$dict->{name}."'";
  $dict ne $idict or die "$what: cannot import into self";
  print STDERR "### $what", ($withroots ? " (with roots)" : ""), ($merge ? " (merged)" : ""), "\n" if $stash_debug;
  if ($withroots) {
    foreach my $root (keys %{$idict->{roots}}) {
      next if $root eq "types";
      die "$what: already has root $root" if exists($dict->{roots}{$root})
	  and $dict->{roots}{$root} ne $idict->{roots}{$root};
      $dict->{roots}{$root} = $idict->{roots}{$root};
    }
  }
  foreach my $tname (sort keys %{$idict->{types}}) {
    my $itype = $idict->{types}{$tname};
    if (!exists($dict->{types}{$tname})) {
      $dict->{types}{$tname} = $itype;
    } else {
      my $type = $dict->{types}{$tname};
      if ($type eq $itype) {
      } elsif (dict_type_equivalent($type, $itype, $tname)) {
	die "$what: already has an equivalent type $tname, merging them not supported\n"
		." existing = ".dumpit($type,1,1)."\n"." imported = ".dumpit($itype,1,1);
	print STDERR "### importing: type $tname is equivalent to existing one, refactoring\n";
	$idict->{types}{$tname} = $type;
	my @more = ($idict);
	my %seen;
	while (@more) {
	    my $obj = shift @more;
	    next unless defined($obj);
	    next if exists($seen{$obj});
	    $seen{$obj} = 1;
	    if (UNIVERSAL::isa($obj, "HASH")) {
		foreach my $key (sort keys %$obj) {
		    my $value = $obj->{$key};
		    next unless defined($value);
		    if ($value eq $itype) {
			$obj->{$key} = $type;
		    } elsif (ref($value)) {
			push @more, $value;
		    }
		}
	    } elsif (UNIVERSAL::isa($obj, "ARRAY")) {
		foreach my $i (0 .. $#$obj) {
		    my $value = $obj->[$i];
		    next unless defined($value);
		    if ($value eq $itype) {
			$obj->[$i] = $type;
		    } elsif (ref($value)) {
			push @more, $value;
		    }
		}
	    }
	}
	$itype = { type => [$tname] };
      } else {
	die "$what: already has non-equivalent type $tname\n (existing=".dumpit($dict->{types}{$tname},1,1).",  imported=".dumpit($itype,1,1).")";
      }
    }
  }
  if ($idict ne stash_base_dict()) {
    die "oops, no types objs" unless exists $dict->{objs}{$dict->{types}};
    my  $objinfo =  $dict->{objs}{ $dict->{types}};
    my $iobjinfo = $idict->{objs}{$idict->{types}};
    push @{$objinfo->{order}}, @{$iobjinfo->{order}};
  }
  foreach my $obj (keys %{$idict->{objs}}) {
    die "$what: already has object ".$obj->{bestfull} if exists($dict->{objs}{$obj})
	and $dict->{objs}{$obj} ne $idict->{objs}{$obj};
    $dict->{objs}{$obj} = $idict->{objs}{$obj};
$dict->{name} or die "dict-oops 7";
$idict->{name} or die "dict-oops 7b";
    $dict->{objs}{$obj}{dict} = $dict if $merge and $idict->{objs}{$obj}{dict} eq $idict;
  }
}
sub dict_lookup_type {
  my ($dict, $typename, $create, @expanded_types) = @_;
  defined($typename) or die "dict_lookup_type: undefined typename";
  if (grep($_ eq $typename, @expanded_types) > 1) {
    die "ERROR: type definition loop for type $typename: ".join(", ",@expanded_types);
  }
  push @expanded_types, $typename;
  UNIVERSAL::isa($dict->{types}, "HASH")	# or UNIVERSAL::isa($dict->{types}, "ObjectClass")
	or die "dict: types must be in a hash (is a ".ref($dict->{types}).")";
  my $named_type = 1;
  my $typehash = $dict->{types};
  my $objname = "types";
  my $what = "";
  my $tname = $typename;
  while ($tname =~ s/^(\w+)\.//) {		
    my $coded_prefix = $1;
    my $prefix = key_decode($coded_prefix);
    $objname .= ".$prefix";
    $what = ($what eq "") ? $prefix : "$what.$prefix";
    my $nextype = $typehash->{$prefix};		
    if (!defined($nextype)) {
      $prefix =~ /^\d+$/ and die "numeric type name components not supported ($typename)";
      $create or return undef;	
      $named_type or die "cannot create type prefix $what (for $tname): not a named type (reached through a non-type_tree type)";
      $nextype = $typehash->{$prefix} = { type => ["type_tree"], entries => {} };
    }
    if ($nextype->{type}[0] ne "type_tree") {
      $named_type = 0;
print STDERR "==>==>==>  unnamed type lookup '$typename' (at $what)\n" if $stash_debug;
    } elsif ($named_type) {
      $nextype->{basetype} and die "intermediate type node $what unexpectedly expanded, attempted as prefix for $tname";
    }
    $typehash = $nextype->{entries};
    $objname .= ".entries";
    defined($typehash) or die "internal error: expanded type $what does not have an {entries} field (for $tname)";
  }
  $tname =~ /^\w+$/ or die "invalid type '$tname' ($typename)";
  $objname .= ".$tname";
  return undef unless $create or exists($typehash->{$tname});
  my $otyperef = \$typehash->{$tname};
  if ($create and !defined($$otyperef)) {
      $named_type or die "cannot create $tname: not a named type (parts of its prefix, $what, was reached through a non-type_tree type)";
      $$otyperef = ref($create) ? $create : { type => [$create] };
  }
  return dict_expand_type($dict, $otyperef, "", $typename, $named_type, $objname, @expanded_types);
}
sub dict_expand_type {
  my ($dict, $otyperef, $owhat, $typename, $named_type, $objname, @expanded_types) = @_;
  my $otype = $$otyperef;
  defined($otype) or return undef;		
  defined($named_type) or $named_type = 0;	
  my $what = $owhat;
  $what .= ": " if $what ne "";
  if (!defined($typename)) {
    my $objinfo = $dict->{objs}{$otype};
    $typename = $objinfo->{bestfull} if defined($objinfo);
    defined($typename) or die "ERROR: ${what}can't figure out type name (named_type=$named_type) otype=".dumpit($otype);
    $what .= "objinfo type $typename";
  } else {
    $what .= "type $typename";
  }
  if (UNIVERSAL::isa($otype, "HASH") # or UNIVERSAL::isa($otype, "ObjectClass")
	and defined($otype->{basetype})) {
    if ($named_type) {
      if ($otype->{named}) {
	print STDERR "... type ".$otype->{name}." also has name $typename, keeping first\n"
		if $stash_debug and $otype->{name} ne $typename;
      } else {
	print STDERR "... type ".$otype->{name}." (unnamed) given real type name $typename\n"
		if $stash_debug;
	$otype->{name} = $typename;
	$otype->{named} = 1;
      }
    }
    return $otype;
  }
  if (ref($otype) eq "") {
print STDERR "... expanding type string '$otype' (for $what)\n" if $stash_debug;
    $otype = { type => [split(",",$otype)] };
    $$otyperef = $otype;
  }
  exists($otype->{type})         or die "ERROR: $what: missing type='...' tag";
  defined($otype->{type})        or return undef;	
  ref($otype->{type}) eq "ARRAY" or UNIVERSAL::isa($otype->{type}, "ArrayObject")
				 or die "ERROR: $what: type='...' tag is not an expected array";
  my @stypes = @{$otype->{type}};
  scalar(@stypes) > 0            or die "ERROR: $what: empty type='...' tag";
  my @supertypes = ();
  my $supername = shift @stypes;	
  $supername eq "type_tree" and die "$what: cannot expand type name hierarchy container (type type_tree)";
  my $supertype = dict_lookup_type($dict, $supername, undef, @expanded_types);
  defined($supertype) or die "ERROR: $what: type '$supername' not found";
  UNIVERSAL::isa($supertype, "HASH")	# or UNIVERSAL::isa($supertype, "ObjectClass")
	or die "ERROR: $what: type '$supername' not a hash";
  push @supertypes, $supertype;
  my $basetype = $supertype->{basetype};
  if (scalar(grep(defined($_), values %$otype)) == 1 and @stypes == 0	
      and !$named_type) {
    print STDERR "### REMAPPING ($what) to $supername.\n" if $stash_debug;
    $$otyperef = $supertype;				
    return $supertype;
  }
  defined($otype->{parents})   and die "$what: type has extraneous parents field";
  defined($otype->{name})      and die "$what: type has extraneous name field";
  defined($otype->{objname})   and die "$what: type has extraneous objname field";
  defined($otype->{flat})      and die "$what: type has extraneous flat field";
  defined($otype->{aflat})     and die "$what: type has extraneous aflat field";
  defined($otype->{flatorder}) and die "$what: type has extraneous flatorder field";
  $otype->{parents}   = \@supertypes;
  $otype->{name}      = $typename;
  $otype->{named}     = $named_type;
  $otype->{objname}   = $objname;
  $otype->{basetype}  = $basetype;
  $otype->{inperl}    = 0;		
  my $supermodel = $supertype->{objmodel};	
  my $objmodel = $otype->{objmodel};
  if (defined($supermodel)) {
    if (!defined($objmodel)) {
      $otype->{objmodel} = $objmodel = $supermodel;	
    } elsif ($objmodel != $supermodel) {
      die "$what: type has different objmodel ($objmodel) than parent type $supername ($supermodel)";
    }
  } elsif (defined($objmodel)) {
    print STDERR "WARNING: $what: type specifies objmodel $objmodel but parent type $supername does not specify one\n" if $stash_debug;
  }
  my $oflat;
  if ($basetype eq "hash") {
    $otype->{flat} = $oflat = { %{$supertype->{flat}} };
    $otype->{flatorder}     = [ @{$supertype->{flatorder}} ];
  } elsif ($basetype eq "array") {
    $otype->{aflat}     = [ @{$supertype->{aflat}} ];
  }
print STDERR "... processing $what\n" if $stash_debug;
  foreach $supername (@stypes) {
    $supername eq "type_tree" and die "$what (multiple inheritance): cannot expand type name hierarchy container (type type_tree)";
    ($basetype eq "hash" or $basetype eq "array")
      or die "ERROR: $what: only hash and array types can have multiple inheritance";
    $supertype = dict_lookup_type($dict, $supername, undef, @expanded_types);
    defined($supertype) or die "ERROR: $what: parent type '$supername' not found";
    push @supertypes, $supertype;
    $supermodel = $supertype->{objmodel};	
    if (defined($supermodel)) {
      if (!defined($objmodel)) {
	print STDERR "WARNING: $what: alternate parent type $supername defines objmodel $supermodel but first parent type does not define any\n" if $stash_debug;
	$otype->{objmodel} = $objmodel = $supermodel;		
      } elsif ($objmodel != $supermodel) {
	die "$what: type has different objmodel ($objmodel) than alternate parent type $supername ($supermodel)";
      }
    } elsif (defined($objmodel)) {
      print STDERR "WARNING: $what: type specifies objmodel $objmodel but alternate parent type $supername does not specify one\n" if $stash_debug;
    }
    print STDERR "WARNING: $what: multiple inheritance ($supername)\n" if $stash_debug;
    $supertype->{basetype} eq $basetype
      or die "ERROR: $what: parent type $supername is not basetype $basetype";
    if ($basetype eq "hash") {
      my $superflat = $supertype->{flat};
      foreach my $key (sortkeys($superflat,$supertype->{flatorder})) {
	my $superkeyinfo = $superflat->{$key};
	my $okeyinfo = $oflat->{$key};
	if (defined($okeyinfo)) {
	  if (!dict_type_isa($dict, $superkeyinfo->{t}, $okeyinfo->{t})) {
	    die "ERROR: $what: $supername mismatching override for key $key";
	  }
	} else {
	  push @{$otype->{flatorder}}, $key;
	}
	$oflat->{$key} = $superkeyinfo;
      }
    } else {
      push @{$otype->{aflat}}, @{$supertype->{aflat}};
    }
  }  
  if ($basetype eq "hash") {
    my $entries = $otype->{entries};
    if (defined($otype->{default})) {
      $entries->{"*"} = $otype->{default};
    }
    if (defined($entries)) {
      my $objinfo = $dict->{objs}{$entries};
      foreach my $key (sortkeys($entries, ($objinfo ? $objinfo->{order} : []))) {
	my $coded_key = key_encode($key);
	my $okeyinfo = $oflat->{$key};
	my $entry = dict_expand_type($dict, \$entries->{$key}, $owhat,
			$typename.".".$coded_key, 0,
			$objname.($key eq "*" ? ".default" : ".entries.$coded_key"),
			@expanded_types);
	if (defined($okeyinfo)) {
	  my $priortype = $okeyinfo->{t};
	  if (!dict_type_isa($dict, $entry, $priortype)) {
	    die "ERROR: $what: mismatching override for key $coded_key (".
		stash_type_name($dict,$entry)." is not a subclass of ".
		stash_type_name($dict,$priortype).")";
	  }
	} else {
	  push @{$otype->{flatorder}}, $key;
	}
	$oflat->{$key} = $okeyinfo = { t => $entry, a => $otype };
      }
      $otype->{default} = $entries->{"*"} if exists($entries->{"*"});
    }
  }
  if ($basetype eq "array" and defined($otype->{subtypes})) {
    foreach my $entry (@{$otype->{subtypes}}) {
      my $i = @{$otype->{aflat}};	
      $otype->{aflat}[$i] = $entry;	
      dict_expand_type($dict, \$otype->{aflat}[$i], $owhat, $typename.".".$i, 0,
	$objname.".subtypes.$i", @expanded_types);
    }
  }
  return $otype;
}
sub dict_expand_all_types {
  my ($dict) = @_;
  my $types = $dict->{types};
  print STDERR "EXPANDING ALL TYPES (", scalar(keys %$types), " types):\n"
    if $stash_debug;
  foreach my $typename (sort keys %$types) {
    my $stypes = $types->{$typename}{type};
    if (defined($stypes) and
	(ref($stypes) eq "" and $stypes eq "type_tree"
	 or $stypes->[0] eq "type_tree")) {
      print STDERR "SKIPPING  $typename (type name prefix)\n" if $stash_debug;
      next;
    }
    print STDERR "EXPANDING $typename ...\n" if $stash_debug;
    dict_expand_type($dict, \$types->{$typename}, "", $typename, 1, "types.$typename");
  }
  print STDERR "EXPANDING DONE.\n" if $stash_debug;
}
sub dict_type_equivalent {
  my ($a, $b, $name) = @_;
  if (ref($a) eq "") { print STDERR "dict_type_equivalent: unprocessed type a ($name)\n"; return 0; }
  if (ref($b) eq "") { print STDERR "dict_type_equivalent: unprocessed type b ($name)\n"; return 0; }
  return 1 if $a eq $b;			
  foreach my $key (keys %$a, keys %$b) {
    next if $key =~/^inperl$/;
    my ($aval, $bval) = ($a->{$key}, $b->{$key});
    next if !defined($aval) and !defined($bval);
    if (!defined($aval) or !defined($bval)) {
      print STDERR "dict_type_equivalent: one of the compared types has an undefined field ($name)\n";
      return 0;
    }
    next if $aval eq $bval;
    my $coded_key = key_encode($key);
    if (ref($aval) ne ref($bval)) { print STDERR "--- key $coded_key: '".ref($aval)."' ne '".ref($bval)."'\n"; return 0; }
    if (ref($aval) eq "ARRAY") {
      if (@$aval != @$bval) { print STDERR "--- key $coded_key array: unequal size\n"; return 0; }	
      foreach my $i (0 .. $#$aval) {
	if ($aval->[$i] ne $bval->[$i]) { print STDERR "--- key $coded_key array entry $i: unequal\n"; return 0; }	
      }
    } elsif (UNIVERSAL::isa($aval, "HASH")) {
      if (scalar(keys(%$aval)) != scalar(keys(%$bval))) { print STDERR "--- key $coded_key hash: unequal size\n"; return 0; }
      foreach my $subkey (keys %$aval, keys %$bval) {
	my $coded_subkey = key_encode($subkey);
	if ($aval->{$subkey} ne $bval->{$subkey}) { print STDERR "--- key $coded_key hash subkey $coded_subkey: unequal\n"; return 0; }	
      }
    } else {
      print STDERR "--- key $coded_key: not hash or array (is ".ref($aval).")\n";
      return 0;		
    }
  }
  return 1;		
}
sub dict_type_isa {
  my ($dict, $type, $ancestor) = @_;
  defined($type->{basetype}) or die "ERROR: dict_type_isa: unprocessed type (".stash_type_name($dict,$type).")";
  defined($ancestor) or die "ERROR: dict_type_isa: undefined ancestor type";	
  return 1 if $type eq $ancestor;
  foreach my $supertype (@{$type->{parents}}) {
    defined($supertype) or die "dict_type_isa: type '".stash_type_name($dict,$type)."' parent type is undef";
    my $supername = stash_type_name($dict, $supertype);
    return 0 if $supertype eq $type;	
    return 1 if dict_type_isa($dict, $supertype, $ancestor);
  }
  return 0;		
}
sub stash_obj_isa {
  my ($dict, $obj, $ancestor) = @_;
  return 1 unless defined($ancestor);
  if (ref($ancestor) eq "") {		
    my $atype = dict_lookup_type($dict, $ancestor);
    defined($atype) or die "ERROR: stash_obj_isa: unknown type '$ancestor'";
    $ancestor = $atype;
  }
  my $objinfo = $dict->{objs}{$obj};
  my $objtype = $objinfo->{type};
  if (!defined($objtype)) {
    $objtype = ref($obj);
    $objtype ne "" or die "ERROR: stash_obj_isa: type lookup not supported for scalar objects";
    $objtype = "hash" if $objtype eq "HASH";
    $objtype = "array" if $objtype eq "ARRAY";
print STDERR "-->--> stash_obj_isa looking up $objtype .\n" if $stash_debug;
    $objtype = dict_lookup_type($dict, $objtype);
  }
  defined($objtype) or die "ERROR: stash_obj_isa: object of unknown type and not in dictionary"
				." (ref ".ref($obj).")";
  return dict_type_isa($dict, $objtype, $ancestor);
}
sub stash_package_to_typename {
  my ($name) = @_;
  die "Prepended, appended, or double underscores not allowed in perl object type names: $name\nStopped"
	if $name =~ /^_/ or $name =~ /_$/ or $name =~ /__/;	
  $name =~ s/::/./g;
  $name;
}
sub stash_typename_to_package {
  my ($name) = @_;
  die "Prepended, appended, or double period not allowed in Stash type names: $name\nStopped"
	if $name =~ /^\./ or $name =~ /\.$/ or $name =~ /\.\./;	
  $name =~ s/\./::/g;
  $name;
}
sub stash_check_type {
  my ($dict, $typeref, $whatref, $where) = @_;
  if (defined($$typeref) and ref($$typeref) eq "") {
    my $typename = $$typeref;
    $$whatref = "$typename type" unless defined($$whatref);
    $$typeref = dict_lookup_type($dict, $typename);
    defined($$typeref) or die "$where: $$whatref (specified by name) doesn't exist in Stash";
  }
}
sub stash_create_hash_object {
  my ($dict, $type, $what) = @_;
  return ({}, $HMODEL_HASH) unless defined($type) and $type->{named};	
  my ($package, $objmodel) = stash_create_hash_package($dict, $type, $what);
  defined($package) or die "ERROR undef package for $what (type ".stash_type_name($dict,$type).")";
  my $obj = $package->o_new();	
  return ($obj, $objmodel);
}
sub stash_create_hash_object_maybetypename {
  my ($dict, $type, $what) = @_;
  stash_check_type($dict, \$type, \$what, "stash_create_hash_object_maybetypename");
  return stash_create_hash_object($dict, $type, $what);
}
sub stash_create_hash_package_byname {
  my ($dict, $type, $what) = @_;
  stash_check_type($dict, \$type, \$what, "stash_create_hash_package_byname");
  return stash_create_hash_package($dict, $type, $what);
}
sub stash_create_hash_package {
  my ($dict, $type, $what) = @_;
  defined($type) or die "really?";
  my $pname = stash_typename_to_package($type->{name});
defined($pname) or print STDERR "OOOOPS: undef package for $what, type $type, type name '".$type->{name}."'\n";
  if (! stash_type_is_variable_hash($dict, $type)) {
    if ($type->{inperl}) {		
      return ($pname, $HMODEL_OBJECT);
    }
    defined($type->{objmodel}) or $type->{objmodel} = $HMODEL_OBJECT;
    my $parents = $type->{parents};
    if (@$parents > 1) {			
      die "stash_create_hash_package: $what: multiple inheritance not yet supported with Object.pm, type $pname";
    }
    my ($parent_package, $parent_objmodel);
    my $parent_type = $parents->[0];
    defined($parent_type) or die "stash_create_hash_package: $what: oops, undef parent type";
    my $parent_name = $parent_type->{name};
    if ($parent_name ne "hash") {	
      ($parent_package, $parent_objmodel) = stash_create_hash_package($dict, $parent_type, "parent $parent_name of $what");
      $parent_objmodel eq $HMODEL_OBJECT or die "stash_create_hash_package: $what: package $pname is Object.pm managed, but parent package $parent_package is not";
print STDERR "%%%% $what: package of parent $parent_name is $parent_package\n" if $stash_debug;
    }
    my ($typehash, $o_parent) = Object::MemberTypes($pname);
    if (defined($typehash)) {
      !defined($parent_package) && !defined($o_parent) or $parent_package eq $o_parent
	  or die "stash_create_hash_package: $what: for type ".$type->{name}
		.", dict ".$dict->{name}." shows parent=$parent_package but Object.pm has parent=$o_parent";
      my @fields = ();
      foreach my $key (@{$type->{flatorder}}) {
	my $coded_key = key_encode($key);
	my $objtype = $typehash->{$key};
	defined($objtype) or die "stash_create_hash_package: $what: dict ".$dict->{name}
		." has key $coded_key for type ".$type->{name}." that Object.pm does not have for package $pname";
	my $subtype = stash_hash_subtype($dict, $type, $key, "package $pname");
	my $expected = '$';
	if (defined($subtype)) {
	  my $basetype = $subtype->{basetype};
	  if ($basetype eq "array") {
	    $expected = '@';
	  } elsif ($subtype->{name} eq "objhash") {	
	    $expected = '%';
	  }
	}
	if ($objtype ne $expected) {
	  die "stash_create_hash_package: $what: dict ".$dict->{name}
		." expects perl type $expected for key $coded_key of type ".$type->{name}
		.", Object.pm has $objtype for that key for package $pname";
	}
      }
print STDERR "%%%%%%%%%%  package $pname - Stash and Object.pm match\n" if $stash_debug;
      $type->{inperl} = 3;		
    } else {
      my @fields = ();
      foreach my $key (@{$type->{flatorder}}) {
	next unless defined($type->{entries}{$key});	
	my $subtype = stash_hash_subtype($dict, $type, $key, "package $pname");
	if (!defined($subtype)) {
	  push @fields, '$'.$key;
	} elsif ($subtype->{basetype} eq "array") {
	  push @fields, '@'.$key;
	} elsif ($subtype->{name} eq "objhash") {
	  push @fields, '%'.$key;
	} else {
	  push @fields, '$'.$key;
	}
      }
print STDERR "%%%% $what: calling _class($pname, ".(defined($parent_package)?$parent_package:"(none)").", ".join(", ",@fields).") (type ".$type->{name}.", parent ".(defined($parent_name)?$parent_name:"(none)").")\n" if $stash_debug;
      Object::_class($pname, $parent_package, @fields);
print STDERR "%%%%%%%%%%  package $pname - Stash taught Object.pm\n" if $stash_debug;
      $type->{inperl} = 1;		
    }
    return ($pname, $HMODEL_OBJECT);
  } else {
    if ($type->{inperl}) {		
      return ($pname, $type->{objmodel});
    }
    defined($type->{objmodel}) or $type->{objmodel} = $HMODEL_HASHOBJ;
    if (package_loaded($pname)) {
      UNIVERSAL::isa($pname, "HashObject")
	or die "stash_create_hash_package: $what: package $pname known to perl, but is not a HashObject";
print STDERR "%%%%%%%%%%  package $pname - Stash and HashObject probably match\n" if $stash_debug;
      $type->{inperl} = 3;		
      return ($pname, $HMODEL_HASHOBJ);
    }
    my @parent_packs;
    push @parent_packs, "HashObject" if $pname eq "hash";
    foreach my $parent_type (@{$type->{parents}}) {
      defined($parent_type) or die "stash_create_hash_package: $what (var): oops, undef parent type";
      my $parent_name = $parent_type->{name};
      if ($parent_name eq "hash") {		
	push @parent_packs, "HashObject";
	next;
      }
      my ($parent_package, $parent_objmodel) = stash_create_hash_package($dict, $parent_type, "parent $parent_name of $what");
      $parent_objmodel eq $HMODEL_OBJECT and die "stash_create_hash_package: $what: package $pname is not fixed (variable hash), but parent package $parent_package is not (is managed by Object.pm)";
      push @parent_packs, $parent_package;	
    }
    my $pkg = "package ${pname};\n"
	."our \@ISA = qw(".join(" ", @parent_packs).");\n"
	."sub empty_obj { return (".join(", ", map("'$_' => undef", grep($_ ne "*",@{$type->{flatorder}}) ))."); }\n"
	."1;\n"
	;
print STDERR "%%%%%%%%%%  ", $pkg if $stash_debug;
    eval $pkg;
print STDERR "%%%%%%%%%%  package $pname - created by Stash using HashObject (ISA ".join(",",@parent_packs).")\n" if $stash_debug;
    $type->{inperl} = 1;		
    return ($pname, $HMODEL_HASHOBJ);
  }
}
sub stash_create_array_object {
  my ($dict, $type, $what) = @_;
  return ([], $AMODEL_ARRAY) unless defined($type) and $type->{named};	
  my ($package) = stash_create_array_package($dict, $type, $what);
  my $obj = $package->o_new();		
  return ($obj, $AMODEL_ARRAYOBJ);
}
sub stash_create_array_package {
  my ($dict, $type, $what) = @_;
  defined($type) or die "oops";
  $type->{basetype} eq "array"
    or die "stash_create_array_package: $what: type is not an array";
  my $pname = stash_typename_to_package($type->{name});
  my ($typehash, $o_parent) = Object::MemberTypes($pname);
  if (defined($typehash)) {
    die "stash_create_array_package: $what: package $pname known by Object.pm, but is an array!";
  }
  if ($type->{inperl}) {		
    return ($pname, 0);
  }
  defined($type->{objmodel}) or $type->{objmodel} = $AMODEL_ARRAYOBJ;
  if (package_loaded($pname)) {
    UNIVERSAL::isa($pname, "ArrayObject")
      or die "stash_create_array_package: $what: package $pname known to perl, but is not an ArrayObject";
print STDERR "%%%%%%%%%%  package $pname - Stash and ArrayObject probably match\n" if $stash_debug;
    $type->{inperl} = 3;	
    return ($pname);
  }
#print STDERR "###################  DID NOT FIND $pname \@ISA\n" if $stash_debug and $pname eq "ArrayObject" if $stash_debug;
  my @parent_packs;
  push @parent_packs, "ArrayObject" if $pname eq "array";
  foreach my $parent_type (@{$type->{parents}}) {
    defined($parent_type) or die "stash_create_array_package: $what (array): oops, undef parent type";
    my $parent_name = $parent_type->{name};
    if ($parent_name eq "array") {		
      push @parent_packs, "ArrayObject" if $pname ne "ArrayObject";
#print STDERR "###################  AVOIDED RECURSION? FOR $pname\n" if $stash_debug and $pname eq "ArrayObject" if $stash_debug;
      next;
    }
    my ($parent_package) = stash_create_array_package($dict, $parent_type, "parent $parent_name of $what");
    push @parent_packs, $parent_package;	
  }
  eval "package ${pname};\n"
      ."our \@ISA = qw(".join(" ", @parent_packs).");\n"
      ."1;\n"
      ;
print STDERR "%%%%%%%%%%  package $pname - created by Stash using ArrayObject\n" if $stash_debug;
  $type->{inperl} = 1;	
  return ($pname);
}
sub stash_typeparents_for_package {
  my ($dict, $package_name, $default, @ancestors) = @_;
  my @parent_packages;
  {
    no strict 'refs';	
    @parent_packages = @{"${package_name}::ISA"};
  }
  @parent_packages or return ($default);
  my @parent_typenames;
  foreach my $parent_pack (@parent_packages) {
    defined($parent_pack) or die "undefined parent for $package_name";
    $parent_pack or die "empty parent for $package_name";
    $parent_pack = "hash" if $parent_pack eq "HashObject";
    $parent_pack = "array" if $parent_pack eq "ArrayObject";
    my ($ptype, $ptypename) = stash_type_for_package($dict, $parent_pack, @ancestors);
    push @parent_typenames, $ptypename;
  }
  return @parent_typenames;
}
sub stash_type_for_package {
  my ($dict, $package, @ancestors) = @_;
  my ($package_name, $objref);
  if (ref($package)) {
    $package_name = ref($package);
    $objref = $package;
  } else {
    $package_name = $package;
    $objref = bless [], $package_name;		
  }
  if (grep($_ eq $package_name, @ancestors)) {
    die "ERROR: stash_type_for_package: circular perl ISA dependency for package $package_name: ".join(", ",@ancestors);
  }
  push @ancestors, $package_name;
  my $objtypename = stash_package_to_typename($package_name);
  my $otype = dict_lookup_type($dict, $objtypename, undef);
  if (defined($otype)) {
    if (!$otype->{inperl}) {
print STDERR "stash_type_for_package: ASSUMING: type $objtypename matches perl object model\n" if $stash_debug;
    }
    return ($otype, $objtypename);
  }
  if (UNIVERSAL::isa($objref, "HashObject")) {
    my @parent_typenames = stash_typeparents_for_package($dict, $package_name, "hash", @ancestors);
print STDERR "stash_type_for_package: CONVERTING: type $objtypename from HashObject (".join(",",@parent_typenames).") ...\n" if $stash_debug;
    $otype = dict_lookup_type($dict, $objtypename, { type => [@parent_typenames] } );
    $otype->{objmodel} = $HMODEL_HASHOBJ;	
  } elsif (UNIVERSAL::isa($objref, "ObjectClass")) {	
    my ($typehash, $parentclass) = Object::MemberTypes($package_name);
    my %conversion = ('$' => '', '@' => 'array', '%' => 'objhash');		# '+' => 'i32', ...
    my $parent_name = "hash";
    if (defined($parentclass)) {
      my ($ptype, $ptypename) = stash_type_for_package($dict, $parentclass, @ancestors);
      $parent_name = $ptypename;
    }
print STDERR "stash_type_for_package: CONVERTING: type $objtypename from Object.pm ($parent_name) ...\n" if $stash_debug;
    my $newtype = { type => [$parent_name], entries => { } };
    foreach my $key (keys %$typehash) {
      my $typechar = $typehash->{$key};	
      my $typebase = $conversion{$typechar};
      my $coded_key = key_encode($key);
      defined($typebase)
	or die "stash_type_for_package: converting to stash type $objtypename:"
	      ." unexpected Object.pm typechar '$typechar' for key $coded_key";
      $newtype->{entries}{$key} = { type => ($typebase eq '' ? undef : [$typebase]) };
    }
    $otype = dict_lookup_type($dict, $objtypename, $newtype);
    defined($otype)
      or die "stash_type_for_package: error converting Object.pm ($objtypename) to stash type";
    $otype->{objmodel} = $HMODEL_OBJECT;	
  } elsif (ref($package)
	   and defined($dict->{objs}{$objref})
	   and $dict->{objs}{$objref}{basetype} eq "hash") {		
    die "stash_type_for_package: unexpected hash-like object neither Object.pm nor HashObject ($objtypename)";
  } elsif (UNIVERSAL::isa($objref, "ArrayObject")) {		
    my @parent_typenames = stash_typeparents_for_package($dict, $package_name, "array", @ancestors);
print STDERR "stash_type_for_package: CONVERTING: type $objtypename from ArrayObject (".join(",",@parent_typenames).") ...\n" if $stash_debug;
    $otype = dict_lookup_type($dict, $objtypename, { type => [@parent_typenames] } );
    $otype->{objmodel} = $AMODEL_ARRAYOBJ;	
  } elsif ($package_name eq "ARRAY") {		
print STDERR "stash_type_for_package: CONVERTING: type $objtypename to simple array ...\n" if $stash_debug;
    $otype = dict_lookup_type($dict, $objtypename, "array");
    $otype->{objmodel} = $AMODEL_ARRAY;		
  } else {
    die "stash_type_for_package: what is this? type $objtypename (package $package_name) is neither array nor expected hash-like or array-like object";
  }
  $otype->{inperl} = 2;		
  return ($otype, $objtypename);
}
sub stash_array_subtype {
  my ($dict, $type, $i, $what) = @_;
  return undef unless defined($type) and $i >= 0;
  $what .= ": index $i";
  my $aflat = $type->{aflat};			
  my $tlen = scalar(@$aflat);			
  if ($tlen == 0) {
    print STDERR "WARNING: $what: empty array type (".$type->{name}.")\n"
	unless $type->{name} eq "array" or $QUIET_HACK;	# basetype "array" expected to be empty
    return undef;
  }
  my $ti = ($i < $tlen) ? $i : $tlen - 1;	
  my $subtype = $aflat->[$ti];
  print STDERR "INFO: $what: unknown type\n" unless defined($subtype);
  return $subtype;
}
sub stash_hash_subtype {
  my ($dict, $type, $key, $what, $exists) = @_;
  my $coded_key = key_encode($key);
  return undef unless defined($type);
  my $subtype;
  my $flat = $type->{flat};
  if (exists($flat->{$key})) {
    $subtype = $flat->{$key}{t};
    $$exists = 1 if ref($exists);
  } elsif (exists($flat->{"*"})) {
    $subtype = $flat->{"*"}{t};
    $$exists = 1 if ref($exists);
  } else {
    $$exists = 0 if ref($exists);
    print STDERR "INFO: $what: unexpected key '$coded_key' of undefined type for type ".stash_type_name($dict,$type)."\n"
	unless stash_type_is_variable_hash($dict, $type);	
  }
print STDERR "INFO: $what: got key '$coded_key' from hash!\n"
	if $stash_debug and defined($subtype) and $type eq $dict->{types}{hash};
  return $subtype;
}
sub stash_type_is_variable_hash {
  my ($dict, $type) = @_;
  my $objmodel = $type->{objmodel};
  return (($objmodel == $HMODEL_OBJECT) ? 0 : 1) if defined($objmodel);
  return 1 if exists($type->{flat}{"*"});	
  return 0 if $type->{name} eq "component" or $type->{name} eq "interface";	
  return 1 unless keys %{$type->{flat}};	
  return 0;
}
sub sortkeys {
  my ($href, @arefs) = @_;
  my @keys = ();
  my %keyhash = %$href;
  foreach my $aref (@arefs) {
    foreach my $key (@$aref) {
      next unless exists($keyhash{$key});
      push @keys, $key;
      delete($keyhash{$key});
    }
  }
  push @keys, sort keys %keyhash;
  return @keys;
}
sub stash_order_roots {
  my ($dict) = @_;
  my $roots = $dict->{roots};
  my $robjinfo = stash_catalog_object($dict, $roots, "", 0, dict_lookup_type($dict, "hash"), 0);
  my $order = $robjinfo->{order};
  @$order = ( 'types',  grep($_ ne 'types' && $_ ne 'modulemap', sortkeys($roots, $order)) );
  sortkeys($roots, $order);
}
sub scalar2int {
  my $value = shift;
  return( $value =~ /^0/   ? oct($value) :
          $value =~ /^\-0/ ? -oct(substr($value,1))
	  		   : 0+$value );
}
sub stash_classify {
  my ($dict, $objref, $objfullname, $expected_type, $doneseq, $fulltypes, $allextern) = @_;
  my $basetype = defined($expected_type) ? $expected_type->{basetype} : "undef";
  if (!defined($objref)) {
    return ($basetype, undef);
  }
  if (ref($objref) eq "") {
    die "stash_classify: $objfullname is a scalar but expected type is not"
	if $basetype eq "array" or $basetype eq "hash";
    if ($basetype eq "undef" or $basetype eq "any") {
      if ($objref =~ /^\-?(0x[0-9a-f]+|0[0-7]*|[1-9][0-9]*)$/i) {
	my $n = scalar2int($objref);
	return ("u32", $n) if $objref !~ /^\-/ and $n > 0x7FFFFFFF;
	return ("i32", $n)
      }  
      return ("string", $objref);
    }
    return ($basetype, scalar2int($objref)) if $basetype eq "u32" or $basetype eq "i32";
    return ($basetype, $objref);
  }
  if (ref($objref) eq "SCALAR") {
    die "stash_classify: unsupported: stashing reference to a scalar ($objfullname = \\'".$$objref."')";
  }
  my $objinfo = $dict->{objs}{$objref};
  if (!defined($objinfo)) {
    die "should not be creating blank objinfo for types ($objfullname)"
      if !defined($expected_type) and $objfullname =~ /^types/;
    $objinfo = stash_catalog_object($dict, $objref, $objfullname, 1, $expected_type, 1);
  } else {
    my $odict = $objinfo->{dict};
    if ($odict ne $dict) {
      defined($odict) or die "oopsie skip";
      print STDERR "### Skipping $objfullname (in dict ".$odict->{name}." not ".$dict->{name}.")\n"
	  if $odict->{name} ne "basic types";	
      $odict->{name} or print STDERR "### EMPTY DICT!\n";
      return ("extern", $objinfo);
    }
    if (defined($doneseq) and $objinfo->{done} == $doneseq) {	
      print STDERR "#### LINK $objfullname",
	    ($objfullname eq $objinfo->{bestfull} ? "" : " as ".$objinfo->{bestfull}), "\n"
	    if $stash_debug;
      #print STDERR "#### OOPS $objfullname\n" if $objfullname eq $objinfo->{bestfull};
      return ("link", $objinfo->{bestfull},
		(ref($objref) eq "ARRAY" or UNIVERSAL::isa($objref,"ArrayObject")));
    }
  }
  $objinfo->{done} = $doneseq if defined($doneseq);
  $objinfo->{bestfull} = $objfullname;	# best name in the context of this {doneseq} (FIXME: don't if !defined($doneseq) !?!?)
  my $type_known_to_reader = defined($expected_type);
  my $saved_type = $objinfo->{type};
  my $saved_typeshow = stash_type_name($dict, $saved_type);
  my $saved_typename = ($saved_typeshow =~ /^\(/) ? undef : $saved_typeshow;	
  if (UNIVERSAL::isa($objref, "UNIVERSAL") and ref($objref) ne "HashObject") {
    my ($obj_type, $obj_typename) = stash_type_for_package($dict, $objref);
    if (!defined($saved_type)) {
      $saved_type = $objinfo->{type} = $obj_type;
      $saved_typename = $obj_typename;
    } elsif ($saved_type ne $obj_type) {
      die "stash_classify: OBJECT CHANGED TYPE: $objfullname previously recorded as type $saved_typeshow,"
		." but is now of type $obj_typename (package ".ref($objref)
		.") - should latter type be used?\n"
		;
    }
  }
  my $expected_typeshow = stash_type_name($dict, $expected_type);
  my $type = $expected_type;
  my $typename = ($expected_typeshow =~ /^\(/) ? undef : $expected_typeshow;	
  if (defined($expected_type)) {
    if (defined($saved_type)) {
      if ($saved_type ne $expected_type) {
	if (dict_type_isa($dict, $saved_type, $expected_type)) {
#print STDERR "#### $objfullname: saved_type ".stash_type_name($dict, $saved_type)
	  $type = $saved_type;
	  $typename = $saved_typename;
	  $type_known_to_reader = 0;	
	} else {
	  if ($objinfo->{typefinal}) {
	    die "stash_classify: $objfullname is type $saved_typeshow"
		  ." which is not a subclass of expected type $expected_typeshow";
	  }
	  if (dict_type_isa($dict, $expected_type, $saved_type)) {
	    die "stash_classify: $objfullname was first encountered with expected type $saved_typeshow"
		." and now with a more specific expected type $expected_typeshow;"
		." partial type specification on first occurrence are not supported";
	  } else {
	    die "stash_classify: $objfullname was earlier expected type $saved_typeshow"
		  ." which is unrelated to expected type $expected_typeshow";
	  }
	}
      }  
    } else {
print STDERR "#### $objfullname: set to expected type $expected_typeshow\n" if $stash_debug;
      $objinfo->{type} = $saved_type = $expected_type;
      $objinfo->{typefinal} = 0;		
      $saved_typename = $typename;
      $saved_typeshow = $expected_typeshow;
    }
  } else {
#print STDERR "#### $objfullname: saved_type $saved_typeshow\n" if $stash_debug;
    $type = $saved_type;
    $typename = $saved_typename;
  }
  die "should not be getting blank ref for types ($objfullname), objref=$objref, dict ".$dict->{name}
    if !defined($type) and $objfullname =~ /^types/;
  return ("skip") if defined($typename) and exists($dict->{skiptype}{$typename});
  if ($objinfo->{basetype} eq "hash") {
    my $href = UNIVERSAL::can($objref, "hashref") ? $objref->hashref : $objref;
    if (defined($type)) {
      die "stash_classify: $objfullname is a hash or object but type is not a hash (is a $typename, basetype ".$type->{basetype}.", object type $saved_typeshow, expected type $expected_typeshow)"
	if $type->{basetype} ne "hash";
    }
    my @skeys = sortkeys($href, ((defined($type) and defined($type->{flatorder})) ? $type->{flatorder} : []),
			$objinfo->{order});
    my @keys = grep(!exists($dict->{skipname}{$_}), @skeys);
    if (@keys != scalar(keys %$href)) {
      die "$objfullname: dropped keys? href={".join(" ",map(key_encode($_),sort keys %$href))."}"
      		."  skeys={".join(" ",map(key_encode($_),sort @skeys))."}"
		."  keys={".join(" ",map(key_encode($_),sort @keys))."}";
    }
    if (stash_type_name($dict, $type) eq "typedef") {
      if ($fulltypes) {
	@keys = grep(!/^inperl$/, @keys);
      } else {
	@keys = grep(!/^inperl|flat|flatorder|aflat|name|named|objname|parents|basetype$/, @keys);
      }
    }
    my ($v, $od);
    my @fkeys = @keys;
    $allextern = 1 if $objfullname ne "types";	
    if (!$allextern) {
      @fkeys = grep(!( ($v = $href->{$_})
		       and ($od = $dict->{objs}{$href->{$_}})
		       and $od->{dict} ne $dict ),
		    @keys);
      print STDERR "DROPPING: $objfullname: {".join(" ",map(key_encode("$_").((($v = $href->{$_}) and ($od = $dict->{objs}{$v})) ? "='".$od->{dict}{name}."'<$v>" : "") ,@keys))."} => {".join(" ",map(key_encode($_),@fkeys))."}\n"
	  if @fkeys != @keys and $stash_debug;
    }
    return ("hash", $href, \@fkeys, $type, $typename, $type_known_to_reader);
  }
  if (ref($objref) eq "ARRAY" or UNIVERSAL::isa($objref, "ArrayObject")) {
    die "stash_classify: $objfullname is an array but type is not"
	if defined($type) and $type->{basetype} ne "array";
    return ("array", $objref, undef, $type, $typename, $type_known_to_reader);
  }
  if (ref($objref) eq "CODE") {
    #print STDERR "### Don't know how to dump code.\n" if $stash_debug;
    return ("code");
  }
  print STDERR "### Don't know how to classify a ".ref($objref)."\n" if $stash_debug;
  return ("other", ref($objref));
}
sub stash_type_name {
  my ($dict, $type) = @_;
  return "(untyped)" unless defined($type);
  my $typename = $type->{name};
  return $typename if defined($typename);
  my $objinfo = $dict->{objs}{$type};
  return "(unknown type)" unless defined($objinfo);
  $typename = $objinfo->{bestfull};
  return "(unnamed type object)" unless defined($typename);
  $typename =~ s/^types\.//;
  $typename =~ s/\.entries\./\./g;
  return $typename;
}
sub stash_catalog_object {
  my ($dict, $objref, $objfullname, $countit, $type, $typefinal) = @_;
  return undef if ref($objref) eq "" or ref($objref) eq "SCALAR";
  if (exists($dict->{objs}{$objref})) {
    my $objinfo = $dict->{objs}{$objref};
    if (defined($type)) {
      my $dtype = $objinfo->{type};
      if (!defined($dtype)) {
	$objinfo->{type} = $type;
	$objinfo->{typefinal} = $typefinal;
      } elsif ($dtype ne $type) {
	die "countrefs: $objfullname: given type ".$type->{name}." (final=$typefinal) not matching existing objinfo type ".$dtype->{name}." (final=".$objinfo->{typefinal}.")";
      }
    }
    if (stash_cmpdepth($objfullname, $objinfo->{bestfull}) < 0) {
      $objinfo->{bestfull} = $objfullname;
      if (ref($objref) eq "ARRAY" or UNIVERSAL::isa($objref,"ArrayObject")) {
	foreach my $i (0 .. $#$objref) {
	  stash_catalog_object($dict, $objref->[$i], $objfullname . ".$i", 0);
	}
      } elsif ($objinfo->{basetype} eq "hash") {
	my $hashref = UNIVERSAL::can($objref, "hashref") ? $objref->hashref : $objref;
	foreach my $key (sort keys %$hashref) {
	  stash_catalog_object($dict, $hashref->{$key}, $objfullname . ".$key", 0);
	}
      }
    }
    return $objinfo;
  }
$dict->{name} or die "oops 5";
  my $objinfo = { full=>$objfullname, obj=>$objref, basetype=>"???", order=>[],
		  bestfull=>$objfullname, done=>0, dict=>$dict,
		  type=>$type, typefinal=>$typefinal };
  $dict->{objs}{$objref} = $objinfo;
  my $isobj = UNIVERSAL::isa($objref, "ObjectClass");
  if ($isobj or UNIVERSAL::isa($objref, "HASH")) {
    $objinfo->{basetype} = "hash";
    $objinfo->{objmodel} = $isobj ? $HMODEL_OBJECT
				  : ref($objref) eq "HASH" ? $HMODEL_HASH
							   : $HMODEL_HASHOBJ;
    my $hashref = UNIVERSAL::can($objref, "hashref") ? $objref->hashref : $objref;
    foreach my $key (sort keys %$hashref) {
      next if exists($dict->{skipname}{$key});
      stash_catalog_object($dict, $hashref->{$key}, $objfullname . ".$key", 1);
    }
  } elsif (UNIVERSAL::isa($objref, "ARRAY")) {
    $objinfo->{basetype} = "array";
    $objinfo->{objmodel} = (ref($objref) eq "ARRAY" ? $AMODEL_ARRAY
						    : $AMODEL_ARRAYOBJ);
    foreach my $i (0 .. $#$objref) {
      stash_catalog_object($dict, $objref->[$i], $objfullname . ".$i", 1);
    }
  } else {
    print STDERR "### Don't know how to catalog a ".ref($objref)." for object $objfullname -- skipped\n" if $stash_debug;
    #print STDERR "### isa ", join(" , ", @{ref($objref)."::ISA"}), ".\n" if $stash_debug;
    print STDERR "### isa ARRAY: ", UNIVERSAL::isa($objref, "ARRAY") ? "yes":"no", "\n" if $stash_debug;
    print STDERR "### isa UNIVERSAL: ", UNIVERSAL::isa($objref, "UNIVERSAL") ? "yes":"no", "\n" if $stash_debug;
    return undef;
  }
  return $objinfo;
}
sub stash_cmpdepth {
  my ($a, $b) = @_;
  return (($a =~ tr/././) <=> ($b =~ tr/././)
	  or length($a) <=> length($b));
}
#  return undef unless $$ref =~ s/^([^\x00-\x20\'\"\#\]\x7f-\xff]+)//;
sub parse_typed_value
{
  my ($dict, $type, $value, $maybe_array, $obj, $what) = @_;
  return (undef, undef) if $value eq "*";	
  $value = xml_decode_string($value);
  if (!defined($type)) {	
    return ($value, undef);
  }
  my $basetype = $type->{basetype};
  defined($basetype) or die "$what: parse_typed_value of '$value' ($maybe_array), type is ".dumpit($type);
  return ($value, undef) if $basetype eq "string";
  return (undef, "cannot parse a hash from a string") if $basetype eq "hash";
  if ($basetype eq "array") {
    return (undef, "unexpected array type, parsing string") unless $maybe_array;
    @$obj = ();
    my $i = 0;
    foreach my $v (split(",", $value)) {
      my $subtype = stash_array_subtype($dict, $type, $i, $what);
      my ($subobj, $err) = parse_typed_value($dict, $subtype, $v, 0, undef, $what);
      return ($obj, "parsing index $i: $err") if defined($err);
      push @$obj, $subobj;
      $i++;
    }
    return ($obj, undef);
  }
  $value =~ s/^\s+//;	
  $value =~ s/\s+$//;
  if ($basetype eq "float") {
    return (undef, "improperly formatted float")
      unless $value =~ /^[-+]?([0-9]+(\.[0-9]*)?|\.[0-9]+)(e[-+]?[0-9]+)?$/i;
    $value =~ s/\+//g;
    return ($value, undef);
  }
  return (undef, "improperly formatted integer")
    unless $value =~ /^(0x[0-9a-f]+|0[0-7]*|\-?[1-9][0-9]*)$/i;
  if ($basetype eq "i64" or $basetype eq "u64") {
    return ($value, undef);
  }
  my $newvalue = ($value =~ /^0/ ? oct($value) : 0+$value);	
  return ($newvalue, undef);
}
sub check_type_override {
  my ($dict, $expected_type, $override_type, $override_named, $objname, $what) = @_;
  my $verbose = 1;			
  my $type = $expected_type;
  my $typefinal = 0;
  my $override_typename;
  $override_type = "typedef_hash"
      if !defined($override_type) and defined($objname) and $objname eq "types";
  if (defined($override_type)) {	
    $typefinal = 1;			
    if ($override_named) {
      $override_typename = $override_type;
      $override_type = dict_lookup_type($dict, $override_typename);
      defined($override_type) or die "$what: unknown type '$override_typename'";
    } else {
      $override_typename = (defined($override_type->{name}) ? $override_type->{name} : "(anonymous)");
    }
    if (defined($expected_type)) {	
      if ($expected_type eq $override_type) {
      } elsif (dict_type_isa($dict, $override_type, $expected_type)) {
	print STDERR "NOTE: $what: subclassing type override :$override_typename:\n" if $verbose >= 2;
      } else {
	die "ERROR: $what: mismatching type override '$override_typename', context implies "
		.(defined($expected_type->{name}) ? $expected_type->{name} : "(anonymous type)")
		." (".join(",",@{$expected_type->{type}}).")";
      }
    }
    $type = $override_type;
  }
  return ($type, $typefinal);
}
my $doneseq = 1;
sub key_encode {
    my ($key) = @_;
    $key =~ s/([^a-zA-Z_0-9:])/sprintf("%%%02X",ord($1))/eg;
    $key;
}
sub key_decode {
    my ($coded) = @_;
    $coded =~ s/%([0-9A-Fa-f][0-9A-Fa-f])/chr(oct("0x$1"))/eg;
    $coded;
}
my $BIN_NIL    = pack("VV", 0, 1);
my $BIN_ABSENT = pack("VV", 0, 2);
my $binread_debug = 0;
my $binwrite_debug = 0;
sub stash_bin_write {
  my ($dict, $topname, $headcomment) = @_;
  dict_expand_all_types($dict);
  my %obj_offsets = ();			
  my %a_offsets = ();			
  my $out = "Stash".pack("CCC", 10, 0, 4)	
	  . pack("VV", 0, 0);			
  $dict->{doneseq} = $doneseq++;	
  $dict->{obj_ofs} = \%obj_offsets;
  $dict->{a_ofs}   = \%a_offsets;
  $dict->{outref}  = \$out;
  stash_bin_write_array($dict, undef, 0, 0,
			[],
			["", ".name", ".description", ".resv0", ".resv1"],
			[$dict->{roots}, $topname, $headcomment, undef, undef]);
  delete $dict->{obj_ofs};
  delete $dict->{a_ofs};
  delete $dict->{outref};
  return $out;
}
sub stash_bin_write_nodeptr {
  my ($outref, $offset, $nodeptr) = @_;
  my ($word_offset, $nextptr) = unpack("VV", $nodeptr);
  if (($nextptr & 0x80000000) != 0) {			
    my $node_ofs = ($word_offset & 0x7FFFFFFF)*8 + 8;	
die "offset $node_ofs too far for output length ".length($$outref) if $node_ofs+4 > length($$outref);
    my $reflist = unpack("V",substr($$outref, $node_ofs, 4));	
    $nodeptr = pack("VV", $word_offset, (0x80000000 | $reflist));	
    my $node_private = ($reflist & 0x80000000);
    substr($$outref, $node_ofs, 4) = pack("V", ($node_private | ($offset >> 3)));
  }
  substr($$outref, $offset, 8) = $nodeptr;
}
sub stash_bin_write_array {
  my ($dict, $name, $priv, $typeofs, $type, $vnames, $values) = @_;
  my $offset = length(${$dict->{outref}});
  my $array_offset = ($offset >> 3);
  ${$dict->{outref}} .= pack("VVVV", (0xA0000000 | @$values), $typeofs, $priv, 0)
	      . ($BIN_ABSENT x @$values);
  $offset += 16;
  foreach my $i (0 .. $#$values) {
    my $e = $values->[$i];
    my $subtype = (ref($type) eq "ARRAY" ? $type->[$i]
		      : stash_array_subtype($dict, $type, $i, $name));
    my $subname = ($vnames ? $vnames->[$i]
		   : defined($name) ? ($name ? $name.".$i" : $i)
		   : undef);
    my $nodeptr = stash_bin_write_node($dict, $e, $subname, $subtype);
    stash_bin_write_nodeptr($dict->{outref}, $offset, $nodeptr);
    $offset += 8;
  }
  return $array_offset | $priv;
}
sub stash_bin_write_string_array {
  my ($dict, $name, $values) = @_;
  my $signature = join(" ", @$values);		
  my $array_offset = $dict->{a_ofs}{$signature};
  if (!defined($array_offset)) {
    $array_offset = stash_bin_write_array($dict, $name, 0x80000000, 0, undef, undef, $values);
    $dict->{a_ofs}{$signature} = $array_offset;
  }
  printf STDERR "%s: string array 0x%x (%s)\n", ($name ? $name : "(unnamed)"),
		$array_offset, $signature if $binwrite_debug;
  return pack("VV", $array_offset, 0x80000000);
}
sub stash_bin_write_node {
  my ($dict, $objref, $objfullname, $expected_type) = @_;
  my $verbose = $binwrite_debug;
  my ($kind, $value, $keyorder, $type, $typename, $type_known_to_reader)
	= stash_classify($dict, $objref, $objfullname, $expected_type, $dict->{doneseq}, 1, 1);
  my $showname = defined($objfullname) ? ($objfullname eq "" ? "(root)" : $objfullname) : "(unnamed)";
  if ($verbose) {
    print STDERR "$showname: $kind, type ", stash_type_name($dict, $type), ": ";
    if (!defined($objref)) {
      print STDERR "nil\n";
    } elsif (UNIVERSAL::isa($objref, "ARRAY")) {
      print STDERR "${objref}[", scalar(@$objref), "]\n";
    } elsif (UNIVERSAL::isa($objref, "HASH")) {
      print STDERR "${objref}{", join(" ",sort keys %$objref), "}\n";
    } else {
      print STDERR "'$objref'\n";
    }
  }
  return $BIN_ABSENT if $kind eq "skip";	
  return $BIN_NIL unless defined($value);			
  return pack("VV",$value,0x40000000) if $kind eq "u32";	
  return pack("VV",$value,($value < 0 ? 0x7FFFFFFF : 0x40000000))
					if $kind eq "i32";	
  if ($kind eq "i64" or $kind eq "u64") {
    my $n = $value;
    if ($n =~ s/^0x//i) {
      if (length($n) > 16 or (length($n) == 16 and $n !~ /^[01ef]/i)) {
	die "stash_bin_write_node: $showname: $kind value too big ($value) to fit in 62 bits";
      }
      $n = substr("0000000000000000$n", -16, 16);
      return pack("VV", hex(substr($n,8,8)), (hex(substr($n,0,8)) | 0x40000000));
    } else {
      if (length($n) > 9) {
	die "stash_bin_write_node: $showname: can't yet support largish $kind value ($value)";
      }
      return pack("VV",$value,($value < 0 ? 0x7FFFFFFF : 0x40000000));
    }
  }
  if ($kind eq "string" and length($value) <= 7) {
    return pack("a7C", $value, ($value =~ /\x00/) ? length($value) : 0x00);
  }
  my $word_offset = $dict->{obj_ofs}{$objref};		
  if (defined($word_offset)) {
    return pack("VV", $word_offset, 0x80000000);		
  }
  if ($kind eq "link") {
    die "stash_bin_write_node: stash_classify() reports link for unmatched object '$objref'";
  }
  my $private = 0x80000000;			
  if ($kind eq "array" or $kind eq "hash") {
    $private = 0;				
  } else {
    $type = $expected_type;
    $typename = defined($type) ? $type->{name} : undef;
  }
  if ($kind eq "extern") {			
    $private = 0;
    $type = undef;
    $typename = undef;
  }
  my $type_offset = 0;
  if (defined($type)) {
    print STDERR "$showname: type $typename okay\n" if $verbose;
    my $tobjname = $type->{objname};
    my $tnodeptr = stash_bin_write_node($dict, $type, $tobjname, $dict->{types}{typedef});
    if ($tnodeptr eq $BIN_ABSENT) {
      die "FIXME this type absenteism for obj $showname type $typename";
    }
    my ($ofs,$link) = unpack("VV", $tnodeptr);
    ($link & 0x80000000) != 0 or die "type not a PTR for obj $showname type $typename";
    $type_offset = ($ofs & 0x7FFFFFFF);
  } else {
    print STDERR "$showname: no type\n" if $verbose;
  }
  my $outref = $dict->{outref};
  my $node_offset = length($$outref);		
  ($node_offset & 7) == 0			
    or die "stash_bin_write_node: $showname: unaligned output ($node_offset)";
  $word_offset = ($node_offset >> 3);		
  $word_offset |= $private;
  $dict->{obj_ofs}{$objref} = $word_offset;	
  my $ptr = pack("VV", $word_offset, 0x80000000);		
  if ($kind eq "float") {					
    $$outref .= pack("VVVVd", 0x80005001, $type_offset, 0x80000000, 0, $value);
  }
  elsif ($kind eq "string") {					
    my $len = length($value);			
    $$outref .= pack("VVVV", $len, $type_offset, 0x80000000, 0)
		. $value
		. (chr(0) x (8 - ($len & 7)));	
  }
  elsif ($kind eq "i64" or $kind eq "u64") {
    die "stash_bin_write_node: $showname: really big $kind values (BIGINT) not yet supported";
  }
  elsif ($kind eq "array") {					
    stash_bin_write_array($dict, $objfullname, 0, $type_offset, $type, undef, $value);
  }
  elsif ($kind eq "hash") {					
    $$outref .= pack("VVVV", 0x80001003, $type_offset, 0, 0)
		. $BIN_ABSENT . $BIN_ABSENT . $BIN_ABSENT;
    my $okeys = stash_bin_write_string_array($dict, undef, $keyorder);
    stash_bin_write_nodeptr($outref, $node_offset+16, $okeys);
    my $skeys = stash_bin_write_string_array($dict, undef, [sort @$keyorder]);
    stash_bin_write_nodeptr($outref, $node_offset+24, $skeys);
    my $offset = length($$outref);		
    $$outref .= pack("VVVV", (0xA0000000 | @$keyorder), 0, 0, 0)
		. ($BIN_ABSENT x @$keyorder);
    stash_bin_write_nodeptr($outref, $node_offset+32,
			pack("VV", ($offset >> 3), 0x80000000));
    $offset += 16;				
    print STDERR "hash {", join(" ", sort @$keyorder), "}\n" if $verbose;
    foreach my $key (sort @$keyorder) {
      my $e = $value->{$key};
      my $subtype = stash_hash_subtype($dict, $type, $key, $objfullname);
      my $coded_key = key_encode($key);
      my $subname = defined($objfullname) ? ($objfullname eq "" ? $coded_key : "${objfullname}.$coded_key") : undef;
      my $nodeptr = stash_bin_write_node($dict, $e, $subname, $subtype);
      stash_bin_write_nodeptr($outref, $offset, $nodeptr);
      $offset += 8;
    }
  }
  elsif ($kind eq "extern") {					
    $$outref .= pack("VVVV", 0x80002002, 0, 0, 0) . $BIN_ABSENT . $BIN_ABSENT;
    defined($value->{full}) or die "writing $showname: referring to external object without an objinfo name";
    my $nodeptr = stash_bin_write_node($dict, $value->{full}, undef);
    stash_bin_write_nodeptr($outref, $node_offset + 16, $nodeptr);
    $nodeptr = stash_bin_write_node($dict, $value->{dict}{name}, undef);
    stash_bin_write_nodeptr($outref, $node_offset + 24, $nodeptr);
  }
  else {
    die "stash_bin_write_node: $showname: unexpected kind '$kind' from stash_classify";
  }
  return $ptr;
}
sub stash_bin_dump_file {
  my ($FH, $outref) = @_;
  my $offset = 16;
  while (length($$outref) >= $offset+16) {
    my $len = stash_bin_dump_node($FH, $outref, $offset);
    $offset += $len;
  }
}
sub stash_bin_dump_node {
  my ($FH, $outref, $offset) = @_;
  my ($len, $n, $nkind);
  my ($a,$e_type,$reflist,$res) = unpack("x$offset VVVV", $$outref);
  my $typeofs = ($e_type & 0x7FFFFFFF) * 8;
  my $refofs = ($reflist & 0x7FFFFFFF) * 8;
  printf $FH "%6X: %stype=%-8s refs=%s%-8s ", $offset,
		(($e_type & 0x80000000) ? "*" : " "),
		($typeofs ? sprintf("[%X]",$typeofs) : "nil"),
		(($reflist & 0x80000000) ? "p" : "="),
		($refofs ? sprintf("[%X]",$refofs) : "nil");
  $offset += 16;
  if (($a & 0x80000000) == 0) {
    $n = $a;
    $len = (($a + 8) & ~7);
    $nkind = 0;
  }
  elsif (($a & 0xF0000000) == 0x80000000) {
    $n = ($a & 0xFFF);
    $len = $n * 8;
    $nkind = ($a & 0xFFFFF000);
  } else {
    $n = ($a & 0xFFFFFFF);
    $len = $n * 8;
    $nkind = ($a & 0xF0000000);
  }
  if ($nkind == 0) {				
    print $FH "STRING '".unpack("x$offset a$a", $$outref)."'";
  }
  elsif ($nkind == 0x80001000) {		
    print $FH "HASH";
    print $FH " !!$n entries instead of 3!!" if $n != 3;
    print $FH " okeys="; stash_bin_dump_nodeptr($FH, $outref, $offset);
    print $FH " skeys="; stash_bin_dump_nodeptr($FH, $outref, $offset+8);
    print $FH " values="; stash_bin_dump_nodeptr($FH, $outref, $offset+16);
    print $FH "\n";
  }
  elsif ($nkind == 0x80002000) {		
    print $FH "LINK ";
    print $FH "!!$n entries instead of 2!! " if $n != 2;
    print $FH "name=";
    stash_bin_dump_nodeptr($FH, $outref, $offset);
    print $FH " dict=";
    stash_bin_dump_nodeptr($FH, $outref, $offset+8);
  }
  elsif ($nkind == 0x80005000) {		
    print $FH "FLOAT ";
    print $FH "!!$n entries instead of 1!! " if $n != 1;
    print $FH "%f", unpack("x$offset d", $$outref);
  }
  elsif ($nkind == 0xA0000000) {		
    print $FH "ARRAY $n (";
    for (my $i = 0; $i < $n; $i++) {
      print $FH ", " if $i;
      stash_bin_dump_nodeptr($FH, $outref, $offset);
      $offset += 8;
    }
    print $FH ")";
  }
  elsif ($nkind == 0xB0000000) {		
    print $FH "BIGINT($n) 0x";
    for (my $i = 0; $i < $n; $i++) {
      print $FH "_" if $i;
      my $ofs = $offset + ($n - 1 - $i) * 8;
      my ($lo,$hi) = unpack("x$ofs VV", $$outref);
      printf $FH "%08X_%08X", $hi, $lo;
    }
  }
  elsif ($nkind == 0xE0000000) {		
    print $FH "EMPTY($n)";
  }
  else {
    printf $FH "UNKNOWN 0x%08X", $nkind;
  }
  print $FH "\n";
  return $len + 16;
}
sub stash_bin_dump_nodeptr {
  my ($FH, $outref, $offset) = @_;
  my ($lo, $hi) = unpack("x$offset VV", $$outref);
  if ($hi & 0x80000000) {			
    printf $FH "[%X]", ($lo & 0x7FFFFFFF)*8;
    print $FH "p" if $lo & 0x80000000;
    $hi &= 0x7FFFFFFF;
    printf $FH "..%X", $hi*8 if $hi;
  }
  elsif (($hi & 0xC0000000) == 0x40000000) {	
    $hi &= 0x3FFFFFFF;
    if ($hi == 0x3FFFFFFF) {
      printf $FH "%d", $lo;
    } elsif ($hi == 0) {
      printf $FH "%u", $lo;
    } else {
      printf $FH "0x%08x_%08x", $hi, $lo;
    }
  }
  elsif ($hi == 0 and $lo == 0) {
    print $FH "''";
  }
  elsif ($hi == 1 and $lo == 0) {
    print $FH "nil";
  }
  elsif ($hi == 2 and $lo == 0) {
    print $FH "absent";
  }
  elsif (($hi & 0xFF000000) == 0) {
    print $FH "'", unpack("x$offset Z8", $$outref), "'";
  }
  elsif (($hi & 0xFF000000) < 0x08000000) {
    my @b = unpack("x$offset C8", $$outref);
    print $FH "BLOB 0x";
    foreach my $i (0 .. $b[7]-1) {
      printf $FH "%02x", $b[$i];
    }
  }
  else {
    printf $FH "UNKNOWN 0x%08x_%08x", $hi, $lo;
  }
}
sub stash_bin_read {
  my ($dict, $bstash, $whatfile, $stashname, $ignore_known_types) = @_;
  $dict->{ignore_known_types} = $ignore_known_types;
  $dict->{stash} = \$bstash;
  $dict->{stashfile} = $whatfile;
  if ($binread_debug) {
    my $dumpname = $whatfile; $dumpname =~ s|.*[\\/]||; $dumpname .= ".dump";
    open(DMP,">$dumpname") or die "could not open '$dumpname' for writing dump: $!";
    $| = 1;
    print STDERR "\n\n############  DUMPING $whatfile (length ".length($bstash).") to '$dumpname'\n\n";
    print DMP "\n\n############  DUMP of $whatfile (length ".length($bstash).")\n\n";
    stash_bin_dump_file(*DMP, \$bstash);
    print DMP "\n";
    close DMP;
  }
  print STDERR "stash_bin_read: unstashing $whatfile (", length($bstash), " bytes).\n";
  if (length($bstash) <= 32
      or substr($bstash, 0, 5) ne "Stash"
      or unpack("xxxxV", $bstash) != 0x04000a68) {
    die "stash_bin_read: not a binary stash format file, incorrect magic number";
  }
  print STDERR "stash read: unstashing <$stashname> into dict '", $dict->{name}, "' from $whatfile\n";
  my ($obj,$error) = stash_bin_read_nodeptr($dict, 32, "", undef, \$dict->{roots}, 0, "");
  die "ERROR: stash_bin_read: $error (from file $whatfile)" if defined($error);
  my $info;
  ($info,$error) = stash_bin_read_node($dict, 16, undef, undef, \$dict->{info}, 0);
  die "ERROR: stash_bin_read: $error (from file $whatfile)" if defined($error);
  print STDERR "====> stash read obj ref=", ref($obj), " keys=",
		join(", ", sort keys %$obj), ".\n" if $stash_debug;
  my @result = map($obj->{$_}, grep($_ ne "types" && $_ ne "modulemap", sort keys %$obj));
  print STDERR "====> stash read ", dumpit(\@result), "\n" if $stash_debug;
  return @result;
}
sub stash_bin_read_nodeptr {
  my ($dict, $offset, $name, $type, $oref, $intype, $what) = @_;
  my $verbose = $binread_debug;
  my $bstash = $dict->{stash};
  my ($lo, $hi) = unpack("x$offset VV", $$bstash);
  my $obj;
  my $showname = (defined($name) ? ($name eq "" ? "(root)" : $name) : "(unnamed)");
  printf STDERR "%6X: hi=0x%08x lo=0x%08x ($what name=$showname) ", $offset, $hi, $lo if $verbose;
  if ($hi & 0x80000000) {			
    my $ofs = ($lo & 0x7FFFFFFF)*8;
    printf STDERR "PTR [%x]\n", $ofs if $verbose;
    return stash_bin_read_node($dict, $ofs, $name, $type, $oref, $intype);
  }
  elsif (($hi & 0xC0000000) == 0x40000000) {	
    $hi &= 0x3FFFFFFF;
    if ($hi == 0x3FFFFFFF or $hi == 0) {
      printf STDERR "%d\n", $lo if $verbose;
      $obj = $lo;
    } else {
      print STDERR "big int\n" if $verbose;
      die "$what: not handling big integers ($hi,$lo)";
    }
  }
  elsif ($hi == 0 and $lo == 0) {		
    print STDERR "''\n" if $verbose;
    $obj = "";
  }
  elsif ($hi == 1 and $lo == 0) {		
    print STDERR "nil\n" if $verbose;
    $obj = undef;
  }
  elsif ($hi == 2 and $lo == 0) {		
    print STDERR "absent\n" if $verbose;
    die "$what: how to return ABSENT?";
    $obj = undef;
  }
  elsif (($hi & 0xFF000000) == 0) {		
    my $str = unpack("x$offset Z8", $$bstash);
    print STDERR "'$str'\n" if $verbose;
    $obj = $str;
  }
  elsif (($hi & 0xFF000000) < 0x08000000) {	
    print STDERR "BLOB\n" if $verbose;
    $obj = substr($$bstash, $offset, ($hi >> 24));
  }
  else {
    print STDERR "??\n" if $verbose;
    die "$what: stash_bin_read_nodeptr: UNKNOWN ".sprintf("0x%08x_%08x", $hi, $lo);
  }
  $$oref = $obj;
  return ($obj, undef);
}
sub stash_bin_read_node {
  my ($dict, $offset, $name, $type, $oref, $intype) = @_;
  my $bstash = $dict->{stash};
  my $whatfile = $dict->{stashfile};
  my $verbose = $binread_debug;
  my $foo;
  defined($oref) or $oref = \$foo;		
  my $rref = \$dict->{readobjs}{$offset};
  my $obj = $$rref;
  if (defined($obj)) {
    printf STDERR "%6X: (already read) '%s'\n", $offset, $obj if $verbose;
    $$oref = $obj;
    return ($obj, undef)
  }
  my $showname = (defined($name) ? ($name eq "" ? "(root)" : $name) : "(unnamed)");
  my $what;
  $what .= sprintf("%s ofs %08x", $showname, $offset);
  my $ofs = $offset + 16;
  if ($ofs > length($$bstash)) {
    return (undef, "$what: offset to node ending past end of file (".sprintf("%08x",length($$bstash)).")");
  }
  my ($len, $n, $nkind);
  my ($kindword,$e_type,$reflist,$res) = unpack("x$offset VVVV", $$bstash);
  my $typeofs = ($e_type & 0x7FFFFFFF) * 8;
  my $refofs = ($reflist & 0x7FFFFFFF) * 8;
  if (($kindword & 0x80000000) == 0) {
    $n = $kindword;
    $len = (($kindword + 8) & ~7);
    $nkind = 0;
  }
  elsif (($kindword & 0xF0000000) == 0x80000000) {
    $n = ($kindword & 0xFFF);
    $len = $n * 8;
    $nkind = ($kindword & 0xFFFFF000);
  } else {
    $n = ($kindword & 0xFFFFFFF);
    $len = $n * 8;
    $nkind = ($kindword & 0xF0000000);
  }
  $what .= sprintf(" (%08x n=%d)", $nkind, $n);
  if ($ofs + $len > length($$bstash)) {
    return (undef, "$what: node at ends at (".sprintf("%08x",$ofs+$len).") past end of file (".sprintf("%08x",length($$bstash)).")");
  }
  printf STDERR "%6X: %stype=%-8s refs=%s%-8s ($showname) ",
		$offset,
		(($e_type & 0x80000000) ? "*" : " "),
		($typeofs ? sprintf("[%X]",$typeofs) : "nil"),
		(($reflist & 0x80000000) ? "p" : "="),
		($refofs ? sprintf("[%X]",$refofs) : "nil")
		if $verbose;
  if ($nkind == 0x80001000) {			
    print STDERR "hash\n" if $verbose;
  } elsif ($nkind == 0xA0000000) {		
    print STDERR "array of $n\n" if $verbose;
  } elsif ($nkind == 0x80002000) {		
    print STDERR "extern link\n" if $verbose;
  } else {
    my ($basetype);
    if ($nkind == 0) {				
      $obj = unpack("x$ofs a$kindword", $$bstash);
      $basetype = "string";
      print STDERR "'$obj'\n" if $verbose;
    } elsif ($nkind == 0x80005000) {		
      $n == 1 or return (undef, "$what: invalid float, node must have 1 element");
      $obj = unpack("x$ofs d", $$bstash);
      $basetype = "float";
      printf STDERR "%f\n", $obj if $verbose;
    } elsif ($nkind == 0xB0000000) {		
      print STDERR "BIGINT\n" if $verbose;
      die "$what: handle BIGINT !?";
    } else {
      printf STDERR "?0x%08x?\n", $nkind if $verbose;
      print STDERR "### Error: $what: unrecognized node kind-word ".sprintf("%08x",$kindword)."\n";
      exit 1;
    }
    if (defined($type) and $type->{basetype} ne $basetype) {
      die sprintf("ERROR: %s: got %s but expected %s (type %s)",
	    $what, $basetype, $type->{basetype}, stash_type_name($dict,$type))
	  .($typeofs ? sprintf(" (typeofs 0x%x ignored)", $typeofs) : "");
    }
    $$oref = $$rref = $obj;
    return ($obj, undef);
  }
  if ($intype >= 2 and $nkind != 0x80002000) {
    die "$what: unexpected intype for a type";
  }
  my $typefinal = 0;
  if ($typeofs != 0) {
    my ($typenode, $err) = stash_bin_read_node($dict, $typeofs, undef, undef, undef, $intype+1);
    return (undef, 0, $err) if $err;
    ($type, $typefinal) = check_type_override($dict, $type, $typenode, 0, $name, $what);
    printf STDERR "%6X:  type at [%X] is %s\n",
	$offset, $typeofs, stash_type_name($dict,$type) if $verbose;
  }
  my ($objinfo, $objmodel, $error);
  if ($nkind == 0x80001000) {		
    $what .= " hash";
    $n == 3 or return (undef, "$what: invalid hash, node must have 3 elements");
    if (defined($obj = $$oref)) {
      $showname eq "(root)" or $showname eq "types" or die "can't re-enter hash $showname";
      $objinfo = $dict->{objs}{$obj};
      if (defined($type)) {
	$objinfo->{type} = $type;
	$objinfo->{typefinal} ||= $typefinal;
      }
      $objmodel = $HMODEL_HASH;
    } else {
      ($obj, $objmodel) = stash_create_hash_object($dict, $type, $showname);
      $objinfo = { full=>$name, bestfull=>$name, obj=>$obj, basetype=>"hash",
      		order=>undef,		
		done=>0, dict=>$dict, type=>$type, typefinal=>$typefinal,
		objmodel=>$objmodel };
      $dict->{objs}{$obj} = $objinfo;
      $$oref = $obj;
    }
    $$rref = $obj;	
    die "ERROR: $what: type is not a hash: ".stash_type_name($dict,$type)." basetype=".$type->{basetype}." dump: ".dumpit($type) if defined($type) and $type->{basetype} ne "hash";
    my ($okeys, $skeys, $values);
    ($okeys, $error)  = stash_bin_read_nodeptr($dict, $ofs, undef, undef, undef, $intype, $what." okeys");
    return (undef,$error) if $error;
    ($skeys, $error)  = stash_bin_read_nodeptr($dict, $ofs+8, undef, undef, undef, $intype, $what." skeys");
    return (undef,$error) if $error;
    if (ref($okeys) ne "ARRAY" or ref($skeys) ne "ARRAY") {
      return (undef, "$what: hash okeys and/or skeys not an array");
    }
    @$okeys == @$skeys or return (undef, "$what: hash okeys & skeys not same size");
    $objinfo->{order} = [@$okeys] unless defined($objinfo->{order});
    my ($lo, $hi) = unpack("x".($ofs+16)." VV", $$bstash);
    ($hi & 0x80000000) != 0 or die "$what: values array nodeptr must be PTR";
    my $values_ofs = ($lo & 0x7FFFFFFF) * 8;		
    my ($akind) = unpack("x$values_ofs V", $$bstash);
    ($akind & 0xF0000000) == 0xA0000000 or die "$what: values array not an array";
    $akind &= 0xFFFFFFF;				
    $akind == @$skeys or return (undef, "$what: hash skeys & values not same size");
    printf STDERR "%6X: hash values array of %d\n", $values_ofs, $akind if $verbose;
    $values_ofs += 16;
    $intype = 1 if $intype == 0 and $name eq "types";
    foreach my $i (0 .. $#$skeys) {
      my $key = $skeys->[$i];
      my $coded_key = key_encode($key);
	$key eq "*" or die "ERROR: $what: invalid subnode name '$coded_key' (from skeys[$i] at ".sprintf("0x%X",$ofs+8).")";
      my $subexists = 0;
      my $subtype = stash_hash_subtype($dict, $type, $key, $what, \$subexists);
      ($objmodel == $HMODEL_OBJECT) and !$subexists and die "ERROR: $what: unexpected key '$coded_key' for fixed class";
      my $subref = (($objmodel == $HMODEL_OBJECT) ? $obj->getref($key) : \$obj->{$key});
      my $subnode;
      my $subname = defined($name) ? ($name eq "" ? $coded_key : "${name}.$coded_key") : undef;
      ($subnode, $error) = stash_bin_read_nodeptr($dict, $values_ofs + ($i * 8),
				$subname, $subtype, $subref, $intype, $what." key $coded_key");
      return (undef, $error) if defined($error);
    }
  } elsif ($nkind == 0xA0000000) {		
    ($obj, $objmodel) = stash_create_array_object($dict, $type, $showname);
    $objinfo = { full=>$name, bestfull=>$name, obj=>$obj, basetype=>"array", order=>[],
	      done=>0, dict=>$dict, type=>$type, typefinal=>$typefinal,
	      objmodel=>$objmodel };
    $dict->{objs}{$obj} = $objinfo;
    $$oref = $$rref = $obj;
    for (my $i = 0; $i < $n; $i++) {
      my $subtype = stash_array_subtype($dict, $type, $i, $what);
      my $subname = defined($name) ? ($name eq "" ? $i : "${name}.$i") : undef;
      my ($subobj, $suberr) = stash_bin_read_nodeptr($dict, $ofs, $subname,
				$subtype, \$obj->[$i], $intype, $what." array elem $i");
      return (undef, $suberr) if defined($suberr);
      $ofs += 8;
    }
  } elsif ($nkind == 0x80002000) {		
    $n == 2 or return (undef, "$what: invalid external link, node must have 2 elements");
    my ($linkname, $dictname);
    ($linkname, $error) = stash_bin_read_nodeptr($dict, $ofs, undef, undef, undef, $intype, "$what linkname");
    return (undef,$error) if $error;
    ($dictname, $error) = stash_bin_read_nodeptr($dict, $ofs+8, undef, undef, undef, $intype, "$what dictname");
    return (undef,$error) if $error;
    ($obj, $error) = stash_lookup($dict, $linkname);
    return (undef, "$what: fw ref or dangling external link?: $error") if defined($error);
    return (undef, "$what: unresolved link to unknown object '$linkname'") unless defined($obj);
    {
      my $objinfo = $dict->{objs}{$obj};
      defined($objinfo) or die "$what: no objinfo for object";
      printf STDERR "===LINK===> %s ===> dict='%s' basetype=%s type=%s objmodel=%d full=%s bestfull=%s\n",
	$linkname, $objinfo->{dict}{name}, $objinfo->{basetype},
	stash_type_name($dict,$objinfo->{type}), $objinfo->{objmodel},
	$objinfo->{full}, $objinfo->{bestfull}
	if $verbose;
    }
    $$oref = $$rref = $obj;
  }
  return ($obj, undef);
}
sub stash_text_write {
  my ($dict, $topname, $headcomment) = @_;
  $dict->{doneseq} = $doneseq++;	
  my $out = "";
  if (defined($headcomment) and $headcomment ne "") {
    $out = $headcomment;
    chomp($out);
    $out .= "\n";
    $out =~ s/^/\#  /mg;
    $out .= "\n";
  }
  foreach my $rootname (stash_order_roots($dict)) {
    my ($elem, $indented) = stash_text_write_node($dict, $dict->{roots}{$rootname}, $rootname, 0, undef);
    $out .= "$rootname = $elem\n\n" unless $elem eq "";
  }
  return $out;
}
sub stash_text_write_node {
  my ($dict, $objref, $objfullname, $indent, $expected_type) = @_;
  my ($kind, $value, $keyorder, $type, $typename, $type_known_to_reader)
	= stash_classify($dict, $objref, $objfullname, $expected_type, $dict->{doneseq});
  return "" if $kind eq "skip"; # or $kind eq "extern";		# object skipped
  my $x = " " x $indent;
  return "nil" unless defined($value);
  return sprintf((($value<10)?"%u":"0x%x"), $value) if $kind eq "u32";
  return sprintf((($value<10)?"%d":"0x%x"), $value) if $kind eq "i32";
  if ($kind eq "float") {
    $value = sprintf("%g", $value);
    $value .= ".0" unless $value =~ /[eE\.]/;
    return $value;
  }
  if ($kind eq "string") {
    my $s = "";
    foreach my $i (0 .. length($value)-1) {
      my $c = substr($value,$i,1);
      if    ($c eq "\n") { $s .= "\\n"; }
      elsif ($c eq "\t") { $s .= "\\t"; }
      elsif ($c eq "\r") { $s .= "\\r"; }
      else {
	my $n = ord($c);
	if ($n >= 32 and $n < 127) {
	  $s .= $c;
	} else {
	  $s .= "\\x".sprintf("%02x", ($n & 0xFF));
	}
      }
    }
    return '"'.$s.'"';
  }
  return '@'.$value->{bestfull} if $kind eq "extern";	
  return '@'.$value if $kind eq "link";
  if ($kind eq "hash") {
    my $href = $value;
    my $indented = 0;
    my $out = "";
    $out .= "$typename:" if defined($typename) and !$type_known_to_reader;
    $out .= "{";
    foreach my $key (@$keyorder) {
      my $e = $href->{$key};
      my $subtype = stash_hash_subtype($dict, $type, $key, $objfullname);
      my $coded_key = key_encode($key);
      my ($elem, $ind) = stash_text_write_node($dict, $e, $objfullname . ".$coded_key", $indent+1, $subtype);
      next unless $elem ne "";		
      if (@$keyorder > 1) {
	$out .= "\n$x";
	$indented = 1;
      }
      $out .= " $coded_key = $elem";
    }
    $out .= ($indented ? "\n$x" : " ");
    $out .= "}";
    return ($out, $indented);
  }
  if ($kind eq "array") {
    my $indented = -1;		
    my $out = "";
    $out .= "$typename:" if defined($typename) and !$type_known_to_reader;
    $out .= "[";
    foreach my $i (0 .. $#$value) {
      my $e = $value->[$i];
      my $subtype = stash_array_subtype($dict, $type, $i, $objfullname);
      my ($elem, $ind) = stash_text_write_node($dict, $e, $objfullname . ".$i", $indent+1, $subtype);
      $elem = "nil" if $elem eq "";	
      my $big = ($ind or length($elem) > 24);
      if ($big or $indented == $i) {
	$out .= "\n$x $elem";
	$indented = $i+1 if $big;
      } else {
	$out .= " " if $i > $indented + 1;
	$out .= $elem;
      }
    }
    $out .= "\n$x" if $indented >= 0;
    $out .= "]";
    return ($out, $indented);
  }
  if ($kind eq "code") {
    #print STDERR "### Don't know how to dump code.\n" if $stash_debug;
    return "?code";
  }
  if ($kind eq "other") {
    print STDERR "### Don't know how to dump kind $kind (a ".ref($objref).")\n" if $stash_debug;
    return "?other?$value";
  }
  if ($kind eq "i64" or $kind eq "u64") {
    return $value;
  }
  die "stash_text_write_node: unexpected kind '$kind' from stash_classify";
}
sub stash_scan_whitespace {
  my ($pstash, $pline) = @_;
  #if ($$pstash =~ s/^((?:[ \t]*(?:\#.*)?\n)*[ \t]*(?:\#.*)?)//)
  $$pstash =~ s/^[ \t]*(?:\#.*)?(\n[ \t]*(?:\#.*)?)*//;
  $$pline += ($1 =~ tr/\n/\n/) if defined($1);
}
my $NOPARSE = "unrecognized input";
sub stash_text_scan_node {
  my ($stash, $line, $whatfile, $indent) = @_;
  my @children = ();
  defined($indent) or $indent = "";
  my ($obj);
  my $node = {line => $$line};
  stash_scan_whitespace($stash, $line);
  $$stash ne "" or return (undef, "unexpected end of input");
  if ($$stash =~ s/^([a-z_]\w*(\.[a-z_]\w*)*)\s*\://i) {
    $node->{type} = $1;
    stash_scan_whitespace($stash, $line);
    $$stash ne "" or return (undef, "unexpected end of input");
  }
  if ($$stash =~ s/^\[//) {
    $node->{kind} = "array";
    $node->{value} = $obj = [];
    stash_scan_whitespace($stash, $line);
    while ($$stash !~ s/^\]//) {
      if ($$stash !~ s/^,+//) {		
	my ($subnode, $error);
	($subnode, $error) = stash_text_scan_node($stash, $line, $whatfile, $indent." ");
	$error and return ($node, $error);
	push @$obj, $subnode;
      }
      stash_scan_whitespace($stash, $line);
    }
  } elsif ($$stash =~ s/^\{//) {
    my $error;
    my $type = $node->{type};
    ($node, $error) = stash_text_scan_hash($stash, $line, $whatfile, $indent);
    $node->{type} = $type if defined($type);
    $error and return ($node, $error);
    $obj = $node->{value};
    $$stash =~ s/^\}//
      or return($node, "expected hash key or '}' for end of hash");
  } elsif ($$stash =~ s/^"//) {
    $node->{kind} = "string";
    $obj = "";
    while ($$stash !~ s/^"//) {
      if ($$stash eq "") {
	return ($node, "unterminated double-quoted string");
      } elsif ($$stash =~ /^\n/) {
	return ($node, "unterminated double-quoted string"
		." (to continue to a new line, use a backslash at the end of the line)");
      } elsif ($$stash =~ s/^\\\n//) {
	$$line++;
      } elsif ($$stash =~ s/^\\n//) {
	$obj .= "\n";
      } elsif ($$stash =~ s/^\\t//) {
	$obj .= "\t";
      } elsif ($$stash =~ s/^\\r//) {
	$obj .= "\r";
      } elsif ($$stash =~ s/^\\x([0-9a-f][0-9a-f])//i) {
	$obj .= chr(hex($1));
      } elsif ($$stash =~ s/^\\([0-3][0-7][0-7])//) {
	$obj .= chr(oct("0".$1));
      } elsif ($$stash =~ s/^\\(.)//) {
	$obj .= $1;
      } else {
	$$stash =~ s/^(.)//;
	$obj .= $1;
      }
    }
  } elsif ($$stash =~ s/^\'//) {
    $node->{kind} = "string";
    $obj = "";
    while ($$stash !~ s/^'//) {
      ($$stash ne "" and $$stash !~ /^\n/)
	or return ($obj, "unterminated single-quoted string");
      if ($$stash =~ s/^\\([\\'])//) {
	$obj .= $1;		
      } else {
	$$stash =~ s/^(.)//;
	$obj .= $1;
      }
    }
  } elsif ($$stash =~ s/^nil\b//) {
    $node->{kind} = "nil";
    $obj = undef;
  } elsif ($$stash =~ s/^([-+]?(?:(?:[0-9]+\.[0-9]*|\.[0-9]+)(?:e[-+]?[0-9]+)?|[0-9]+e[-+]?[0-9]+))\b//i) {
    $node->{kind} = "float";
    $obj = $1;
    $obj =~ s/\+//g;		
  } elsif ($$stash =~ s/^([-+]?(?:0x[0-9a-f]+|0[0-7]*|[1-9][0-9]*))\b//i) {	
    $node->{kind} = "int";
    $obj = $1;
    $obj =~ s/^\+//;
    $obj = scalar2int($obj);
  } elsif ($$stash =~ s/^\@//) {
    $node->{kind} = "link";
    $$stash =~ s/^\s*((\d+|[a-z_]\w*)(\.(\d+|[a-z_]\w*))*)\b//i
      or return (undef, "missing or invalid link target");
    $obj = $1;
  } else {
    return (undef, $NOPARSE);	
  }
  $node->{value} = $obj;
  $node->{endline} = $$line;
  return ($node, undef);
}
my $globcount = 0;
sub stash_text_scan_hash {
  my ($stash, $line, $whatfile, $indent) = @_;
  my $obj = {};
  my $node = {kind => "hash", value => $obj, line => $$line};
  my @keys;
  stash_scan_whitespace($stash, $line);
  while (1) {
#    if ($$stash =~ s/^,//) {		# allow commas {or semicolons??} between keys
    $$stash =~ s/^([a-zA-Z_0-9:%]+)//
      or last;		
    my $coded_key = $1;
    my $key = key_decode($coded_key);
    stash_scan_whitespace($stash, $line);
    my ($subnode, $error);
    if ($$stash =~ s/^=//) {
      ($subnode, $error) = stash_text_scan_node($stash, $line, $whatfile, $indent." ");
      $error and return ($node, $error);
      stash_scan_whitespace($stash, $line);
    } else {
      $subnode = {kind => "int", value => 1, line => $$line, endline => $$line};
    }
defined($subnode) and defined($subnode->{kind}) or die "$whatfile line $$line key $coded_key: strange subnode: $subnode .";
print STDERR "." if (++$globcount & 63) == 0 and $stash_debug; # "${indent}line $$line $node value $obj {$key} = $subnode\n";
    exists($obj->{$key})
      and return($node, "duplicate key '$coded_key' in hash");
    $obj->{$key} = $subnode;
    push @keys, $key;	
  }
  $node->{order} = \@keys;
  $node->{endline} = $$line;
  return ($node, undef);
}
sub stash_text_dump_node {
  my ($whatfile, $name, $node, $indent) = @_;
  my $k = $node->{kind};
  my $v = $node->{value};
  my $t = $node->{type};
  printf STDERR "line %5d-%-5d %s (%s) ", $node->{line}, $node->{endline}, $name, $k;
  printf STDERR "type=%s ", $t if defined($t);
  if ($k eq "hash") {
    print STDERR "{\n";
    foreach my $key (sort keys %$v) {
      stash_text_dump_node($whatfile, $name.".".$key, $v->{$key}, $indent." ");
    }
    print STDERR "                 $name }";
  } elsif ($k eq "array") {
    print STDERR "[\n";
    foreach my $i (0 .. $#$v) {
      stash_text_dump_node($whatfile, $name.".".$i, $v->[$i], $indent." ");
    }
    print STDERR "                 $name ]";
  } else {
    print STDERR "$v";
  }
  print STDERR "\n";
}
sub stash_text_read {
  my ($dict, $stash, $whatfile, $stashname, $ignore_known_types) = @_;
  $dict->{ignore_known_types} = $ignore_known_types;
  print STDERR "stash_text_read: parsing $whatfile (", length($stash), " bytes).\n" if $stash_debug;
  my $line = 1;
  my ($topnode, $error) = stash_text_scan_hash(\$stash, \$line, $whatfile, "");
print STDERR "\n" if $stash_debug;
  if (!defined($error) and $stash ne "") {
    $error = "unrecognized input, expected hash key or end-of-file";
  }
  if (defined($error)) {
    print STDERR "### Error parsing Stash file: $error\n";
    print STDERR "### $whatfile line $line (", substr($stash, 0, 60), "...)\n";
    die;
    return undef;
  }
  print STDERR "stash read: unstashing <$stashname> into dict '", $dict->{name}, "' from $whatfile\n" if $stash_debug;
  my $obj;
  ($obj,$error) = stash_text_read_node($dict, $topnode, "", undef, \$dict->{roots}, $whatfile);
  die "ERROR: stash_text_read: $error (from file $whatfile)" if defined($error);
  print STDERR "====> stash read obj ref=", ref($obj), " keys=",
		join(", ", sort keys %$obj), ".\n" if $stash_debug;
  my @result = map($obj->{$_}, grep($_ ne "types" && $_ ne "modulemap", sort keys %$obj));
  print STDERR "====> stash read ", dumpit(\@result), "\n" if $stash_debug;
  return @result;
}
sub stash_text_read_node {
  my ($dict, $node, $name, $type, $oref, $whatfile) = @_;
  my $verbose = 1;			
  my $showname = ($name eq "" ? "(root)" : $name);
  my $kind = $node->{kind};
  my $line = $node->{line};
  my $nodevalue = $node->{value};
  my $what = $whatfile;
  $what .= ": $showname ($kind)";
  my ($obj, $error) = stash_lookup($dict, $name);
  return (undef, "$what: $error") if defined($error);
  if (defined($obj)) {
    my $testobjinfo = $dict->{objs}{$obj};
    defined($testobjinfo) or die "ERROR: $what: testobjinfo unexpectedly undefined (obj $obj ref ".ref($obj).")";
  }
  my $typefinal;
  my $nodetypename = $node->{type};	
  ($type, $typefinal) = check_type_override($dict, $type, $nodetypename, 1, $name, $what);
  if ($kind eq "hash") {
    my ($objinfo, $objmodel);
    if (defined($obj)) {
      $objinfo = $dict->{objs}{$obj};
      defined($objinfo) or die "ERROR: $what: objinfo unexpectedly undefined";
      die "ERROR: $what: cannot re-enter hash of another dictionary"
      	if $obj ne $dict->{roots} and $objinfo->{dict} ne $dict;
      $objinfo->{type} = $type if defined($type);	
      $objinfo->{typefinal} ||= $typefinal;
    } else {
      ($obj, $objmodel) = stash_create_hash_object_maybetypename($dict, $type, $name);
$dict->{name} or die "oops 1";
      $objinfo = { full=>$name, bestfull=>$name, obj=>$obj, basetype=>"hash", order=>[],
		done=>0, dict=>$dict, type=>$type, typefinal=>$typefinal,
		objmodel=>$objmodel };
      $dict->{objs}{$obj} = $objinfo;
    }
    $$oref = $obj;
    my $objpm = UNIVERSAL::isa($obj, "ObjectClass");	
    $objpm or UNIVERSAL::isa($obj, "HASH") or die "ERROR: $what: $name is not a hash";
    die "ERROR: $what: type is not a hash" if defined($type) and $type->{basetype} ne "hash";
    my @order = @{$node->{order}};
    foreach my $key (@order) {
      my $coded_key = key_encode($key);
      my $subnode = $nodevalue->{$key};
      my $swhat = $what;
      $swhat .= "; key $coded_key (".$subnode->{kind}.")";
defined($subnode) and defined($subnode->{kind}) or die "$swhat: strange subnode: $node value $nodevalue {$key} = $subnode keys=".join(",",sort keys %$subnode)." values=".join(",",values %$subnode).".";
      my $subexists = 0;
      my $subtype = stash_hash_subtype($dict, $type, $key, $what, \$subexists);
      $objpm and !$subexists and die "ERROR: $swhat: unexpected key for fixed class";
      my $subname = ($name eq "" ? $key : "${name}.${key}");
      if ($objpm) {
      } elsif (exists($obj->{$key})) {
	if (defined($obj->{$key})) {
	  my $keyis = "and defined";
	  my $subobjinfo = $dict->{objs}{$obj->{$key}};
	  if (defined($subobjinfo)) {
	    my $odict = $subobjinfo->{dict};		# dict of existing $obj->{$key}
	    if ($odict ne $dict) {
	      if ($name eq "types" and $dict->{ignore_known_types}) {
		next;
	      }
	    }
	  } else {
	    $keyis = "untracked";
	  }
	  print STDERR "WARNING: $swhat: subnode $key already exists $keyis (overridden)\n"
		if $verbose and $subname ne "types";
	}
      } else {
	push @{$objinfo->{order}}, $key;
      }
      my $subref = ($objpm ? $obj->getref($key) : \$obj->{$key});
      ($$subref, $error) = stash_text_read_node($dict, $subnode, $subname, $subtype, $subref, $whatfile);
      return (undef, $error) if defined($error);
    }
    return ($obj, undef);
  } elsif ($kind eq "array") {
    my ($objinfo, $objmodel);
    if (defined($obj)) {
      $objinfo = $dict->{objs}{$obj};
      defined($objinfo) or die "ERROR: $what: objinfo unexpectedly undefined";
      die "ERROR: dict ".$dict->{name}.": $what: cannot re-enter array of another dictionary (".$objinfo->{dict}{name}.")"
      	if $objinfo->{dict} ne $dict;
      $objinfo->{type} = $type if defined($type);	
      $objinfo->{typefinal} ||= $typefinal;
    } else {
      ($obj, $objmodel) = stash_create_array_object($dict, $type, $name);
$dict->{name} or die "oops 2";
      $objinfo = { full=>$name, bestfull=>$name, obj=>$obj, basetype=>"array", order=>[],
		done=>0, dict=>$dict, type=>$type, typefinal=>$typefinal,
		objmodel=>$objmodel };
      $dict->{objs}{$obj} = $objinfo;
    }
    $$oref = $obj;
    ref($obj) eq "ARRAY" or UNIVERSAL::isa($obj, "ArrayObject") or die "ERROR: $what: $name is not an array";
    if (!defined($type)) {
      $type = dict_lookup_type($dict, "array");
    }
    die "ERROR: $what: type is not an array" if $type->{basetype} ne "array";
    my $i = 0;			
    foreach my $subnode (@$nodevalue) {
      my $swhat = $what;
      $swhat .= "; index $i (".$subnode->{kind}.")";
      my $subtype = stash_array_subtype($dict, $type, $i, $what);
      if (exists($obj->[$i])) {
	print STDERR "WARNING: $swhat: index already exists (overridden)\n" if $verbose;
      }
      my $iobj;
      ($iobj, $error) = stash_text_read_node($dict, $subnode, "${name}.${i}", $subtype, \$obj->[$i], $whatfile);
      return (undef, $error) if defined($error);
      $i++;
    }
    return ($obj, undef);
  } elsif ($kind eq "link") {
    defined($obj) and die "$what: object of that name already exists";
    defined($nodetypename) and !$dict->{ignore_unsupported_typing} and print STDERR
	"WARNING: $what: type override (TYPE: prefix) not expected or yet supported with link (ignored)\n";
    my $link = $nodevalue;
    defined($link) or return (undef, "$what: missing link");
    if ($link =~ /^\./) {	
      my @prefix = split(".", $name); pop @prefix;
      my $blink = $link;
      while ($blink =~ s/^\.//) {
	@prefix or die "$what: relative link goes back too far ($link)";
	pop @prefix; 
      }
      $link = join(".", (@prefix, $blink));
    }
    ($obj, $error) = stash_lookup($dict, $link);
    return (undef, "$what: fw ref or dangling link?: $error") if defined($error);
    return (undef, "$what: can't link to undef") unless defined($obj);
    my $objinfo = $dict->{objs}{$obj};
    $$oref = $obj;
    return ($obj, undef);
  } elsif ($kind =~ /^int|string|float$/) {
    defined($obj) and print STDERR "INFO: $what: object exists, content overridden\n" if $verbose;
    defined($nodetypename) and !$dict->{ignore_unsupported_typing} and print STDERR
	"WARNING: $what: type override (TYPE: prefix) not expected or yet supported with $kind (ignored)\n";
    my %mapscalars = (i32=>'int', u32=>'int', i64=>'int', u64=>'int',
		      string=>'string', float=>'float');
    if (defined($type) and $mapscalars{$type->{basetype}} ne $kind) {
      die "ERROR: $what: got kind $kind but expected type ".stash_type_name($dict,$type)
		." (basetype ".$type->{basetype}.")"
    }
    $$oref = $nodevalue;
    return ($nodevalue, undef);
  } elsif ($kind eq "nil") {
    defined($obj) and die "$what: object exists, but read as nil";
    defined($nodetypename) and !$dict->{ignore_unsupported_typing} and print STDERR
	"WARNING: $what: type override (TYPE: prefix) not expected or yet supported with nil (ignored)\n";
    $$oref = undef;
    return (undef, undef);
  } else {
    print STDERR "### Error: $what: unrecognized kind '$kind'\n";
    exit 1;
  }
}
sub stash_xml_read {
  my ($dict, $xml, $whatfile, $topname, $ignore_known_types, $ignore_sari_weirdness) = @_;
  my $ndos = ($xml =~ s/\r\n/\n/g);
  print STDERR "### Fixed $ndos DOS lines\n" if ($ndos && $stash_debug);
  return stash_bin_read($dict, $xml, $whatfile, $topname, $ignore_known_types)
	if length($xml) > 32 and substr($xml,0,5) eq "Stash" and unpack("xxxxV",$xml) == 0x04000a68;
  return stash_text_read($dict, $xml, $whatfile, $topname, $ignore_known_types)
	if $force_no_xml and $xml !~ /^\s*\</;
  defined($topname) or $topname = "stash";
  $dict->{ignore_known_types} = $ignore_known_types;
  $dict->{ignore_unsupported_typing} = $ignore_sari_weirdness;	
  print STDERR "stash_xml_read: parsing $whatfile (", length($xml), " bytes).\n" if $stash_debug;
  my @xmlines = split(/\n/, $xml);
  my ($children, $lasttext, $line, $error) = xml_scan2(\@xmlines, "", "", 1);
  my $remain = (@xmlines ? $xmlines[0] : "");
  if (defined($error)) {
    print STDERR "### Error parsing XML: $error\n";
    print STDERR "### $whatfile line $line (", substr($remain, 0, 60), "...)\n";
    die;
    return undef;
  }
  my @stashes = grep($_->{".tag"} !~ /^\?/, @$children);
  if (@stashes != 1) {
    print STDERR "### Error: $whatfile: did not find exactly one top-level element (", scalar(@stashes), " top-level element tags: ", join(", ", map($_->{".tag"}, @stashes)), ")\n";
    die;
    return undef;
  }
  my $stash = $stashes[0];
  my $stashname = $stash->{".tag"};
  if ($stashname ne $topname) {
    print STDERR "WARNING: stash_xml_read: top-level element named <$stashname>, expected <$topname>\n";
  }
  print STDERR "stash read: unstashing <$stashname> into dict '", $dict->{name}, "' from $whatfile\n" if $stash_debug;
  my $obj;
  ($obj,$error) = stash_xml_read_node($dict, $stash, "", undef, \$dict->{roots}, $whatfile);
  die "ERROR: <$stashname>: $error (from file $whatfile)" if defined($error);
  print STDERR "====> stash read obj ref=", ref($obj), " keys=",
		join(", ", sort keys %$obj), ".\n" if $stash_debug;
  my @result = map($obj->{$_}, grep($_ ne "types" && $_ ne "modulemap", sort keys %$obj));
  print STDERR "====> stash read ", dumpit(\@result), "\n" if $stash_debug;
  return @result;
}
sub stash_xml_read_node {
  my ($dict, $node, $name, $type, $oref, $whatfile) = @_;
  my $verbose = 1;			
  my $showname = ($name eq "" ? "(root)" : $name);
  my $tag  = $node->{".tag"};			# node tag (e.g. 'float' in '<float value="1.3"/>')
  my $line = $node->{".line"};			
  my $what = $whatfile;
  $what .= ": $showname ($tag)";
  my @children = @{$node->{".children"}};	
  my $headkeys = $node->{".headkeys"};		
  if ($name eq "" and !defined($node->{n}) and !defined($node->{b})) {
    $tag = "hash";
  }
  my ($obj, $error) = stash_lookup($dict, $name);
  return (undef, "$what: $error") if defined($error);
  if (defined($obj)) {
    my $testobjinfo = $dict->{objs}{$obj};
    defined($testobjinfo) or die "ERROR: $what: testobjinfo unexpectedly undefined (obj $obj ref ".ref($obj).")";
  }
  my $typefinal;
  my $nodetypename = delete $node->{"t"};	
  ($type, $typefinal) = check_type_override($dict, $type, $nodetypename, 1, $name, $what);
  if ($tag eq "hash") {
    my ($objinfo, $objmodel);
    if (defined($obj)) {
      $objinfo = $dict->{objs}{$obj};
      defined($objinfo) or die "ERROR: $what: objinfo unexpectedly undefined";
      die "ERROR: $what: cannot re-enter hash of another dictionary"
      	if $obj ne $dict->{roots} and $objinfo->{dict} ne $dict;
      $objinfo->{type} = $type if defined($type);	
      $objinfo->{typefinal} ||= $typefinal;
    } else {
      ($obj, $objmodel) = stash_create_hash_object_maybetypename($dict, $type, $name);
$dict->{name} or die "oops 3";
      $objinfo = { full=>$name, bestfull=>$name, obj=>$obj, basetype=>"hash", order=>[],
		done=>0, dict=>$dict, type=>$type, typefinal=>$typefinal,
		objmodel=>$objmodel };
      $dict->{objs}{$obj} = $objinfo;
    }
    $$oref = $obj;
    my $objpm = UNIVERSAL::isa($obj, "ObjectClass");	
    $objpm or UNIVERSAL::isa($obj, "HASH") or die "ERROR: $what: $name is not a hash";
    die "ERROR: $what: type is not a hash" if defined($type) and $type->{basetype} ne "hash";
    foreach my $key (sortkeys($node, $headkeys)) {
      next if $key =~ /^\./;	
      my $coded_key = key_encode($key);
      my $value = $node->{$key};
      my $subexists = 0;
      my $subtype = stash_hash_subtype($dict, $type, $key, $what, \$subexists);
      $objpm and !$subexists and die "ERROR: $what: unexpected key '$coded_key' for fixed class";
      my $objvalue = ($objpm ? $obj->getvalue($key) : $obj->{$key});
      my ($subobj, $err) = parse_typed_value($dict, $subtype, $value, 1, $objvalue, "$what: key $coded_key");
      defined($err) and die "ERROR: $what: key $coded_key: $err";
      if ($objpm) {
      } elsif (exists($obj->{$key})) {
	if (defined($obj->{$key})) {
	  print STDERR "WARNING: $what: key $coded_key already exists (overridden)\n" if $verbose;
	}
      } else {
	push @{$objinfo->{order}}, $key;
      }
      if ($objpm) {
	$obj->setvalues($key => $subobj);
      } else {
	$obj->{$key} = $subobj;
      }
    }
    foreach my $subnode (@children) {
      my $swhat = $what;
      my $coded_key = delete $subnode->{n};	
      if (!defined($coded_key)) {		
	my $basetype = delete $subnode->{b};	
	defined($basetype) or die "ERROR: $swhat: subnode missing name, expected <BASETYPE n='NAME'...> or <NAME b='BASETYPE'...>";
	$coded_key = $subnode->{".tag"};	
	$subnode->{".tag"} = $basetype;		
      }
      my $key = key_decode($coded_key);
      $coded_key = key_encode($key);		
      $swhat .= "; key $coded_key (".$subnode->{".tag"}.")";
      defined($subnode->{i}) and die "ERROR: $swhat: named subnode has array index (i)";
      my $subname = ($name eq "" ? $coded_key : "${name}.${coded_key}");
      if ($objpm) {
      } elsif (exists($obj->{$key})) {
	if (defined($obj->{$key})) {
	  my $keyis = "and defined";
	  my $subobjinfo = $dict->{objs}{$obj->{$key}};
	  if (defined($subobjinfo)) {
	    my $odict = $subobjinfo->{dict};		# dict of existing $obj->{$key}
	    if ($odict ne $dict) {
	      if ($name eq "types" and $dict->{ignore_known_types}) {
		next;
	      }
	    }
	  } else {
	    $keyis = "untracked";
	  }
	  print STDERR "WARNING: $swhat: subnode $coded_key already exists $keyis (overridden)\n"
	      if $verbose and $subname ne "types";	
	}
      } else {
	push @{$objinfo->{order}}, $key;
      }
      my $subtype = stash_hash_subtype($dict, $type, $key, $what);
      my $subref = ($objpm ? $obj->getref($key) : \$obj->{$key});
      ($$subref, $error) = stash_xml_read_node($dict, $subnode, $subname, $subtype, $subref, $whatfile);
      return (undef, $error) if defined($error);
    }
    return ($obj, undef);
  } elsif ($tag eq "array") {
    my ($objinfo, $objmodel);
    if (defined($obj)) {
      $objinfo = $dict->{objs}{$obj};
      defined($objinfo) or die "ERROR: $what: objinfo unexpectedly undefined";
      die "ERROR: dict ".$dict->{name}.": $what: cannot re-enter array of another dictionary (".$objinfo->{dict}{name}.")"
      	if $objinfo->{dict} ne $dict;
      $objinfo->{type} = $type if defined($type);	
      $objinfo->{typefinal} ||= $typefinal;
    } else {
      ($obj, $objmodel) = stash_create_array_object($dict, $type, $name);
$dict->{name} or die "oops 4";
      $objinfo = { full=>$name, bestfull=>$name, obj=>$obj, basetype=>"array", order=>[],
		done=>0, dict=>$dict, type=>$type, typefinal=>$typefinal,
		objmodel=>$objmodel };
      $dict->{objs}{$obj} = $objinfo;
    }
    $$oref = $obj;
    ref($obj) eq "ARRAY" or UNIVERSAL::isa($obj, "ArrayObject") or die "ERROR: $what: $name is not an array";
    if (!defined($type)) {
      $type = dict_lookup_type($dict, "array");
    }
    die "ERROR: $what: type is not an array" if $type->{basetype} ne "array";
    if (exists($node->{"value"})) {
      @children and die "ERROR: $what: array has both value=... and subnode entries, can only have either one";
      my $value = $node->{"value"};
      ($obj, $error) = parse_typed_value($dict, $type, $value, 1, $obj, $what);
      defined($error) and die "ERROR: $what: $error";
    } else {
      my $i = 0;			
      foreach my $subnode (@children) {
	my $swhat = $what;
	$swhat .= "; index $i (".$subnode->{".tag"}.")";
	my $index = delete $subnode->{i};
	if (defined($index)) {
	  $index =~ /^(?:0|[1-9]\d*)$/ or die "ERROR: $swhat: invalid array index '$index'";
	  $index >= 1000000 and print STDERR "WARNING: $swhat: excessive array index $index\n";
	  $index == $i and print STDERR "WARNING: $swhat: redundant array index $index\n";
	  $i = $index;
	  $swhat = "$what; index $i (".$subnode->{".tag"}.")";
	}
	defined($subnode->{n}) and print STDERR "WARNING: $swhat: array subnode has name (n) (reserved for hash subnodes)";
	my $subtype = stash_array_subtype($dict, $type, $i, $what);
	if (exists($obj->[$i])) {
	  print STDERR "WARNING: $swhat: index already exists (overridden)\n" if $verbose;
	}
	my $iobj;
	($iobj, $error) = stash_xml_read_node($dict, $subnode, "${name}.${i}", $subtype, \$obj->[$i], $whatfile);
	return (undef, $error) if defined($error);
	$i++;
      }
    }
    return ($obj, undef);
  } elsif ($tag eq "refh" or $tag eq "refa") {
    defined($obj) and die "$what: object of that name already exists";
    defined($nodetypename) and !$dict->{ignore_unsupported_typing} and print STDERR
	"WARNING: $what: type override (t='...') not expected or yet supported with $tag (ignored)\n";
    my $link = $node->{link};
    defined($link) or return (undef, "$what: missing link");
    if ($link =~ /^\./) {	
      my @prefix = split(".", $name); pop @prefix;
      my $blink = $link;
      while ($blink =~ s/^\.//) {
	@prefix or die "$what: relative link goes back too far ($link)";
	pop @prefix; 
      }
      $link = join(".", (@prefix, $blink));
    }
    ($obj, $error) = stash_lookup($dict, $link);
    return (undef, "$what: fw ref or dangling link?: $error") if defined($error);
    return (undef, "$what: can't link to undef") unless defined($obj);
    my $objinfo = $dict->{objs}{$obj};
    $$oref = $obj;
    return ($obj, undef);
  } elsif ($tag =~ /^i32|u32|i64|u64|string|float$/) {
    defined($obj) and print STDERR "INFO: $what: object exists, content overridden\n" if $verbose;
    defined($nodetypename) and !$dict->{ignore_unsupported_typing} and print STDERR
	"WARNING: $what: type override (t='...') not expected or yet supported with $tag (ignored)\n";
    $$oref = undef;
    if (defined($type)) {
      die "ERROR: $what: got tag $tag but expected type ".stash_type_name($dict,$type)
		." (basetype ".$type->{basetype}.")" if $type->{basetype} ne $tag;
    } else {
      $type = dict_lookup_type($dict, $tag);	
    }
    defined($node->{"value"}) or return (undef, undef);
    my $v = $node->{"value"};
    ($v, $error) = parse_typed_value($dict, $type, $v, 0, undef, $what);
    return ($v, "$what: $error") if defined($error);
    $$oref = $v;
    return ($v, undef);
  } elsif ($tag eq "undef") {
    defined($obj) and die "$what: object exists, but read as undef";
    defined($nodetypename) and !$dict->{ignore_unsupported_typing} and print STDERR
	"WARNING: $what: type override (t='...') not expected or yet supported with $tag (ignored)\n";
    $$oref = undef;
    return (undef, undef);
  } else {
    print STDERR "### WARNING: ignoring unrecognized tag '$tag'\n" if $stash_debug;
    $$oref = $obj;
    return ($obj, undef);
  }
}
sub xml_scan2 {
  my ($xmlines, $endtag, $indent, $line) = @_;
  my $matchtag = ($endtag eq "" ? "" : "\L</$endtag>");	
  my $whattag = ($endtag eq "" ? "EOF" : $matchtag);
  my @children = ();
  defined($indent) or $indent = "";
  my $xml = "";
  my $freetext;
  MAINLOOP: while (1) {
    $freetext = "";
    while(1) {
      $xml =~ s/^([^<]*)//;
      $freetext .= $1;
      if ($xml eq "") {			
	$freetext .= "\n";
	last MAINLOOP unless @$xmlines;
	$xml = shift @$xmlines;
	$line++;
	next;
      }
      last unless $xml =~ s/^<\!--//;
      $freetext .= "<!--";
      while (1) {
	if ($xml =~ s/(.*?-->)//) {	
	  $freetext .= $1;
	  last;
	}
	$freetext .= $xml."\n";
	last MAINLOOP unless @$xmlines;
	$xml = shift @$xmlines;
	$line++;
      }
    }
    if ($xml =~ s@^(</\w+>)@@) {	
      unshift @$xmlines, $xml;
      return (\@children, $freetext, $line, undef) if $matchtag eq "\L$1";
      $xmlines->[0] = $1 . $xmlines->[0];
      return (\@children, $freetext, $line,
	      "mismatching or unexpected end tag, expected ".($endtag eq "" ? "EOF" : "</$endtag>"));
    }
    if ($xml !~ s/^<(\??\w+)//) {
      unshift @$xmlines, $xml;
      return (\@children, $freetext, $line, "incorrectly formatted XML");
    }
    my $tag = $1;
    my $node = { ".tag" => $tag, ".prevtext" => $freetext, ".lasttext" => "",
		 ".headkeys" => [], ".children" => [],
    		 ".line" => $line, ".endline" => $line };
    push @children, $node;
    while ($xml =~ s/^\s+//) {		
      if ($xml eq "") {			
	@$xmlines or return (\@children, "", $line, "unterminated <$tag> tag");
	$xml = shift @$xmlines;
	$line++;
	$node->{".endline"} = $line;
	next;
      }
      last unless $xml =~ s/^([a-zA-Z_0-9%]+)\s*//;
      my $key = key_decode($1);
      if ($xml !~ s/^=\s*('|")//) {
	$xml = $key.$xml;  unshift @$xmlines, $xml;
	return (\@children, "", $line, "unsupported key=value (eg. value must start on same line) (xml='$xml')");
      }
      my $qu = $1;
      my $value = "";
      while (1) {
	($qu eq "'") ? ($xml =~ s/^([^']*)//) : ($xml =~ s/^([^"]*)//);
	$value .= $1;
	last if $xml ne "";
	@$xmlines or return (\@children, "", $line, "unterminated quoted value of key $key in tag <$tag>");
	$xml = shift @$xmlines;
	$line++;
	$node->{".endline"} = $line;
	$value .= "\n";
      }
      $xml =~ s/^.//;	
      if (exists($node->{$key})) {
	$node->{$key} eq $value or return (\@children, "", $line,
		"duplicate key '$key' (value '$value' != '".$node->{$key}."') in tag '$tag'");
	next;	
      }
      push @{$node->{".headkeys"}}, $key;
      $node->{$key} = xml_decode_string($value);
    }
    if ($tag =~ /^\?/) {
      $xml =~ s/^\?>// or return (\@children, "", $line, "expected '?>' to terminate '<$tag'");
    } elsif ($xml =~ s/^>//) {		
      my ($gchild,$glast,$gerror);
      unshift @$xmlines, $xml;
      ($gchild,$glast,$line,$gerror) = xml_scan2($xmlines, $tag, $indent." ", $line);
      last unless @$xmlines;
      $node->{".children"} = $gchild;
      $node->{".lasttext"} = $glast;
      defined($gerror) and return (\@children, "", $line, $gerror);
      $xml = shift @$xmlines;
    } elsif ($xml =~ s|^/>||) {
    } else {
      return (\@children, "", $line, "expected '>' or '/>' at end of tag '$tag'");
    }
  }
  return (\@children, $freetext, $line, undef) if $endtag eq "";
  return (\@children, $freetext, $line,
	  "unexpected EOF, expected </$endtag>");
}
sub xml_scan {
  my ($xml, $endtag, $indent, $line) = @_;
  my $matchtag = ($endtag eq "" ? "" : "\L</$endtag>");	
  my $whattag = ($endtag eq "" ? "EOF" : $matchtag);
  my @children = ();
  defined($indent) or $indent = "";
  while (1) {
    my $freetext = "";
    while ($xml =~ s/^([^<]*(?:<\!--.*?-->)?)//s and $1 ne "") {
      $freetext .= $1;
      $line += ($1 =~ tr/\n/\n/);
    }
    if ($xml =~ s@^(</\w+>|$)@@) {
      return (\@children, $freetext, $xml, $line, undef) if $matchtag eq "\L$1";
      return (\@children, $freetext, $1.$xml, $line,
	      "mismatching or unexpected end tag, expected ".($endtag eq "" ? "EOF" : "</$endtag>"));
    }
    $xml =~ s/^<(\??\w+)// or return (\@children, $freetext, $xml, $line, "incorrectly formatted XML");
    my $tag = $1;
    my $node = { ".tag" => $tag, ".prevtext" => $freetext, ".lasttext" => "",
		 ".headkeys" => [], ".children" => [],
    		 ".line" => $line, ".endline" => $line };
    push @children, $node;
    while ($xml =~ s/^(\s*(\w+)\s*=\s*(?:'([^']*)'|"([^"]*)"))//) {
      my ($key,$val1,$val2) = ($2,$3,$4);
      $line += ($1 =~ tr/\n/\n/); $node->{".endline"} = $line;
      defined($val1) or $val1 = "";
      defined($val2) or $val2 = "";
      my $value = $val1.$val2;
      if (exists($node->{$key})) {
	$node->{$key} eq $value or return (\@children, "", $xml, $line,
		"duplicate key '$key' (value '$value' != '".$node->{$key}."') in tag '$tag'");
	next;	
      }
      push @{$node->{".headkeys"}}, $key;
      $node->{$key} = xml_decode_string($value);
    }
    if ($tag =~ /^\?/) {
      $xml =~ s/^(\s*\?>)// or return (\@children, "", $xml, $line, "expected '?>' to terminate '<$tag'");
      $line += ($1 =~ tr/\n/\n/); $node->{".endline"} = $line;
    } elsif ($xml =~ s/^(\s*>)//) {
      $line += ($1 =~ tr/\n/\n/); $node->{".endline"} = $line;
      my ($gchild,$glast,$gerror);
      ($gchild,$glast,$xml,$line,$gerror) = xml_scan($xml, $tag, $indent." ", $line);
      $node->{".children"} = $gchild;
      $node->{".lasttext"} = $glast;
      defined($gerror) and return (\@children, "", $xml, $line, $gerror);
    } elsif ($xml =~ s|^(\s*/>)||) {
      $line += ($1 =~ tr/\n/\n/); $node->{".endline"} = $line;
    } else {
      return (\@children, "", $xml, $line, "expected '>' or '/>' at end of tag '$tag'");
    }
  }
}
my %expmap = (
	"<"  => "&lt;",
	">"  => "&gt;",
	"&"  => "&amp;",
	"'"  => "&apos;",
	"\"" => "&quot;",
);
my %decmap = (
	"lt"	=> "<",
	"gt"	=> ">",
	"amp"	=> "&",
	"apos"	=> "'",
	"quot"	=> "\"",
);
my $xml_default_encode = "<>&'\"";
sub xml_encode_string {
  my ($str, $expanded) = @_;
  my @emap = (1 x 32, 0 x 95, 1 x 129);		
  foreach my $i (0 .. length($expanded)-1) {
    $emap[ord(substr($expanded,$i,1))] ^= 1;
  }
  my $s = "";
  foreach my $i (0 .. length($str)-1) {
    my $c = substr($str,$i,1);
    my $n = ord($c);
    $s .= $emap[$n] ? ($expmap{$c} || "&\#$n;") : $c;
#      $s .= (exists($expmap{$c}) ? $expmap{$c} : "&\#$n;");
  }
  return $s;
}
sub xml_decode_string {
  my ($s) = @_;
  my $str = "";
  while ($s =~ s/^([^&]*)&//) {
    $str .= $1;
    if ($s =~ s/^\#(\d+);//) {		# decode &#NN; form
      $str .= chr($1);
    } elsif ($s =~ s/^(lt|gt|amp|apos|quot);//) {	
      $str .= $decmap{$1};
    } else {
      $str .= "&";
    }
  }
  return $str.$s;
}
sub stash_xml_write {
  my ($dict, $topname, $headcomment) = @_;
  return stash_text_write($dict, $topname, $headcomment) if $force_no_xml == 1;
  return stash_bin_write($dict, $topname, $headcomment) if $force_no_xml == 2;
  $dict->{doneseq} = $doneseq++;	
  my $xml = "";
  if (defined($headcomment) and $headcomment ne "") {
    $xml = $headcomment;
    chomp($xml);
    $xml .= "\n";
    $xml =~ s/^/     /mg;
    $xml =~ s/^     //;
    $xml = "<!-- $xml  -->\n\n";
  }
  defined($topname) or $topname = "stash";
  $xml .= "<$topname>\n"
  	   ."  <!-- This XML sequence is in 'Stash' format - see Xtensa/share/Stash.pm -->\n"
	   ."\n";
  foreach my $rootname (stash_order_roots($dict)) {
    $xml .= stash_xml_write_node($dict, $dict->{roots}{$rootname}, $rootname, 1, $rootname, 0, undef);
    $xml .= "\n";
  }
  return $xml . "</$topname>\n";
}
sub stash_xml_write_node {
  my ($dict, $objref, $objfullname, $namekind, $indexname, $indent, $expected_type) = @_;
  my $named_tags = 1;		
  if (!defined($indexname) and $objfullname =~ m/([^\.]+)$/) {
    $indexname = $1;
    $namekind = ($indexname =~ /^\d+$/) ? 0 : 1;
  }
  my ($kind, $value, $keyorder, $type, $typename, $type_known_to_reader)
	= stash_classify($dict, $objref, $objfullname, $expected_type, $dict->{doneseq});
  if ($kind eq "extern") {	
    $value = $value->{bestfull};
    $kind = "link";
  }
  $kind = ($keyorder ? "refa" : "refh") if $kind eq "link";
  return "" if $kind eq "skip" or $kind eq "extern";	
  my $xml = "";
  my $x = " " x $indent;
  my ($pretag, $posttag);
  if ($named_tags and $namekind and $indexname =~ /^\w+$/ and $indexname !~ /^\d+$/) {
    $pretag = "$indexname b=\"$kind\"";
    $posttag = $indexname;
  } else {
    $pretag = "$kind";
    if ($namekind) {
      $pretag .= " n=\"$indexname\"";
    } elsif (defined($namekind)) {
      $pretag .= " i=\"$indexname\"";
    }
    $posttag = $kind;
  }
  return "$x<$pretag/>\n" unless defined($value);
  die "stash_xml_write_node: can't output string valued '*' ($objfullname)"
	if $kind eq "string" and $value eq "*";
  return sprintf("$x<%s value=\"".(($value<10)?"%u":"0x%x")."\"/>\n", $pretag, $value)
	if $kind eq "u32";
  return sprintf("$x<%s value=\"".(($value<10)?"%d":"0x%x")."\"/>\n", $pretag, $value)
	if $kind eq "i32";
  return sprintf("$x<%s value=\"%s\"/>\n", $pretag, xml_encode_string($value, $xml_default_encode))
	if $kind eq "string" or $kind eq "float";
  return sprintf("$x<%s link=\"%s\"/>\n", $pretag, $value)
	if $kind eq "refa" or $kind eq "refh";
  if ($kind eq "hash") {
    my $href = $value;
    my $comment = "";
    $xml .= "$x<$pretag";
    if (defined($typename) and $typename ne "hash") {
      if (!$type_known_to_reader) {
	$xml .= " t=\"$typename\"";
      } else {
      }
    }
    my @nonscalar = ();
    foreach my $key (@$keyorder) {
      my $e = $href->{$key};
      my $subtype = stash_hash_subtype($dict, $type, $key, $objfullname);
      my $coded_key = key_encode($key);
      my ($etype, $evalue, $estr) = xml_scalar_typevalue($e, $objfullname.".".$coded_key, $subtype);
      if ( 
	  ref($e) eq ""
	  and !($etype eq "string" and $estr eq "*")
	  and ($etype ne "string" or $evalue =~ /^[-a-zA-Z0-9_.=+:]*$/)) {
	die "=== ${objfullname}.$coded_key of expected basetype ".$subtype->{basetype}." (type ", stash_type_name($dict,$subtype), ") yet thought to be scalar $etype (value $e).\n" if defined($e) and defined($subtype) and $subtype->{basetype} =~ /^array|hash$/;
	$xml .= sprintf " %s=\"%s\"", $coded_key, $estr;
	if ($key eq "xt") {
	  die "$objfullname: caught xt -- $e -- ref is ".ref($e).", real xt is ".$objref->xt." (ref is ".ref($objref->xt).")";
	}
      } else {
	push @nonscalar, $key;
      }
    }
    if (@nonscalar) {
      $xml .= ">$comment\n";
      foreach my $key (@nonscalar) {
	my $e = $href->{$key};
	my $coded_key = key_encode($key);
	$xml .= stash_xml_write_node($dict, $e, $objfullname . ".$coded_key", 1, $key, $indent+1,
				stash_hash_subtype($dict, $type, $key, $objfullname));
      }
      $xml .= "$x</$posttag>\n";
    } else {
      $xml .= "/>$comment\n";
    }
    return $xml;
  }
  if ($kind eq "array") {
    $xml .= "$x<$pretag";
    $xml .= " t=\"$typename\"" if defined($typename) and !$type_known_to_reader and $typename ne "array";
    if (@$value) {
      my %etypes = ();
      my @evalues = ();
      foreach my $i (0 .. $#$value) {
	my $e = $value->[$i];
	my $subtype = stash_array_subtype($dict, $type, $i, $objfullname);
	my ($etype, $evalue, $estr) = xml_scalar_typevalue($e, $objfullname, $subtype);
	$etypes{$etype}++;
	push @evalues, $estr;
      }
      my $ets = join("+", sort keys %etypes);
      if ($type_known_to_reader and ($ets eq "i32" or $ets eq "u32"
		or ($ets eq "string" and !grep(/[^-a-zA-Z0-9_.[\]=+{}:<>@()]/,@evalues)))) {
	$xml .= " value=\"" . join(",",@evalues) . "\"/>\n";
      } else {
	$xml .= ">\n";
	foreach my $i (0 .. $#$value) {
	  my $e = $value->[$i];
	  my $subtype = stash_array_subtype($dict, $type, $i, $objfullname);
	  $xml .= stash_xml_write_node($dict, $e, $objfullname . ".$i", undef, "", $indent+1, $subtype);
	}
	$xml .= "$x</$posttag>\n";
      }
    } else {
      $xml .= "/>\n";
    }
    return $xml;
  }
  if ($kind eq "code") {
    #print STDERR "### Don't know how to dump code.\n" if $stash_debug;
    return $xml . "$x<$pretag> ... </$posttag>\n";
  }
  if ($kind eq "other") {
    print STDERR "### Don't know how to dump kind $kind (a ".ref($objref).")\n" if $stash_debug;
    return $xml . sprintf "$x<%s type=\"%s\"> ... </%s>\n", $pretag, $value, $posttag;
  }
  die "stash_xml_write_node: unexpected kind '$kind' from stash_classify";
}
sub xml_scalar_typevalue {
  my ($obj, $fullname, $type) = @_;
  my $basetype = "undef";
  $basetype = $type->{basetype} if defined($type);
  return ($basetype, undef, "*") unless defined($obj);
  return ($basetype, $obj, "+++") if $basetype eq "array" or $basetype eq "hash";
  use Carp;
  confess "object $fullname is not a scalar (".ref($obj).") but typed as one ($basetype)" if $basetype ne "undef" and ref($obj) ne "";
  return (ref($obj), undef, "....") if ref($obj) ne "";
  return ($basetype, 0+$obj,sprintf(((0+$obj<10)?"%u":"0x%x"),0+$obj)) if $basetype eq "u32";
  return ($basetype, 0+$obj,sprintf(((0+$obj<10)?"%d":"0x%x"),0+$obj)) if $basetype eq "i32";
  return ($basetype, $obj, xml_encode_string($obj, $xml_default_encode)) if $basetype ne "";
  if ($obj =~ /^\-?(0x[0-9a-f]+|0[0-7]*|[1-9][0-9]*)$/i) {
    return ("s32", 0+$obj,sprintf("%d",0+$obj)) if $obj =~ /^\-/;
    return ("u32", 0+$obj,sprintf("0x%x",0+$obj)) if 0+$obj > 0x7FFFFFFF;
    return ("i32", 0+$obj,sprintf(((0+$obj<10)?"%u":"0x%x"),0+$obj));
  }
  return ("string", $obj, xml_encode_string($obj, $xml_default_encode));
}
1;
