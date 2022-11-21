
This set of test programs is meant as a sanity check suite and also as some
examples of XOS API usage. As you inspect the code, pay attention to the
various ifdefs, particularly the ones to do with various XOS build options.


NOTES:

    - "make all" will build (almost) all the tests in here. The ones excluded
      are the ones that need something different, as listed below.

    - TARGET=SIM will build for the simulator (this is the default).

    - TARGET=ML605 will build for the ML605 FPGA board. You can replace this
      with your own LSP/libs to build for your board.

    - TARGET=KC705 will build for the KC705 FPGA board.

    - MODE=OPT will optimize for speed. Note that if you want to use this mode
      then the XOS lib must also be built with MODE=OPT. The build rules in
      this makefile will do the right thing if invoked to build the XOS lib.

    - MODE=DEV is also optimized, but includes some debug options etc. that
      make it easier to debug the code.

    - MODE=DBG will turn on a lot more debug options in the XOS code.

    - MODE=SIZE will optimize for smallest size. When this is used, the XOS
      lib must also be built with MODE=SIZE. This mode will NOT work for all
      the tests, as it disables C lib thread safety and also reduces the size
      of the interrupt stack.

    - The "multi" test is a stress test that runs multiple threads doing many
      different things. This test will not terminate by itself; it must be
      stopped manually. This test uses the default MPU init routine to set
      up the MPU if present.

    - The "perf" test measures performance, and also reports the sizes of 
      various data structures. This test should be built with MODE=OPT for
      proper results.

    - The "stats" test exercises the statistics functions. This test must be
      built with XOS_OPT_STATS defined > 0.

    - The "small" test is an example that is built for the smallest possible
      size. This must be built with MODE=SIZE.

    - If some basic output functions are needed, use libxtutil which provides
      small-footprint versions of some functions such as printf() etc. These
      functions are not completely compliant with the standard C versions.
      Examples of libxtutil use are available in some of the tests. The tests
      do require libxtutil, and so does XOS when debug options are enabled.

    - USE_MAIN_THREAD controls whether the main() function is converted
      into a thread or not. The tests are structured to work either way.
      Define this by editing the Makefile or specify it on the command line
      e.g. CFLAGS_EXTRA=-DUSE_MAIN_THREAD=1.

    Building the tests for Secure Mode:

    Most of the XOS tests can be run as nonsecure applications using the
    Secure Monitor on Xtensa LX cores configured for Secure Mode. The tests
    can be built as nonsecure apps like so:

      xt-make all CFLAGS_EXTRA="-DXOS_OPT_SECMON=1" LSP=app-sim

    The exception test currently does not work in this mode because not all
    exceptions are allowed to be handled in nonsecure code.

    The "multi" test can run into problems in this mode because it tries to
    program the MPU in a certain way. If the Secure Monitor disables nonsecure
    write access to the MPU, the MPU programming will fail. Even if nonsecure
    write access is allowed, the MPU setup may conflict with whatever was
    already set up during secure boot. We recommend you disable the MPU setup
    in the multi test if you wish to run it as a nonsecure application.

    Note that the executables have to be converted into packed binary format
    via xt-elf2rom before they can be loaded by the Secure Monitor. See the
    Secure Mode Programmers Guide and the secure mode examples for details.

---

