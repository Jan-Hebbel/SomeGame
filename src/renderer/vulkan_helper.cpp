#include "vulkan_helper.hpp"

#include "renderer/vulkan_init.hpp"

#include <vulkan/vulkan.h>

VkCommandBuffer begin_single_time_commands() {
	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = c.command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(c.device, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	vkBeginCommandBuffer(command_buffer, &begin_info);

	return command_buffer;
}

void end_single_time_commands(VkCommandBuffer command_buffer) {
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &command_buffer,
	};
	vkQueueSubmit(c.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(c.graphics_queue); // @Speed @Performance; use vkWaitForFences

	vkFreeCommandBuffers(c.device, c.command_pool, 1, &command_buffer);
}

bool find_memory_type(uint32 type_filter, VkMemoryPropertyFlags property_flags, uint32 *index) {
	VkPhysicalDeviceMemoryProperties memory_properties = {};
	vkGetPhysicalDeviceMemoryProperties(c.physical_device, &memory_properties);

	for (uint32 i = 0; i < memory_properties.memoryTypeCount; ++i) {
		if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
			*index = i;
			return true;
		}
	}

	return false;
}

bool create_buffer(VkDeviceSize size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags property_flags, VkBuffer &buffer, VkDeviceMemory &memory) {
	VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage_flags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VkResult result = vkCreateBuffer(c.device, &buffer_info, 0, &buffer);
	if (result != VK_SUCCESS) {
		return false;
	}

	VkMemoryRequirements memory_requirements = {};
	vkGetBufferMemoryRequirements(c.device, buffer, &memory_requirements);
	uint32 memory_type_index;
	bool res = find_memory_type(memory_requirements.memoryTypeBits, property_flags, &memory_type_index);
	if (!res) {
		return false;
	}
	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = memory_type_index,
	};

	// NOTE: don't call vkAllocateMemory for every individual buffer, the number of allocations is limited; instead when allocating memory for a large number of objects, create a custom allocator that splits up a single allocation among many different objects by using the offset parameters
	// NOTE: maybe go a step further (as driver developers recommend): also store multiple buffers like the vertex buffer and index buffer into a single VkBuffer and use offsets in commands like vkCmdBindVertexBuffers => more cache friendly @Performance
	result = vkAllocateMemory(c.device, &alloc_info, 0, &memory);
	if (result != VK_SUCCESS) {
		return false;
	}

	vkBindBufferMemory(c.device, buffer, memory, 0);

	return true;
}

bool copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) {
	VkCommandBuffer command_buffer = begin_single_time_commands();

	VkBufferCopy copy_region = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size,
	};
	vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

	end_single_time_commands(command_buffer);

	return true;
}
