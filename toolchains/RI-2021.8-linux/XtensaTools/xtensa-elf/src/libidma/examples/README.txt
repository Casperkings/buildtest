
The libidma examples provided here exercise various idma features.
The functionality of each example is described in the source code.
If your target hardware configuration does not include some idma
features, the corresponding examples may not build/run.

It is expected that users familiarize themselves with the IDMA 
hardware and the libidma API before trying these examples. These
examples are provided as-is and are not meant to be tutorials.

Each example is compiled in two versions, one using the generic IDMA
library and the other using the XTOS-specific library. Each of the
two are again compiled in two ways, one using the standard library
and one using the debug version. The makefile defines IDMA_DEBUG
for all the examples when building the debug versions.

In general, regular non-debug variants of the examples will output
two messages only: "Test '<TESTNAME>'" at the begining and either 
"COPY OK" or  "COPY FAILED" message at the end. The latter two
come from the shared code in idma_tests that compares copied data
and expected data.

Note that some examples might generate warnings and failures during
copying, as that is their intended behaviour.

---

