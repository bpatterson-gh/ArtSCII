#include "artscii.h"

extern bool OCL_Init();
extern void OCL_Cleanup();

// Prints a cl_mem object as hexadecimal bytes
bool _dump_mem_obj(cl_mem obj, size_t length) {
	unsigned char *outs = malloc(length);
	result = clEnqueueReadBuffer(queue, obj, CL_TRUE,
		0, length, outs, 0, NULL, NULL);
	checkResultAndFree(outs)
	result = clFinish(queue);
	checkResultAndFree(outs)

	printf("\n%02x ", outs[0]);
	for (size_t i = 1; i < length; i++) {
		if (i % 64 == 0) printf("\n");
		printf("%02x ", outs[i]);
	}
	printf("\n");

	free(outs);
	return true;
}

// Prints a cl_mem object as hexadecimal bytes, or an error message
void dumpMemObj(cl_mem obj, size_t length) {
	if (!_dump_mem_obj(obj, length)) printf("Debug Error: could not dump cl_mem.\n");
}

// Writes a cl_mem object to a BMP file
bool dumpBitmap(cl_mem obj, const char *fileName, int width, int height) {
	unsigned int length = width * height * 3;

	// Get image data from buffer before attempting to create the file
	unsigned char *outs = malloc(length);

	result = clEnqueueReadBuffer(queue, obj, CL_TRUE, 0, length, outs, 0, NULL, NULL);
	checkResultAndFree(outs)
	result = clFinish(queue);
	checkResultAndFree(outs)

	unsigned int dibSize = 40,
				 zero = 0,
				 padRow = (((width * 3) % 4) == 0)? 0 : 4 - ((width * 3) % 4),
				 padLength = ((width * 3) + padRow) * height,
				 fileSize = 14 + dibSize + padLength,
				 imgOffset = fileSize - padLength;
	unsigned short colorPlanes = 1, bitsPerPixel = 24;
	int pixelsPerMeter = 2835;

	FILE *bmp = fopen(fileName, "wb+");
	if (bmp == NULL) {
		printf("Failed to create file \"%s\"\n", fileName);
		return false;
	}

	fputs("BM", bmp); // Header field ("BM")
	fwrite(&fileSize, 4, 1, bmp); // File Size field
	fwrite(&zero, 4, 1, bmp); // Reserved fields, set to 0
	fwrite(&imgOffset, 4, 1, bmp); // Image offset field

	// Using BITMAPINFOHEADER for DIB
	fwrite(&dibSize, 4, 1, bmp); // DIB Header Size
	fwrite(&width, 4, 1, bmp); // Bitmap width
	fwrite(&height, 4, 1, bmp); // Bitmap height
	fwrite(&colorPlanes, 2, 1, bmp); // # of color planes
	fwrite(&bitsPerPixel, 2, 1, bmp); // Bits per pixel
	fwrite(&zero, 4, 1, bmp); // Compression method
	fwrite(&padLength, 4, 1, bmp); // Image size
	fwrite(&pixelsPerMeter, 4, 1, bmp); // Horizontal resolution
	fwrite(&pixelsPerMeter, 4, 1, bmp); // Vertical resolution
	fwrite(&zero, 4, 1, bmp); // Number of colors in pallette
	fwrite(&zero, 4, 1, bmp); // Number of important colors
	
	// Pixel data
	size_t col, i;
	for (size_t row = height - 1; row < height; row--) {
		// Pixels are in G,B,R format
		for (col = 0; col < width; col++) {
			i = ((row * width) + col) * 3;
			fputc(outs[i + 2], bmp);
			fputc(outs[i + 1], bmp);
			fputc(outs[i], bmp);
		}
		// Pad each row to be a multiple of 4 bytes
		for (col = 0; col < padRow; col++) {
			fputc((char)0, bmp);
		}
	}

	fclose(bmp);
	free(outs);
	return true;
}

// Compile this to debug
int main() {
	unsigned char square[] = { 255, 0, 0,    0, 255, 0,      0, 0, 255,
						 	   255, 255, 0,  0, 255, 255,    255, 0, 255,
						 	   0, 0, 0,      127, 127, 127,  255, 255, 255 };
	int squareW = 3, squareH = 3;

	unsigned char fatRect[] = { 255, 0, 0,    0, 255, 0,    0, 0, 255,    0, 0, 0,
						  	    255, 255, 0,  0, 255, 255,  255, 0, 255,  255, 255, 255 };
	int fatRectW = 4, fatRectH = 2;

	unsigned char tallRect[] = { 255, 0, 0,  255, 255, 0,
						 	     0, 255, 0,  0, 255, 255,
						 	     0, 0, 255,  255, 0, 255 };
	int tallRectW = 2, tallRectH = 3;

	if(!OCL_Init()) return -1;
	cl_int result;

	cl_mem squareMem = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
		squareW * squareH * 3, square, &result);
	checkResult(-2)
	cl_mem fatRectMem = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
		fatRectW * fatRectH * 3, fatRect, &result);
	checkResult(-2)
	cl_mem tallRectMem = clCreateBuffer(context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
		tallRectW * tallRectH * 3, tallRect, &result);
	checkResult(-2)

	dumpBitmap(squareMem, "DebugFiles\\square.bmp", squareW, squareH);
	dumpBitmap(fatRectMem, "DebugFiles\\fatRect.bmp", fatRectW, fatRectH);
	dumpBitmap(tallRectMem, "DebugFiles\\tallRect.bmp", tallRectW, tallRectH);

	OCL_Cleanup();
	return 0;
}
