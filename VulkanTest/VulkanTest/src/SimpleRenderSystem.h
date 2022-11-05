#pragma once

#include "Camera.h"
#include "Pipeline.h"
#include "Device.h"
#include "GameObject.h"

#include <memory>
#include <vector>

class SimpleRenderSystem
{
private:
	Device& m_device;

	std::unique_ptr<Pipeline> m_pipeline;
	VkPipelineLayout m_pipelineLayout;

public:
	SimpleRenderSystem(Device& device, VkRenderPass renderPass);
	~SimpleRenderSystem();

	SimpleRenderSystem(const SimpleRenderSystem&) = delete;
	SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

	void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<GameObject>& gameObjects, const Camera& camera);

private:
	void createPipelineLayout();
	void createPipeline(VkRenderPass renderPass);
};