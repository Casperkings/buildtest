#include "Halide.h"
using namespace Halide;
namespace {
class PyramidU8 : public Halide::Generator<PyramidU8> {
public:
  Input<Buffer<uint16_t>>  input{"input", 2};
  Output<Buffer<uint8_t>> output{"output", 2};
  
  void generate(){
    Func blur_x("blur_x"), IM_output("IM_output");
    Var x("x"), y("y"), xi("xi"), yi("yi");
    
    blur_x(x, y) = (input(x, y)&0xFF) +
                   (input(x, y)>>8)*4 +
                   (input(x+1, y)&0xFF)*6 +
                   (input(x+1, y)>>8)*4 +
                   (input(x+2, y)&0xFF);
				   

    IM_output(x, y) = cast(UInt(8),((blur_x(x, 2*y) +
                                     blur_x(x, 2*y+1)*4 + 
                                     blur_x(x, 2*y+2)*6 + 
                                     blur_x(x, 2*y+3)*4 +
                                     blur_x(x, 2*y+4) + 128) >> 8));

    output(x, y) = IM_output(x, y);

    if (get_target().has_feature(Target::CLCadence)) {

      input.in()
           .compute_at(output, x)
           .dma_copy(_1);

      blur_x.compute_at(IM_output, x)
            .vectorize(x, 32)
            .store_in(MemoryType::Register);

      IM_output.compute_at(output, x)
               .vectorize(x, 32)
               .reorder(y, x);

      output.compute_root()
            .tile(x, y, xi, yi, 64, 32)
            .gpu_blocks(x, y)
            .reorder(xi, yi, x, y)
            .dma_copy(yi);
    }
  }
};
}

HALIDE_REGISTER_GENERATOR(PyramidU8, PyramidU8)
