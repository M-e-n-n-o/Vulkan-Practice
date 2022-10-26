#include "Application.h"
#include <stdexcept>
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

struct SimplePushConstantData
{
	glm::mat2 transform{ 1.0f };
	glm::vec2 offset;
	alignas(16) glm::vec3 color;
};

Application::Application()
{
	loadGameObjects();
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

void Application::loadGameObjects()
{
	std::vector<Model::Vertex> vertices
	{
		{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}} 
	};

	auto lveModel = std::make_shared<Model>(m_device, vertices);

	std::vector<glm::vec3> colors
	{
		{1.f, .7f, .73f},
		{1.f, .87f, .73f},
		{1.f, 1.f, .73f},
		{.73f, 1.f, .8f},
		{.73, .88f, 1.f}
	};

	for (auto& color : colors) 
	{
		color = glm::pow(color, glm::vec3{ 2.2f });
	}

	for (int i = 0; i < 40; i++) 
	{
		auto triangle = GameObject::CreateGameObject();
		triangle.m_model = lveModel;
		triangle.m_transform2D.scale = glm::vec2(.5f) + i * 0.025f;
		triangle.m_transform2D.rotation = i * glm::pi<float>() * .025f;
		triangle.m_color = colors[i % colors.size()];
		m_gameObjects.push_back(std::move(triangle));
	}
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
	clearValues[0].color = { 0.05f, 0.05f, 0.05f, 1.0f };
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

	renderGameObjects(m_commandBuffers[imageIndex]);

	vkCmdEndRenderPass(m_commandBuffers[imageIndex]);

	result = vkEndCommandBuffer(m_commandBuffers[imageIndex]);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record command buffer");
	}
}

void Application::renderGameObjects(VkCommandBuffer commandBuffer)
{
	// update
	int i = 0;
	for (auto& obj : m_gameObjects) 
	{
		i += 1;
		obj.m_transform2D.rotation =
			glm::mod<float>(obj.m_transform2D.rotation + 0.001f * i, 2.f * glm::pi<float>());
	}

	// render
	m_pipeline->bind(commandBuffer);
	for (auto& obj : m_gameObjects) 
	{
		SimplePushConstantData push{};
		push.offset = obj.m_transform2D.translation;
		push.color = obj.m_color;
		push.transform = obj.m_transform2D.mat2();

		vkCmdPushConstants(
			commandBuffer,
			m_pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(SimplePushConstantData),
			&push);

		obj.m_model->bind(commandBuffer);
		obj.m_model->draw(commandBuffer);
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
