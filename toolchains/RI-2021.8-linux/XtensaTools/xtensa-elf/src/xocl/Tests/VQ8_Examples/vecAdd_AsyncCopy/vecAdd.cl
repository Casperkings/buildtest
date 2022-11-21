#define LOCAL_SIZE 8192

__kernel void vecAdd(__global const int * restrict in1,
                     __global const int * restrict in2,
                     __global       int * restrict out)
{
  __local int local_in1[LOCAL_SIZE];
  __local int local_in2[LOCAL_SIZE];
  __local int local_out[LOCAL_SIZE];

  int grp_id = get_group_id(0);
  event_t evt1 = async_work_group_copy(local_in1,
                                       in1 + grp_id * LOCAL_SIZE,
                                       LOCAL_SIZE, 0);
  event_t evt2 = async_work_group_copy(local_in2,
                                       in2 + grp_id * LOCAL_SIZE,
                                       LOCAL_SIZE, 0);
  wait_group_events(1, &evt1);
  wait_group_events(1, &evt2);

  int lid = 32 * get_local_id(0);
  int32 vecA = *(__local int32 *)&local_in1[lid];
  int32 vecB = *(__local int32 *)&local_in2[lid];
  int32 vecC = vecA + vecB;
  *(__local int32 *)&local_out[lid] = vecC;

  event_t evt3 = async_work_group_copy(out + grp_id * LOCAL_SIZE,
                                       local_out, LOCAL_SIZE, 0);
  wait_group_events(1, &evt3);
}
