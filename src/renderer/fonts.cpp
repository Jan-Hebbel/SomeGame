//#include "fonts.hpp"
//
//#include "types.hpp"
//#include "assets.hpp"
//#include "platform.hpp"
//#include "renderer/vulkan_init.hpp"
//
//#include <vulkan/vulkan.h>
//#include <lib/stb_truetype.h>
//
//Font_Asset *load_font(const char *path_to_font, float font_height) {
//	Font_Asset *font_asset = new Font_Asset;
//
//	const int bitmap_w = 512;
//	const int bitmap_h = bitmap_w;
//	unsigned char *temp_bitmap = new unsigned char[bitmap_w * bitmap_h];
//
//	File_Asset file_asset = {};
//	uint32 bytes_read = platform_read_file(path_to_font, &file_asset);
//	if (bytes_read == 0) {
//		delete font_asset;
//		return NULL;
//	}
//
//	int chars_fit = stbtt_BakeFontBitmap(reinterpret_cast<const unsigned char *>(file_asset.data), 0, font_height, temp_bitmap, bitmap_w, bitmap_h, 32, 96, font_asset->cdata);
//	platform_free_file(&file_asset);
//	if (chars_fit <= 0) {
//		delete font_asset;
//		return NULL;
//	}
//
//	const Vertex vertices[] = {
//		{.pos = {-0.5f, -0.5f}, .tex_coord = {0.0f, 0.0f}},
//		{.pos = { 0.5f, -0.5f}, .tex_coord = {1.0f, 0.0f}},
//		{.pos = { 0.5f,  0.5f}, .tex_coord = {1.0f, 1.0f}},
//		{.pos = {-0.5f,  0.5f}, .tex_coord = {0.0f, 1.0f}},
//	};
//
//	const uint indices[] = {
//		0, 1, 2, 2, 3, 0
//	};
//		
//	bool result = create_texture_asset(reinterpret_cast<char *>(temp_bitmap), vertices, indices);
//	delete[] temp_bitmap;
//	if (!result) {
//		delete font_asset;
//		return NULL;
//	}
//
//	return font_asset;
//}
//
//void delete_font_asset(Font_Asset *font_asset) {
//	delete_texture_asset(font_asset->texture_asset);
//	delete font_asset;
//}
