/*
 * Copyright (c) 2020 by Cadence Design Systems, Inc. ALL RIGHTS RESERVED. 
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of 
 * Cadence Design Systems Inc. Any rights to use, modify, and create 
 * derivative works of this file are set forth under the terms of your 
 * license agreement with Cadence Design Systems, Inc.
 */

#ifndef __XOCL_COMMON_H__
#define __XOCL_COMMON_H__

#include <CL/opencl.h>
#include <cstring>
#include <iostream>
#include <cstdlib>

template <typename T>
struct tuple3 {
  T _x, _y, _z;

  tuple3() {
    _x = _y = _z = T();
  }

  tuple3(T x, T y, T z) : _x(x), _y(y), _z(z) {
  }

  tuple3 operator+(const tuple3 &rhs) const {
    return tuple3(_x + rhs._x,
                  _y + rhs._y, 
                  _z + rhs._z);
  }

  bool operator==(const tuple3 &rhs) const {
    return _x == rhs._x && 
           _y == rhs._y && 
           _z == rhs._z;
  }
};

typedef tuple3<size_t> ID3;

template<typename T>
inline cl_int setParam(void *param_value, T value, size_t param_value_size,
                       size_t *param_value_size_ret) {
  if (param_value_size < sizeof(T) && param_value)
    return CL_INVALID_VALUE;
  if (param_value)
    *((T *)param_value) = value;
  if (param_value_size_ret)
    *param_value_size_ret = sizeof(T);
  return CL_SUCCESS;
}

inline int mxpa_strcpy(char *strDestination, size_t numberOfElements, 
                       const char *strSource) {
  if (nullptr == strSource || nullptr == strDestination || 
      numberOfElements == 0)
    return -1;
  size_t len = strlen(strSource);
  if (len >= numberOfElements)
    memcpy(strDestination, strSource, numberOfElements);
  else {
    memcpy(strDestination, strSource, len);
    strDestination[len] = '\0';
  }
  return 0;
}

inline cl_int setParamStr(void *param_value, const char *string,
                          size_t param_value_size,
                          size_t *param_value_size_ret) {
  size_t len = strlen(string) + 1;
  if (param_value_size < len && param_value)
    return CL_INVALID_VALUE;
  if (param_value)
    mxpa_strcpy((char *)param_value, param_value_size, string);
  if (param_value_size_ret)
    *param_value_size_ret = len;
  return CL_SUCCESS;
}

__attribute__((noreturn)) inline
void mxpa_unreachable_internal(const char *msg, const char *file, unsigned line)
{
  std::cerr << msg << std::endl;
  std::cerr << "Unreachable executed at " << file << ": " << line << std::endl;
  abort();
}

#define mxpa_unreachable(msg) \
  mxpa_unreachable_internal(msg, __FILE__, __LINE__)
#endif

inline bool IsPowerOf2(int Num) { return (Num & (Num - 1)) == 0; }

