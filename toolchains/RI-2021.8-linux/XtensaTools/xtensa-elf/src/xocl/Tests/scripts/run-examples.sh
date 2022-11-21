#!/bin/sh

set -x

examples="Tests/Examples/MatrixMultiply_AsyncCopy_Int \
          Tests/Examples/MatrixMultiply_AsyncCopy     \
          Tests/Examples/MatrixMultiply_AsyncCopy_fp16 \
          Tests/Examples/sobel                        \
          Tests/Examples/Gaussian5x5                  \
          Tests/Examples/BilinearUpscale              \
          Tests/Examples/BilinearDownscale            \
          Tests/Examples/Median3x3                    \
          Tests/Examples/LensDistortionCorrection     \
          Tests/Examples/PerspectiveTransform         \
          Tests/Examples/FFT2D                        \
          Tests/Examples/Filter2D_5x5                 \
          Tests/Examples/LocalLaplacian               \
          Tests/Examples/RGB2YUV                      \
          Tests/Examples/Convolve3D_5x5               \
          Tests/Examples/Bilateral5x5                 \
          Tests/Examples/vecAdd                       \
          Tests/Examples/vecAdd_AsyncCopy             \
          Tests/Examples/subDevice                    \
          Tests/Examples/Dilate3x3                    \
          Tests/Examples/TransposeU16                 \
          Tests/Examples/TableLookUpU8                \
          Tests/Examples/GradiantInterleave           \
          Tests/Examples/BoxFilter3x3                 \
          Tests/Examples/RGB2YUV420                   \
          Tests/Examples/HarrisCornerU8               \
          Tests/Examples/PyramidU8                    \
          Tests/Examples/IntegralU8U32                \
          Tests/Examples/YUV420toRGB                  \
          Tests/Examples/SumOfSquaredDiffsU8S32       \
          Tests/Examples/Histogram                    \
          Tests/Examples/Camera_pipe                  \
          "

for i in $examples
do
  pushd $i
  make clean
  bname=$(basename $i)
  rm -f ../$bname.log
  popd
done

for i in $examples
do
  pushd $i
  bname=$(basename $i)
  echo "Building $bname"
  make XT_OCL_DYN_LIB=$1 -j4 build > ../$bname.log 2>&1
  popd
done

for i in $examples
do
  pushd $i
  bname=$(basename $i)
  echo "Running $bname"
  make >> ../$bname.log 2>&1 &
  popd
done

wait

>Tests/Examples/examples-results.log
for i in $examples
do
  pushd $i
  bname=$(basename $i)
  grep -e 'PASS' ../$bname.log
  if [ $? -eq 0 ] ; then
    printf "%-40s %-10s\n" $bname "PASS" >> ../examples-results.log
  else
    printf "%-40s %-10s\n" $bname "FAIL" >> ../examples-results.log
  fi
  popd
done
