#include "artscii.h"

cl_kernel clkMult;

// Multiply an image by a scalar value
bool Mult(size_t length, cl_mem imgA, float scalar, cl_mem *product) {
	result = clSetKernelArg(clkMult, 0, sizeof(cl_mem), &imgA);
	checkResult(false)
	result = clSetKernelArg(clkMult, 1, sizeof(float), &scalar);
	checkResult(false)
	result = clSetKernelArg(clkMult, 2, sizeof(cl_mem), product);
	checkResult(false)

	const size_t globalWorkSize[] = { length };
	const size_t localWorkSize[] = { 1 };

	result = clEnqueueNDRangeKernel(queue, clkMult, 1, NULL,
		globalWorkSize, localWorkSize, 0, NULL, NULL);
	checkResult(false)
	
	result = clFinish(queue);
	checkResult(false)

	return true;
}
