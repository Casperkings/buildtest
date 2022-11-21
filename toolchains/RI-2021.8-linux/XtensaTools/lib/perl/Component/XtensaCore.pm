# Copyright (c) 2018 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
package Component::XtensaConfig;
our $VERSION = "1.0";
our @ISA = qw(Component::Object);
use strict;
use Component;
sub new {
    my ($class, $layout, $name, $xtensa_system, $params) = @_;
    if (defined($layout)) {
	my $config = $layout->core_configs->{"\L$name"};
	if (defined($config)) {
	    return $config if $config->name eq $name;
	    Error("multiple core configs with names differing only by case:  ".$config->name.", $name");
	}
	if (!defined($xtensa_system)) {
	    $xtensa_system = $layout->xtensa_system;
	    if (!defined($xtensa_system) and !defined($params)) {
		Error("no XTENSA_SYSTEM specified for layout, cannot lookup core config $name");
	    }
	}
    } elsif (!defined($xtensa_system) and !defined($params)) {
	Error("xtensa_system missing, cannot lookup core config $name");
    }
    use Xtensa::Params;
    $params //= { xtensa_params_init($xtensa_system, $name) };
    my $xtensa_root = xtensa_params_config_path($params) or Error("missing config-prefix for core config $name");
    my $config = $class->SUPER::new(undef, $name);
    $config->{layout} = $layout;		
    $config->{xtensa_root} = $xtensa_root;	
    $config->{default_sys} = undef;		
    $config->{pr} = undef;			
    bless ($config, $class);	
    $layout->core_configs->{"\L$name"} = $config if defined($layout);
    return $config;		
}
sub new_internal {
    my ($class, $layout, $name, $sys, $pr, $xtensa_root) = @_;
    if (defined($layout)) {
	my $config = $layout->core_configs->{"\L$name"};
	if (defined($config)) {
	    return $config if $config->name eq $name;
	    Error("multiple core configs with names differing only by case:  ".$config->name.", $name");
	}
    }
    my $config = $class->SUPER::new(undef, $name);
    $config->{layout} = $layout;		
    $config->{default_sys} = $sys;		
    $config->{pr} = $pr;			
    $config->{xtensa_root} = $xtensa_root;	
    bless ($config, $class);	
    $layout->core_configs->{"\L$name"} = $config if defined($layout);
    return $config;		
}
sub load_config {
    my ($config) = @_;
    return if defined($config->{config_hash});
    use Xtensa::YAML;
    use Object;
    my $usehash = Object::UsingHashObjects() // 1;
    my $path_yaml_config = $config->xtensa_root."/config/".($usehash ? "core.yml" : "core.p.yml");
    my $config_yaml = Xtensa::AddressLayout::readfile($path_yaml_config, "core config ".$config->name);
    my $debug_yaml = 0;
    my $debug_autoload = 0;
    my $load_code = 0;
    my $hash = YAML_Load_objects($config_yaml, 'semiload', ['Xtensa::ProcessorModule', 'ProcessorModule'],
				    $load_code, $debug_yaml, $debug_autoload);
    my $default_sys = $hash->{sys};
    my $pr = $default_sys->componentInstances("xtb.pr");
    defined($default_sys) and defined($pr) or Error("config ".$config->name.": no config info from $path_yaml_config");
    $config->{config_hash} = $hash;
    $config->{default_sys} = $default_sys;
    $config->{pr} = $pr;
}
sub pr {
    my ($config) = @_;
    my $pr = $config->{pr};
    return $pr if defined($pr);
    $config->load_config;
    return $config->{pr};
}
sub default_sys {
    my ($config) = @_;
    my $default_sys = $config->{default_sys};
    return $default_sys if defined($default_sys);
    $config->load_config;
    return $config->{default_sys};
}
sub cleanup {
    my ($config) = @_;
    return unless defined($config->{config_hash});
    undef $config->{pr};
    undef $config->{default_sys};
    undef $config->{config_hash};
}
1;
package Component::XtensaCore;
our $VERSION = "1.0";
our @ISA = qw(Component::Instance);
use strict;
use Component;
sub new {
    my ($class, $layout, $name, $index, $config, $swoptions) = @_;
    if (defined($layout)) {
	if (!defined($index)) {
	    $index = $layout->find_core_index;
	} else {
	    $index = Xtensa::AddressLayout::strtoint($index, "index of core $name");
	    $index >= 0 && $index <= 0xFFFF or Error("core $name index $index outside PRID range 0..0xFFFF");
	    my ($match) = grep($_->index == $index, $layout->cores);
	    Error("duplicate index $index for cores ".$match->name." and $name")
		if defined($match);
	}
    }
    my $pr = $config->pr;
    $swoptions = {} unless defined($swoptions);
    if ($pr->vectors->relocatableVectorOption) {
	$swoptions->{vecselect}       //= $pr->vectors->SW_stationaryVectorBaseSelect;
	$swoptions->{vecbase}         //= $pr->vectors->relocatableVectorBaseResetVal;
	$swoptions->{vecreset_pins}   //= $pr->vectors->stationaryVectorBase1;
	$swoptions->{static_vecbase0} //= $pr->vectors->stationaryVectorBase0;
	$swoptions->{static_vecbase1} //= $pr->vectors->stationaryVectorBase1;
    } else {
	$swoptions->{vecselect}       = undef;
	$swoptions->{vecbase}         = undef;
	$swoptions->{vecreset_pins}   = undef;
	$swoptions->{static_vecbase0} = undef;
	$swoptions->{static_vecbase1} = undef;
    }
    my $core = $class->SUPER::new(undef, $name);
    $core->{layout} = $layout;		
    $core->{index} = $index;		
    $core->{config} = $config;		
    $core->{swoptions} = $swoptions;	
    bless ($core, $class);	
    $layout->add_core($core) if defined($layout);
    return $core;		
}
sub cleanup {
    my ($core) = @_;
    $core->config->cleanup;
}
sub type_family_lookup {
    my ($class, $system, $type, $where) = @_;
    my $config_name = $type->{config_name}
	or die "ERROR: $where: expected config_name parameter not specified";
    my $core_type_name = "XtensaCore_$config_name";
    my $diag_core_type_name = "XtensaCoreDiag_$config_name";
    my $core_type = $system->types->{$core_type_name};
    return $core_type if defined($core_type);
    my $config_type_name = "XtensaConfig_$config_name";
    my $config = $system->types->{$config_type_name};
    if (!defined($config)) {
	$config = Component::XtensaConfig->new($system->{layout}, $config_name, $system->xtensa_system);
print STDERR "Loaded config $config_name into type $config_type_name\n";
	$system->types->{$config_type_name} = $config;
    }
    my $sys = Component::read_xld_raw($config->xtensa_root . "/lib/components/xtensa-core/config-type.xld");
    my $types = $sys->{types} // {};
    $core_type = $types->{XtensaConfiguredCore};
    bless($core_type, "Component::XtensaCore");		# TODO: use "Component::$core_type_name" instead and create package/class for it?
    $system->types->{$core_type_name} = $core_type;
    $core_type->{parent} = $system->types;
    $core_type->{fullname} = $system->name . ".types." . $core_type_name;
    my $diag_core_type = $types->{XtensaDiagConfiguredCore};
    bless($diag_core_type, "Component::XtensaCore");	
    $system->types->{$diag_core_type_name} = $diag_core_type;
    $diag_core_type->{parent} = $system->types;
    $diag_core_type->{fullname} = $system->name . ".types." . $diag_core_type_name;
    return $core_type;
}
sub instance_class {
    my ($core) = @_;
    return "Component::XtensaCore";	
}
1;
