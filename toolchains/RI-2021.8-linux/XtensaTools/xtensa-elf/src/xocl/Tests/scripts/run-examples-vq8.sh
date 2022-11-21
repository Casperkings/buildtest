#!/bin/sh

set -x

examples="Tests/VQ8_Examples/MatrixMultiply_AsyncCopy_Int \
          Tests/VQ8_Examples/MatrixMultiply_AsyncCopy     \
          Tests/VQ8_Examples/MatrixMultiply_AsyncCopy_fp16 \
          Tests/VQ8_Examples/sobel                        \
          Tests/VQ8_Examples/Gaussian5x5                  \
          Tests/VQ8_Examples/BilinearUpscale              \
          Tests/VQ8_Examples/BilinearDownscale            \
          Tests/VQ8_Examples/Median3x3                    \
          Tests/VQ8_Examples/PerspectiveTransform         \
          Tests/VQ8_Examples/Filter2D_5x5                 \
          Tests/VQ8_Examples/Convolve3D_5x5               \
          Tests/VQ8_Examples/Bilateral5x5                 \
          Tests/VQ8_Examples/vecAdd_AsyncCopy             \
          Tests/VQ8_Examples/Dilate3x3                    \
          Tests/VQ8_Examples/TransposeU16                 \
          Tests/VQ8_Examples/TableLookUpU8                \
          Tests/VQ8_Examples/GradiantInterleave           \
          Tests/VQ8_Examples/BoxFilter3x3                 \
          Tests/VQ8_Examples/RGB2YUV420                   \
          Tests/VQ8_Examples/HarrisCornerU8               \
          Tests/VQ8_Examples/PyramidU8                    \
          Tests/VQ8_Examples/IntegralU8U32_FullImage      \
          Tests/VQ8_Examples/YUV420toRGB                  \
          Tests/VQ8_Examples/SumOfSquaredDiffsU8S32       \
          Tests/VQ8_Examples/Histogram                    \
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

>Tests/VQ8_Examples/examples-results.log
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
