#include "Halide.h"

using namespace Halide;

namespace {
class Dilate3x3 : public Halide::Generator<Dilate3x3> {
public:
  
  Input<Buffer<uint8_t>>  input{"input", 2};
  Output<Buffer<uint8_t>> output{"output", 2};

  Expr max2_(Expr a, Expr b){
    return select(a > b, a, b);
  }

  Expr max3_(Expr a, Expr b, Expr c){
    return max2_(max2_(a, b), c);
  }

  void generate() {
    Func IM_output("IM_output");
    Var x("x"), y("y"), xi("xi"), yi("yi");

    // Algorithm.
    Func max("max");
    max(x, y) = max3_(input(x, y), input(x + 1, y), input(x + 2, y));

    IM_output(x, y) = max3_(max(x, y), max(x, y + 1), max(x, y + 2));
    
    output(x, y) = IM_output(x, y);

    if (get_target().has_feature(Target::CLCadence)) {
      input.in()
           .compute_at(output, x)
           .dma_copy(_1);

      max.compute_at(IM_output, x)
         .vectorize(x, 64)
         .store_in(MemoryType::Register);

      IM_output.compute_at(output, x)
               .vectorize(x, 64)
               .reorder(y, x);
               
      output.compute_root()
            .tile(x, y, xi, yi, 128, 64)
            .gpu_blocks(x, y)  
            .dma_copy(yi);
    }
  }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Dilate3x3, Dilate3x3)
