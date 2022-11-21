#!/bin/sh

bname=("gaussian5x5" "sobel" "median3x3" "rgb2yuv" "filter2d_5x5" \
       "convolve3d_5x5" "bilateral5x5" "matMul_int" "bilinear_upscale" \
       "bilinear_downscale" "perspective" "lensdistortion" "fft2d" \
       "locallaplacian" "tablelookupu8" "transposeu16" "boxfilter3x3" \
       "dilate3x3" "gradiantinterleave" "pyramidu8" "harriscorneru8" \
       "integralu8u32" "sumofsqrdiffsu8s32")
halide_bmarks=(Gaussian5x5 sobel Median3x3 RGB2YUV Filter2D_5x5 \
               Convolve3D_5x5 Bilateral5x5 MatrixMultiply_Int \
               BilinearUpscale BilinearDownscale PerspectiveTransform \
               LensDistortionCorrection FFT2D LocalLaplacian TableLookUpU8 \
               TransposeU16 BoxFilter3x3 Dilate3x3 GradientInterleave \
               PyramidU8 HarrisCornerU8 IntegralU8U32 SumOfSquaredDiffsU8S32)
opencl_bmarks=(Gaussian5x5 sobel Median3x3 RGB2YUV Filter2D_5x5 \
               Convolve3D_5x5 Bilateral5x5 MatrixMultiply_AsyncCopy_Int  \
               BilinearUpscale BilinearDownscale PerspectiveTransform \
               LensDistortionCorrection FFT2D LocalLaplacian TableLookUpU8 \
               TransposeU16 BoxFilter3x3 Dilate3x3 GradiantInterleave \
               PyramidU8 HarrisCornerU8 IntegralU8U32 SumOfSquaredDiffsU8S32)

num="$((${#bname[@]}-1))"

halide_results_log=halide/XtensaTests/examples-results.log
opencl_results_log=$1/Tests/Examples/examples-results.log

>halide/XtensaTests/cmp-results.log
printf "%-40s %-10s %-10s %-10s\n" "Benchmark" "Halide" "OpenCL" "OpenCL/Halide" > \
       halide/XtensaTests/cmp-results.log;
geomean=0
count=0
for i in $(seq 0 $num)
do
  opencl_cycles=`grep -w ${opencl_bmarks[$i]} $opencl_results_log | awk '{split($0,a," "); print a[3]}'`
  if [ -z "$opencl_cycles" ]; then
   opencl_cycles="0"
  fi
  halide_cycles=`grep -w ${halide_bmarks[$i]} $halide_results_log | awk '{split($0,a," "); print a[3]}'`
  if [ -z "$halide_cycles" ]; then
   halide_cycles="0"
  fi
  ratio=`echo "scale=2; $opencl_cycles/$halide_cycles" | bc`
  printf "%-40s" ${bname[$i]} >>  halide/XtensaTests/cmp-results.log
  printf "%-10d %-10d %6.2f\n" $halide_cycles $opencl_cycles $ratio >>  halide/XtensaTests/cmp-results.log
  if [ "$opencl_cycles" == "0" ] || [ "$halide_cycles" == "0" ]; then
    continue
  fi
  geomean=`echo "scale=2; $geomean + l($ratio)"  | bc -l`
  count=`echo "$count + 1" | bc -l`
done
geomean=`echo "scale=2; e($geomean/($count))"  | bc -l`
printf "GeoMean %60.2f\n" $geomean >> halide/XtensaTests/cmp-results.log
