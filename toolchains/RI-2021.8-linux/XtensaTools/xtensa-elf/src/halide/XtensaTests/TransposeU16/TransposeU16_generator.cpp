#include "Halide.h"
using namespace Halide::ConciseCasts;
using namespace Halide;

namespace {
class TransposeU16 : public Halide::Generator<TransposeU16> {
public:
  
  Input<Buffer<uint16_t>>  input{"input", 2};
  Output<Buffer<uint16_t>> output{"output", 2};


  void generate() {
    Func IM_output("IM_output");
    Func temp("temp");
    Var x("x"), y("y"), xi("xi"), yi("yi");

    // Algorithm.
    IM_output(x, y) = input(y, x);
    
    output(x, y) = IM_output(x, y);

    if (get_target().has_feature(Target::CLCadence)) {
      input.in()
           .compute_at(output, x)
           .dma_copy(_1);

       IM_output.compute_at(output, x)
                .store_in(MemoryType::Xt_Shared)
                .vectorize(x, 32)
                .reorder(y, x);
               
      output.compute_root()
            .tile(x, y, xi, yi, 64, 128)
            .gpu_blocks(x, y)  
            .dma_copy(yi);
    }
  }
};
 
}  // namespace

HALIDE_REGISTER_GENERATOR(TransposeU16, TransposeU16)
