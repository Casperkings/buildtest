#include <stdio.h>
#include <stdlib.h>
#include <uuid/uuid.h>
#include "xrp_api.h"
#include "xdyn_lib.h"

// Loads contents of filename into buffer lib and also returns the size of 
// file lib_size
//
// Returns 0 if successful, else returns -1
static int load_file(const char *filename, char **lib, size_t *lib_size) {
  FILE *fp;
  long size;
  int r = 0;

  fp = fopen(filename, "rb");
  if (fp == NULL) {
    printf("Host: load_file: Could not open %s\n", filename);
    return -1;
  }

  r = fseek(fp , 0, SEEK_END);
  if (r == -1) {
    printf("Host: load_file: fseek of file %s failed\n", filename);
    return -1;
  }

  size = ftell(fp);
  if (size == -1) {
    printf("Host: load_file: ftell of file %s failed\n", filename);
    return -1;
  }

  rewind(fp);

  void *buf = malloc(size);
  if (buf == NULL) {
    printf("Host: load_file: Failed to malloc %ld bytes\n", size);
    return -1;
  }

  r = fread(buf, 1, size, fp);
  if (r != size) {
    printf("Host: load_file: Loading file failed\n");
    return -1;
  }

  *lib = buf;
  *lib_size = size;

  fclose(fp);
  return 0;
}

int main()
{
  enum xrp_status status;
  xdyn_lib_status_t xdlib_status;
  struct xrp_queue *queue = NULL;
  unsigned char lib_uuid[16];
  char *lib = NULL;
  size_t lib_size = 0;

  struct xrp_device *device = xrp_open_device(0, &status);

  if (status != XRP_STATUS_SUCCESS) {
    printf("Host: Failed to open device\n");
    return -1;
  }

  // Generate unique UUID for the library
  uuid_generate(lib_uuid);

  int r = load_file("device.lib.pkg", &lib, &lib_size);
  if (r == -1) {
    printf("Host: Error loading library\n");
    return -1;
  }

  // Load library
  xdlib_status = xdyn_lib_load(device, lib, lib_size, lib_uuid);
  if (xdlib_status != XDYN_LIB_OK) {
    printf("Host: Error loading library, error = %d\n", xdlib_status);
    return -1;
  }

  // Create queue to push commands to the library
  queue = xrp_create_ns_queue(device, lib_uuid, &status);
  if (status != XRP_STATUS_SUCCESS) {
    printf("Failed to create queue\n");
    return -1;
  }

  // Enqueue a dummy command
  int cmd = 0xdeadbeef;
  int result;
  xrp_run_command_sync(queue, &cmd, sizeof(cmd), &result, 
                       sizeof(result), NULL, &status);
  if (status != XRP_STATUS_SUCCESS) {
    printf("Failed to execute command\n");
    return -1;
  }

  printf("Got command result %x\n", result);

  xrp_release_queue(queue);

  // Unload library
  xdlib_status = xdyn_lib_unload(device, lib_uuid);
  if (xdlib_status != XDYN_LIB_OK) {
    printf("Host: Error unloading library, error = %d\n", xdlib_status);
    return -1;
  }

  xrp_release_device(device);

  xrp_exit();

  return 0;
}
