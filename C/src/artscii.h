#pragma once

#include <CL/opencl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/timeb.h>
#include <sys/types.h>

// ----------- Data structures from C# ----------- //
typedef struct ImageInfo {
	unsigned int width;
	unsigned int height;
	int bufSize;
	bool final;
	unsigned char buffer[65536 - 17]; // ~64KB buffer (due to CLR array constraints)
} ImageInfo;

typedef struct KernelInfo {
	unsigned int width;
	unsigned int height;
	float mult;
	bool invert;
	int bufSize;
	float buffer[256]; // 1 KB buffer
} KernelInfo;
// ----------------------------------------------- //

// --------------- OpenCL arguments -------------- //
typedef struct MultiConvolveArgs {
	cl_mem input,
	       *outputs,
	       *kernels,
		   *knlSizes;
	float *knlMults;
	unsigned char *knlInverts;
	size_t numKernels;
} MultiConvolveArgs;

typedef struct CharacterMatchArgs {
	cl_mem imgs,
		   imgSize,
		   charImg,
		   charSize,
		   currentChar,
		   diffs,
		   matches,
		   outColors;
	size_t charBufX,
	       charMapX;
} CharacterMatchArgs;
// ----------------------------------------------- //

// --------------- Global Variables -------------- //
extern cl_command_queue queue;
extern cl_context context;
extern cl_int result;
extern long perfElapsed;
extern struct _timeb perfStart, perfEnd;
// ----------------------------------------------- //

// ------------- artscii.c functions ------------- //
extern int loadImage(ImageInfo *imgBufs, cl_mem *clBuf, size_t offset);
// ----------------------------------------------- //

// ------------------ Debugging ------------------ //
extern void dumpMemObj(cl_mem obj, size_t length);
extern bool dumpBitmap(cl_mem obj, const char *fileName, int width, int height);
// ----------------------------------------------- //

// ---- Check the result of OpenCL operations ---- //
extern void err(int errorCode);

#define checkResult(ret) if (result != CL_SUCCESS) { \
						  err(result);			     \
						  return ret; }

#define checkResultAndFree(f) if (result != CL_SUCCESS) { \
						  		  err(result);			  \
						  		  free(f);				  \
						  		  return false; }

#define checkResultAndRelease(r) if (result != CL_SUCCESS) { \
									 err(result);			 \
									 clReleaseMemObject(r);	 \
									 return false; }
// ----------------------------------------------- //

// ------------- Performance testing ------------- //
#define resetPerformanceTimer() perfElapsed = 0; \
								_ftime_s(&perfStart);
								
#define logPerformance(str) _ftime_s(&perfEnd);									   \
							perfElapsed = ((perfEnd.time - perfStart.time) * 1000) \
								+ (perfEnd.millitm - perfStart.millitm);           \
							printf(str, perfElapsed);
// ----------------------------------------------- //
