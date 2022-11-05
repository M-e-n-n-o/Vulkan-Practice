#include "Application.h"
#include "SimpleRenderSystem.h"
#include "Camera.h"
#include "KeyBoardMovementController.h"

#include <stdexcept>
#include <array>
#include <chrono>

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
			m_renderer.beginSwapChainRenderPass(commandBuffer);

			simpleRenderSystem.renderGameObjects(commandBuffer, m_gameObjects, camera);

			m_renderer.endSwapChainRenderPass(commandBuffer);
			m_renderer.endFrame();
		}
	}

	vkDeviceWaitIdle(m_device.device());
}

void Application::loadGameObjects()
{
	std::shared_ptr<Model> model = Model::CreateModelFromFile(m_device, "res/smooth_vase.obj");

	auto cube = GameObject::CreateGameObject();
	cube.model = model;
	cube.transform.translation = { 0.0f, 0.0f, 2.5f };
	cube.transform.rotation = { 0, 0, 0 };
	cube.transform.scale = { 0.5f, 0.5f, 0.5f };

	m_gameObjects.push_back(std::move(cube));
}
