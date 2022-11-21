#include "Halide.h"

using namespace Halide;

namespace {
class Median3x3 : public Halide::Generator<Median3x3> {
public:
  
  Input<Buffer<uint8_t>>  input{"input", 2};
  Output<Buffer<uint8_t>> output{"output", 2};

  Expr max2_(Expr a, Expr b){
    return select(a > b, a, b);
  }

  Expr max3_(Expr a, Expr b, Expr c){
    return max2_(max2_(a, b), c);
  }

  Expr min2_(Expr a, Expr b){
    return select(a < b, a , b);
  }

  Expr min3_(Expr a, Expr b, Expr c){
    return min2_(min2_(a, b), c);
  }

  Expr mid3_(Expr a, Expr b, Expr c){
    return max2_(min2_(max2_(a, b), c), min2_(a, b));
  }

  void generate() {
    Func IM_output("IM_output");
    Var x("x"), y("y"), xi("xi"), yi("yi");

    // Algorithm.
    Func max_x("max_x"), min_x("min_x"), mid_x("mid_x");
    max_x(x, y) = max3_(input(x, y), input(x + 1, y), input(x + 2, y));
    min_x(x, y) = min3_(input(x, y), input(x + 1, y), input(x + 2, y));
    mid_x(x, y) = mid3_(input(x, y), input(x + 1, y), input(x + 2, y));

    Func min_max("min_max"), max_min("max_min"), mid_mid("mid_mid");
    min_max(x, y) = min3_(max_x(x, y), max_x(x, y + 1), max_x(x, y + 2));
    max_min(x, y) = max3_(min_x(x, y), min_x(x, y + 1), min_x(x, y + 2));
    mid_mid(x, y) = mid3_(mid_x(x, y), mid_x(x, y + 1), mid_x(x, y + 2));

    Func median3x3("median3x3");
    IM_output(x, y) = mid3_(min_max(x, y), max_min(x, y), mid_mid(x, y));
    
    output(x, y) = IM_output(x, y);

    if (get_target().has_feature(Target::CLCadence)) {
      input.in()
           .compute_at(output, x)
           .dma_copy(_1);

      max_x.compute_at(IM_output, xi)
           .vectorize(x, 64)
           .store_in(MemoryType::Register);

      min_x.compute_at(IM_output, xi)
           .vectorize(x, 64)
           .compute_with(max_x, y)
           .store_in(MemoryType::Register);

      mid_x.compute_at(IM_output, xi)
           .vectorize(x, 64)
           .compute_with(max_x, y)
           .store_in(MemoryType::Register);

      IM_output.compute_at(output, x)
               .tile(x, y, xi, yi, 128, 64)
               .vectorize(xi, 64)
               .reorder(yi, xi, x, y);
               
      output.compute_root()
            .tile(x, y, xi, yi, 128, 64)
            .gpu_blocks(x, y)  
            .dma_copy(yi);
    }
  }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Median3x3, Median3x3)
