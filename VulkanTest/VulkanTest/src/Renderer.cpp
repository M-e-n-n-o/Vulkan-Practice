#include "Renderer.h"
#include <stdexcept>
#include <array>

Renderer::Renderer(Window& window, Device& device): m_window(window), m_device(device)
{
	recreateSwapChain();
	createCommandBuffers();
}

Renderer::~Renderer()
{
	freeCommandBuffers();
}

void Renderer::recreateSwapChain()
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
		std::shared_ptr<SwapChain> oldSwapChain = std::move(m_swapChain);
		m_swapChain = std::make_unique<SwapChain>(m_device, extent, oldSwapChain);

		if (!oldSwapChain->compareSwapFormats(*m_swapChain.get()))
		{
			throw std::runtime_error("Swap chain image format has changed");
		}
	}
}

void Renderer::createCommandBuffers()
{
	m_commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

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

void Renderer::freeCommandBuffers()
{
	vkFreeCommandBuffers(m_device.device(), m_device.getCommandPool(), static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
	m_commandBuffers.clear();
}

VkCommandBuffer Renderer::beginFrame()
{
	assert(!m_isFrameStarted && "Cannot begin frame, frame has already been started");

	auto result = m_swapChain->acquireNextImage(&m_currentImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapChain();
		return nullptr;
	}

	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swap chain image");
	}

	m_isFrameStarted = true;

	auto commandBuffer = getCurrentCommandBuffer();
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to start recording command buffer");
	}

	return commandBuffer;
}

void Renderer::endFrame()
{
	assert(m_isFrameStarted && "Cannot end frame if frame has not been started");

	auto commandBuffer = getCurrentCommandBuffer();
	bool result = vkEndCommandBuffer(commandBuffer);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to record command buffer");
	}

	result = m_swapChain->submitCommandBuffers(&commandBuffer, &m_currentImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window.wasWindowResized())
	{
		m_window.resetWindowResizedFlag();
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present swap chain image");
	}

	m_isFrameStarted = false;
	m_currentFrameIndex = (m_currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
	assert(m_isFrameStarted && "Cannot call beginSwapChainRenderPass if frame has not been started");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot begin render pass on command buffer from a different frame");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_swapChain->getRenderPass();
	renderPassInfo.framebuffer = m_swapChain->getFrameBuffer(m_currentImageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_swapChain->getSwapChainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { 0.05f, 0.05f, 0.05f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	// Start recording
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_swapChain->getSwapChainExtent().width);
	viewport.height = static_cast<float>(m_swapChain->getSwapChainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{ {0, 0}, m_swapChain->getSwapChainExtent() };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Renderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer)
{
	assert(m_isFrameStarted && "Cannot call endSwapChainRenderPass if frame has not been started");
	assert(commandBuffer == getCurrentCommandBuffer() && "Cannot end render pass on command buffer from a different frame");

	vkCmdEndRenderPass(commandBuffer);
}