#ifndef __XI_LIB_H__
#define __XI_LIB_H__

#ifdef __OPENCL_C_VERSION__
#include "xi_tile_manager_ocl.h"
#else
#include "xi_tile_manager.h"
#endif
#include "xi_version.h"

#ifdef __cplusplus
#define XI_DEFAULT(value) = (value)
#else
#define XI_DEFAULT(value)
#endif

#if defined __GNUC__ && __GNUC__ >= 4
#define _XI_EXPORTS_ __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#if defined(XI_CREATE_SHARED_LIBRARY)
#define _XI_EXPORTS_ __declspec(dllexport)
#else
#define _XI_EXPORTS_ __declspec(dllimport)
#endif
#else
#define _XI_EXPORTS_
#endif

typedef uint8_t xi_bool;
typedef int32_t XI_ERR_TYPE;

/* Error codes */

#define XI_ERR_OK         0 // no error
#define XI_ERR_IALIGNMENT 1 // input alignment requirements are not satisfied
#define XI_ERR_OALIGNMENT 2 // output alignment requirements are not satisfied
#define XI_ERR_MALIGNMENT                                                      \
  3                       // same modulo alignment requirement is not satisfied
#define XI_ERR_BADARG   4 // arguments are somehow invalid
#define XI_ERR_MEMLOCAL 5 // tile is not placed in local memory
#define XI_ERR_INPLACE  6 // inplace operation is not supported
#define XI_ERR_EDGE     7 // edge extension size is too small
#define XI_ERR_DATASIZE                                                        \
  8 // input/output tile size is too small or too big or otherwise inconsistent
#define XI_ERR_TMPSIZE                                                         \
  9 // temporary tile size is too small or otherwise inconsistent
#define XI_ERR_KSIZE          10 // filer kernel size is not supported
#define XI_ERR_NORM           11 // invalid normalization divisor or shift value
#define XI_ERR_COORD          12 // invalid coordinates
#define XI_ERR_BADTRANSFORM   13 // the transform is singular or otherwise invalid
#define XI_ERR_NULLARG        14 // one of required arguments is null
#define XI_ERR_THRESH_INVALID 15 // threshold value is somehow invalid
#define XI_ERR_SCALE          16 // provided scale factor is not supported
#define XI_ERR_OVERFLOW       17 // tile size can lead to sum overflow
#define XI_ERR_NOTIMPLEMENTED                                                  \
  18 // the requested functionality is absent in current version
#define XI_ERR_CHANNEL_INVALID 19 // invalid channel number
#define XI_ERR_DATATYPE        20 // argument has invalid data type

#define XI_ERR_LAST 20

/* non-fatal errors */

#define XI_ERR_POOR_DECOMPOSITION                                              \
  1024 // computed transform decomposition can produce visual artifacts
#define XI_ERR_OUTOFTILE  1025 // arguments or results are out of tile
#define XI_ERR_OBJECTLOST 1026 // tracked object is lost
#define XI_ERR_RANSAC_NOTFOUND                                                 \
  1027                     // there is no found appropriate model for RANSAC
#define XI_ERR_REPLAY 1028 // function has to be called again for completion
/* error check levels */

/* do not check arguments for errors */
#define XI_ERROR_LEVEL_NO_ERROR 0
/* call exit(-1) in case of error */
#define XI_ERROR_LEVEL_TERMINATE_ON_ERROR 1
/* return corresponding error code on error without any processing
 * (recommended)*/
#define XI_ERROR_LEVEL_RETURN_ON_ERROR 2
/* capture error but attempt continue processing (dangerous!) */
#define XI_ERROR_LEVEL_CONTINUE_ON_ERROR 3
/* print error message to stdout and return without any processing */
#define XI_ERROR_LEVEL_PRINT_ON_ERROR 4
/* print error message but attempt continue processing (dangerous!) */
#define XI_ERROR_LEVEL_PRINT_AND_CONTINUE_ON_ERROR 5

#ifndef XI_ERROR_LEVEL
#define XI_ERROR_LEVEL XI_ERROR_LEVEL_RETURN_ON_ERROR
#endif

/* helper macro */

#ifdef XCHAL_IVPN_SIMD_WIDTH
#define XI_SIMD_WIDTH XCHAL_IVPN_SIMD_WIDTH
#else
#define XI_SIMD_WIDTH 32
#endif

/* error code to text conversion */
const char *xiErrStr(XI_ERR_TYPE code);

#endif /* __XI_LIB_H__ */
