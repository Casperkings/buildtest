#include <xtensa/config/core.h>

#include "libio.h"


clock_t
_times_r (struct _reent *reent, struct tms *buf)
{
    switch (libio_mode) {
    case LIBIO_SIM:
        {
            register int a2 __asm__ ("a2") = SYS_times;
            register struct tms *a3 __asm__ ("a3") = buf;
            register clock_t ret_val __asm__ ("a2");
            register int ret_err __asm__ ("a3");

            __asm__ ("simcall"
                     : "=a" (ret_val), "=a" (ret_err)
                     : "a" (a2), "a" (a3));

            reent->_errno = ret_err;
            return ret_val;
        }

    case LIBIO_GDB:
        {
#if XCHAL_HAVE_CCOUNT
            clock_t clk;

            __asm__ ("rsr.ccount %0" : "=a" (clk));
            if (buf) {
                buf->tms_utime  = clk;
                buf->tms_stime  = 0;
                buf->tms_cutime = 0;
                buf->tms_cstime = 0;
            }
            return clk;
#else
            if (buf) {
                buf->tms_utime  = 0;
                buf->tms_stime  = 0;
                buf->tms_cutime = 0;
                buf->tms_cstime = 0;
            }
            reent->_errno = ENOSYS;
            return (clock_t) -1;
#endif
        }

    default:
        break;
    }

    reent->_errno = ENOSYS;
    return (clock_t) -1;
}

