#include "Application.h"
#include "SimpleRenderSystem.h"
#include "Camera.h"
#include "KeyBoardMovementController.h"
#include "Buffer.h"

#include <stdexcept>
#include <array>
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

struct GlobalUbo
{
	glm::mat4 projectionMatrix{ 1.0f };
	glm::mat4 viewMatrix{ 1.0f };
	glm::vec3 lightDirection = glm::normalize(glm::vec3{ 1.0f, -3.0f, -1.0f });
};

Application::Application()
{
	loadGameObjects();
}

Application::~Application()
{

}

void Application::run()
{

	// Double buffering (use a buffer for every possible frame (so we don't have to wait for a frame to finish rendering, 
	// and therefore finish using the ubo) and just write data to the ubo that is going to be used next)
	std::vector<std::unique_ptr<Buffer>> uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < uboBuffers.size(); i++)
	{
		uboBuffers[i] = std::make_unique<Buffer>
		(
			m_device,
			sizeof(GlobalUbo),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		);

		uboBuffers[i]->map();
	}

	SimpleRenderSystem simpleRenderSystem{ m_device, m_renderer.getSwapChainRenderPass() };

	Camera camera{};
	//camera.setViewDirection(glm::vec3(0.0f), glm::vec3(0.5f, 0.0f, 1.0f));
	//camera.setViewTarget(glm::vec3(-1.0f, -2.0f, 20.0f), glm::vec3(0.0f, 0.0f, 2.5f));

	auto viewerObject = GameObject::CreateGameObject();
	KeyboardMovementController cameraController{};

	auto currentTime = std::chrono::high_resolution_clock::now();

	while (!m_window.shouldClose())
	{
		m_window.update();
		
		auto newTime = std::chrono::high_resolution_clock::now();
		float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
		currentTime = newTime;

		cameraController.moveInPlaneXZ(m_window.getNativeWindow(), frameTime, viewerObject);
		camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

		float aspect = m_renderer.getAspectRatio();
		camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f, 100.0f);

		auto commandBuffer = m_renderer.beginFrame();
		if (commandBuffer != nullptr)
		{
			int frameIndex = m_renderer.getFrameIndex();

			FrameInfo frameInfo
			{
				frameIndex,
				frameTime,
				commandBuffer,
				camera
			};

			// Update
			GlobalUbo ubo{};
			ubo.projectionMatrix = camera.getProjectionMatrix();
			ubo.viewMatrix = camera.getViewMatrix();
			uboBuffers[frameIndex]->writeToBuffer(&ubo);
			uboBuffers[frameIndex]->flush();

			// Render
			m_renderer.beginSwapChainRenderPass(commandBuffer);
			simpleRenderSystem.renderGameObjects(frameInfo, m_gameObjects);
			m_renderer.endSwapChainRenderPass(commandBuffer);
			m_renderer.endFrame();
		}
	}

	vkDeviceWaitIdle(m_device.device());
}

void Application::loadGameObjects()
{
	std::shared_ptr<Model> model = Model::CreateModelFromFile(m_device, "res/flat_vase.obj");

	auto cube = GameObject::CreateGameObject();
	cube.model = model;
	cube.transform.translation = { 0.0f, 0.5f, 2.5f };
	cube.transform.rotation = { 0, 0, 0 };
	cube.transform.scale = { 0.5f, 0.5f, 0.5f };

	m_gameObjects.push_back(std::move(cube));
}
