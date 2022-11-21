#!/bin/sh

bname=("sobel5x5" "matMul_int" "matMul" "gaussian5x5" "bilinearupscale" \
       "bilineardownscale" "median3x3" "lensdistortion" "perspective" \
       "fft2d" "filter2d_5x5" "locallaplacian" "rgb2yuv" "convolve3d_5x5" \
       "bilateral5x5" "dilate3x3" "transposeu16" "tablelookupu8" \
       "gradiantinterleave" "boxfilter3x3" "rgb2yuv420" "harriscorneru8" \
       "pyramidu8" "integralu8u32" "yuv420toRGB" "sumofsqrdiffsu8s32" \
       "histogram" "camera_pipe")
bmarks=(sobel MatrixMultiply_AsyncCopy_Int MatrixMultiply_AsyncCopy \
        Gaussian5x5 BilinearUpscale \
        BilinearDownscale Median3x3 LensDistortionCorrection \
        PerspectiveTransform FFT2D Filter2D_5x5 LocalLaplacian RGB2YUV \
        Convolve3D_5x5 Bilateral5x5 Dilate3x3 \
        TransposeU16 TableLookUpU8 GradiantInterleave BoxFilter3x3 \
        RGB2YUV420 HarrisCornerU8 PyramidU8 IntegralU8U32 YUV420toRGB \
        SumOfSquaredDiffsU8S32 Histogram Camera_pipe)

num="$((${#bname[@]}-1))"

opencl_results_log=Tests/Examples/examples-results.log
native_results_log=Tests/Examples/native-results.log

>Tests/cmp-results.log
printf "%-40s %-10s %-10s %-10s\n" "Benchmark" "OpenCL" "Native" "Native/OpenCL" > \
       Tests/cmp-results.log;
geomean=0
count=0
for i in $(seq 0 $num)
do
  opencl_cycles=`grep -w ${bmarks[$i]} $opencl_results_log | awk '{split($0,a," "); printf "%d", a[3]}'`
  if [ -z "$opencl_cycles" ]; then
   opencl_cycles="0"
  fi
  native_cycles=`grep -w ${bmarks[$i]} $native_results_log | awk '{split($0,a," "); printf "%d", a[2]}'`
  if [ -z "$native_cycles" ]; then
   native_cycles="0"
  fi
  ratio=`echo "scale=2; $native_cycles/$opencl_cycles" | bc`
  printf "%-40s" ${bname[$i]} >>  Tests/cmp-results.log
  printf "%-10d %-10d %6.2f\n" $opencl_cycles $native_cycles $ratio >>  Tests/cmp-results.log
  if [ "$opencl_cycles" == "0" ] || [ "$native_cycles" == "0" ]; then
    continue
  fi
  geomean=`echo "scale=2; $geomean + l($ratio)"  | bc -l`
  count=`echo "$count + 1" | bc -l`
done
geomean=`echo "scale=2; e($geomean/($count))"  | bc -l`
printf "GeoMean %60.2f\n" $geomean >> Tests/cmp-results.log
