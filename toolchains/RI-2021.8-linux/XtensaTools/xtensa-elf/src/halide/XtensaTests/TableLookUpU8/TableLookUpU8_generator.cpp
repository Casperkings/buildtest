#include "Halide.h"
using namespace Halide::ConciseCasts;
using namespace Halide;

namespace {
class TableLookUpU8 : public Halide::Generator<TableLookUpU8> {
public:
  
  Input<Buffer<uint8_t>>  input{"input", 2};
  Input<Buffer<uint8_t>>  lut{"lut", 1};
  Output<Buffer<uint8_t>> output{"output", 2};
  
  void generate() {
    Func IM_output("IM_output");
    Var x("x"), y("y"), xi("xi"), yi("yi");
            
    IM_output(x, y) = u8(lut(u16(input(x,y))));  
  
    output(x, y) = u8(IM_output(x, y));

    if (get_target().has_feature(Target::CLCadence)) {
      input.in()
           .compute_at(output, x)
           .store_in(MemoryType::Xt_Shared)
           .dma_copy(_1);
       
      lut.in()
         .compute_at(output, x)
         .store_in(MemoryType::Xt_Shared)
         .bound(_0, 0, 256)
         .dma_copy(_0);

      IM_output.compute_at(output, x)               
               .reorder(y, x)
               .vectorize(x, 32);//.unroll(y,4);
               
      output.compute_root()
            .tile(x, y, xi, yi, 128, 64)
            .gpu_blocks(x, y)  
            .dma_copy(yi);
    }
  }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(TableLookUpU8, TableLookUpU8)
