#include "artscii.h"

cl_kernel clkAddImg;

extern void nocl_kAddImg(const unsigned char *a, const unsigned char *b,
	unsigned char *sum, const size_t global_id);


// Add two images together
bool AddImg(size_t length, cl_mem imgA, cl_mem imgB, cl_mem *sum) {
	result = clSetKernelArg(clkAddImg, 0, sizeof(cl_mem), &imgA);
	CHECK_RESULT(false)
	result = clSetKernelArg(clkAddImg, 1, sizeof(cl_mem), &imgB);
	CHECK_RESULT(false)

	if (*sum != NULL) clReleaseMemObject(*sum);
	*sum = clCreateBuffer(context, CL_MEM_READ_WRITE, length, NULL, &result);
	CHECK_RESULT(false)
	result = clSetKernelArg(clkAddImg, 2, sizeof(cl_mem), sum);
	CHECK_RESULT(false)

	const size_t globalWorkSize[] = { length / 3 };
	const size_t localWorkSize[] = { 1 };

	cl_event event;
	result = clEnqueueNDRangeKernel(queue, clkAddImg, 1, NULL,
		globalWorkSize, localWorkSize, 0, NULL, &event);
	CHECK_RESULT(false)
	
	result = clFinish(queue);
	CHECK_RESULT(false)

	return true;
}

void nocl_AddImg(const size_t globalWorkSize, const unsigned char *imgA,
			const unsigned char *imgB, unsigned char *sum) {
	for (size_t i = 0; i < globalWorkSize; i++) {
			nocl_kAddImg(imgA, imgB, sum, i);
	}
}