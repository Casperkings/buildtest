#include "Halide.h"

using namespace Halide;

namespace {
class Filter2D5x5 : public Halide::Generator<Filter2D5x5> {
public:
  
  Input<Buffer<uint8_t>>  input{"input", 2};
  Input<Buffer<uint8_t>>  kernel{"kernel", 2};

  Output<Buffer<uint8_t>> output{"output", 2};

  void generate() {
    Func Row_1("Row_1"), Row_2("Row_2"), Row_3("Row_3"), Row_4("Row_4"),
         Row_5("Row_5");

    Func output_tmp("output_tmp");

    Var x("x"), y("y"), xi("xi"), yi("yi"), 
       xin_outer("xin_outer"), xin_inner("xin_inner");

    // Algorithm
    Row_1(x, y) = cast(UInt(16), input(x,   y)) * kernel(0, 0) +
                  cast(UInt(16), input(x+1, y)) * kernel(0, 1) +
                  cast(UInt(16), input(x+2, y)) * kernel(0, 2) +
                  cast(UInt(16), input(x+3, y)) * kernel(0, 3) +
                  cast(UInt(16), input(x+4, y)) * kernel(0, 4);

    Row_2(x, y) = cast(UInt(16), input(x   ,y+1)) * kernel(1, 0) +
                  cast(UInt(16), input(x+1 ,y+1)) * kernel(1, 1) +
                  cast(UInt(16), input(x+2 ,y+1)) * kernel(1, 2) +
                  cast(UInt(16), input(x+3 ,y+1)) * kernel(1, 3) +
                  cast(UInt(16), input(x+4 ,y+1)) * kernel(1, 4);

    Row_3(x, y) = cast(UInt(16), input(x   ,y+2)) * kernel(2, 0) +
                  cast(UInt(16), input(x+1 ,y+2)) * kernel(2, 1) +
                  cast(UInt(16), input(x+2 ,y+2)) * kernel(2, 2) +
                  cast(UInt(16), input(x+3 ,y+2)) * kernel(2, 3) +
                  cast(UInt(16), input(x+4 ,y+2)) * kernel(2, 4);

    Row_4(x, y) = cast(UInt(16), input(x   ,y+3)) * kernel(3, 0) +
                  cast(UInt(16), input(x+1 ,y+3)) * kernel(3, 1) +
                  cast(UInt(16), input(x+2 ,y+3)) * kernel(3, 2) +
                  cast(UInt(16), input(x+3 ,y+3)) * kernel(3, 3) +
                  cast(UInt(16), input(x+4 ,y+3)) * kernel(3, 4);

    Row_5(x, y) = cast(UInt(16), input(x   ,y+4)) * kernel(4, 0) +
                  cast(UInt(16), input(x+1 ,y+4)) * kernel(4, 1) +
                  cast(UInt(16), input(x+2 ,y+4)) * kernel(4, 2) +
                  cast(UInt(16), input(x+3 ,y+4)) * kernel(4, 3) +
                  cast(UInt(16), input(x+4 ,y+4)) * kernel(4, 4);
    
    output_tmp(x, y) = cast(UInt(8), (Row_1(x, y) + 
                                      Row_2(x, y) +
                                      Row_3(x, y) +
                                      Row_4(x, y) +
                                      Row_5(x, y)) >> 8);

    output(x, y) = output_tmp(x, y);
  
    // Schedule
    if (get_target().has_feature(Target::CLCadence)) {

      input.in()
           .compute_at(output, x)
           .dma_copy(_1);

      kernel.in()
           .compute_at(output, x)
           .dma_copy(_1);

      output_tmp.compute_at(output, x)
                .tile(x, y, xi, yi, 128, 64)
                .split(xi, xin_outer, xin_inner, 32)
                .vectorize(xin_inner, 32)
                .store_in(MemoryType::Xt_Shared)
                .reorder(yi, xin_outer);

      output.compute_root()
            .tile(x, y, xi, yi, 128, 64)
            .gpu_blocks(x, y)
            .dma_copy(yi);
    } 
  }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Filter2D5x5, filter2D5x5)
