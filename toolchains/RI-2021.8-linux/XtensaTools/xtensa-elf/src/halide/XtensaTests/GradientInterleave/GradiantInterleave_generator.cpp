#include "Halide.h"
using namespace Halide;
using namespace Halide::ConciseCasts;
namespace {

  class GradiantInterleaveClass : public Halide::Generator<GradiantInterleaveClass> {
  public:

    Input<Buffer<uint8_t>> input{"input", 2};
    Output<Buffer<int16_t>> Finaloutput{"Finaloutput", 2};

    void generate() {
      Func input_cast("input_cast"), output("output");
      
      Var x("x"), y("y"), xin("xin"), yin("yin");
      
      input_cast(x, y) = cast(Int(16), input(x, y));
      
      Expr resX = -input_cast(x/2, y + 1) + input_cast((x/2) + 2, y + 1);
      
      Expr resY = -input_cast((x/2) + 1, y) + input_cast((x/2) + 1, y + 2);
                          
      output(x, y) = select(x%2 == 0, resX, resY);
    
      Finaloutput(x, y) = output(x, y);

      // Schedule
      if (get_target().has_feature(Target::CLCadence)) {
        input.in()
             .compute_at(Finaloutput, x)
             .store_in(MemoryType::Xt_Shared)
             .dma_copy(_1);
        
        output.compute_at(Finaloutput, x)
              .vectorize(x, 64)
              .reorder(y, x);   

        Finaloutput.tile(x, y, xin, yin, 256, 64)
                   .dma_copy(yin)
                   .gpu_blocks(x, y); 
      }
    }
  };

} // namespace

HALIDE_REGISTER_GENERATOR(GradiantInterleaveClass, GradiantInterleave)
