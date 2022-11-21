#include "Halide.h"

using namespace Halide;
using namespace Halide::ConciseCasts;

namespace {
class IntegralU8U32 : public Halide::Generator<IntegralU8U32> {
public:
  Input<Buffer<uint8_t>>  input{"input", 4};
  Output<Buffer<uint32_t>> output{"output", 4};
  
  void generate() {
    Func RowWiseSum("RowWiseSum");
    Func temp_output("temp_output");
    Func input_padded("input_padded");
    Var x("x"), y("y"), xi("xi"), yi("yi"), tx("tx"), ty("ty"), l("l");
   
    RDom r(1, 128, 1, 64), r1(0, 1, 1, 64), r2(1, 128, 0, 1);

    input_padded(x, y, tx, ty) =
        BoundaryConditions::constant_exterior(input, 0)(x, y, tx, ty);

    //RowWiseSum(x, y, tx, ty) = undef<uint32_t>();
    //RowWiseSum(r1[0], r1[1], tx, ty) = u32(0);
    //RowWiseSum(r[0], r[1], tx, ty) = RowWiseSum(r[0]-1, r[1], tx, ty) + 
    //                                 u32(input(r[0]-1, r[1]-1, tx, ty));
                   
    Expr Input_l1 = select((u16(x) & u16(0x1F)) > u16(0),
                            u16(input_padded(x-1, y, tx, ty)), 0);
    Expr Input_l2 = select((u16(x) & u16(0x1F)) > u16(1), 
                           u16(input_padded(x-2, y, tx, ty)), 0);
    Expr Input_l3 = select((u16(x) & u16(0x1F)) > u16(2),
                           u16(input_padded(x-3, y, tx, ty)), 0);
  
    Func RowSumTemp("RowSumTemp");
    RowSumTemp(x, y, tx, ty) = Input_l1 + Input_l2 + Input_l3 +
                               u16(input_padded(x, y, tx, ty));
  
    Func RowSumTemp1("RowSumTemp1"), RowSumTemp2("RowSumTemp2"), 
         RowSumTemp3("RowSumTemp3");
  
    RowSumTemp1(x, y, tx, ty) = RowSumTemp(x, y, tx, ty) +
                                select((u16(x) & u16(0x1F)) > u16(3),
                                       RowSumTemp(x-4, y, tx, ty), 0);
    RowSumTemp2(x, y, tx, ty) = RowSumTemp1(x, y, tx, ty) +
                                select((u16(x) & u16(0x1F)) > u16(7),
                                       RowSumTemp1(x-8, y, tx, ty), 0);
    RowSumTemp3(x, y, tx, ty) = RowSumTemp2(x, y, tx, ty) +
                                select((u16(x) & u16(0x1F)) > u16(15),
                                       RowSumTemp2(x-16, y, tx, ty), 0);
  
    RDom r4(32, 128, 0, 64);
    RowSumTemp3(r4[0], r4[1], tx, ty) = 
        RowSumTemp3(r4[0], r4[1], tx, ty) +
        RowSumTemp3(((r4[0]/32)*32)-1, r4[1], tx, ty);

    temp_output(x, y, tx, ty) = undef<uint32_t>();
    temp_output(r2[0], r2[1], tx, ty) = u32(0);
    temp_output(r[0], r[1], tx, ty) =
         u32(RowSumTemp3(r[0]-1, r[1]-1, tx, ty)) + 
         temp_output(r[0], r[1]-1, tx, ty);
    
    // Single pass integral
    // temp_output(r[0], r[1], tx, ty) = u32(input(r[0]-1, r[1]-1, tx, ty)) + 
    //                                   temp_output(r[0], r[1]-1, tx, ty) +  
    //                                   temp_output(r[0]-1, r[1], tx, ty) - 
    //                                   temp_output(r[0]-1, r[1]-1, tx, ty); 
     
    output(x, y, tx, ty) = temp_output(x+1, y+1, tx, ty);

    if (get_target().has_feature(Target::CLCadence)) {
      
      input_padded.compute_root();
      
      input_padded.in()
                  .compute_at(output, tx)
                  .dma_copy(y)
                  .store_in(MemoryType::Xt_Shared);     

      RowSumTemp.compute_at(output, tx)
                .store_in(MemoryType::Register)
                .vectorize(x, 32)
                .reorder(y, x);
         
      // RowSumTemp1.compute_at(output, tx)
      //            .store_in(MemoryType::Register)
      //            .vectorize(x, 32)
      //            .reorder(y, x);
  
      RowSumTemp2.compute_at(output, tx)
                 .store_in(MemoryType::Register)
                 .vectorize(x, 32)
                 .reorder(y, x);
         
      RowSumTemp3.compute_at(output, tx)
                 .store_in(MemoryType::Register)
                 .bound(x, 0, 128)
                 .bound(y, 0, 64)
                 .vectorize(x, 32)
                 .reorder(y, x);
         
      RowSumTemp3.compute_at(output, tx)
                 .store_in(MemoryType::Register)
                 .bound(x, 0, 128+32)
                 .bound(y, 0, 64)
                 .update(0)
                 .allow_race_conditions()
                 .vectorize(r4[0], 32)
                 .reorder(r4[1], r4[0])
                 .unroll(r4[1], 4);
         
      temp_output.compute_at(output, tx)
                 .bound(x, 0, 129)
                 .bound(y, 0, 65)
                 .update(0)
                 .vectorize(r2[0], 32)
                 .unroll(r2[1]);
      
      temp_output.compute_at(output, tx)
                 .bound(x, 0, 129)
                 .bound(y, 0, 65)
                 .update(1)
                 .vectorize(r[0], 32)
                 .reorder(r[1], r[0]);

      output.compute_root()
            .gpu_blocks(tx, ty)
            .dma_copy(y);
    }
  }
};
}

HALIDE_REGISTER_GENERATOR(IntegralU8U32, IntegralU8U32)
