#include "pipeline.hpp"

#include "types.hpp"
#include "assets.hpp"
#include "platform.hpp"
#include "vulkan_init.hpp"

#include <vulkan/vulkan.h>

//
// INTERNAL
//

//struct Pipelines {
//	
//};

internal_function bool create_shader_module(const char *shader_file, VkShaderModule *shader_module) {
	File_Asset file_asset = {};
	uint32 size = platform_read_file(shader_file, &file_asset);
	if (size == 0) {
		platform_log("Failed to read shader file!\n");
		return false;
	}

	VkShaderModuleCreateInfo shader_module_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = size,
		.pCode = reinterpret_cast<const uint32_t *>(file_asset.data),
	};

	VkResult result = vkCreateShaderModule(c.device, &shader_module_info, 0, shader_module);
	platform_free_file(&file_asset);
	if (VK_SUCCESS != result) {
		return false;
	}

	return true;
}

//
// EXTERNAL
//

bool create_default_pipeline() {
	VkShaderModule vertex_shader_module = {};
	bool shader_created = create_shader_module("res/shaders/default_vs.spv", &vertex_shader_module);
	if (!shader_created) {
		return false;
	}

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertex_shader_module,
		.pName = "main"
	};

	VkShaderModule fragment_shader_module = {};
	shader_created = create_shader_module("res/shaders/default_fs.spv", &fragment_shader_module);
	if (!shader_created) {
		return false;
	}

	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragment_shader_module,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo shader_stage_create_infos[] = { vert_shader_stage_info, frag_shader_stage_info };

	VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamic_state_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = sizeof(shader_stage_create_infos) / sizeof(shader_stage_create_infos[0]),
		.pDynamicStates = dynamic_states
	};

	VkVertexInputBindingDescription binding_description = {
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
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
		//.pScissors = &scissors
	};

	VkPipelineRasterizationStateCreateInfo rasterization_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE, // NOTE: this could be used for shadow mapping
		.lineWidth = 1.0f
	};

	VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE
	};

	VkPipelineColorBlendAttachmentState color_blend_attachment = {
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo color_blending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &color_blend_attachment,
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
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

	VkResult result = vkCreatePipelineLayout(c.device, &pipeline_layout_info, 0, &c.pipeline_layout[0]);
	if (result != VK_SUCCESS) {
		platform_log("Fatal: Failed to create pipeline layout!\n");
		return false;
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
		.layout = c.pipeline_layout[0],
		.renderPass = c.main_pass,
		.subpass = 0
	};

	result = vkCreateGraphicsPipelines(c.device, VK_NULL_HANDLE, 1, &pipeline_info, 0, &c.graphics_pipeline[0]);
	if (result != VK_SUCCESS) {
		platform_log("Fatal: Failed to create graphics pipelines!\n");
		return false;
	}

	vkDestroyShaderModule(c.device, vertex_shader_module, 0);
	vkDestroyShaderModule(c.device, fragment_shader_module, 0);

	return true;
}

bool create_font_pipeline() {
	VkShaderModule font_vs_module = {};
	bool shader_created = create_shader_module("res/shaders/font_vs.spv", &font_vs_module);
	if (!shader_created) {
		platform_log("Fatal: Failed to create the font vertex shader module!\n");
		return GAME_FAILURE;
	}

	VkPipelineShaderStageCreateInfo font_vs_stage_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = font_vs_module,
		.pName = "main"
	};

	VkShaderModule font_fs_module = {};
	shader_created = create_shader_module("res/shaders/font_fs.spv", &font_fs_module);
	if (!shader_created) {
		platform_log("Fatal: Failed to create the font fragment shader module!\n");
		return GAME_FAILURE;
	}

	VkPipelineShaderStageCreateInfo font_fs_stage_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = font_fs_module,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo shader_stage_create_infos[] = { font_vs_stage_info, font_fs_stage_info };

	VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamic_state_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = sizeof(shader_stage_create_infos) / sizeof(shader_stage_create_infos[0]),
		.pDynamicStates = dynamic_states
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
		//.pScissors = &scissors
	};

	VkPipelineRasterizationStateCreateInfo rasterization_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.lineWidth = 1.0f
	};

	VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE
	};

	VkPipelineColorBlendAttachmentState color_blend_attachment = {
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo color_blending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &color_blend_attachment,
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
	};

	VkPushConstantRange ranges[] = {
		{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 0,
			.size = 64 // 4 by 4 matrix with 4 bytes per float
		},
		{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 64,
			.size = 8
		},
		{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset = 72,
			.size = 8
		}
	};

	VkPipelineLayoutCreateInfo pipeline_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &c.descriptor_set_layout,
		.pushConstantRangeCount = 3,
		.pPushConstantRanges = ranges
	};

	VkResult result = vkCreatePipelineLayout(c.device, &pipeline_layout_info, 0, &c.pipeline_layout[1]);
	if (result != VK_SUCCESS) {
		platform_log("Fatal: Failed to create pipeline layout!\n");
		return false;
	}

	VkGraphicsPipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = 2,
		.pStages = shader_stage_create_infos,
		.pVertexInputState = NULL,
		.pInputAssemblyState = &input_assembly_info,
		.pViewportState = &viewport_info,
		.pRasterizationState = &rasterization_info,
		.pMultisampleState = &multisampling,
		.pColorBlendState = &color_blending,
		.pDynamicState = &dynamic_state_info,
		.layout = c.pipeline_layout[1],
		.renderPass = c.main_pass,
		.subpass = 0
	};

	result = vkCreateGraphicsPipelines(c.device, VK_NULL_HANDLE, 1, &pipeline_info, 0, &c.graphics_pipeline[1]);
	if (result != VK_SUCCESS) {
		platform_log("Fatal: Failed to create graphics pipelines!\n");
		return false;
	}

	vkDestroyShaderModule(c.device, font_vs_module, 0);
	vkDestroyShaderModule(c.device, font_fs_module, 0);

	return true;
}
