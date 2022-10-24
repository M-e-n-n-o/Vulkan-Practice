#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

class Window
{
private:
	GLFWwindow* m_window;

	unsigned int m_width;
	unsigned int m_height;

public:
	Window(const std::string& windowName, int width, int height);
	~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	void update();

	void createWindowSurface(VkInstance instance, VkSurfaceKHR* surface);

	bool shouldClose();
	VkExtent2D getExtent() { return { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height) }; }
};