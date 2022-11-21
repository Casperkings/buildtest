__kernel void vecAdd(__global const int *in1,
                     __global int *out) {
	int gid = get_global_id(0) * 16;
	int16 vecA = *(__global const int16 *)&in1[gid];
	int16 vecB = *(__global int16 *)&out[gid];
	vecB = vecA + vecB;
	*(__global int16 *)&out[gid]= vecB;
}
 
__kernel void vecMul0(__global const int *in1,
                     __global const int *in2,
                     __global int *out) {
	int gid = get_global_id(0) * 16;
	int16 vecA = *(__global const int16 *)&in1[gid];
	int16 vecB = *(__global const int16 *)&in2[gid];
	int16 vecC = vecA * vecB;
	*(__global int16 *)&out[gid]= vecC;
} 
__kernel void vecMul1(__global const int *in1,
                     __global const int *in2,
                     __global int *out) {
	int gid = get_global_id(0) * 16;
	int16 vecA = *(__global const int16 *)&in1[gid];
	int16 vecB = *(__global const int16 *)&in2[gid];
	int16 vecC = vecA * vecB;
	*(__global int16 *)&out[gid]= vecC;
} 
__kernel void vecMul2(__global const int *in1,
                     __global const int *in2,
                     __global int *out) {
	int gid = get_global_id(0) * 16;
	int16 vecA = *(__global const int16 *)&in1[gid];
	int16 vecB = *(__global const int16 *)&in2[gid];
	int16 vecC = vecA * vecB;
	*(__global int16 *)&out[gid]= vecC;
}
__kernel void vecMul3(__global const int *in1,
                     __global const int *in2,
                     __global int *out) {
	int gid = get_global_id(0) * 16;
	int16 vecA = *(__global const int16 *)&in1[gid];
	int16 vecB = *(__global const int16 *)&in2[gid];
	int16 vecC = vecA * vecB;
	*(__global int16 *)&out[gid]= vecC;
}

