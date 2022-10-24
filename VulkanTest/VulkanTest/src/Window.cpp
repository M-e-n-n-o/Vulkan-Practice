#include "Window.h"
#include <stdexcept>

Window::Window(const std::string& windowName, int width, int height)
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
	m_width = width;
	m_height = height;
}

Window::~Window()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Window::update()
{
	glfwPollEvents();
}

void Window::createWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
{
	bool result = glfwCreateWindowSurface(instance, m_window, nullptr, surface);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface");
	}
}

bool Window::shouldClose()
{
	return glfwWindowShouldClose(m_window);
}
