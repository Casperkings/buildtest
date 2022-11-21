#include "Halide.h"

using namespace Halide;
using namespace Halide::ConciseCasts;

HalideExtern_3(uint32_t, xt_mad32, uint16_t, uint16_t, uint32_t);
HalideExtern_2(uint16_t, abs_diff, int16_t, int16_t);

namespace {
class SumOfSquaredDiffsU8S32 : public Halide::Generator<SumOfSquaredDiffsU8S32>
{
public:
  Input<Buffer<uint8_t>>  inputsrc{"inputsrc", 2};
  Input<Buffer<uint8_t>>  inputsrc1{"inputsrc1", 1};
  Output<Buffer<uint32_t>> output{"output", 2};
  
  void generate() {
    Func IM_output("IM_output");
    Var x("x"), y("y"), xi("xi"), yi("yi");
  
    RDom rx(0, 16);
    IM_output(y, x) = u32(0);
    Expr ad = abs_diff(i16(inputsrc(x*16 + rx, y)), i16(inputsrc1(x*16 + rx)));
    IM_output(y, x) = xt_mad32(ad, ad, IM_output(y,  x));
        
    output(x, y) = IM_output(x, y);

    if (get_target().has_feature(Target::CLCadence)) {
      
      inputsrc.in()
              .compute_at(output, x)
              .dma_copy(_1); 
     
      inputsrc1.in()
               .compute_at(output, x)
               .store_in(MemoryType::Xt_Shared)
               .dma_copy(_0);        
       
      IM_output.compute_at(output, x)
               .store_in(MemoryType::Xt_Shared)
               .unroll(y)
               .vectorize(y, 32);
        
      IM_output.compute_at(output, x)
               .store_in(MemoryType::Xt_Shared)
               .update(0)
               .reorder(y, rx, x)
               .unroll(y)
               .vectorize(y, 32) ;

      output.compute_root()
            .tile(x, y, xi, yi, 64, 8)
            .gpu_blocks(x, y)
            .dma_copy(yi);
    }
  }
};

}

HALIDE_REGISTER_GENERATOR(SumOfSquaredDiffsU8S32, SumOfSquaredDiffsU8S32)
