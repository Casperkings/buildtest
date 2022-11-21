# Copyright (c) 2005-2007 by Tensilica Inc.  ALL RIGHTS RESERVED.
package Xtensa::Params;
use Exporter 'import';
@EXPORT = qw(xtensa_params_init xtensa_params_tools_path xtensa_params_config_path);
use strict;
sub xtensa_params_init {
  my ($xtensa_system, $xtensa_core, $xtensa_params) = @_;
  $xtensa_system = $ENV{"XTENSA_SYSTEM"} unless defined($xtensa_system);
  $xtensa_system = [split(/;/, $xtensa_system)] unless ref($xtensa_system) eq "ARRAY";
  my @xtensa_systems = grep($_ ne "", @$xtensa_system);	
  map(s|[\\/]+$||, @xtensa_systems);			
  die "ERROR: Params.pm xtensa_params_init: XTENSA_SYSTEM not defined"
	unless @xtensa_systems;
  $xtensa_core = $ENV{"XTENSA_CORE"} unless defined($xtensa_core) and $xtensa_core ne "";
  my $used_default = 0;
  if (!defined($xtensa_core) or $xtensa_core eq "") {
    $xtensa_core = "default";
    $used_default = 1;
  }
  $xtensa_params = $ENV{"XTENSA_PARAMS"} unless defined($xtensa_params);
  $xtensa_params = [] unless defined($xtensa_params);
  $xtensa_params = [split(/;/, $xtensa_params)] unless ref($xtensa_params) eq "ARRAY";
  my $parmfile;
  foreach my $path (@xtensa_systems) {
    $parmfile = $path."/".$xtensa_core."-params";
    last if open(IN, "<$parmfile");
    undef $parmfile;
  }
  die "ERROR: Params.pm xtensa_params_init: could not find core $xtensa_core (file ${xtensa_core}-params) in:\n"
		.join("", map("   $_\n", @xtensa_systems))."Stopped"
	unless defined($parmfile);
  my %parms = ();
  my $line = 0;
  while (<IN>) {
    $line++;
    s/^\s+//;		
    if (!s/^([-a-zA-Z0-9_\.]+)//) {
      print STDERR "WARNING: Params.pm xtensa_params_init: Unexpected char in line $line of $parmfile: $_\n" if /^[^\#]/;
      next;
    }
    my $key = $1;
    if (!s/^\s*=//) {
      print STDERR "WARNING: Params.pm xtensa_params_init: missing '=' in line $line of $parmfile\n";
      next;
    }
    if (s/^\s*\[//) {
      my @values = ();
      while (!s/^\s*\]//) {
	my $value = parseParamsValue(\$_);
	if (defined($value)) {
	  push @values, $value;
	} elsif (/^[^\#]/) {
	  print STDERR "WARNING: Params.pm xtensa_params_init: unexpected char in line $line of $parmfile: $_\n";
	  last;
	} else {
	  $line++;
	  if (!defined($_ = <IN>)) {
	    print STDERR "WARNING: Params.pm xtensa_params_init: unexpected end of file within array of key '$key' in file $parmfile\n";
	    last;
	  }
	}
      }
      $parms{$key} = \@values;
    } else {
      my $value = parseParamsValue(\$_);
      $value = "" unless defined($value);
      $parms{$key} = $value;
    }
    s/^\s+//;		
    if (/^[^\#]/) {
      print STDERR "WARNING: Params.pm xtensa_params_init: extraneous characters at end of line $line of $parmfile: $_\n";
    }
  }
  close(IN);
  my $config_name = $parms{"ConfigName"};
  $xtensa_core = $config_name if $used_default and defined($config_name) and $config_name ne "";
  $parms{"xtensa-core"} = $xtensa_core;		
  $parms{"xtensa-system"} = \@xtensa_systems;	
  $parms{"xtensa-params"} = $xtensa_params;	
  return %parms;
}
sub xtensa_params_tools_path {
  my ($params) = @_;
  my $install_prefix = $params->{'install-prefix'};
  my $xtensa_tools_path = $ENV{'XTENSA_TOOLS_PATH'};
  if (defined($xtensa_tools_path)) {
    $install_prefix = $xtensa_tools_path;
  }
  return $install_prefix;
}
sub xtensa_params_config_path {
  my ($params) = @_;
  my $config_prefix = $params->{'config-prefix'};
  my $xtensa_config_path = $ENV{'XTENSA_CONFIG_PATH'};
  if (defined($xtensa_config_path)) {
    $config_prefix = $xtensa_config_path;
  }
  my $xtensa_builds_path = $ENV{'XTENSA_BUILDS_PATH'};
  if (defined($xtensa_builds_path)) {
    my $corename = $params->{"xtensa-core"};
    my $configname = $params->{"ConfigName"};
    if (defined($corename) and $config_prefix =~ s|^(.*)/$corename$|$xtensa_builds_path|e ) {
    } elsif (defined($configname) and $config_prefix =~ s|^(.*)/$configname$|$xtensa_builds_path|e ) {
    } else {
      print STDERR "WARNING: Params.pm xtensa_params_config_path: unable to apply XTENSA_BUILDS_PATH\n";
    }
  }
  return $config_prefix;
}
sub parseParamsValue {
  my ($ref) = @_;
  $$ref =~ s/^\s+//;
  return $1 if $$ref =~ s/^'([^']*)'//;
  return $1 if $$ref =~ s/^"([^"]*)"//;
  return undef unless $$ref =~ s/^([^\x00-\x20\'\"\#\]\x7f-\xff]+)//;
  my $value = $1;
  $value = ($value =~ /^0/ ? oct($value) : 0+$value) if $value =~ /^(0x[0-9a-f]+|0[0-7]*|[1-9][0-9]*)$/i;
  return $value;
}
1;
