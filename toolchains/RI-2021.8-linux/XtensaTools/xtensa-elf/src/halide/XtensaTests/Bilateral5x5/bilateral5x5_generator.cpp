#include "Halide.h"
using namespace Halide;
using namespace Halide::ConciseCasts;
namespace {
class Bilateral5x5 : public Halide::Generator<Bilateral5x5> {
public:

  Input<Buffer<uint8_t>>  input{"input", 2};
  Input<Buffer<uint8_t>>  coeff{"coeff",2};
  Output<Buffer<uint8_t>> final_output{"final_output", 2};

  void generate() {
  
  Var x("x"), y("y"), xout("xout"), yout("yout"), xin("xin"), yin("yin");
  
  RDom m(-2, 5, "m");
  
  Func input_u16("input_u16"), Lcoeff("Lcoeff");
  Func tupleFunc("tupleFunc"), output("output");

  input_u16(x, y) = u16(input(x, y));
  
  Lcoeff(x, y) = coeff(x, y);

  // Manual loop unroll
  Expr ExactIndex0 = (absd(input_u16(x-2, y+m), input_u16(x, y)) >> 2) +
                     u16((-2+2)*(64)) + u16((m+2)*(320)) ;

  Expr ExactIndex1 = (absd(input_u16(x-1, y+m), input_u16(x, y)) >> 2) + 
                     u16((-1+2)*(64)) + u16((m+2)*(320)) ;

  Expr ExactIndex2 = (absd(input_u16(x+0, y+m), input_u16(x, y)) >> 2) + 
                     u16((0+2)*(64)) + u16((m+2)*(320)) ;

  Expr ExactIndex3 = (absd(input_u16(x+1, y+m), input_u16(x, y)) >> 2) + 
                     u16((+1+2)*(64)) + u16((m+2)*(320)) ;

  Expr ExactIndex4 = (absd(input_u16(x+2, y+m), input_u16(x, y)) >> 2) + 
                     u16((+2+2)*(64)) + u16((m+2)*(320)) ;

  // WtSum, Filt computation
  tupleFunc(x, y) = { u16(0), u32(0) };
  tupleFunc(x, y) = { tupleFunc(x,y)[0] + u16(Lcoeff(ExactIndex0, 0))
                                        + u16(Lcoeff(ExactIndex1, 0))
                                        + u16(Lcoeff(ExactIndex2, 0))
                                        + u16(Lcoeff(ExactIndex3, 0))
                                        + u16(Lcoeff(ExactIndex4, 0)),

                      tupleFunc(x,y)[1] + 
                         u16(Lcoeff(ExactIndex0, 0)) * u32(input_u16(x-2, y+m))
                       + u16(Lcoeff(ExactIndex1, 0)) * u32(input_u16(x-1, y+m))
                       + u16(Lcoeff(ExactIndex2, 0)) * u32(input_u16(x+0, y+m))
                       + u16(Lcoeff(ExactIndex3, 0)) * u32(input_u16(x+1, y+m))
                       + u16(Lcoeff(ExactIndex4, 0)) * u32(input_u16(x+2, y+m))
                   };
  
  output(x,y) = u8(tupleFunc(x, y)[1] / u32(tupleFunc(x, y)[0]));
  
  final_output(x, y) = output(x, y); 

  if (get_target().has_feature(Target::CLCadence)) {
  
    input.in()
         .compute_at(final_output, xout)
         .dma_copy(_1);  
         
    Lcoeff.compute_at(final_output, xout)
          .bound(x, 0, 1600)
          .dma_copy(x);

    tupleFunc.compute_at(output, x)
             .vectorize(x, 32)
             .reorder(y, x)
             .store_in(MemoryType::Register);

    tupleFunc.update(0)
             .reorder(y, x)
             .vectorize(x, 32);

    output.compute_at(final_output, xout)
          .reorder(y, x)
          .vectorize(x, 32);

    final_output.compute_root()
                .tile(x, y, xout, yout, xin, yin, 128, 64)
                .gpu_blocks(xout, yout)
                .dma_copy(yin);
    }
  }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(Bilateral5x5, bilateral5x5)
