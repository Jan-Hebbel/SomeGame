#include "pch.hpp"

#include "game/game.hpp"
#include "game/game_vulkan.hpp"
#include "platform/platform.hpp"

// TODO: other operating systems
#ifdef PLATFORM_WINDOWS
// TODO: this cpp file includes windows.h; maybe there is another way of getting things up and running without including windows.h in this file
#include "game/game_vulkan_surface_win32.cpp"
#endif

#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <cstring>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>

#ifdef _DEBUG
constexpr bool enable_validation_layers = true;
#elif defined NDEBUG
constexpr bool enable_validation_layers = false;
#else 
#error Unsrecognised build mode!
#endif

struct GameVulkanContext
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
	VkPipelineLayout pipeline_layout;
	VkPipeline graphics_pipeline;
	std::vector<VkFramebuffer> swapchain_framebuffers;
	VkCommandPool command_pool;
	VkCommandBuffer command_buffer;
};

global_variable GameVulkanContext context{};

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

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data, void *p_user_data)
{
	platform_log("%s\n\r", p_callback_data->pMessage);

	return VK_FALSE;
}

internal_function std::vector<char> read_file(const std::string &filename)
{
	std::vector<char> buffer;

	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		return buffer;
	}

	size_t file_size = (size_t)file.tellg();
	buffer.resize(file_size);
	file.seekg(0);
	file.read(buffer.data(), file_size);
	file.close();

	return buffer;
}

bool32 game_vulkan_init()
{
	// vulkan extensions
	std::vector<const char *> extensions = {
		VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
	};

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

	QueueFamilyIndices queue_family_indices;

	std::vector<const char *> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	SwapchainDetails swapchain_support;



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

		if (enable_validation_layers)
		{
			// check layer support
			bool layers_supported = false;
			uint32 supported_layer_count = 0;
			vkEnumerateInstanceLayerProperties(&supported_layer_count, 0);

			// TODO: memory arena
			std::vector<VkLayerProperties> layer_properties_dynamic_array(supported_layer_count);
			vkEnumerateInstanceLayerProperties(&supported_layer_count, layer_properties_dynamic_array.data());

			for (const auto &layer_properties : layer_properties_dynamic_array)
			{
				for (uint i = 0; i < layer_count; ++i)
				{
					if (std::strcmp(layer_properties.layerName, layers[i]) == 0)
					{
						layers_supported = true;
						break;
					}
				}

				if (layers_supported)
					break;
			}

			if (layers_supported)
			{
				instance_info.enabledLayerCount = layer_count;
				instance_info.ppEnabledLayerNames = layers;

				instance_info.pNext = &messenger_info;

				extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}
			else
			{
				// TODO: Logging (enable_validation_layers is true but validation layers not supported; continuing without validation layers)
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

		instance_info.enabledExtensionCount = (uint32)extensions.size();
		instance_info.ppEnabledExtensionNames = extensions.data();

		if (vkCreateInstance(&instance_info, 0, &context.instance))
		{
			// TODO: Logging (Failed to create vulkan instance! Exiting Game!)
			return GAME_FAILURE;
		}
		// TODO: Logging (successfully created vulkan instance)
	}



	// create vulkan debug callback
	{
		if (enable_validation_layers)
		{
			// load vkCreateDebugUtilsMessengerEXT
			auto createDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");

			if (createDebugUtilsMessenger)
			{
				// successfully loaded
				VkResult result = createDebugUtilsMessenger(context.instance, &messenger_info, 0, &context.debug_callback);

				if (result == VK_SUCCESS)
				{
					// TODO: Logging (successfully created debug callback)
				}
				else
				{
					// TODO: Logging (Failed to create debug callback; proceeding without it)
				}
			}
			else 
			{
				// TODO: Logging (couldn't load vkCreateDebugUtilsMessengerEXT; proceeding without it)
			}
		}
	}



	// create surface
	{
		bool32 result = vulkan_surface_create(context.instance, &context.surface);
		if (!result)
		{
			// TODO: Logging (successfully created vulkan surface!)
			
		}
		else
		{
			// TODO: Logging (failed to create vulkan surface!)
			return GAME_FAILURE;
		}
	}



	// pick physical device
	{
		uint32 physical_device_count = 0;
		vkEnumeratePhysicalDevices(context.instance, &physical_device_count, 0);

		// TODO: vector?
		std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
		vkEnumeratePhysicalDevices(context.instance, &physical_device_count, physical_devices.data());

		// going through all physical devices and picking the one that is suitable for our needs
		for (const auto &physical_device : physical_devices)
		{
			VkPhysicalDeviceProperties physical_device_properties;
			vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

			// find queue families for a physical device
			uint32 queue_family_count;
			vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, 0);

			// TODO: vector?
			std::vector<VkQueueFamilyProperties> queue_family_properties_array(queue_family_count);
			vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties_array.data());

			int i = 0;
			bool queue_family_indices_is_complete = false;
			for (const auto &queue_family_properties : queue_family_properties_array)
			{
				// NOTE: there might be a change in logic necessary if for some reason another queue family is better
				if (queue_family_properties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					queue_family_indices.graphics_family = i;
				}

				VkBool32 present_support = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, context.surface, &present_support);
				if (present_support)
				{
					queue_family_indices.present_family = i;
				}

				// check if all indices have a value, if they do break the loop early
				if (queue_family_indices.graphics_family.has_value() && queue_family_indices.present_family.has_value())
				{
					queue_family_indices_is_complete = true;
					break;
				}

				++i;
			}

			// are my device extensions supported for this physical device?
			uint32 property_count;
			vkEnumerateDeviceExtensionProperties(physical_device, 0, &property_count, 0);
			// TODO: vector?
			std::vector<VkExtensionProperties> available_device_extensions(property_count);
			vkEnumerateDeviceExtensionProperties(physical_device, 0, &property_count, available_device_extensions.data());
			std::set<std::string> required_device_extensions(device_extensions.begin(), device_extensions.end());
			for (const auto &extension : available_device_extensions)
			{
				required_device_extensions.erase(extension.extensionName);

				if (required_device_extensions.empty()) break;
			}

			// does this device support the swapchain i want to create?
			uint32_t format_count;
			vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, context.surface, &format_count, 0);
			if (format_count > 0)
			{
				swapchain_support.surface_formats.resize(format_count);
				vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, context.surface, &format_count, swapchain_support.surface_formats.data());
			}
			uint32_t present_mode_count;
			vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, context.surface, &present_mode_count, 0);
			if (present_mode_count > 0)
			{
				swapchain_support.present_modes.resize(present_mode_count);
				vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, context.surface, &present_mode_count, swapchain_support.present_modes.data());
			}
			bool swapchain_supported = !swapchain_support.surface_formats.empty() && !swapchain_support.present_modes.empty();
			
			// search for other devices if the current driver is not a dedicated gpu, doesnt support the required queue families, doesnt support the required device extensions
			// TODO: (potentially support multiple graphics cards)
			if (queue_family_indices_is_complete && physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && required_device_extensions.empty() && swapchain_supported)
			{
				// picking physical device here
				context.physical_device = physical_device;
				break;
			}
		}

		if (context.physical_device == 0)
		{
			platform_error_message_window("Error!", "Your device has no suitable Vulkan driver!");
			return GAME_FAILURE;
		}

		// TODO: Log the picked physical device
	}



	// create logical device
	{
		// queues to be created with the logical device
		std::vector<VkDeviceQueueCreateInfo> queue_infos{};
		std::set<uint32_t> unique_queue_family_indices = { queue_family_indices.graphics_family.value(), queue_family_indices.present_family.value() };

		float queue_priority = 1.0f;
		for (const auto &queue_family_index : unique_queue_family_indices)
		{
			VkDeviceQueueCreateInfo queue_info{};
			queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_info.queueFamilyIndex = queue_family_index;
			queue_info.queueCount = 1;
			queue_info.pQueuePriorities = &queue_priority;
			queue_infos.push_back(queue_info);
		}

		// device creation; unused for now
		VkPhysicalDeviceFeatures device_features{};

		VkDeviceCreateInfo device_info{};
		device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
		device_info.pQueueCreateInfos = queue_infos.data();
		// TODO: set layer info for older version (since deprecated)?
		device_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
		device_info.ppEnabledExtensionNames = device_extensions.data();
		device_info.pEnabledFeatures = &device_features;

		VkResult result = vkCreateDevice(context.physical_device, &device_info, 0, &context.device);
		if (result != VK_SUCCESS)
		{
			// TODO: log failure
			return GAME_FAILURE;
		}

		// get handle to graphics queue; these are the same since we chose a queue family that can do both
		vkGetDeviceQueue(context.device, queue_family_indices.graphics_family.value(), 0, &context.graphics_queue);
		vkGetDeviceQueue(context.device, queue_family_indices.present_family.value(), 0, &context.present_queue);
		
		// TODO: log success
	}



	// create swap chain
	{
		VkSurfaceCapabilitiesKHR capabilities{};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physical_device, context.surface, &capabilities);

		VkSwapchainCreateInfoKHR swapchain_info{};
		swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchain_info.surface = context.surface;
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
			uint width, height;
			platform_get_window_dimensions(&width, &height);
			VkExtent2D actual_extent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height),
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

		VkResult result = vkCreateSwapchainKHR(context.device, &swapchain_info, 0, &context.swapchain);
		if (result != VK_SUCCESS)
		{
			// TODO: log failure
			return GAME_FAILURE;
		}

		vkGetSwapchainImagesKHR(context.device, context.swapchain, &min_image_count, 0);
		context.swapchain_images.resize(min_image_count);
		vkGetSwapchainImagesKHR(context.device, context.swapchain, &min_image_count, context.swapchain_images.data());

		context.swapchain_image_format = swapchain_info.imageFormat;
		context.swapchain_image_extent = swapchain_info.imageExtent;

		// TODO: log success
	}



	// create image views
	{
		context.swapchain_image_views.resize(context.swapchain_images.size());

		for (uint i = 0; i < context.swapchain_image_views.size(); ++i)
		{
			VkImageViewCreateInfo image_view_info{};
			image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			image_view_info.image = context.swapchain_images[i];
			image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			image_view_info.format = context.swapchain_image_format;
			image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_view_info.subresourceRange.baseMipLevel = 0;
			image_view_info.subresourceRange.levelCount = 1;
			image_view_info.subresourceRange.baseArrayLayer = 0;
			image_view_info.subresourceRange.layerCount = 1;

			VkResult result = vkCreateImageView(context.device, &image_view_info, 0, &context.swapchain_image_views[i]);
			if (result != VK_SUCCESS)
			{
				// TODO: log failure
				return GAME_FAILURE;
			}

			// TODO: log success
		}
	}



	// create render pass
	{
		VkAttachmentDescription color_attachment_description{
			.format = context.swapchain_image_format,
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

		VkRenderPassCreateInfo render_pass_info{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &color_attachment_description,
			.subpassCount = 1,
			.pSubpasses = &subpass,
		};

		VkResult result = vkCreateRenderPass(context.device, &render_pass_info, 0, &context.render_pass);
		if (result != VK_SUCCESS)
		{
			// TODO: log failure
			return GAME_FAILURE;
		}

		// TODO: log success
	}

	

	// create graphics pipeline
	{
		auto vert_shader_code = read_file("res/shaders/vert.spv");
		auto frag_shader_code = read_file("res/shaders/frag.spv");

		// wrap vertex shader code in VkShaderModule object
		VkShaderModuleCreateInfo vertex_shader_module_info{};
		vertex_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vertex_shader_module_info.codeSize = vert_shader_code.size();
		vertex_shader_module_info.pCode = reinterpret_cast<const uint32_t *>(vert_shader_code.data());

		VkShaderModule vertex_shader_module;
		vkCreateShaderModule(context.device, &vertex_shader_module_info, 0, &vertex_shader_module);

		VkShaderModuleCreateInfo fragment_shader_module_info{};
		fragment_shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		fragment_shader_module_info.codeSize = frag_shader_code.size();
		fragment_shader_module_info.pCode = reinterpret_cast<const uint32_t *>(frag_shader_code.data());

		VkShaderModule fragment_shader_module;
		vkCreateShaderModule(context.device, &fragment_shader_module_info, 0, &fragment_shader_module);

		// Do something with the shader modules
		VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
		vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_info.module = vertex_shader_module;
		vert_shader_stage_info.pName = "main";

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

		VkPipelineVertexInputStateCreateInfo vertex_input_info{};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 0;
		vertex_input_info.vertexAttributeDescriptionCount = 0;

		VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
		input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly_info.primitiveRestartEnable = VK_FALSE;

		// NOTE: we are using dynamic state, so we need to specify viewport and scissor at drawing time, instead of here
		//VkViewport viewport{ 
		//	.x = 0.0f, 
		//	.y = 0.0f, 
		//	.width = context.swapchain_image_extent.width, 
		//	.height = context.swapchain_image_extent.height, 
		//	.minDepth = 0.0f, 
		//	.maxDepth = 1.0f
		//};

		//VkRect2D scissor{
		//	.offset = { 0, 0 },
		//	.extent = context.swapchain_image_extent
		//};

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
		};

		VkResult result = vkCreatePipelineLayout(context.device, &pipeline_layout_info, 0, &context.pipeline_layout);
		if (result != VK_SUCCESS)
		{
			// TODO: log failure
		}

		// TODO: log success

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
			.layout = context.pipeline_layout,
			.renderPass = context.render_pass,
			.subpass = 0,
		};

		result = vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipeline_info, 0, &context.graphics_pipeline);
		if (result != VK_SUCCESS)
		{
			// TODO: log failure
			return GAME_FAILURE;
		}

		// TODO: log success

		vkDestroyShaderModule(context.device, vertex_shader_module, 0);
		vkDestroyShaderModule(context.device, fragment_shader_module, 0);
	}



	// create framebuffers
	{
		context.swapchain_framebuffers.resize(context.swapchain_image_views.size());

		for (size_t i = 0; i < context.swapchain_image_views.size(); ++i)
		{
			VkImageView attachments[] = {
				context.swapchain_image_views[i],
			};

			VkFramebufferCreateInfo framebuffer_info{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = context.render_pass,
				.attachmentCount = 1,
				.pAttachments = attachments,
				.width = context.swapchain_image_extent.width,
				.height = context.swapchain_image_extent.height,
				.layers = 1,
			};

			VkResult result = vkCreateFramebuffer(context.device, &framebuffer_info, 0, context.swapchain_framebuffers.data());
			if (result != VK_SUCCESS)
			{
				// TODO: log failure
				return GAME_FAILURE;
			}

			// TODO: log success
		}
	}



	// create command pool
	{
		VkCommandPoolCreateInfo pool_info{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = queue_family_indices.graphics_family.value(),
		};

		VkResult result = vkCreateCommandPool(context.device, &pool_info, 0, &context.command_pool);
		if (result != VK_SUCCESS)
		{
			// TODO: log failure
			return GAME_FAILURE;
		}

		// TODO: log success
	}



	// create command buffer
	{
		VkCommandBufferAllocateInfo allocate_info{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = context.command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		VkResult result = vkAllocateCommandBuffers(context.device, &allocate_info, &context.command_buffer);
		if (result != VK_SUCCESS)
		{
			// TODO: log failure
			return GAME_FAILURE;
		}

		// TODO: log success
	}




	{

	}

	return GAME_SUCCESS;
}

void game_vulkan_cleanup()
{
	// TODO
}
