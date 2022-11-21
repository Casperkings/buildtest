#include "Halide.h"

using namespace Halide;
using namespace Halide::ConciseCasts;
#define Q15_1div9      3641
namespace {
class BoxFilter3x3 : public Halide::Generator<BoxFilter3x3> {
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
    Func sum("sum");
    sum(x, y) = (u16(input(x, y)) + u16(input(x + 1, y)) + 
                 u16(input(x + 2, y)));

    IM_output(x, y) = u8(((u32(sum(x, y) + sum(x, y + 1) + sum(x, y + 2)) * 
                           u32(Q15_1div9) ) + u32(1<<14)) >> 15);
    
    output(x, y) = IM_output(x, y);

    if (get_target().has_feature(Target::CLCadence)) {
      input.in()
           .compute_at(output, x)
           .dma_copy(_1);

      sum.compute_at(IM_output, x)
           .vectorize(x, 32)
           .store_in(MemoryType::Register);

      IM_output.compute_at(output, x)
               .vectorize(x, 32)
               .reorder(y, x);
               
      output.compute_root()
            .tile(x, y, xi, yi, 128, 64)
            .gpu_blocks(x, y)  
            .dma_copy(yi);
    }
  }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(BoxFilter3x3, BoxFilter3x3)
