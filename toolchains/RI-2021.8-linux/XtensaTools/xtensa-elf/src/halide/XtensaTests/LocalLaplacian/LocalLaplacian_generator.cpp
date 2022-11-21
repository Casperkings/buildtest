#include "Halide.h"

#define IMAGE_W       512
#define IMAGE_H       512
#define SRC_BIT_DEPTH 8
#define DST_BIT_DEPTH 8
#define TILE_W        128
#define TILE_H        64
#define W_EXT         1
#define H_EXT         1
#define IMAGE_PAD     1
#define Level         8

using namespace Halide;
using namespace Halide::ConciseCasts;

class InputGaussian : public Halide::Generator<InputGaussian> {
public:
  Input<Buffer<uint8_t>>  input{"input", 2};
  Output<Buffer<uint8_t>> InputGaussian_output{"InputGaussian_output", 2};
  
  void generate() {
    Func blur_x("blur_x"), input_padded("input_padded"), IM_output("IM_output");
    Var x("x"), y("y"), xi("xi"), yi("yi");
    
    input_padded(x, y) = BoundaryConditions::repeat_edge(input)(x, y);

    blur_x(x, y) = (u16(input_padded(2*x-1, y))) * 1 +
                   (u16(input_padded(2*x, y)))   * 2 +
                   (u16(input_padded(2*x+1, y))) * 1;

    IM_output(x, y) = u8((blur_x(x, 2*y-1) * 1 +
                          blur_x(x, 2*y)   * 2 +
                          blur_x(x, 2*y+1) * 1) >> 4);

    InputGaussian_output(x, y) = IM_output(x, y);

    if (get_target().has_feature(Target::CLCadence)) {
      input_padded.compute_root();

      input_padded.in()
                  .compute_at(InputGaussian_output, x)
                  .dma_copy(y);

      blur_x.compute_at(IM_output, xi)
            .vectorize(x, 32)
            .store_in(MemoryType::Register);

      IM_output.compute_at(InputGaussian_output, x)
               .tile(x, y, xi, yi, TILE_W, TILE_H)
               .vectorize(xi, 32)
               .reorder(yi, xi);

      InputGaussian_output.compute_root()
                          .tile(x, y, xi, yi, TILE_W, TILE_H)
                          .gpu_blocks(x, y)
                          .reorder(xi, yi, x, y)
                          .dma_copy(yi);
    }
  }
};

HALIDE_REGISTER_GENERATOR(InputGaussian, InputGaussian)

// ContrastLevel
namespace {
class ContrastLevel : public Halide::Generator<ContrastLevel> {
public:
  Input<Buffer<uint8_t>> src_{"src_", 2};
  Input<Buffer<int16_t>> remap_table{"remap_table", 2};
  Output<Buffer<int16_t>> cont_output{"cont_output", 3};
  
  void generate(){
    Func IM_output("IM_output");
    Var x("x"), y("y"), l("l"), xi("xi"), yi("yi");

    Expr xGray = u16(src_(x, y)) * 8 + l;
    IM_output(x, y, l) = remap_table(xGray, 0);
    cont_output(x, y, l) = IM_output(x, y, l);

    if (get_target().has_feature(Target::CLCadence)) {
      src_.in()
          .compute_at(IM_output, x)
          .store_in(MemoryType::Xt_Shared)
          .dma_copy(_1);
   
      remap_table.in()
                 .compute_at(IM_output, x)
                 .store_in(MemoryType::Xt_Shared)
                 .dma_copy(_0);

      IM_output.compute_at(cont_output, x)
               .tile(x, y, xi, yi, TILE_W, TILE_H)
               .reorder(yi, xi, l, x, y)
               .unroll(l,4)
               .vectorize(xi, 32);
                  
      cont_output.compute_root()
                 .tile(x, y, xi, yi, TILE_W, TILE_H)
                 .bound(l, 0, 8)
                 .gpu_blocks(x, y)
                 .reorder(xi, yi, l, x, y)
                 .unroll(l)
                 .dma_copy(yi);
    }
  }  
};

}  // namespace

HALIDE_REGISTER_GENERATOR(ContrastLevel, ContrastLevel)

// GaussianPyr
namespace {
class GaussianPyr : public Halide::Generator<GaussianPyr> {
public:
  Input<Buffer<int16_t>> src_{"src_", 3};
  Output<Buffer<int16_t>> Gaussian_output{"Gaussian_output", 3};
  
  void generate(){
    Func blur_x("blur_x"), IM_output("IM_output"), input_padded("input_padded");
    Var x("x"), y("y"), l("l"), xi("xi"), yi("yi");

    input_padded(x, y, l) = BoundaryConditions::repeat_edge(src_)(x, y, l);
    
    blur_x(x, y, l) = input_padded(2*x-1, y, l) * 1 +
                      input_padded(2*x, y,   l) * 2 +
                      input_padded(2*x+1, y, l) * 1;

    IM_output(x, y, l) = i16((blur_x(x, 2*y-1, l) * 1 +
                              blur_x(x, 2*y,   l) * 2 + 
                              blur_x(x, 2*y+1, l) * 1) >> 4);

    Gaussian_output(x, y, l) = IM_output(x, y, l);

    if (get_target().has_feature(Target::CLCadence)) {

      input_padded.compute_root();

      input_padded.in()
                  .compute_at(Gaussian_output, x)
                  .bound(l, 0, 8)
                  .unroll(l)
                  .dma_copy(y);

      blur_x.compute_at(IM_output, xi)
            .vectorize(x, 32)
            .store_in(MemoryType::Register);

      IM_output.compute_at(Gaussian_output, x)
               .tile(x, y, xi, yi, TILE_W/2, TILE_H/2)
               .vectorize(xi, 32)
               .bound(l, 0, 8)
               .reorder(yi, xi, l);

      Gaussian_output.compute_root()
                     .tile(x, y, xi, yi, TILE_W/2, TILE_H/2)
                     .gpu_blocks(x, y)
                     .reorder(xi, yi, l, x, y)
                     .bound(l, 0, 8)
                     .unroll(l)
                     .dma_copy(yi);
    }
  }  
};

}  // namespace

HALIDE_REGISTER_GENERATOR(GaussianPyr, GaussianPyr)

// LaplacianPyr
namespace {
class LaplacianPyr : public Halide::Generator<LaplacianPyr> {
public:
  Input<Buffer<int16_t>> src_1{"src_1", 3};
  Input<Buffer<int16_t>> src_2{"src_2", 3};
  Output<Buffer<int16_t>> Laplacian_output{"Laplacian_output", 3};
  
  void generate(){
    Func src_2_padded("src_2_padded"), upsample("upsample"), 
         upsampleX("upsampleX"), IM_output("IM_output");
    
    Var x("x"), y("y"), l("l"), xi("xi"), yi("yi");

    src_2_padded(x, y, l) = BoundaryConditions::repeat_edge(src_2)(x, y, l);

    upsampleX(x, y, l) = src_2_padded((x/2), (y), l) +
                         src_2_padded(((x/2) + (x % 2)), (y), l);
  
    upsample(x, y, l) = src_1(x, y, l) - 
                        ((upsampleX(x, (y/2), l) +
                          upsampleX(x, ((y/2) + (y % 2)),  l)) >> 2);
                            
    Laplacian_output(x, y, l) = upsample(x, y, l);

    if (get_target().has_feature(Target::CLCadence)) {
     
      src_1.in()
           .compute_at(Laplacian_output, x)
           .bound(_2, 0, 8)
           .unroll(_2)
           .dma_copy(_1);

      src_2_padded.compute_root();

      src_2_padded.in()
                  .compute_at(Laplacian_output, x)
                  .bound(l, 0, 8)
                  .unroll(l)
                  .dma_copy(y);
          
      upsampleX.compute_at(upsample, x)
               .store_in(MemoryType::Register)
               .vectorize(x, 64)
               .reorder(y, x, l);

      upsample.compute_at(Laplacian_output, x)
              .vectorize(x, 64)
              .reorder(y, x, l).unroll(y,2)
              .store_in(MemoryType::Xt_Shared);

      Laplacian_output.compute_root()
                      .tile(x, y, xi, yi, TILE_W, TILE_H/2)
                      .gpu_blocks(x, y)
                      .bound(l, 0, 8)
                      .unroll(l)
                      .reorder(xi, yi, l, x, y)
                      .dma_copy(yi);
    }
  }  
};

}  // namespace

HALIDE_REGISTER_GENERATOR(LaplacianPyr, LaplacianPyr)

// FuseLaplacianPyr
namespace {
class FuseLaplacianPyr : public Halide::Generator<FuseLaplacianPyr> {
public:
  Input<Buffer<int16_t>> src_1{"src_1", 3};
  Input<Buffer<uint8_t>> src_2{"src_2", 2};
  Output<Buffer<int16_t>> FuseLaplacian_output{"FuseLaplacian_output", 2};
  
  void generate() {
    Func IM_output("IM_output"), src_2_16("src_2_16");
    Var x("x"), y("y"), l("l"), xi("xi"), yi("yi");

    src_2_16(x, y) = i16(src_2(x, y));
    
    Expr level = i16(src_2_16(x, y)) * (Level-1);
    Expr li    = max(0, min(Level-2, level >> 8));
    Expr lf    = (level - (i16(li) << 8));
    Expr a     = ((i16(1) << 8) - (lf)) * i32(src_1(x, y, li));
    Expr b     = i32(lf) * i32(src_1(x, y, li + 1));

    IM_output(x, y) = i16((a + b) >> 8) ;
    
    FuseLaplacian_output(x, y) = IM_output(x, y);

    if (get_target().has_feature(Target::CLCadence)) {
                  
      src_1.in()
           .compute_at(FuseLaplacian_output, x)
           .bound(_2, 0, 8)
           .unroll(_2)
           .dma_copy(_1);

      src_2.in()
           .compute_at(FuseLaplacian_output, x)
           .dma_copy(_1);

      IM_output.compute_at(FuseLaplacian_output,x)
               .tile(x, y, xi, yi, TILE_W, TILE_H/2)
               .store_in(MemoryType::Xt_Shared)
               .vectorize(xi, 32)
               .reorder(yi, xi);
      
      FuseLaplacian_output.compute_root()
                          .tile(x, y, xi, yi, TILE_W, TILE_H/2)
                          .gpu_blocks(x, y)
                          .reorder(xi, yi, x, y)
                          .dma_copy(yi);
    }
  }  
};
}  // namespace

HALIDE_REGISTER_GENERATOR(FuseLaplacianPyr, FuseLaplacianPyr)

//FusedPyrUpSample
namespace {
class FusedPyrUpSample : public Halide::Generator<FusedPyrUpSample> {
public:
  Input<Buffer<int16_t>> src_1{"src_1", 2};
  Input<Buffer<int16_t>> src_2{"src_2", 2};
  Output<Buffer<int16_t>> FusedPyrUpSample_output{"FusedPyrUpSample_output", 2};
  
  void generate() {
    Func blur_x("blur_x"), IM_output("IM_output"),
         src_2_padded_1("src_2_padded_1"), upsample("upsample");
    
    Var x("x"), y("y"), l("l"), xi("xi"), yi("yi");

    src_2_padded_1(x, y) = BoundaryConditions::repeat_edge(src_2)(x, y);
   
    upsample(x, y) = 
      ((src_2_padded_1((x/2), (y/2)) +
        src_2_padded_1(((x/2) + (x % 2)), (y/2)) +
        src_2_padded_1((x/2), (((y/2) + (y % 2)))) +
        src_2_padded_1(((x/2) + (x % 2)), (((y/2) + (y % 2))))) >> 2);

    IM_output(x, y) = src_1(x, y) + upsample(x, y);
    
    FusedPyrUpSample_output(x, y) = IM_output(x, y);

    if (get_target().has_feature(Target::CLCadence)) {

      src_1.in()
           .compute_at(FusedPyrUpSample_output, x)
           .dma_copy(_1);

      src_2_padded_1.compute_root();
      src_2_padded_1.in()
                    .compute_at(FusedPyrUpSample_output, x)
                    .dma_copy(y);

      IM_output.compute_at(FusedPyrUpSample_output, x)
               .tile(x, y, xi, yi, TILE_W/2, TILE_H/2)
               .store_in(MemoryType::Xt_Shared)
               .vectorize(xi, 64)
               .reorder(xi, yi);
               
      FusedPyrUpSample_output.compute_root()
                             .tile(x, y, xi, yi, TILE_W/2, TILE_H/2)
                             .gpu_blocks(x, y)
                             .reorder(xi, yi, x, y)
                             .dma_copy(yi);
    }
  }  
};
}  // namespace

HALIDE_REGISTER_GENERATOR(FusedPyrUpSample, FusedPyrUpSample)

//FusedPyrUpSampleClip
namespace {
class FusedPyrUpSampleClip : public Halide::Generator<FusedPyrUpSampleClip> {
public:
  Input<Buffer<int16_t>> src_1{"src_1", 2};
  Input<Buffer<int16_t>> src_2{"src_2", 2};
  Output<Buffer<uint8_t>> FusedPyrUpSampleClip_output{"FusedPyrUpSampleClip_output", 2};

  void generate(){
    Func src_2_padded_2("src_2_padded_2"), IM_output("IM_output"),
         upsample("upsample");
    
    Var x("x"), y("y"), l("l"), xi("xi"), yi("yi");

    src_2_padded_2(x, y) = BoundaryConditions::repeat_edge(src_2)(x, y);
    
    upsample(x,y) = 
        ((src_2_padded_2((x/2), (y/2)) +
          src_2_padded_2(((x/2) + (x % 2)), (y/2)) +
          src_2_padded_2((x/2), (((y/2) + (y % 2)))) +
          src_2_padded_2(((x/2) + (x % 2)), (((y/2) + (y % 2))))) >> 2);
                      
    IM_output(x, y) = u8(max(i16(0), min(i16(255), 
                                         src_1(x, y) + upsample(x, y))));

    FusedPyrUpSampleClip_output(x, y) = IM_output(x, y);

    if (get_target().has_feature(Target::CLCadence)) {
      src_1.in()
           .compute_at(FusedPyrUpSampleClip_output, x)
           .store_in(MemoryType::Xt_Shared)
           .dma_copy(_1);

      src_2_padded_2.compute_root();

      src_2_padded_2.in()
                    .compute_at(FusedPyrUpSampleClip_output, x)
                    .store_in(MemoryType::Xt_Shared)
                    .dma_copy(y);

      IM_output.compute_at(FusedPyrUpSampleClip_output, x)
               .tile(x, y, xi, yi, TILE_W/2, TILE_H/2)
               .vectorize(xi, 64)
               .reorder(yi, xi);

      FusedPyrUpSampleClip_output.compute_root()
                                 .tile(x, y, xi, yi, TILE_W/2, TILE_H/2)
                                 .gpu_blocks(x, y)
                                 .reorder(xi, yi, x, y)
                                 .dma_copy(yi);
  
    }
  }  
};
}  // namespace

HALIDE_REGISTER_GENERATOR(FusedPyrUpSampleClip, FusedPyrUpSampleClip)
