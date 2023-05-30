
/*
	TODO:
	* memory arena
	* display text to screen
		* load font with platform_load_file(...)
	* free everything 
	* take care of @Performance tags
*/

#define _CRT_SECURE_NO_WARNINGS

#include "types.hpp"
#include "math.hpp"
#include "game.hpp"
#include "renderer/vulkan_draw.hpp"
#include "platform/platform.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include <lib/stb_truetype.h>
#define STB_IMAGE_IMPLEMENTATION
#include <lib/stb_image.h>

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

global_variable constexpr uint MAX_FRAMES_IN_FLIGHT = 2;

enum Buffer_Type {
	VERTEX_BUFFER = 0,
	INDEX_BUFFER = 1,
};

struct Render_Buffer {
	Buffer_Type type;
	VkBuffer buffer;
	VkDeviceMemory memory;
};

struct Uniform_Buffers {
	VkBuffer buffer[MAX_FRAMES_IN_FLIGHT];
	VkDeviceMemory memory[MAX_FRAMES_IN_FLIGHT];
	void *mapped[MAX_FRAMES_IN_FLIGHT];
};

struct Texture {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView image_view;
	VkSampler sampler;
};

struct Global_Vulkan_Context
{
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_callback;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkQueue graphics_queue;
	VkQueue present_queue;
	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchain_images;
	VkFormat swapchain_image_format;
	VkExtent2D swapchain_image_extent;
	std::vector<VkImageView> swapchain_image_views;
	VkRenderPass render_pass;
	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSet descriptor_sets[MAX_FRAMES_IN_FLIGHT];
	VkPipelineLayout pipeline_layout;
	VkPipeline graphics_pipeline;
	std::vector<VkFramebuffer> swapchain_framebuffers;
	VkCommandPool command_pool;
	std::vector<VkCommandBuffer> command_buffers;
	std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkSemaphore> render_finished_semaphores;
	std::vector<VkFence> in_flight_fences;
	uint32 current_frame = 0;
	Render_Buffer vertex_buffer[2];
	Render_Buffer index_buffer[2];
	Uniform_Buffers uniform_buffers[2];
	Texture texture[2];
};

global_variable Global_Vulkan_Context c{};

// TODO: temporary solution
global_variable unsigned char ttf_buffer[1 << 20];
global_variable unsigned char font_bitmap[1024 * 1024];
global_variable stbtt_bakedchar data[96];

struct QueueFamilyIndices
{
	// fill this out as the need for more queue families arises
	std::optional<uint32> graphics_family;
	std::optional<uint32> present_family;
};

struct SwapchainDetails
{
	std::vector<VkSurfaceFormatKHR> surface_formats;
	std::vector<VkPresentModeKHR> present_modes;
};

struct Vertex
{
	Vec2 pos;
	Vec2 tex_coord;
};

struct Uniform_Buffer_Object
{
	Mat4 model;
	Mat4 view;
	Mat4 proj;
};

global_variable const uint16 indices[] = {
	0, 1, 2, 2, 3, 0
};

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

void create_framebuffers()
{
	c.swapchain_framebuffers.resize(c.swapchain_image_views.size());

	for (size_t i = 0; i < c.swapchain_image_views.size(); ++i)
	{
		VkImageView attachments[] = {
			c.swapchain_image_views[i],
		};

		VkFramebufferCreateInfo framebuffer_info{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = c.render_pass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = c.swapchain_image_extent.width,
			.height = c.swapchain_image_extent.height,
			.layers = 1,
		};

		VkResult result = vkCreateFramebuffer(c.device, &framebuffer_info, 0, &c.swapchain_framebuffers[i]);
		if (VK_SUCCESS != result)
		{
			platform_log("Fatal: Failed to create framebuffer!\n");
			assert(VK_SUCCESS == result);
		}
	}
}

void recreate_swapchain()
{
	vkDeviceWaitIdle(c.device);

	cleanup_swapchain();

	create_swapchain();
	create_image_views();
	create_framebuffers();
}

void draw_frame(Game_State *game_state)
{
	// 
	// Wait for the previous frame to finish.
	//
	vkWaitForFences(c.device, 1, &c.in_flight_fences[c.current_frame], VK_TRUE, UINT64_MAX);
	vkResetFences(c.device, 1, &c.in_flight_fences[c.current_frame]);

	//
	// Recreate the swapchain if the swapchain is outdated (resizing or minimizing window).
	//
	static bool swapchain_outdated = false;
	if (swapchain_outdated)
	{
		recreate_swapchain();
	}

	//
	// Acquire an image from the swapchain.
	//
	uint32 image_index;
	VkResult result = vkAcquireNextImageKHR(c.device, c.swapchain, UINT64_MAX, c.image_available_semaphores[c.current_frame], VK_NULL_HANDLE, &image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		swapchain_outdated = true;
		return;
	}
	else if (result != VK_SUCCESS)
	{
		platform_log("Fatal: Failed to acquire the next image!\n");
		assert(VK_SUCCESS == result);
	}

	//
	// Update Uniform Buffers. (@Performance: Most efficient way to pass a frequently changing small amount of data to the shader are push constants)
	//
	float scale = 3.0f;
	float aspect = (float)c.swapchain_image_extent.width / (float)c.swapchain_image_extent.height;
	Uniform_Buffer_Object ubo{
		.model = transpose(translate({game_state->player.position.x, game_state->player.position.y, 0})),
		.view = identity(),
		// NOTE: Transposing here because my math library stores matrices in row major notation.
		.proj = transpose(orthographic_projection(-aspect * scale, aspect * scale, -scale, scale, 0.1f, 2.0f)),
	};
	memcpy(c.uniform_buffers[0].mapped[c.current_frame], &ubo, sizeof(ubo));

	// 
	// Draw. (Record command buffer that draws image.)
	//
	VkCommandBuffer command_buffer = c.command_buffers[c.current_frame];
	vkResetCommandBuffer(command_buffer, 0);

	VkCommandBufferBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};

	result = vkBeginCommandBuffer(command_buffer, &begin_info);
	if (VK_SUCCESS != result)
	{
		platform_log("Fatal: Failed to begin command buffer!\n");
		assert(result == VK_SUCCESS);
	}

	VkClearValue clear_color{ {{0.0f, 0.0f, 0.0f, 1.0f}} };

	VkRenderPassBeginInfo render_pass_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = c.render_pass,
		.framebuffer = c.swapchain_framebuffers[image_index],
		.renderArea = { {0, 0}, c.swapchain_image_extent },
		.clearValueCount = 1,
		.pClearValues = &clear_color,
	};

	vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, c.graphics_pipeline);

	VkViewport viewport{
		.x = 0.0f, .y = 0.0f,
		.width = static_cast<float>(c.swapchain_image_extent.width),
		.height = static_cast<float>(c.swapchain_image_extent.height),
		.minDepth = 0.0f, .maxDepth = 1.0f
	};
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = c.swapchain_image_extent,
	};
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	VkBuffer vertex_buffers[] = { c.vertex_buffer[0].buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(command_buffer, c.index_buffer[0].buffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, c.pipeline_layout, 0, 1, &c.descriptor_sets[c.current_frame], 0, 0);

	vkCmdDrawIndexed(command_buffer, sizeof(indices) / sizeof(indices[0]), 1, 0, 0, 0);

	vkCmdEndRenderPass(command_buffer);

	result = vkEndCommandBuffer(command_buffer);
	if (VK_SUCCESS != result)
	{
		platform_log("Fatal: Failed to end command buffer!\n");
		assert(VK_SUCCESS == result);
	}

	//
	// Submit the draw command.
	// 
	VkSemaphore wait_semaphores[] = { c.image_available_semaphores[c.current_frame] };
	VkSemaphore signal_semaphores[] = { c.render_finished_semaphores[c.current_frame] };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submit_info{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = wait_semaphores,
		.pWaitDstStageMask = wait_stages,
		.commandBufferCount = 1,
		.pCommandBuffers = &c.command_buffers[c.current_frame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signal_semaphores
	};
	result = vkQueueSubmit(c.graphics_queue, 1, &submit_info, c.in_flight_fences[c.current_frame]);
	if (VK_SUCCESS != result)
	{
		platform_log("Fatal: Failed to submit to queue!\n");
		assert(VK_SUCCESS == result);
	}

	//
	// Present the image.
	//
	VkSwapchainKHR swapchains[] = { c.swapchain };
	VkPresentInfoKHR present_info{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signal_semaphores,
		.swapchainCount = 1,
		.pSwapchains = swapchains,
		.pImageIndices = &image_index,
	};
	result = vkQueuePresentKHR(c.present_queue, &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		swapchain_outdated = true;
		return;
	}
	else if (result != VK_SUCCESS)
	{
		platform_log("Fatal: Failed to present!\n");
		assert(VK_SUCCESS == result);
	}

	c.current_frame = (c.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
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

	VkImageMemoryBarrier barrier{
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
	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
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

void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32 width, uint32 height)
{
	VkCommandBuffer command_buffer = begin_single_time_commands();

	VkBufferImageCopy region{
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

internal_function bool32 create_shader_module(const char *shader_file, VkShaderModule *shader_module)
{
	uint32 size;
	File_Asset *shader_code = platform_read_file(shader_file, &size);
	if (!shader_code)
	{
		return GAME_FAILURE;
	}

	VkShaderModuleCreateInfo shader_module_info{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = size,
		.pCode = reinterpret_cast<const uint32_t *>(shader_code),
	};

	VkResult result = vkCreateShaderModule(c.device, &shader_module_info, 0, shader_module);
	platform_free_file(shader_code);
	if (VK_SUCCESS != result)
	{
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

bool create_texture_image(const char *file_path, int *width, int *height, int *nr_channels, Texture *texture) {
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

	create_image(w, h, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture->image, texture->memory);

	transition_image_layout(texture->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	copy_buffer_to_image(staging_buffer, texture->image, static_cast<uint32_t>(w), static_cast<uint32_t>(h));

	transition_image_layout(texture->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(c.device, staging_buffer, 0);
	vkFreeMemory(c.device, staging_buffer_memory, 0);

	return true;
}

bool32 renderer_vulkan_init()
{
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



	// create vulkan instance
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



	// create vulkan debug callback
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



	// create surface
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



	// pick physical device
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



	// create logical device
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



	// create swap chain
	{
		create_swapchain();
	}



	// create image views
	{
		create_image_views();
	}



	// create render pass
	{
		VkAttachmentDescription color_attachment_description{
			.format = c.swapchain_image_format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		};

		VkAttachmentReference color_attachment_reference{
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkSubpassDescription subpass{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment_reference,
		};

		VkSubpassDependency dependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		};

		VkRenderPassCreateInfo render_pass_info{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &color_attachment_description,
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency,
		};

		VkResult result = vkCreateRenderPass(c.device, &render_pass_info, 0, &c.render_pass);
		if (result != VK_SUCCESS)
		{
			platform_log("Fatal: Failed to create render pass!\n");
			return GAME_FAILURE;
		}
	}

	

	// create descriptor set layout
	{
		VkDescriptorSetLayoutBinding ubo_layout_binding{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		};

		VkDescriptorSetLayoutBinding sampler_layout_binding{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		};

		VkDescriptorSetLayoutBinding bindings[] = { ubo_layout_binding, sampler_layout_binding };

		VkDescriptorSetLayoutCreateInfo layout_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = sizeof(bindings) / sizeof(bindings[0]),
			.pBindings = bindings,
		};

		VkResult result = vkCreateDescriptorSetLayout(c.device, &layout_info, 0, &c.descriptor_set_layout);
		if (VK_SUCCESS != result)
		{
			platform_log("Fatal: Failed to create descriptor set layout!\n");
			return GAME_FAILURE;
		}
	}



	// create graphics pipeline
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

		VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
		frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_stage_info.module = fragment_shader_module;
		frag_shader_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo shader_stage_create_infos[] = { vert_shader_stage_info, frag_shader_stage_info };

		VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamic_state_info{};
		dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state_info.dynamicStateCount = sizeof(shader_stage_create_infos) / sizeof(shader_stage_create_infos[0]);
		dynamic_state_info.pDynamicStates = dynamic_states;

		VkVertexInputBindingDescription binding_description{
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};
		VkVertexInputAttributeDescription attribute_descriptions[2]{};
		attribute_descriptions[0].binding = 0;
		attribute_descriptions[0].location = 0;
		attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_descriptions[0].offset = offsetof(Vertex, pos);
		attribute_descriptions[1].binding = 0;
		attribute_descriptions[1].location = 1;
		attribute_descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attribute_descriptions[1].offset = offsetof(Vertex, tex_coord);
		VkPipelineVertexInputStateCreateInfo vertex_input_info{};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.vertexAttributeDescriptionCount = 2;
		vertex_input_info.pVertexBindingDescriptions = &binding_description;
		vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions;

		VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
		input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly_info.primitiveRestartEnable = VK_FALSE;

		// NOTE: we are using dynamic state, so we need to specify viewport and scissor at drawing time, instead of here

		VkPipelineViewportStateCreateInfo viewport_info{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			//.pViewports = &viewport,
			.scissorCount = 1,
			//.pScissors = &scissors,
		};

		VkPipelineRasterizationStateCreateInfo rasterization_info{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.depthBiasEnable = VK_FALSE, // NOTE: this could be used for shadow mapping
			.lineWidth = 1.0f,
		};

		VkPipelineMultisampleStateCreateInfo multisampling{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
		};

		VkPipelineColorBlendAttachmentState color_blend_attachment{
			.blendEnable = VK_FALSE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};

		VkPipelineColorBlendStateCreateInfo color_blending{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &color_blend_attachment,
			.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f },
		};

		VkPipelineLayoutCreateInfo pipeline_layout_info{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &c.descriptor_set_layout,
		};

		VkResult result = vkCreatePipelineLayout(c.device, &pipeline_layout_info, 0, &c.pipeline_layout);
		if (result != VK_SUCCESS)
		{
			platform_log("Fatal: Failed to create pipeline layout!\n");
			return GAME_FAILURE;
		}

		VkGraphicsPipelineCreateInfo pipeline_info{
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
			.renderPass = c.render_pass,
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



	// create framebuffers
	{
		create_framebuffers();
	}



	// create command pool
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



	// create texture image
	{
		int width, height, nr_channels;
		bool result;

		// Player
		result = create_texture_image("res/textures/knight.png", &width, &height, &nr_channels, &c.texture[0]);
		if (!result) {
			platform_log("Failed to create texture image!\n");
		}

		// Background
		result = create_texture_image("res/textures/grass.png", &width, &height, &nr_channels, &c.texture[1]);
		if (!result) {
			platform_log("Failed to create texture image!\n");
		}
	}



	// create texture image view
	{
		VkImageViewCreateInfo view_info{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = c.texture[0].image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VK_FORMAT_R8G8B8A8_SRGB,
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
		};
		VkResult result = vkCreateImageView(c.device, &view_info, 0, &c.texture[0].image_view);
		if (result != VK_SUCCESS)
		{
			platform_log("Fatal: Failed to create texture image views!\n");
			return GAME_FAILURE;
		}
	}



	// create texture sampler
	{
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
		VkResult result = vkCreateSampler(c.device, &sampler_info, 0, &c.texture[0].sampler);
		if (result != VK_SUCCESS)
		{
			platform_log("Fatal: Failed to create a sampler!\n");
			return GAME_FAILURE;
		}
	}

	//
	// create vertex buffer
	//
	{
		// Player
		const Vertex vertices[] = {
			{.pos = {-0.6f, -0.5f}, .tex_coord = {0.0f, 0.0f}},
			{.pos = { 0.6f, -0.5f}, .tex_coord = {1.0f, 0.0f}},
			{.pos = { 0.6f,  0.5f}, .tex_coord = {1.0f, 1.0f}},
			{.pos = {-0.6f,  0.5f}, .tex_coord = {0.0f, 1.0f}},
		};

		create_render_buffer(vertices, sizeof(vertices), &c.vertex_buffer[0], VERTEX_BUFFER);

		// Background
		const Vertex bg_vertices[] = {
			{.pos = {-1.0f, -1.0f}, .tex_coord = {0.0f, 0.0f}},
			{.pos = { 1.0f, -1.0f}, .tex_coord = {1.0f, 0.0f}},
			{.pos = { 1.0f,  1.0f}, .tex_coord = {1.0f, 1.0f}},
			{.pos = {-1.0f,  1.0f}, .tex_coord = {0.0f, 1.0f}},
		};

		create_render_buffer(bg_vertices, sizeof(bg_vertices), &c.vertex_buffer[1], VERTEX_BUFFER);
	}

	//
	// create index buffer
	//
	{
		// Player
		create_render_buffer(indices, sizeof(indices), &c.index_buffer[0], INDEX_BUFFER);

		// Background
		create_render_buffer(indices, sizeof(indices), &c.index_buffer[1], INDEX_BUFFER);
	}



	// create uniform buffers
	{
		VkDeviceSize buffer_size = sizeof(Uniform_Buffer_Object);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			create_buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, c.uniform_buffers[0].buffer[i], c.uniform_buffers[0].memory[i]);

			vkMapMemory(c.device, c.uniform_buffers[0].memory[i], 0, buffer_size, 0, &c.uniform_buffers[0].mapped[i]);
		}
	}



	// create descriptor pool
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



	// create descriptor sets
	{
		VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT];
		for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			layouts[i] = c.descriptor_set_layout;
		}
		VkDescriptorSetAllocateInfo alloc_info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = c.descriptor_pool,
			.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
			.pSetLayouts = layouts,
		};

		VkResult result = vkAllocateDescriptorSets(c.device, &alloc_info, c.descriptor_sets);
		if (VK_SUCCESS != result)
		{
			platform_log("Fatal: Failed to allocate descriptor sets!\n");
			return GAME_FAILURE;
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			VkDescriptorBufferInfo buffer_info{
				.buffer = c.uniform_buffers[0].buffer[i],
				.offset = 0,
				.range = sizeof(Uniform_Buffer_Object),
			};
			VkDescriptorImageInfo image_info{
				.sampler = c.texture[0].sampler,
				.imageView = c.texture[0].image_view,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
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
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &image_info,
				},
			};
			vkUpdateDescriptorSets(c.device, sizeof(descriptor_writes) / sizeof(descriptor_writes[0]), descriptor_writes, 0, 0);
		}
	}



	// create command buffers
	{
		c.command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocate_info{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = c.command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = static_cast<uint32_t>(c.command_buffers.size()),
		};

		VkResult result = vkAllocateCommandBuffers(c.device, &allocate_info, c.command_buffers.data());
		if (result != VK_SUCCESS)
		{
			platform_log("Fatal: Failed to allocate command buffers!\n");
			return GAME_FAILURE;
		}
	}



	// create synchronization objects
	{
		c.image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		c.render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		c.in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphore_info{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};

		VkFenceCreateInfo fence_info{
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
