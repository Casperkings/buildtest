/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#include "kernel.h"
#include "mxpa_debug.h"
#include "mxpa_common.h"
#include "CL/cl.h"
#include "cl_internal_objects.h"
#include <vector>
#include <cstring>
#include <cstdlib>
#include <map>
#include <algorithm>
#include <stdint.h>

namespace {
class RegisteredKernelsImpl {
public:
  typedef std::map<std::string, mxpa::KernelRegistry *>        ProgramManager;
  typedef std::map<mxpa::KernelRegistry *, unsigned long long> KernelIdMap;
  typedef 
    std::map<const std::string, std::vector<std::string>>      KernelNamesTab;

  void add(mxpa::KernelRegistry *k, unsigned long long id) {
    assert(k && "KernelRegistry can't be null.");
    for (auto &Iter : kernel_id_map) {
      if (Iter.second == id) {
        XOCL_ERR("Error: duplicate kernel id when add kernel %s with id %llu\n",
                 k->kernel_name, id);
        break;
      }
    }

    if (1 == k->isCallPrintfFun)
      isCallPrintf_ = true;

    auto result = kernel_id_map.insert(
                    std::pair<mxpa::KernelRegistry *, 
                              unsigned long long>(k, id));
    if (result.second == false) {
      XOCL_ERR("Error, the kernel %s has been added.\n", k->kernel_name);
    }

    std::string kernel_fullname = k->source_hash;
    kernel_fullname += "_";
    kernel_fullname += k->kernel_name;

    XOCL_DPRINTF("Register kernel(%s) at %p\n", kernel_fullname.c_str(), k);

    kernels_[kernel_fullname] = k;
    krl_names_tab_[k->source_hash].push_back(k->kernel_name);
  }

  unsigned long long getKernelId(mxpa::KernelRegistry *kr) {
    assert(kernel_id_map.find(kr) != kernel_id_map.end());
    return kernel_id_map[kr];
  }

  bool isCallPrintfFun() {
    return isCallPrintf_;
  }

  mxpa::KernelRegistry *lookup(const char *const name) {
    if (kernels_.find(name) == kernels_.end()) {
      XOCL_ERR("Can not find kernel registry: %s\n", name);
      return nullptr;
    }
    return kernels_[name];
  }

  size_t getNumKernels(const std::string &sourceHash) {
    if (krl_names_tab_.find(sourceHash) == krl_names_tab_.end())
      return 0;
    return krl_names_tab_[sourceHash].size();
  }

  size_t getKernelNames(const std::string &sourceHash, void *param_value) {
    size_t kernel_count = getNumKernels(sourceHash);
    size_t total_length = 0;
    std::vector<std::string>::iterator it = krl_names_tab_[sourceHash].begin();
    std::vector<std::string>::iterator ie = krl_names_tab_[sourceHash].end();
    for (size_t i = 0; it != ie; ++it, ++i) {
      if (param_value) {
        char *kernel_names = (char *)param_value;
        strcpy(kernel_names + total_length + i, it->c_str());
        total_length += strlen(it->c_str());
        if (i < kernel_count - 1) {
          *(kernel_names + total_length + i) = ';';
        } else {
          *(kernel_names + total_length + i) = '\0';
        }
      } else {
        total_length += strlen(it->c_str());
      }
    }
    return total_length + kernel_count;
  }

  const std::vector<std::string> &
  getKernelNamesByHash(const std::string &hash) {
    return krl_names_tab_[hash];
  }

  RegisteredKernelsImpl() : isCallPrintf_(false) {}

  ~RegisteredKernelsImpl() {
    XOCL_DPRINTF("Destructing RegisteredKernelsImpl\n");
  }

private:
  KernelIdMap kernel_id_map;
  ProgramManager kernels_;
  KernelNamesTab krl_names_tab_;
  bool isCallPrintf_;
};

} // end namespace

RegisteredKernelsImpl& getRegKrnlImpl() {
  static RegisteredKernelsImpl RegKrnlImpl;
  return RegKrnlImpl;
}

extern "C" {
  void _xt_ocl_reg_krldes_func(int kernelNum, 
                               mxpa::KernelRegistry *kernelregister) {
    for (int i = 0; i < kernelNum; ++i) {
      getRegKrnlImpl().add(&(kernelregister[i]), kernelregister[i].kernel_id);
    }
  }
}

namespace mxpa {

Kernel::Kernel(mxpa::KernelRegistry *kern_reg, cl_context ctx)
  : kernel_registry_(kern_reg), cl_ctx_(ctx), cl_krnl_(nullptr) {

  if (!kern_reg)
    return;

  num_args_ = kern_reg->get_arg_nr[0]();
  arguments_.resize(num_args_);
  args_size_.resize(num_args_);

  // init arg_type
  for (unsigned idx = 0; idx < num_args_; ++idx) {
    OCL_ARG_T arg_type = OCL_ARG_T_NONE;
    const xocl_opencl_kernel_arg_info *arg_info = 
                                       &kernel_registry_->arg_info[idx];
    switch (arg_info->address_qualifier) {
      case CL_KERNEL_ARG_ADDRESS_GLOBAL:
        arg_type = OCL_ARG_T_MEM;
        break;
      case CL_KERNEL_ARG_ADDRESS_LOCAL:
        arg_type = OCL_ARG_T_LOCAL;
        break;
      case CL_KERNEL_ARG_ADDRESS_CONSTANT:
        arg_type = 
            strchr(arg_info->type_name, '*') != nullptr ? OCL_ARG_T_MEM
                                                        : OCL_ARG_T_PRIVATE;
        break;
      case CL_KERNEL_ARG_ADDRESS_PRIVATE:
        arg_type = OCL_ARG_T_PRIVATE;
        break;
      default:
        arg_type = OCL_ARG_T_UNKNOWN;
        break;
    }
    args_type_.push_back(arg_type);
  }

  memset(global_size_, 0, sizeof(global_size_));
  memset(local_size_, 0, sizeof(local_size_));
  memset(global_offset_, 0, sizeof(global_offset_));
  work_dim_ = 3;
}

Kernel::~Kernel() {
  cl_context ctx = cl_ctx_;
  // release memory
  std::sort(dustbin_.begin(), dustbin_.end());
  std::vector<const void*>::iterator vi = 
                            std::unique(dustbin_.begin(), dustbin_.end());
  if (vi != dustbin_.end())
    dustbin_.erase(vi, dustbin_.end());

  for (auto& it : dustbin_) {
    if (it)
      ctx->freeDevMem((buf_obj)it);
  }
  dustbin_.clear();
  arguments_.clear();
}

mxpa::Direction Kernel::argumentDirection(unsigned arg_no) const {
  const xocl_opencl_kernel_arg_info *arg_info = nullptr;
  arg_info = kernel_registry_->arg_info + arg_no;

  if (!(arg_info->address_qualifier == CL_KERNEL_ARG_ADDRESS_GLOBAL || 
        ((arg_info->address_qualifier == CL_KERNEL_ARG_ADDRESS_CONSTANT) && 
         (strchr(arg_info->type_name, '*') != nullptr)))) {
    return mxpa::DIRECTION_NOT_MEM;
  }

  int ret = (kernel_registry_->get_arg_dir[0])(arg_no);
  switch (ret) {
    case 0:
      return mxpa::DIRECTION_INPUT;
    case 1:
      return mxpa::DIRECTION_OUTPUT;
    case 3:
      return mxpa::DIRECTION_INPUT_OUTPUT;
  }
  return mxpa::DIRECTION_UNKNOWN;
}

unsigned Kernel::numArguments() const { 
  return num_args_; 
}

unsigned *Kernel::getWorkGroup() const { 
  return kernel_registry_->workgroup; 
}

unsigned Kernel::getReqdWorkDimSize() const {
  return kernel_registry_->reqd_wd_size;
}

const char *Kernel::getKernelName() const { 
  return kernel_registry_->kernel_name; 
}

bool Kernel::isCallPrintfFun() const { 
  return kernel_registry_->isCallPrintfFun; 
} 

const xocl_opencl_kernel_arg_info *Kernel::kernelArgInfo() const {
  return kernel_registry_->arg_info;
}

void Kernel::getGroupIdFromBatch(unsigned batch, unsigned group[3]) const {
  unsigned nr_groups[2] = {global_size_[0] / local_size_[0],
                           global_size_[1] / local_size_[1]};
  group[0] = batch % nr_groups[0];
  group[1] = (batch % (nr_groups[1] * nr_groups[0])) / nr_groups[0];
  group[2] = batch / (nr_groups[1] * nr_groups[0]);
}

void Kernel::setGlobalSizes(size_t g0, size_t g1, size_t g2) {
  global_size_[0] = g0;
  global_size_[1] = g1;
  global_size_[2] = g2;
}

void Kernel::setLocalSizes(size_t l0, size_t l1, size_t l2) {
  local_size_[0] = l0;
  local_size_[1] = l1;
  local_size_[2] = l2;
}

void Kernel::setGlobalOffsets(size_t _0, size_t _1, size_t _2) {
  global_offset_[0] = _0;
  global_offset_[1] = _1;
  global_offset_[2] = _2;
}

void Kernel::setWorkDim(unsigned work_dim) { 
  work_dim_ = work_dim; 
}

size_t Kernel::getBatchCount(void) const {
  size_t nr_groups[] = {global_size_[0] / local_size_[0],
                        global_size_[1] / local_size_[1],
                        global_size_[2] / local_size_[2]};
  return nr_groups[0] * nr_groups[1] * nr_groups[2];
}

size_t Kernel::getArgSize(unsigned arg_no) const {
  return args_size_[arg_no];
}

int Kernel::setArgument(unsigned arg_no, size_t arg_size,
                        const void *arg_value) {
  assert(arg_no < num_args_);
  cl_context ctx = cl_ctx_;
  OCL_ARG_T arg_t = args_type_[arg_no];
  switch (arg_t) {
    case OCL_ARG_T_MEM: {
      // Handle NULL ptr case:
      //   CL-1.2 of clSetKernelArg:
      //     "If the argument is a buffer object, the arg_value pointer can be
      //      NULL or point to a NULL value "
      if (nullptr == arg_value || nullptr == *(cl_mem *)arg_value) {
        args_size_[arg_no] = arg_size;
        arguments_[arg_no] = nullptr;
        return CL_SUCCESS;
      }

      cl_mem mem = *(cl_mem *)arg_value;
      if (!ctx->isValidMemoryObject(mem))
        return CL_INVALID_MEM_OBJECT;

      // Retain it when enqueueNDRangeKernel
      // mem->retain();
      mem_args_[arg_no] = mem;
      args_size_[arg_no] = mem->getSize();
      arguments_[arg_no] = mem;
      return CL_SUCCESS;
    }
    case OCL_ARG_T_LOCAL: {
      // Free this buffer after clfinish.
      // if (void *dev_ptr = (void *)arguments_[arg_no])
      //  ctx->FreeDevLocalMem(dev_ptr);
      arguments_[arg_no] = ctx->allocDevLocalMem(arg_size);
      args_size_[arg_no] = arg_size;
      // Store buffers which need to free after clfinish
      dustbin_.push_back(arguments_[arg_no]);
      return CL_SUCCESS;
    }
    case OCL_ARG_T_PRIVATE: {
      buf_obj dev_buf = (buf_obj)arguments_[arg_no];
      // FIXME: 'args_size_' for private arguments should obtain from compiler
      if (args_size_[arg_no] && args_size_[arg_no] != arg_size)
        return CL_INVALID_ARG_SIZE;
      // need to free when release kernel
      dev_buf = ctx->allocDevMem(arg_size);
      args_size_[arg_no] = arg_size;
      arguments_[arg_no] = dev_buf;
      // Store buffers which need to free after clfinish
      dustbin_.push_back(arguments_[arg_no]);
      ctx->copyToDevMem(dev_buf, arg_value, arg_size);
      return CL_SUCCESS;
    }
    default:
      return CL_SUCCESS;
  }
}

RegisteredKernels::RegisteredKernels(KernelRegistry *kreg,
                                     unsigned long long id) {
  getRegKrnlImpl().add(kreg, id);
}

KernelRegistry *RegisteredKernels::lookup(const char *const k) {
  return getRegKrnlImpl().lookup(k);
}

unsigned long long RegisteredKernels::getKernelId(KernelRegistry *k) {
  return getRegKrnlImpl().getKernelId(k);
}

bool RegisteredKernels::isCallPrintfFun() {
  return getRegKrnlImpl().isCallPrintfFun();
}

size_t getNumKernels(const std::string &sourceHash) {
  return getRegKrnlImpl().getNumKernels(sourceHash);
}

size_t getKernelNames(const std::string &sourceHash, void *param_value) {
  return getRegKrnlImpl().getKernelNames(sourceHash, param_value);
}

const std::vector<std::string> &getKernelNamesByHash(const std::string &hash) {
  return getRegKrnlImpl().getKernelNamesByHash(hash);
}

}  // end of namespace mxpa

static mxpa::KernelCreator ExternalKernelCreator = nullptr;

bool mxpa::registerKernelCreator(KernelCreator kernCreator) {
  if (ExternalKernelCreator) {
    XOCL_ERR("Kernel creator has been registered.\n");
    return false;
  }
  ExternalKernelCreator = kernCreator;
  return true;
}

mxpa::Kernel *mxpa::createKernel(cl_context ctx, const char *name) {
  mxpa::KernelRegistry * kreg = mxpa::RegisteredKernels::lookup(name);
  if (!kreg)
    return nullptr;
  if (!ExternalKernelCreator)
     return nullptr;
  return ExternalKernelCreator(kreg, ctx, nullptr);
}

mxpa::Kernel *mxpa::createKernel(mxpa::Kernel *k) {
  if (!ExternalKernelCreator)
    return nullptr;
  return ExternalKernelCreator(nullptr, nullptr, k);
}
