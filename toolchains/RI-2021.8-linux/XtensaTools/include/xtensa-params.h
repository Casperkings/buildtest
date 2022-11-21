//depot/dev/Eagleridge/Xtensa/Software/xtparams/xtensa-params.h#2 - integrate change 193572 (text+ko)
/* Interface to the xtparams library for reading Xtensa parameter files.  */

/* Copyright (c) 2003-2013 Tensilica Inc.

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#ifndef XTENSA_PARAMS_H
#define XTENSA_PARAMS_H

#ifdef __cplusplus
extern "C" {
#endif

/* This header file presents an API used to access Xtensa processor
   configuration information, as well as software installation parameters such
   as installation file paths, etc.

   The API is initialized by calling xtensa_params_init() to load the
   parameter file information.  The result is an xtensa_params opaque value
   that can subsequently be used to retrieve configuration information with
   the xtensa_find_key() function.  The API also provides a few related
   utility routines to list parameter files in an Xtensa registry, convert
   parameter-file-relative paths into absolute paths, retrieve error
   information, etc.

   There are also several convenience functions intended for use in
   command-line programs.  The xtensa_getopt() function is intended to be
   called from beginning of the main program.  It scans the command-line for
   options to specify the parameter file(s) to be read and checks for the same
   information in corresponding environment variables.  It then calls
   xtensa_params_init() to load the information.  The xtensa_string_value(),
   xtensa_int_value() and xtensa_path_value() functions are also intended as a
   convenience for command-line programs.  All of these command-line oriented
   functions will write to stderr and exit if there is an error, so they
   should never be used from GUI applications where more careful error
   handling is required.

   Configuration information is organized as key/value pairs.  The exact full
   list of keys supported is not described in this header file, and may vary
   slightly from one release to the next.  Perusal of an actual parameter file
   may provide some clues (e.g., <configroot>/config/default-params).


   Format of the parameter file:

   param1 = value1
   param2 = value2
   param3 = [value1 value2 value3]
         ...
   paramN = valueN


   Whitespace is not significant. Comments begin with '#' and continue
   to the end of the line.  Parameter names are case-sensitive and
   consist of alphanumeric characters in addition to [._-].  Values
   are strings delimited by whitespace; no special characters are
   currently allowed (i.e., backslash escapes do not work).  Values
   may optionally be enclosed in either single or double quotes,
   which are stripped off when they are read, to allow values containing
   whitespace.  Each entry must be on one line, with the exception of
   bracketed lists of values where only the opening bracket must be on
   the same line as the parameter name.

   Aside from the parameters set explicitly in the parameter file(s),
   there are currently three "magic" parameters:

	xtensa-system:	the XTENSA_SYSTEM directory list (multiple strings)
	xtensa-core:	the XTENSA_CORE name (string)
	xtensa-params:	the XTENSA_PARAMS file names (multiple strings)

   These parameters are set automatically based on the values of the
   environment variables or the corresponding command-line overrides.
   The "xtensa-params" parameter may not be present.  The xtensa_params_init
   function will automatically export the environment variables, but
   these parameters may be needed to invoke a subprocess with the appropriate
   command-line overrides when dealing with multiple cores at the same time.
*/


/* Make an opaque "xtensa_params" type to be used as a handle to the
   internal data structures.  */

typedef struct xtensa_params_opaque { int unused; } *xtensa_params;


/* Error codes.  For any result other than xtparams_ok, an error message
   can be retrieved using the xtensa_params_error_msg function.  */

typedef enum xtparams_status_enum {
  xtparams_ok = 0,
  xtparams_no_default_xtensa_system,
  xtparams_no_xtensa_system,
  xtparams_no_default_core,
  xtparams_no_core,
  xtparams_no_home_directory,
  xtparams_home_not_supported,
  xtparams_cannot_open_paramfile,
  xtparams_parse_error,
  xtparams_end_of_file,
  xtparams_out_of_memory,
  xtparams_internal_error,
  xtparams_unexpected_value
} xtparams_status;

extern xtparams_status
xtensa_params_errno (void);

extern char *
xtensa_params_error_msg (void);


/* Given a semicolon-separated list of file/directory names, such as the
   value of the XTENSA_PARAMS or XTENSA_SYSTEM environment variable, return
   a null-terminated array of those file names.  The input string is
   modified in the process so it should not be shared with other code.  */

extern char **
xtensa_split_filelist (char *filelist);

extern void
xtensa_free_filelist (char **files);


/* The xtensa_getopt function initializes this parameters API.
   Alternatively, the lower-level xtensa_params_init function may be called
   instead.  The xtensa_getopt function is designed to be called with the
   argc/argv options given to main.  It checks for the "--xtensa-system",
   "--xtensa-core" and "--xtensa-params" options on the command line.  If
   found, these options are stripped out by modifying argc/argv so that the
   calling program need not be aware of them.  If the system directories
   are not specified on the command line, they are found by calling
   xtensa_get_system_dirs (see below).  If not specified on the command
   line, the core name and TDK parameter file name(s) are found in the
   environment (XTENSA_CORE and XTENSA_PARAMS, respectively).  Note that
   the "--xtensa-params" and "--xtensa-system" options may be repeated, and
   XTENSA_PARAMS and XTENSA_SYSTEM may contain semicolon-separated lists of
   files/directories, so there can be multiple TDK parameter files and
   system directories.  The return value of xtensa_getopt is found by
   calling xtensa_params_init to read the parameter files.  The return
   value is NULL if any errors occur while reading the files.  Since this
   function is presumably only used for command-line-oriented programs, it
   prints error messages to stderr.  */

extern xtensa_params
xtensa_getopt (int *argc_p, char **argv);


/* The xtensa_quiet_getopt function does exactly the same thing as
   xtensa_getopt but does not print an error message for "benign"
   errors (e.g., no default config).  */

extern xtensa_params
xtensa_quiet_getopt (int *argc_p, char **argv);


/* The xtensa_params_init function does the work of reading the parameter
   files to construct the xtensa_params key/value data structure.  It first
   reads the default parameter file "<system_dir>/<core_name>-params".  If
   the paramfiles array specifies one or more TDK parameter files, those
   files are read consecutively, overriding previous parameter values.  The
   paramfiles array must contain a NULL as the last entry.  If any errors
   occur, the return value is NULL and xtensa_params_errno is set.  */

extern xtensa_params
xtensa_params_init (char **system_dirs /*ReadOnly*/,
		    char *core_name    /*ReadOnly*/,
		    char **paramfiles  /*ReadOnly*/);

/* DEPRECATED: Old interface retained for backward compatibility.  */
extern xtensa_params
xtensa_read_params (char *paramfile);


/* Find the list of Xtensa system directories, either from the
   XTENSA_SYSTEM environment variable or by using xtensa_find_prefix()
   to locate the default system directory.  */

extern xtparams_status
xtensa_get_system_dirs (char *progname, char ***psystem_dirs);

/* DEPRECATED: Old interface retained for backward compatibility.  */
extern int
xtensa_find_system (char *progname);


/* Deduce the install directory prefix from the name used to invoke a
   program (i.e., argv[0]).  This assumes the program is installed in
   "<prefix>/bin" (or some other directory that is one level below the
   "<prefix>" directory.  The "<prefix>" directory name is returned
   through the "prefixp" argument.  */

extern xtparams_status
xtensa_find_prefix (char *progname, char **prefixp);


/* Get the core names that are present in the current Xtensa core registry.
   The result is a heap-allocated array that should be freed when no longer
   needed.  The end of the array is marked with a null pointer.  Returns
   NULL and sets xtensa_params_errno on failure.  */

extern char **
xtensa_get_core_names (char **system_dirs);


/* Define a global variable to hold the parameters in the common case where
   there is only one processor and hence, one set of parameters.  This is
   automatically set to the most recent result of xtensa_params_init.
   (Note: Since this variable is just a convenience, it is not exported
   when xtparams is built as a DLL, due to the hassle of dealing with
   declspecs.) */

extern xtensa_params xtensa_default_params;


/* xtensa_value: This type is a single value.  The single value has a type
   and value that is valid based upon that type.  */

typedef enum xtensa_value_type_enum
{
  XTPM_INT,
  XTPM_STRING
} xtensa_value_type;

typedef struct xtensa_value_struct
{
  xtensa_value_type type;	/* the type of the value */
  union {
    char *s;
    int i;
  } v;
} xtensa_value;


/* xtensa_values: This is a set of values.  Each parameter has a set of
   values represented with this data structure.  The pathbase field records
   the path of the directory containing the parameter file which set these
   values, so that relative paths in the parameter files can be
   resolved.  */

typedef struct xtensa_values_struct
{
  int num_values;               /* the number of values */
  xtensa_value *values;         /* the array of values. */
  char *pathbase;		/* path for the parameter file */
} xtensa_values;


/* The xtensa_find_key function searches for the values assigned to the
   specified parameter.  If the parameter has not been assigned a value,
   the result is NULL.  Otherwise the result is the set of values.  */

extern xtensa_values *
xtensa_find_key (xtensa_params params, const char *key);


/* Convert a string to an absolute path.  If the string begins with "~",
   this function attempts to expand it to the appropriate home directory.
   Otherwise, if the string is not already absolute, it prepends the
   pathbase (which is the directory containing the parameter file).  The
   result is always allocated separately and should be freed when no longer
   needed.  Returns NULL and sets xtensa_params_errno on failure.  */

extern char *
xtensa_make_absolute_path (char *path, char *pathbase);


/* Convenience functions for retrieving the first (and only) string,
   integer, or path value.  If the value is anything else, the program will
   print an error message to stderr and immediately exit.  If you want any
   other sort of error handling, don't use these functions.  The result of
   xtensa_path_value is always allocated separately and should be freed
   when no longer needed.  */

extern char *
xtensa_string_value (xtensa_values *v);

extern int
xtensa_int_value (xtensa_values *v);

extern char *
xtensa_path_value (xtensa_values *v);


/* Convenience function to take any number of DLL paths or lists of paths and
   return a null-terminated array of these paths.  The end of the argument
   list must be marked with a null pointer.  The result array and the
   individual path elements are allocated separately and should be freed
   when they are no longer needed.  Unlike the other convenience functions,
   this one will not write to stderr or exit on error; instead it returns
   NULL and sets xtensa_params_errno on failure.  */

extern char **
xtensa_path_array (xtensa_values *v, ...);


#define VALID_VALUES( v ) ( (v) && (v)->num_values )

#ifdef __cplusplus
}
#endif
#endif /* XTENSA_PARAMS_H */
