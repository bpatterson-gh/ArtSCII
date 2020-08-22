R"(
// Filters an image thru a kernel
// 2D, img[x, y] = global_id[0] + (imgW * global_id[1])
__kernel void convolve(constant uchar *img, global uchar *output, constant float *k,
		constant uint *knlSize, float knlMult, uchar knlInvert, float alpha) {
	size_t px = get_global_id(0),
		   py = get_global_id(1),
		   imgW = get_global_size(0),
		   imgH = get_global_size(1),
		   xMin = px - (size_t)(knlSize[0] / 2),
		   xMax = px + (size_t)(knlSize[0] / 2),
		   yMin = py - (size_t)(knlSize[1] / 2),
		   yMax = py + (size_t)(knlSize[1] / 2);
	if (xMin >= imgW || xMax >= imgW || yMin >= imgH || yMax >= imgH) {
		return;
	}
	uchar3 src;
	float3 pixel = (float3)(0.f, 0.f, 0.f);
	size_t x = xMin, xRel = 0, y, yRel;
	size_t ip, ik;
	for (; x <= xMax; x++, xRel++) {
		yRel = 0;
		for (y = yMin; y <= yMax; y++, yRel++) {
			ip = x + (imgW * y);
			ik = xRel + (knlSize[0] * yRel);
			src = vload3(ip, img);
			pixel += (float3)(src.x * k[ik], src.y * k[ik], src.z * k[ik]);
		}
	}
	pixel = fmax(fmin(pixel * knlMult, 255.f), 0.f);
	if (knlInvert > 0) pixel = 255.f - pixel;
	pixel *= alpha;
	uchar3 out = (uchar3)(pixel.x, pixel.y, pixel.z);
	size_t i = ((px - (knlSize[0] / 2)) + ((py - (knlSize[1] / 2)) * (imgW - knlSize[0] + 1)));
	vstore3(out, i, output);
}

// Adds two images together
// 1D, image index = global_id[0]
__kernel void addImg(global uchar *a, global uchar *b, global uchar *sum) {
	size_t i = get_global_id(0);
	uchar3 a3 = vload3(i, a), b3 = vload3(i, b);
	uchar3 s = a3 + b3;
	float3 si = fmax(fmin((float3)(s.x, s.y, s.z), 255.f), 0.f);
	vstore3((uchar3)(si.x, si.y, si.z), i, sum);
}

// Multiplies all pixels in an image by a scalar value
// 1D, image index = global_id[0]
__kernel void mult(global uchar *a, float m, global uchar *product) {
	size_t i = get_global_id(0);
	uchar3 p = vload3(i, a);
	float3 ip = fmax(fmin((float3)(p.x * m, p.y * m, p.z * m), 255.f), 0.f);
	vstore3((uchar3)(ip.x, ip.y, ip.z), i, product);
}

// Work-item is the size in pixels of one character
__kernel void characterMatch(constant uchar *imgs, constant int *imgSize,
		int numImgs, constant uchar *charImg, constant int *charSize,
		char currentChar, global uint *diffs, global uchar *matches,
		constant uchar *colorImg, global uchar *colors) {
	size_t gID = get_global_id(0) + (get_global_size(0) * get_global_id(1));
	if (get_global_id(0) == get_global_size(0) - 1) {
		matches[gID] = '\n';
		vstore3((uchar3)(255, 255, 255), gID, colors);
		return;
	}
	// diffs has one less column than global size, can't use gID
	size_t diffID = get_global_id(0) + ((get_global_size(0) - 1) * get_global_id(1));
	size_t bx = get_global_id(0) * charSize[0],
		   by = get_global_id(1) * charSize[1],
		   ex = bx + charSize[0],
		   ey = by + charSize[1],
		   x, y, xRel, yRel, i, iRel, img;
	uchar3 pixel, colorPixel, ch;
	uint3 color = (uint3)(0, 0, 0);
	uint diff = 0, area = charSize[0] * charSize[1];
	if (ex > imgSize[0]) ex = imgSize[0];
	if (ey > imgSize[1]) ey = imgSize[1];
	for (x = bx, xRel = 0; x < ex; x++, xRel++) {
		for (y = by, yRel = 0; y < ey; y++, yRel++) {
			i = x + (imgSize[0] * y);
			iRel = xRel + (charSize[0] * yRel);
			ch = vload3(iRel, charImg);
			for (img = 0; img < numImgs; img++) {
				pixel = vload3(i + (img * imgSize[0] * imgSize[1]), imgs);
				if(img == 0) {
					colorPixel = vload3(i, colorImg);
					color.x += (colorPixel.x - 64) * 1.333333f;
					color.y += (colorPixel.y - 64) * 1.333333f;
					color.z += (colorPixel.z - 64) * 1.333333f;
				}
				diff += abs((int)ch.x - (int)pixel.x);
				diff += abs((int)ch.y - (int)pixel.y);
				diff += abs((int)ch.z - (int)pixel.z);
			}
		}
	}
	if (diff < diffs[diffID]) {
		diffs[diffID] = diff;
		matches[gID] = currentChar;
	}
	vstore3((uchar3)(color.x / area, color.y / area, color.z / area), gID, colors);
}
)"
