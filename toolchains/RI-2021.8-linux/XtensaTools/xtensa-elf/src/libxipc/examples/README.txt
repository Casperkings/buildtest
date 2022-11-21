This directory provides simple examples to illustrate the use of each of
the libxipc primitives. There is one directory per primitive. 
Below briefly describes the files and the steps required to build and run
the examples.

The examples have been tested for a 3-core visionp6_ao based MP-subsystem, 
although in theory it should extend to other configs and more cores.
The example assumes that all cores are symmetric.

The following files are available for each primitive:
 - core_template.c
     A generic program that can run on all cores.
     Internally, it conditionalizes, based on the processor id, 
     what each core does.

 - ../common/xmp_log.h
     Defines a simple printing routine xmp_log() that can be used by the
     application to format and print messages from each core. Requires 
     libxtutil.a.

 - shared.c, shared.h
     Defines data structures that are placed in shared system memory
     and used by each of the cores in the subsystem.

 - ../common/xmp_mpu_attr.c
     For configs with MPU, this sets up the relevant attributes.

 - subsys.xtsys
   Specification file for a 3-core visionp6_ao subsystem to build the 
   MP LSPs using xt-mbuild

 - subsys.yml
   Specification file for a 3-core reference visionp6_ao subsystem to build the 
   MP XTSC subsystem and XIPC system layer using xt-sysbuilder

To build, first do 'make build' to build the subsystem, followed by make
to build and run all the tests.
