#ifndef HEADER_panasonic_avc_distortion
#define HEADER_panasonic_avc_distortion

#define RECT_SIZE (32)
#define RECT_SIZE_SHIFT (5)
#define PIXEL_Q (4)
#define PIXEL_DEC (1 << (PIXEL_Q))
#define PIXEL_Q_MASK ((1 << PIXEL_Q) - 1)
#define PIXEL_Q_MASK_UV ((1 << (PIXEL_Q + 1)) - 1)

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#endif /*  */
