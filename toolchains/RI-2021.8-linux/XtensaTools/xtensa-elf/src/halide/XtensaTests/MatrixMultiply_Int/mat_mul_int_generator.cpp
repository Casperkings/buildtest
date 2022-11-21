#include "Halide.h"
using namespace Halide;
namespace {
class MatMulInt : public Halide::Generator<MatMulInt> {
public:
  
  // Input and output Buffer  
  Input<Buffer<uint8_t>>   a{"MatA", 2};
  Input<Buffer<uint8_t>>   b{"MatB", 2};  
  Output<Buffer<uint32_t>> c{"MatC", 2};

  void generate() {
    
    // Variable declaration 
    Var x("x"), y("y"), ch("ch"), x_outer("x_outer"),
        y_outer("y_outer"), x_inner("x_inner"), y_inner("y_inner"),
        xs("xs"), ys("ys");

    // Function declaration
    Func sumInter("sumInter"), temp("temp");

    // Reduction Domain declaration
    RDom k(0, 64, 0, 128, 0, 64, "j");
    RVar kin;
    
    // Funtion definition 
    sumInter(k.x, k.z) += (cast(UInt(32),
                           (cast(UInt(16), a(k.y, k.z)) *
                            cast(UInt(16), b(k.x, k.y)))));

    temp(xs, ys) = sumInter(xs, ys);
    c(xs, ys) = temp(xs, ys);

    if (get_target().has_feature(Target::CLCadence)) {
    
      a.in(sumInter)
       .compute_at(c, x_outer)
       .dma_copy(_1);

      b.in(sumInter)
       .compute_at(c, x_outer)
       .dma_copy(_1);

      // refers to initialization step
      sumInter.compute_at(temp, x_outer)
              .vectorize(k.x, 64)
              .store_in(MemoryType::Register); 

      // refers to actual funtion    
      sumInter.update(0)
              .vectorize(k.x, 32)
              .unroll(k.x)
              .split(k.y, k.y, kin, 2)
              .unroll(kin);

      temp.tile(xs, ys, x_outer, y_outer, x_inner, y_inner, 64, 64)
          .vectorize(x_inner, 32)
          .compute_at(c, x_outer);

      c.compute_root()
       .tile(xs, ys, x_outer, y_outer, x_inner, y_inner, 64, 64)    
       .gpu_blocks(x_outer, y_outer)
       .dma_copy(y_inner);
   }
  }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(MatMulInt, mat_mul_int)
