#include "assets.hpp"

#include "renderer/vulkan_init.hpp"
#include "renderer/vulkan_helper.hpp"

#include <lib/stb_image.h>
#include <vulkan/vulkan.h>

//
// Internal
//

typedef struct Tag_Texture_Asset_List {
	uint count;
	Texture textures[MAX_TEXTURE_BINDINGS];
	Render_Buffer vertex_buffers[MAX_TEXTURE_BINDINGS];
	Render_Buffer index_buffers[MAX_TEXTURE_BINDINGS];
} Texture_Asset_List;

Texture_Asset_List texture_asset_list = {};

internal_function bool create_image(uint32 width, uint32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage_flags, VkMemoryPropertyFlags property_flags, VkImage &image, VkDeviceMemory &image_memory) {
	VkImageCreateInfo image_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = tiling,
		.usage = usage_flags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};
	VkResult result = vkCreateImage(c.device, &image_info, 0, &image);
	if (result != VK_SUCCESS) {
		return false;
	}
	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(c.device, image, &memory_requirements);

	uint32_t index;
	bool res = find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &index);
	if (!res) {
		return false;
	}
	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = index,
	};

	result = vkAllocateMemory(c.device, &alloc_info, 0, &image_memory);
	if (result != VK_SUCCESS) {
		return false;
	}

	vkBindImageMemory(c.device, image, image_memory, 0);

	return true;
}

internal_function bool transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout) {
	VkCommandBuffer command_buffer = begin_single_time_commands();

	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
	};

	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags destination_stage;
	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		return false;
	}

	vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, 0, 0, 0, 1, &barrier);

	end_single_time_commands(command_buffer);

	return true;
}

internal_function bool copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32 width, uint32 height) {
	VkCommandBuffer command_buffer = begin_single_time_commands();

	VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
		.imageOffset = { 0, 0, 0 },
		.imageExtent = { width, height, 1 },
	};
	vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	end_single_time_commands(command_buffer);

	return true;
}

internal_function bool create_texture_image(const char *texture_data, int width, int height, int nr_channels, Texture *texture, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB) {
	if (!texture_data) return false;

	VkDeviceSize image_size = width * height * nr_channels;

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;

	bool result = create_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);
	if (!result) return false;

	void *data;
	vkMapMemory(c.device, staging_buffer_memory, 0, image_size, 0, &data);
	memcpy(data, texture_data, static_cast<size_t>(image_size));
	vkUnmapMemory(c.device, staging_buffer_memory);

	result = create_image(width, height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture->image, texture->memory);
	if (!result) return false;

	result = transition_image_layout(texture->image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	if (!result) return false;

	result = copy_buffer_to_image(staging_buffer, texture->image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	if (!result) return false;

	result = transition_image_layout(texture->image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	if (!result) return false;

	vkDestroyBuffer(c.device, staging_buffer, 0);
	vkFreeMemory(c.device, staging_buffer_memory, 0);

	return true;
}

internal_function bool create_texture_image_view(Texture *texture, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB) {
	VkImageViewCreateInfo view_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = texture->image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
	};
	VkResult result = vkCreateImageView(c.device, &view_info, 0, &texture->image_view);
	if (result != VK_SUCCESS) {
		return false;
	}

	return true;
}

internal_function bool create_texture_image_sampler(Texture *texture) {
	VkSamplerCreateInfo sampler_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_NEAREST,
		.minFilter = VK_FILTER_NEAREST,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_FALSE,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
	};
	VkResult result = vkCreateSampler(c.device, &sampler_info, 0, &texture->sampler);
	if (result != VK_SUCCESS) {
		return false;
	}

	return true;
}

internal_function bool create_render_buffer(const void *elements, size_t size, Render_Buffer *render_buffer, Buffer_Type type) {
	VkBufferUsageFlagBits usage_type;
	switch (type) {
		case VERTEX_BUFFER: {
			usage_type = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			break;
		}

		case INDEX_BUFFER: {
			usage_type = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			break;
		}

		default: {
			return false;
		}
	}
	render_buffer->type = type;

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;

	bool result = create_buffer(static_cast<uint64_t>(size), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);
	if (!result) {
		return false;
	}

	void *data;
	vkMapMemory(c.device, staging_buffer_memory, 0, static_cast<uint64_t>(size), 0, &data);
	memcpy(data, elements, size);
	vkUnmapMemory(c.device, staging_buffer_memory);

	result = create_buffer(static_cast<uint64_t>(size), VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage_type, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, render_buffer->buffer, render_buffer->memory);
	if (!result) {
		return false;
	}

	result = copy_buffer(staging_buffer, render_buffer->buffer, size);
	if (!result) {
		return false;
	}

	vkDestroyBuffer(c.device, staging_buffer, 0);
	vkFreeMemory(c.device, staging_buffer_memory, 0);

	return true;
}

internal_function bool create_texture(const char *texture_data, int width, int height, int nr_channels, Texture *texture, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB) {
	uint count = texture_asset_list.count;

	//
	// create texture image and memory
	// 
	if (!texture_data) return false;

	VkDeviceSize image_size = width * height * nr_channels;

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;

	bool result = create_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);
	if (!result) return false;

	void *data;
	vkMapMemory(c.device, staging_buffer_memory, 0, image_size, 0, &data);
	memcpy(data, texture_data, static_cast<size_t>(image_size));
	vkUnmapMemory(c.device, staging_buffer_memory);

	result = create_image(width, height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture->image, texture->memory);
	if (!result) return false;

	result = transition_image_layout(texture->image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	if (!result) return false;

	result = copy_buffer_to_image(staging_buffer, texture->image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
	if (!result) return false;

	result = transition_image_layout(texture->image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	if (!result) return false;

	vkDestroyBuffer(c.device, staging_buffer, 0);
	vkFreeMemory(c.device, staging_buffer_memory, 0);

	//
	// create texture image view
	//
	VkImageViewCreateInfo view_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = texture->image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
	};
	VkResult vk_result = vkCreateImageView(c.device, &view_info, 0, &texture->image_view);
	if (vk_result != VK_SUCCESS) return false;

	//
	// create texture image sampler
	//
	VkSamplerCreateInfo sampler_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_NEAREST,
		.minFilter = VK_FILTER_NEAREST,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_FALSE,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};
	vk_result = vkCreateSampler(c.device, &sampler_info, 0, &texture->sampler);
	if (vk_result != VK_SUCCESS) return false;

	return true;
}

//
// Exported
//

bool create_texture_asset(const char *file_path, const Vertex *vertices, const uint *indices) {
	uint count = texture_asset_list.count;

	if (count == MAX_TEXTURE_BINDINGS) {
		return false;
	}
	
	int width, height, nr_channels;
	stbi_uc *pixels = stbi_load(file_path, &width, &height, &nr_channels, STBI_rgb_alpha);
	if (!pixels) {
		return false;
	}

	VkFormat format;
	switch (nr_channels) {
		case 1: {
			format = VK_FORMAT_R8_SRGB;
			break;
		}
		case 2: {
			format = VK_FORMAT_R8G8_SRGB;
			break;
		}
		case 3: {
			format = VK_FORMAT_R8G8B8_SRGB;
			break;
		}
		case 4: {
			format = VK_FORMAT_R8G8B8A8_SRGB;
			break;
		}
		default: {
			return false;
		}
	}

	//
	// create texture
	// 
	bool result = create_texture(reinterpret_cast<const char *>(pixels), width, height, nr_channels, &texture_asset_list.textures[count], format);
	if (!result) {
		return false;
	}

	//
	// create vertex buffer
	//
	result = create_render_buffer(vertices, 4 * sizeof(Vertex), &texture_asset_list.vertex_buffers[count], VERTEX_BUFFER);
	if (!result) {
		return false;
	}

	//
	// create index buffer
	//
	result = create_render_buffer(indices, 6 * sizeof(uint), &texture_asset_list.index_buffers[count], INDEX_BUFFER);
	if (!result) {
		return false;
	}

	++texture_asset_list.count;
	return true;
}

void delete_texture_asset(const char *label) {
	// @ToDo
}

Texture_Asset get_next_texture_asset(uint *index) {
	Texture_Asset texture_asset = {};
	texture_asset.texture = texture_asset_list.textures[*index];
	texture_asset.vertex_buffer = texture_asset_list.vertex_buffers[*index];
	texture_asset.index_buffer = texture_asset_list.index_buffers[*index];
	++(*index);
	return texture_asset;
}

uint get_texture_asset_count() {
	return texture_asset_list.count;
}
