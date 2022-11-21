/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __GENERIC_CONTAINER_H__
#define __GENERIC_CONTAINER_H__

#include <stddef.h>
#include <string.h>

// Generic type safe vector.

// Define a vector with the give TYPE and NAME
#define XOCL_VECTOR_TYPE(TYPE, NAME) \
typedef struct {   \
  TYPE *data;      \
  size_t num_elem; \
  size_t capacity; \
} xocl_vec_##NAME;

// Define basic operations on the vector of the given TYPE and NAME.
// Note, data allocation/free is done using the user provided 
// ALLOC/FREE routines.
#define XOCL_VECTOR_NAME(TYPE, NAME, ALLOC, FREE)                          \
                                                                           \
/* Return -1 if failed, else returns 0 */                                  \
int xocl_vec_init_##NAME(xocl_vec_##NAME *vec, size_t num_elem) {          \
  unsigned size = sizeof(TYPE) * num_elem;                                 \
  XOCL_DEBUG_ASSERT(!vec->data);                                           \
  vec->data = (TYPE *)(ALLOC)(size);                                       \
  if (!vec->data)                                                          \
    return -1;                                                             \
  vec->num_elem = 0;                                                       \
  vec->capacity = num_elem;                                                \
  return 0;                                                                \
}                                                                          \
                                                                           \
void xocl_vec_destroy_##NAME(xocl_vec_##NAME *vec) {                       \
  if (vec->data)                                                           \
    (FREE)(vec->data);                                                     \
  vec->data = NULL;                                                        \
  vec->num_elem = 0;                                                       \
  vec->capacity = 0;                                                       \
}                                                                          \
                                                                           \
/* Return -1 if failed, else returns 0 */                                  \
int xocl_vec_resize_##NAME(xocl_vec_##NAME *vec, size_t new_capacity) {    \
  if (vec->capacity == new_capacity)                                       \
    return 0;                                                              \
                                                                           \
  if (!new_capacity) {                                                     \
    xocl_vec_destroy_##NAME(vec);                                          \
    return 0;                                                              \
  }                                                                        \
                                                                           \
  size_t new_size = new_capacity * sizeof(TYPE);                           \
  size_t old_size = vec->capacity * sizeof(TYPE);                          \
  size_t copy_size = old_size < new_size ? old_size : new_size;            \
                                                                           \
  TYPE *new_data = (TYPE *)(ALLOC)(new_size);                              \
  if (!new_data)                                                           \
    return -1;                                                             \
                                                                           \
  if (vec->data) {                                                         \
    memcpy(new_data, vec->data, copy_size);                                \
    (FREE)(vec->data);                                                     \
  }                                                                        \
                                                                           \
  if (new_capacity < vec->num_elem)                                        \
    vec->num_elem = new_capacity;                                          \
                                                                           \
  vec->data = new_data;                                                    \
  vec->capacity = new_capacity;                                            \
  return 0;                                                                \
}                                                                          \
                                                                           \
/* Return -1 if failed, else returns 0 */                                  \
int xocl_vec_push_back_##NAME(xocl_vec_##NAME *vec, TYPE new_elem) {       \
  if (vec->capacity <= vec->num_elem) {                                    \
    int r = xocl_vec_resize_##NAME(vec, (vec->capacity + 1) * 2);          \
    if (r)                                                                 \
      return -1;                                                           \
  }                                                                        \
  vec->data[vec->num_elem] = new_elem;                                     \
  ++vec->num_elem;                                                         \
  return 0;                                                                \
}                                                                          \
                                                                           \
TYPE *xocl_vec_data_##NAME(xocl_vec_##NAME *vec) {                         \
  return vec->data;                                                        \
}                                                                          \
                                                                           \
TYPE xocl_vec_at_##NAME(xocl_vec_##NAME *vec, size_t i) {                  \
  XOCL_DEBUG_ASSERT(vec->num_elem > i);                                    \
  return vec->data[i];                                                     \
}                                                                          \

#endif // __GENERIC_CONTAINER_H__
