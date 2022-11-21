#ifndef XI_LIBRARY_VERSION

#define XI_LIBRARY_VERSION_MAJOR 7
#define XI_LIBRARY_VERSION_MINOR 12
#define XI_LIBRARY_VERSION_PATCH 1

#define XI_MAKE_LIBRARY_VERSION(major, minor, patch)                           \
  ((major)*100000 + (minor)*1000 + (patch)*10)
#define XI_AUX_STR_EXP(__A) #__A
#define XI_AUX_STR(__A)     XI_AUX_STR_EXP(__A)

#define XI_LIBRARY_VERSION                                                     \
  (XI_MAKE_LIBRARY_VERSION(XI_LIBRARY_VERSION_MAJOR, XI_LIBRARY_VERSION_MINOR, \
                           XI_LIBRARY_VERSION_PATCH))
#define XI_LIBRARY_VERSION_STR                                                 \
  XI_AUX_STR(XI_LIBRARY_VERSION_MAJOR)                                         \
  "." XI_AUX_STR(XI_LIBRARY_VERSION_MINOR) "." XI_AUX_STR(                     \
      XI_LIBRARY_VERSION_PATCH)

#endif
