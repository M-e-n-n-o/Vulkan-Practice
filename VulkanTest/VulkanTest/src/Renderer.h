#pragma once

#include "Window.h"
#include "Device.h"
#include "SwapChain.h"

#include <memory>
#include <vector>
#include <cassert>

class Renderer
{
private:
	Window& m_window;
	Device& m_device;
	std::unique_ptr<SwapChain> m_swapChain;
	std::vector<VkCommandBuffer> m_commandBuffers;

	uint32_t m_currentImageIndex;
	int m_currentFrameIndex = 0;
	bool m_isFrameStarted = false;

public:
	Renderer(Window& m_window, Device& m_device);
	~Renderer();

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	VkRenderPass getSwapChainRenderPass() const { return m_swapChain->getRenderPass(); }

	bool isFrameInProgress() const { return m_isFrameStarted; }

	VkCommandBuffer getCurrentCommandBuffer() const 
	{ 
		assert(m_isFrameStarted && "Cannot get command buffer when frame not in progress");
		return m_commandBuffers[m_currentFrameIndex];
	}

	int getFrameIndex() const 
	{
		assert(m_isFrameStarted && "Cannot get frame index when frame not in progress");
		return m_currentFrameIndex;
	}

	VkCommandBuffer beginFrame();
	void endFrame();

	void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
	void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

private:
	void recreateSwapChain();
	void createCommandBuffers();
	void freeCommandBuffers();
};