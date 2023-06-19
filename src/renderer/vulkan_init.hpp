#ifndef VULKAN_INIT_H
#define VULKAN_INIT_H

#include "types.hpp"
#include "math.hpp"

#include <vulkan/vulkan.hpp>

#include <vector>

constexpr uint MAX_FRAMES_IN_FLIGHT = 2;
constexpr uint MAX_TEXTURE_BINDINGS = 2;

struct Uniform_Buffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
	void *mapped;
};

struct Global_Vulkan_Context {
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
	VkRenderPass main_pass;
	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSet descriptor_sets[MAX_FRAMES_IN_FLIGHT];
	VkPipelineLayout pipeline_layout[2];
	VkPipeline graphics_pipeline[2];
	std::vector<VkFramebuffer> swapchain_framebuffers;
	VkCommandPool command_pool;
	VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore render_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
	VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];
	uint32 current_frame = 0;
	Uniform_Buffer uniform_buffer;
};

struct Uniform_Buffer_Object {
	Mat4 view;
	Mat4 proj;
};

extern Global_Vulkan_Context c;

void cleanup_swapchain();
void create_swapchain();
void create_image_views();
void create_framebuffers();

#endif
