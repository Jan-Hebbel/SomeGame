#include "fonts.hpp"

#include "platform.hpp"

#include <lib/stb_truetype.h>

#define SIZE(x) sizeof(x) / sizeof(x[0])

const char *fonts[] = { "res/fonts/SourceCodePro-Regular.ttf" };
stbtt_bakedchar cdata[96];

void load_default_fonts() {
	float font_height = 32.0f; // pixel
	const int bitmap_w = 512;
	const int bitmap_h = bitmap_w;
	unsigned char *temp_bitmap = new unsigned char[bitmap_w * bitmap_h];

	for (int i = 0; i < SIZE(fonts); ++i) {
		File_Asset file_asset = {};
		uint32 bytes_read = platform_read_file(fonts[i], &file_asset);
		if (bytes_read == 0) { continue; }

		int chars_fit = stbtt_BakeFontBitmap(reinterpret_cast<const unsigned char *>(file_asset.data), 0, font_height, temp_bitmap, bitmap_w, bitmap_h, 32, 96, cdata);
		platform_free_file(&file_asset);
		if (chars_fit <= 0) {
			__debugbreak();
			continue;
		}

		const uint indices[] = {
			0, 1, 2, 2, 3, 0
		};

		// @ToDo: create font atlas as a combined image sampler to sample from later on
	}

	delete[] temp_bitmap;
}
