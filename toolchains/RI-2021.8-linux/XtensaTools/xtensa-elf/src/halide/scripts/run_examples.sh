#!/bin/sh

set -x

examples="halide/XtensaTests/Gaussian5x5 \
          halide/XtensaTests/sobel \
          halide/XtensaTests/Median3x3 \
          halide/XtensaTests/RGB2YUV \
          halide/XtensaTests/Filter2D_5x5 \
          halide/XtensaTests/Convolve3D_5x5 \
          halide/XtensaTests/Bilateral5x5 \
          halide/XtensaTests/MatrixMultiply_Int \
          halide/XtensaTests/BilinearUpscale \
          halide/XtensaTests/BilinearDownscale \
          halide/XtensaTests/PerspectiveTransform \
          halide/XtensaTests/LensDistortionCorrection \
          halide/XtensaTests/FFT2D \
          halide/XtensaTests/LocalLaplacian \
          halide/XtensaTests/TableLookUpU8 \
          halide/XtensaTests/TransposeU16 \
          halide/XtensaTests/BoxFilter3x3 \
          halide/XtensaTests/Dilate3x3 \
          halide/XtensaTests/GradientInterleave \
          halide/XtensaTests/PyramidU8 \
          halide/XtensaTests/HarrisCornerU8 \
          halide/XtensaTests/IntegralU8U32 \
          halide/XtensaTests/SumOfSquaredDiffsU8S32 \
         "
for i in $examples
do
  pushd $i
  make -f Makefile.halide clean
  make -f Makefile.ocl clean
  bname=$(basename $i)
  rm -f ../$bname.log
  popd
done

for i in $examples
do
  echo "Building $i"
  pushd $i
  bname=$(basename $i)
  make -j4 -f Makefile.halide > ../$bname.log 2>&1
  make XT_OCL_EXPT=$1 -j4 -f Makefile.ocl build >> ../$bname.log 2>&1
  popd
done

for i in $examples
do
  echo "Running $i"
  pushd $i
  bname=$(basename $i)
  make MEM_LATENCY=100 -f Makefile.ocl xtsc-run >> ../$bname.log 2>&1 &
  popd
done

wait

>halide/XtensaTests/examples-results.log
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
