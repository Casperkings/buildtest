This folder includes sources to libxipc. Below gives a brief description
of the files. For more details on XIPC, please refer to the XIPC Reference
Manual.

libxipc library user visible header files
-----------------------------------------

These files are visible to the user and included in the target application.
They define the APIs and data types for each of the XIPC primitives.

- xipc_common.h 
- xipc_barrier.h
- xipc_mutex.h
- xipc_sem.h
- xipc_cond.h
- xipc_counted_event.h
- xipc_cqueue.h
- xipc_counted_event.h
- xipc_pkt_channel.h
- xipc_msg_channel.h 
- xipc_rpc.h 
- xipc.h
- xipc_primitves.h

libxipc internal core C source and header files
-----------------------------------------------

These are base internal source files that implement the XIPC primitives.

- xipc_init.c
- xipc_barrier.c
- xipc_mutex.c
- xipc_sem.c
- xipc_cond.c
- xipc_counted_event.c
- xipc_cqueue.c
- xipc_msg_channel.c
- xipc_pkt_channel.c
- xipc_rpc.c
- xipc_misc.h
- xipc_barrier_internal.h
- xipc_mutex_internal.h
- xipc_sem_internal.h
- xipc_cond_internal.h
- xipc_counted_event_internal.h
- xipc_cqueue_internal.h
- xipc_msg_channel_internal.h
- xipc_pkt_channel_internal.h
- xipc_rpc_internal.h

libxipc runtime interface files
-------------------------------

These defines runtime specific features like enabling/disabling interrupts,
sleep wait etc. By default, XTOS and XOS versions of the runtime are supported.

xipc_noos.h defines the basic functions that would need to be implemented
if porting XIPC to different runtime.

xipc_xos.h
xipc_xtos.h
xipc_noos.h
xipc_xtos.c
xipc_xos.c

Generic subsystem interface files
---------------------------------

libxipc expects a target subsystem that defines the number of processors,
inter-processor interrupts, address translation etc. The default implementation
assumes a system-builder that automatically generates these parameters. The
default implementation is provided in the below files 

- xipc_addr.c
- xipc_intr.c
- xipc_addr.h
- xipc_sys.h 

Custom subsystem interface files
--------------------------------

To port libxipc to a custom subsystem the below files need to be modified
to define the target subsystem specific parameters.

- xipc_custom_addr.c 
- xipc_custom_intr.c 
- xmp_custom_subsystem.h
- xipc_custom_sys.h
