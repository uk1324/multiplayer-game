#include <Engine/Window.hpp>
#include <Engine/Input/Input.hpp>
#include <Log/Log.hpp>
#include <glfw/glfw3.h>

static GLFWwindow* windowHandle = nullptr;
static bool resizedThisFrame = true;

static void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		Input::onKeyDown(key, false);
	} else if (action == GLFW_RELEASE) {
		Input::onKeyUp(key);
	}
}

static void mouseMoveCallback(GLFWwindow* window, double mouseX, double mouseY) {
	Input::onMouseMove(Vec2(static_cast<float>(mouseX), static_cast<float>(mouseY)));
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	// @Hack: Assumes the button ids don't conflinct with keyboard ids.
	if (action == GLFW_PRESS) {
		Input::onKeyDown(button, false);
	} else if (action == GLFW_RELEASE) {
		Input::onKeyUp(button);
	}
}

static void mouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
	Input::onMouseScroll(static_cast<float>(yOffset));
}

static void windowResizeCallback(GLFWwindow* window, int width, int height) {
	resizedThisFrame = true;
}

void Window::init(int width, int height, std::string_view title) {
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	windowHandle = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
	if (windowHandle == nullptr)
	{
		LOG_FATAL("failed to create window");
	}
	glfwMakeContextCurrent(windowHandle);

	glfwSetKeyCallback(windowHandle, keyboardCallback);
	glfwSetCursorPosCallback(windowHandle, mouseMoveCallback);
	glfwSetMouseButtonCallback(windowHandle, mouseButtonCallback);
	glfwSetScrollCallback(windowHandle, mouseScrollCallback);
	glfwSetWindowSizeCallback(windowHandle, windowResizeCallback);
}

void Window::terminate() {
	glfwDestroyWindow(windowHandle);
}

Vec2 Window::size() {
	int x, y;
	glfwGetFramebufferSize(windowHandle, &x, &y);
	return Vec2{ static_cast<float>(x), static_cast<float>(y) };
}

float Window::aspectRatio() {
	const auto size = Window::size();
	return size.x / size.y;
}

const char* Window::getClipboard() {
	return glfwGetClipboardString(windowHandle);
}

void Window::setClipboard(const char* string) {
	glfwSetClipboardString(windowHandle, string);
}

void Window::update() {
	resizedThisFrame = false;
	glfwSwapBuffers(windowHandle);
}

void Window::close() {
	glfwSetWindowShouldClose(windowHandle, true);
}

void Window::setTitle(const char* title) {
	glfwSetWindowTitle(windowHandle, title);
}

void Window::setPos(Vec2 pos) {
	glfwSetWindowPos(windowHandle, static_cast<int>(pos.x), static_cast<int>(pos.y));
}

void Window::enableWindowedFullscreen() {
	//    if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED))
	//    {
	//        glfwRestoreWindow(engine.window().handle());
	//    }
	//    else
	//    {
	//        glfwMaximizeWindow(engine.window().handle());
	//    }
	glfwMaximizeWindow(windowHandle);
}

void Window::hideCursor() {
	glfwSetInputMode(windowHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Window::showCursor() {
	glfwSetInputMode(windowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

bool Window::resized() {
	return resizedThisFrame;
}

bool Window::shouldClose() {
	return glfwWindowShouldClose(windowHandle);
}

void* Window::handle() {
	return windowHandle;
}