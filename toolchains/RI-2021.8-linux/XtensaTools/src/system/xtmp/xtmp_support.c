// xtmp_support.c -- Command-line parsing and other functions for generated XTMP models

// Copyright (c) 2006 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iss/mp.h"


typedef struct core_options {
  char** argv;		/* core-specific options not already parsed (for post-processing) */
  int	print_summary;
} core_options;

static char*		progname;
static int		synchro;
static int		dump_info;
static long		cycle_limit;
static int		num_cores;
static core_options *	c_options;
static XTMP_core *	cores;
static const char **	c_configs;
static const char **	c_names;

extern void display_version(void);


/*
 *  Simplify memory allocation, just exit on any error:
 */
static void* malloc_check(size_t len)
{
  void *p = malloc(len);
  if (p)
    return p;
  fprintf (stderr, "XTMP simulation ran out of memory during initialization (%d bytes).\n", len);
  exit (1);
}


/*
 *  Takes an argc/argv argument list, and expands any references
 *  to files containing options (eg. @file).
 *  Returns 1 on success (*pargc and *pargv are modified if expanded), 0 on error.
 */
static int expand_argv(int *pargc, char ***pargv)
{
  int argc = *pargc;	/* total number of options so far */
  int newc = argc;	/* number array entries allocated in newv[] (-1) */
  int newi = 1;		/* index of next entry to add in newv[] (skip argv[0]) */
  char **argv = *pargv;	/* next option to read from original array */
  char **newv = argv;	/* new expanded array */
  char buf[10000];	/* big enough for longest option allowed (allow really long!) */
  FILE *in;

  for (argv++; *argv; argv++) {
    if (**argv != '@') {
      newv[newi++] = *argv;
      continue;
    }
    in = fopen (1+*argv, "r");
    if (in == 0) {
      fprintf (stderr, "%s: @file option: error opening '%s' for reading", **pargv, 1+*argv);
      perror (": ");
      return 0;				/* error */
    }
    while (1) {
      if (argc >= newc) {		/* need more array space? */
	char **growv;
	newc = newc*2 + 10;		/* grow quickly */
	growv = (char**) malloc_check ((newc+1) * sizeof(char*));	/* +1 for last NULL */
	memcpy ((void*)growv, (void*)newv, newi * sizeof(char*));
	if (newv != *pargv)		/* can't free original array */
	  free (newv);
	newv = growv;
      }
      if (fgets(buf, sizeof(buf), in) == 0)	/* read one line == one option */
	break;
      if (buf[0] != 0 && buf[strlen(buf)-1] != '\n') {
	fprintf (stderr, "%s: @file option: line too long (%d max) in file '%s'\n",
		**pargv, sizeof(buf)-1, 1+*argv);
	return 0;			/* error */
      }
      newv[newi] = (char*) malloc_check (strlen(buf)+1);
      strcpy (newv[newi++], buf);
      argc++;				/* total grows by one option */
    }
    fclose (in);
  }
  if (newv != *pargv) {			/* did we grow the options array? */
    newv[newi] = 0;			/* terminate it */
    *pargc = argc;
    *pargv = newv;
  }
  return 1;				/* all is okay */
}


/*
 *  Given an argc/argv argument list, construct the argument list that
 *  applies to the specified processor.  Both the full name and the index
 *  of the processor must be provided (both are accepted by the --core option).
 *  The returned array does *not* contain the initial argv[0] entry (program name).
 *  Returns pointer to core-specific argument array, or 0 on error.
 *  NOTE:  --core options that specify a non-existent core are simply ignored.
 */
static char** core_argv(int argc, char **argv, const char *corename, int coreindex)
{
  char **corev = (char**) malloc_check (argc * sizeof (char*));
  char **av = corev;
  int index, selected = 1;		/* default to all-cores-selected */

  for (argv++; argc > 1; argc--) {
    char *opt = *argv++;
    if (strncmp(opt, "--core=", 7) == 0) {
      opt += 7;
      selected = 0;
      while (*opt) {			/* look at all comma-separated core names/indices */
	char *id = opt, *idend;
	if ((opt = strchr(opt, ',')) != 0)
	  *opt++ = 0;			/* terminate id, go to next name/index */
	else
	  opt = "";
	/*  Convert id to core index:  */
	index = (int) strtoul(id, &idend, 0);
	if (idend != id && *idend == 0) {	/* valid integer? */
	  if (index < 0 || index >= num_cores) {
	    fprintf(stderr, "%s: Error: core index %d out of range 0 .. %d\n",
	    	progname, index, num_cores-1);
	    return 0;
	  }
	} else if (strcmp(id, ".") == 0) {
	  selected = 1;			/* "." selects all cores */
	} else {			/* else must be a name... */
	  for (index = 0; index < num_cores; index++)
	    if (strcmp(id, c_names[index]) == 0)
	      break;
	  if (index >= num_cores) {
	    fprintf(stderr, "%s: Error: core name '%s' not recognized; valid names are:\n",
	    	progname, id);
	    for (index = 0; index < num_cores; index++)
	      fprintf(stderr, "        %s\n", c_names[index]);
	    return 0;
	  }
	}
	/*  Compare id against corename and coreindex:  */
	if (index == coreindex)
	  selected = 1;
	/* (future: allow processor group names) */
#if 0
	if (corename != 0 && strcmp(id, corename) == 0)
	  selected = 1;			/* core name match */
	if (coreindex >= 0 && index == coreindex)
	  selected = 1;			/* core index match */
#endif
      }
    } else if (selected)
      *av++ = opt;
  }
  *av = 0;				/* terminate array */
  return corev;
}


/*  For debugging only:  */
#ifdef DEBUG
static void print_argv(char *what, int argc, char **argv)
{
  printf("%s (%2d): ", what, argc);
  while (*argv)
    printf(" '%s'", *argv++);
  printf("\n");
}
#endif


/*
 *  Given an argument list (eg. returned by core_argv()), create an argument
 *  list for those arguments that XTMP_coreNew() can handle, and remove them
 *  from the given list.
 */
static char** iss_argv(char **argv)
{
  int n;
  char *opt, **av, **issv, **iv;

  for (n = 0, av = argv; *av++; n++)
    ;
  issv = (char**) malloc_check ((n+1) * sizeof (char*));
# define MATCH_PREFIX(opt,str)	((strncmp(opt, str, sizeof(str)-1) == 0) ? ((opt += sizeof(str)-1), 1) : 0)
# define MATCH_STRING(opt,str)	(strcmp(opt, str) == 0)
  for (av = argv, iv = issv; (opt = *av) != 0; n--) {
    if (MATCH_PREFIX(opt,"--dcline=")
     || MATCH_PREFIX(opt,"--dcsize=")
     || MATCH_PREFIX(opt,"--dcways=")
     || MATCH_PREFIX(opt,"--icline=")
     || MATCH_PREFIX(opt,"--icsize=")
     || MATCH_PREFIX(opt,"--icways=")
     || MATCH_STRING(opt,"--no_debug_flush")
     || MATCH_STRING(opt,"--no_zero_bss")
     || MATCH_PREFIX(opt,"--proc_id=")		/* FIXME: get default from system description */	/* NOTE: one cpu only */
     || MATCH_STRING(opt,"--reference")
     || MATCH_PREFIX(opt,"--target_input=")	/* NOTE: one cpu only? */
     || MATCH_PREFIX(opt,"--target_output=")	/* NOTE: can multiple cpus share? */
     || MATCH_PREFIX(opt,"--wbsize=")
    ) {
      *iv++ = opt;			/* it's an ISS option */
      memmove (av, av+1, n * sizeof(char*));
    } else {
      av++;				/* it's another option */
    }
  }
  *iv = 0;
  return issv;
}


static void usage(void)
{
  printf("Usage:\n"
	"   %s [options | @filepath]*\n"
	"\n"
	"Occurrences of @filepath in the list of options are replaced (non-recursively)\n"
	"with the contents of the file, one option per line (spaces and all).\n"
	"\n"
	"Options are either global or core-specific.  Global options are usually\n"
	"given at the beginning, but may be located *anywhere* (subject to change).\n"
	"Core-specific options can be different for each core.  By default, they\n"
	"apply to all cores.  After a --core=<id>[,<id>]* option, they apply only\n"
	"to the core(s) listed by that option.  Each <id> is either:\n"
	"   - the numerical index of a core, or\n"
	"   - the full hierarchical instance name of the core in the system\n"
	"     description (minus the top-level system component name), or\n"
	"   - a period (.) which refers to all cores in the system.\n"
	/*"   - (support for processor group names may be added in the future)\n"*/
	"\n"
	"Global options include the following:\n"
	"	--help			Display this message and exit.\n"
	"	--version		Display generated model version info and exit.\n"
	"	--cycle_limit=n		Limit simulation length to n cycles\n"
	"	--dump_init_file	Dump info about the simulation to stdout (for XX)\n"
	"	--debug_sync		See XTMP_setSynchronizedSimulation()\n"
	"\n"
	"Core-specific options include the following:\n"
	"	--xtensa-system=dir	Specify path to registry of cores\n"
	"	--xtensa-params=dir	Specify path to TDK (TIE dev kit)\n"
	"	--dcline=n		(See `xt-run --help`)\n"
	"	--dcsize=n		(See `xt-run --help`)\n"
	"	--dcways=n		(See `xt-run --help`)\n"
	"	--icline=n		(See `xt-run --help`)\n"
	"	--icsize=n		(See `xt-run --help`)\n"
	"	--icways=n		(See `xt-run --help`)\n"
	"	--wbsize=n		(See `xt-run --help`)\n"
	"	--no_debug_flush	(See `xt-run --help`)\n"
	"	--no_zero_bss		(See `xt-run --help`)\n"
	"	--reference		(See `xt-run --help`)\n"
	"	--load=file		(See `xt-run --help`)\n"
	"	--loadbin=file@addr	(See `xt-run --help`)\n"
	"	--client_cmds='...'	(See `xt-run --help`)\n"
	"	--trace=n		(See `xt-run --help`)\n"
	"	--trace_file=file	(See `xt-run --help`)\n"
	"	--profile=file		(See `xt-run --help`)\n"
	"	--debugwait		Wait for debugger connection before starting core\n"
	"	--nodebugwait		Do not wait for debugger (this is the default)\n"
	"	--debug_poll_cycles=n	Poll interval, if --debug_sync not selected\n"
	"	--summary		Display execution statistics at end of simulation\n"
	"The following core-specific options must be provided separately for each core,\n"
	"because two cores cannot share the same value:\n"
	"	--proc_id=n		Set PRID value;	FIXME: get default from system description\n"
	"	--socket=n		(See `xt-run --help --internal`)\n"
	"(Not sure whether these need to be separate:)\n"
	"	--target_input=file	Redirect stdin from file\n"
	"	--target_output=file	Redirect stdout (and stderr?) to file\n"
	,progname);
}




/*
 *  Instantiate an Xtensa processor core.
 */
static XTMP_core instantiate_core(int argc, char **argv, core_options *opts,
			   const char *configname, const char *corename, int coreindex)
{
  char **corev, **issv, **av, **pv, *opt, *xtensa_system = 0, *tdkdirs[2] = {0,0}, buf[BUFSIZ+100];
  XTMP_params params;
  XTMP_core core;

  opts->print_summary = 0;
  opts->argv = corev = core_argv(argc, argv, corename, coreindex);
  if (corev == 0)
    return 0;
  issv  = iss_argv(corev);

  /*
   *  Look for, and strip out, a few options that are needed before instantiating the core:
   */
  for (av = pv = corev; (opt = *av++) != 0; ) {
    if (MATCH_PREFIX(opt, "--xtensa-system="))
      /* FIXME: handle multiple registry directories. */
      xtensa_system = opt;
    else if (MATCH_PREFIX(opt, "--xtensa-params="))
      tdkdirs[0] = opt;
    /*else if (MATCH_PREFIX(opt, "--xtensa-core="))
      configname = opt;		??? */
    else
      *pv++ = opt;
  }
  *pv = 0;

  /*
   *  Get parameters, and instantiate the core:
   */
  if (xtensa_system) {
    char *sysdirs[2] = {xtensa_system, 0};
    params = XTMP_paramsNewFromPath(sysdirs, configname, tdkdirs);
  } else {
    params = XTMP_paramsNew(configname, tdkdirs);
  }
  if (params == 0) {
    fprintf(stderr, "%s: Error opening parameter file for core '%s', config '%s' in registry %s\n",
		argv[0], corename, configname, xtensa_system ? xtensa_system : "(default)");
    return 0;
  }
  core = XTMP_coreNew(corename, params, issv);
  if (core == 0) {
    fprintf(stderr, "%s: Error creating core '%s', config '%s' in registry %s\n",
		argv[0], corename, configname, xtensa_system ? xtensa_system : "(default)");
    return 0;
  }

  /*
   *  Parse remaining parameters.
   *  Some parameters must be processed *after* the entire system has been
   *  instantiated and connected (e.g. loading a program requires memories
   *  to be present, etc).  We keep these, and strip out those we've processed:
   */
  for (av = pv = corev; (opt = *av++) != 0; ) {
    char *opt2 = opt;
    if (MATCH_PREFIX(opt2, "--load=")
      || MATCH_PREFIX(opt2, "--loadbin=")) {
      *pv++ = opt;		/* save for postprocessing */
    } else if (opt[0] != '-') {	/* not an option? must be target program name and arguments */
      do { *pv++ = opt; } while((opt = *av++) != 0);
      break;
    } else if (MATCH_PREFIX(opt, "--client_commands=")) {
      XTMP_clientLoad(core, opt);
    } else if (MATCH_PREFIX(opt, "--client_cmds=")) {
      XTMP_clientLoad(core, opt);
    } else if (MATCH_PREFIX(opt, "--trace=")) {
      XTMP_setTraceLevel(core, atoi(opt));	/* FIXME: add error checking */
    } else if (MATCH_PREFIX(opt, "--trace_file=")) {
      XTMP_setTraceFile(core, opt);		/* FIXME: add error checking */
    } else if (MATCH_PREFIX(opt, "--profile=")) {
      if (strlen(opt) > BUFSIZ) {
	fprintf(stderr, "%s: --profile filename too long (%s)\n", argv[0], opt);
	return 0;
      }
      sprintf(buf, "profile %s", opt);
      XTMP_clientLoad(core, buf);
    } else if (MATCH_PREFIX(opt, "--socket=")) {
      XTMP_enableDebug(core, atoi(opt));	/* FIXME: add error checking */
    } else if (MATCH_STRING(opt, "--debugwait")) {
      XTMP_setWaitForDebugger(core, true);
    } else if (MATCH_STRING(opt, "--nodebugwait")) {
      XTMP_setWaitForDebugger(core, false);
    } else if (MATCH_PREFIX(opt, "--debug_poll_cycles=")) {
      XTMP_setDebugPollInterval(core, atoi(opt)); /* FIXME: add error checking */
#if 0	/* FIXME: these apply to memories, not cores */
    } else if (MATCH_PREFIX(opt, "--read_delay=")) {
      XTMP_setReadDelay(memory, atoi(opt));	/* FIXME: add error checking */
    } else if (MATCH_PREFIX(opt, "--write_delay=")) {
      XTMP_setWriteDelay(memory, atoi(opt));	/* FIXME: add error checking */
    } else if (MATCH_PREFIX(opt, "--read_repeat=")) {
      XTMP_setBlockReadDelays(memory, ...last delay..., atoi(opt));	/* FIXME: add error checking */
    } else if (MATCH_PREFIX(opt, "--write_repeat=")) {
      XTMP_setBlockWriteDelays(memory, ...last delay..., atoi(opt)); /* FIXME: add error checking */
#endif /*0*/
    } else if (MATCH_STRING(opt, "--summary")) {
      opts->print_summary = 1;	/* FIXME: do something with this... */
    } else {
      fprintf(stderr,   "%s: Unrecognized option '%s'\n"
			"For usage details, invoke as:  %s --help\n", argv[0], opt, argv[0]);
      return 0;
    }
  }
  *pv = 0;

  return core;
}



static int postprocess_core (XTMP_core core, core_options *opts)
{
  char *opt, **av = opts->argv;
  char buf[BUFSIZ+100];

  while ((opt = *av++) != 0) {
    /* FIXME: do this *after* memories are created:  */
    if (MATCH_PREFIX(opt, "--load=")) {
      if (!XTMP_loadProgram(core, opt, 0)) {
	fprintf(stderr, "%s: Error loading ELF image '%s'\n", progname, opt);
	return 0;
      }
    } else if (MATCH_PREFIX(opt, "--loadbin=")) {
      /* FIXME: do this *after* memories are created:  */
      if (strlen(opt) > BUFSIZ) {
	fprintf(stderr, "%s: --loadbin argument too long (%s)\n", progname, opt);
	return 0;
      }
      sprintf(buf, "loadbin %s", opt);
      XTMP_clientLoad(core, buf);		/* opt is file@address */
    } else if (opt[0] != '-') {		/* not an option? must be target program name and arguments */
      if (!XTMP_loadProgram(core, opt, av)) {
	fprintf(stderr, "%s: Error loading target ELF image '%s'\n", progname, opt);
	return 0;
      }
      break;		/* terminate option processing */
    } else {
      fprintf(stderr, "%s: Internal error, unrecognized option '%s'\n", progname, opt);
      return 0;
    }
  }
  return 1;
}



/*
 *  Process options that apply to the entire simulation,
 *  i.e. not to any specific core or component.
 */
static int preprocess_generic (int *pargc, char **argv)
{
  char **av, **pv, *opt;

  progname = *argv++;
  synchro = 0;
  dump_info = 0;
  cycle_limit = -1;
  for (av = pv = argv; (opt = *av++) != 0; ) {
    *pargc--;
    if (MATCH_STRING(opt, "--help")) {
      usage();
      return 0;
    } else if (MATCH_STRING(opt, "--version")) {
      display_version();		/* generated code */
      return 0;
    } else if (MATCH_PREFIX(opt, "--cycle_limit=")) {
      cycle_limit = atol(opt);
    } else if (MATCH_STRING(opt, "--dump_init_file")) {
      dump_info = 1;
    } else if (MATCH_STRING(opt, "--debug_sync")) {
      synchro = 1;
    } else {
      *pv++ = opt;
      *pargc++;
    }
  }
  *pv = 0;
  return -1;
}


/*
 *  The generated XTMP model calls this at the beginning of XTMP_main():
 */
int main_start(int *pargc, char ***pargv, int n_cores, const char **core_configs, const char **core_names)
{
  int i, result;

  num_cores = n_cores;
  c_configs = core_configs;
  c_names = core_names;

  c_options = (core_options*) malloc_check (num_cores * sizeof(core_options));
  cores = (XTMP_core*) malloc_check (num_cores * sizeof(XTMP_core));

  /*  Parse general options:  */
  if (!expand_argv (pargc, pargv))
    return 1;
  if ((result = preprocess_generic (pargc, *pargv)) >= 0)
    return result;

  /*  Instantiate cores, and parse core-specific options:  */
  for (i = 0; i < num_cores; i++)
    if ((cores[i] = instantiate_core (*pargc, *pargv, &c_options[i], core_configs[i], core_names[i], i)) == 0)
      return 1;

  return -1;
}

/*
 *  The generated XTMP model calls this at the end of XTMP_main():
 */
int main_end (void)
{
  int i;

  /*  Post-process options (e.g. loading programs requires a fully connected system):  */
  for (i = 0; i < num_cores; i++)
    if (!postprocess_core (cores[i], &c_options[i]))
      return 1;

  if (synchro)
    XTMP_setSynchronizedSimulation(1);

  if (dump_info) {
    /* this call prints information required for the Xplorer debugger to connect */
    printf( "%s\n", XTMP_getCoreInformation() );
    fflush( stdout );
  }

  XTMP_start(cycle_limit);

  /*  Do we ever get here?:  */
  for (i = 0; i < num_cores; i++)
    if (c_options[i].print_summary)
      printf ("\n*** CORE %d (%s) ***\n%s\n", i, c_names[i], XTMP_getSummary(cores[i]));

  /* Must return to main, which does cleanup */
  return 0;
}






#if 1
/* int main(int argc, char **argv) { return XTMP_main (argc, argv); } */
#else
int main(int argc, char **argv)
{
  char **testv, **issv;

  print_argv ("original", argc, argv);
  if (!expand_argv (&argc, &argv))
    return 1;
  print_argv ("expanded", argc, argv);
  testv = core_argv (argc, argv, "PA", 2);
  print_argv ("cpu PA/2", -1, testv);
  issv = iss_argv (testv);
  print_argv ("iss opts", -1, issv);
  print_argv ("remain'g", -1, testv);
  return 0;
}
#endif


