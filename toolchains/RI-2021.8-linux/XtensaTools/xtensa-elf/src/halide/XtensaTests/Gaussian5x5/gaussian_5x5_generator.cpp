#include "Halide.h"
using namespace Halide;
namespace {
class Gaussian5x5 : public Halide::Generator<Gaussian5x5> {
public:
  
  Input<Buffer<uint8_t>>  input{"input", 2};
  Output<Buffer<uint8_t>> output{"output", 2};

  void generate() {
    Func blur_x("blur_x"), IM_output("IM_output");
    Var x("x"), y("y"), xi("xi"), yi("yi");
    
    // Algorithm
    blur_x(x, y) = cast(UInt(16), input(x, y)) +
                   cast(UInt(16), input(x+1, y))*4 +
                   cast(UInt(16), input(x+2, y))*6 +
                   cast(UInt(16), input(x+3, y))*4 +
                   cast(UInt(16), input(x+4, y));

    IM_output(x, y) = cast(UInt(8),((blur_x(x, y) +
                                     blur_x(x, y+1)*4 + 
                                     blur_x(x, y+2)*6 + 
                                     blur_x(x, y+3)*4 +
                                     blur_x(x, y+4) + 128) >> 8));
    output(x, y) = IM_output(x, y);

    // Schedule
    if (get_target().has_feature(Target::CLCadence)) {

      input.dim(0).set_min(0);
      input.dim(1).set_min(0);

      output.dim(0).set_min(0);
      output.dim(1).set_min(0);

      input.in()
           .compute_at(output, x)
           .dma_copy(_1);

      blur_x.compute_at(IM_output, x)
            .vectorize(x, 32)
            .store_in(MemoryType::Register);

      IM_output.compute_at(output, x)
               .vectorize(x, 32)
               .reorder(y,x);

      output.compute_root()
            .tile(x, y, xi, yi, 128, 64)
            .gpu_blocks(x, y)
            .reorder(xi, yi, x, y)
            .dma_copy(yi);
    }
  }
};
}  // namespace

HALIDE_REGISTER_GENERATOR(Gaussian5x5, gaussian5x5)
