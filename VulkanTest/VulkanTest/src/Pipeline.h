#pragma once

#include <string>
#include <vector>

#include "Device.h"

struct PipelineConfigInfo
{
	PipelineConfigInfo() = default;
	PipelineConfigInfo(const PipelineConfigInfo&) = delete;
	PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

	VkPipelineViewportStateCreateInfo viewportInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo multisampleInfo;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	std::vector<VkDynamicState> dynamicStateEnables;
	VkPipelineDynamicStateCreateInfo dynamicStateInfo;
	VkPipelineLayout pipelineLayout = nullptr;
	VkRenderPass renderPass = nullptr;
	uint32_t subpass = 0;
};

class Pipeline
{
private:
	Device& m_device;
	VkPipeline m_graphicsPipeline{};
	VkShaderModule m_vertShaderModule{};
	VkShaderModule m_fragShaderModule{};

public:
	Pipeline() = default;
	Pipeline(Device& device, const std::string& vertexFilePath, const std::string& fragmentFilePath, const PipelineConfigInfo& configInfo);
	~Pipeline();

	Pipeline(const Pipeline&) = delete;
	Pipeline& operator=(const Pipeline&) = delete;

	void bind(VkCommandBuffer commandBuffer);

	static void DefaultPipelineConfigInfo(PipelineConfigInfo& configInfo);

private:
	void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

	static std::vector<char> ReadFile(const std::string& filePath);
};