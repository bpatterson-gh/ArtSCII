#include "artscii.h"

CharacterMatchArgs *characterMatchArgs = NULL;

cl_kernel clkCharacterMatch;

extern int loadImage(ImageInfo *imgBufs, cl_mem *clBuf, size_t offset);

void freeCharacterMatchArgs() {
	if (characterMatchArgs != NULL) {
		if (characterMatchArgs->imgs != NULL) clReleaseMemObject(characterMatchArgs->imgs);
		if (characterMatchArgs->imgSize != NULL) clReleaseMemObject(characterMatchArgs->imgSize);
		if (characterMatchArgs->charImg != NULL) clReleaseMemObject(characterMatchArgs->charImg);
		if (characterMatchArgs->charSize != NULL) clReleaseMemObject(characterMatchArgs->charSize);
		if (characterMatchArgs->currentChar != NULL) clReleaseMemObject(characterMatchArgs->currentChar);
		if (characterMatchArgs->diffs != NULL) clReleaseMemObject(characterMatchArgs->diffs);
		if (characterMatchArgs->matches != NULL) clReleaseMemObject(characterMatchArgs->matches);
		if (characterMatchArgs->outColors != NULL) clReleaseMemObject(characterMatchArgs->outColors);
		free(characterMatchArgs);
		characterMatchArgs = NULL;
	}
}

// Initializes characterMatchArgs
bool setCharacterMatchArgs(cl_mem *imgs, int *imgSize, int numImgs, ImageInfo *characters,
		int *charSize, int numChars, char *charMap, unsigned char *matches,
		cl_mem colorImg, unsigned char *outColors, size_t *globalSize) {
	freeCharacterMatchArgs();
	characterMatchArgs = calloc(1, sizeof(CharacterMatchArgs));

	characterMatchArgs->imgs = clCreateBuffer(context, CL_MEM_READ_ONLY,
											  imgSize[0] * imgSize[1] * 3 * numImgs, NULL, &result);
	CHECK_RESULT(false)

	for (size_t i = 0; i < numImgs; i++) {
		result = clEnqueueCopyBuffer(queue, imgs[i], characterMatchArgs->imgs, 0,
									 i * imgSize[0] * imgSize[1] * 3, imgSize[0] * imgSize[1] * 3,
									 0, NULL, NULL);
		CHECK_RESULT(false)
	}
	result = clFinish(queue);
	CHECK_RESULT(false)

	characterMatchArgs->imgSize = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
									   sizeof(int) * 2, imgSize, &result);
	CHECK_RESULT(false)

	characterMatchArgs->charImg = clCreateBuffer(context, CL_MEM_READ_ONLY, 
											   charSize[0] * charSize[1] * 3, NULL, &result);
	CHECK_RESULT(false)

	int res = loadImage(characters, &characterMatchArgs->charImg, 0);
	if (res == -1) return false;

	characterMatchArgs->charBufX = res;
	characterMatchArgs->charMapX = 1;
	characterMatchArgs->charSize = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
									   			  sizeof(int) * 2, charSize, &result);
	CHECK_RESULT(false)

	characterMatchArgs->currentChar = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
													 sizeof(char), charMap, &result);
	CHECK_RESULT(false)

	unsigned int diffLen = (globalSize[0] - 1) * globalSize[1];
	unsigned int *diffs = malloc(sizeof(unsigned int) * diffLen);
	for (size_t d = 0; d < diffLen; d++) diffs[d] = 0xffffffff;
	characterMatchArgs->diffs = clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,
											   sizeof(unsigned int) * diffLen,
											   diffs, &result);
	CHECK_RESULT_AND_FREE(diffs)
	free(diffs);

	characterMatchArgs->matches = clCreateBuffer(context, CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,
												 sizeof(unsigned char) * globalSize[0] * globalSize[1],
												 matches, &result);
	CHECK_RESULT(false)

	characterMatchArgs->outColors = clCreateBuffer(context, CL_MEM_READ_WRITE,
												   3 * sizeof(unsigned char) * globalSize[0] * globalSize[1],
												   NULL, &result);
	CHECK_RESULT(false)

	result = clSetKernelArg(clkCharacterMatch, 0, sizeof(cl_mem), &characterMatchArgs->imgs);
	CHECK_RESULT(false)
	result = clSetKernelArg(clkCharacterMatch, 1, sizeof(cl_mem), &characterMatchArgs->imgSize);
	CHECK_RESULT(false)
	result = clSetKernelArg(clkCharacterMatch, 2, sizeof(int), &numImgs);
	CHECK_RESULT(false)
	result = clSetKernelArg(clkCharacterMatch, 3, sizeof(cl_mem), &characterMatchArgs->charImg);
	CHECK_RESULT(false)
	result = clSetKernelArg(clkCharacterMatch, 4, sizeof(cl_mem), &characterMatchArgs->charSize);
	CHECK_RESULT(false)
	result = clSetKernelArg(clkCharacterMatch, 5, sizeof(char), &charMap[0]);
	CHECK_RESULT(false)
	result = clSetKernelArg(clkCharacterMatch, 6, sizeof(cl_mem), &characterMatchArgs->diffs);
	CHECK_RESULT(false)
	result = clSetKernelArg(clkCharacterMatch, 7, sizeof(cl_mem), &characterMatchArgs->matches);
	CHECK_RESULT(false)
	result = clSetKernelArg(clkCharacterMatch, 8, sizeof(cl_mem), &colorImg);
	CHECK_RESULT(false)
	result = clSetKernelArg(clkCharacterMatch, 9, sizeof(cl_mem), &characterMatchArgs->outColors);
	CHECK_RESULT(false)

	return true;
}

// Switches to the next character for comparison to the image
bool setNextCharacter(ImageInfo *characters, char *charMap) {
	int res = loadImage(&characters[characterMatchArgs->charBufX], &characterMatchArgs->charImg, 0);
	if (res == -1) return false;
	characterMatchArgs->charBufX += res;

	result = clSetKernelArg(clkCharacterMatch, 5, sizeof(char), &charMap[characterMatchArgs->charMapX++]);
	CHECK_RESULT(false)
	
	return true;
}

// Matches ASCII characters and colors to the input Image
bool OCL_CharacterMatch(cl_mem *imgs, int *imgSize, int numImgs, ImageInfo *characters, int numChars,
		char *charMap, unsigned char *matches, cl_mem colorImg, unsigned char *outColors) {
	int charSize[2] = { characters[0].width, characters[0].height };
	size_t globalSize[2] = { (size_t)((imgSize[0] / charSize[0]) + 1),
							 (size_t)((imgSize[1] / charSize[1])) };
	size_t localSize[2] = { 1, 1 };

	if (!setCharacterMatchArgs(imgs, imgSize, numImgs, characters, charSize, numChars,
							   charMap, matches, colorImg, outColors, globalSize)) return false;

	while (true) {
		result = clEnqueueNDRangeKernel(queue, clkCharacterMatch, 2, NULL, 
			globalSize, localSize, 0, NULL, NULL);
		CHECK_RESULT(false)
	
		result = clFinish(queue);
		CHECK_RESULT(false)

		if (characterMatchArgs->charMapX < numChars) {
			setNextCharacter(characters, charMap);
		}
		else break;
	}

	result = clEnqueueReadBuffer(queue, characterMatchArgs->matches, CL_TRUE,
		0, sizeof(char) * globalSize[0] * globalSize[1], matches, 0, NULL, NULL);
	CHECK_RESULT(false)

	result = clEnqueueReadBuffer(queue, characterMatchArgs->outColors, CL_TRUE,
		0, 3 * sizeof(unsigned char) * globalSize[0] * globalSize[1], outColors, 0, NULL, NULL);
	CHECK_RESULT(false)

	result = clFinish(queue);
	CHECK_RESULT(false)

	return true;
}
