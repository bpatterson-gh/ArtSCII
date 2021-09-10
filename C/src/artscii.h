#pragma once

#define CL_TARGET_OPENCL_VERSION 120

#include <CL/opencl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/timeb.h>
#include <sys/types.h>

// ----------- Data structures from C# ----------- //

// ~64KB ImageInfo buffer (due to CLR array constraints)
#define IMG_INFO_BUF_SIZE 65536 - 17

typedef struct ImageInfo {
	unsigned int width;
	unsigned int height;
	unsigned int bufSize;
	bool final;
	unsigned char buffer[IMG_INFO_BUF_SIZE];
} ImageInfo;

// 1KB KernelInfo buffer
#define KNL_INFO_BUF_SIZE 256

typedef struct KernelInfo {
	unsigned int width;
	unsigned int height;
	float mult;
	bool invert;
	unsigned int bufSize;
	float buffer[KNL_INFO_BUF_SIZE]; // 1 KB buffer
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

// --------------- NOCL arguments -------------- //
typedef struct NOCL_MultiConvolveArgs {
	float **kernels,
		  *knlMults;
	unsigned char *input,
				  **outputs,
				  *knlInverts;
	size_t *knlSizes,
		   numKernels,
		   outputIndex;
} NOCL_MultiConvolveArgs;

typedef struct NOCL_CharacterMatchArgs {
	char currentChar;
	int *imgSize,
		*charSize;
	unsigned char *imgs,
				  *charImg,
				  *matches;
	unsigned int *diffs;
	size_t charMapX;
} NOCL_CharacterMatchArgs;
// ----------------------------------------------- //

// --------------- Global Variables -------------- //
extern cl_command_queue queue;
extern cl_context context;
extern cl_int result;
extern long perfElapsed;
#ifdef _WIN32
	#define TIMEB _timeb
#else
	#define TIMEB timeb
#endif
extern struct TIMEB perfStart, perfEnd;
// ----------------------------------------------- //

// ------------------ Debugging ------------------ //
extern void dumpMemObj(cl_mem obj, size_t length);
extern bool dumpBitmap(cl_mem obj, const char *fileName, int width, int height);
extern bool nocl_dumpBitmap(unsigned char *outs, const char *fileName, int width, int height);
// ----------------------------------------------- //

// ---- Check the result of OpenCL operations ---- //
extern void err(int errorCode);

#define CHECK_RESULT(ret) if (result != CL_SUCCESS) { \
							err(result);              \
							return ret; }

#define CHECK_RESULT_AND_FREE(f) if (result != CL_SUCCESS) { \
						  			 err(result);            \
						  			 free(f);                \
						  			 return false; }
// ----------------------------------------------- //

// ------------- Performance testing ------------- //
#define RESET_PERF_TIMER() perfElapsed = 0;      \
						   _ftime_s(&perfStart);
								
#define LOG_PERFORMANCE(str) _ftime_s(&perfEnd);                                   \
							perfElapsed = ((perfEnd.time - perfStart.time) * 1000) \
								+ (perfEnd.millitm - perfStart.millitm);           \
							printf(str, perfElapsed);
// ----------------------------------------------- //

// -------------------- Exports ------------------ //
#ifdef _WIN32
	#define EXPORT __declspec(dllexport)
#else
    #ifdef _DEBUG
		#define EXPORT 
	#else
		#define EXPORT __attribute__((visibility("default")))
	#endif
#endif
// ----------------------------------------------- //
