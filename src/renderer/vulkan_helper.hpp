#ifndef VULKAN_HELPER_H
#define VULKAN_HELPER_H

#include "types.hpp"

#include <vulkan/vulkan.h>

VkCommandBuffer begin_single_time_commands();
void end_single_time_commands(VkCommandBuffer command_buffer);

bool find_memory_type(uint32 type_filter, VkMemoryPropertyFlags property_flags, uint32 *index);

bool create_buffer(VkDeviceSize size, VkBufferUsageFlags usage_flags, VkMemoryPropertyFlags property_flags, VkBuffer &buffer, VkDeviceMemory &memory);
bool copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);

#endif
