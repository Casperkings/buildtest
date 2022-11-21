#include "Halide.h"

using namespace Halide;
using namespace Halide::ConciseCasts;

#define Q15_0_7874 25802
#define Q15_0_9278 30402
#define Q15_0_0936 3068
#define Q15_0_2340 7669

namespace {
class YUV420toRGB : public Halide::Generator<YUV420toRGB> {
public:
  
  Input<Buffer<uint8_t>>  inputY{"inputY", 2};
  Input<Buffer<uint8_t>>  inputU{"inputU", 2};
  Input<Buffer<uint8_t>>  inputV{"inputV", 2};
  Output<Buffer<uint8_t>> output{"output", 3};

  void generate() {
    Func temp_planar("temp_planar");
    
    Var x("x"), y("y"), c("c"), xi("xi"), yi("yi");

    Expr Y = i16(inputY(x, y));
    Expr U = i16(inputU(x/2, y/2)) - 128;
    Expr V = i16(inputV(x/2, y/2)) - 128;

    Expr A = i32(V) * i16(Q15_0_7874) + (1<<13);
    Expr C = i32(V) * -Q15_0_2340 - i32(U) * Q15_0_0936 + (1<<13);
    Expr D = i32(U) * Q15_0_9278 + (1<<13);

    Expr R = i16((A>>14) + Y);
    Expr G = i16((C>>14) + Y);
    Expr B = i16((D>>14) + Y);
  
    Expr R_out = u8(max(min(R, 255), 0));
    Expr G_out = u8(max(min(G, 255), 0));
    Expr B_out = u8(max(min(B, 255), 0));

    temp_planar(c, x, y) = select(c == 0, R_out,
                                  c == 1, G_out,
                                  c == 2, B_out,
                                  0);
                  
    output(c, x, y) = temp_planar(c, x, y);
  
    if (get_target().has_feature(Target::CLCadence)) {

      output.dim(0).set_bounds(0,3).set_stride(1);
        
      inputY.in()
            .compute_at(output, x)
            .dma_copy(_1);
      
      inputU.in()
            .compute_at(output, x)
            .dma_copy(_1);
      
      inputV.in()
            .compute_at(output, x)
            .dma_copy(_1);

      temp_planar.compute_at(output, x)
                 .reorder(c, y, x)
                 // cannot vectorize
                 // .vectorize(x, 64)
                 .bound(c, 0, 3)
                 .unroll(c);

      output.compute_root()
            .tile(x, y, xi, yi, 128, 64)
            .reorder(c, xi, yi, x, y)
            .bound(c, 0, 3)
            .gpu_blocks(x, y)
            .unroll(c)
            .dma_copy(yi);
    } 
  }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(YUV420toRGB, yuv420torgb)
