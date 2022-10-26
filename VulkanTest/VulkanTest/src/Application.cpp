#include "Application.h"
#include "SimpleRenderSystem.h"

#include <stdexcept>
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

Application::Application()
{
	loadGameObjects();
}

Application::~Application()
{
	
}

void Application::run()
{
	SimpleRenderSystem simpleRenderSystem{ m_device, m_renderer.getSwapChainRenderPass() };

	while (!m_window.shouldClose())
	{
		m_window.update();
		
		auto commandBuffer = m_renderer.beginFrame();
		if (commandBuffer != nullptr)
		{
			m_renderer.beginSwapChainRenderPass(commandBuffer);

			simpleRenderSystem.renderGameObjects(commandBuffer, m_gameObjects);

			m_renderer.endSwapChainRenderPass(commandBuffer);
			m_renderer.endFrame();
		}
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
