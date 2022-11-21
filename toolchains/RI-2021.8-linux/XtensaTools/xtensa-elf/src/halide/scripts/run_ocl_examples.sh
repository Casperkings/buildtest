#!/bin/sh

set -x

examples="$1/Tests/Examples/Gaussian5x5                 \
          $1/Tests/Examples/sobel                       \
          $1/Tests/Examples/Median3x3                   \
          $1/Tests/Examples/RGB2YUV                     \
          $1/Tests/Examples/Filter2D_5x5                \
          $1/Tests/Examples/Convolve3D_5x5              \
          $1/Tests/Examples/Bilateral5x5                \
          $1/Tests/Examples/MatrixMultiply_AsyncCopy_Int \
          $1/Tests/Examples/BilinearUpscale             \
          $1/Tests/Examples/BilinearDownscale           \
          $1/Tests/Examples/PerspectiveTransform        \
          $1/Tests/Examples/LensDistortionCorrection    \
          $1/Tests/Examples/FFT2D                       \
          $1/Tests/Examples/LocalLaplacian              \
          $1/Tests/Examples/TableLookUpU8               \
          $1/Tests/Examples/TransposeU16                \
          $1/Tests/Examples/BoxFilter3x3                \
          $1/Tests/Examples/Dilate3x3                   \
          $1/Tests/Examples/GradiantInterleave          \
          $1/Tests/Examples/PyramidU8                   \
          $1/Tests/Examples/HarrisCornerU8              \
          $1/Tests/Examples/IntegralU8U32               \
          $1/Tests/Examples/SumOfSquaredDiffsU8S32      \
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
  echo "Building $i"
  pushd $i
  bname=$(basename $i)
  make XT_OCL_DYN_LIB=$2 -j4 build > ../$bname.log 2>&1
  popd
done

for i in $examples
do
  echo "Running $i"
  pushd $i
  bname=$(basename $i)
  make MEM_LATENCY=100 xtsc-run >> ../$bname.log 2>&1 &
  popd
done

wait

>$1/Tests/Examples/examples-results.log
for i in $examples
do
  pushd $i
  bname=$(basename $i)
  grep -e 'PASS' ../$bname.log
  if [ $? -eq 0 ] ; then
    grep -e 'XOCL_CMD_EXE_KERNEL' ../$bname.log
    if [ $? -eq 0 ] ; then
      cycles=`grep XOCL_CMD_EXE_KERNEL ../$bname.log | awk '{total += $5} END {print total}'`
    else
      cycles=0
    fi
    printf "%-40s %-10s %-10d\n" $bname "PASS" $cycles >> \
                                               ../examples-results.log
  else
    printf "%-40s %-10s\n" $bname "FAIL" >> ../examples-results.log
  fi
  popd
done
