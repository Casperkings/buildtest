/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#define CL_TARGET_OPENCL_VERSION 120
#include <CL/opencl.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <md5.h>

static inline int writeSource(int count, const char **strings,
                              const size_t *length, const char *kernelfile) {
  if (*strings == nullptr) {
    return -1;
  }

  FILE *fio = fopen(kernelfile, "wb");
  if (nullptr == fio) {
    return -1;
  }

  if (length != nullptr) {
    for (int i = 0; i < count; ++i) {
      int str_len = length[i];
      if (0 == str_len) {
        str_len = strlen(strings[i]);
      }
      fwrite(strings[i], str_len, 1, fio);
    }
  } else {
    for (int i = 0; i < count; ++i) {
      size_t str_len = strlen(strings[i]);
      fwrite(strings[i], str_len, 1, fio);
    }
  }
  fclose(fio);
  return 0;
}

static inline void getBinariesHash(size_t count, const char **strings,
                                   const size_t *length, md5_byte_t *md5_hash) {
  md5_state_t program_md5;
  md5_init(&program_md5);
  if (length != nullptr) {
    for (int i = 0; i < (int)count; i++) {
      if (0 == length[i]) {
        md5_append(&program_md5, (const md5_byte_t *)strings[i], 
                   (int)strlen(strings[i]));
      } else {
        md5_append(&program_md5, (const md5_byte_t *)strings[i], 
                   (int)length[i]);
      }
    }
  } else {
    for (int i = 0; i < (int)count; i++) {
      md5_append(&program_md5, (const md5_byte_t *)strings[i], 
                 (int)strlen(strings[i]));
    }
  }
  md5_finish(&program_md5, md5_hash);
}

static inline void appendBinaryHex(std::string &str, 
                                   const unsigned char *binary,
                                   size_t binary_len) {
  char hex_buf[4];
  for (size_t i = 0; i < binary_len; i++) {
    sprintf(hex_buf, "%02x", (unsigned int) binary[i]);
    str.append(hex_buf, 2);
  }
}

// OpenCL API Implementations
extern "C" {

  cl_program clCreateProgramWithSource(cl_context context, cl_uint count,
                                       const char **strings,
                                       const size_t *lengths,
                                       cl_int *errcode_ret) {
    md5_byte_t md5_hash[16];
    std::string src_fname = "kernel_";
    getBinariesHash(count, strings, lengths, md5_hash);
    appendBinaryHex(src_fname, md5_hash, 16);
    src_fname.append(".cl");
    writeSource(count, strings, lengths, src_fname.c_str()); 
    return nullptr;
  }

  cl_int clGetPlatformIDs(cl_uint num_entries, cl_platform_id *platforms,
                          cl_uint *num_platforms) {
    return CL_SUCCESS;
  }

  cl_int clGetDeviceIDs(cl_platform_id platform, cl_device_type device_type,
                        cl_uint num_entries, cl_device_id *devices,
                        cl_uint *num_devices) {
    return CL_SUCCESS;
  }

  cl_context clCreateContext(const cl_context_properties *properties,
                             cl_uint num_devices, const cl_device_id *devices,
                             void (CL_CALLBACK * /* pfn_notify */)(const char *,
                                                                   const void *,
                                                                   size_t, 
                                                                   void *),
                             void * /* user_data */, cl_int *errcode_ret) {
    return nullptr;
  }

  cl_context clCreateContextFromType(const cl_context_properties *properties,
                                     cl_device_type device_type,
                                     void (CL_CALLBACK * /* pfn_notify */)(
                                         const char *, const void *, 
                                         size_t, void *),
                                     void * /* user_data */, 
                                     cl_int *errcode_ret) {
    return nullptr;
  }

  cl_command_queue clCreateCommandQueue(cl_context context, cl_device_id device,
                                        cl_command_queue_properties properties,
                                        cl_int *errcode_ret) {
    return nullptr;
  }

  cl_command_queue clCreateCommandQueueWithProperties(
                                       cl_context context, 
                                       cl_device_id device,
                                       cl_command_queue_properties *properties,
                                       cl_int *errcode_ret) {
    return nullptr;
  }

  cl_int clBuildProgram(cl_program program, cl_uint num_devices,
                        const cl_device_id *device_list, const char *options,
                        void (*pfn_notify)(cl_program, void *user_data),
                        void *user_data) {
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
    return CL_SUCCESS;
  }

  cl_int clReleaseProgram(cl_program program) {
    return CL_SUCCESS;
  }

  cl_int clGetProgramInfo(cl_program program, cl_program_info param_name,
                          size_t param_value_size, void *param_value,
                          size_t *param_value_size_ret) {
    return CL_SUCCESS;
  }

  cl_int clCreateKernelsInProgram(cl_program program, cl_uint num_kernels,
                                  cl_kernel *kernels, cl_uint *num_kernels_ret) {
    return CL_SUCCESS;
  }

  cl_mem clCreateBuffer(cl_context context, cl_mem_flags flags, size_t size,
                        void *host_ptr, cl_int *errcode_ret) {
    return nullptr;
  }

  cl_mem clCreateDMABuffer(cl_context context, cl_mem_flags flags, size_t size,
                           void *host_ptr, cl_int *errcode_ret, size_t cmx_size) {
    return nullptr;
  }

  cl_int clFinish(cl_command_queue command_queue) {
    return CL_SUCCESS;
  }

  cl_int clEnqueueReadBuffer(cl_command_queue command_queue, cl_mem buffer,
                             cl_bool blocking_read, size_t offset, size_t size,
                             void *ptr, cl_uint num_events_in_wait_list,
                             const cl_event *event_wait_list, cl_event *event) {
    return CL_SUCCESS;
  }

  cl_int clEnqueueWriteBuffer(cl_command_queue command_queue, cl_mem buffer,
                              cl_bool blocking_write, size_t offset, 
                              size_t size, const void *ptr, 
                              cl_uint num_events_in_wait_list,
                              const cl_event *event_wait_list, 
                              cl_event *event) {
    return CL_SUCCESS;
  }

  cl_kernel clCreateKernel(cl_program program, const char *kernel_name,
                           cl_int *errcode_ret) {
    return nullptr;
  }

  cl_int clSetKernelArg(cl_kernel kernel, cl_uint arg_index, size_t arg_size,
                        const void *arg_value) {
    return CL_SUCCESS;
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
    return CL_SUCCESS;
  }

  cl_int clEnqueueTask(cl_command_queue command_queue, cl_kernel kernel,
                       cl_uint num_events_in_wait_list,
                       const cl_event *event_wait_list, cl_event *event) {
    return CL_SUCCESS;
  }

  cl_int clGetPlatformInfo(cl_platform_id platform, cl_platform_info param_name,
                           size_t param_value_size, void *param_value,
                           size_t *param_value_size_ret) {
    return CL_SUCCESS;
  }

  cl_int clGetDeviceInfo(cl_device_id device, cl_device_info param_name,
                         size_t param_value_size, void *param_value,
                         size_t *param_value_size_ret) {
    return CL_SUCCESS;
  }

  cl_int clRetainDevice(cl_device_id device) {
    return CL_SUCCESS;
  }

  cl_int clReleaseDevice(cl_device_id device) {
    return CL_SUCCESS;
  }

  cl_int clRetainContext(cl_context context) {
    return CL_SUCCESS;
  }

  cl_int clReleaseContext(cl_context context) {
    return CL_SUCCESS;
  }

  cl_int clGetContextInfo(cl_context context, cl_context_info param_name,
                          size_t param_value_size, void *param_value,
                          size_t *param_value_size_ret) {
    return CL_SUCCESS;
  }

  cl_int clRetainCommandQueue(cl_command_queue command_queue) {
    return CL_SUCCESS;
  }

  cl_int clReleaseCommandQueue(cl_command_queue command_queue) {
    return CL_SUCCESS;
  }

  cl_int clGetCommandQueueInfo(cl_command_queue command_queue,
                               cl_command_queue_info param_name,
                               size_t param_value_size, void *param_value,
                               size_t *param_value_size_ret) {
    return CL_SUCCESS;
  }

  cl_mem clCreateSubBuffer(cl_mem buffer, cl_mem_flags flags,
                           cl_buffer_create_type buffer_create_type,
                           const void *buffer_create_info, cl_int *errcode_ret) {
    return nullptr;
  }

  cl_int clRetainMemObject(cl_mem memobj) {
    return CL_SUCCESS;
  }

  cl_int clReleaseMemObject(cl_mem memobj) {
    return CL_SUCCESS;
  }

  cl_int clGetMemObjectInfo(cl_mem memobj, cl_mem_info param_name,
                            size_t param_value_size, void *param_value,
                            size_t *param_value_size_ret) {
    return CL_SUCCESS;
  }

  cl_int clSetMemObjectDestructorCallback(cl_mem memobj,
                                          void (*pfn_notify)(cl_mem memobj, 
                                                             void *user_data), 
                                                             void *user_data) {
    return CL_SUCCESS;
  }

  cl_int clRetainKernel(cl_kernel kernel) {
    return CL_SUCCESS;
  }

  cl_int clReleaseKernel(cl_kernel kernel) {
    return CL_SUCCESS;
  }

  cl_int clGetKernelInfo(cl_kernel kernel, cl_kernel_info param_name,
                         size_t param_value_size, void *param_value,
                         size_t *param_value_size_ret) {
    return CL_SUCCESS;
  }

  cl_int clGetKernelArgInfo(cl_kernel kernel,
                            cl_uint arg_indx,
                            cl_kernel_arg_info param_name,
                            size_t param_value_size,
                            void *param_value,
                            size_t *param_value_size_ret) {
    return CL_SUCCESS;
  }

  cl_int clGetKernelWorkGroupInfo(cl_kernel kernel,
                                  cl_device_id device,
                                  cl_kernel_work_group_info param_name,
                                  size_t param_value_size,
                                  void *param_value,
                                  size_t *param_value_size_ret) {
    return CL_SUCCESS;
  }

  cl_int clWaitForEvents(cl_uint num_events,
                         const cl_event *event_list) {
    return CL_SUCCESS;
  }

  cl_int clGetEventInfo(cl_event event,
                        cl_event_info param_name,
                        size_t param_value_size,
                        void *param_value,
                        size_t *param_value_size_ret) {
    return CL_SUCCESS;
  }

  cl_event clCreateUserEvent(cl_context context,
                             cl_int *errcode_ret) {
    return nullptr;
  }

  cl_int clRetainEvent(cl_event event) {
    return CL_SUCCESS;
  }

  cl_int clReleaseEvent(cl_event event) {
    return CL_SUCCESS;
  }

  cl_int clSetUserEventStatus(cl_event event,
                              cl_int execution_status) {
    return CL_SUCCESS;
  }

  cl_int clSetEventCallback(cl_event event,
                            cl_int command_exec_callback_type,
                            void (CL_CALLBACK *pfn_notify)(cl_event, cl_int, 
                                                           void *),
                            void *user_data) {
    return CL_SUCCESS;
  }

  cl_int clGetEventProfilingInfo(cl_event event,
                                 cl_profiling_info param_name,
                                 size_t param_value_size,
                                 void *param_value,
                                 size_t *param_value_size_ret) {
    return CL_SUCCESS;
  }

  cl_int clFlush(cl_command_queue command_queue) {
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
    return CL_SUCCESS;
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
    return CL_SUCCESS;
  }

  cl_int clEnqueueFillBuffer(cl_command_queue command_queue, cl_mem buffer,
                             const void *pattern, size_t pattern_size,
                             size_t offset, size_t size,
                             cl_uint num_events_in_wait_list,
                             const cl_event *event_wait_list, cl_event *event) {
    return CL_SUCCESS;
  }

  cl_int clEnqueueCopyBuffer(cl_command_queue command_queue, cl_mem src_buffer,
                             cl_mem dst_buffer, size_t src_offset,
                             size_t dst_offset, size_t size,
                             cl_uint num_events_in_wait_list,
                             const cl_event *event_wait_list, cl_event *event) {
    return CL_SUCCESS;
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
    return CL_SUCCESS;
  }

  void *clEnqueueMapBuffer(cl_command_queue command_queue, cl_mem buffer,
                           cl_bool blocking_map, cl_map_flags map_flags,
                           size_t offset, size_t size,
                           cl_uint num_events_in_wait_list,
                           const cl_event *event_wait_list, cl_event *event,
                           cl_int *errcode_ret) {
    return nullptr;
  }

  cl_int clEnqueueUnmapMemObject(cl_command_queue command_queue, cl_mem memobj,
                                 void *mapped_ptr, 
                                 cl_uint num_events_in_wait_list,
                                 const cl_event *event_wait_list, 
                                 cl_event *event) {
    return CL_SUCCESS;
  }

  cl_int clEnqueueMarker(cl_command_queue command_queue, cl_event *event) {
    return CL_SUCCESS;
  }

  cl_int clEnqueueWaitForEvents(cl_command_queue command_queue,
                                cl_uint num_events, 
                                const cl_event *event_list) {
    return CL_SUCCESS;
  }

  cl_int clEnqueueMarkerWithWaitList(cl_command_queue command_queue,
                                     cl_uint num_events_in_wait_list,
                                     const cl_event *event_wait_list,
                                     cl_event *event) {
    return CL_SUCCESS;
  }

  cl_int clEnqueueBarrierWithWaitList(cl_command_queue command_queue,
                                      cl_uint num_events_in_wait_list,
                                      const cl_event *event_wait_list,
                                      cl_event *event) {
    return CL_SUCCESS;
  }

  cl_int clEnqueueBarrier(cl_command_queue command_queue) {
    return CL_SUCCESS;
  }

  // NO SUPPORT from below
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

  cl_program clCreateProgramWithBinary(cl_context context,
                                       cl_uint num_devices,
                                       const cl_device_id *device_list,
                                       const size_t *lengths,
                                       const unsigned char **binaries,
                                       cl_int *binary_status,
                                       cl_int *errcode_ret) {
    return nullptr;
  }

  cl_program clCreateProgramWithIL(cl_context context,
                                   const void *il,
                                   size_t length,
                                   cl_int *errcode_ret) {
    return nullptr;
  }

  cl_int clGetProgramBuildInfo(cl_program program,
                               cl_device_id device,
                               cl_program_build_info param_name,
                               size_t param_value_size,
                               void *param_value,
                               size_t *param_value_size_ret) {
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
    return nullptr;
  }

  cl_int clUnloadPlatformCompiler(cl_platform_id platform) {
    return CL_SUCCESS;
  }

  cl_int clUnloadCompiler(void) {
    return CL_SUCCESS;
  }

} // extern "C"
