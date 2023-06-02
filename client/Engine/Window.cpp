#include <Engine/Window.hpp>
#include <Engine/Input/Input.hpp>
#include <Log/Log.hpp>

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		Input::onKeyDown(key, false);
	} else if (action == GLFW_RELEASE)
	{
		Input::onKeyUp(key);
	}
}

void mouseMoveCallback(GLFWwindow* window, double mouseX, double mouseY)
{
	Input::onMouseMove(Vec2(static_cast<float>(mouseX), static_cast<float>(mouseY)));
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	// @Hack: Assumes the button ids don't conflinct with keyboard ids.
	if (action == GLFW_PRESS) {
		Input::onKeyDown(button, false);
	} else if (action == GLFW_RELEASE) {
		Input::onKeyUp(button);
	}
}

void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	Input::onMouseScroll(static_cast<float>(yOffset));
}

void Window::init(int width, int height, std::string_view title)
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	m_handle = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
	if (m_handle == nullptr)
	{
		LOG_FATAL("failed to create window");
	}
	glfwMakeContextCurrent(m_handle);

	glfwSetKeyCallback(m_handle, keyboardCallback);
	glfwSetCursorPosCallback(m_handle, mouseMoveCallback);
	glfwSetMouseButtonCallback(m_handle, mouseButtonCallback);
	glfwSetScrollCallback(m_handle, mouseScrollCallback);
}

void Window::terminate()
{
	glfwDestroyWindow(m_handle);
}

Vec2 Window::size()
{
	int x, y;
	glfwGetFramebufferSize(m_handle, &x, &y);
	return Vec2{ static_cast<float>(x), static_cast<float>(y) };
}

float Window::aspectRatio() {
	const auto size = Window::size();
	return size.x / size.y;
}

void Window::update()
{
	glfwSwapBuffers(m_handle);
}

void Window::close()
{
	glfwSetWindowShouldClose(m_handle, true);
}

void Window::setTitle(const char* title)
{
	glfwSetWindowTitle(m_handle, title);
}

void Window::setPos(Vec2 pos)
{
	glfwSetWindowPos(m_handle, static_cast<int>(pos.x), static_cast<int>(pos.y));
}

void Window::hideCursor()
{
	glfwSetInputMode(m_handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Window::showCursor()
{
	glfwSetInputMode(m_handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

bool Window::shouldClose()
{
	return glfwWindowShouldClose(m_handle);
}

GLFWwindow* Window::handle()
{
	return m_handle;
}

GLFWwindow* Window::m_handle;