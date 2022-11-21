#include "Halide.h"

using namespace Halide;
using namespace Halide::ConciseCasts;

#define Q15_0_2126 6966
#define Q15_0_7152 23436
#define Q15_0_0722 2366
#define Q15_0_5389 17659
#define Q15_0_6350 20808

namespace {
class RGB2YUV420 : public Halide::Generator<RGB2YUV420> {
public:
  
  Input<Buffer<uint8_t>>  input{"input", 3};
  Output<Buffer<uint8_t>> output{"output", 3};

  void generate() {
    Func colorY("colorY"),
         IM_output("IM_output"),
         temp_planar("temp_planar"),
         temp_planar_16b("temp_planar_16b") ;
    
    Var x("x"), y("y"), c("c"), xi("xi"), yi("yi");
  
    temp_planar(x, y, c)  = select(c == 0, input(0, x, y),
                                   c == 1, input(1, x, y),
                                   c == 2, input(2, x, y), 
                                   (uint16_t)0);
    
    temp_planar_16b(x, y, c) = u16(temp_planar(x, y, c));

    Func R("R"), G("G"), B("B");
    R(x, y) = temp_planar_16b(x, y, 0);
    G(x, y) = temp_planar_16b(x, y, 1);
    B(x, y) = temp_planar_16b(x, y, 2);
    Expr xconst = cast(UInt(32), (1 << 14));

    colorY(x, y) = (xconst + (u32(R(x, y)) * Q15_0_2126) + 
                             (u32(G(x, y)) * Q15_0_7152) + 
                             (u32(B(x, y)) * Q15_0_0722)) >> 15 ;
    
    Func Y_out("Y_out"), U_out("U_out"), V_out("V_out");
    Y_out(x, y) = cast(UInt(16), colorY(x, y));

    Func Y_sum ("Y_sum"), R_sum("R_sum"), B_sum("B_sum");

    Y_sum(x, y) = Y_out(x*2 , y*2) + Y_out(x*2+1,y*2) + 
                  Y_out(x*2, y*2 +1) + Y_out(x*2+1,y*2+1);
    R_sum(x, y) = R(x*2, y*2) + R(x*2+1, y*2) + 
                  R(x*2, y*2 +1) + R(x*2+1, y*2+1);
    B_sum(x, y) = B(x*2, y*2) + B(x*2+1, y*2) + 
                  B(x*2, y*2 +1) + B(x*2+1, y*2+1);
  
    Expr U_temp = i16(B_sum(x, y) - Y_sum(x, y));
    Expr V_temp = i16(R_sum(x, y) - Y_sum(x, y));

    Expr xconstuv = cast(UInt(32), 65535);
  
    U_out(x, y) = cast(UInt(16), 
                       (((xconstuv + (U_temp) * u32(Q15_0_5389)) >> 17))) + 128;
    V_out(x, y) = cast(UInt(16), 
                       (((xconstuv + (V_temp) * u32(Q15_0_6350)) >> 17))) + 128;

    IM_output(x, y, c) = cast(UInt(8), select(c == 0, Y_out(x, y),
                                              c == 1, U_out(x, y), 
                                              c == 2, V_out(x, y),
                                              0));

    output(x, y, c) = IM_output(x, y, c);

    if (get_target().has_feature(Target::CLCadence)) {
      input.dim(0).set_bounds(0,3).set_stride(1);

#if 0
      input.in()
           .compute_at(output, x)
           .dma_copy(_2);
      
      temp_planar.compute_at(output, x)
                 .reorder(c, y, x)
                 .unroll(c)
                 .vectorize(x, 32);

      Y_out.compute_at(output, x)
           .vectorize(x, 32)
           .reorder(y, x);   

      U_out.compute_at(output, x)
           .vectorize(x, 32)
           .reorder(y, x); 

      V_out.compute_at(output, x)
           .vectorize(x, 32)
           .reorder(y, x);

      IM_output.compute_at(output, x)
               .vectorize(x, 32)
               .reorder(y, x);
#endif

      output.compute_root()
            .tile(x, y, xi, yi, 128, 64)
            .reorder(xi, yi, c, x, y)
            .bound(c, 0, 3)
            .unroll(c)
            .gpu_blocks(x, y)
            ;//.dma_copy(yi);
    } 
  }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(RGB2YUV420, rgb2yuv420)
