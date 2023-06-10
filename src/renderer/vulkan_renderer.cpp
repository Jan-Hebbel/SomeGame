#include "renderer.hpp"

#include "renderer/vulkan_init.hpp"
#include "platform.hpp"
#include "math.hpp"

#include <vulkan/vulkan.h>

#include <assert.h>

void wait_for_current_frame_to_finish() {
	vkWaitForFences(c.device, 1, &c.in_flight_fences[c.current_frame], VK_TRUE, UINT64_MAX);
	vkResetFences(c.device, 1, &c.in_flight_fences[c.current_frame]);
}

void recreate_swapchain() {
	vkDeviceWaitIdle(c.device);

	cleanup_swapchain();

	create_swapchain();
	create_image_views();
	create_framebuffers();
}

VkCommandBuffer begin_render_pass(VkClearValue *clear_values, uint32_t image_index) {
	VkCommandBuffer command_buffer = c.command_buffers[c.current_frame];
	vkResetCommandBuffer(command_buffer, 0);

	VkCommandBufferBeginInfo begin_info{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	};

	VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info);
	if (VK_SUCCESS != result)
	{
		platform_log("Fatal: Failed to begin command buffer!\n");
		assert(result == VK_SUCCESS);
		return command_buffer;
	}

	VkRenderPassBeginInfo render_pass_info{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = c.render_pass,
		.framebuffer = c.swapchain_framebuffers[image_index],
		.renderArea = { {0, 0}, c.swapchain_image_extent },
		.clearValueCount = 1,
		.pClearValues = clear_values,
	};
	vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	return command_buffer;
}

void end_render_pass(VkCommandBuffer command_buffer) {
	vkCmdEndRenderPass(command_buffer);

	VkResult result = vkEndCommandBuffer(command_buffer);
	if (VK_SUCCESS != result)
	{
		platform_log("Fatal: Failed to end command buffer!\n");
		assert(VK_SUCCESS == result);
	}
}

void draw_game(Game_State *game_state, uint32_t image_index) {
	VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
	VkCommandBuffer command_buffer = begin_render_pass(&clear_color, image_index);

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

	// Draw background
	VkBuffer vertex_buffers2[] = { c.vertex_buffer[1].buffer };
	VkDeviceSize offsets2[] = { 0 };
	vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers2, offsets2);
	vkCmdBindIndexBuffer(command_buffer, c.index_buffer[1].buffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, c.pipeline_layout, 0, 1, &c.descriptor_sets[1][c.current_frame], 0, 0);
	Push_Constants constants = {};
	constants.model = identity();
	vkCmdPushConstants(command_buffer, c.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push_Constants), &constants);

	vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0); // @Hardcode: 6 == count of indices

	// Draw player
	VkBuffer vertex_buffers[] = { c.vertex_buffer[0].buffer };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(command_buffer, c.index_buffer[0].buffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, c.pipeline_layout, 0, 1, &c.descriptor_sets[0][c.current_frame], 0, 0); // holds uniforms (texture sampler, uniform buffers)
	constants.model = transpose(translate({ game_state->player.position.x, game_state->player.position.y, 0 }));
	vkCmdPushConstants(command_buffer, c.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push_Constants), &constants);

	vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0); // @Hardcode: 6 == count of indices

	end_render_pass(command_buffer);
}

void draw_menu(Game_State *game_state, uint32_t image_index) {
	VkClearValue clear_color{ {{0.05f, 0.3f, 0.3f, 1.0f}} };
	VkCommandBuffer command_buffer = begin_render_pass(&clear_color, image_index);
	
	// @ToDo: look at draw_game for comparison

	end_render_pass(command_buffer);
}

void draw_text(Vec2 top_left, uint font_height, const char *format, ...) {

}

void draw_performance_metrics(Game_State *game_state, uint32_t image_index) {
	VkClearValue clear_value = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	VkCommandBuffer command_buffer = begin_render_pass(&clear_value, image_index);

	draw_text({0.01f, 0.01f}, 10 /*pixel high*/, "%.2f fps", perf_metrics.fps);

	end_render_pass(command_buffer);
}

void game_render(Game_State *game_state)
{
	//
	// Don't render when game is minimized.
	//
	Window_Dimensions dimensions = {};
	platform_get_window_dimensions(&dimensions);
	if (dimensions.width == 0 || dimensions.height == 0) return;

	// 
	// Wait until current frame is not in use.
	//
	wait_for_current_frame_to_finish();

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
	// Draw. (Record command buffer that draws image.)
	//
	switch (game_state->mode) {
		case MODE_PLAY: {
			draw_game(game_state, image_index);
			break;
		}

		case MODE_MENU: {
			draw_menu(game_state, image_index);
			break;
		}

		default: {
			platform_log("Different Game Modes not yet supported!");
			break;
		}
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
