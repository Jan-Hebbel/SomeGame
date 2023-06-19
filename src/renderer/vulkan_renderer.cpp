#include "renderer.hpp"

#include "vulkan_init.hpp"
#include "platform.hpp"
#include "math.hpp"
#include "assets.hpp"
#include "fonts.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include <lib/stb_truetype.h>
#include <vulkan/vulkan.h>

#include <assert.h>
#include <stdarg.h>

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

VkCommandBuffer begin_render_pass(VkRenderPass render_pass, VkClearValue *clear_values, uint32_t image_index) {
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

	VkRenderPassBeginInfo render_pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = render_pass,
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

void draw_game(VkCommandBuffer command_buffer, Game_State *game_state) {
	uint index = 0;
	Texture_Asset texture_asset; 

	// Draw background
	texture_asset = get_next_texture_asset(&index);
	VkBuffer vertex_buffers2[] = { texture_asset.vertex_buffer.buffer };
	VkDeviceSize offsets2[] = { 0 };

	vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers2, offsets2);
	vkCmdBindIndexBuffer(command_buffer, texture_asset.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, c.pipeline_layout[0], 0, 1, &c.descriptor_sets[c.current_frame], 0, 0);
	Mat4 model = identity();
	vkCmdPushConstants(command_buffer, c.pipeline_layout[0], VK_SHADER_STAGE_VERTEX_BIT, 0, 64, &model);
	int texture_index = 0;
	vkCmdPushConstants(command_buffer, c.pipeline_layout[0], VK_SHADER_STAGE_FRAGMENT_BIT, 64, sizeof(int), &texture_index); 

	vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0); // @Hardcode: 6 == count of indices

	// Draw player
	texture_asset = get_next_texture_asset(&index);
	VkBuffer vertex_buffers[] = { texture_asset.vertex_buffer.buffer };
	VkDeviceSize offsets[] = { 0 };

	vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(command_buffer, texture_asset.index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, c.pipeline_layout[0], 0, 1, &c.descriptor_sets[c.current_frame], 0, 0); // holds uniforms (texture sampler, uniform buffers)
	model = transpose(translate({ game_state->player.position.x, game_state->player.position.y, 0 }));
	vkCmdPushConstants(command_buffer, c.pipeline_layout[0], VK_SHADER_STAGE_VERTEX_BIT, 0, 64, &model);
	texture_index = 1;
	vkCmdPushConstants(command_buffer, c.pipeline_layout[0], VK_SHADER_STAGE_FRAGMENT_BIT, 64, sizeof(int), &texture_index);

	vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0); // @Hardcode: 6 == count of indices
}

void draw_menu(VkCommandBuffer command_buffer) {
	// @ToDo
}

char *get_format_as_string(const char *format, ...) {
	va_list arg_ptr;
	va_start(arg_ptr, format);

	uint size = 1 + vsnprintf(0, 0, format, arg_ptr);
	char *out_message = new char[size];
	vsnprintf(out_message, static_cast<size_t>(size), format, arg_ptr);
	
	va_end(arg_ptr);
	
	return out_message;
}

void draw_text(VkCommandBuffer command_buffer, Vec2 top_left, const char *text) {
	stbtt_bakedchar *cdata = get_cdata();

	//char *ch = (char *)text;
	//while (*ch) {
	//	if (*ch >= 32 && *ch < 128) {
	//		stbtt_aligned_quad q;
	//		stbtt_GetBakedQuad(cdata, 512, 512, *ch - 32, &top_left.x, &top_left.y, &q, 1);
	//		//platform_log("%c", *ch); 

	//		VkBuffer vertex_buffers[] = { c.vertex_buffer[2].buffer };
	//		VkDeviceSize offsets[] = { 0 };

	//		vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
	//		vkCmdBindIndexBuffer(command_buffer, c.index_buffer[2].buffer, 0, VK_INDEX_TYPE_UINT16);
	//		vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, c.pipeline_layout, 0, 1, &c.descriptor_sets[c.current_frame], 0, 0); // holds uniforms (texture sampler, uniform buffers)
	//		Mat4 model = identity();
	//		vkCmdPushConstants(command_buffer, c.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 64, &model);
	//		int texture_index = 2;
	//		vkCmdPushConstants(command_buffer, c.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 64, sizeof(int), &texture_index);

	//		vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0); // @Hardcode: 6 == count of indices
	//	}
	//	++ch;
	//}
	//platform_log("\n");
}

void draw_performance_metrics(VkCommandBuffer command_buffer) {
	char *text = get_format_as_string("%.2f ms", perf_metrics.ms_per_frame);
	draw_text(command_buffer, {0.01f, 0.01f}, text);
	delete[] text;
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
	if (swapchain_outdated) {
		recreate_swapchain();
		swapchain_outdated = false;
	}

	//
	// Acquire an image from the swapchain.
	//
	uint32 image_index;
	VkResult result = vkAcquireNextImageKHR(c.device, c.swapchain, UINT64_MAX, c.image_available_semaphores[c.current_frame], VK_NULL_HANDLE, &image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		swapchain_outdated = true;
		return;
	}
	else if (result != VK_SUCCESS) {
		platform_log("Fatal: Failed to acquire the next image!\n");
		assert(VK_SUCCESS == result);
	}

	// 
	// Draw. (Record command buffer that draws image.)
	//
	VkClearValue clear_color = { {{0.05f, 0.3f, 0.3f, 1.0f}} };
	VkCommandBuffer command_buffer = begin_render_pass(c.main_pass, &clear_color, image_index);

	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, c.graphics_pipeline[0]);

	VkViewport viewport = {
		.x = 0.0f, .y = 0.0f,
		.width = static_cast<float>(c.swapchain_image_extent.width),
		.height = static_cast<float>(c.swapchain_image_extent.height),
		.minDepth = 0.0f, .maxDepth = 1.0f
	};
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	VkRect2D scissor = {
		.offset = { 0, 0 },
		.extent = c.swapchain_image_extent,
	};
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	switch (game_state->mode) {
		case MODE_PLAY: {
			draw_game(command_buffer, game_state);
			break;
		}

		case MODE_MENU: {
			draw_menu(command_buffer);
			break;
		}

		default: {
			platform_log("This mode is not recognized as a mode the game could be in!");
			break;
		}
	}

	//vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, c.graphics_pipeline[1]);
	draw_performance_metrics(command_buffer);

	end_render_pass(command_buffer);

	//
	// Submit the draw command.
	// 
	VkSemaphore wait_semaphores[] = { c.image_available_semaphores[c.current_frame] };
	VkSemaphore signal_semaphores[] = { c.render_finished_semaphores[c.current_frame] };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submit_info = {
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
	if (VK_SUCCESS != result) {
		platform_log("Fatal: Failed to submit to queue!\n");
		assert(VK_SUCCESS == result);
	}

	//
	// Present the image.
	//
	VkSwapchainKHR swapchains[] = { c.swapchain };
	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signal_semaphores,
		.swapchainCount = 1,
		.pSwapchains = swapchains,
		.pImageIndices = &image_index,
	};
	result = vkQueuePresentKHR(c.present_queue, &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		swapchain_outdated = true;
		return;
	}
	else if (result != VK_SUCCESS) {
		platform_log("Fatal: Failed to present!\n");
		assert(VK_SUCCESS == result);
	}

	c.current_frame = (c.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}
