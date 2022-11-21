#!/bin/sh

set -x

examples_native="Tests/Native/MatrixMultiply_AsyncCopy \
                "
examples="Tests/Examples/MatrixMultiply_AsyncCopy_Int \
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

for i in $examples $examples_native
do
  pushd $i
  make clean
  bname=$(basename $i)
  rm -f ../$bname.native.log
  popd
done

for i in $examples
do
  pushd $i
  bname=$(basename $i)
  echo "Building $bname"
  make -j4 NATIVE_KERNEL=1 XT_OCL_DYN_LIB=$1 build > ../$bname.native.log 2>&1 
  popd
done

for i in $examples
do
  pushd $i
  bname=$(basename $i)
  echo "Running $bname"
  make NATIVE_KERNEL=1 MEM_LATENCY=100 xtsc-run >> ../$bname.native.log 2>&1 &
  popd
done

wait

for i in $examples_native
do
  pushd $i
  echo "Running $i"
  bname=$(basename $i)
  make -j4 MEM_DELAY=100 run > ../$bname.native.log 2>&1 &
  popd
done

wait

>Tests/Examples/native-results.log

for i in $examples_native
do
  pushd $i
  bname=$(basename $i)
  cycles=`grep -w Cycles ../$bname.native.log | awk '{total += $3} END {printf "%d", total}'`
  popd
  printf "%-40s %-10d\n" $bname $cycles >> Tests/Examples/native-results.log
done

for i in $examples
do
  pushd $i
  bname=$(basename $i)
  grep -e 'PASS' ../$bname.native.log
  if [ $? -eq 0 ] ; then
    grep -e 'XOCL_CMD_EXE_KERNEL' ../$bname.native.log
    if [ $? -eq 0 ] ; then
      cycles=`grep XOCL_CMD_EXE_KERNEL ../$bname.native.log | awk '{total += $5} END {printf "%d", total}'`
    else
      cycles=0
    fi
  else
    cycles=0
  fi
  popd
  printf "%-40s %-10d\n" $bname $cycles >> Tests/Examples/native-results.log
done
