#include "Halide.h"
#include "Complex.h"

using namespace Halide;
using namespace Halide::ConciseCasts;
using std::vector;

typedef FuncRefT<ComplexExpr> ComplexFuncRef;

const ComplexExpr j(Expr(0), Expr(1));

ComplexExpr undef_z() {
  return ComplexExpr(undef(Int(32)), undef(Int(32)));
}

Expr mulpack(Expr a, Expr b) {
  auto mulLo  = a * b;
  auto mulHi  = mul_hi(a, b);
  auto _mulLo = (u32(mulLo) >> 17);
  auto _mulHi = (u32(mulHi) << 15);
  auto or_mul = _mulLo | _mulHi;
  auto res    = i32(or_mul);
  return res;
}

ComplexExpr shifted_mul_17(ComplexExpr a, ComplexExpr b) {
  auto re = mulpack(a.x, b.x) - mulpack(a.y, b.y);
  auto im = mulpack(a.y, b.x) + mulpack(a.x, b.y);
  return ComplexExpr(re, im);
}

vector<ComplexFuncRef> get_func_refs(ComplexFunc x, int N) {
  vector<Var> args(x.args());
  vector<ComplexFuncRef> refs;
  for (int i = 0; i < N; i++) {
    refs.push_back(x(Expr(i), args[1], args[2], args[3], args[4]));
  }
  return refs;
}

ComplexFunc dft4(ComplexFunc &f, std::string name) {
  ComplexFunc F(name + "F");
  F(f.args()) = undef_z();

  vector<ComplexFuncRef> x = get_func_refs(f, 4);
  vector<ComplexFuncRef> X = get_func_refs(F, 4);

  auto T0 = (x[0] + x[2]);
  auto T1 = (x[0] - x[2]);
  auto T2 = (x[1] + x[3]);
  auto T3 = (x[1] - x[3]) * j * -1;

  X[0] = (T0 + T2);
  X[1] = (T1 + T3);
  X[2] = (T0 - T2);
  X[3] = (T1 - T3);

  return F;
}

ComplexFunc fft_vert_top(ComplexFunc &x_in, ComplexFunc &W64, Func output_ph) {
  
  auto args = x_in.args();
  Var n0(args[0]), n1(args[1]), t_x(args[2]), t_y(args[3]);
  
  Var r("r"), s("s");

  ComplexFunc s1_in("s1_in"), s1_out("s1_out"),
              s2_in("s2_in"), s2_out("s2_out"),
              s3_in("s3_in"), s3_out("s3_out"),
              output("output")
              ;
    
  RDom Stage1(0,4,0,16), Stage2(0,4,0,16), Stage3(0,4,0,16);

  auto s1_r_ = Stage1.x; auto s1_s_ = Stage1.y;
  auto s2_r_ = Stage2.x; auto s2_s_ = Stage2.y;
  auto s3_r_ = Stage3.x; auto s3_s_ = Stage3.y;
  
  s1_out(n0, n1, t_x, t_y) = undef_z();
  s2_out(n0, n1, t_x, t_y) = undef_z();
  s3_out(n0, n1, t_x, t_y) = undef_z();
  s1_out.bound(n1, 0, 64);
  s2_out.bound(n1, 0, 64);
  s3_out.bound(n1, 0, 64);
  
  s1_in(r, s, n0, t_x, t_y) = x_in(n0, s + r*16, t_x, t_y) >> 2;
  auto s1_temp = dft4(s1_in, "s1_");
  s1_out(n0, s1_s_*4 + s1_r_, t_x, t_y)  = 
    select(s1_r_ == 0, s1_temp(s1_r_,s1_s_,n0,t_x,t_y) >> 2,
           shifted_mul_17(s1_temp(s1_r_, s1_s_, n0, t_x, t_y), W64(s1_s_*s1_r_))
          );

  s2_in(r, s, n0, t_x, t_y) = s1_out(n0, s + r*16, t_x, t_y);
  auto s2_temp = dft4(s2_in, "s2_");
  s2_out(n0,(s2_s_/4)*16 + s2_s_%4 + s2_r_*4,t_x,t_y) = 
    select(s2_r_ == 0, s2_temp(s2_r_, s2_s_, n0, t_x, t_y) >> 2,
           shifted_mul_17(s2_temp(s2_r_, s2_s_, n0, t_x, t_y), 
                          W64((s2_s_/4)*s2_r_*4))
          );
  
  s3_in(r, s, n0, t_x, t_y) = s2_out(n0, s + r*16, t_x, t_y);
  auto s3_temp = dft4(s3_in, "s3_");
  s3_out(n0, s3_s_%16 + s3_r_*16, t_x, t_y) =
                                  s3_temp(s3_r_, s3_s_, n0, t_x, t_y);
  
  output(n0, n1, t_x, t_y) = s3_out(n0, n1, t_x, t_y);
  
  s1_temp.compute_at(s1_out, s1_s_)
         .store_in(MemoryType::Register);
  
  s1_out.compute_at(output_ph, t_x)
        .store_in(MemoryType::Register)
        .update()
        .unroll(s1_r_)
        .vectorize(n0, 16);
  
  s2_temp.compute_at(s2_out, s2_s_)
         .store_in(MemoryType::Register);
  
  s2_out.compute_at(output_ph, t_x)
        .store_in(MemoryType::Register)
        .update()
        .unroll(s2_r_)
        .vectorize(n0, 16);
  
  s3_temp.compute_at(s3_out, s3_s_)
         .vectorize(n0, 16)
         .store_in(MemoryType::Register);
  
  s3_out.compute_at(output_ph, t_x)
        .update()
        .unroll(s3_r_)
        .vectorize(n0, 16);
  
  return output;
}

namespace {

class FFT2D : public Halide::Generator<FFT2D> {
public:
  
  Input<Buffer<int32_t>>  input_buf{"input", 2};
  Input<Buffer<int32_t>>  W64_buf{"W64", 1};
  Input<Buffer<int32_t>>  W64_horiz_buf{"W64_horiz", 1};
  
  Output<Buffer<int32_t>> FinalOut{"FinalOut", 5};

  void generate() {
  
    Var n0("n0"), n1("n1"), c("c"), t_x("t_x"), t_y("t_y");
    Func input("input"), output("output");
    ComplexFunc W64("W64"), W64_horiz("W64_horiz"), zipped("zipped"),
                v_dft_t("v_dft_t"), h_dft_t("h_dft_t");

    input(n0, n1)  = input_buf(n0, n1);
    W64(n0)        = ComplexExpr(W64_buf(2*n0), W64_buf(2*n0+1));
    W64_horiz(n0)  = ComplexExpr(W64_horiz_buf(2*n0), W64_horiz_buf(2*n0+1));
    
    zipped(n0, n1, t_x, t_y) = ComplexExpr(input(t_x*128 + 2*n0,   t_y*64 + n1),
                                           input(t_x*128 + 2*n0+1, t_y*64 + n1)
                                          );

    auto v_dft                = fft_vert_top(zipped, W64, output);
    v_dft_t(n0, n1, t_x, t_y) = v_dft(n1, n0, t_x, t_y);
    auto h_dft                = fft_vert_top(v_dft_t, W64_horiz, output);
    h_dft_t(n0, n1, t_x, t_y) = h_dft(n1, n0, t_x, t_y);
    
    output(n0, n1, c, t_x, t_y) = select(c == 0, re(h_dft_t(n0, n1, t_x, t_y)),
                                                 im(h_dft_t(n0, n1, t_x, t_y))
 
                                  );
    
    FinalOut(n0, n1, c, t_x, t_y) = output(n0, n1, c, t_x, t_y);
    
    if (get_target().has_feature(Target::CLCadence)) {
      input_buf.in()
               .compute_at(FinalOut, t_x)
               .dma_copy(_1);

      W64_buf.dim(0).set_bounds(0, 2*64).set_stride(1);
      
      W64_buf.in()
             .bound(_0, 0, 2*64)
             .compute_at(FinalOut, t_x)
             .dma_copy(_0);
      
      W64_horiz_buf.dim(0).set_bounds(0, 2*64).set_stride(1);
      
      W64_horiz_buf.in()
                   .bound(_0, 0, 2*64)
                   .compute_at(FinalOut, t_x)
                   .dma_copy(_0);
      
      output.compute_at(FinalOut, t_x)
            .reorder(n1, n0, c)
            .bound(c, 0, 2)
            .vectorize(n0, 16);
      
      FinalOut.dim(0).set_bounds(0, 64).set_stride(1);
      FinalOut.dim(1).set_bounds(0, 64).set_stride(64);
      
      FinalOut.bound(c, 0, 2)
              .unroll(c)
              .dma_copy(n1)
              .gpu_blocks(t_x, t_y);
    }
  }
};

}  // namespace

HALIDE_REGISTER_GENERATOR(FFT2D, FFT2D)

