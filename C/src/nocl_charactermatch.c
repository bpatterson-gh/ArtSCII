#include "artscii.h"

NOCL_CharacterMatchArgs *nocl_characterMatchArgs = NULL;

extern int nocl_loadImage(ImageInfo *imgBufs, unsigned char *charBuf, size_t offset);
extern void nocl_kCharacterMatch(const unsigned char *imgs, const int *imgSize,
		int numImgs, const unsigned char *charImg, const int *charSize,
		char currentChar, unsigned int *diffs, unsigned char *matches,
		const unsigned char *colorImg, unsigned char *colors,
		const size_t *global_id, const size_t *global_size);

void nocl_freeCharacterMatchArgs() {
	if (nocl_characterMatchArgs != NULL) {
		if (nocl_characterMatchArgs->imgs != NULL) free(nocl_characterMatchArgs->imgs);
		if (nocl_characterMatchArgs->charImg != NULL) free(nocl_characterMatchArgs->charImg);
		if (nocl_characterMatchArgs->diffs != NULL) free(nocl_characterMatchArgs->diffs);
		free(nocl_characterMatchArgs);
		nocl_characterMatchArgs = NULL;
	}
}

// Initializes nocl_characterMatchArgs
bool nocl_setCharacterMatchArgs(unsigned char **imgs, int *imgSize,
		const int numImgs, ImageInfo *characters, int *charSize,
		const int numChars, char *charMap, unsigned char *matches,
		unsigned char *outColors, const size_t *globalSize) {
	nocl_freeCharacterMatchArgs();
	nocl_characterMatchArgs = calloc(1, sizeof(NOCL_CharacterMatchArgs));

	const size_t length = imgSize[0] * imgSize[1] * 3;

	nocl_characterMatchArgs->imgs = malloc(length * numImgs);
	for (size_t i = 0; i < numImgs; i++) {
		const size_t offset = i * length;
		for (size_t j = offset; j < (length * (i + 1)); j++) {
			nocl_characterMatchArgs->imgs[j] = imgs[i][j - offset];
		}
	}

	nocl_characterMatchArgs->imgSize = imgSize;

	nocl_characterMatchArgs->charImg = malloc(charSize[0] * charSize[1] * 3);
	int res = nocl_loadImage(characters, nocl_characterMatchArgs->charImg, 0);
	if (res == -1) return false;
	nocl_characterMatchArgs->charMapX = 1;
	nocl_characterMatchArgs->charSize = charSize;
	nocl_characterMatchArgs->currentChar = charMap[0];

	unsigned int diffLen = (globalSize[0] - 1) * globalSize[1];
	nocl_characterMatchArgs->diffs = malloc(sizeof(unsigned int) * diffLen);
	for (size_t d = 0; d < diffLen; d++) nocl_characterMatchArgs->diffs[d] = 0xffffffff;

	nocl_characterMatchArgs->matches = matches;

	return true;
}

// Switches to the next character for comparison to the image
bool nocl_setNextCharacter(ImageInfo *characters, char *charMap) {
	int res = nocl_loadImage(&characters[nocl_characterMatchArgs->charMapX], nocl_characterMatchArgs->charImg, 0);
	if (res == -1) return false;
	
	nocl_characterMatchArgs->currentChar = charMap[nocl_characterMatchArgs->charMapX++];

	return true;
}

// Matches ASCII characters and colors to the input Image
bool NOCL_CharacterMatch(unsigned char **imgs, int *imgSize, const int numImgs,
		ImageInfo *characters, int numChars, char *charMap, unsigned char *matches,
		unsigned char *colorImg, unsigned char *outColors) {
	int charSize[2] = { characters[0].width, characters[0].height };
	const size_t globalSize[2] = { (size_t)((imgSize[0] / charSize[0]) + 1),
								  (size_t)((imgSize[1] / charSize[1])) };
	size_t globalID[2] = {0, 0};
	
	if (!nocl_setCharacterMatchArgs(imgs, imgSize, numImgs, characters, charSize, numChars,
							   charMap, matches, outColors, globalSize)) return false;

	while (true) {
		for (size_t i = 0; i < globalSize[0]; i++) {
			globalID[0] = i;
			for (size_t j = 0; j < globalSize[1]; j++) {
				globalID[1] = j;
				nocl_kCharacterMatch(nocl_characterMatchArgs->imgs, imgSize, numImgs,
									 nocl_characterMatchArgs->charImg, charSize,
									 nocl_characterMatchArgs->currentChar, nocl_characterMatchArgs->diffs,
									 nocl_characterMatchArgs->matches, colorImg, outColors, globalID, globalSize);
			}
		}

		if (nocl_characterMatchArgs->charMapX < numChars) {
			nocl_setNextCharacter(characters, charMap);
		}
		else break;
	}

	return true;
}
