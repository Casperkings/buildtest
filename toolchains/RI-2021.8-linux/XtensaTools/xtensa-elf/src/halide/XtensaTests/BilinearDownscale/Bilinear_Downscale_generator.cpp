#include "Halide.h"

#define IMAGE_W        512
#define IMAGE_H        512
#define SRC_BIT_DEPTH  8
#define DST_BIT_DEPTH  8
#define TILE_W         128
#define TILE_H         64
#define W_EXT          0
#define H_EXT          0
#define IMAGE_PAD      0
#define OUT_IMAGE_W    (256)
#define OUT_IMAGE_H    (280)
#define OUT_TILE_W     (64)
#define OUT_TILE_H     (35)

using namespace Halide;
using namespace Halide::ConciseCasts;

namespace {
class BilinearDownscale : public Halide::Generator<BilinearDownscale> {
public:
  
  Input<Buffer<uint8_t>>  input{"input", 2};
  Output<Buffer<uint8_t>> Finaloutput{"Finaloutput", 2};

  int xscale = (IMAGE_W << 18) / OUT_IMAGE_W;
  int yscale = (IMAGE_H << 18) / OUT_IMAGE_H; 
  int xshift = 0;
  int yshift = 0;

  void generate() {
    Var  x("x"), y("y"), xi("xi"), yi("yi"), f("f");
    Func input_i16("input_i16"), step3("step3"),
         step2("step2"), step1("step1"), output("output");
  
    input_i16(x, y) = i16(input(x, y));
  
    Expr yoffs = (y * yscale  + (yscale >> 1) - (1 << 17)) + 4;
    Expr yf = i16(((yoffs & ((1 << 18) - 1)) >> 3));
    Expr yo = (yoffs >> 18) + IMAGE_PAD;

    Expr xoffs = (x) * xscale + (xscale >> 1) - (1 << 17);
    Expr xf = i16((xoffs & ((1 << 18) - 1)) >> 3);
    Expr xo = (xoffs >> 18) + IMAGE_PAD;
  
    Expr Myf_factor = (i16(1 << 15) - yf);
    Expr Pyf_factor = (yf);
    Expr Mxf_factor = (i16(1 << 15) - xf);
    Expr Pxf_factor = (xf);

    step1(x, y) = i16(((input_i16(xo, yo) * i32(Myf_factor)) + 
                       (input_i16(xo, yo+1) * i32(Pyf_factor)) + 
                        i32(1 << 14)) >> 15);
    step2(x, y) = i16(((input_i16(xo+1, yo) * i32(Myf_factor)) + 
                       (input_i16(xo+1, yo+1) * i32(Pyf_factor)) + 
                        i32(1 << 14)) >> 15);
    /*
     * Add and Subract 1 to prevent direct cast from Int to UChar on 
     * simplification. Remove once cast from int32 to uchar32 support 
     * is provided.
     */
    step3(x, y) = u8(i16((((step1(x, y) * i32(Mxf_factor)) + 
                           (step2(x, y) * i32(Pxf_factor)) + 
                           i32(1<<14))>>15) + 1) - 1); 

    Finaloutput(x, y) = step3(x, y);
  
    if (get_target().has_feature(Target::CLCadence)) {
      input.in()
           .compute_at(Finaloutput, x)
           .dma_copy(_1);

      step3.compute_at(Finaloutput, x)
            .reorder(y, x)
            .vectorize(x, 32)
            .store_in(MemoryType::Xt_Shared);

      Finaloutput.compute_root()
                 .tile(x, y, xi, yi, OUT_TILE_W, OUT_TILE_H)
                 .gpu_blocks(x, y)
                 .dma_copy(yi);
     }
  }
};

}  // namespace
HALIDE_REGISTER_GENERATOR(BilinearDownscale, Bilinear_Downscale)
