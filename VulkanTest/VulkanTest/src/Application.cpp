#include "Application.h"
#include <stdexcept>

Application::Application()
{
	createPipelineLayout();
	createPipeline();
	createCommandBuffers();
}

Application::~Application()
{
	vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
}

void Application::run()
{
	while (!m_window.shouldClose())
	{
		m_window.update();
	}
}

void Application::createPipelineLayout()
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	bool result = vkCreatePipelineLayout(m_device.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}
}

void Application::createPipeline()
{
	//auto pipelineConfig = Pipeline::DefaultPipelineConfigInfo(m_swapChain.width(), m_swapChain.height());

	PipelineConfigInfo pipelineConfig{};
	Pipeline::DefaultPipelineConfigInfo(pipelineConfig, m_swapChain.width(), m_swapChain.height());

	pipelineConfig.renderPass = m_swapChain.getRenderPass();
	pipelineConfig.pipelineLayout = m_pipelineLayout;

	m_pipeline = std::make_unique<Pipeline>(m_device, "shaders/simple.vert.spv", "shaders/simple.frag.spv", pipelineConfig);
}

void Application::createCommandBuffers()
{

}

void Application::drawFrame()
{
}
