// Data size = 0
/**
 * atax.cl: This file is part of the PolyBench/GPU 1.0 test suite.
 *
 *
 * Contact: Scott Grauer-Gray <sgrauerg@gmail.com>
 * Will Killian <killian@udel.edu>
 * Louis-Noel Pouchet <pouchet@cse.ohio-state.edu>
 * Web address: http://www.cse.ohio-state.edu/~pouchet/software/polybench/GPU
 */

#if defined(cl_khr_fp64)  // Khronos extension available?
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#elif defined(cl_amd_fp64)  // AMD extension available?
#pragma OPENCL EXTENSION cl_amd_fp64 : enable
#endif

typedef float DATA_TYPE;

__kernel void atax_kernel1(__global float *A, __global float *x, __global float *tmp, int nx, int ny) {
    
	int i = get_global_id(0);

	if (i < nx)
	{
		int j;
		for(j=0; j < ny; j++)
		{
			tmp[i] += A[i * ny + j] * x[j];
		}
	}
}

__kernel void atax_kernel2(__global float *A, __global float *y, __global float *tmp, int nx, int ny) {
    
	int j = get_global_id(0);

	if (j < ny)
	{
		int i;
		for(i=0; i < nx; i++)
		{
			y[j] += A[i * ny + j] * tmp[i];
		}
	}
}

