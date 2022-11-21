# Copyright (c) 2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
package Xtensa::YAML;
use Exporter 'import';
@EXPORT = qw(YAML_Load YAML_Dump YAML_Dump_withcode YAML_Dump_objects YAML_Load_objects);
use SelfLoader;
use strict;
sub YAML_Load;
sub YAML_Dump;
sub YAML_Dump_withcode;
1;
__DATA__
sub UseYAML {
    if (0 and $^O =~ /MSWin32|cygwin/i) {
	my $swtools = $ENV{SWTOOLS};
	if (defined($swtools)) {
	    my $libpath = $swtools."/lib/perl";
	    if (-d $libpath) {
		push @INC, $libpath;
	    }
	}
	require YAML;
	import YAML;
	return 0;
    } else {
	require YAML::XS;
	import YAML::XS;
	return 1;
    }
}
sub YAML_Load {
    my ($yaml, $blessok, $codeok) = @_;
    $blessok = 0 unless defined($blessok);
    $codeok = 0 unless defined($codeok);
    my $is_yaml_xs = UseYAML();
    if ($is_yaml_xs) {
	no warnings 'once';
	local $YAML::XS::LoadBlessed = $blessok;
	local $YAML::XS::LoadCode = $codeok;
	local $YAML::XS::UseCode = $codeok;
	return Load($yaml);
    }
    return Load($yaml);
}
sub YAML_Dump {
    my (@objs) = @_;
    my $is_yaml_xs = UseYAML();
    if ($is_yaml_xs) {
	no warnings 'once';
	local $YAML::XS::DumpCode = 0;
	local $YAML::XS::UseCode = 0;
	return Dump(@objs);
    }
    return Dump(@objs);
}
sub YAML_Dump_withcode {
    my (@objs) = @_;
    my $is_yaml_xs = UseYAML();
    if (0 and $is_yaml_xs) {	
	no warnings 'once';
	local $YAML::XS::DumpCode = 1;
	local $YAML::XS::UseCode = 1;
	return Dump(@objs);
    }
    return Dump(@objs);
}
sub YAML_Dump_objects {
    my ($objs, $skip_packages, $withcode) = @_;
    $withcode = ($withcode ? 1 : 0);
    UNIVERSAL::isa($objs, "HASH") or die "YAML_Dump_objects: object to dump must be a hash";
    my $debug_yaml = 0;
    printf STDERR "--- Got %d entries in classdef\n", scalar(keys %Object::classdef) if $debug_yaml;
    printf STDERR "--- Got %d entries in classfields\n", scalar(@Object::classfields) if $debug_yaml;
    my $obj = {
	%$objs,
	_class_def => \%Object::classdef,		
	_class_fields => \@Object::classfields,		
	_use_hash => Object::UsingHashObjects(),
    };
    bless($obj, ref($objs)) if ref($objs) ne "HASH";	
    $obj->{_skipped_packages} = $skip_packages if defined($skip_packages);
    my $yaml;
    my $is_yaml_xs = UseYAML();
    if ($is_yaml_xs) {
	no warnings 'once';	
	$yaml = Dump( $obj );
	if ($withcode) {
	    my @fixes; my $id = 0; my $tag;
	    $yaml =~ s|^( *)(\S.*):(?: \&(\S+))?( \!\!\S+)?\s*\n^\1  (\S.*):( \&\S+)? \!\!perl/code(\S*)| $tag=(defined($3)?$3:"id$id"); push @fixes,[$tag,$5,$7]; $id++; $1.$2.": &$tag".($4//"")."\n".$1."  ".$5.":".($6//""); |gem;
	    $yaml =~ s|^( *)- (\S.*):(?: \&(\S+))?( \!\!\S+)?\s*\n^\1    (\S.*):( \&\S+)? \!\!perl/code(\S*)| $tag=(defined($3)?$3:"id$id"); push @fixes,[$tag,$5,$7]; $id++; $1."- ".$2.": &$tag".($4//"")."\n".$1."    ".$5.":".($6//""); |gem;
	    print STDERR "Got these code fixups: ".join(", ", map($_->[0]."|".$_->[1], @fixes)).".\n" if $debug_yaml;
	    if ($yaml =~ /\!\!perl\/code/) {
		print STDERR "WARNING: unhandled perl code is present in generated YAML, loading this YAML in perl will fail.\n";
	    }
	    $yaml .= "_code_fixup:\n".join("", map("- [ *".$_->[0].", '".$_->[1]."', ".(defined($_->[2]) ? "'".$_->[2]."'" : "~")." ]\n", @fixes));
	}
    } else {
	$yaml = Dump( $obj );
    }
    return $yaml;
}
sub YAML_Load_objects {
    my ($yaml, $loadpkg, $loadpkg_list, $codeok, $debug_yaml, $_debug_autoload) = @_;
    $loadpkg //= 'autoload';
    $loadpkg_list //= [];
    defined({nobless=>0, never=>1, semiload=>2, autoload=>3, always=>4}->{$loadpkg})
	or die "YAML_Load_objects: unrecognized 2nd argument, must be one of nobless|never|semiload|autoload|always";
    our $debug_autoload = $_debug_autoload;
    our $show_timing = 0;
    use Time::HiRes qw( gettimeofday tv_interval );
my $t2 = [gettimeofday];
    my $hash;
    my $is_yaml_xs = UseYAML();
    if ($is_yaml_xs) {
	no warnings 'once';
	local $YAML::XS::LoadBlessed = ($loadpkg ne 'nobless');
	local $YAML::XS::LoadCode = $codeok;
	local $YAML::XS::UseCode = $codeok;
	$hash = Load($yaml);
    } else {
	$hash = Load($yaml);
    }
    if (!UNIVERSAL::isa($hash, "HASH")) {
	printf STDERR "--- YAML_Load_objects: got a %s instead of a HASH, returning as is\n", ref($hash) if $debug_yaml;
	return $hash;
    }
my $t3 = [gettimeofday];
    use vars qw(%classdef @classfields);
    our $cdefs    = delete $hash->{_class_def};
    our $cfields  = delete $hash->{_class_fields};
    our $usehash  = delete $hash->{_use_hash};
    our $skippkgs = delete $hash->{_skipped_packages};
    my $fixups = delete $hash->{_code_fixup};
    if ($codeok) {
	foreach my $fixup (@$fixups) {
	    my ($tagged, $member, $opttype) = @$fixup;
	    print STDERR "--- Fixing up $tagged -> $member (".($opttype // "undef").")\n" if $debug_yaml;
	    my $code_string = $tagged->{$member};
	    $tagged->{$member} = eval "sub $code_string"; die if $@;
	    print STDERR "---- and is now: ", $tagged->{$member}, "\n" if $debug_yaml;
	}
    }
    printf STDERR "--- Got %d entries in cdefs\n", scalar(keys %$cdefs) if $debug_yaml;
    printf STDERR "--- Got %d entries in cfields\n", scalar(@$cfields) if $debug_yaml;
    printf STDERR "--- Using %s based objects\n", ($usehash ? "hash" : "array") if $debug_yaml;
    printf STDERR "BEFORE: --- Got %d entries in classdef\n", scalar(keys %Object::classdef) if $debug_yaml;
    printf STDERR "BEFORE: --- Got %d entries in classfields\n", scalar(@Object::classfields) if $debug_yaml;
my $t4 = [gettimeofday];
    use Object;
    my $was_hash = Object::UsingHashObjects();
    if (defined($usehash) and defined($was_hash) and ($usehash?1:0) != ($was_hash?1:0)) {
	print STDERR "WARNING: YAML_Load_objects: Object.pm switch from %s based objects (the two must not mix for a given class)\n",
		($usehash ? "array to hash" : "hash to array");
    }
    Object::UseHashObjects($usehash);	
    our %autoload_classes;
    printf STDERR "MID: --- Got %d entries in classdef\n", scalar(keys %Object::classdef) if $debug_yaml;
    printf STDERR "MID: --- Got %d entries in classfields\n", scalar(@Object::classfields) if $debug_yaml;
    foreach my $class (@$cfields) {
	my ($classname, $classbase, $classfieldsref) = @$class;
	last if $loadpkg eq 'nobless';
	if (!exists($Object::classdef{$classname})) {
	    print STDERR "Declaring class '$classname'\n" if $debug_yaml;
	    Object::_class($classname, (defined($classbase) ? $classbase->[0] : undef), @$classfieldsref);
	}
	next if grep($_ eq $classname, @$skippkgs);
	next if @$loadpkg_list and !grep($_ eq $classname, @$loadpkg_list);
	if ($loadpkg eq 'autoload' or $loadpkg eq 'semiload') {
	    print STDERR "Setting up AUTOLOAD for class '$classname'\n" if $debug_yaml;
	    eval "package $classname;\n"
		."sub AUTOLOAD {\n"
		."  our \$AUTOLOAD;\n"
		."  my (\$self,\$name) = Xtensa::YAML::class_autoload(\$_[0], '$classname', \$AUTOLOAD);\n"
		."  if (\$self) { shift; return \$self->\$name(\@_); } elsif (\$name) { goto &\$name; }\n"
		."}\n"
		."__DATA__\n";
	    $@ and die "Something went wrong defining ${classname}::AUTOLOAD : $@";
	    $autoload_classes{$classname} = 1 if $loadpkg eq 'autoload';
	}
	elsif ($loadpkg eq 'always') {
	    print STDERR "... Requiring package '$classname'\n" if $debug_yaml;
	    eval "require $pkg";
	    $@ and die "ERROR: YAML_Load_objects: error on require '$pkg': $@";
	}
    }
my $t5 = [gettimeofday];
    printf STDERR "AFTER: --- Got %d entries in classdef\n", scalar(keys %Object::classdef) if $debug_yaml;
    printf STDERR "AFTER: --- Got %d entries in classfields\n", scalar(@Object::classfields) if $debug_yaml;
    print STDERR "--- Parse YAML      delay = ", tv_interval($t2, $t3), "\n" if $show_timing;
    print STDERR "--- Fixups          delay = ", tv_interval($t3, $t4), "\n" if $show_timing;
    print STDERR "--- Class defs      delay = ", tv_interval($t4, $t5), "\n" if $show_timing;
    return $hash;
}
sub class_autoload {
    my ($self, $pkg, $fullname) = @_;
    local $@;		
    our $debug_autoload;
    my ($package, $name) = ($pkg, $fullname);
    $fullname =~ m/^(.*)::(.*)$/ and ($package, $name) = ($1, $2);
    return if $name eq 'DESTROY';
    my $t1 = [gettimeofday];
    print STDERR "obj AUTOLOAD '$fullname'   ($pkg)\n" if $debug_autoload;
    use Object;
    if ((UNIVERSAL::isa($self, 'Xtensa::ProcessorModule') or UNIVERSAL::isa($self, 'ProcessorModule')) and $self->can("xt") and $self->xt->can($name)) {
      print STDERR "obj AUTOLOAD DELEGATING $fullname FROM ProcessorModule ($self) TO Processor (".$self->xt.")->$name(...)\n" if $debug_autoload;
      return ($self->xt, $name);
    }
    our %autoload_classes;
    if (!exists($autoload_classes{$pkg})) {
	{no strict "refs"; delete ${"${pkg}::"}{AUTOLOAD};}
    } else {
	print STDERR "... Requiring package '$pkg' for function '$fullname'\n" if $debug_autoload;
	eval "require $pkg";
	$@ and die "ERROR: obj AUTOLOAD: error on require '$pkg' for sub '$fullname': $@";
	my @keys = sort keys %autoload_classes;
	foreach my $cname (@keys) {
	    (my $path = $cname) =~ s|::|/|g;
	    if (exists($INC{"${path}.pm"})) {
		print STDERR "... Saw $cname as loaded\n" if $debug_autoload;
		delete $autoload_classes{$cname};	
	    }
	}
	my $t2 = [gettimeofday];
	our $show_timing;
	print STDERR "--- obj AUTOLOAD    delay = ", tv_interval($t1, $t2), "\n" if $show_timing;
    }
    if (UNIVERSAL::isa($self, $package)) {	
	print STDERR "--- Invoking $self -> $name (...)  --- ", ref($self), "\n" if $debug_autoload;
	return ($self, $name);
    } else {
	print STDERR "--- Invoking $fullname (...)  --- ", ref($self), "\n" if $debug_autoload;
	return (undef, $fullname);
    }
}
1;
