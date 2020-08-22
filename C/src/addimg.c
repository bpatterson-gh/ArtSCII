#include "artscii.h"

cl_kernel clkAddImg;

// Add two images together
bool AddImg(size_t length, cl_mem imgA, cl_mem imgB, cl_mem *sum) {
	result = clSetKernelArg(clkAddImg, 0, sizeof(cl_mem), &imgA);
	checkResult(false)
	result = clSetKernelArg(clkAddImg, 1, sizeof(cl_mem), &imgB);
	checkResult(false)

	if (*sum != NULL) clReleaseMemObject(*sum);
	*sum = clCreateBuffer(context, CL_MEM_READ_WRITE, length, NULL, &result);
	checkResult(false)
	result = clSetKernelArg(clkAddImg, 2, sizeof(cl_mem), sum);
	checkResult(false)

	const size_t globalWorkSize[] = { length / 3 };
	const size_t localWorkSize[] = { 1 };

	cl_event event;
	result = clEnqueueNDRangeKernel(queue, clkAddImg, 1, NULL,
		globalWorkSize, localWorkSize, 0, NULL, &event);
	checkResult(false)
	
	result = clFinish(queue);
	checkResult(false)

	return true;
}
