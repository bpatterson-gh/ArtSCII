#include "artscii.h"

MultiConvolveArgs *multiConvolveArgs = NULL;

cl_kernel clkConvolve;

extern bool AddImg(size_t length, cl_mem imgA, cl_mem imgB, cl_mem *sum);
extern bool Mult(size_t length, cl_mem imgA, float scalar, cl_mem *product);

void freeMultiConvolveArgs() {
	if (multiConvolveArgs != NULL) {
		clReleaseMemObject(multiConvolveArgs->input);
		for (int i = 0; i < multiConvolveArgs->numKernels; i++) {
			if (multiConvolveArgs->outputs != NULL) clReleaseMemObject(multiConvolveArgs->outputs[i]);
			if (multiConvolveArgs->kernels != NULL) clReleaseMemObject(multiConvolveArgs->kernels[i]);
			if (multiConvolveArgs->knlSizes != NULL) clReleaseMemObject(multiConvolveArgs->knlSizes[i]);
		}
		if (multiConvolveArgs->outputs != NULL) free(multiConvolveArgs->outputs);
		if (multiConvolveArgs->kernels != NULL) free(multiConvolveArgs->kernels);
		if (multiConvolveArgs->knlSizes != NULL) free(multiConvolveArgs->knlSizes);
		if (multiConvolveArgs->knlMults != NULL) free(multiConvolveArgs->knlMults);
		if (multiConvolveArgs->knlInverts != NULL) free(multiConvolveArgs->knlInverts);
		free(multiConvolveArgs);
		multiConvolveArgs = NULL;
	}
}

// Loads Kernel information into multiConvolveArgs
bool loadKernels(KernelInfo *kernelBufs, size_t numKernels) {
	multiConvolveArgs->kernels = malloc(sizeof(cl_mem) * numKernels);
	multiConvolveArgs->knlSizes = malloc(sizeof(cl_mem) * numKernels);
	multiConvolveArgs->knlMults = malloc(sizeof(float) * numKernels);
	multiConvolveArgs->knlInverts = malloc(sizeof(unsigned char) * numKernels);
	multiConvolveArgs->numKernels = numKernels;

	for (int i = 0; i < numKernels; i++) {
		multiConvolveArgs->kernels[i] = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
			kernelBufs[i].bufSize * sizeof(float), kernelBufs[i].buffer, &result);
		checkResult(false)

		unsigned int knlSize[2] = { kernelBufs[i].width, kernelBufs[i].height };
		multiConvolveArgs->knlSizes[i] = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
			sizeof(unsigned int) * 2, knlSize, &result);
		checkResult(false)

		multiConvolveArgs->knlMults[i] = kernelBufs[i].mult;
		multiConvolveArgs->knlInverts[i] = kernelBufs[i].invert? 1 : 0;
	}
	return true;
}

// Sets the current Kernel to the one at index i
bool setStdKernel(size_t i) {
	result = clSetKernelArg(clkConvolve, 2, sizeof(cl_mem), &multiConvolveArgs->kernels[i]);
	checkResult(false)
	result = clSetKernelArg(clkConvolve, 3, sizeof(cl_mem), &multiConvolveArgs->knlSizes[i]);
	checkResult(false)
	result = clSetKernelArg(clkConvolve, 4, sizeof(float), &multiConvolveArgs->knlMults[i]);
	checkResult(false)
	result = clSetKernelArg(clkConvolve, 5, sizeof(unsigned char), &multiConvolveArgs->knlInverts[i]);
	checkResult(false)
	return true;
}

// Initializes multiConvolveArgs
bool setMultiConvolveArgs(ImageInfo *imgBufs, KernelInfo *kernels, size_t numKernels) {
	freeMultiConvolveArgs();
	multiConvolveArgs = calloc(1, sizeof(MultiConvolveArgs));

	multiConvolveArgs->input = clCreateBuffer(context, CL_MEM_READ_ONLY,
		imgBufs[0].width * imgBufs[0].height * 3, NULL, &result);
	checkResult(false)

	if(loadImage(imgBufs, &multiConvolveArgs->input, 0) == -1) return false;

	multiConvolveArgs->outputs = malloc(sizeof(cl_mem) * numKernels);
	for (int k = 0; k < numKernels; k++) {
		multiConvolveArgs->outputs[k] = clCreateBuffer(context, CL_MEM_READ_WRITE,
			imgBufs[0].width * imgBufs[0].height * 3, NULL, &result);
		checkResult(false)
	}

	if (!loadKernels(kernels, numKernels)) return false;

	return true;
}

// Pads an image for use with a Kernel
bool pad(cl_mem *img, cl_mem *padded, size_t imgW, size_t imgH, size_t kernelW, size_t kernelH) {
	size_t padW = imgW + kernelW - 1,
		   padH = imgH + kernelH - 1;
	unsigned char *temp = calloc(padW * padH * 3, sizeof(unsigned char));

	size_t offset = 0; // Start at the beginning of the input
	size_t padOffset = (padW * 3) + ((kernelW / 2) * 3); // Empty first row + Initial padding for second row
	for (size_t row = 0; row < imgH; row++) {
		result = clEnqueueReadBuffer(queue, *img, CL_TRUE,	offset, imgW * 3, &temp[padOffset], 0, NULL, NULL);
		checkResultAndFree(temp)
		offset += imgW * 3;
		padOffset += padW * 3;
	}
	result = clFinish(queue);
	checkResultAndFree(temp)

	if (padded != NULL) clReleaseMemObject(*padded);
	*padded = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR, padW * padH * 3, temp, &result);
	checkResultAndFree(temp)

	free(temp);
	return true;
}

// Filter an Image through a Kernel
bool convolve(cl_mem *input, ImageInfo imgBuf, KernelInfo kernelBuf, unsigned int kernelIndex,
	          const size_t globalWorkSize[], const size_t localWorkSize[], float alpha) {
	if (!setStdKernel(kernelIndex)) return false;

	cl_event event;
	cl_mem padded = NULL;
	pad(input, &padded, imgBuf.width, imgBuf.height, kernelBuf.width, kernelBuf.height);
	result = clSetKernelArg(clkConvolve, 0, sizeof(cl_mem), &padded);
	checkResult(false)

	result = clSetKernelArg(clkConvolve, 6, sizeof(float), &alpha);
	checkResult(false)

	result = clEnqueueNDRangeKernel(queue, clkConvolve, 3, NULL,
		globalWorkSize, localWorkSize, 0, NULL, &event);
	checkResult(false)

	result = clFinish(queue);
	checkResult(false)

	return true;
}

// Run all Kernels to prepare an image for ASCII matching
__declspec(dllexport) bool OCL_MultiConvolve(ImageInfo *imgBufs, KernelInfo *kernels,
		size_t numKernels) {
	if (!setMultiConvolveArgs(imgBufs, kernels, numKernels)) return false;

	const size_t globalWorkSize[] = { imgBufs[0].width + kernels[0].width - 1,
									  imgBufs[0].height + kernels[0].height - 1,
									  1 };
	const size_t localWorkSize[] = { 1, 1, 1 };

	size_t length = imgBufs[0].width * imgBufs[0].height * 3;
	cl_mem padded = NULL, sum = NULL, totalOutput = NULL;
	for (int k = 0; k < numKernels; k++) {
		result = clSetKernelArg(clkConvolve, 1, sizeof(cl_mem), &multiConvolveArgs->outputs[k]);
		checkResult(false)
		totalOutput = clCreateBuffer(context, CL_MEM_READ_WRITE, length, NULL, &result);
		for (int k2 = 0; k2 < numKernels; k2++) {
			if (k == k2) {
				if (!convolve(&multiConvolveArgs->input, imgBufs[0], kernels[k], k,
	          				  globalWorkSize, localWorkSize, 1.f / (float)numKernels)) return false;
			}
			else {
				if (!convolve(&multiConvolveArgs->input, imgBufs[0], kernels[k], k,
	          				  globalWorkSize, localWorkSize, 1.f)) return false;

				if (!convolve(&multiConvolveArgs->outputs[k], imgBufs[0], kernels[k2], k2,
	          				  globalWorkSize, localWorkSize, 1.f / (float)numKernels)) return false;
			}
			if (!AddImg(length, multiConvolveArgs->outputs[k], totalOutput, &sum)) {
				if (totalOutput != NULL) clReleaseMemObject(totalOutput);
				if (sum != NULL) clReleaseMemObject(sum);
				return false;
			}
			clReleaseMemObject(totalOutput);
			totalOutput = sum;
			sum = NULL;
		}
		clReleaseMemObject(multiConvolveArgs->outputs[k]);
		multiConvolveArgs->outputs[k] = totalOutput;
	}
	clReleaseMemObject(padded);
	return true;
}
