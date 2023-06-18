#ifndef ASSETS_H
#define ASSETS_H

#include "types.hpp"
#include "math.hpp"

#include <vulkan/vulkan.h>

//typedef struct sVkImage VkImage;
//typedef struct sVkDeviceMemory VkDeviceMemory;
//typedef struct sVkImageView VkImageView;

struct Texture {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView image_view;
	VkSampler sampler;
};

enum Buffer_Type {
	VERTEX_BUFFER = 0,
	INDEX_BUFFER = 1,
};

struct Render_Buffer {
	Buffer_Type type;
	VkBuffer buffer;
	VkDeviceMemory memory;
};

struct Texture_Asset { // @Optimization: SoA vs AoS
	Texture texture;
	Render_Buffer vertex_buffer;
	Render_Buffer index_buffer;
};

struct Vertex {
	Vec2 pos;
	Vec2 tex_coord;
};

bool create_texture_asset(const char *file_path, const Vertex *vertices, const uint *indices);
void delete_texture_asset(const char *label);

Texture_Asset get_next_texture_asset(uint *index);
uint get_texture_asset_count();

#endif
