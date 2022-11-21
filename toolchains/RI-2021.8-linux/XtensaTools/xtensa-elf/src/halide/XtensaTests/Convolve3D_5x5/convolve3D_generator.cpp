#include "Halide.h"
using namespace Halide;
namespace {
class CONVOLVE3D : public Halide::Generator<CONVOLVE3D> {
public:
  
  Input<Buffer<int8_t>>  input{"input", 3};
  Input<Buffer<int8_t>>  kernel{"kernel", 4};
  Output<Buffer<int8_t>> output{"output", 3};

  void generate() {
    Func IM_output("IM_output"),
         sum_output("sum_output"),
         input_i32("input_i32"),
         temp_input("temp_input"),
         temp_kernel("temp_kernel");
    Var c("c"),
        w("w"),
        h("h"),
        ci("ci"),
        wi("wi"),
        hi("hi"),
        N("N"),
        c_inner("c_inner"),
        w_inner("w_inner"),
        h_inner("h_inner"),
        c_outer("c_outer"),
        w_outer("w_outer"),
        h_outer("h_outer");
    RDom r(0, 5,  //width
           0, 5,  //height
           0, 64);//channel

    sum_output(c, w, h) = cast(Int(32), 0); 
    sum_output(c, w, h) += cast(Int(32),
                                cast(Int(16), (input(r.z, w + r.x , h + r.y))) *
                                kernel(c, r.x, r.y, r.z));
    IM_output(c, w, h) = cast(Int(8), ((sum_output(c, w, h) + (1<<6)) >> 7));

    output(c, w, h) = IM_output(c, w, h);

   if (get_target().has_feature(Target::CLCadence)) {

      input.dim(0).set_bounds(0, 64).set_stride(1);

      kernel.dim(3).set_bounds(0, 64).set_stride(64*5*5);
      kernel.dim(2).set_bounds(0, 5).set_stride(64*5);
      kernel.dim(1).set_bounds(0, 5).set_stride(64);
      kernel.dim(0).set_bounds(0, 64).set_stride(1);

      output.dim(0).set_bounds(0, 64).set_stride(1);

      input.in()
           .compute_at(output, c_outer)
           .dma_copy(_2);
    
      kernel.in()
            .bound(_0, 0, 64)
            .compute_at(output, c_outer)
            .dma_copy(_3);

      sum_output.update(0).vectorize(c, 32);
      sum_output.compute_at(IM_output, ci)
                .vectorize(c, 32)   
                .store_in(MemoryType::Register);

      IM_output.compute_at(output, c_outer)
               .tile(w, h, wi, hi, 10, 5)
               .split(c, c, ci, 64)
               .reorder(ci, wi, hi, c, w, h)
               .vectorize(ci, 32);
 
      output.compute_root()
            .split(h, h_outer, h_inner, 5)
            .split(w, w_outer, w_inner, 10)
            .split(c, c_outer, c_inner, 64)
            .reorder(c_inner, w_inner, h_inner, c_outer, w_outer, h_outer)
            .gpu_blocks(c_outer, w_outer, h_outer)
            .dma_copy(h_inner);
    } 
  }
};
}  // namespace

HALIDE_REGISTER_GENERATOR(CONVOLVE3D, convolve3D)
