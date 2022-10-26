#pragma once

#include "Window.h"
#include "Pipeline.h"
#include "Device.h"
#include "SwapChain.h"
#include "GameObject.h"

#include <memory>
#include <vector>

class Application
{
public:
	static constexpr int WIDTH = 720;
	static constexpr int HEIGHT = 720;

private:
	Window m_window{ "Vulkan practice", WIDTH, HEIGHT };
	Device m_device{ m_window };
	std::unique_ptr<SwapChain> m_swapChain;
	
	std::unique_ptr<Pipeline> m_pipeline;
	VkPipelineLayout m_pipelineLayout;
	std::vector<VkCommandBuffer> m_commandBuffers;

	std::vector<GameObject> m_gameObjects;

public:
	Application();
	~Application();

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	void run();

private:
	void loadGameObjects();
	void createPipelineLayout();
	void recreateSwapChain();
	void createPipeline();
	void createCommandBuffers();
	void recordCommandBuffer(int imageIndex);
	void freeCommandBuffers();
	void drawFrame();
	void renderGameObjects(VkCommandBuffer commandBuffer);
};