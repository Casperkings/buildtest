/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifdef EMIT_OPENCL_KERNEL_SRC
#include <fstream>
#endif
#ifdef PROCESS_OPENCL_BUILD_OPTIONS
#include <unistd.h>
#endif
#include "kernel.h"
#include "mxpa_profile.h"
#include "mxpa_common.h"
#include "cl_internal_objects.h"
#include "mxpa_debug.h"
#include <cstring>
#include <algorithm>
#include "md5.h"
#ifdef XOS
#include <xtensa/xos.h>
#else
#include <signal.h>
#endif

extern cl_device_id initDevice();
extern void unInitDevice();

namespace mxpa {
extern mxpa::KernelCreator CreateDeviceKernel;
}

static void __attribute__((destructor)) xoclUnInit() {
  unInitDevice();
}

#ifndef XOS
// Handle process termination
static void sig_handler(int signum, siginfo_t *info, void *ptr) {
  xoclUnInit();
  exit(1);
}

static void catchProcessTermination() {
  static struct sigaction sigact;
  memset(&sigact, 0, sizeof(sigact));
  sigact.sa_sigaction = sig_handler;
  sigact.sa_flags = SA_SIGINFO;
  sigaction(SIGTERM, &sigact, NULL);
}
#endif

static void __attribute__((constructor)) xoclInit() {
#ifdef XOS
  xos_start_main("main", 1, 0);
#define TICK_CYCLES (xos_get_clock_freq()/10)
  xos_start_system_timer(-1, TICK_CYCLES);
#endif
  cl_device_id newdev = initDevice();
  getPlatform()->registerDevice(newdev);
  mxpa::registerKernelCreator(mxpa::CreateDeviceKernel);
#ifndef XOS
  catchProcessTermination();
#endif
}

_cl_kernel::_cl_kernel(const char *const name, cl_program program) :
  mxpa_kernel(nullptr), context(program->context), 
  program(program), dev_(nullptr) 
{
  program->retain();
  mxpa_kernel = mxpa::createKernel(program->context,
                                   (program->file_hash + "_" + 
                                    std::string(name)).c_str());
  pKernel_name_ = (char *)malloc(strlen(name) + 1);
  std::memcpy(pKernel_name_, name, strlen(name) + 1);
  if (mxpa_kernel == nullptr) {
    XOCL_DPRINTF("XOCL: Except source hash is: %s\n", 
                 program->file_hash.c_str());
  } else {
    mxpa_kernel->setCLKrnlObj(this);
  }
}

_cl_kernel::~_cl_kernel() {
  if (pKernel_name_)
    free(pKernel_name_);
  program->release();
  delete mxpa_kernel;
}

const cl_platform_id getPlatform() {
  static _cl_platform_id platform;
  return &platform;
}

_cl_platform_id::_cl_platform_id() : device_(nullptr), 
                                     is_init_device_print_buf_(false) {}

void _cl_platform_id::registerDevice(cl_device_id newDev) {
  assert(device_ == nullptr && "Device has been registered!");
  device_ = newDev;
  addDevice(device_);
}

cl_int _cl_platform_id::getDeviceIDs(cl_device_type device_type,
                                     cl_device_id *devices,
                                     cl_uint *num_devices) {
  switch (device_type) {
  case CL_DEVICE_TYPE_CPU:
  case CL_DEVICE_TYPE_GPU:
  case CL_DEVICE_TYPE_ACCELERATOR:
  case CL_DEVICE_TYPE_DEFAULT:
  case CL_DEVICE_TYPE_ALL:
    // single device, ignore num_entries
    if (num_devices)
      *num_devices = 1;
    if (devices)
      devices[0] = device_;
    return CL_SUCCESS;
  case CL_DEVICE_TYPE_CUSTOM:
    if (num_devices)
      *num_devices = 0;
    return CL_DEVICE_NOT_FOUND;
  default:
    return CL_INVALID_DEVICE_TYPE;
  }
}

cl_int _cl_platform_id::getInfo(cl_platform_info param_name,
                                size_t param_value_size,
                                void *param_value,
                                size_t *param_value_size_ret) {
  switch (param_name) {
  case CL_PLATFORM_NAME:
    return setParamStr(param_value, XOCL_PLATFORM_NAME, param_value_size,
                       param_value_size_ret);
  case CL_PLATFORM_VERSION:
    return setParamStr(param_value, XOCL_PLATFORM_VERSION, 
                       param_value_size, param_value_size_ret);
  case CL_PLATFORM_VENDOR:
    return setParamStr(param_value, XOCL_PLATFORM_VENDOR, 
                       param_value_size, param_value_size_ret);
  case CL_PLATFORM_EXTENSIONS:
    return setParamStr(param_value, XOCL_PLATFORM_EXTENSIONS, 
                       param_value_size, param_value_size_ret);
  case CL_PLATFORM_PROFILE:
    return setParamStr(param_value, XOCL_PLATFORM_PROFILE, 
                       param_value_size, param_value_size_ret);
  default:
    return CL_INVALID_VALUE;
  }
}

cl_int _cl_context::getInfo(cl_context_info param_name, size_t param_value_size,
                            void *param_value, size_t *param_value_size_ret) {
  switch (param_name) {
  case CL_CONTEXT_DEVICES: {
    if (param_value_size < sizeof(cl_device_id) * device_count_ && param_value)
      return CL_INVALID_VALUE;
    if (param_value)
      std::memcpy(param_value, (void *)device_list_, 
                  sizeof(cl_device_id) * device_count_);
    if (param_value_size_ret)
      *param_value_size_ret = sizeof(cl_device_id) * device_count_;
    return CL_SUCCESS;
  }
  case CL_CONTEXT_PROPERTIES: {
    if (param_value_size < context_properties_len_ && param_value)
      return CL_INVALID_VALUE;
    if (context_properties_) {
      if (param_value)
        std::memcpy(param_value, (void *)context_properties_, 
                    context_properties_len_);
      if (param_value_size_ret)
        *param_value_size_ret = context_properties_len_;
    } else {
      if (param_value)
        param_value = nullptr;
      if (param_value_size_ret)
        *param_value_size_ret = 0;
    }
    return CL_SUCCESS;
  }
  case CL_CONTEXT_REFERENCE_COUNT:
    return setParam<cl_uint>(param_value, (cl_uint)ref_count_,
                             param_value_size, param_value_size_ret);
  case CL_CONTEXT_NUM_DEVICES:
    return setParam<cl_uint>(param_value, device_count_,
                             param_value_size, param_value_size_ret);
  default:
    return CL_INVALID_VALUE;
  }
}

#if 0
static inline void appendHash(std::string str, unsigned char *binary,
                              size_t binary_len) {
  unsigned char *binary_str = (unsigned char *)str.c_str();
  for (size_t i = 0; i < binary_len; i++) {
    binary[i] = (binary_str[2 * i] > 96 ? binary_str[2 * i] - 87
                                        : binary_str[2 * i] - 48) * 16 + 
                (binary_str[2 * i + 1] > 96 ? binary_str[2 * i + 1] - 87 
                                            : binary_str[2 * i + 1] - 48);
  }
}
#endif

cl_int _cl_program::getInfo(cl_program_info param_name, size_t param_value_size,
                            void *param_value, size_t *param_value_size_ret) {
  switch (param_name) {
  case CL_PROGRAM_KERNEL_NAMES: {
    size_t ret_size = mxpa::getKernelNames(file_hash, nullptr);
    if (param_value_size < ret_size && param_value)
      return CL_INVALID_VALUE;
    if (param_value)
      mxpa::getKernelNames(file_hash, param_value);
    if (param_value_size_ret)
      *param_value_size_ret = ret_size;
    return CL_SUCCESS;
  }

  case CL_PROGRAM_NUM_KERNELS:
    return setParam<size_t>(param_value,
                            mxpa::getNumKernels(file_hash),
                            param_value_size, param_value_size_ret);

  case CL_PROGRAM_REFERENCE_COUNT:
    return setParam<cl_uint>(param_value, (cl_uint)ref_count_,
                             param_value_size, param_value_size_ret);

  case CL_PROGRAM_CONTEXT:
    return setParam<cl_context>(param_value, context,
                                param_value_size, param_value_size_ret);

  case CL_PROGRAM_NUM_DEVICES:
    return setParam<cl_uint>(param_value, 1,
                             param_value_size, param_value_size_ret);

  case CL_PROGRAM_DEVICES:
    return setParam<cl_device_id>(param_value,
                                  getPlatform()->device_,
                                  param_value_size, param_value_size_ret);

  case CL_PROGRAM_BINARY_SIZES: {
    // The binary is essentially the whole program source + 4-bytes 
    // to represent the type (compiled object, executable, library etc.)
    int size = file_src.length();
    return setParam<size_t>(param_value, size+4,
                            param_value_size, param_value_size_ret);
  }

  case CL_PROGRAM_BINARIES:
    if (param_value_size < sizeof(size_t) && param_value)
      return CL_INVALID_VALUE;
    if (param_value) {
#if 0
      unsigned char src_hash[16];
      appendHash(file_hash, src_hash, 16);
      memcpy(*(char **)param_value, src_hash, 16);
#endif
      // The whole program is represented as a binary with a binary type 
      // (4-bytes) appended to the end
      int size = file_src.length();
      memcpy(*(char **)param_value, file_src.c_str(), size);
      *((int *)((*(char **)param_value)+size)) = binary_type;
    }
    if (param_value_size_ret)
      *param_value_size_ret = sizeof(size_t);
    return CL_SUCCESS;

  case CL_PROGRAM_SOURCE:
    if (param_value_size < sizeof(size_t) && param_value)
      return CL_INVALID_VALUE;
    if (param_value) {
      memcpy((char *)param_value, file_src.c_str(), file_src.length()+1);
    }
    if (param_value_size_ret)
      *param_value_size_ret = file_src.length() + 1;
    return CL_SUCCESS;
  default:
    return CL_INVALID_VALUE;
  }
}

cl_int _cl_kernel::getInfo(cl_kernel_info param_name, size_t param_value_size,
                           void *param_value, size_t *param_value_size_ret) {
  switch (param_name) {
  case CL_KERNEL_NUM_ARGS:
    return setParam<cl_uint>(param_value, mxpa_kernel->numArguments(),
                             param_value_size, param_value_size_ret);
  case CL_KERNEL_REFERENCE_COUNT:
    return setParam<cl_uint>(param_value, (cl_uint)ref_count_,
                             param_value_size, param_value_size_ret);
  case CL_KERNEL_CONTEXT:
    return setParam<cl_context>(param_value, context,
                                param_value_size, param_value_size_ret);
  case CL_KERNEL_PROGRAM:
    return setParam<cl_program>(param_value, program,
                                param_value_size, param_value_size_ret);
  case CL_KERNEL_FUNCTION_NAME:
    return setParamStr(param_value, pKernel_name_,
                       param_value_size, param_value_size_ret);
  // no support
  case CL_KERNEL_ATTRIBUTES:
  default:
    return CL_INVALID_VALUE;
  }
}

cl_int _cl_kernel::getArgInfo(cl_uint arg_indx, cl_kernel_arg_info param_name,
                              size_t param_value_size, void *param_value, 
                              size_t *param_value_size_ret) {

  if (arg_indx > this->mxpa_kernel->numArguments())
    return CL_INVALID_ARG_INDEX;

  const xocl_opencl_kernel_arg_info *arg_info = 
                                     this->mxpa_kernel->kernelArgInfo();

  switch (param_name) {
  case CL_KERNEL_ARG_ADDRESS_QUALIFIER:
    return setParam<cl_kernel_arg_address_qualifier>(
               param_value,
               arg_info[arg_indx].address_qualifier,
               param_value_size, param_value_size_ret);

  case CL_KERNEL_ARG_ACCESS_QUALIFIER:
    return setParam<cl_kernel_arg_access_qualifier>(
               param_value,
               arg_info[arg_indx].access_qualifier,
               param_value_size, param_value_size_ret);

  case CL_KERNEL_ARG_TYPE_QUALIFIER:
    return setParam<cl_kernel_arg_type_qualifier>(
               param_value,
               arg_info[arg_indx].type_qualifier,
               param_value_size, param_value_size_ret);

  case CL_KERNEL_ARG_TYPE_NAME:
    return setParamStr(param_value, arg_info[arg_indx].type_name,
                       param_value_size, param_value_size_ret);

  case CL_KERNEL_ARG_NAME:
    return setParamStr(param_value, arg_info[arg_indx].arg_name,
                       param_value_size, param_value_size_ret);

  default:
    return CL_INVALID_VALUE;
  }
}

static bool doesEventsHaveValidContext(cl_command_queue cmdq,
                                       cl_uint num_events, 
                                       const cl_event *events) {
  for (cl_uint i = 0; i < num_events; ++i)
    if (events[i]->getCommandQueue()->getContext() != cmdq->getContext())
      return false;
  return true;
}

template<class clObj>
bool isValidContext(clObj *obj, cl_command_queue cmdq,
                    cl_uint num_events, const cl_event *events) {
  if (obj && obj->getContext() != cmdq->getContext())
    return false;
  return doesEventsHaveValidContext(cmdq, num_events, events);
}

// OpenCL API Implementations
extern "C" {
  cl_int clGetPlatformIDs(cl_uint num_entries, cl_platform_id *platforms,
                          cl_uint *num_platforms) {
    if (num_platforms == nullptr && platforms == nullptr)
      return CL_INVALID_VALUE;
    if (platforms) {
      if (num_entries >= 1) {
        getPlatform()->initDeviceIOBuf();
        platforms[0] = getPlatform();
      }
      else
        return CL_INVALID_VALUE;
    }
    if (num_platforms)
      *num_platforms = 1;
    return CL_SUCCESS;
  }

  cl_int clGetDeviceIDs(cl_platform_id platform, cl_device_type device_type,
                        cl_uint num_entries, cl_device_id *devices,
                        cl_uint *num_devices) {
    if (platform == nullptr)
      return CL_INVALID_PLATFORM;
    if ((devices == nullptr && num_devices == nullptr) ||
        (devices != nullptr && num_entries == 0))
      return CL_INVALID_VALUE;
    return platform->getDeviceIDs(device_type, devices, num_devices);
  }

  cl_context clCreateContext(const cl_context_properties *properties,
                             cl_uint num_devices, const cl_device_id *devices,
                             void (CL_CALLBACK * /* pfn_notify */)(
                                  const char *, const void *, size_t,
                                   void *),
                             void * /* user_data */, cl_int *errcode_ret) {
    cl_platform_id platform = getPlatform();
    cl_int ret_code = CL_SUCCESS;
    cl_context ctx = nullptr;
    /// one device
    if (devices == nullptr || num_devices == 0) {
      ret_code = CL_INVALID_VALUE;
      goto end;
    }
    for (unsigned i = 0; i < num_devices; ++i) {
      if (platform->getDeviceList().end() ==
          std::find(platform->getDeviceList().begin(),
                    platform->getDeviceList().end(), devices[i])) {
        ret_code = CL_INVALID_DEVICE;
        goto end;
      }
    }
    ctx = new _cl_context(platform, properties, devices, num_devices);
    if (nullptr == ctx)
      ret_code = CL_OUT_OF_HOST_MEMORY;

end:
    if (errcode_ret)
      *errcode_ret = ret_code;

    return ctx;
  }

  cl_context clCreateContextFromType(const cl_context_properties *properties,
                                     cl_device_type device_type,
                                     void (CL_CALLBACK * /* pfn_notify */)(
                                          const char *, const void *, size_t,
                                          void *),
                                     void * /* user_data */, 
                                     cl_int *errcode_ret) {
    cl_int ret_code = CL_SUCCESS;
    cl_context ctx = nullptr;
    cl_platform_id platform = getPlatform();

    if (device_type == CL_DEVICE_TYPE_CPU || 
        device_type == CL_DEVICE_TYPE_ALL ||
        device_type == CL_DEVICE_TYPE_GPU ||
        device_type == CL_DEVICE_TYPE_ACCELERATOR ||
        device_type == CL_DEVICE_TYPE_DEFAULT)
      ctx = new _cl_context(platform, 
                            const_cast<cl_context_properties *>(properties),
                            &(platform->device_), 1);
    else
      ret_code = CL_INVALID_DEVICE_TYPE | CL_DEVICE_NOT_FOUND ;

    if (errcode_ret)
      *errcode_ret = ret_code;

    return ctx;
  }

  cl_command_queue clCreateCommandQueue(cl_context context, cl_device_id device,
                                        cl_command_queue_properties properties,
                                        cl_int *errcode_ret) {
    cl_int ret_code = CL_SUCCESS;
    cl_command_queue q = nullptr;

    if (context == nullptr) {
      ret_code = CL_INVALID_CONTEXT;
      goto end;
    }

    if (device == nullptr) {
      ret_code = CL_INVALID_DEVICE;
      goto end;
    }

    if (properties != 0 && 
        properties != CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE && 
        properties != CL_QUEUE_PROFILING_ENABLE && 
        properties != (CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | 
                       CL_QUEUE_PROFILING_ENABLE)) {
      ret_code = CL_INVALID_VALUE;
      goto end;
    }

    if (properties == CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) {
      ret_code = CL_INVALID_QUEUE_PROPERTIES;
      goto end;
    }

    q = new _cl_command_queue(context, device, properties);

    if (q == nullptr) {
      ret_code = CL_OUT_OF_HOST_MEMORY;
      goto end;
    }
    context->setCmdq(q);

end:
    if (errcode_ret)
      *errcode_ret = ret_code;

    return q;
  }

  cl_command_queue clCreateCommandQueueWithProperties(
                                       cl_context context, 
                                       cl_device_id device,
                                       cl_command_queue_properties *properties,
                                       cl_int *errcode_ret) {
    return nullptr;
  }

#ifdef PROCESS_OPENCL_BUILD_OPTIONS
  static void getPreProcessorOptions(std::string& process_options, 
                                     const char *options) {
    while (*options != '\0') {
      if (options[0] == '-' && options[1] == 'D') {
        // Extract any '-D <name>' or a '-D <name> = <value>' 
        // from the options string
        options += 2;
        // Skip white space
        while (*options == ' ')
          ++options;
        // Extract the name
        const char *name = options;
        while (*options != ' ' && *options != '=' && *options != '\0')
          ++options;
        std::string sname(name, options-name);
        process_options += "#define " + sname;
        // Extract values
        if (*options == '=') {
          ++options;
          const char *def = options;
          while (*options != ' ' && *options != '\0')
            ++options;
          std::string dname(def, options-def);
          process_options += " " + dname;
        }
        process_options += "\n";
      } else if (options[0] == '-' && options[1] == 'I') {
        // Extract '-I path' from the options string
        options += 3;
        process_options += "// -I ";
        // Strip the current working dir.
        char buff[FILENAME_MAX];
        getcwd(buff, FILENAME_MAX);
        std::string cwd(buff);
        std::string path(options);
        std::string basename = path.find(cwd) == 0 ? path.substr(cwd.length()+1)
                                                   : path;
        process_options += basename;
        // std::cout << "path:" << options << " cwd:" 
        //           << cwd << " basename: " << basename << "\n";
        // while (*options != ' ' && *options != '\0') {
        //   process_options += *options;
        //   ++options;
        // }
        process_options += "\n";
      }
      ++options;
    }
  }
#endif

#if 0
  static std::string commentOutIncludes(std::string& file_src) {
    std::string new_file_src;
    for (unsigned i = 0; i < file_src.length(); ++i) {
      char ch = file_src[i];
      if (ch == '#' && (file_src.substr(i, 8) == "#include")) {
        new_file_src += "//";
      }
      new_file_src += ch;
    }
    return new_file_src;
  }
#endif

  static inline void getKernelSrc(std::string& kernel_src, size_t count, 
                                  const char **strings) {
    for (int i = 0; i < (int)count; ++i)
      kernel_src += strings[i];
  }

  static inline void getBinariesHash(size_t count, const char **strings,
                                     const size_t *length, 
                                     md5_byte_t *md5_hash) {
    md5_state_t program_md5;
    md5_init(&program_md5);
    if (length != nullptr) {
      for (int i = 0; i < (int)count; ++i) {
        if (0 == length[i])
          md5_append(&program_md5, (const md5_byte_t *)strings[i],
                     (int)strlen(strings[i]));
        else
          md5_append(&program_md5, (const md5_byte_t *)strings[i], 
                     (int)length[i]);
      }
    } else {
      for (int i = 0; i < (int)count; ++i)
        md5_append(&program_md5, (const md5_byte_t *)strings[i],
                   (int)strlen(strings[i]));
    }
    md5_finish(&program_md5, md5_hash);
  }

  static inline void appendBinaryHex(std::string &str,
                                     const unsigned char *binary,
                                     size_t binary_len) {
    char hex_buf[4];
    for (size_t i = 0; i < binary_len; ++i) {
      sprintf(hex_buf, "%02x", (unsigned int) binary[i]);
      str.append(hex_buf, 2);
    }
  }

  static bool isValidSource(cl_uint count, const char **strings) {
    if (nullptr == strings)
      return false;

    for (cl_uint i = 0; i < count; ++i) {
      if (nullptr == strings[i])
        return false;
    }

    return true;
  }

  cl_program clCreateProgramWithSource(cl_context context, cl_uint count,
                                       const char **strings,
                                       const size_t *lengths,
                                       cl_int *errcode_ret) {
    cl_program prgm = nullptr;
    cl_int ret_code = CL_SUCCESS;
    if (0 == count)
      ret_code = CL_INVALID_VALUE;
    else if (!isValidSource(count, strings))
      ret_code = CL_INVALID_VALUE;
    else {
      md5_byte_t md5_hash[16];
      std::string file_hash;
      getBinariesHash(count, strings, lengths, md5_hash);
      appendBinaryHex(file_hash, md5_hash, 16);
      prgm = new _cl_program(context, file_hash);
      if (nullptr == prgm)
        ret_code = CL_OUT_OF_HOST_MEMORY;
      getKernelSrc(prgm->file_src, count, strings);
      prgm->orig_file_src = prgm->file_src;
    }

    if (errcode_ret)
      *errcode_ret = ret_code;

    return prgm;
  }

  cl_int clBuildProgram(cl_program program, cl_uint num_devices,
                        const cl_device_id *device_list, const char *options,
                        void (*pfn_notify)(cl_program, void *user_data),
                        void *user_data) {
    std::string file_src;
    std::string process_options;
#ifdef PROCESS_OPENCL_BUILD_OPTIONS
    if (options) {
      getPreProcessorOptions(process_options, options);
    }
    if (!process_options.empty()) {
      file_src = process_options;
      file_src += program->orig_file_src;
      // Update hash with the build options
      md5_byte_t md5_hash[16];
      std::string file_hash;
      const char *strings[] = {file_src.c_str()};
      getBinariesHash(1, strings, nullptr, md5_hash);
      appendBinaryHex(file_hash, md5_hash, 16);
      XOCL_DPRINTF("Updating file hash orig: %s, new hash: %s\n", 
                   program->orig_file_hash.c_str(), file_hash.c_str());
      program->file_hash = file_hash;
      program->file_src = file_src;
    }
#endif
#ifdef EMIT_OPENCL_KERNEL_SRC
    std::string file_name = "_xt_ocl_src_" + program->file_hash + ".cl";
    std::ofstream fp(file_name);
    fp << program->file_src;
    fp.close();
#endif
    if (!mxpa::getNumKernels(program->file_hash)) {
      program->binary_type = CL_PROGRAM_BINARY_TYPE_NONE;
      program->build_status = CL_BUILD_ERROR;
    } else {
      program->binary_type = CL_PROGRAM_BINARY_TYPE_EXECUTABLE;
      program->build_status = CL_BUILD_SUCCESS;
    }
    if (options)
      program->build_options = options;
    if (pfn_notify)
      pfn_notify(program, user_data);
    return CL_SUCCESS;
  }

  cl_program clCreateProgramWithBuiltInKernels(cl_context context,
                                               cl_uint num_devices,
                                               const cl_device_id *device_list,
                                               const char *kernel_names,
                                               cl_int *errcode_ret) {
    return nullptr;
  }

  cl_int clRetainProgram(cl_program program) {
    if (program == nullptr)
      return CL_INVALID_PROGRAM;
    program->retain();
    return CL_SUCCESS;
  }

  cl_int clReleaseProgram(cl_program program) {
    if (program == nullptr)
      return CL_INVALID_PROGRAM;
    program->release();
    return CL_SUCCESS;
  }

  cl_int clGetProgramInfo(cl_program program, cl_program_info param_name,
                          size_t param_value_size, void *param_value,
                          size_t *param_value_size_ret) {
    if (program == nullptr)
      return CL_INVALID_PROGRAM;
    return program->getInfo(param_name, param_value_size, param_value,
                            param_value_size_ret);
  }

  cl_int clCreateKernelsInProgram(cl_program program, cl_uint num_kernels,
                                  cl_kernel *kernels, 
                                  cl_uint *num_kernels_ret) {
    cl_int ret_code = CL_SUCCESS;

    const std::vector<std::string> &krl_name_vec =
                     mxpa::getKernelNamesByHash(program->file_hash);

    size_t krl_cnt_in_prgm = krl_name_vec.size();

    if ((cl_uint)krl_cnt_in_prgm < num_kernels)
      return CL_INVALID_VALUE;

    if (kernels) {
      for (unsigned i = 0; i < krl_name_vec.size(); ++i)
        kernels[i] = new _cl_kernel(krl_name_vec[i].c_str(), program);
    }

    if (num_kernels_ret)
      *num_kernels_ret = (cl_uint) krl_cnt_in_prgm;

    return ret_code;
  }

  cl_mem clCreateBuffer(cl_context context, cl_mem_flags flags, size_t size,
                        void *host_ptr, cl_int *errcode_ret) {
    cl_int ret = CL_SUCCESS;
    cl_mem new_buf = nullptr;

    if (context == nullptr) {
      ret = CL_INVALID_CONTEXT;
      goto end;
    }

    if ((flags & CL_MEM_READ_WRITE)  && ((flags & CL_MEM_WRITE_ONLY) ||
                                         (flags & CL_MEM_READ_ONLY))) {
      ret = CL_INVALID_VALUE;
      goto end;
    }

    if (((flags & CL_MEM_WRITE_ONLY) && (flags & CL_MEM_READ_ONLY)) ||
        ((flags & CL_MEM_HOST_READ_ONLY) && (flags & CL_MEM_HOST_WRITE_ONLY)) ||
        ((flags & CL_MEM_ALLOC_HOST_PTR) && (flags & CL_MEM_USE_HOST_PTR)) ||
        ((flags & CL_MEM_COPY_HOST_PTR) && (flags & CL_MEM_USE_HOST_PTR))) {
      ret = CL_INVALID_VALUE;
      goto end;
    }

    if (((flags & CL_MEM_HOST_WRITE_ONLY) || (flags & CL_MEM_HOST_READ_ONLY)) &&
        (flags & CL_MEM_HOST_NO_ACCESS)) {
      ret = CL_INVALID_VALUE;
      goto end;
    }

    if (0 == size) {
      ret = CL_INVALID_BUFFER_SIZE;
      goto end;
    }

    if (((flags & CL_MEM_USE_HOST_PTR) || (flags & CL_MEM_COPY_HOST_PTR)) &&
        (nullptr == host_ptr)) {
      ret = CL_INVALID_HOST_PTR;
      goto end;
    }

    if (!((flags & CL_MEM_USE_HOST_PTR) || (flags & CL_MEM_COPY_HOST_PTR)) &&
        host_ptr) {
      ret = CL_INVALID_HOST_PTR;
      goto end;
    }

    ret = context->createBuffer(&new_buf, flags, size, host_ptr);

end:
    if (errcode_ret)
      *errcode_ret = ret;

    return new_buf;
  }

  cl_mem clCreateDMABuffer(cl_context context, cl_mem_flags flags, size_t size,
                           void *host_ptr, cl_int *errcode_ret, 
                           size_t cmx_size) {
    cl_int ret = CL_SUCCESS;
    cl_mem new_buf = NULL;
    ret = context->createBuffer(&new_buf, flags, size, host_ptr);
    if (errcode_ret)
      *errcode_ret = ret;
    return new_buf;
  }


  cl_int clFinish(cl_command_queue command_queue) {
    command_queue->run();
    command_queue->waitAll();
    return 0;
  }

  static bool isValidEventList(cl_uint num_events_in_wait_list,
                               const cl_event *event_wait_list) {
    if ((num_events_in_wait_list && !event_wait_list) ||
        (!num_events_in_wait_list && event_wait_list))
      return false;

    for (cl_uint i = 0; i < num_events_in_wait_list; ++i)
      if (event_wait_list[i] == nullptr)
        return false;
    return true;
  }

  cl_int clEnqueueReadBuffer(cl_command_queue command_queue, cl_mem buffer,
                             cl_bool blocking_read, size_t offset, size_t size,
                             void *ptr, cl_uint num_events_in_wait_list,
                             const cl_event *event_wait_list, cl_event *event) {
    if (nullptr == buffer)
      return CL_INVALID_MEM_OBJECT;
    if (nullptr == ptr)
      return CL_INVALID_VALUE;
    if (nullptr == command_queue)
      return CL_INVALID_COMMAND_QUEUE;
    if (!isValidContext(buffer, command_queue,
                        num_events_in_wait_list, event_wait_list))
      return CL_INVALID_CONTEXT;

    if ((offset > buffer->getSize()) || (size > buffer->getSize()) ||
        ((offset + size) > buffer->getSize()) || (size == 0))
      return CL_INVALID_VALUE;
    if ((buffer->getFlags() & CL_MEM_HOST_WRITE_ONLY) ||
        (buffer->getFlags() & CL_MEM_HOST_NO_ACCESS))
      return CL_INVALID_OPERATION;
    if (!isValidEventList(num_events_in_wait_list, event_wait_list))
      return CL_INVALID_EVENT_WAIT_LIST;

    return command_queue->enqueueReadBuffer(buffer,
                                            blocking_read, offset, size, ptr,
                                            num_events_in_wait_list, 
                                            event_wait_list, event);
  }

  cl_int clEnqueueWriteBuffer(cl_command_queue command_queue, cl_mem buffer,
                              cl_bool blocking_write, size_t offset, 
                              size_t size, const void *ptr, 
                              cl_uint num_events_in_wait_list,
                              const cl_event *event_wait_list, 
                              cl_event *event) {
    if (nullptr == buffer)
      return CL_INVALID_MEM_OBJECT;
    if (nullptr == ptr)
      return CL_INVALID_VALUE;
    if (nullptr == command_queue)
      return CL_INVALID_COMMAND_QUEUE;
    if (!isValidContext(buffer, command_queue,
                        num_events_in_wait_list, event_wait_list))
      return CL_INVALID_CONTEXT;

    if ((offset > buffer->getSize()) || (size > buffer->getSize()) ||
        ((offset + size) > buffer->getSize()) || (size == 0))
      return CL_INVALID_VALUE;
    if ((buffer->getFlags() & CL_MEM_HOST_READ_ONLY) ||
        (buffer->getFlags() & CL_MEM_HOST_NO_ACCESS))
      return CL_INVALID_OPERATION;
    if (!isValidEventList(num_events_in_wait_list, event_wait_list))
      return CL_INVALID_EVENT_WAIT_LIST;

    return command_queue->enqueueWriteBuffer(buffer, blocking_write, 
                                             offset, size, ptr, 
                                             num_events_in_wait_list,
                                             event_wait_list, event);
  }

  cl_kernel clCreateKernel(cl_program program, const char *kernel_name,
                           cl_int *errcode_ret) {
    if (!program) {
      if (errcode_ret)
        *errcode_ret = CL_INVALID_PROGRAM;
      return nullptr;
    }
    cl_kernel kernel = new _cl_kernel(kernel_name, program);
    if (errcode_ret) {
      if (!kernel->mxpa_kernel)
        *errcode_ret = CL_INVALID_KERNEL_NAME;
      else
        *errcode_ret = CL_SUCCESS;
    }
    return kernel;
  }

  cl_int clSetKernelArg(cl_kernel kernel, cl_uint arg_index, size_t arg_size,
                        const void *arg_value) {
    if (arg_index > CL_INT_MAX || 
        kernel->mxpa_kernel->numArguments() <= arg_index)
      return CL_INVALID_ARG_INDEX;
    if (0 == arg_size || arg_size > CL_INT_MAX)
      return CL_INVALID_ARG_SIZE;
    return kernel->mxpa_kernel->setArgument(arg_index, arg_size, arg_value);
  }

  static bool isValidGlobalWorkSize(cl_uint work_dim,
                                    const size_t *global_work_size) {
    if (nullptr == global_work_size)
      return false;

    for (cl_uint i = 0; i < work_dim; ++i) {
      if (0 == global_work_size[i])
        return false;
    }
    return true;
  }

  static bool isValidLocalWorkSize(cl_uint work_dim,
                                   const size_t *global_work_size, 
                                   const size_t *local_work_size) {
    if (local_work_size == nullptr)
      return true;

    for (cl_uint i = 0; i < work_dim ; ++i) {
      if ((global_work_size[i] % local_work_size[i]) != 0)
        return false;
    }
    return true;
  }

  static bool isValidWorkGroupSize(cl_uint work_dim,
                                   const size_t *global_work_size,
                                   const size_t *local_work_size,
                                   size_t *wgsize) {
    if (local_work_size == nullptr)
      return true;

    for (cl_uint i = 0; i < work_dim ; ++i) {
      if ((global_work_size[i] % local_work_size[i]) != 0 ||
          ((size_t)wgsize[0] != 0 &&
           (size_t)wgsize[1] != 0 && (size_t)wgsize[2] != 0 &&
           local_work_size[i] != (size_t)wgsize[i]))
        return false;
    }
    return true;
  }

  static void getPreferredLocalSize(cl_device_id dev,
                                     const size_t global[3], size_t local[3]) {
    size_t naturalWgSize = dev->getPreferedWGSizeMul();
    size_t prefMinNumWg = dev->getPreferredMinWGNum();

    assert(IsPowerOf2(naturalWgSize) && IsPowerOf2(prefMinNumWg) &&
           "The algorithm assume natural wg size and min num wg is power of 2");

    size_t gsize[3];
    size_t numWg = 1;
    for (int i = 0; i < 3; ++i) {
      gsize[i] = global[i];
      local[i] = 1;
      numWg = numWg * global[i];
    }

    for (int i = 0; i < 3; ++i) {
      // numWg is used for preventing from dividing gsize too much. so we can
      // keep a balance between total number work groups and local size.
      while (gsize[i] % 2 == 0 && local[i] < naturalWgSize &&
             numWg > prefMinNumWg) {
        numWg = numWg / 2;
        gsize[i] = gsize[i] / 2;
        local[i] = local[i] * 2;
      }
    }

    // LOCAL size can not too big. Because of the limit of dram
    while (local[0] * local[1] * local[2] > 1024 &&
           (local[0] % 2 == 0 || local[1] % 2 == 0 || local[2] % 2 == 0)) {
      for (int i = 0; i < 3; ++i) {
        if (local[i] % 2 == 0) {
          local[i] /= 2;
          break;
        }
      }
    }

    XOCL_DPRINTF_IF(XOCL_DEBUG_LAUNCH, "Adjusted local size = %zd %zd %zd\n",
                    local[0], local[1], local[2]);
  }

  cl_int clEnqueueNDRangeKernel(cl_command_queue command_queue, 
                                cl_kernel kernel,
                                cl_uint work_dim,
                                const size_t *global_work_offset,
                                const size_t *global_work_size,
                                const size_t *local_work_size,
                                cl_uint num_events_in_wait_list,
                                const cl_event *event_wait_list, 
                                cl_event *event) {
    if (work_dim < 1 || work_dim > 3)
      return CL_INVALID_WORK_DIMENSION;

    // Runtime check for xtensa-specific kernel attribute 'reqd_work_dim_size'.
    unsigned reqd_wd_size = kernel->mxpa_kernel->getReqdWorkDimSize();
    if (reqd_wd_size && reqd_wd_size != work_dim)
      return CL_INVALID_REQD_WORK_DIMENSION;
 
    if (!isValidGlobalWorkSize(work_dim, global_work_size))
      return  CL_INVALID_GLOBAL_WORK_SIZE;

    size_t wgsize[3] = {kernel->mxpa_kernel->getWorkGroup()[0],
                        kernel->mxpa_kernel->getWorkGroup()[1],
                        kernel->mxpa_kernel->getWorkGroup()[2]};

    size_t off[3] = {0, 0, 0}, g[3] = {1, 1, 1}, l[3] = {1, 1, 1};

    for (cl_uint i = 0; i < std::min(work_dim, (cl_uint)3); ++i) {
      off[i] = global_work_offset ? global_work_offset[i] : 0;
      g[i] = global_work_size[i];
      l[i] = local_work_size ? local_work_size[i] : 1;
      l[i] = wgsize[i] ? wgsize[i] : l[i];
    }

    if (!local_work_size)
      getPreferredLocalSize(command_queue->getDevice(), g, l);

    if (!isValidWorkGroupSize(work_dim, global_work_size, local_work_size, 
                              wgsize))
      return CL_INVALID_WORK_GROUP_SIZE;

    if (!isValidLocalWorkSize(work_dim, g, l))
      return CL_INVALID_WORK_ITEM_SIZE;

    if (!isValidContext(kernel, command_queue,
                        num_events_in_wait_list, event_wait_list))
      return CL_INVALID_CONTEXT;

    if (!isValidEventList(num_events_in_wait_list, event_wait_list))
      return CL_INVALID_EVENT_WAIT_LIST;

    kernel->mxpa_kernel->setGlobalSizes(g[0], g[1], g[2]);
    kernel->mxpa_kernel->setLocalSizes(l[0], l[1], l[2]);
    kernel->mxpa_kernel->setGlobalOffsets(off[0], off[1], off[2]);
    kernel->mxpa_kernel->setWorkDim(work_dim);
    kernel->retain();

    return command_queue->enqueueNDRangeKernel(kernel->mxpa_kernel,
                                               num_events_in_wait_list,
                                               event_wait_list, event);
  }

  cl_int clEnqueueTask(cl_command_queue command_queue, cl_kernel kernel,
                       cl_uint num_events_in_wait_list,
                       const cl_event *event_wait_list, cl_event *event) {
    if (command_queue == nullptr)
      return CL_INVALID_COMMAND_QUEUE;

    if (kernel == nullptr)
      return CL_INVALID_KERNEL;

    if (!isValidEventList(num_events_in_wait_list, event_wait_list))
      return CL_INVALID_EVENT_WAIT_LIST;

    if (!isValidContext(kernel, command_queue,
                        num_events_in_wait_list, event_wait_list))
      return CL_INVALID_CONTEXT;

    kernel->mxpa_kernel->setGlobalSizes(1, 1, 1);
    kernel->mxpa_kernel->setLocalSizes(1, 1, 1);
    kernel->mxpa_kernel->setGlobalOffsets(0, 0, 0);
    kernel->mxpa_kernel->setWorkDim(3);
    kernel->retain();

    return command_queue->enqueueNDRangeKernel(kernel->mxpa_kernel,
                                               num_events_in_wait_list,
                                               event_wait_list, event);
  }

  cl_int clGetPlatformInfo(cl_platform_id platform, cl_platform_info param_name,
                           size_t param_value_size, void *param_value,
                           size_t *param_value_size_ret) {
    if (platform == nullptr)
      return CL_INVALID_PLATFORM;
    if (platform != getPlatform())
      return CL_INVALID_PLATFORM;
    return platform->getInfo(param_name, param_value_size, param_value,
                             param_value_size_ret);
  }

  cl_int clGetDeviceInfo(cl_device_id device, cl_device_info param_name,
                         size_t param_value_size, void *param_value,
                         size_t *param_value_size_ret) {
    if (device == nullptr)
      return CL_INVALID_DEVICE;
    return device->getInfo(param_name, param_value_size, param_value,
                           param_value_size_ret);
  }

  cl_int clRetainDevice(cl_device_id device) {
    if (device == nullptr)
      return CL_INVALID_DEVICE;
    device->retain();
    return CL_SUCCESS;
  }

  cl_int clReleaseDevice(cl_device_id device) {
    if (device == nullptr)
      return CL_INVALID_DEVICE;
    device->release();
    return CL_SUCCESS;
  }

  cl_int clRetainContext(cl_context context) {
    if (context == nullptr)
      return CL_INVALID_CONTEXT;
    context->retain();
    return CL_SUCCESS;
  }

  cl_int clReleaseContext(cl_context context) {
    if (context == nullptr)
      return CL_INVALID_CONTEXT;
    context->release();
    return CL_SUCCESS;
  }

  cl_int clGetContextInfo(cl_context context, cl_context_info param_name,
                          size_t param_value_size, void *param_value,
                          size_t *param_value_size_ret) {
    if (context == nullptr)
      return CL_INVALID_CONTEXT;
    return context->getInfo(param_name, param_value_size, param_value,
                            param_value_size_ret);
  }

  cl_int clRetainCommandQueue(cl_command_queue command_queue) {
    if (command_queue == nullptr)
      return CL_INVALID_COMMAND_QUEUE;
    command_queue->retain();
    return CL_SUCCESS;
  }

  cl_int clReleaseCommandQueue(cl_command_queue command_queue) {
    if (command_queue == nullptr)
      return CL_INVALID_COMMAND_QUEUE;
    command_queue->run();
    command_queue->waitAll();
    command_queue->release();
    return CL_SUCCESS;
  }

  cl_int clGetCommandQueueInfo(cl_command_queue command_queue,
                               cl_command_queue_info param_name,
                               size_t param_value_size, void *param_value,
                               size_t *param_value_size_ret) {
    if (command_queue == nullptr)
      return CL_INVALID_COMMAND_QUEUE;
    return command_queue->getInfo(param_name, param_value_size, param_value,
                                  param_value_size_ret);
  }

  cl_mem clCreateSubBuffer(cl_mem buffer, cl_mem_flags flags,
                           cl_buffer_create_type buffer_create_type,
                           const void *buffer_create_info, 
                           cl_int *errcode_ret) {
    cl_int ret = CL_SUCCESS;
    cl_mem new_buf = nullptr;
    cl_buffer_region *buf_info = (cl_buffer_region *)buffer_create_info;
    cl_mem_flags flag_tmp = flags;

    if (nullptr == buffer_create_info) {
      ret = CL_INVALID_VALUE;
      goto bail;
    }

    if ((buf_info->origin + buf_info->size) > buffer->getSize()) {
      ret = CL_INVALID_VALUE;
      goto bail;
    }

    if (0 == buf_info->size) {
      ret = CL_INVALID_BUFFER_SIZE;
      goto bail;
    }

    if (CL_BUFFER_CREATE_TYPE_REGION != buffer_create_type) {
      ret = CL_INVALID_VALUE;
      goto bail;
    }

    if ((buffer->getFlags() == CL_MEM_WRITE_ONLY) && 
        ((flags == CL_MEM_READ_WRITE) || (flags == CL_MEM_READ_ONLY))) {
      ret = CL_INVALID_VALUE;
      goto bail;
    }

    if ((buffer->getFlags() == CL_MEM_READ_ONLY) && 
        ((flags == CL_MEM_READ_WRITE) || (flags == CL_MEM_WRITE_ONLY))) {
      ret = CL_INVALID_VALUE;
      goto bail;
    }

    if ((flags == CL_MEM_ALLOC_HOST_PTR) || 
        (flags == CL_MEM_USE_HOST_PTR) || 
        (flags == CL_MEM_COPY_HOST_PTR)) {
      ret = CL_INVALID_VALUE;
      goto bail;
    }

    if ((buffer->getFlags() == CL_MEM_HOST_WRITE_ONLY) && 
        (flags == CL_MEM_HOST_READ_ONLY)) {
      ret = CL_INVALID_VALUE;
      goto bail;
    }

    if ((buffer->getFlags() == CL_MEM_HOST_READ_ONLY) && 
        (flags == CL_MEM_HOST_WRITE_ONLY)) {
      ret = CL_INVALID_VALUE;
      goto bail;
    }

    if ((buffer->getFlags() == CL_MEM_HOST_NO_ACCESS) && 
        (flags == CL_MEM_HOST_READ_ONLY || flags == CL_MEM_HOST_WRITE_ONLY)) {
      ret = CL_INVALID_VALUE;
      goto bail;
    }

    if ((flags & 
         (CL_MEM_READ_WRITE | CL_MEM_READ_ONLY | CL_MEM_WRITE_ONLY)) == 0)
      flag_tmp |= buffer->getFlags() & (CL_MEM_READ_WRITE | CL_MEM_READ_ONLY |
                                        CL_MEM_WRITE_ONLY);

    flag_tmp |= buffer->getFlags() & 
                (CL_MEM_ALLOC_HOST_PTR | 
                 CL_MEM_COPY_HOST_PTR  |
                 CL_MEM_USE_HOST_PTR);

    if ((flags & (CL_MEM_HOST_READ_ONLY  | 
                  CL_MEM_HOST_WRITE_ONLY |
                  CL_MEM_HOST_NO_ACCESS)) == 0)
      flag_tmp |= buffer->getFlags() & 
                  (CL_MEM_HOST_READ_ONLY  |
                   CL_MEM_HOST_WRITE_ONLY |
                   CL_MEM_HOST_NO_ACCESS);

    ret = buffer->getContext()->createSubBuffer(&new_buf, buffer, flag_tmp,
                                                buf_info->size, 
                                                buf_info->origin);
bail:
    if (errcode_ret != nullptr)
      *errcode_ret = ret;

    return new_buf;
  }

  cl_int clRetainMemObject(cl_mem memobj) {
    if (memobj == nullptr)
      return CL_INVALID_MEM_OBJECT;
    memobj->retain();
    return CL_SUCCESS;
  }

  cl_int clReleaseMemObject(cl_mem memobj) {
    if (memobj == nullptr)
      return CL_INVALID_MEM_OBJECT;
    memobj->release();
    return CL_SUCCESS;
  }

  cl_int clGetMemObjectInfo(cl_mem memobj, cl_mem_info param_name,
                            size_t param_value_size, void *param_value,
                            size_t *param_value_size_ret) {
    if (memobj == nullptr)
      return CL_INVALID_MEM_OBJECT;
    return memobj->getInfo(param_name, param_value_size, param_value,
                           param_value_size_ret);
  }

  cl_int clSetMemObjectDestructorCallback(cl_mem memobj,
                                          void (*pfn_notify)(cl_mem memobj, 
                                                             void *user_data), 
                                          void *user_data) {
    if (memobj == nullptr)
      return CL_INVALID_MEM_OBJECT;
    return memobj->setMemObjectDestructorCallback(pfn_notify, user_data);
  }

  cl_int clRetainKernel(cl_kernel kernel) {
    if (kernel == nullptr)
      return CL_INVALID_KERNEL;
    kernel->retain();
    return CL_SUCCESS;
  }

  cl_int clReleaseKernel(cl_kernel kernel) {
    if (kernel == nullptr)
      return CL_INVALID_KERNEL;
    kernel->release();
    return CL_SUCCESS;
  }

  cl_int clGetKernelInfo(cl_kernel kernel, cl_kernel_info param_name,
                         size_t param_value_size, void *param_value,
                         size_t *param_value_size_ret) {
    if (kernel == nullptr)
      return CL_INVALID_KERNEL;
    return kernel->getInfo(param_name, param_value_size, param_value,
                           param_value_size_ret);
  }

  cl_int clGetKernelArgInfo(cl_kernel kernel,
                            cl_uint arg_indx,
                            cl_kernel_arg_info param_name,
                            size_t param_value_size,
                            void *param_value,
                            size_t *param_value_size_ret) {
    if (kernel == nullptr)
      return CL_INVALID_KERNEL;

    return kernel->getArgInfo(arg_indx, param_name, param_value_size, 
                              param_value, param_value_size_ret);
  }

  cl_int clGetKernelWorkGroupInfo(cl_kernel kernel,
                                  cl_device_id device,
                                  cl_kernel_work_group_info param_name,
                                  size_t param_value_size,
                                  void *param_value,
                                  size_t *param_value_size_ret) {
    switch (param_name) {
    case CL_KERNEL_WORK_GROUP_SIZE:
      return setParam<size_t>(param_value, device->getMaxWGSize(),
                              param_value_size, param_value_size_ret);

    case CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE :
      return setParam<size_t>(param_value, device->getPreferedWGSizeMul(),
                              param_value_size, param_value_size_ret);

    case CL_KERNEL_COMPILE_WORK_GROUP_SIZE: {
      ID3 required_wgz(kernel->mxpa_kernel->getWorkGroup()[0],
                       kernel->mxpa_kernel->getWorkGroup()[1],
                       kernel->mxpa_kernel->getWorkGroup()[2]);
      return setParam<ID3>(param_value, required_wgz,
                           param_value_size, param_value_size_ret);
    }
    case CL_KERNEL_LOCAL_MEM_SIZE: {
      unsigned local_size = 0;
      const xocl_opencl_kernel_arg_info *arg_info = 
                                         kernel->mxpa_kernel->kernelArgInfo();
      for (cl_uint arg_idx = 0;
           arg_idx < kernel->mxpa_kernel->numArguments();
           ++arg_idx) {
        if (CL_KERNEL_ARG_ADDRESS_LOCAL == arg_info[arg_idx].address_qualifier)
          local_size += kernel->mxpa_kernel->getArgSize(arg_idx);
      }
      // for kernel_fusion & dma_promotion
      local_size += (device->getLocalMemSize() / 4 * 3);
      //TODO
      //local_size += local mem in kernel;
      return setParam<cl_ulong>(param_value, local_size,
                                param_value_size, param_value_size_ret);
    }
    case CL_KERNEL_PRIVATE_MEM_SIZE:
      // not implemented, get those from compiler output
      return setParam<cl_ulong>(param_value, 0,
                                param_value_size, param_value_size_ret);

    case CL_KERNEL_GLOBAL_WORK_SIZE:
    // TODO: dummy, this param is only valid for built-in kernel type
    default:
      return CL_INVALID_VALUE;
    }
  }

  cl_int clWaitForEvents(cl_uint num_events,
                         const cl_event *event_list) {

    cl_int ret_code = CL_SUCCESS;

    if (!isValidEventList(num_events, event_list))
      ret_code = CL_INVALID_EVENT;

    std::vector<cl_command_queue> cmdqs;
    for (cl_uint i = 0; i < num_events; ++i) {
      cmdqs.push_back(event_list[i]->getCommandQueue());
      event_list[i]->retain();
    }
    sort(cmdqs.begin(), cmdqs.end());
    cmdqs.erase(std::unique(cmdqs.begin(), cmdqs.end()), cmdqs.end());
    // Launch commandqueue->run()
    for (unsigned i = 0; i < cmdqs.size(); ++i)
      cmdqs[i]->run();

    for (int i = 0; i < (int)num_events; ++i) {
      event_list[i]->innerEvent()->wait();
      event_list[i]->release();
    }

    if (0 == num_events || nullptr == event_list)
      ret_code = CL_INVALID_VALUE;

    for (cl_uint k = 0; k < num_events; ++k) {
      if (event_list[k]->getExeStatus())
        ret_code = CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST;
    }

    return ret_code;
  }

  cl_int clGetEventInfo(cl_event event,
                        cl_event_info param_name,
                        size_t param_value_size,
                        void *param_value,
                        size_t *param_value_size_ret) {
    return event->getInfo(param_name, param_value_size, param_value, 
                          param_value_size_ret);
  }

  cl_event clCreateUserEvent(cl_context context,
                             cl_int *errcode_ret) {
    return context->createUserEvent(errcode_ret);
  }

  cl_int clRetainEvent(cl_event event) {
    if (event == nullptr)
      return CL_INVALID_EVENT;
    event->retain();
    return CL_SUCCESS;
  }

  cl_int clReleaseEvent(cl_event event) {
    if (event == nullptr)
      return CL_INVALID_EVENT;
    event->release();
    return CL_SUCCESS;
  }

  cl_int clSetUserEventStatus(cl_event event,
                              cl_int execution_status) {
    if (!event)
      return CL_INVALID_EVENT;

    return event->setUserEventStatus(execution_status);
  }

  cl_int clSetEventCallback(cl_event event,
                            cl_int command_exec_callback_type,
                            void (CL_CALLBACK *pfn_notify)(cl_event, cl_int, 
                                                           void *),
                            void *user_data) {
    if (!event)
      return CL_INVALID_EVENT;

    return event->setEventCallback(command_exec_callback_type, pfn_notify,
                                   user_data);
  }

  cl_int clGetEventProfilingInfo(cl_event event,
                                 cl_profiling_info param_name,
                                 size_t param_value_size,
                                 void *param_value,
                                 size_t *param_value_size_ret) {

    if (!event)
      return CL_INVALID_EVENT;

    return event->getProfilingInfo(param_name, param_value_size, param_value, 
                                   param_value_size_ret);
  }

  cl_int clFlush(cl_command_queue command_queue) {
    command_queue->run();
    return CL_SUCCESS;
  }

  cl_int clEnqueueReadBufferRect(cl_command_queue command_queue, cl_mem buffer,
                                 cl_bool blocking_read,
                                 const size_t *buffer_origin,
                                 const size_t *host_origin, 
                                 const size_t *region,
                                 size_t buffer_row_pitch,
                                 size_t buffer_slice_pitch,
                                 size_t host_row_pitch, size_t host_slice_pitch,
                                 void *ptr, cl_uint num_events_in_wait_list,
                                 const cl_event *event_wait_list, 
                                 cl_event *event) {
    if (nullptr == buffer)
      return CL_INVALID_MEM_OBJECT;
    if (nullptr == ptr || ((region[0] * region[1] * region[2]) == 0))
      return CL_INVALID_VALUE;
    if (nullptr == command_queue)
      return CL_INVALID_COMMAND_QUEUE;
    if (!isValidContext(buffer, command_queue,
                        num_events_in_wait_list, event_wait_list))
      return CL_INVALID_CONTEXT;

    if ((buffer->getFlags() & CL_MEM_HOST_WRITE_ONLY) ||
        (buffer->getFlags() & CL_MEM_HOST_NO_ACCESS))
      return CL_INVALID_OPERATION;
    if (!isValidEventList(num_events_in_wait_list, event_wait_list))
      return CL_INVALID_EVENT_WAIT_LIST;

    if ((host_row_pitch && host_row_pitch < region[0]) ||
        (buffer_row_pitch && buffer_row_pitch < region[0]))
      return CL_INVALID_VALUE;

    if (buffer_slice_pitch && 
        ((buffer_slice_pitch < region[1] * buffer_row_pitch) || 
         (buffer_slice_pitch % buffer_row_pitch)))
      return CL_INVALID_VALUE;

    if (host_slice_pitch && 
        ((host_slice_pitch < region[1] * host_row_pitch) || 
         (host_slice_pitch % host_row_pitch)))
      return CL_INVALID_VALUE;

    buffer_row_pitch = buffer_row_pitch ? buffer_row_pitch : region[0];
    buffer_slice_pitch =
      buffer_slice_pitch ? buffer_slice_pitch : region[1] * buffer_row_pitch;
    host_row_pitch = host_row_pitch ? host_row_pitch : region[0];
    host_slice_pitch =
      host_slice_pitch ? host_slice_pitch : region[1] * host_row_pitch;

    if (buffer_origin[0] + region[0] - 1 +
        buffer_row_pitch * (buffer_origin[1] + region[1] - 1) +
        buffer_slice_pitch * (buffer_origin[2] + region[2] - 1) >= 
                                                              buffer->getSize())
      return CL_INVALID_VALUE;

    return command_queue->enqueueReadBufferRect(buffer, blocking_read, 
                                                buffer_origin, host_origin, 
                                                region, buffer_row_pitch, 
                                                buffer_slice_pitch, 
                                                host_row_pitch, 
                                                host_slice_pitch, ptr, 
                                                num_events_in_wait_list, 
                                                event_wait_list, event);
  }

  cl_int clEnqueueWriteBufferRect(cl_command_queue command_queue, cl_mem buffer,
                                  cl_bool blocking_write,
                                  const size_t *buffer_origin,
                                  const size_t *host_origin, 
                                  const size_t *region,
                                  size_t buffer_row_pitch,
                                  size_t buffer_slice_pitch,
                                  size_t host_row_pitch, 
                                  size_t host_slice_pitch,
                                  const void *ptr,
                                  cl_uint num_events_in_wait_list,
                                  const cl_event *event_wait_list,
                                  cl_event *event) {
    if (nullptr == buffer)
      return CL_INVALID_MEM_OBJECT;
    if (nullptr == ptr || ((region[0] * region[1] * region[2]) == 0))
      return CL_INVALID_VALUE;
    if (nullptr == command_queue)
      return CL_INVALID_COMMAND_QUEUE;
    if (!isValidContext(buffer, command_queue,
                        num_events_in_wait_list, event_wait_list))
      return CL_INVALID_CONTEXT;

    if ((buffer->getFlags() & CL_MEM_HOST_READ_ONLY)
        || (buffer->getFlags() & CL_MEM_HOST_NO_ACCESS))
      return CL_INVALID_OPERATION;

    if (!isValidEventList(num_events_in_wait_list, event_wait_list))
      return CL_INVALID_EVENT_WAIT_LIST;

    if ((host_row_pitch && host_row_pitch < region[0]) ||
        (buffer_row_pitch && buffer_row_pitch < region[0]))
      return CL_INVALID_VALUE;

    if (buffer_slice_pitch && 
        ((buffer_slice_pitch < region[1] * buffer_row_pitch) || 
         (buffer_slice_pitch % buffer_row_pitch)))
      return CL_INVALID_VALUE;

    if (host_slice_pitch && 
        ((host_slice_pitch < region[1] * host_row_pitch) || 
         (host_slice_pitch % host_row_pitch)))
      return CL_INVALID_VALUE;

    buffer_row_pitch = buffer_row_pitch ? buffer_row_pitch : region[0];
    buffer_slice_pitch =
      buffer_slice_pitch ? buffer_slice_pitch : region[1] * buffer_row_pitch;
    host_row_pitch = host_row_pitch ? host_row_pitch : region[0];
    host_slice_pitch =
      host_slice_pitch ? host_slice_pitch : region[1] * host_row_pitch;

    if (buffer_origin[0] + region[0] - 1 +
        buffer_row_pitch * (buffer_origin[1] + region[1] - 1) +
        buffer_slice_pitch * (buffer_origin[2] + region[2] - 1) >= 
                                                          buffer->getSize())
      return CL_INVALID_VALUE;

    return command_queue->enqueueWriteBufferRect(buffer, blocking_write, 
                                                 buffer_origin, host_origin, 
                                                 region, buffer_row_pitch, 
                                                 buffer_slice_pitch, 
                                                 host_row_pitch, 
                                                 host_slice_pitch, ptr, 
                                                 num_events_in_wait_list, 
                                                 event_wait_list, event);
  }

  cl_int clEnqueueFillBuffer(cl_command_queue command_queue, cl_mem buffer,
                             const void *pattern, size_t pattern_size,
                             size_t offset, size_t size,
                             cl_uint num_events_in_wait_list,
                             const cl_event *event_wait_list, cl_event *event) {
    if (nullptr == buffer)
      return CL_INVALID_MEM_OBJECT;

    if ((nullptr == pattern) || (pattern_size == 0) || !(pattern_size == 1 ||
        pattern_size == 2 || pattern_size == 4 || pattern_size == 8 ||
        pattern_size == 16 || pattern_size == 32 || pattern_size == 64 ||
        pattern_size == 128))
      return CL_INVALID_VALUE;

    if (nullptr == command_queue)
      return CL_INVALID_COMMAND_QUEUE;

    if (!isValidContext(buffer, command_queue,
                        num_events_in_wait_list, event_wait_list))
      return CL_INVALID_CONTEXT;

    if (!isValidEventList(num_events_in_wait_list, event_wait_list))
      return CL_INVALID_EVENT_WAIT_LIST;

    if (offset % pattern_size || size % pattern_size ||
        offset > buffer->getSize() || ((offset + size) > buffer->getSize()))
      return CL_INVALID_VALUE;

    return command_queue->enqueueFillBuffer(buffer, pattern, pattern_size, 
                                            offset, size, 
                                            num_events_in_wait_list, 
                                            event_wait_list, event);
  }

  cl_int clEnqueueCopyBuffer(cl_command_queue command_queue, cl_mem src_buffer,
                             cl_mem dst_buffer, size_t src_offset,
                             size_t dst_offset, size_t size,
                             cl_uint num_events_in_wait_list,
                             const cl_event *event_wait_list, cl_event *event) {
    if (nullptr == src_buffer || nullptr == dst_buffer)
      return CL_INVALID_MEM_OBJECT;

    if (nullptr == command_queue)
      return CL_INVALID_COMMAND_QUEUE;

    if ((src_buffer->getContext() != command_queue->getContext()) ||
        (dst_buffer->getContext() != command_queue->getContext()))
      return CL_INVALID_CONTEXT;
    if (!doesEventsHaveValidContext(command_queue,
                                    num_events_in_wait_list, event_wait_list))
      return CL_INVALID_CONTEXT;

    if ((src_offset + size > src_buffer->getSize()) ||
        (dst_offset + size > dst_buffer->getSize()))
      return CL_INVALID_VALUE;

    if (size == 0)
      return CL_INVALID_VALUE;

    /// TODO check subbuffer overlap
    if (src_buffer == dst_buffer)
      return CL_MEM_COPY_OVERLAP;

    if (!isValidEventList(num_events_in_wait_list, event_wait_list))
      return CL_INVALID_EVENT_WAIT_LIST;

    return command_queue->enqueueCopyBuffer(src_buffer, dst_buffer, src_offset, 
                                            dst_offset, size, 
                                            num_events_in_wait_list, 
                                            event_wait_list, event);
  }

  cl_int clEnqueueCopyBufferRect(cl_command_queue command_queue,
                                 cl_mem src_buffer,
                                 cl_mem dst_buffer, const size_t *src_origin,
                                 const size_t *dst_origin, const size_t *region,
                                 size_t src_row_pitch, size_t src_slice_pitch,
                                 size_t dst_row_pitch, size_t dst_slice_pitch,
                                 cl_uint num_events_in_wait_list,
                                 const cl_event *event_wait_list, 
                                 cl_event *event) {
    if (nullptr == src_buffer || nullptr == dst_buffer)
      return CL_INVALID_MEM_OBJECT;

    if (nullptr == command_queue)
      return CL_INVALID_COMMAND_QUEUE;

    if ((src_buffer->getContext() != command_queue->getContext()) ||
        (dst_buffer->getContext() != command_queue->getContext()))
      return CL_INVALID_CONTEXT;

    if (!doesEventsHaveValidContext(command_queue,
                                    num_events_in_wait_list, event_wait_list))
      return CL_INVALID_CONTEXT;

    if ((region[0] * region[1] * region[2]) == 0)
      return CL_INVALID_VALUE;

    if ((src_row_pitch && src_row_pitch < region[0]) ||
        (dst_row_pitch && dst_row_pitch < region[0]))
      return CL_INVALID_VALUE;

    if (src_slice_pitch && 
        ((src_slice_pitch < region[1] * src_row_pitch) || 
         (src_slice_pitch % src_row_pitch)))
      return CL_INVALID_VALUE;

    if (dst_slice_pitch && 
        ((dst_slice_pitch < region[1] * dst_row_pitch) || 
         (dst_slice_pitch % dst_row_pitch)))
      return CL_INVALID_VALUE;

    if (dst_buffer == src_buffer) {
      if ((src_slice_pitch != dst_slice_pitch) || 
          (src_row_pitch != dst_row_pitch))
        return CL_INVALID_VALUE;
    }

    if (!isValidEventList(num_events_in_wait_list, event_wait_list))
      return CL_INVALID_EVENT_WAIT_LIST;
    // TODO check overlap
    // TODO check subbuffer alignment

    src_row_pitch = src_row_pitch ? src_row_pitch : region[0];
    dst_row_pitch = dst_row_pitch ? dst_row_pitch : region[0];
    src_slice_pitch = src_slice_pitch ? src_slice_pitch 
                                      : region[1] * src_row_pitch;
    dst_slice_pitch = dst_slice_pitch ? dst_slice_pitch 
                                      : region[1] * dst_row_pitch;

    size_t cb = region[0] * region[1] * region[2];
    size_t src_offset = src_origin[2] * src_slice_pitch +
                        src_origin[1] * src_row_pitch + src_origin[0];
    size_t dst_offset = dst_origin[2] * dst_slice_pitch +
                        dst_origin[1] * dst_row_pitch + dst_origin[0];

    if ((src_offset > src_buffer->getSize()) || (cb > src_buffer->getSize()) ||
        (dst_offset > dst_buffer->getSize()) || (cb > dst_buffer->getSize()) ||
        ((src_offset + cb) > src_buffer->getSize()) ||
        ((dst_offset + cb) > dst_buffer->getSize()))
      return CL_INVALID_VALUE;

    return command_queue->enqueueCopyBufferRect(src_buffer, dst_buffer, 
                                                src_offset, dst_offset, 
                                                src_origin, dst_origin, 
                                                region, src_row_pitch, 
                                                src_slice_pitch, dst_row_pitch,
                                                dst_slice_pitch, 
                                                num_events_in_wait_list, 
                                                event_wait_list, event);
  }

  void *clEnqueueMapBuffer(cl_command_queue command_queue, cl_mem buffer,
                           cl_bool blocking_map, cl_map_flags map_flags,
                           size_t offset, size_t size,
                           cl_uint num_events_in_wait_list,
                           const cl_event *event_wait_list, cl_event *event,
                           cl_int *errcode_ret) {
    cl_int ret = CL_SUCCESS;
    void *mapped = nullptr;

    if (nullptr == buffer) {
      ret = CL_INVALID_MEM_OBJECT;
      goto end;
    }

    if (nullptr == command_queue) {
      ret = CL_INVALID_COMMAND_QUEUE;
      goto end;
    }

    if (!isValidContext(buffer, command_queue,
                        num_events_in_wait_list, event_wait_list)) {
      ret = CL_INVALID_CONTEXT;
      goto end;
    }

    if ((offset > buffer->getSize()) || (size > buffer->getSize()) ||
        ((offset + size) > buffer->getSize())) {
      ret = CL_INVALID_VALUE;
      goto end;
    }

    if (size == 0) {
      ret = CL_INVALID_VALUE;
      goto end;
    }

    if (!(map_flags == (CL_MAP_READ | CL_MAP_WRITE) ||
          map_flags == CL_MAP_READ || map_flags == CL_MAP_WRITE ||
          map_flags == CL_MAP_WRITE_INVALIDATE_REGION)) {
      ret = CL_INVALID_VALUE;
      goto end;
    }

    if (((buffer->getFlags() & CL_MEM_HOST_WRITE_ONLY) ||
         (buffer->getFlags() & CL_MEM_HOST_NO_ACCESS)) &&
        (map_flags == CL_MAP_READ)) {
      ret = CL_INVALID_OPERATION;
      goto end;
    }

    if (((buffer->getFlags() & CL_MEM_HOST_READ_ONLY) ||
         (buffer->getFlags() & CL_MEM_HOST_NO_ACCESS)) &&
        ((map_flags == CL_MAP_WRITE) ||
         (map_flags == CL_MAP_WRITE_INVALIDATE_REGION))) {
      ret = CL_INVALID_OPERATION;
      goto end;
    }

    if (!isValidEventList(num_events_in_wait_list, event_wait_list)) {
      ret = CL_INVALID_EVENT_WAIT_LIST;
      goto end;
    }

    mapped = command_queue->enqueueMapBuffer(buffer, blocking_map, map_flags, 
                                             offset, size, 
                                             num_events_in_wait_list, 
                                             event_wait_list, event, 
                                             errcode_ret);
end:
    if (errcode_ret)
      *errcode_ret = ret;

    return mapped;
  }

  cl_int clEnqueueUnmapMemObject(cl_command_queue command_queue, cl_mem memobj,
                                 void *mapped_ptr, 
                                 cl_uint num_events_in_wait_list,
                                 const cl_event *event_wait_list, 
                                 cl_event *event) {
    if (nullptr == memobj)
      return CL_INVALID_MEM_OBJECT;

    if (nullptr == mapped_ptr)
      return CL_INVALID_VALUE;

    if (nullptr == command_queue)
      return CL_INVALID_COMMAND_QUEUE;

    if (!isValidContext(memobj, command_queue,
                        num_events_in_wait_list, event_wait_list))
      return CL_INVALID_CONTEXT;

    if (!isValidEventList(num_events_in_wait_list, event_wait_list))
      return CL_INVALID_EVENT_WAIT_LIST;

    return command_queue->enqueueUnmapMemObject(memobj, mapped_ptr, 
                                                num_events_in_wait_list, 
                                                event_wait_list, event);
  }

  cl_int clEnqueueMarker(cl_command_queue command_queue, cl_event *event) {
    if (command_queue == nullptr)
      return CL_INVALID_COMMAND_QUEUE;

    if (event == nullptr)
      return CL_INVALID_VALUE;

    return command_queue->enqueueMarker(event);
  }

  cl_int clEnqueueWaitForEvents(cl_command_queue command_queue, 
                                cl_uint num_events,
                                const cl_event *event_list) {
    return command_queue->enqueueWaitForEvents(num_events, event_list);
  }

  cl_int clEnqueueMarkerWithWaitList(cl_command_queue command_queue,
                                     cl_uint num_events_in_wait_list,
                                     const cl_event *event_wait_list,
                                     cl_event *event) {
    if (command_queue == nullptr)
      return CL_INVALID_COMMAND_QUEUE;

    if (!isValidEventList(num_events_in_wait_list, event_wait_list))
      return CL_INVALID_EVENT_WAIT_LIST;

    if (!doesEventsHaveValidContext(command_queue,
                                    num_events_in_wait_list, event_wait_list))
      return CL_INVALID_CONTEXT;

    if ((event != nullptr))
      return command_queue->enqueueMarker(event);

    return CL_SUCCESS;
  }

  cl_int clEnqueueBarrierWithWaitList(cl_command_queue command_queue,
                                      cl_uint num_events_in_wait_list,
                                      const cl_event *event_wait_list,
                                      cl_event *event) {
    if (command_queue == nullptr)
      return CL_INVALID_COMMAND_QUEUE;

    if (!isValidEventList(num_events_in_wait_list, event_wait_list))
      return CL_INVALID_EVENT_WAIT_LIST;

    //if (num_events_in_wait_list) {
    //  clWaitForEvents(num_events_in_wait_list, event_wait_list);
    //}

    return CL_SUCCESS;
  }

  cl_int clEnqueueBarrier(cl_command_queue command_queue) {
    if (command_queue == nullptr)
      return CL_INVALID_COMMAND_QUEUE;

    return CL_SUCCESS;
  }

  void *clGetExtensionFunctionAddress(const char *func_name) {
    return nullptr;
  }

  void *clGetExtensionFunctionAddressForPlatform(cl_platform_id platform,
                                                 const char *func_name) {
    return CL_SUCCESS;
  }

  cl_int clEnqueueReadImage(cl_command_queue command_queue,
                            cl_mem image,
                            cl_bool blocking_read,
                            const size_t origin[3],
                            const size_t region[3],
                            size_t row_pitch,
                            size_t slice_pitch,
                            void *ptr,
                            cl_uint num_events_in_wait_list,
                            const cl_event *event_wait_list,
                            cl_event *event) {
    return CL_SUCCESS;
  }

  cl_int clEnqueueWriteImage(cl_command_queue command_queue,
                             cl_mem image,
                             cl_bool blocking_write,
                             const size_t origin[3],
                             const size_t region[3],
                             size_t input_row_pitch,
                             size_t input_slice_pitch,
                             const void *ptr,
                             cl_uint num_events_in_wait_list,
                             const cl_event *event_wait_list,
                             cl_event *event) {
    return CL_SUCCESS;
  }

  cl_int clEnqueueFillImage(cl_command_queue command_queue,
                            cl_mem image,
                            const void *fill_color,
                            const size_t origin[3],
                            const size_t region[3],
                            cl_uint num_events_in_wait_list,
                            const cl_event *event_wait_list,
                            cl_event *event) {
    return CL_SUCCESS;
  }

  cl_int clEnqueueCopyImage(cl_command_queue command_queue,
                            cl_mem src_image,
                            cl_mem dst_image,
                            const size_t src_origin[3],
                            const size_t dst_origin[3],
                            const size_t region[3],
                            cl_uint num_events_in_wait_list,
                            const cl_event *event_wait_list,
                            cl_event *event) {
    return CL_SUCCESS;
  }

  cl_int clEnqueueCopyImageToBuffer(cl_command_queue command_queue,
                                    cl_mem src_image,
                                    cl_mem dst_buffer,
                                    const size_t src_origin[3],
                                    const size_t region[3],
                                    size_t dst_offset,
                                    cl_uint num_events_in_wait_list,
                                    const cl_event *event_wait_list,
                                    cl_event *event) {
    return CL_SUCCESS;
  }

  cl_int clEnqueueCopyBufferToImage(cl_command_queue command_queue,
                                    cl_mem src_buffer,
                                    cl_mem dst_image,
                                    size_t src_offset,
                                    const size_t dst_origin[3],
                                    const size_t region[3],
                                    cl_uint num_events_in_wait_list,
                                    const cl_event *event_wait_list,
                                    cl_event *event) {
    return CL_SUCCESS;
  }

  void *clEnqueueMapImage(cl_command_queue command_queue,
                          cl_mem image,
                          cl_bool blocking_map,
                          cl_map_flags map_flags,
                          const size_t origin[3],
                          const size_t region[3],
                          size_t *image_row_pitch,
                          size_t *image_slice_pitch,
                          cl_uint num_events_in_wait_list,
                          const cl_event *event_wait_list,
                          cl_event *event,
                          cl_int *errcode_ret) {
    return nullptr;
  }

  cl_mem clCreateImage2D(cl_context context,
                         cl_mem_flags flags,
                         const cl_image_format *image_format,
                         size_t image_width,
                         size_t image_height,
                         size_t image_row_pitch,
                         void *host_ptr,
                         cl_int *errcode_ret) {
    return nullptr;
  }

  cl_mem clCreateImage3D(cl_context context,
                         cl_mem_flags flags,
                         const cl_image_format *image_format,
                         size_t image_width,
                         size_t image_height,
                         size_t image_depth,
                         size_t image_row_pitch,
                         size_t image_slice_pitch,
                         void *host_ptr,
                         cl_int *errcode_ret) {
    return nullptr;
  }

  cl_int clEnqueueMigrateMemObjects(cl_command_queue command_queue,
                                    cl_uint num_mem_objects,
                                    const cl_mem *mem_objects,
                                    cl_mem_migration_flags flags,
                                    cl_uint num_events_in_wait_list,
                                    const cl_event *event_wait_list,
                                    cl_event *event) {
    return CL_SUCCESS;
  }

  cl_sampler clCreateSampler(cl_context context,
                             cl_bool normalized_coords,
                             cl_addressing_mode addressing_mode,
                             cl_filter_mode filter_mode,
                             cl_int *errcode_ret) {
    return nullptr;
  }

  cl_int clRetainSampler(cl_sampler sampler) {
    return CL_SUCCESS;
  }

  cl_int clReleaseSampler(cl_sampler sampler) {
    return CL_SUCCESS;
  }

  cl_int clGetSamplerInfo(cl_sampler sampler,
                          cl_sampler_info param_name,
                          size_t param_value_size,
                          void *param_value,
                          size_t *param_value_size_ret) {
    return CL_SUCCESS;
  }

  cl_mem clCreateImage(cl_context context,
                       cl_mem_flags flags,
                       const cl_image_format *image_format,
                       const cl_image_desc *image_desc,
                       void *host_ptr,
                       cl_int *errcode_ret) {
    return nullptr;
  }

  cl_int clGetImageInfo(cl_mem image,
                        cl_image_info param_name,
                        size_t param_value_size,
                        void *param_value,
                        size_t *param_value_size_ret) {
    return CL_SUCCESS;
  }

  cl_int clGetSupportedImageFormats(cl_context context,
                                    cl_mem_flags flags,
                                    cl_mem_object_type image_type,
                                    cl_uint num_entries,
                                    cl_image_format *image_formats,
                                    cl_uint *num_image_formats) {
    return CL_SUCCESS;
  }

  cl_int clCreateSubDevices(cl_device_id in_device,
                            const cl_device_partition_property *properties,
                            cl_uint num_devices,
                            cl_device_id *out_devices,
                            cl_uint *num_devices_ret) {
    if (!properties)
      return CL_INVALID_VALUE;
    if (!in_device)
      return CL_INVALID_DEVICE;

    unsigned _num_devices_ret = 0;
    unsigned ret = in_device->createSubDevices(properties, num_devices,
                                               out_devices, &_num_devices_ret);
    if (ret)
      return ret;

    if (num_devices_ret)
      *num_devices_ret = _num_devices_ret;

    if (out_devices) {
      for (unsigned i = 0; i < _num_devices_ret; ++i)
        getPlatform()->addDevice(out_devices[i]);
    }

    return CL_SUCCESS;
  }

  cl_int clEnqueueNativeKernel(cl_command_queue command_queue,
                               void (*user_func)(void *),
                               void *args,
                               size_t cb_args,
                               cl_uint num_mem_objects,
                               const cl_mem *mem_list,
                               const void **args_mem_loc,
                               cl_uint num_events_in_wait_list,
                               const cl_event *event_wait_list,
                               cl_event *event) {
    return CL_SUCCESS;
  }

  cl_program clCreateProgramWithIL(cl_context context,
                                   const void *il,
                                   size_t length,
                                   cl_int *errcode_ret) {
    return nullptr;
  }
 
  cl_program clCreateProgramWithBinary(cl_context context,
                                       cl_uint num_devices,
                                       const cl_device_id *device_list,
                                       const size_t *lengths,
                                       const unsigned char **binaries,
                                       cl_int *binary_status,
                                       cl_int *errcode) {

    if (context == nullptr) {
      if (errcode != nullptr)
        *errcode = CL_INVALID_CONTEXT;
      return nullptr;
    }

    cl_int ret_code = CL_SUCCESS;
    cl_int b_status = CL_SUCCESS;
    cl_program program = nullptr;
    std::string hash_name;
    bool isIn = false;
    md5_byte_t md5_hash[16];
    std::string file_hash;
    char *file_src;
    const char *strings[1];
    int len;

    if (!device_list || !num_devices || !lengths || !binaries) {
      ret_code = b_status = CL_INVALID_VALUE;
      goto end;
    }

    for (cl_uint i = 0; i < sizeof(*lengths) / sizeof(size_t); i++) {
      if (!lengths[i] || !binaries[i]) {
        ret_code = b_status = CL_INVALID_VALUE;
        goto end;
      }
    }

    for (cl_uint i = 0; i < num_devices; ++i) {
      if (device_list[i] == (context->getDeviceList())[0]) {
        isIn = true;
        break;
      }
    }

    if (isIn == false) {
      ret_code = CL_INVALID_DEVICE;
      b_status = CL_INVALID_VALUE;
      goto end;
    }

    len = lengths[0]-4;
    file_src = (char *)malloc(len+1);
    file_src[len] = 0;
    strings[0] = file_src;
    memcpy((void *)file_src, ((const unsigned char **)binaries)[0], len);
    getBinariesHash(1, strings, nullptr, md5_hash);
    appendBinaryHex(hash_name, md5_hash, 16);

    program = new _cl_program(context, hash_name);
    if (nullptr == program)
      ret_code = CL_OUT_OF_HOST_MEMORY;

    if (program) {
      // Append binary type to program
      program->binary_type = 
               *(int *)
               (((const unsigned char **)binaries)[0] + len);
    }
    getKernelSrc(program->file_src, 1, strings);

end:
    if (errcode)
      *errcode = ret_code;

    if (binary_status)
      *binary_status = b_status;

    return program;
  }

  cl_int clGetProgramBuildInfo(cl_program program,
                               cl_device_id device,
                               cl_program_build_info param_name,
                               size_t param_value_size,
                               void *param_value,
                               size_t *param_value_size_ret) {
    switch (param_name) {
    case CL_PROGRAM_BUILD_OPTIONS: {
      if (param_value) {
        memcpy((char *)param_value, program->build_options.c_str(), 
               param_value_size-1);
        *((char *)param_value + param_value_size - 1) = '\0';
      } 
      if (param_value_size_ret)
        *((cl_int *)param_value_size_ret) = program->build_options.length()+1;
      return CL_SUCCESS;
    }
    case CL_PROGRAM_BUILD_LOG: {
      const char *log = "CL_BUILD_SUCCESS";
      if (param_value) {
        unsigned len = strlen(log)+1 < param_value_size ? strlen(log)+1
                                                        : param_value_size;
        memcpy((char *)param_value, "CL_BUILD_SUCCESS\0", len);
      }
      if (param_value_size_ret)
        *param_value_size_ret = strlen(log)+1;
      return CL_SUCCESS;
    }
    case CL_PROGRAM_BUILD_STATUS: {
      if (param_value)
        *((cl_int *)param_value) = program->build_status;
      if (param_value_size_ret)
        *param_value_size_ret = sizeof(cl_int);
      return CL_SUCCESS;
    }
    case CL_PROGRAM_BINARY_TYPE: {
      if (param_value)
        *((cl_int *)param_value) = program->binary_type;
      if (param_value_size_ret)
        *((cl_int *)param_value_size_ret) = sizeof(cl_int);
      return CL_SUCCESS;
    }
    }
    return CL_SUCCESS;
  }

  cl_int clCompileProgram(cl_program program,
                          cl_uint num_devices,
                          const cl_device_id *device_list,
                          const char *options,
                          cl_uint num_input_headers,
                          const cl_program *input_headers,
                          const char **header_include_names,
                          void (*pfn_notify)(cl_program program, 
                          void *user_data),
                          void *user_data) {
    std::string file_src;
    std::string process_options;
#ifdef PROCESS_OPENCL_BUILD_OPTIONS
    if (options) {
      getPreProcessorOptions(process_options, options);
    }
    if (!process_options.empty()) {
      file_src = process_options;
      file_src += program->orig_file_src;
      // Update hash with the build options
      md5_byte_t md5_hash[16];
      std::string file_hash;
      const char *strings[] = {file_src.c_str()};
      getBinariesHash(1, strings, nullptr, md5_hash);
      appendBinaryHex(file_hash, md5_hash, 16);
      XOCL_DPRINTF("Updating file hash orig: %s, new hash: %s\n", 
                   program->orig_file_hash.c_str(), file_hash.c_str());
      program->file_hash = file_hash;
      program->file_src = file_src;
    }
#endif
    // Update with headers
    const char *strings[num_input_headers];
    for (unsigned i = 0; i < num_input_headers; ++i) {
      strings[i] = input_headers[i]->file_src.c_str();
    }
    std::string headers_src;
    getKernelSrc(headers_src, num_input_headers, strings);
    headers_src += program->file_src; 
    // headers_src = commentOutIncludes(headers_src);
    {
      // Update hash
      md5_byte_t md5_hash[16];
      std::string file_hash;
      const char *strings[] = {headers_src.c_str()};
      getBinariesHash(1, strings, nullptr, md5_hash);
      appendBinaryHex(file_hash, md5_hash, 16);
      program->file_hash = file_hash;
      program->file_src = headers_src;
    }
    program->binary_type = CL_PROGRAM_BINARY_TYPE_COMPILED_OBJECT;
    program->build_status = CL_BUILD_SUCCESS;
#ifdef EMIT_OPENCL_KERNEL_SRC
    std::string file_name = "_xt_ocl_src_" + program->file_hash + ".cl";
    std::ofstream fp(file_name);
    fp << program->file_src;
    fp.close();
#endif
    if (options)
      program->build_options = options;
    if (pfn_notify)
      pfn_notify(program, user_data);
    return CL_SUCCESS;
  }

  cl_program clLinkProgram(cl_context context,
                           cl_uint num_devices,
                           const cl_device_id *device_list,
                           const char *options,
                           cl_uint num_input_programs,
                           const cl_program *input_programs,
                           void (*pfn_notify)(cl_program program, 
                           void *user_data),
                           void *user_data,
                           cl_int *errcode_ret) {
    
    cl_int ret_code = CL_SUCCESS;

    if (pfn_notify)
      pfn_notify(nullptr, user_data);

    const char *strings[num_input_programs];
    size_t lengths[num_input_programs]; 
    
    for (unsigned i = 0; i < num_input_programs; ++i) {
      strings[i] = input_programs[i]->file_src.c_str();
      lengths[i] = input_programs[i]->file_src.length();
    }

    md5_byte_t md5_hash[16];
    std::string file_hash;
    getBinariesHash(num_input_programs, strings, lengths, md5_hash);
    appendBinaryHex(file_hash, md5_hash, 16);
    cl_program prgm = new _cl_program(context, file_hash);
    if (nullptr == prgm) {
      ret_code = CL_OUT_OF_HOST_MEMORY;
    } else {
      getKernelSrc(prgm->file_src, num_input_programs, strings);
      prgm->orig_file_src = prgm->file_src;
#ifdef EMIT_OPENCL_KERNEL_SRC
      std::string file_name = "_xt_ocl_src_" + prgm->file_hash + ".cl";
      std::ofstream fp(file_name);
      fp << prgm->file_src;
      fp.close();
#endif
      prgm->build_status = CL_BUILD_SUCCESS;
      if (options)
        prgm->binary_type = !strcmp(options, "-create-library") 
                            ? CL_PROGRAM_BINARY_TYPE_LIBRARY
                            : CL_PROGRAM_BINARY_TYPE_EXECUTABLE;
      else
        prgm->binary_type = CL_PROGRAM_BINARY_TYPE_EXECUTABLE;
    }
    if (errcode_ret)
      *errcode_ret = ret_code;
    return prgm;
  }

  cl_int clUnloadPlatformCompiler(cl_platform_id platform) {
    return CL_SUCCESS;
  }

  cl_int clUnloadCompiler(void) {
    return CL_SUCCESS;
  }

} // extern "C"
