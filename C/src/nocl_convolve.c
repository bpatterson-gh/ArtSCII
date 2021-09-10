#include "artscii.h"

NOCL_MultiConvolveArgs *nocl_multiConvolveArgs = NULL;

extern void nocl_AddImg(const size_t globalWorkSize, const unsigned char *imgA,
			const unsigned char *imgB, unsigned char *sum);
extern void nocl_kConvolve(const unsigned char *img, unsigned char *output, const float *k,
		const unsigned int *knlSize, float knlMult, unsigned char knlInvert, float alpha,
		const size_t *global_id, const size_t *global_size);
extern int nocl_loadImage(ImageInfo *imgBufs, unsigned char *charBuf, size_t offset);

void nocl_freeMultiConvolveArgs() {
	if (nocl_multiConvolveArgs != NULL) {
		if (nocl_multiConvolveArgs->input != NULL) free(nocl_multiConvolveArgs->input);
		if (nocl_multiConvolveArgs->outputs != NULL) {
			for (size_t i = 0; i < nocl_multiConvolveArgs->numKernels; i++) {
				if (nocl_multiConvolveArgs->outputs[i] != NULL) free(nocl_multiConvolveArgs->outputs[i]);
			}
			free(nocl_multiConvolveArgs->outputs);
		}
		if (nocl_multiConvolveArgs->kernels != NULL) {
			for (size_t i = 0; i < nocl_multiConvolveArgs->numKernels; i++) {
				if (nocl_multiConvolveArgs->kernels[i] != NULL) free(nocl_multiConvolveArgs->kernels[i]);
			}
			free(nocl_multiConvolveArgs->kernels);
		}
		if (nocl_multiConvolveArgs->knlSizes != NULL) free(nocl_multiConvolveArgs->knlSizes);
		if (nocl_multiConvolveArgs->knlMults != NULL) free(nocl_multiConvolveArgs->knlMults);
		if (nocl_multiConvolveArgs->knlInverts != NULL) free(nocl_multiConvolveArgs->knlInverts);
		free(nocl_multiConvolveArgs);
		nocl_multiConvolveArgs = NULL;
	}
}

// Loads Kernel information into nocl_multiConvolveArgs
bool nocl_loadKernels(KernelInfo *kernelBufs, size_t numKernels) {
	nocl_multiConvolveArgs->kernels = malloc(sizeof(float*) * numKernels);
	nocl_multiConvolveArgs->knlSizes = malloc(sizeof(size_t) * numKernels);
	nocl_multiConvolveArgs->knlMults = malloc(sizeof(float) * numKernels);
	nocl_multiConvolveArgs->knlInverts = malloc(sizeof(unsigned char) * numKernels);
	nocl_multiConvolveArgs->numKernels = numKernels;

	for (int i = 0; i < numKernels; i++) {
		const unsigned int knlSize[2] = { kernelBufs[i].width, kernelBufs[i].height };

		nocl_multiConvolveArgs->kernels[i] = malloc(sizeof(float) * knlSize[0] * knlSize[1]);
		for (size_t j = 0; j < knlSize[0] * knlSize[1]; j++) {
			nocl_multiConvolveArgs->kernels[i][j] = kernelBufs[i].buffer[j];
		}
		nocl_multiConvolveArgs->knlMults[i] = kernelBufs[i].mult;
		nocl_multiConvolveArgs->knlInverts[i] = kernelBufs[i].invert? 1 : 0;
	}
	return true;
}

// Initializes nocl_multiConvolveArgs
bool nocl_setMultiConvolveArgs(ImageInfo *imgBufs, KernelInfo *kernels, const size_t numKernels) {
	nocl_freeMultiConvolveArgs();
	nocl_multiConvolveArgs = calloc(1, sizeof(NOCL_MultiConvolveArgs));

	const size_t length = imgBufs[0].width * imgBufs[0].height * 3;

	nocl_multiConvolveArgs->input = calloc(length, sizeof(unsigned char));
	if(nocl_loadImage(imgBufs, nocl_multiConvolveArgs->input, 0) == -1) return false;

	nocl_multiConvolveArgs->outputs = malloc(sizeof(unsigned char *) * numKernels);
	for (size_t i = 0; i < numKernels; i++) {
		nocl_multiConvolveArgs->outputs[i] = calloc(length, sizeof(unsigned char));
	}

	if (!nocl_loadKernels(kernels, numKernels)) return false;

	return true;
}

// Pads an image for use with a Kernel
void nocl_pad(unsigned char *img, unsigned char **padded, const size_t imgW, const size_t imgH, const size_t kernelW, const size_t kernelH) {
	const size_t padW = imgW + kernelW - 1,
				 padH = imgH + kernelH - 1;

	*padded = calloc(padW * padH * 3, sizeof(unsigned char));
	
	size_t offset = 0; // Start at the beginning of the input
	size_t padOffset = (padW * 3) + ((kernelW / 2) * 3); // Empty first row + Initial padding for second row
	for (size_t row = 0; row < imgH; row++) {
		for (size_t col = 0; col < imgW * 3; col++) {
			(*padded)[padOffset + col] = img[offset + col];
		}
		offset += imgW * 3;
		padOffset += padW * 3;
	}
}

// Filter an Image through a Kernel
bool nocl_convolve(unsigned char *input, ImageInfo imgBuf, KernelInfo kernelBuf, const size_t kernelIndex,
	    const size_t *globalWorkSize, const float alpha) {

	unsigned char **padded = malloc(sizeof(unsigned char *)); // Contents allocated in nocl_pad()
	nocl_pad(input, padded, imgBuf.width, imgBuf.height, kernelBuf.width, kernelBuf.height);

	const unsigned int knlSize[2] = { kernelBuf.width, kernelBuf.height };
	size_t globalID[2] = {};

	for (size_t i = 0; i < globalWorkSize[0]; i++) {
		globalID[0] = i;
		for (size_t j = 0; j < globalWorkSize[1]; j++) {
			globalID[1] = j;
			nocl_kConvolve(*padded, nocl_multiConvolveArgs->outputs[nocl_multiConvolveArgs->outputIndex],
						   nocl_multiConvolveArgs->kernels[kernelIndex], knlSize, kernelBuf.mult, kernelBuf.invert,
						   alpha, globalID, globalWorkSize);
		}
	}
	
	free(*padded);
	free(padded);

	return true;
}

// Run all Kernels to prepare an image for ASCII matching
EXPORT bool NOCL_MultiConvolve(ImageInfo *imgBufs, KernelInfo *kernels,
		const size_t numKernels) {
	if (!nocl_setMultiConvolveArgs(imgBufs, kernels, numKernels)) return false;

	const size_t globalWorkSize[] = { imgBufs[0].width + kernels[0].width - 1,
									  imgBufs[0].height + kernels[0].height - 1 };
	const size_t length = imgBufs[0].width * imgBufs[0].height * 3;

	unsigned char *sum = NULL, *totalOutput = NULL;
	for (size_t k = 0; k < numKernels; k++) {
		nocl_multiConvolveArgs->outputIndex = k;
		totalOutput = calloc(1, length);
		for (size_t k2 = 0; k2 < numKernels; k2++) {
			sum = calloc(1, length);
			if (k == k2) {
				if (!nocl_convolve(nocl_multiConvolveArgs->input, imgBufs[0], kernels[k], k,
	          				  globalWorkSize, 1.f / (float)numKernels)) return false;
			}
			else {
				if (!nocl_convolve(nocl_multiConvolveArgs->input, imgBufs[0], kernels[k], k,
	          				  globalWorkSize, 1.f)) return false;

				if (!nocl_convolve(nocl_multiConvolveArgs->outputs[k], imgBufs[0], kernels[k2], k2,
	          				  globalWorkSize, 1.f / (float)numKernels)) return false;
			}
			nocl_AddImg(imgBufs[0].width * imgBufs[0].height, nocl_multiConvolveArgs->outputs[k], totalOutput, sum);
			free(totalOutput);
			totalOutput = sum;
			sum = NULL;
		}
		free(nocl_multiConvolveArgs->outputs[k]);
		nocl_multiConvolveArgs->outputs[k] = totalOutput;
	}
	return true;
}
