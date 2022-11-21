#include "Halide.h"

#define RECT_SIZE	      (32)
#define RECT_SIZE_SHIFT	(5)
#define PIXEL_Q		      (4)
#define PIXEL_DEC	      (1<<(u16(PIXEL_Q)))
#define PIXEL_Q_MASK	  ((1<<u16(PIXEL_Q))-1)
#define PIXEL_Q_MASK_UV	((1<<(u16(PIXEL_Q)+1))-1)

#define MAX(a,b)	      ((a) > (b)?(a):(b))
#define MIN(a,b)	      ((a) < (b)?(a):(b))

using namespace Halide::ConciseCasts;
using namespace Halide;

namespace {
class LDC : public Halide::Generator<LDC> {
public:
  
  Input<Buffer<uint8_t>>  inputY_source{"inputY_source", 2};
  Input<Buffer<uint8_t>>  inputUV_source{"inputUV_source", 2};
  Input<Buffer<uint16_t>> inputX{"inputX", 3};
  Input<Buffer<uint16_t>> inputY{"inputY", 3};
  Input<Buffer<uint16_t>> inputBB{"inputBB", 3};

  Output<Buffer<uint8_t>> final_output{"final_output", 5};

  void generate() {
    Var x("x"), y("y"), w("w"), h("h"), l("l"), 
        TW("TW"), TH("TH"), hi("hi"), wi("wi");

    Func input_srcY("input_srcY"), outputYTemp("outputYTemp"), 
         outputUVTemp("outputUVTemp"), temp_output("temp_output");;

    Func inputxt_0("inputxt_0"), inputxt_1("inputxt_1"), 
         inputxt_2("inputxt_2"), inputxt_3("inputxt_3");
    Func inputyt_0("inputyt_0"), inputyt_1("inputyt_1"), 
         inputyt_2("inputyt_2"), inputyt_3("inputyt_3");

    input_srcY(w, h, TW, TH) = (inputY_source(
                                  clamp((w + inputBB(TW, TH, 0)), 0, 511),
                                  clamp((h + inputBB(TW, TH, 1)), 0, 511)));

    inputxt_0(TW, TH) = u16(inputX(0, TW, TH) - 
                            (inputBB(TW, TH, 0) << u16(PIXEL_Q)));
    inputxt_1(TW, TH) = u16(inputX(1, TW, TH) - 
                            (inputBB(TW, TH, 0) << u16(PIXEL_Q)));
    inputxt_2(TW, TH) = u16(inputX(2, TW, TH) - 
                            (inputBB(TW, TH, 0) << u16(PIXEL_Q)));

    inputxt_3(TW, TH) = u16(inputX(3, TW, TH) - 
                            (inputBB(TW, TH, 0) << u16(PIXEL_Q)));
    inputyt_0(TW, TH) = u16(inputY(0, TW, TH) - 
                            (inputBB(TW, TH, 1) << u16(PIXEL_Q)));
    inputyt_1(TW, TH) = u16(inputY(1, TW, TH) - 
                            (inputBB(TW, TH, 1) << u16(PIXEL_Q)));
    inputyt_2(TW, TH) = u16(inputY(2, TW, TH) - 
                            (inputBB(TW, TH, 1) << u16(PIXEL_Q)));
    inputyt_3(TW, TH) = u16(inputY(3, TW, TH) - 
                            (inputBB(TW, TH, 1) << u16(PIXEL_Q)));

    // Linear interpolation of the coordinates
    Expr x_32 = u16(u16(RECT_SIZE) - u16(w));
    Expr y_32 = u16(u16(RECT_SIZE) - u16(h));

    Expr x0 = u16((x_32 * inputxt_0(TW, TH) + 
                   u16(w) * inputxt_1(TW, TH) + 
                   u16(RECT_SIZE >> 1)) >> u16(5));

    Expr x1 = u16((x_32 * inputxt_2(TW, TH) + 
                   u16(w) * inputxt_3(TW, TH) + 
                   u16(RECT_SIZE >> 1)) >> u16(5));

    Expr x_ = u16((y_32 * u16(x0) + 
                   u16(h) * x1 + 
                   u16(RECT_SIZE >> 1)) >> u16(5));

		Expr y0 = u16((x_32 * inputyt_0(TW, TH) + 
                   u16(w) * inputyt_1(TW, TH) + 
                   u16(RECT_SIZE >> 1)) >> u16(5));

		Expr y1 = u16((x_32 * inputyt_2(TW, TH) + 
                   u16(w) * inputyt_3(TW, TH) + u16(RECT_SIZE >> 1)) >> u16(5));

    Expr y_ = u16((y_32 * y0 + 
                   u16(h) * y1 + 
                   u16(RECT_SIZE >> 1)) >> u16(5));

		Expr x_int = u16(x_ >> u16(PIXEL_Q));
    Expr y_int = u16(y_ >> u16(PIXEL_Q));
    Expr x_dec = u16(x_ &  u16(PIXEL_Q_MASK));
    Expr y_dec = u16(y_ &  u16(PIXEL_Q_MASK));

    Expr pixel_value_0 = u16(input_srcY(u16(x_int),
                                        u16(y_int), TW, TH));
    Expr pixel_value_1 = u16(input_srcY(u16(x_int + u16(1)),
                                        u16(y_int), TW, TH));
    Expr pixel_value_2 = u16(input_srcY(u16(x_int),
                                        u16(y_int + u16(1)), TW, TH));
    Expr pixel_value_3 = u16(input_srcY(u16(x_int + u16(1)),
                                        u16(y_int + u16(1)), TW, TH));

    Expr res0   = u16(((u16(PIXEL_DEC) - x_dec) * pixel_value_0 + 
                      x_dec * pixel_value_1 + u16(PIXEL_DEC >> 1)) >> u16(4));

    Expr res1   = u16(((u16(PIXEL_DEC) - x_dec) * pixel_value_2 + 
                      x_dec * pixel_value_3 + u16(PIXEL_DEC >> 1)) >> u16(4));

    Expr output = u16(((u16(PIXEL_DEC) - y_dec) * res0  + y_dec * res1 + 
                       u16(PIXEL_DEC >> 1)) >> u16(4));
    
    outputYTemp(w, h, TW ,TH) = u8(output);

    // for dest_UV
    RDom t(0,32,0,16);
    Func inputUV("inputUV");
    inputUV(w, h, TW, TH) = (inputUV_source(
                               clamp((w + inputBB(TW, TH, 0)),     0, 511),
                               clamp((h + inputBB(TW, TH, 1) / 2), 0, 255)));
    
    Expr UV_x_32 = u16(u16(RECT_SIZE) - (u16(w) / u16(2)) * u16(2));
    Expr UV_y_32 = u16(u16(RECT_SIZE) * u16(2) - ((u16(h) * u16(4)) + u16(1)));

    Expr UV_x0 = u16((UV_x_32 * inputxt_0(TW, TH) + 
                      u16((u16(w) / u16(2)) * u16(2)) * inputxt_1(TW, TH) + 
                      u16(RECT_SIZE >> 1)) >> u16(5));

    Expr UV_x1 = u16((UV_x_32 * inputxt_2(TW, TH) + 
                     u16((u16(w) / u16(2)) * u16(2)) * inputxt_3(TW, TH) + 
                     u16(RECT_SIZE >> 1)) >> u16(5));

    Expr UV_x  = u16((UV_y_32 * UV_x0 + 
                      u16((u16(h) * u16(4)) + 
                      u16(1)) * UV_x1 + u16(RECT_SIZE)) >> u16(6));

    Expr UV_y0 = u16((UV_x_32 * inputyt_0(TW, TH) + 
                     u16((u16(w) / u16(2)) * u16(2)) * inputyt_1(TW, TH) + 
                     u16(RECT_SIZE >> 1)) >> u16(5));

    Expr UV_y1 = u16((UV_x_32 * inputyt_2(TW, TH) + 
                      u16((u16(w) / u16(2)) * u16(2)) * inputyt_3(TW, TH) + 
                      u16(RECT_SIZE >> 1)) >> u16(5));

    Expr UV_y  = u16((UV_y_32 * UV_y0 + 
                      u16((u16(h) * u16(4)) + u16(1)) * UV_y1 + 
                      u16(RECT_SIZE)) >> u16(6));

    Expr UV_x_int = u16(UV_x >> u16(u16(PIXEL_Q)  + 1));
    Expr UV_y_int = u16(UV_y >> u16(u16(PIXEL_Q)  + 1));
    Expr UV_x_dec = u16(UV_x &  u16(PIXEL_DEC * 2 - 1));
    Expr UV_y_dec = u16(UV_y &  u16(PIXEL_DEC * 2 - 1));
    
    Expr UV_pixel_values_0 = u16(inputUV(u16(UV_x_int * u16(2) + 
                                         (u16(w) % u16(2))), 
                                         u16( UV_y_int), TW, TH));
    Expr UV_pixel_values_1 = u16(inputUV(u16((UV_x_int + u16(1)) * u16(2) + 
                                             (u16(w) % u16(2))), 
                                             u16( UV_y_int), TW, TH));

    Expr UV_pixel_values_2 = u16(inputUV(u16(UV_x_int * u16(2) + 
                                         (u16(w) % u16(2))),
                                         u16( UV_y_int + u16(1)), 
                                         TW, TH));

    Expr UV_pixel_values_3 = u16(inputUV(u16((UV_x_int + u16(1)) * u16(2) + 
                                         (u16(w) % u16(2))), 
                                         u16( UV_y_int + u16(1)), 
                                         TW, TH));

    Expr UV_res0  = u16(((u16(PIXEL_DEC * 2) - UV_x_dec) * UV_pixel_values_0 + 
                         UV_x_dec * UV_pixel_values_1 + 
                         u16(PIXEL_DEC)) >> u16(5));

    Expr UV_res1   = u16(((u16(PIXEL_DEC * 2) - UV_x_dec) * UV_pixel_values_2 +
                           UV_x_dec * UV_pixel_values_3 + 
                           u16(PIXEL_DEC)) >> u16(5));

    Expr UV_output = u16(((u16(PIXEL_DEC * 2) - UV_y_dec) * UV_res0 + 
                           UV_y_dec * UV_res1 + 
                           u16(PIXEL_DEC)) >> u16(5));
    
    outputUVTemp(w, h, TW, TH) = 
                      u8(select((UV_x_int < 0 ||
                                 UV_y_int < 0 ||
                                 UV_x_int >= (inputBB(TW, TH, 2) / 2) - 1 ||
                                 UV_y_int >= (inputBB(TW, TH, 3) / 2) - 1), 
                                 128, UV_output));

    temp_output(w, h, TW, TH, l) = undef<uint8_t>();
    temp_output(w, h, TW, TH, 0) = outputYTemp(w, h, TW, TH);
    temp_output(t[0], t[1], TW, TH, 1) = outputUVTemp(t[0], t[1], TW, TH);

    final_output(w, h, TW, TH, l) = temp_output(w, h, TW, TH, l); 

   if (get_target().has_feature(Target::CLCadence)) {
     // Dst_Y
     input_srcY.compute_at(final_output, TW)
               .bound(w, 0, 35)
               .bound(h, 0, 35)
               .dma_copy(h);

     inputUV.compute_at(final_output, TW)
            .bound(w, 0, 36)
            .bound(h, 0, 17)
            .dma_copy(h);

                
     temp_output.update(0)
                .vectorize(w, 32)
                .reorder(h, w);

     temp_output.update(1)
                .vectorize(t[0], 32)
                .reorder(t[1], t[0]);

     temp_output.compute_at(final_output, TW)
                  .bound(w, 0, 32)
                  .bound(h, 0, 32)
                  .bound(l, 0, 2)
                  .reorder(l, h, w)
                  .unroll(l)
                  .vectorize(w,32);

     final_output.reorder(w, h, l, TW, TH)
                 .bound(w, 0, 32)
                 .bound(h, 0, 32)
                 .bound(l, 0, 2)
                 .unroll(l)
                 .gpu_blocks(TW, TH)
                 .dma_copy(h);
    } 
  }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(LDC, LDC)
