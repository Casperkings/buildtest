#include "Halide.h"

/* Input/Output dimensions     */
#define IMAGE_W       512
#define IMAGE_H       512
#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W        128
#define TILE_H        128
#define OUTPUT_TILE_W 128
#define OUTPUT_TILE_H 64
#define W_EXT         0
#define H_EXT         0
#define IMAGE_PAD     0

using namespace Halide;
using namespace Halide::ConciseCasts;

namespace {
  class Perspective : public Halide::Generator<Perspective> {
  public:
      
    Input<Buffer<uint8_t>> input{"input", 2};
    Input<Buffer<uint32_t>> per{"per", 2};
    Input<Buffer<uint16_t>> InputBB{"InputBB", 3};
    Output<Buffer<uint8_t>> FinalOutput{"FinalOutput", 4};
    
    void generate()
    {
      Var x("x"), y("y"), z("z"), w("w");

      RDom r(0, 3, 0, 2), t(0, 3);
      
      Func ns("ns"), per_t("per_t"), per_p("per_p"), inputTemp("inputTemp"),
           output("output"), D("D"), X("X"), Y("Y"), Qx("Qx"), Qy("Qy");
      
      per_t(x, y) = per(x, y);
      per_p(x, y, z, w) = undef<int32_t>();
      per_p(r.x, r.y, z, w) = cast(Int(32), per_t(r.x+r.y*3, 0) - 
                                            per_t(r.x+6, 0)*InputBB(r.y, z, w) -
                                            per_t(r.x+6, 0)/2);
      per_p(t, 2, z, w) = cast(Int(32), per_t(t+6, 0));

      inputTemp(x, y, z, w) = input(min(x + InputBB(0, z, w), 511), 
                                    min(y + InputBB(1, z, w), 511));
      
      X(x, y, z, w) = u32((x+(z*128)) * per_p(0, 0, z, w) +
                          (y+(w*64)) * per_p(1, 0, z, w) +
                          (per_p(0, 0, z, w) + per_p(1, 0, z, w)) / 2 + 
                          per_p(2, 0, z, w));

      Y(x, y, z, w) = u32((x+(z*128)) * per_p(0, 1, z, w) +
                          (y+(w*64)) * per_p(1, 1, z, w) +
                          (per_p(0, 1, z, w) + per_p(1, 1, z, w)) / 2 +
                          per_p(2, 1, z, w) );      

      D(x, y, z, w) = u32((x+(z*128)) * per_p(0, 2, z, w) +
                          (y+(w*64)) * per_p(1, 2, z, w) +
                          (per_p(0, 2, z, w) + per_p(1, 2, z, w)) / 2 + 
                          per_p(2, 2, z, w));
      
      ns(x, y, z, w) = u32(count_leading_zeros(D(x, y, z, w)));
      Expr as = u32(16) - ns(x, y, z, w) ;

      Expr D1 = D(x, y, z, w) << ns(x, y, z, w);   
      
      Qx(x, y, z, w) = (X(x, y, z, w) / (D1 >> u32(16)));
      Expr Rx = Qx(x, y, z, w) << ns(x, y, z, w) ;
      Expr Q1x = Qx(x, y, z, w) >> as;

      Expr Xint = u16(Q1x);
      Expr Xrem = u16(Rx) >> 1;

      Qy(x, y, z, w) = (Y(x, y, z, w) / (D1 >> u32(16)));
      Expr Ry = Qy(x, y, z, w) << ns(x, y, z, w) ;
      Expr Q1y = Qy(x, y, z, w) >> as;

      Expr Yint = u16(Q1y);
      Expr Yrem = u16(Ry) >> 1;
  
      Expr maxX = u8(110);
      Expr maxY = u8(61);
      
      Expr val00 = u16(inputTemp(clamp(Xint, 0, maxX),
                                 clamp(Yint, 0, maxY), z, w));
      Expr val01 = u16(inputTemp(clamp(Xint+1, 0, maxX), 
                                 clamp(Yint, 0, maxY), z, w));
      Expr val10 = u16(inputTemp(clamp( Xint, 0, maxX), 
                                 clamp(Yint+1, 0, maxY), z, w));
      Expr val11 = u16(inputTemp(clamp(Xint+1, 0, maxX),
                                 clamp(Yint+1, 0, maxY), z, w));
         
      Expr curRowIntplVal = u16(val00 + (((u32(val01 - val00) * u32(Xrem)) + 
                                         u16(0x4000)) >> 15));
      Expr nxtRowIntplVal = u16(val10 + (((u32(val11 - val10 ) * u32(Xrem)) + 
                                u16(0x4000)) >> 15));
      Expr intplValue = u8(u16(curRowIntplVal) + 
                           (((u32(nxtRowIntplVal - curRowIntplVal) * 
                              u32(Yrem)) + u16(0x4000)) >> 15)) & 0xff ;
      
      output(x, y, z, w) =  intplValue;
      FinalOutput(x, y, z, w) = output(x, y, z, w); 

      if (get_target().has_feature(Target::CLCadence)) {             

	      inputTemp.compute_at(FinalOutput,z)
	               .dma_copy(y);

        per_t.compute_at(FinalOutput, z)
             .store_in(MemoryType::Register);

        per_p.compute_at(FinalOutput, z)
             .store_in(MemoryType::Register);  
             
        D.compute_at(output, z)
         .vectorize(x, 16)
         .unroll(x)
         .store_in(MemoryType::Register);
        
        X.compute_at(output, z)
         .vectorize(x, 16)
         .store_in(MemoryType::Register);

        Y.compute_at(output, z)
         .vectorize(x, 16)
         .store_in(MemoryType::Register);
        
        ns.compute_at(output, z)
          .vectorize(x, 16)
          .store_in(MemoryType::Register);
        
        output.compute_at(FinalOutput, z)
              .bound(x, 0, 128)
              .bound(y, 0, 64)
              .reorder(y, x)
              .vectorize(x, 32);

        FinalOutput.compute_root()
                   .gpu_blocks(z, w)
                   .dma_copy(y);
      }
    }
  };

}

HALIDE_REGISTER_GENERATOR(Perspective, perspective);
