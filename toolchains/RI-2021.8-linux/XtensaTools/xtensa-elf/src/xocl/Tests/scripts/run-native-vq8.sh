#!/bin/sh

set -x


examples_native="Tests/Native/MatrixMultiply_AsyncCopy \
                "
examples="Tests/VQ8_Examples/MatrixMultiply_AsyncCopy_Int \
          Tests/VQ8_Examples/sobel                        \
          Tests/VQ8_Examples/Gaussian5x5                  \
          Tests/VQ8_Examples/BilinearUpscale              \
          Tests/VQ8_Examples/BilinearDownscale            \
          Tests/VQ8_Examples/Median3x3                    \
          Tests/VQ8_Examples/PerspectiveTransform         \
          Tests/VQ8_Examples/Filter2D_5x5                 \
          Tests/VQ8_Examples/Convolve3D_5x5               \
          Tests/VQ8_Examples/Bilateral5x5                 \
          Tests/VQ8_Examples/Dilate3x3                    \
          Tests/VQ8_Examples/TransposeU16                 \
          Tests/VQ8_Examples/TableLookUpU8                \
          Tests/VQ8_Examples/GradiantInterleave           \
          Tests/VQ8_Examples/BoxFilter3x3                 \
          Tests/VQ8_Examples/RGB2YUV420                   \
          Tests/VQ8_Examples/HarrisCornerU8               \
          Tests/VQ8_Examples/PyramidU8                    \
          Tests/VQ8_Examples/YUV420toRGB                  \
          Tests/VQ8_Examples/SumOfSquaredDiffsU8S32       \
          Tests/VQ8_Examples/Histogram                    \
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

>Tests/VQ8_Examples/native-results.log

for i in $examples_native
do
  pushd $i
  bname=$(basename $i)
  cycles=`grep -w Cycles ../$bname.native.log | awk '{total += $3} END {printf "%d", total}'`
  popd
  printf "%-40s %-10d\n" $bname $cycles >> Tests/VQ8_Examples/native-results.log
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
  printf "%-40s %-10d\n" $bname $cycles >> Tests/VQ8_Examples/native-results.log
done
