
/*
	TODO:
	* memory arena
	* display text to screen
		* load font with platform_load_file(...)
	* free everything 
	* take care of @Performance tags
*/

#define _CRT_SECURE_NO_WARNINGS

#if defined PLATFORM_WINDOWS
	#define VK_USE_PLATFORM_WIN32_KHR
	#include <windows.h>
	struct WindowHandles
	{
		HINSTANCE hinstance;
		HWND hwnd;
	};
#elif defined PLATFORM_LINUX
	// TODO: support linux
#elif defined PLATFORM_MACOS
	// TODO: support macos
#else 
	#error Unsupported Operating System!
#endif
#include <vulkan/vulkan.h>

#include "renderer.hpp"

#include "types.hpp"
#include "math.hpp"
#include "game.hpp"
#include "platform.hpp"
#include "renderer/vulkan_init.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <lib/stb_image.h>
#include <lib/stb_truetype.h>

#include <vector>
#include <string>
#include <cstring>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef _DEBUG
constexpr bool enable_validation_layers = true;
#elif defined NDEBUG
constexpr bool enable_validation_layers = false;
#else 
#error Unsrecognised build mode!
#endif

Global_Vulkan_Context c{};

struct QueueFamilyIndices {
	// fill this out as the need for more queue families arises
	std::optional<uint32> graphics_family;
	std::optional<uint32> present_family;
};

struct SwapchainDetails {
	std::vector<VkSurfaceFormatKHR> surface_formats;
	std::vector<VkPresentModeKHR> present_modes;
};

global_variable const uint16 indices[] = {
	0, 1, 2, 2, 3, 0
};

stbtt_bakedchar cdata[96];

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data, void *p_user_data)
{
	platform_log("%s\n", p_callback_data->pMessage);

	return VK_FALSE;
}

void cleanup_swapchain()
{
	for (size_t i = 0; i < c.swapchain_framebuffers.size(); ++i)
	{
		vkDestroyFramebuffer(c.device, c.swapchain_framebuffers[i], 0);
	}

	for (size_t i = 0; i < c.swapchain_image_views.size(); ++i)
	{
		vkDestroyImageView(c.device, c.swapchain_image_views[i], 0);
	}

	vkDestroySwapchainKHR(c.device, c.swapchain, 0);
}

SwapchainDetails get_swapchain_support_details(VkPhysicalDevice physical_device)
{
	SwapchainDetails swapchain_support{};
	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, c.surface, &format_count, 0);
	if (format_count > 0)
	{
		swapchain_support.surface_formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, c.surface, &format_count, swapchain_support.surface_formats.data());
	}
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, c.surface, &present_mode_count, 0);
	if (present_mode_count > 0)
	{
		swapchain_support.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, c.surface, &present_mode_count, swapchain_support.present_modes.data());
	}
	return swapchain_support;
}

QueueFamilyIndices get_queue_family_indices(VkPhysicalDevice physical_device)
{
	QueueFamilyIndices queue_family_indices{};

	uint32 queue_family_count;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, 0);

	VkQueueFamilyProperties *queue_family_properties_array = (VkQueueFamilyProperties *)malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
	if (queue_family_properties_array == NULL)
	{
		platform_log("Fatal: Failed to allocate memory for queue_family_properties_array!\n");
		return queue_family_indices;
	}
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties_array);

	for (uint i = 0; i < queue_family_count; ++i)
	{
		VkQueueFamilyProperties queue_family_properties = queue_family_properties_array[i];
		// NOTE: there might be a change in logic necessary if for some reason another queue family is better
		if (queue_family_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			queue_family_indices.graphics_family = i;
		}

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, c.surface, &present_support);
		if (present_support)
		{
			queue_family_indices.present_family = i;
		}

		// check if all indices have a value, if they do break the loop early
		if (queue_family_indices.graphics_family.has_value() && queue_family_indices.present_family.has_value())
		{
			break;
		}
	}

	free(queue_family_properties_array);
	return queue_family_indices;
}

void create_swapchain()
{
	SwapchainDetails swapchain_support = get_swapchain_support_details(c.physical_device);
	QueueFamilyIndices queue_family_indices = get_queue_family_indices(c.physical_device);

	VkSurfaceCapabilitiesKHR capabilities{};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(c.physical_device, c.surface, &capabilities);

	VkSwapchainCreateInfoKHR swapchain_info{};
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.surface = c.surface;
	// choose image count
	uint32 min_image_count = capabilities.minImageCount + 1; // this will most likely equal 3
	if (min_image_count > capabilities.maxImageCount && capabilities.maxImageCount != 0)
		min_image_count -= 1;
	swapchain_info.minImageCount = min_image_count;
	// choose surface format and color space
	for (const auto &surface_format : swapchain_support.surface_formats)
	{
		if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			swapchain_info.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
			swapchain_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		}
	}
	if (swapchain_info.imageFormat == 0)
	{
		swapchain_info.imageFormat = swapchain_support.surface_formats[0].format;
		swapchain_info.imageColorSpace = swapchain_support.surface_formats[0].colorSpace;
	}
	// choose image extent
	if (capabilities.currentExtent.width == 0xFFFFFFFF || capabilities.currentExtent.height == 0xFFFFFFFF)
	{
		Window_Dimensions dimensions = {};
		platform_get_window_dimensions(&dimensions);
		VkExtent2D actual_extent = {
			static_cast<uint32_t>(dimensions.width),
			static_cast<uint32_t>(dimensions.height),
		};
		actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		swapchain_info.imageExtent = actual_extent;
	}
	else
	{
		swapchain_info.imageExtent = capabilities.currentExtent;
	}
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	// choose a sharing mode dependent on if the queue families are actually the same
	if (queue_family_indices.graphics_family == queue_family_indices.present_family)
	{
		swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_info.queueFamilyIndexCount = 0;
		swapchain_info.pQueueFamilyIndices = 0;
	}
	else // queue_family_indices.graphics_family != queue_family_indices.present_family
	{
		swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_info.queueFamilyIndexCount = 2;
		uint32_t indices[] = { queue_family_indices.graphics_family.value(), queue_family_indices.present_family.value() };
		swapchain_info.pQueueFamilyIndices = indices;
	}
	swapchain_info.preTransform = capabilities.currentTransform;
	swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	// choose present mode
	for (const auto &present_mode : swapchain_support.present_modes)
	{
		if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			swapchain_info.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
	}
	if (swapchain_info.presentMode == 0)
	{
		swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	}
	// temporary; to check the maximum amount of fps you can run at
	swapchain_info.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	swapchain_info.clipped = VK_TRUE;
	swapchain_info.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(c.device, &swapchain_info, 0, &c.swapchain);
	if (VK_SUCCESS != result)
	{
		platform_log("Failed to create swapchain!\n");
		assert(VK_SUCCESS == result);
	}

	vkGetSwapchainImagesKHR(c.device, c.swapchain, &min_image_count, 0);
	c.swapchain_images.resize(min_image_count);
	vkGetSwapchainImagesKHR(c.device, c.swapchain, &min_image_count, c.swapchain_images.data());

	c.swapchain_image_format = swapchain_info.imageFormat;
	c.swapchain_image_extent = swapchain_info.imageExtent;
}

void create_image_views()
{
	c.swapchain_image_views.resize(c.swapchain_images.size());

	for (uint i = 0; i < c.swapchain_image_views.size(); ++i)
	{
		VkImageViewCreateInfo image_view_info{};
		image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_info.image = c.swapchain_images[i];
		image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_info.format = c.swapchain_image_format;
		image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_info.subresourceRange.baseMipLevel = 0;
		image_view_info.subresourceRange.levelCount = 1;
		image_view_info.subresourceRange.baseArrayLayer = 0;
		image_view_info.subresourceRange.layerCount = 1;

		VkResult result = vkCreateImageView(c.device, &image_view_info, 0, &c.swapchain_image_views[i]);
		if (VK_SUCCESS != result)
		{
			platform_log("Fatal: Failed to create image view!\n");
			assert(VK_SUCCESS == result);
		}
	}
}

void create_framebuffers() {
	c.swapchain_framebuffers.resize(c.swapchain_image_views.size());

	for (size_t i = 0; i < c.swapchain_image_views.size(); ++i) {
		VkImageView attachments[] = {
			c.swapchain_image_views[i],
		};

		VkFramebufferCreateInfo framebuffer_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = c.main_pass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = c.swapchain_image_extent.width,
			.height = c.swapchain_image_extent.height,
			.layers = 1,
		};

		VkResult result = vkCreateFramebuffer(c.device, &framebuffer_info, 0, &c.swapchain_framebuffers[i]);
		if (VK_SUCCESS != result) {
			platform_log("Fatal: Failed to create framebuffer!\n");
			assert(VK_SUCCESS == result);
		}
	}
}

bool find_memory_type(uint32 type_filter, VkMemoryPropertyFlags property_flags, uint32 *index)
{
	VkPhysicalDeviceMemoryProperties memory_properties{};
	vkGetPhysicalDeviceMemoryProperties(c.physical_device, &memory_properties);

	for (uint32 i = 0; i < memory_properties.memoryTypeCount; ++i)
	{
		if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags)
		{
			*index = i;
			return true;
		}
	}

	return false;
}

void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags property_flags, VkBuffer &buffer, VkDeviceMemory &memory)
{
	VkBufferCreateInfo buffer_info{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage_flags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VkResult result = vkCreateBuffer(c.device, &buffer_info, 0, &buffer);
	if (result != VK_SUCCESS)
	{
		platform_log("Fatal: Failed to create buffer!\n");
		assert(result == VK_SUCCESS);
	}

	VkMemoryRequirements memory_requirements{};
	vkGetBufferMemoryRequirements(c.device, buffer, &memory_requirements);
	uint32 memory_type_index;
	bool res = find_memory_type(memory_requirements.memoryTypeBits, property_flags, &memory_type_index);
	if (!res)
	{
		platform_log("Fatal: Failed to find a suitable memory type!\n");
		assert(result == VK_SUCCESS);
	}
	VkMemoryAllocateInfo alloc_info{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = memory_type_index,
	};

	// NOTE: don't call vkAllocateMemory for every individual buffer, the number of allocations is limited; instead when allocating memory for a large number of objects, create a custom allocator that splits up a single allocation among many different objects by using the offset parameters
	// NOTE: maybe go a step further (as driver developers recommend): also store multiple buffers like the vertex buffer and index buffer into a single VkBuffer and use offsets in commands like vkCmdBindVertexBuffers => more cache friendly @Performance
	result = vkAllocateMemory(c.device, &alloc_info, 0, &memory);
	if (result != VK_SUCCESS)
	{
		platform_log("Fatal: Failed to allocate memory!\n");
		assert(result == VK_SUCCESS);
	}

	vkBindBufferMemory(c.device, buffer, memory, 0);
}

VkCommandBuffer begin_single_time_commands()
{
	VkCommandBufferAllocateInfo alloc_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = c.command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(c.device, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	vkBeginCommandBuffer(command_buffer, &begin_info);

	return command_buffer;
}

void end_single_time_commands(VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &command_buffer,
	};
	vkQueueSubmit(c.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(c.graphics_queue); // @Speed @Performance; use vkWaitForFences

	vkFreeCommandBuffers(c.device, c.command_pool, 1, &command_buffer);
}

void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
	VkCommandBuffer command_buffer = begin_single_time_commands();

	VkBufferCopy copy_region{
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size,
	};
	vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

	end_single_time_commands(command_buffer);
}

void create_image(uint32 width, uint32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage_flags, VkMemoryPropertyFlags property_flags, VkImage &image, VkDeviceMemory &image_memory)
{
	VkImageCreateInfo image_info{
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
	if (result != VK_SUCCESS)
	{
		platform_log("Failed to create a vulkan image for the texture!\n");
		assert(result == VK_SUCCESS);
	}
	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(c.device, image, &memory_requirements);

	uint32_t index;
	bool res = find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &index);
	if (!res)
	{
		platform_log("Fatal: Failed to find suitable memory type!\n");
		assert(res);
	}
	VkMemoryAllocateInfo alloc_info{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = index,
	};

	result = vkAllocateMemory(c.device, &alloc_info, 0, &image_memory);
	if (result != VK_SUCCESS)
	{
		platform_log("Failed to allocate memory for the vulkan image!\n");
		assert(result == VK_SUCCESS);
	}

	vkBindImageMemory(c.device, image, image_memory, 0);
}

void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
{
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
	else
	{
		platform_log("Unsupported layout transition!\n");
		return; 
	}

	vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, 0, 0, 0, 1, &barrier);

	end_single_time_commands(command_buffer);
}

void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32 width, uint32 height) {
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
}

internal_function bool32 create_shader_module(const char *shader_file, VkShaderModule *shader_module) {
	File_Asset file_asset = {}; 
	uint32 size = platform_read_file(shader_file, &file_asset);
	if (size == 0) {
		platform_log("Failed to read shader file!\n");
		return GAME_FAILURE;
	}

	VkShaderModuleCreateInfo shader_module_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = size,
		.pCode = reinterpret_cast<const uint32_t *>(file_asset.data),
	};

	VkResult result = vkCreateShaderModule(c.device, &shader_module_info, 0, shader_module);
	platform_free_file(&file_asset);
	if (VK_SUCCESS != result) {
		return GAME_FAILURE;
	}

	return GAME_SUCCESS;
}

void create_render_buffer(const void *elements, size_t size, Render_Buffer *render_buffer, Buffer_Type type) {
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
			assert(render_buffer->type == VERTEX_BUFFER || render_buffer->type == INDEX_BUFFER);
			break;
		}
	}
	render_buffer->type = type;

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;

	create_buffer(static_cast<uint64_t>(size), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	void *data;
	vkMapMemory(c.device, staging_buffer_memory, 0, static_cast<uint64_t>(size), 0, &data);
	memcpy(data, elements, size);
	vkUnmapMemory(c.device, staging_buffer_memory);

	create_buffer(static_cast<uint64_t>(size), VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage_type, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, render_buffer->buffer, render_buffer->memory);

	copy_buffer(staging_buffer, render_buffer->buffer, size);

	vkDestroyBuffer(c.device, staging_buffer, 0);
	vkFreeMemory(c.device, staging_buffer_memory, 0);
}

bool create_texture_image(const char *file_path, int *width, int *height, int *nr_channels, Texture *texture, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB) {
	stbi_uc *pixels = stbi_load(file_path, width, height, nr_channels, STBI_rgb_alpha);
	if (!pixels) {
		return false;
	}

	int w = *width;
	int h = *height;

	VkDeviceSize image_size = w * h * 4;

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;

	create_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	void *data;
	vkMapMemory(c.device, staging_buffer_memory, 0, image_size, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(image_size));
	vkUnmapMemory(c.device, staging_buffer_memory);
	stbi_image_free(pixels);

	create_image(w, h, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture->image, texture->memory);

	transition_image_layout(texture->image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	copy_buffer_to_image(staging_buffer, texture->image, static_cast<uint32_t>(w), static_cast<uint32_t>(h));

	transition_image_layout(texture->image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(c.device, staging_buffer, 0);
	vkFreeMemory(c.device, staging_buffer_memory, 0);

	return true;
}

bool create_texture_image(const char *texture_data, int width, int height, int nrChannels, Texture *texture, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB) {
	if (!texture_data) {
		return false;
	}

	VkDeviceSize image_size = width * height * nrChannels;

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;

	create_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	void *data;
	vkMapMemory(c.device, staging_buffer_memory, 0, image_size, 0, &data);
	memcpy(data, texture_data, static_cast<size_t>(image_size));
	vkUnmapMemory(c.device, staging_buffer_memory);

	create_image(width, height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture->image, texture->memory);

	transition_image_layout(texture->image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	copy_buffer_to_image(staging_buffer, texture->image, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

	transition_image_layout(texture->image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(c.device, staging_buffer, 0);
	vkFreeMemory(c.device, staging_buffer_memory, 0);

	return true;
}

bool create_texture_image_view(Texture *texture, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB) {
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

bool create_texture_image_sampler(Texture *texture) {
	VkSamplerCreateInfo sampler_info{
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
	if (result != VK_SUCCESS)
	{
		platform_log("Fatal: Failed to create a sampler!\n");
		return false;
	}

	return true;
}

bool32 renderer_vulkan_init() {
	// debug callback: which messages are filtered and which are not
	VkDebugUtilsMessengerCreateInfoEXT messenger_info{};
	messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messenger_info.messageSeverity =
		//VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		//VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messenger_info.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messenger_info.pfnUserCallback = vulkan_debug_callback;

	const char *device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	uint device_extensions_size = sizeof(device_extensions) / sizeof(device_extensions[0]);

	//
	// create vulkan instance
	//
	{
		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "Game"; // TODO: real name
		app_info.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
		app_info.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo instance_info{};
		instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_info.pApplicationInfo = &app_info;

		// vulkan layers
		const char *layers[] = {
			"VK_LAYER_KHRONOS_validation",
		};
		const uint32 layer_count = sizeof(layers) / sizeof(layers[0]);

		const char *extensions[] = {
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		};
		uint32_t extension_count = 2;

		if (enable_validation_layers)
		{
			// check layer support
			bool layers_supported = false;
			uint32 supported_layer_count = 0;
			vkEnumerateInstanceLayerProperties(&supported_layer_count, 0);

			VkLayerProperties *layer_properties_dynamic_array = (VkLayerProperties *)malloc(supported_layer_count * sizeof(VkLayerProperties));
			if (!layer_properties_dynamic_array)
			{
				platform_log("Fatal: Failed to allocate memory for all layer properties!\n");
				return GAME_FAILURE;
			}
			vkEnumerateInstanceLayerProperties(&supported_layer_count, layer_properties_dynamic_array);

			for (uint i = 0; i < supported_layer_count; ++i)
			{
				for (uint j = 0; j < layer_count; ++j)
				{
					if (strcmp(layer_properties_dynamic_array[i].layerName, layers[j]) == 0)
					{
						layers_supported = true;
						break;
					}
				}

				if (layers_supported)
					break;
			}

			free(layer_properties_dynamic_array);

			if (layers_supported)
			{
				instance_info.enabledLayerCount = layer_count;
				instance_info.ppEnabledLayerNames = layers;

				instance_info.pNext = &messenger_info;

				++extension_count;
			}
			else
			{
				platform_log("Validation layers are to be enabled but validation layers are not supported! Continuing without validation layers.\n");
				instance_info.enabledLayerCount = 0;
				instance_info.ppEnabledLayerNames = 0;

				instance_info.pNext = 0;
			}
		}
		else
		{
			instance_info.enabledLayerCount = 0;
			instance_info.ppEnabledLayerNames = 0;

			instance_info.pNext = 0;
		}

		instance_info.enabledExtensionCount = extension_count;
		instance_info.ppEnabledExtensionNames = extensions;

		VkResult result = vkCreateInstance(&instance_info, 0, &c.instance);
		if (VK_SUCCESS != result)
		{
			platform_log("Fatal: Failed to create vulkan instance!\n");
			return GAME_FAILURE;
		}
	}

	//
	// create vulkan debug callback
	//
	{
		if (enable_validation_layers)
		{
			// load vkCreateDebugUtilsMessengerEXT
			auto createDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(c.instance, "vkCreateDebugUtilsMessengerEXT");

			if (createDebugUtilsMessenger)
			{
				// successfully loaded
				VkResult result = createDebugUtilsMessenger(c.instance, &messenger_info, 0, &c.debug_callback);
				if (result != VK_SUCCESS)
				{
					platform_log("Failed to create debug callback! Continuing without validation layers.\n");
				}
			}
			else 
			{
				platform_log("Failed to load the vkCreateDebugUtilsMessengerEXT function! Continuing without validation layers.\n");
			}
		}
	}

	//
	// create surface
	//
	{
		WindowHandles *window_handles = (WindowHandles *)platform_get_window_handles();

		VkWin32SurfaceCreateInfoKHR surface_info{};
		surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surface_info.hinstance = window_handles->hinstance;
		surface_info.hwnd = window_handles->hwnd;

		VkResult result = vkCreateWin32SurfaceKHR(c.instance, &surface_info, 0, &c.surface);
		if (result != VK_SUCCESS)
		{
			platform_log("Fatal: Failed to create a win32 surface!\n");
			return GAME_FAILURE;
		}
	}

	//
	// pick physical device
	//
	{
		uint32 physical_device_count = 0;
		vkEnumeratePhysicalDevices(c.instance, &physical_device_count, 0);

		VkPhysicalDevice *physical_devices = (VkPhysicalDevice *)malloc(physical_device_count * sizeof(VkPhysicalDevice));
		if (!physical_devices)
		{
			platform_log("Fatal: Failed to allocate memory for all physical devices!\n");
			return GAME_FAILURE;
		}
		vkEnumeratePhysicalDevices(c.instance, &physical_device_count, physical_devices);

		// going through all physical devices and picking the one that is suitable for our needs
		for (uint i = 0; i < physical_device_count; ++i)
		{
			VkPhysicalDevice physical_device = physical_devices[i];

			VkPhysicalDeviceProperties physical_device_properties;
			vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

			// find queue families for a physical device
			QueueFamilyIndices queue_family_indices = get_queue_family_indices(physical_device);
			bool queue_family_indices_is_complete = queue_family_indices.graphics_family.has_value() && queue_family_indices.present_family.has_value();

			// are my device extensions supported for this physical device?
			uint32 property_count;
			vkEnumerateDeviceExtensionProperties(physical_device, 0, &property_count, 0);
			
			VkExtensionProperties *available_device_extensions = (VkExtensionProperties *)malloc(sizeof(VkExtensionProperties) * property_count);
			if (!available_device_extensions)
			{
				platform_log("Fatal: Failed to allocate memory for VkExtensionProperties dynamic memory!\n");
				return GAME_FAILURE;
			}
			vkEnumerateDeviceExtensionProperties(physical_device, 0, &property_count, available_device_extensions);
			uint extensions_supported = 0;
			bool all_device_extensions_supported = false;
			for (uint i = 0; i < property_count; ++i)
			{
				if (0 == strcmp(device_extensions[extensions_supported], available_device_extensions[i].extensionName))
					++extensions_supported;

				if (extensions_supported == device_extensions_size)
				{
					all_device_extensions_supported = true;
					break;
				}
			}
			free(available_device_extensions);
			if (!all_device_extensions_supported)
			{
				platform_log("Fatal: Not all specified device extensions are supported!\n");
				return GAME_FAILURE;
			}

			// does this device support the swapchain i want to create?
			SwapchainDetails swapchain_support = get_swapchain_support_details(physical_device);
			bool swapchain_supported = !swapchain_support.surface_formats.empty() && !swapchain_support.present_modes.empty();
			
			// search for other devices if the current driver is not a dedicated gpu, doesnt support the required queue families, doesnt support the required device extensions
			// TODO: (potentially support multiple graphics cards)
			if (queue_family_indices_is_complete && physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && all_device_extensions_supported && swapchain_supported)
			{
				// picking physical device here
				c.physical_device = physical_device;
				break;
			}
		}

		free(physical_devices);

		if (c.physical_device == 0)
		{
			platform_error_message_window("Error!", "Your device has no suitable Vulkan driver!");
			return GAME_FAILURE;
		}
	}

	QueueFamilyIndices queue_family_indices = get_queue_family_indices(c.physical_device);

	//
	// create logical device
	//
	{
		// queues to be created with the logical device
		std::set<uint32_t> unique_queue_family_indices = { queue_family_indices.graphics_family.value(), queue_family_indices.present_family.value() };
		VkDeviceQueueCreateInfo *queue_infos = (VkDeviceQueueCreateInfo *)malloc(unique_queue_family_indices.size() * sizeof(VkDeviceQueueCreateInfo));
		if (!queue_infos)
		{
			platform_log("Fatal: Failed to allocate enough memory for all queue infos!\n");
			return GAME_FAILURE;
		}

		float queue_priority = 1.0f;
		uint i = 0;
		for (const auto &queue_family_index : unique_queue_family_indices)
		{
			VkDeviceQueueCreateInfo queue_info{};
			queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_info.queueFamilyIndex = queue_family_index;
			queue_info.queueCount = 1;
			queue_info.pQueuePriorities = &queue_priority;
			queue_infos[i] = queue_info;
			++i;
		}

		// device creation; unused for now
		VkPhysicalDeviceFeatures device_features{};

		VkDeviceCreateInfo device_info{};
		device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_info.queueCreateInfoCount = static_cast<uint32_t>(unique_queue_family_indices.size());
		device_info.pQueueCreateInfos = queue_infos;
		// TODO: set layer info for older version (since deprecated)?
		device_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions_size);
		device_info.ppEnabledExtensionNames = device_extensions;
		device_info.pEnabledFeatures = &device_features;

		VkResult result = vkCreateDevice(c.physical_device, &device_info, 0, &c.device);
		if (result != VK_SUCCESS)
		{
			platform_log("Fatal: Failed to create logical device!\n");
			return GAME_FAILURE;
		}

		free(queue_infos);

		// get handle to graphics queue; these are the same since we chose a queue family that can do both
		vkGetDeviceQueue(c.device, queue_family_indices.graphics_family.value(), 0, &c.graphics_queue);
		vkGetDeviceQueue(c.device, queue_family_indices.present_family.value(), 0, &c.present_queue);
	}

	//
	// create swap chain
	//
	{
		create_swapchain();
	}

	//
	// create image views
	//
	{
		create_image_views();
	}

	//
	// create render pass
	//
	{
		VkAttachmentDescription color_attachment_description = {
			.format = c.swapchain_image_format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		};

		VkAttachmentReference color_attachment_reference = {
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkSubpassDescription subpass = {
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment_reference,
		};

		VkSubpassDependency dependency = {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		};

		VkRenderPassCreateInfo render_pass_info = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &color_attachment_description,
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency,
		};

		VkResult result = vkCreateRenderPass(c.device, &render_pass_info, 0, &c.main_pass);
		if (result != VK_SUCCESS)
		{
			platform_log("Fatal: Failed to create main pass!\n");
			return GAME_FAILURE;
		}
	}

	//
	// create descriptor set layout
	//
	{
		VkDescriptorSetLayoutBinding ubo_layout_binding = {
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		};

		VkDescriptorSetLayoutBinding sampler_layout_binding = {
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = MAX_TEXTURE_BINDINGS,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		};

		VkDescriptorSetLayoutBinding bindings[] = { ubo_layout_binding, sampler_layout_binding };

		VkDescriptorSetLayoutCreateInfo layout_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = sizeof(bindings) / sizeof(bindings[0]),
			.pBindings = bindings,
		};

		VkResult result = vkCreateDescriptorSetLayout(c.device, &layout_info, 0, &c.descriptor_set_layout);
		if (VK_SUCCESS != result) {
			platform_log("Fatal: Failed to create descriptor set layout!\n");
			return GAME_FAILURE;
		}
	}

	//
	// create graphics pipeline
	//
	{
		bool32 shader_created = GAME_FAILURE;

		VkShaderModule vertex_shader_module = {}; 
		shader_created = create_shader_module("res/shaders/vert.spv", &vertex_shader_module);
		if (shader_created != GAME_SUCCESS)
		{
			platform_log("Fatal: Failed to create a vertex shader module!\n");
			return GAME_FAILURE;
		}

		VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
		vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_info.module = vertex_shader_module;
		vert_shader_stage_info.pName = "main";

		VkShaderModule fragment_shader_module = {};
		shader_created = create_shader_module("res/shaders/frag.spv", &fragment_shader_module);
		if (shader_created != GAME_SUCCESS)
		{
			platform_log("Fatal: Failed to create a vertex shader module!\n");
			return GAME_FAILURE;
		}

		VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
		frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_stage_info.module = fragment_shader_module;
		frag_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo shader_stage_create_infos[] = { vert_shader_stage_info, frag_shader_stage_info };

		VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
		dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state_info.dynamicStateCount = sizeof(shader_stage_create_infos) / sizeof(shader_stage_create_infos[0]);
		dynamic_state_info.pDynamicStates = dynamic_states;

		VkVertexInputBindingDescription binding_description{
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};

		VkVertexInputAttributeDescription attribute_descriptions[] = {
			{
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offsetof(Vertex, pos)
			},
			{
				.location = 1,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offsetof(Vertex, tex_coord)
			}
		};

		VkPipelineVertexInputStateCreateInfo vertex_input_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &binding_description,
			.vertexAttributeDescriptionCount = 2,
			.pVertexAttributeDescriptions = attribute_descriptions
		};

		VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
		};

		// NOTE: we are using dynamic state, so we need to specify viewport and scissor at drawing time, instead of here

		VkPipelineViewportStateCreateInfo viewport_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			//.pViewports = &viewport,
			.scissorCount = 1,
			//.pScissors = &scissors,
		};

		VkPipelineRasterizationStateCreateInfo rasterization_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.depthBiasEnable = VK_FALSE, // NOTE: this could be used for shadow mapping
			.lineWidth = 1.0f,
		};

		VkPipelineMultisampleStateCreateInfo multisampling = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
		};

		VkPipelineColorBlendAttachmentState color_blend_attachment = {
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};

		VkPipelineColorBlendStateCreateInfo color_blending = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &color_blend_attachment,
			.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
		};

		VkPushConstantRange ranges[] = {
			{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
				.offset = 0,
				.size = 64 // 4 by 4 matrix with 4 bytes per float
			},
			{
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 64,
				.size = sizeof(int)
			}
		};

		VkPipelineLayoutCreateInfo pipeline_layout_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &c.descriptor_set_layout,
			.pushConstantRangeCount = 2,
			.pPushConstantRanges = ranges
		};

		VkResult result = vkCreatePipelineLayout(c.device, &pipeline_layout_info, 0, &c.pipeline_layout);
		if (result != VK_SUCCESS)
		{
			platform_log("Fatal: Failed to create pipeline layout!\n");
			return GAME_FAILURE;
		}

		VkGraphicsPipelineCreateInfo pipeline_info = {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = 2,
			.pStages = shader_stage_create_infos,
			.pVertexInputState = &vertex_input_info,
			.pInputAssemblyState = &input_assembly_info,
			.pViewportState = &viewport_info,
			.pRasterizationState = &rasterization_info,
			.pMultisampleState = &multisampling,
			.pColorBlendState = &color_blending,
			.pDynamicState = &dynamic_state_info,
			.layout = c.pipeline_layout,
			.renderPass = c.main_pass,
			.subpass = 0,
		};

		result = vkCreateGraphicsPipelines(c.device, VK_NULL_HANDLE, 1, &pipeline_info, 0, &c.graphics_pipeline);
		if (result != VK_SUCCESS)
		{
			platform_log("Fatal: Failed to create graphics pipelines!\n");
			return GAME_FAILURE;
		}

		vkDestroyShaderModule(c.device, vertex_shader_module, 0);
		vkDestroyShaderModule(c.device, fragment_shader_module, 0);
	}

	//
	// create framebuffers
	//
	{
		create_framebuffers();
	}

	//
	// create command pool
	//
	{
		VkCommandPoolCreateInfo pool_info{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = queue_family_indices.graphics_family.value(),
		};

		VkResult result = vkCreateCommandPool(c.device, &pool_info, 0, &c.command_pool);
		if (result != VK_SUCCESS)
		{
			platform_log("Fatal: Failed to create command pool!\n");
			return GAME_FAILURE;
		}
	}

	//
	// create texture image (and memory)
	//
	{
		int width, height, nr_channels;
		bool result;

		// Player
		result = create_texture_image("res/textures/option2.png", &width, &height, &nr_channels, &c.texture[0]);
		if (!result) {
			platform_log("Fatal: Failed to create texture image!\n");
			return GAME_FAILURE;
		}

		// Background
		result = create_texture_image("res/textures/grass.png", &width, &height, &nr_channels, &c.texture[1]);
		if (!result) {
			platform_log("Fatal: Failed to create texture image!\n");
			return GAME_FAILURE;
		}

		// Font 
		const int bitmap_w = 512;
		const int bitmap_h = 512;
		unsigned char *temp_bitmap = new unsigned char[bitmap_w * bitmap_h];

		const char *font = "C:/Dev/MyGame/res/fonts/SourceCodePro-Regular.ttf";

		File_Asset font_asset = {};
		uint32 bytes_read = platform_read_file(font, &font_asset);
		if (bytes_read == 0) {
			platform_log("Fatal: Failed to read the font file!\n");
			return GAME_FAILURE;
		}
		int chars_fit = stbtt_BakeFontBitmap(reinterpret_cast<const unsigned char*>(font_asset.data), 0, 32.0f, temp_bitmap, bitmap_w, bitmap_h, 32, 96, cdata);
		platform_free_file(&font_asset);
		if (chars_fit <= 0) {
			platform_log("Fatal: No or not all characters fit into the bitmap.\n");
			return GAME_FAILURE;
		}

		result = create_texture_image(reinterpret_cast<char *>(temp_bitmap), bitmap_w, bitmap_h, 1, &c.texture[2], VK_FORMAT_R8_SRGB);
		if (!result) {
			platform_log("Fatal: Line %d: Failed to create texture image!\n", __LINE__);
			return GAME_FAILURE;
		}

		delete[] temp_bitmap;
	}

	//
	// create texture image view
	//
	{
		bool result; 

		// Player
		result = create_texture_image_view(&c.texture[0]);
		if (!result) {
			platform_log("Failed to create a texture image view!\n");
		}

		// Background
		result = create_texture_image_view(&c.texture[1]);
		if (!result) {
			platform_log("Failed to create a texture image view!\n");
		}

		// Font
		result = create_texture_image_view(&c.texture[2], VK_FORMAT_R8_SRGB);
		if (!result) {
			platform_log("Failed to create a texture image view!\n");
		}
	}

	//
	// create texture sampler
	//
	{
		bool result;

		result = create_texture_image_sampler(&c.texture[0]); // Player
		if (!result) {
			platform_log("Failed to create a texture image sampler!\n");
		}

		result = create_texture_image_sampler(&c.texture[1]); // Background
		if (!result) {
			platform_log("Failed to create a texture image sampler!\n");
		}

		result = create_texture_image_sampler(&c.texture[2]);
		if (!result) {
			platform_log("Failed to create a texture image sampler!\n");
		}
	}

	//
	// create vertex buffer
	//
	{
		// Player
		const Vertex vertices[] = {
			{.pos = {-0.5f, -0.5f}, .tex_coord = {0.0f, 0.0f}},
			{.pos = { 0.5f, -0.5f}, .tex_coord = {1.0f, 0.0f}},
			{.pos = { 0.5f,  0.5f}, .tex_coord = {1.0f, 1.0f}},
			{.pos = {-0.5f,  0.5f}, .tex_coord = {0.0f, 1.0f}},
		};

		create_render_buffer(vertices, sizeof(vertices), &c.vertex_buffer[0], VERTEX_BUFFER);

		// Background
		const Vertex bg_vertices[] = {
			{.pos = {-6.0f, -6.0f}, .tex_coord = {0.0f, 0.0f}},
			{.pos = { 6.0f, -6.0f}, .tex_coord = {6.0f, 0.0f}},
			{.pos = { 6.0f,  6.0f}, .tex_coord = {6.0f, 6.0f}},
			{.pos = {-6.0f,  6.0f}, .tex_coord = {0.0f, 6.0f}},
		};

		create_render_buffer(bg_vertices, sizeof(bg_vertices), &c.vertex_buffer[1], VERTEX_BUFFER);

		// Font 
		const Vertex font_verts[] = {
			{.pos = {-1.0f, -1.0f}, .tex_coord = {0.0f, 0.0f}},
			{.pos = { 1.0f, -1.0f}, .tex_coord = {1.0f, 0.0f}},
			{.pos = { 1.0f,  1.0f}, .tex_coord = {1.0f, 1.0f}},
			{.pos = {-1.0f,  1.0f}, .tex_coord = {0.0f, 1.0f}}
		};

		create_render_buffer(font_verts, sizeof(font_verts), &c.vertex_buffer[2], VERTEX_BUFFER);
	}

	//
	// create index buffer
	//
	{
		// Player
		create_render_buffer(indices, sizeof(indices), &c.index_buffer[0], INDEX_BUFFER);

		// Background
		create_render_buffer(indices, sizeof(indices), &c.index_buffer[1], INDEX_BUFFER);

		// Font 
		create_render_buffer(indices, sizeof(indices), &c.index_buffer[2], INDEX_BUFFER);
	}

	//
	// create uniform buffer
	//
	{
		float scale = 3.0f;
		float aspect = (float)c.swapchain_image_extent.width / (float)c.swapchain_image_extent.height;
		Uniform_Buffer_Object ubo = {
			.view = identity(),
			.proj = transpose(orthographic_projection(-aspect * scale, aspect * scale, -scale, scale, 0.1f, 2.0f)),
		};
		size_t ubo_size = sizeof(ubo);
		Uniform_Buffer *uniform = &c.uniform_buffer;

		// creating uniform buffer here (takes ubo, ubo_size, uniform as parameters)
		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;
		create_buffer(ubo_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

		void *data;
		vkMapMemory(c.device, staging_buffer_memory, 0, static_cast<uint64_t>(ubo_size), 0, &data);
		memcpy(data, &ubo, ubo_size);
		vkUnmapMemory(c.device, staging_buffer_memory);

		create_buffer(ubo_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uniform->buffer, uniform->memory);

		copy_buffer(staging_buffer, uniform->buffer, ubo_size);

		vkDestroyBuffer(c.device, staging_buffer, 0);
		vkFreeMemory(c.device, staging_buffer_memory, 0);
	}

	//
	// create descriptor pool
	//
	{
		VkDescriptorPoolSize pool_sizes[] = {
			{
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
			},
			{
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
			}
		};

		VkDescriptorPoolCreateInfo pool_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
			.poolSizeCount = sizeof(pool_sizes) / sizeof(pool_sizes[0]),
			.pPoolSizes = pool_sizes,
		};

		VkResult result = vkCreateDescriptorPool(c.device, &pool_info, 0, &c.descriptor_pool);
		if (VK_SUCCESS != result)
		{
			platform_log("Fatal: Failed to create descriptor pool!\n");
			return GAME_FAILURE;
		}
	}

	//
	// create descriptor sets
	//
	{
		VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT];
		for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			layouts[i] = c.descriptor_set_layout;
		}
		VkDescriptorSetAllocateInfo alloc_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = c.descriptor_pool,
			.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
			.pSetLayouts = layouts,
		};

		VkResult result = vkAllocateDescriptorSets(c.device, &alloc_info, c.descriptor_sets);
		if (VK_SUCCESS != result) {
			platform_log("Fatal: Failed to allocate descriptor sets!\n");
			return GAME_FAILURE;
		}

		VkDescriptorImageInfo image_infos[NUMBER_OF_ASSETS];
		for (int j = 0; j < NUMBER_OF_ASSETS; ++j) {
			assert(j < MAX_TEXTURE_BINDINGS);
			image_infos[j].sampler = c.texture[j].sampler;
			image_infos[j].imageView = c.texture[j].image_view;
			image_infos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		};

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			VkDescriptorBufferInfo buffer_info = {
				.buffer = c.uniform_buffer.buffer,
				.offset = 0,
				.range = sizeof(Uniform_Buffer_Object),
			};

			VkWriteDescriptorSet descriptor_writes[] = {
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = c.descriptor_sets[i],
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.pBufferInfo = &buffer_info,
				},
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = c.descriptor_sets[i],
					.dstBinding = 1,
					.dstArrayElement = 0,
					.descriptorCount = NUMBER_OF_ASSETS,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = image_infos,
				}
			};
			vkUpdateDescriptorSets(c.device, sizeof(descriptor_writes) / sizeof(descriptor_writes[0]), descriptor_writes, 0, 0);
		}
	}

	//
	// create command buffers
	//
	{
		VkCommandBufferAllocateInfo allocate_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = c.command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = MAX_FRAMES_IN_FLIGHT, // count of command_buffer
		};

		VkResult result = vkAllocateCommandBuffers(c.device, &allocate_info, c.command_buffers);
		if (result != VK_SUCCESS) {
			platform_log("Fatal: Failed to allocate command buffers!\n");
			return GAME_FAILURE;
		}
	}

	//
	// create synchronization objects
	//
	{
		VkSemaphoreCreateInfo semaphore_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};

		VkFenceCreateInfo fence_info = {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			if (VK_SUCCESS != vkCreateSemaphore(c.device, &semaphore_info, 0, &c.image_available_semaphores[i]) ||
				VK_SUCCESS != vkCreateSemaphore(c.device, &semaphore_info, 0, &c.render_finished_semaphores[i]) ||
				VK_SUCCESS != vkCreateFence(c.device, &fence_info, 0, &c.in_flight_fences[i]))
			{
				platform_log("Fatal: Failed to create synchronization objects!\n");
				return GAME_FAILURE;
			}
		}
	}

	return GAME_SUCCESS;
}

void renderer_vulkan_cleanup()
{
	// TODO
	cleanup_swapchain();
}

void renderer_vulkan_wait_idle()
{
	vkDeviceWaitIdle(c.device);
}
