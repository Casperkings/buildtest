# Copyright (c) 2001, 2008 by Tensilica Inc.  ALL RIGHTS RESERVED.
package Object;
use Exporter ();
@Object::ISA = qw(Exporter);
@Object::EXPORT = qw(BaseClass Class BaseStruct Struct DerivedMethods);
@Object::EXPORT_OK = qw(BaseClass Class BaseStruct Struct DerivedMethods EnableDerivedMethods Deriving UseHashObjects UsingHashObjects);
%Object::EXPORT_TAGS = ();
use strict;
use vars qw(%classdef);
use vars qw(@classfields);
use vars qw(%derived_methods);
our $deriving = 0;		
our $use_hash = undef;		
sub _class {
  my ($package, $basename, @fields) = @_;
  my $base;
  if (defined($basename)) {
    $base = $classdef{$basename};
    if (!defined($base) and $basename eq "HashObject") {
      $base = $classdef{$basename} = ["HashObject", undef, 0, 0, undef, '', {}, undef];
    }
    die ("Object.pm: $basename is not a Class in definition of $package.\n")
	unless defined($base);
  }
  my $sig = join(',', defined($base) ? $base->[0] : '', @fields);
  my $d = $classdef{$package};
  if (defined($d)) {
    if ($sig ne $d->[5]) {
      die("$package class being redefined.\n");
    } else {
      return;
    }
  }
  my $containing_package;
  my ($cpackage, $filename, $line, $subroutine) = caller(1);
  if ($cpackage eq $package and $subroutine =~ m/^Object::/) {
    my ($findinc) = grep($_ ne "" && substr($filename, 0, length($_)) eq $_, @INC);
    if (defined($findinc)) {
      $containing_package = substr($filename, length($findinc));
      $containing_package =~ s|^[/\\]+||;
      $containing_package =~ s|[/\\]|::|g;
      $containing_package =~ s|\.pm$||;
    } 
  } 
  my @blist = ();
  my $b;
  for ($b = $base; defined($b); $b = $b->[1]) {
    push (@blist, $b->[0]);
  }
  my $is_hash = (@blist and $blist[-1] eq "HashObject");
  $use_hash //= 0 unless $is_hash;
  push @blist, ($use_hash ? 'HashObject' : 'ObjectClass'), 'Exporter' unless $is_hash;
  my $code = "package $package;\n";
  $code .= '@' . $package . '::ISA = qw(' . join(' ', @blist) . ') unless (@' . $package . "::ISA);\n";
  my $new;
  my $size;
  my $arg;
  my %members = ();
  if (!$is_hash) {
  $code .= "use Exporter ();\n";
  $code .= '@' . $package . '::EXPORT = () unless (@' . $package . "::EXPORT);\n";
  $code .= '@' . $package . '::EXPORT_OK = () unless (@' . $package . "::EXPORT_OK);\n";
  $code .= '%' . $package . '::EXPORT_TAGS = () unless (%' . $package . "::EXPORT_TAGS);\n";
  my %builtin;
  foreach (grep(/^:/, @fields)) {
    $builtin{substr($_, 1)} = 1;
  }
  $new = $builtin{'new'} || $builtin{'newarg'} ? 'new' : '_init';
  my $init;
  if (defined($base)) {
    $init = "sub $new {\n  my \$self = " . $base->[0] . "::" . $base->[4] . "(\@_);\n";
    $init .= "  _extend(\$self, \@_); }\n\n";
    $init .= "sub _extend {\n  my \$self = shift;\n";
    $size = $base->[2];
    $arg = $base->[3];
    %members = %{$base->[6]};
  } else {
    $init = "sub $new {\n  my \$self = bless ".($use_hash ? "{}" : "[]").", \$_[0];\n";
    $size = 0;
    $arg = 1;
  }
  foreach (@fields) {
    my ($type, $name) = /^([\$\+\@\%\&\:])(\w+)$/;
    die ("Object.pm: '$_' is not a legal class name.\n")
      unless defined($name);
    my $index = ($use_hash ? "{'$name'}" : "[$size]");
    if ($type eq "\$" || $type eq "\+") {
      $code .= "sub $name { \@_<=2 or \$_[0]->_err_parm_; \$_[0]->${index}; }\n";
      $code .= "sub set_$name { \@_==2 or \$_[0]->_err_parm_; \$_[0]->${index} = \$_[1]; }\n";
      $code .= "sub inc_$name { \@_==2 or \$_[0]->_err_parm_; \$_[0]->${index} += \$_[1]; }\n"
	  if $type eq "\+";
      if ($builtin{'newarg'}) {
	$init .= "  \$self->${index} = \$_[$arg];\n";
	$arg += 1;
      }
    } elsif ($type eq "\@") {
      $code .= "sub $name { \@_<=2 or \$_[0]->_err_parm_; \@_ == 1 ? \@{\$_[0]->${index}} : \$_[0]->${index}->[\$_[1]]; }\n";
      $code .= "sub set_$name { (\@_==2 or \@_==3) or \$_[0]->_err_parm_; if (\@_ == 2) { \$_[0]->${index} = \$_[1]; } else { \$_[0]->${index}->[\$_[1]] = \$_[2]; } }\n";
      $code .= "sub ref_$name { \@_==1 or \$_[0]->_err_parm_; \$_[0]->${index}; }\n";
      $code .= "sub push_$name { my \$self = shift(\@_); push(\@{\$self->${index}}, \@_); }\n";
      $code .= "sub pop_$name { \@_==1 or \$_[0]->_err_parm_; pop(\@{\$_[0]->${index}}); }\n";
      if ($builtin{'newarg'}) {
	$init .= "  \$self->${index} = \$_[$arg];\n";
	$arg += 1;
      } else {
	$init .= "  \$self->${index} = [];\n";
      }
    } elsif ($type eq "\%") {
      $code .= "sub $name { "
      ." \@_<=2 or \$_[0]->_err_parm_; \@_ == 1 ? values \%{\$_[0]->${index}} : \$_[0]->${index}->{\$_[1]}; }\n";
      $code .= "sub set_$name { (\@_==2 or \@_==3) or \$_[0]->_err_parm_; if (\@_ == 2) { \$_[0]->${index} = \$_[1]; } else { \$_[0]->${index}->{\$_[1]} = \$_[2]; } }\n";
      $code .= "sub del_$name { \@_==2 or \$_[0]->_err_parm_; if (\@_ == 2) { delete \$_[0]->${index}->{\$_[1]} } }\n";
      $code .= "sub ref_$name { \@_==1 or \$_[0]->_err_parm_; \$_[0]->${index}; }\n";
      $code .= "sub keys_$name { \@_==1 or \$_[0]->_err_parm_; keys(\%{\$_[0]->${index}}); }\n";
      if ($builtin{'newarg'}) {
	$init .= "  \$self->${index} = \$_[$arg];\n";
	$arg += 1;
      } else {
	$init .= "  \$self->${index} = {};\n";
      }
    } elsif ($type eq "\&") {
      $code .= "sub $name { my \$self = shift(\@_); \&{\$self->${index}}(\@_); }\n";
      $code .= "sub set_$name { \$_[0]->${index} = \$_[1]; }\n";
      $code .= "sub ref_$name { \$_[0]->${index}; }\n";
      if ($builtin{'newarg'}) {
	$init .= "  \$self->${index} = \$_[$arg];\n";
	$arg += 1;
      }
    } elsif ($type eq "\:") {
      if ($name eq 'new') {
      } elsif ($name eq 'newarg') {
      } elsif ($name eq 'copy') {
	my $newobj = ($use_hash ? "{ \%{\$_[0]} }" : "[ \@{\$_[0]} ]");
	$code .= "sub copy { bless $newobj, ref(\$_[0]); }\n";
      } else {
	die ("Object.pm: '$type$name' not understood.\n");
      }
      next;			
    } else {
      die ("Object.pm: '$_' must begin with '\$', '\+', '\@', '\%', or '\&'.\n");
    }
    $members{$name} = [$size, $type];
    $size += 1;
  }
  $init .= "  \$self;\n}\n";
  $code .= $init;
  }
  my @test = ("\$_[1] eq '$package'");
  for ($b = $base; defined($b); $b = $b->[1]) {
    push (@test, "\$_[1] eq '$b->[0]'");
  }
  $code .= "sub isa { " . join(' || ', @test) . "; }\n";
  eval $code;
  die ($@ . $code) if $@;
  $classdef{$package} = [$package, $base, $size, $arg, $new, $sig, \%members, $containing_package];
  push @classfields, [$package, $base, \@fields];
}
sub BaseClass {
  _class ((caller())[0], undef, @_);
}
sub Class {
  my ($base, @fields) = @_;
  my $name = (caller())[0];
  _class ($name, $base, @fields);
}
sub BaseStruct {
  my ($name, @fields) = @_;
  _class ($name, undef, ':newarg', @fields);
}
sub Struct {
  my ($name, $base, @fields) = @_;
  _class ($name, $base, ':newarg', @fields);
}
sub DerivedMethods {
  my (@methods) = @_;
  return unless $deriving;
  my $classname = (caller())[0];
  my $list = ($derived_methods{$classname} //= []);
  my %present = map { $_ => 1 } @$list;
  foreach (@methods) {
    next if $present{$_};
    push @$list, $_;
    $present{$_} = 1;
  }
}
sub EnableDerivedMethods {
  $deriving = $_[0] // 1;
}
sub Deriving {
  $deriving;
}
sub UseHashObjects {
  $use_hash = $_[0] // 1;
}
sub UsingHashObjects {
  $use_hash;
}
sub MemberTypes {
  my ($name) = @_;
  my $class = $classdef{$name};
  return (undef) unless defined($class);
  my $typehash = {};
  my $classhash = $class->[6];
  foreach my $member (keys %$classhash) {
    $typehash->{$member} = $classhash->{$member}[1];
  }
  return ($typehash, (defined($class->[1]) ? $class->[1][0] : undef));
}
sub KnownClasses {
  keys %classdef;
}
1;
package ObjectClass;
use strict;
sub DESTROY { }
sub o_new {
    bless [], $_[0];
}
sub dup_to {
    my ($class, $obj) = @_;
    my $newobj = bless [], $class;
    if ($newobj->isa(ref($obj))) {
      @$newobj = @$obj;		
    } elsif ($obj->isa($class)) {
      @$newobj = @$obj;		
      my $newsize = $Object::classdef{$class}[2];
      splice @$newobj, $Object::classdef{$class}[2];	
    } else {
      die "ERROR: dup_to: $class is not a subclass or superclass of ".ref($obj);
    }
    $newobj;
}
sub setvalues {
    my ($self, @pairs) = @_;
    my $class = $Object::classdef{ref($self)};
    die "Object.pm: Not a recognized object: ".ref($self).", stopped" unless defined($class);
    my $keys = $class->[6];
    while (@pairs) {
	my $key = shift @pairs;
	my $value = shift @pairs;
	my $keyinfo = $keys->{$key};
	defined($keyinfo) or die "Object.pm: unknown key '$key' for object of package ".ref($self);
	my ($index, $type) = @$keyinfo;
	if (defined($value)) {		
	    if ($type eq "@" and ref($value) ne "ARRAY" and !UNIVERSAL::isa($value,"ArrayObject")) {
		die "Object.pm: setting key '$key' to non-array value, for object of package ".ref($self);
	    }
	    if ($type eq "%" and ref($value) ne "HASH" and !UNIVERSAL::isa($value,"HashObject")) {
		die "Object.pm: setting key '$key' to non-array value, for object of package ".ref($self);
	    }
	}
	$self->[$index] = $value;
    }
}
sub getref {
    my ($self, $key) = @_;
    my $class = $Object::classdef{ref($self)};
    die "Object.pm: getref: Not a recognized object: ".ref($self).", stopped" unless defined($class);
    my $keyinfo = $class->[6]{$key};
    if (!defined($keyinfo)) {
	die "Object.pm: getref: unrecognized key $key of class: ".ref($self).", stopped" unless defined($class);
	return undef;
    }
    \$self->[$keyinfo->[0]];		
}
sub getvalue {
    my ($self, $key) = @_;
    my $ref = $self->getref($key);
    defined($ref) or die "Object.pm: getvalue: unknown key '$key' for object of package ".ref($self);
    $$ref;				
}
sub keys {
    my $self = shift;
    die "Object.pm: Extraneous parameter(s) to sub keys in class ".ref($self) if @_;
    my $class = $Object::classdef{ref($self)};
    die "Object.pm: Not a recognized object: ".ref($self).", stopped" unless defined($class);
    CORE::keys %{$class->[6]};
}
sub members {
    my $self = shift;
    die "Object.pm: Extraneous parameter(s) to sub hash in class ".ref($self) if @_;
    my $class = $Object::classdef{ref($self)};
    die "Object.pm: Not a recognized object: ".ref($self).", stopped" unless defined($class);
    %{$class->[6]};
}
sub hash {
    my $self = shift;
    die "Object.pm: Extraneous parameter(s) to sub hash in class ".ref($self) if @_;
    my $class = $Object::classdef{ref($self)};
    die "Object.pm: Not a recognized object: ".ref($self).", stopped" unless defined($class);
    my $members = $class->[6];
    my %hash = ();
    for my $key (CORE::keys %$members) {
	$hash{$key} = $self->[$members->{$key}[0]];
    }
    %hash;
}
sub hashref {
    my $self = shift;
    my %hash = $self->hash(@_);
    \%hash;
}
sub _err_parm_ {
    my $self = shift;
    my ($package,$filename,$line,$subroutine) = caller(1);
    print STDERR "WARNING: file $filename line $line (package $package): incorrect number of parameters invoking $subroutine\n";	# " on object of package ".ref($self)
}
1;
package HashObject;
use strict;
use Carp;
our $AUTOLOAD;
sub AUTOLOAD {
    my $self = shift;
    my $type = ref($self) or croak "$self is not an object";
    my $name = $AUTOLOAD;
    $name =~ s/.*://;   
    return if $name eq "DESTROY";		
    if (!exists($self->{$name})) {
	if ($name =~ s/^ref_//) {
	    if (@_) {				
		my $ref = shift;
		bless $ref, "ArrayObject" if ref($ref) eq "ARRAY";
		return ($self->{$name} = $ref);
	    }
	    return $self->{$name};		
	}
	if ($name =~ s/^set_//) {
	    @_==1 or croak "Bad invocation of autoload set_$name (need exactly one argument) in class $type";
	    my $ref = shift;
	    bless $ref, "ArrayObject" if ref($ref) eq "ARRAY";
	    return($self->{$name} = $ref);	
	}
	if ($name =~ m/^push_(.*)$/) {
	    my $aname = $1;
	    my $aref = $self->{$aname};
	    if (defined($aref) and UNIVERSAL::isa($aref, "ARRAY")) {
		push @$aref, @_;		
		return scalar(@$aref);		
	    }
	}
	my ($package,$filename,$line) = caller;	
	croak "Undefined subroutine ${name} in class $type"
		."\nIn line $line of file $filename (package $package)\n";
    }
    my $element = $self->{$name};
    if (ref($element) eq "ARRAY") {
	@_==0 and return wantarray ? @$element : $element;
	croak "Accessing array $name (in class $type) using any arguments (".scalar(@_).") is not defined";
    } elsif (UNIVERSAL::isa($element, "ARRAY")) {	# ref($element) eq "ARRAY"
	@_==0 and return $element;		
	croak "Accessing array $name (in class $type) using ".scalar(@_)." arguments is not defined";
    } elsif (ref($element) eq "HASH") {
	@_==0 and return $element;		
	my ($package,$filename,$line) = caller;	
	croak "Accessing hashes not defined, accessing field $name in class $type"
		."\nIn line $line of file $filename (package $package)\n";
    } else {
	$self->{$name} = shift if @_;		
	croak "Extraneous parameter(s), setting field $name in class $type" if @_;
	return $self->{$name};			
    }
}
sub DESTROY { }
sub can {
    my ($self, $name) = @_;
    return 1 if exists($self->{$name});
    return $self->SUPER::can($name);
}
sub keys {
    my $self = shift;
    croak "Extraneous parameter(s) to sub keys in class ".ref($self) if @_;
    CORE::keys %$self;
}
sub hash {
    my $self = shift;
    croak "Extraneous parameter(s) to sub hash in class ".ref($self) if @_;
    %$self;
}
sub hashref {
    my $self = shift;
    croak "Extraneous parameter(s) to sub hash in class ".ref($self) if @_;
    $self;
}
sub empty_obj {
}
sub o_new {
    bless { $_[0]->empty_obj }, $_[0];
}
sub dup_to {
    my ($class, $obj) = @_;
    my $newobj = bless {}, $class;
    $newobj->isa(ref($obj)) or $obj->isa($class)
	or die "ERROR: HashObject dup_to: $class is not a subclass or superclass of ".ref($obj);
    %$newobj = %$obj;		
    $newobj;
}
1;
package ArrayObject;
use strict;
use Carp;
sub new {
    my $class = shift;
    my $self = [];
    bless ($self, $class);	
    return $self;		
}
sub o_new {
    my $class = shift;
    my $self = [];
    bless ($self, $class);	
    return $self;		
}
sub n {
    my $self = shift;
    croak "Extraneous parameter(s) to sub n in class ".ref($self) if @_;
    scalar(@$self);
}
sub a {
    my $self = shift;
    croak "Extraneous parameter(s) to sub n in class ".ref($self) if @_;
    @$self;
}
sub values {
    my $self = shift;
    croak "Extraneous parameter(s) to sub n in class ".ref($self) if @_;
    @$self;
}
sub set {
    my $self = shift;
    @$self = @_;
}
sub push {
    my $self = shift;
    CORE::push @$self, @_;
}
sub pop {
    my $self = shift;
    croak "Extraneous parameter(s) to sub pop in class ".ref($self) if @_;
    CORE::pop @$self;
}
sub unshift {
    my $self = shift;
    CORE::unshift @$self, @_;
}
sub shift {
    my $self = shift;
    croak "Extraneous parameter(s) to sub shift in class ".ref($self) if @_;
    CORE::shift @$self;
}
sub ref {
    my $self = CORE::shift;
    croak "Extraneous parameter(s) to sub ref in class ".CORE::ref($self) if @_;
    $self;
}
1;
