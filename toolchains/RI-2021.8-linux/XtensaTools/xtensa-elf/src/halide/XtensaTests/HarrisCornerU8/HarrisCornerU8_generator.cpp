#include "Halide.h"

using namespace Halide;
using namespace Halide::ConciseCasts;
namespace {
class HarrisCornerU8 : public Halide::Generator<HarrisCornerU8> {
public:
  
  Input<Buffer<uint8_t>>  input{"input", 2};
  Output<Buffer<uint8_t>> output{"output", 2};

  void generate() {
    Func IM_output("IM_output"), input_cast("input_cast"), subX("subX"), 
         subY("subY"), resX("resX"), resY("resY"),
         DxDy("DxDy"), Dx2("Dx2"), Dy2("Dy2"), Dx2Acc("Dx2Acc"), 
         Dy2Acc("Dy2Acc"), DxDyAcc("DxDyAcc"),
         DxSq("DxSq"), DySq("DySq"), Dx_Dy("Dx_Dy");

    Var x("x"), y("y"), xi("xi"), yi("yi");

    // Algorithm.
    input_cast(x, y) = cast(Int(16), input(x,y));
      
    subX(x, y) = - input_cast(x, y) + input_cast(x+2, y);
      
    subY(x, y) = input_cast(x, y) + 2*input_cast(x+1, y) + input_cast(x+2, y);
      
    resX(x, y) = i8(( subX(x, y) + 2*subX(x, y+1) + subX(x, y+2)) >> 3);
      
    resY(x, y) = i8((- subY(x, y) + subY(x, y+2))>>3);
      
    DxSq(x, y) = i32(i16(resX(x, y)) * i16(resX(x, y)));
    Dx2(x, y) = DxSq(x, y) + DxSq(x+1, y) + DxSq(x+2, y)  + DxSq(x+3, y) + 
                DxSq(x+4, y);    
    DySq(x, y) = i32(i16(resY(x, y)) * i16(resY(x, y)));
    Dy2(x, y) = DySq(x, y) + DySq(x+1, y) + DySq(x+2, y) + DySq(x+3, y) + 
                DySq(x+4, y);
    Dx_Dy(x, y) = i32(i16(resY(x, y))) * i32(i16(resX(x, y)));
    DxDy(x, y) = Dx_Dy(x, y) + Dx_Dy(x+1, y) + Dx_Dy(x+2, y) + Dx_Dy(x+3, y)+ 
                 Dx_Dy(x+4, y);
    
    Dx2Acc(x, y) = Dx2(x, y) + Dx2(x, y + 1) + Dx2(x, y + 2) + Dx2(x, y + 3) +
                   Dx2(x, y + 4);
    Dy2Acc(x,y) = Dy2(x, y) + Dy2(x, y + 1) + Dy2(x, y + 2) + Dy2(x, y + 3) + 
                  Dy2(x, y + 4);
    DxDyAcc(x,y) = DxDy(x, y) + DxDy(x, y + 1) + DxDy(x, y + 2) + 
                   DxDy(x, y + 3) + DxDy(x, y + 4);
    
    Expr Dx_2 = i16(Dx2Acc(x, y) >> 6);
    Expr Dy_2 = i16(Dy2Acc(x, y) >> 6);
    Expr DxDy_ = i16(DxDyAcc(x, y) >> 6);
    
    Expr SumDxDy = Dx_2 + Dy_2;
    Expr Acc = i32(Dx_2) * i32(Dy_2) - i32(DxDy_) * i32(DxDy_) -
               i32(SumDxDy >> 5);
    Expr Result = i16(Acc >> 8);
    
    IM_output(x, y) = u8(select(Result < 0, 0,
                                Result > 255, 255,
                                Result));
         
    output(x, y) = IM_output(x, y);

    if (get_target().has_feature(Target::CLCadence)) {
      input.in()
           .compute_at(output, x)
           .dma_copy(_1);

      resX.compute_at(output, x)
          .vectorize(x, 32)
          .align_storage(x, 64)
          .reorder(y, x)
          .store_in(MemoryType::Register);
              
      resY.compute_at(output, x)
          .compute_with(resX, y)
          .vectorize(x, 32)
          .align_storage(x, 64)
          .reorder(y, x)
          .store_in(MemoryType::Register);

      Dx2.compute_at(IM_output, x)
         .vectorize(x, 32)
         .align_storage(x, 64)
         .store_in(MemoryType::Register);
       
      Dy2.compute_at(IM_output, x)
         .compute_with(Dx2, y)
         .vectorize(x, 32)
         .align_storage(x, 64)
         .store_in(MemoryType::Register);
       
      DxDy.compute_at(IM_output, x)
          .compute_with(Dx2, y)
          .vectorize(x, 32)
          .align_storage(x, 64)
          .store_in(MemoryType::Register);

      IM_output.compute_at(output, x)
               .vectorize(x, 32)
               .reorder(y, x);
               
      output.compute_root()
            .tile(x, y, xi, yi, 128, 64)
            .gpu_blocks(x, y)  
            .dma_copy(yi);
    }
  }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(HarrisCornerU8, HarrisCornerU8)
