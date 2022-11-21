#define LOCAL_SIZE 8192
__kernel void vecAdd(__global const int * restrict in1, 
                     __global const int * restrict in2, 
                     __global int * restrict out) 
{ 
  int gid = get_global_id(0) * 16;
  int16 vecA = *(__global const int16 *)&in1[gid];
  int16 vecB = *(__global const int16 *)&in2[gid];
  int16 vecC = vecA + vecB;
  *(__global int16 *)&out[gid]= vecC;
}
