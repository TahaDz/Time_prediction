/**
 * syr2k.c: This file is part of the PolyBench/GPU 1.0 test suite.
 *
 *
 * Contact: Scott Grauer-Gray <sgrauerg@gmail.com>
 * Will Killian <killian@udel.edu>
 * Louis-Noel Pouchet <pouchet@cse.ohio-state.edu>
 * Web address: http://www.cse.ohio-state.edu/~pouchet/software/polybench/GPU
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif



#define POLYBENCH_TIME 1

//select the OpenCL device to use (can be GPU, CPU, or Accelerator such as Intel Xeon Phi)
#define OPENCL_DEVICE_SELECTION CL_DEVICE_TYPE_GPU

#include "syr2k.h"
#include "Polybench/polybench.h"
#include "Polybench/polybenchUtilFuncts.h"

//define the error threshold for the results "not matching"
#define PERCENT_DIFF_ERROR_THRESHOLD 0.05

#define MAX_SOURCE_SIZE (0x100000)

#if defined(cl_khr_fp64)  // Khronos extension available?
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#elif defined(cl_amd_fp64)  // AMD extension available?
#pragma OPENCL EXTENSION cl_amd_fp64 : enable
#endif

char str_temp[1024];
int NI = 0;
int NJ = 0;

DATA_TYPE acc;

DATA_TYPE ALPHA = 1;
DATA_TYPE BETA = 1;

cl_platform_id platform_id;
cl_device_id device_id;   
cl_uint num_devices;
cl_uint num_platforms;
cl_int errcode;
cl_context clGPUContext;
cl_kernel clKernel1;
cl_kernel clKernel2;
cl_kernel clKernel3;
cl_command_queue clCommandQue;
cl_program clProgram;

cl_mem a_mem_obj;
cl_mem b_mem_obj;
cl_mem c_mem_obj;

FILE *fp;
char *source_str;
size_t source_size;



void read_cl_file(char* path)
{
	// Load the kernel source code into the array source_str
	char *pwd = strcat(path,"/Cfiles/syr2k.cl");
	fp = fopen(pwd, "r");
	if (!fp) {
		fprintf(stderr, "Failed to load kernel.\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread( source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose( fp );
}


void init_arrays(int ni, int nj,
		DATA_TYPE *alpha,
		DATA_TYPE *beta,
		DATA_TYPE POLYBENCH_2D(A,NI,NJ,ni,nj),
		DATA_TYPE POLYBENCH_2D(B,NI,NJ,ni,nj),
		DATA_TYPE POLYBENCH_2D(C,NI,NI,ni,ni))
{
	int i, j;

	*alpha = 32412;
	*beta = 2123;

	for (i = 0; i < ni; i++)
	{
		for (j = 0; j < nj; j++) 
		{
			A[i][j] = ((DATA_TYPE) i*j) / ni;
			B[i][j] = ((DATA_TYPE) i*j) / ni;
		}
	}

	for (i = 0; i < ni; i++)
	{
		for (j = 0; j < ni; j++)
		{
			C[i][j] = ((DATA_TYPE) i*j) / ni;
		}
	}
}


void cl_initialization(int plat, int dev)
{	

	cl_uint dev_cnt = 0;
  	clGetPlatformIDs(0, 0, &dev_cnt);
	
   	cl_platform_id platform_ids[100];
  	clGetPlatformIDs(dev_cnt, platform_ids, NULL);

   // Connect to a compute device

   	int num_devices = 0;
   	errcode = clGetDeviceIDs(platform_ids[plat], CL_DEVICE_TYPE_ALL, 1, &device_id, &num_devices);
   	if (errcode != CL_SUCCESS)
   	{
       	printf("Error: Failed to create a device group!\n");
       	return EXIT_FAILURE;
  	}
   
   	cl_device_id * device_list = NULL;
   	device_list = (cl_device_id *) malloc(sizeof(cl_device_id)*num_devices);
   	cl_int clStatus = clGetDeviceIDs(platform_ids[plat], CL_DEVICE_TYPE_ALL,num_devices,device_list,NULL);
   
   
   	char query[1024] ;
   	cl_int clError = clGetDeviceInfo(device_list[dev], CL_DEVICE_NAME, sizeof(query), &query, NULL);
   	printf(" The program syrk2 will execute on : %s \n", query);
  
   	// Create a compute context 
   	clGPUContext = clCreateContext(NULL, num_devices, device_list, NULL, NULL, &errcode);
  
   	if (!clGPUContext)
   	{
       	printf("Error: Failed to create a compute context!\n");
       	return EXIT_FAILURE;
   	}

   // Create a command commands

   	clCommandQue = clCreateCommandQueue(clGPUContext, device_list[dev], CL_QUEUE_PROFILING_ENABLE, &errcode);
   	if (!clCommandQue)
   	{
       	printf("Error: Failed to create a command queue!\n");
       	return EXIT_FAILURE;
   	}
}


void cl_mem_init(DATA_TYPE POLYBENCH_2D(A,NI,NJ,ni,nj), DATA_TYPE POLYBENCH_2D(B,NI,NJ,ni,nj), DATA_TYPE POLYBENCH_2D(C,NI,NJ,ni,nj))
{
	a_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, NI*NJ*sizeof(DATA_TYPE), NULL, &errcode);
	b_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, NI*NJ*sizeof(DATA_TYPE), NULL, &errcode);
	c_mem_obj = clCreateBuffer(clGPUContext, CL_MEM_READ_WRITE, NI*NI*sizeof(DATA_TYPE), NULL, &errcode);
	
	if(errcode != CL_SUCCESS) printf("Error in creating buffers\n");

	errcode = clEnqueueWriteBuffer(clCommandQue, a_mem_obj, CL_TRUE, 0, NI*NJ*sizeof(DATA_TYPE), A, 0, NULL, NULL);
	errcode = clEnqueueWriteBuffer(clCommandQue, b_mem_obj, CL_TRUE, 0, NI*NJ*sizeof(DATA_TYPE), B, 0, NULL, NULL);
	errcode = clEnqueueWriteBuffer(clCommandQue, c_mem_obj, CL_TRUE, 0, NI*NI*sizeof(DATA_TYPE), C, 0, NULL, NULL);
	if(errcode != CL_SUCCESS)printf("Error in writing buffers\n");
}


void cl_load_prog()
{
	// Create a program from the kernel source
	clProgram = clCreateProgramWithSource(clGPUContext, 1, (const char **)&source_str, (const size_t *)&source_size, &errcode);

	if(errcode != CL_SUCCESS) printf("Error in creating program\n");

	// Build the program
	errcode = clBuildProgram(clProgram, 1, &device_id, NULL, NULL, NULL);
	if(errcode != CL_SUCCESS) printf("Error in building program\n");
		
	// Create the OpenCL kernel
	clKernel1 = clCreateKernel(clProgram, "syr2k_kernel", &errcode);
	if(errcode != CL_SUCCESS) printf("Error in creating kernel1\n");
	clFinish(clCommandQue);
}


void cl_launch_kernel(int ni, int nj, DATA_TYPE alpha, DATA_TYPE beta, char* filename)
{
	size_t localWorkSize[2], globalWorkSize[2];
	localWorkSize[0] = DIM_LOCAL_WORK_GROUP_X;
	localWorkSize[1] = DIM_LOCAL_WORK_GROUP_Y;
	globalWorkSize[0] = (size_t)ceil(((float)NI) / ((float)DIM_LOCAL_WORK_GROUP_X)) * DIM_LOCAL_WORK_GROUP_X;
	globalWorkSize[1] = (size_t)ceil(((float)NJ) / ((float)DIM_LOCAL_WORK_GROUP_Y)) * DIM_LOCAL_WORK_GROUP_Y;
	
	/* Start timer. */
  	polybench_start_instruments;
  	

	// Set the arguments of the kernel
	errcode =  clSetKernelArg(clKernel1, 0, sizeof(cl_mem), (void *)&a_mem_obj);
	errcode =  clSetKernelArg(clKernel1, 1, sizeof(cl_mem), (void *)&b_mem_obj);
	errcode |= clSetKernelArg(clKernel1, 2, sizeof(cl_mem), (void *)&c_mem_obj);
	errcode |= clSetKernelArg(clKernel1, 3, sizeof(DATA_TYPE), (void *)&alpha);
	errcode |= clSetKernelArg(clKernel1, 4, sizeof(DATA_TYPE), (void *)&beta);
	errcode |= clSetKernelArg(clKernel1, 5, sizeof(int), (void *)&ni);
	errcode |= clSetKernelArg(clKernel1, 6, sizeof(int), (void *)&nj);
	
	if(errcode != CL_SUCCESS) printf("Error in setting arguments1\n");

	// Execute the OpenCL kernel
	errcode = clEnqueueNDRangeKernel(clCommandQue, clKernel1, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	if(errcode != CL_SUCCESS) printf("Error in launching kernel1\n");
	clFinish(clCommandQue);
	
	/* Stop and print timer. */
	printf("Execution Time in seconds:");
  	polybench_stop_instruments;
 	double executionTime = polybench_print_instruments;
 	printf ("%0.6lf\n",executionTime);
 	FILE *myfile ; 
	myfile = fopen ("b2.txt","a");
   	fprintf(myfile,"%s","syr2k");
   	fprintf(myfile,"|");
   	fprintf(myfile,"%0.6lf",executionTime);
   	fprintf(myfile,"|");
   	fprintf(myfile,"%d",NI*NJ+NI*NJ+NI*NJ);
   	fprintf(myfile,"\n");
  	fclose (myfile); 
}

void cl_clean_up()
{
	// Clean up
	errcode = clFlush(clCommandQue);
	errcode = clFinish(clCommandQue);
	errcode = clReleaseKernel(clKernel1);
	errcode = clReleaseProgram(clProgram);
	errcode = clReleaseMemObject(a_mem_obj);
	errcode = clReleaseMemObject(c_mem_obj);
	errcode = clReleaseCommandQueue(clCommandQue);
	errcode = clReleaseContext(clGPUContext);
	if(errcode != CL_SUCCESS) printf("Error in cleanup\n");
}

/*
void syr2kCpu(int ni, int nj,
		  DATA_TYPE alpha,
		  DATA_TYPE beta,
		  DATA_TYPE POLYBENCH_2D(A,NI,NJ,ni,nj),
		  DATA_TYPE POLYBENCH_2D(B,NI,NJ,ni,nj),
		  DATA_TYPE POLYBENCH_2D(C,NI,NI,ni,ni))
{
	int i, j, k;

	//    C := alpha*A*B' + alpha*B*A' + beta*C 
	for (i = 0; i < _PB_NI; i++)
	{
		for (j = 0; j < _PB_NI; j++)
		{
			C[i][j] *= beta;
		}
	}
	
	for (i = 0; i < _PB_NI; i++)
	{
		for (j = 0; j < _PB_NI; j++)
		{
			for (k = 0; k < _PB_NJ; k++)
			{
				C[i][j] += alpha * A[i][k] * B[j][k];
				C[i][j] += alpha * B[i][k] * A[j][k];
			}
		}
	}
}
*/

/* DCE code. Must scan the entire live-out data.
   Can be used also to check the correctness of the output. */
static
void print_array(int ni, DATA_TYPE POLYBENCH_2D(C,NI,NI,ni,ni))
{
  int i, j;

  for (i = 0; i < ni; i++)
    for (j = 0; j < ni; j++) {
	fprintf (stderr, DATA_PRINTF_MODIFIER, C[i][j]);
	if ((i * ni + j) % 20 == 0) fprintf (stderr, "\n");
    }
  fprintf (stderr, "\n");
}

void get_pwd(char* path, int n){

  FILE *fp;
  

  /* Open the command for reading. */
  fp = popen("pwd", "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit(1);
  }
   

    int i = 0;
    char c = fgetc(fp);
    while ( c != EOF)
    {
	
	if (c == '\n') break;
        path[i++] = (char) c;
        c = fgetc(fp);
    }

    path[i] = '\0';  
    pclose(fp);
  
}

void get_filename(char* filename){

		for (int i = 0; i<strlen(filename);i++){ 
		if (filename[i] != '/') i++;
		else{
			int j = 0;
			i++;
			for (; j < strlen(filename)&&i<strlen(filename);j++) {
				filename[j] = filename[i];
				i++;
			}
			filename[j] = '\0';
			break;
		}	
	}

}


int main(int argc, char *argv[])
{
	char filename[100];
	
	strcpy(filename,argv[0]);
	// argv[0] = Cfiles/2mm => remove Cfile/ => 2mm
	get_filename(filename);

	printf("%s\n",filename);
	char path[1000]; // current folder path
   	get_pwd(path,sizeof(path));
   	int plat = atoi(argv[1]);// which platform to run on
        int dev = atoi(argv[2]);// which device to run on
        
	/* Retrieve problem size. */
	NI = atoi(argv[3]);
    	NJ = atoi(argv[4]);
	int ni = NI;
	int nj = NJ;

	/* Variable declaration/allocation. */
	DATA_TYPE alpha;
	DATA_TYPE beta;
	POLYBENCH_2D_ARRAY_DECL(A,DATA_TYPE,NI,NJ,ni,nj); 
	POLYBENCH_2D_ARRAY_DECL(B,DATA_TYPE,NI,NJ,ni,nj);
	POLYBENCH_2D_ARRAY_DECL(C,DATA_TYPE,NI,NI,ni,ni);
	POLYBENCH_2D_ARRAY_DECL(C_outputFromGpu,DATA_TYPE,NI,NI,ni,ni);

	init_arrays(ni, nj, &alpha, &beta, POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(C));
	
	read_cl_file(path);
	cl_initialization(plat,dev);
	
	cl_mem_init(POLYBENCH_ARRAY(A), POLYBENCH_ARRAY(B), POLYBENCH_ARRAY(C));
	cl_load_prog();

	cl_launch_kernel(ni, nj, alpha, beta,filename);

	errcode = clEnqueueReadBuffer(clCommandQue, c_mem_obj, CL_TRUE, 0, NI*NJ*sizeof(DATA_TYPE), POLYBENCH_ARRAY(C_outputFromGpu), 0, NULL, NULL);
	if(errcode != CL_SUCCESS) printf("Error in reading GPU mem\n");

	cl_clean_up();

	POLYBENCH_FREE_ARRAY(A);
	POLYBENCH_FREE_ARRAY(B);
	POLYBENCH_FREE_ARRAY(C);
	POLYBENCH_FREE_ARRAY(C_outputFromGpu);

	return 0;
}

#include "Polybench/polybench.c"
