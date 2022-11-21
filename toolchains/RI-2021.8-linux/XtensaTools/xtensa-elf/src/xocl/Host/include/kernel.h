/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __KERNEL_H__
#define __KERNEL_H__

#define CL_TARGET_OPENCL_VERSION 120
#include "CL/cl.h"
#include "xocl_kernel_info.h"
#include <vector>
#include <map>
#include <cstring>

struct xrp_buffer_group;
typedef xrp_buffer_group *buf_group;

namespace mxpa {

enum OCL_ARG_T {
  OCL_ARG_T_NONE,
  OCL_ARG_T_MEM,
  OCL_ARG_T_LOCAL,
  OCL_ARG_T_PRIVATE,
  OCL_ARG_T_UNKNOWN,
};

enum Direction {
  DIRECTION_INPUT, 
  DIRECTION_OUTPUT, 
  DIRECTION_NOT_MEM, 
  DIRECTION_INPUT_OUTPUT,
  DIRECTION_UNKNOWN = -1
};

typedef xocl_opencl_kernel_descriptor KernelRegistry;

class Kernel {
 public:
  Kernel(KernelRegistry *kern_reg, cl_context ctx);
  Kernel(Kernel* k) :
    work_dim_(k->work_dim_),
    kernel_registry_(k->kernel_registry_),
    num_args_(k->num_args_),
    args_type_(k->args_type_),
    arguments_(k->arguments_),
    args_size_(k->args_size_),
    mem_args_(k->mem_args_),
    cl_ctx_(k->cl_ctx_),
    cl_krnl_(k->cl_krnl_) {
      std::memcpy(global_size_, k->global_size_, sizeof(unsigned) * 3);
      std::memcpy(local_size_, k->local_size_, sizeof(unsigned) * 3);
      std::memcpy(global_offset_, k->global_offset_, sizeof(unsigned) * 3);
    }

  virtual ~Kernel();

  // Returns the direction of specified argument.
  virtual Direction argumentDirection(unsigned arg_no) const;

  // Returns access range lower bound based on specified argument 
  // and batch number.
  virtual size_t argumentLowerBound(unsigned arg_no, size_t batch) const = 0;

  // Returns access range upper bound based on specified argument 
  // and batch number.
  virtual size_t argumentUpperBound(unsigned arg_no, size_t batch) const = 0;

  // Argument passing
  virtual int setArgument(unsigned arg_no, size_t sz, const void *value);
  virtual size_t getArgSize(unsigned arg_no) const;

  // todo: we should already knew expected value of *isBuf in compilation time
  //       (do it in argumentDirection() or something else)
  virtual const void* getArgument(unsigned arg_no, 
                                  bool *isBuf = NULL) const = 0;

  virtual void invokeBatchByRange(size_t, size_t) = 0;
  virtual void invokeGroupByID(size_t batch, size_t processor_no) = 0;

  virtual void invokeGroupByRange(size_t processor_no,
                                  buf_group shared_buf_group = NULL,
                                  bool use_printf = false) = 0;

  virtual int passParameters2Device(size_t use_num_cores,
                                    buf_group shared_buf_group = NULL,
                                    unsigned long long id = 0) = 0;

  // Returns total argument count.
  unsigned numArguments() const;
  unsigned* getWorkGroup() const;
  unsigned getReqdWorkDimSize() const;
  const xocl_opencl_kernel_arg_info* kernelArgInfo() const;
  bool isCallPrintfFun() const;

  void setGlobalSizes(size_t, size_t, size_t);
  void setLocalSizes(size_t, size_t, size_t);
  void setGlobalOffsets(size_t, size_t, size_t);
  void setWorkDim(unsigned);
  size_t getBatchCount(void) const;
  const char* getKernelName() const;
  void getGroupIdFromBatch(unsigned batch, unsigned group[3]) const;

  cl_kernel getCLKrnlObj() const { 
    return cl_krnl_; 
  }

  void setCLKrnlObj(cl_kernel k) { 
    cl_krnl_ = k; 
  }

  std::map<unsigned, cl_mem>& getMemArgs() { 
    return mem_args_; 
  }

protected:
  unsigned global_size_[3];
  unsigned local_size_[3];
  unsigned global_offset_[3];
  unsigned work_dim_;
  mxpa::KernelRegistry *kernel_registry_;
  unsigned num_args_;
  std::vector<OCL_ARG_T> args_type_;
  std::vector<const void *> arguments_;
  std::vector<size_t> args_size_;
  std::map<unsigned, cl_mem> mem_args_;
  std::vector<const void *> dustbin_;
  cl_context cl_ctx_;
  cl_kernel cl_krnl_;
};

class RegisteredKernels {
public:
  RegisteredKernels(KernelRegistry *kreg, unsigned long long id);
  static KernelRegistry *lookup(const char * const name);
  static unsigned long long getKernelId(KernelRegistry *);
  static bool isCallPrintfFun();
};

size_t getNumKernels(const std::string &);
size_t getKernelNames(const std::string &, void *param_value);
const std::vector<std::string> &getKernelNamesByHash(const std::string &hash);

typedef mxpa::Kernel * (*KernelCreator)(mxpa::KernelRegistry *kern_reg,
                                        cl_context owner, mxpa::Kernel* k);
bool registerKernelCreator(KernelCreator kernCreator);
mxpa::Kernel *createKernel(cl_context ctx, const char *name);
mxpa::Kernel *createKernel(mxpa::Kernel *k);

buf_group createSharedBufGroup();
void releaseSharedBufGroup(buf_group bufgrp);

} // namespace mxpa
 
#endif // __KERNEL_H__
