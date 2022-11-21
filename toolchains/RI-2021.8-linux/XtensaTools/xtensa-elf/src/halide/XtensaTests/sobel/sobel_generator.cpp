#include "Halide.h"
using namespace Halide;
namespace {

  class sobelClass : public Halide::Generator<sobelClass> {
  public:

    Input<Buffer<uint8_t>> input{"input", 2};
    Output<Buffer<int16_t>> Finaloutput{"Finaloutput", 3};

    void generate() {
      Func  subX("subX"),
            subY("subY"),
            resX("resX"),
            resY("resY"),
            input_cast("input_cast"),
            output("output")
            ;
      
      Var x("x"), y("y"), xin("xin"), yin("yin"), xInOutput("xInOuptut"), c("c");
      
      input_cast(x, y)  = cast(Int(16), input(x,y));
      
      subX(x, y)  =  - input_cast(x, y)
                     - 2*input_cast(x+1, y)
                     + 2*input_cast(x+3, y)
                     + input_cast(x+4, y);
      
      subY(x, y)  =  input_cast(x, y)
                     + 4*input_cast(x+1, y)
                     + 6*input_cast(x+2, y)
                     + 4*input_cast(x+3, y)
                     + input_cast(x+4, y);
      
      resX(x, y)  =   subX(x, y)
                    + 4*subX(x, y+1)
                    + 6*subX(x, y+2)
                    + 4*subX(x, y+3)
                    + subX(x, y+4);
      
      resY(x, y)  = - subY(x, y)
                    - 2*subY(x, y+1)
                    + 2*subY(x, y+3)
                    + subY(x, y+4);
      
                    
      // Final Dx stored in channel 0 and Dy stored in channel 1 
      output(x, y, c) = select(c == 0, resX(x, y),
                             c == 1, resY(x, y),
                             (int16_t)0);
    
      Finaloutput(x, y, c)  = output(x, y, c);

      // Schedule
      if (get_target().has_feature(Target::CLCadence)) {
        input.in()
             .compute_at(Finaloutput, x)
             .dma_copy(_1);
        
        subX .compute_at(output, x)
             .vectorize(x, 32)
             .store_in(MemoryType::Register)
             .compute_with(subY, x, LoopAlignStrategy::AlignEnd);
        
        subY.compute_at(output, x)
            .vectorize(x, 32)
            .store_in(MemoryType::Register) ;
        
        output.compute_at(Finaloutput, x)
              .split(x, x, xInOutput, 32)
              .vectorize(xInOutput, 32)
              .reorder(c,xInOutput, y, x)
              .unroll(c);

        Finaloutput.tile(x, y, xin, yin, 128, 64)
                   .reorder(xin, yin, c, x, y)
                   .bound(c, 0, 2)
                   .unroll(c)
                   .dma_copy(yin)
                   .gpu_blocks(x, y);
      }
    }
  };

} // namespace

HALIDE_REGISTER_GENERATOR(sobelClass, sobel)
