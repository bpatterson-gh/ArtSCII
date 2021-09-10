// This is a non-OpenCL implementation of the same algorithms in kernels.cl
// Changes here should have corresponding changes in kernels.cl

#include <math.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "artscii.h"

typedef unsigned char uchar;
typedef unsigned int uint;

// OpenCL library functions

void nocl_vload3(uchar *u3, size_t index, const uchar *data) {
	index *= 3;
	u3[0] = data[index];
	u3[1] = data[++index];
	u3[2] = data[++index];
}

void nocl_vstore3(const uchar *u3, size_t index, uchar *data) {
	index *= 3;
	data[index] = u3[0];
	data[++index] = u3[1];
	data[++index] = u3[2];
}

// Filters an image thru a kernel
// 2D, img[x, y] = global_id[0] + (imgW * global_id[1])
void nocl_kConvolve(const uchar *img, uchar *output, const float *k,
		const uint *knlSize, float knlMult, uchar knlInvert, float alpha,
		const size_t *global_id, const size_t *global_size) {
	size_t px = global_id[0],
		   py = global_id[1],
		   imgW = global_size[0],
		   imgH = global_size[1],
		   xMin = px - (size_t)(knlSize[0] / 2),
		   xMax = px + (size_t)(knlSize[0] / 2),
		   yMin = py - (size_t)(knlSize[1] / 2),
		   yMax = py + (size_t)(knlSize[1] / 2);
	if (xMin >= imgW || xMax >= imgW || yMin >= imgH || yMax >= imgH) {
		return;
	}
	uchar src[3];
	float pixel[3] = {0.f, 0.f, 0.f};
	size_t x = xMin, xRel = 0, y, yRel;
	size_t ip, ik;
	for (; x <= xMax; x++, xRel++) {
		yRel = 0;
		for (y = yMin; y <= yMax; y++, yRel++) {
			ip = x + (imgW * y);
			ik = xRel + (knlSize[0] * yRel);
			nocl_vload3(src, ip, img);
			pixel[0] += src[0] * k[ik];
			pixel[1] += src[1] * k[ik];
			pixel[2] += src[2] * k[ik];
		}
	}
	pixel[0] = fmax(fmin(pixel[0] * knlMult, 255.f), 0.f);
	pixel[1] = fmax(fmin(pixel[1] * knlMult, 255.f), 0.f);
	pixel[2] = fmax(fmin(pixel[2] * knlMult, 255.f), 0.f);
	if (knlInvert > 0) {
		pixel[0] = 255.f - pixel[0];
		pixel[1] = 255.f - pixel[1];
		pixel[2] = 255.f - pixel[2];
	}
	uchar out[3];
	out[0] = (uchar)(pixel[0] * alpha);
	out[1] = (uchar)(pixel[1] * alpha);
	out[2] = (uchar)(pixel[2] * alpha);
	size_t i = ((px - (knlSize[0] / 2)) + ((py - (knlSize[1] / 2)) * (imgW - knlSize[0] + 1)));
	nocl_vstore3(out, i, output);
}

// Adds two images together
// 1D, image index = global_id[0]
void nocl_kAddImg(const uchar *a, const uchar *b, uchar *sum, const size_t global_id) {
	uchar a3[3], b3[3], s[3];
	nocl_vload3(a3, global_id, a);
	nocl_vload3(b3, global_id, b);
	s[0] = fmax(fmin((float)(a3[0] + b3[0]), 255.f), 0.f);
	s[1] = fmax(fmin((float)(a3[1] + b3[1]), 255.f), 0.f);
	s[2] = fmax(fmin((float)(a3[2] + b3[2]), 255.f), 0.f);
	nocl_vstore3(s, global_id, sum);
}

// Multiplies all pixels in an image by a scalar value
// 1D, image index = global_id[0]
void nocl_kMult(const uchar *a, float m, uchar *product, const size_t global_id) {
	uchar p[3];
	nocl_vload3(p, global_id, a);
	p[0] = (uchar)fmax(fmin(p[0] * m, 255.f), 0.f);
	p[1] = (uchar)fmax(fmin(p[1] * m, 255.f), 0.f);
	p[2] = (uchar)fmax(fmin(p[2] * m, 255.f), 0.f);
	nocl_vstore3(p, global_id, product);
}

// Matches characters to parts of an image
// Work-item is the size in pixels of one character
void nocl_kCharacterMatch(const uchar *imgs, const int *imgSize,
		int numImgs, const uchar *charImg, const int *charSize,
		char currentChar, uint *diffs, uchar *matches,
		const uchar *colorImg, uchar *colors,
		const size_t *global_id, const size_t *global_size) {
	size_t gID = global_id[0] + (global_size[0] * global_id[1]);
	if (global_id[0] == global_size[0] - 1) {
		matches[gID] = '\n';
		uchar ucharMax[3] = {255, 255, 255};
		nocl_vstore3(ucharMax, gID, colors);
		return;
	}
	// diffs has one less column than size, can't use gID
	size_t diffID = global_id[0] + ((global_size[0] - 1) * global_id[1]);
	size_t bx = global_id[0] * charSize[0],
		   by = global_id[1] * charSize[1],
		   ex = bx + charSize[0],
		   ey = by + charSize[1],
		   x, y, xRel, yRel, i, iRel, img;
	uchar pixel[3], colorPixel[3], ch[3];
	uint color[3] = {0, 0, 0};
	uint diff = 0, area = charSize[0] * charSize[1];
	if (ex > imgSize[0]) ex = imgSize[0];
	if (ey > imgSize[1]) ey = imgSize[1];
	for (x = bx, xRel = 0; x < ex; x++, xRel++) {
		for (y = by, yRel = 0; y < ey; y++, yRel++) {
			i = x + (imgSize[0] * y);
			iRel = xRel + (charSize[0] * yRel);
			nocl_vload3(ch, iRel, charImg);
			for (img = 0; img < numImgs; img++) {
				nocl_vload3(pixel, i + (img * imgSize[0] * imgSize[1]), imgs);
				if(img == 0) {
					nocl_vload3(colorPixel, i, colorImg);
					color[0] += (colorPixel[0] - 64) * 1.333333f;
					color[1] += (colorPixel[1] - 64) * 1.333333f;
					color[2] += (colorPixel[2] - 64) * 1.333333f;
				}
				diff += abs((int)ch[0] - (int)pixel[0]);
				diff += abs((int)ch[1] - (int)pixel[1]);
				diff += abs((int)ch[2] - (int)pixel[2]);
			}
		}
	}
	if (diff < diffs[diffID]) {
		diffs[diffID] = diff;
		matches[gID] = currentChar;
	}
	uchar out[3];
	out[0] = (uchar)(color[0] / area);
	out[1] = (uchar)(color[1] / area);
	out[2] = (uchar)(color[2] / area);
	nocl_vstore3(out, gID, colors);
}


// Returns the number of buffers processed
int nocl_loadImage(ImageInfo *imgBufs, unsigned char *charBuf, size_t offset) {
	size_t i = 0;
	while (true) {
		for (size_t j = offset, k = 0; k < imgBufs[i].bufSize; j++, k++) {
			charBuf[j] = imgBufs[i].buffer[k];
		}
		offset += imgBufs[i].bufSize;
		if (imgBufs[i].final) break;
		i++;
	}
	return i + 1;
}
