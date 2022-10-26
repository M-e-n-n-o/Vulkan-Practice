#pragma once

#include "Window.h"
#include "Device.h"
#include "GameObject.h"
#include "Renderer.h"

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
	Renderer m_renderer{ m_window, m_device };

	std::vector<GameObject> m_gameObjects;

public:
	Application();
	~Application();

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	void run();

private:
	void loadGameObjects();
};