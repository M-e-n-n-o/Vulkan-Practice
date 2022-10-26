#include "Application.h"
#include <stdexcept>
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

struct SimplePushConstantData
{
	glm::vec2 offset;
	alignas(16) glm::vec3 color;
};

Application::Application()
{
	loadModels();
	createPipelineLayout();
	recreateSwapChain();
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
		drawFrame();
	}

	vkDeviceWaitIdle(m_device.device());
}

void Application::loadModels()
{
	std::vector<Model::Vertex> vertices
	{
		// Position     Color
		{{0.0f, -0.5f}, {1, 0, 0}},
		{{0.5f, 0.5f}, {0, 1, 0}},
		{{-0.5f, 0.5f}, {0, 0, 1}}
	};

	m_model = std::make_unique<Model>(m_device, vertices);
}

void Application::createPipelineLayout()
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(SimplePushConstantData);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	bool result = vkCreatePipelineLayout(m_device.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}
}

void Application::recreateSwapChain()
{
	auto extent = m_window.getExtent();
	while (extent.width == 0 || extent.height == 0)
	{
		extent = m_window.getExtent();
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_device.device());

	if (m_swapChain == nullptr)
	{
		m_swapChain = std::make_unique<SwapChain>(m_device, extent);
	}
	else
	{
		m_swapChain = std::make_unique<SwapChain>(m_device, extent, std::move(m_swapChain));

		if (m_swapChain->imageCount() != m_commandBuffers.size())
		{
			freeCommandBuffers();
			createCommandBuffers();
		}
	}

	// TODO if render pass is still compatible with the pipeline, no need to recreate pipeline
	createPipeline();
}

void Application::createPipeline()
{
	assert(m_swapChain != nullptr && "Cannot create pipeline before swap chain");
	assert(m_pipelineLayout != nullptr && "Cannot create pupeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};
	Pipeline::DefaultPipelineConfigInfo(pipelineConfig);

	pipelineConfig.renderPass = m_swapChain->getRenderPass();
	pipelineConfig.pipelineLayout = m_pipelineLayout;

	m_pipeline = std::make_unique<Pipeline>(m_device, "shaders/simple.vert.spv", "shaders/simple.frag.spv", pipelineConfig);
}

void Application::createCommandBuffers()
{
	m_commandBuffers.resize(m_swapChain->imageCount());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_device.getCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

	bool result = vkAllocateCommandBuffers(m_device.device(), &allocInfo, m_commandBuffers.data());
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate command buffers");
	}
}

void Application::recordCommandBuffer(int imageIndex)
{
	static int frame = 0;
	frame = (frame + 1) % 1000;

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	bool result = vkBeginCommandBuffer(m_commandBuffers[imageIndex], &beginInfo);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to start recording command buffer");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_swapChain->getRenderPass();
	renderPassInfo.framebuffer = m_swapChain->getFrameBuffer(imageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_swapChain->getSwapChainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();


	vkCmdBeginRenderPass(m_commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapChain->getSwapChainExtent().width);
	viewport.height = static_cast<float>(m_swapChain->getSwapChainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(m_commandBuffers[imageIndex], 0, 1, &viewport);

	VkRect2D scissor{ {0, 0}, m_swapChain->getSwapChainExtent() };
	vkCmdSetScissor(m_commandBuffers[imageIndex], 0, 1, &scissor);

	m_pipeline->bind(m_commandBuffers[imageIndex]);
	m_model->bind(m_commandBuffers[imageIndex]);

	for (int j = 0; j < 4; j++)
	{
		SimplePushConstantData push{};
		push.offset = { -0.5f + frame * 0.02f, -0.4f + j * 0.25f };
		push.color = { 0.0f, 0.0f, 0.2f + 0.2f * j };

		vkCmdPushConstants(m_commandBuffers[imageIndex],
			m_pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(SimplePushConstantData),
			&push);

		m_model->draw(m_commandBuffers[imageIndex]);
	}

	vkCmdEndRenderPass(m_commandBuffers[imageIndex]);

	result = vkEndCommandBuffer(m_commandBuffers[imageIndex]);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record command buffer");
	}
}

void Application::freeCommandBuffers()
{
	vkFreeCommandBuffers(m_device.device(), m_device.getCommandPool(), static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
	m_commandBuffers.clear();
}

void Application::drawFrame()
{
	uint32_t imageIndex;
	auto result = m_swapChain->acquireNextImage(&imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swap chain image");
	}

	recordCommandBuffer(imageIndex);

	result = m_swapChain->submitCommandBuffers(&m_commandBuffers[imageIndex], &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window.wasWindowResized())
	{
		m_window.resetWindowResizedFlag();
		recreateSwapChain();
		return;
	}

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present swap chain image");
	}
}
