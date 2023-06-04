#ifndef VULKAN_INIT_H
#define VULKAN_INIT_H

#include "types.hpp"
#include "math.hpp"

#include <vulkan/vulkan.hpp>

#include <vector>

constexpr uint MAX_FRAMES_IN_FLIGHT = 2;

enum Buffer_Type {
	VERTEX_BUFFER = 0,
	INDEX_BUFFER = 1,
};

struct Render_Buffer {
	Buffer_Type type;
	VkBuffer buffer;
	VkDeviceMemory memory;
};

struct Uniform_Buffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
	void *mapped;
};

struct Texture {
	VkImage image;
	VkDeviceMemory memory;
	VkImageView image_view;
	VkSampler sampler;
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
	VkRenderPass render_pass;
	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorPool descriptor_pool[2];
	VkDescriptorSet descriptor_sets[2][MAX_FRAMES_IN_FLIGHT];
	VkPipelineLayout pipeline_layout;
	VkPipeline graphics_pipeline;
	std::vector<VkFramebuffer> swapchain_framebuffers;
	VkCommandPool command_pool;
	VkCommandBuffer command_buffers[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore image_available_semaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore render_finished_semaphores[MAX_FRAMES_IN_FLIGHT];
	VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];
	uint32 current_frame = 0;
	Render_Buffer vertex_buffer[2];
	Render_Buffer index_buffer[2];
	Uniform_Buffer uniform_buffer;
	Texture texture[2];
};

struct Vertex {
	Vec2 pos;
	Vec2 tex_coord;
};

struct Uniform_Buffer_Object {
	Mat4 view;
	Mat4 proj;
};

struct Push_Constants {
	Mat4 model;
};

extern Global_Vulkan_Context c;

void cleanup_swapchain();
void create_swapchain();
void create_image_views();
void create_framebuffers();

#endif
