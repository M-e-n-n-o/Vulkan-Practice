#include "SimpleRenderSystem.h"
#include <stdexcept>
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// Vulkan only guarantees 128bytes of space for the push constant (that is 2x mat4)
struct SimplePushConstantData
{
	glm::mat4 modelMatrix{ 1.0f };
	glm::mat4 normalMatrix{ 1.0f };
};

SimpleRenderSystem::SimpleRenderSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout): m_device(device)
{
	createPipelineLayout(globalSetLayout);
	createPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem()
{
	vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
}

void SimpleRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(SimplePushConstantData);

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{ globalSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	bool result = vkCreatePipelineLayout(m_device.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}
}

void SimpleRenderSystem::createPipeline(VkRenderPass renderPass)
{
	assert(m_pipelineLayout != nullptr && "Cannot create pupeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};
	Pipeline::DefaultPipelineConfigInfo(pipelineConfig);

	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = m_pipelineLayout;

	m_pipeline = std::make_unique<Pipeline>(m_device, "shaders/simple.vert.spv", "shaders/simple.frag.spv", pipelineConfig);
}

void SimpleRenderSystem::renderGameObjects(FrameInfo& frameInfo, std::vector<GameObject>& gameObjects)
{
	m_pipeline->bind(frameInfo.commandBuffer);

	vkCmdBindDescriptorSets(
		frameInfo.commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_pipelineLayout,
		0,
		1,
		&frameInfo.globalDescriptorSet,
		0,
		nullptr);

	for (auto& obj : gameObjects)
	{
		SimplePushConstantData push{};
		push.modelMatrix = obj.transform.getTransformationMatrix();
		push.normalMatrix = obj.transform.getNormalMatrix();

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			m_pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(SimplePushConstantData),
			&push);

		obj.model->bind(frameInfo.commandBuffer);
		obj.model->draw(frameInfo.commandBuffer);
	}
}
