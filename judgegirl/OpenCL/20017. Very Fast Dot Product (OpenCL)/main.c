#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <CL/cl.h>
#include "utils.h"

#define MAXGPU 1
#define MAXK 1024
#define MAXN 16777216

int main(int argc, char *argv[]) {
    cl_int status;
    cl_platform_id platform_id;
    cl_uint platform_id_got;
    status = clGetPlatformIDs(1, &platform_id, &platform_id_got);
    assert(status == CL_SUCCESS && platform_id_got == 1);

    cl_device_id GPU[MAXGPU];
    cl_uint GPU_id_got;
    status = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, MAXGPU, GPU, &GPU_id_got);
    assert(status == CL_SUCCESS);

    /* getcontext */
    cl_context context = clCreateContext(NULL, 1, GPU, NULL, NULL, &status);
    assert(status == CL_SUCCESS);

    /* commandqueue */
    cl_command_queue commandQueue = clCreateCommandQueue(context, GPU[0], 0, &status);
    assert(status == CL_SUCCESS);

    /* kernelsource */
    FILE *kernelfp = fopen("vecdot.cl", "r");
    assert(kernelfp != NULL);
    char kernelBuffer[MAXK];
    const char *constKernelSource = kernelBuffer;
    size_t kernelLength = fread(kernelBuffer, 1, MAXK, kernelfp);
    cl_program program = clCreateProgramWithSource(context, 1, &constKernelSource, &kernelLength, &status);
    assert(status == CL_SUCCESS);

    /* buildprogram */
    status = clBuildProgram(program, 1, GPU, NULL, NULL, NULL);
    assert(status == CL_SUCCESS);

    /* createkernel */
    cl_kernel kernel = clCreateKernel(program, "vec_dot", &status);
    assert(status == CL_SUCCESS);

    /* vector */
    cl_uint* A = (cl_uint*)malloc(MAXN * sizeof(cl_uint));
    assert(A != NULL);

    /* createbuffer */
    cl_mem bufferA = clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, MAXN * sizeof(cl_uint), A, &status);
    assert(status == CL_SUCCESS);

    int N, local_work_size = 1024;
    uint32_t key1, key2;

    while (scanf("%d %" PRIu32 " %" PRIu32, &N, &key1, &key2) == 3) {
        status = clSetKernelArg(kernel, 0, sizeof(uint32_t), (void *)&key1);
        assert(status == CL_SUCCESS);
        status = clSetKernelArg(kernel, 1, sizeof(uint32_t), (void *)&key2);
        assert(status == CL_SUCCESS);
        status = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&bufferA);
        assert(status == CL_SUCCESS);
        status = clSetKernelArg(kernel, 3, sizeof(int), (void *)&local_work_size);
        assert(status == CL_SUCCESS);
        status = clSetKernelArg(kernel, 4, sizeof(int), (void *)&N);
        assert(status == CL_SUCCESS);

        size_t expanded_N = (N-1) / local_work_size + 1;
        expanded_N = (expanded_N % 256 == 0) ? expanded_N : ((expanded_N / 256) + 1) * 256;
        size_t globalThreads[] = {(size_t)expanded_N};
        size_t localThreads[] = {256};
        status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, globalThreads, localThreads, 0, NULL, NULL);
        assert(status == CL_SUCCESS);

        clEnqueueReadBuffer(commandQueue, bufferA, CL_TRUE, 0, expanded_N * sizeof(cl_uint), A, 0, NULL, NULL);

        uint32_t sum = 0;
        for (int i = 0; i < expanded_N; i++)
            sum += A[i];

        printf("%" PRIu32 "\n", sum);
    }

    /* free */
    free(A);
    clReleaseContext(context);
    clReleaseCommandQueue(commandQueue);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseMemObject(bufferA);

    return 0;
}
