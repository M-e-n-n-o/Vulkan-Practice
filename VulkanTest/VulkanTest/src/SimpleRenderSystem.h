#pragma once

#include "Camera.h"
#include "Pipeline.h"
#include "Device.h"
#include "GameObject.h"
#include "FrameInfo.h"

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

	void renderGameObjects(FrameInfo& frameInfo, std::vector<GameObject>& gameObjects);

private:
	void createPipelineLayout();
	void createPipeline(VkRenderPass renderPass);
};