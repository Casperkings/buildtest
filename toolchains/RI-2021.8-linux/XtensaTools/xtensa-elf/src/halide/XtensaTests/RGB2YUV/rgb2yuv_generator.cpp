#include "Halide.h"

using namespace Halide;
using namespace Halide::ConciseCasts;

#define Q15_0_2126 6966
#define Q15_0_7152 23436
#define Q15_0_0722 2366
#define Q15_0_5389 17659
#define Q15_0_6350 20808

namespace {
class RGB2YUV : public Halide::Generator<RGB2YUV> {
public:
  
  Input<Buffer<uint8_t>>  input{"input", 3};
  Output<Buffer<uint8_t>> output{"output", 3};

  void generate() {
    Func colorY("colorY"),
         IM_output("IM_output"),
         temp_input("temp_input"),
         temp_planar("temp_planar");
    
    Var x("x"), y("y"), c("c"), xi("xi"), yi("yi");

    temp_planar(x, y, c)  = select(c == 0, input(0, x, y),
                                   c == 1, input(1, x, y),
                                   c == 2, input(2, x, y),
                                   (uint16_t) 0
                                );
    
    Func temp_planar_2("temp_planar_2");
    temp_planar_2(x, y, c) = u16(temp_planar(x, y, c));

    // Y Part 
    Expr R = temp_planar_2(x, y, 0);
    Expr G = temp_planar_2(x, y, 1);
    Expr B = temp_planar_2(x, y, 2);
    Expr xconst = cast(UInt(32), (1 << 14));

    colorY(x, y) = (xconst + (u32(R) * Q15_0_2126) + (u32(G) * Q15_0_7152) + 
                    (u32(B) * Q15_0_0722)) >> 15 ;
    
    // Y Part
    Func Y_out("Y_out"), U_out("U_out"), V_out("V_out");
    Y_out(x, y) = cast(UInt(16), colorY(x, y));

    Expr U_temp = i16(B - Y_out(x, y));
    Expr V_temp = i16(R - Y_out(x, y));

    // U Part
    U_out(x, y) = cast(UInt(16), 
                       (((xconst + (U_temp) * u32(Q15_0_5389)) >> 15))) + 128;
    // V_Part
    V_out(x, y) = cast(UInt(16), 
                       (((xconst + (V_temp) * u32(Q15_0_6350)) >> 15))) + 128;

    IM_output(x, y, c) = cast(UInt(8), select(c == 0, Y_out(x, y),
                                              c == 1, U_out(x, y), 
                                              c == 2, V_out(x, y),
                                              (uint8_t)0));
    output(x, y, c) = IM_output(x, y, c);

    if (get_target().has_feature(Target::CLCadence)) {
      input.dim(0).set_bounds(0,3).set_stride(1);

      input.in()
           .compute_at(output, x)
           .store_in(MemoryType::Xt_Shared)
           .dma_copy(_2);
      
      temp_planar.compute_at(output, x)
                 .reorder(c, x, y)
                 .vectorize(x, 64)
                 .unroll(c)
                 .store_in(MemoryType::Register);

      Y_out.compute_at(IM_output,y)
           .store_in(MemoryType::Register);

      IM_output.compute_at(output, x)
               .reorder(c, y, x)
               .unroll(c)
               .vectorize(x, 32);

      output.compute_root()
            .tile(x, y, xi, yi, 128, 64)
            .reorder(xi, yi, c, x, y)
            .bound(c, 0, 3)
            .gpu_blocks(x, y)
            .unroll(c)
            .dma_copy(yi);
    } 
  }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(RGB2YUV, rgb2yuv)
