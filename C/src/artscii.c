#include "artscii.h"

cl_platform_id platform = NULL;
cl_device_id device;
cl_context context;
cl_command_queue queue;
cl_program program;
cl_int result = CL_SUCCESS;

struct _timeb perfStart, perfEnd;
long perfElapsed = 0;

extern MultiConvolveArgs *multiConvolveArgs;
extern CharacterMatchArgs *characterMatchArgs;
extern cl_kernel clkConvolve, clkAddImg, clkMult, clkCharacterMatch;

extern void freeMultiConvolveArgs();
extern void freeCharacterMatchArgs();
extern bool OCL_MultiConvolve(ImageInfo *imgBufs, KernelInfo *kernels, size_t numKernels);
extern bool OCL_CharacterMatch(cl_mem *imgs, int *imgSize, int numImgs, ImageInfo *characters,
		int numChars, char *charMap, unsigned char *matches, cl_mem colorImg, unsigned char *colors);

// Cleans up all dynamic memory associated with this library
__declspec(dllexport) void OCL_Cleanup() {
	freeMultiConvolveArgs();
	freeCharacterMatchArgs();
	clReleaseKernel(clkConvolve);
	clReleaseKernel(clkAddImg);
	clReleaseKernel(clkMult);
	clReleaseKernel(clkCharacterMatch);
	clReleaseProgram(program);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
}

// Handles OpenCL errors and displays error messages
void err(int errorCode) {
	const char *errPrefix = "Error: ";
	const char *errStr;
	const char *errSuffix = " The program will terminate.";
	switch (errorCode) {
		case CL_OUT_OF_HOST_MEMORY:
		case CL_MEM_OBJECT_ALLOCATION_FAILURE:
			errStr = "Not enough memory for OpenCL.";
			break;
		case CL_OUT_OF_RESOURCES:
			errStr = "Could not allocate resources for OpenCL.";
			break;
		case CL_DEVICE_NOT_AVAILABLE:
			errStr = "Your OpenCL device is not available for use.";
			break;
		case CL_DEVICE_NOT_FOUND:
			errStr = "Your CPU does not support OpenCL.";
			break;
		case CL_COMPILER_NOT_AVAILABLE:
			errStr = "No compiler available for OpenCL.";
			break;
		case CL_BUILD_PROGRAM_FAILURE:
			errStr = "Failed to build kernels.cl for OpenCL device.";
			break;
		case CL_INVALID_BUFFER_SIZE:
			errStr = "Invalid image size.";
			break;
		case CL_INVALID_KERNEL_ARGS:
			errStr = "Invalid Kernel args.";
			break;
		case CL_INVALID_ARG_VALUE:
			errStr = "Invalid arg value.";
			break;
		case CL_INVALID_MEM_OBJECT:
			errStr = "Invalid mem object.";
			break;
		case CL_INVALID_ARG_SIZE:
			errStr = "Invalid arg size.";
			break;
		case CL_INVALID_EVENT:
			errStr = "Event is invalid.";
			break;
		case CL_INVALID_VALUE:
			errStr = "Invalid value.";
			break;
		case CL_INVALID_COMMAND_QUEUE:
			errStr = "Invalid command queue.";
			break;
		case CL_INVALID_CONTEXT:
			errStr = "Invalid context.";
			break;
		case CL_INVALID_EVENT_WAIT_LIST:
			errStr = "Invalid event wait list.";
			break;
		default:
			errPrefix = "";
			errStr = "An error has occurred with OpenCL.";
			break;
	}
	fprintf(stderr, "%s%s%s\n", errPrefix, errStr, errSuffix);

	OCL_Cleanup();
}

// Initializes the ArtSCII OpenCL library
__declspec(dllexport) bool OCL_Init() {
	// Import OpenCL C source (compile with -std=gnu99)
	const char *kernelSrc = 
		#include "kernels.cl"
	;

	cl_uint count;
	result = clGetPlatformIDs(1, &platform, &count);
	if (count == 0) return false;
	checkResult(false)
	
	result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
	if (result == CL_DEVICE_NOT_FOUND) {
		result = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL);
	}
	checkResult(false)

	context = clCreateContext(NULL, 1, &device, NULL, NULL, &result);
	checkResult(false)
	
	queue = clCreateCommandQueue(context, device, 0, &result);
	checkResult(false)

	program = clCreateProgramWithSource(context, 1, &kernelSrc, NULL, &result);
	checkResult(false)
	
	result = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (result != CL_SUCCESS) {
		// kernel.cl build logs
		char buf[900000];
		clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 900000, buf, NULL);
		printf(buf);
		err(result);
		return false;
	}

	// Kernels
	clkConvolve = clCreateKernel(program, "convolve", &result);
	checkResult(false)
	clkAddImg = clCreateKernel(program, "addImg", &result);
	checkResult(false)
	clkMult = clCreateKernel(program, "mult", &result);
	checkResult(false)
	clkCharacterMatch = clCreateKernel(program, "characterMatch", &result);
	checkResult(false)

	return true;
}

// Converts an Image to ASCII characters
__declspec(dllexport) bool OCL_ToAscii(ImageInfo *imgBufs, unsigned char *outChars,
		unsigned char *outColors, KernelInfo *kernels, size_t numKernels, ImageInfo* charBufs,
		int numChars, char *charMap) {
	int imgSize[2] = { imgBufs[0].width, imgBufs[0].height };

	if (!OCL_MultiConvolve(imgBufs, kernels, numKernels)) return false;

	if (!OCL_CharacterMatch(multiConvolveArgs->outputs, imgSize, numKernels,
						charBufs, numChars, charMap, outChars,
						multiConvolveArgs->input, outColors)) return false;
	freeMultiConvolveArgs();
	freeCharacterMatchArgs();
	return true;
}

// Returns the number of buffers processed, or -1 if an error occurred.
int loadImage(ImageInfo *imgBufs, cl_mem *clBuf, size_t offset) {
	size_t i = 0;
	while (true) {
		result = clEnqueueWriteBuffer(queue, *clBuf, CL_TRUE, offset, imgBufs[i].bufSize,
			imgBufs[i].buffer, 0, NULL, NULL);
		checkResult(-1)
		offset += imgBufs[i].bufSize;
		if (imgBufs[i].final) break;
		i++;
	}
	result = clFinish(queue);
	checkResult(-1)
	return i + 1;
}
