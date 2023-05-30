#include <Engine/Engine.hpp>
#include <Engine/Window.hpp>
#include <shared/Utils/Assertions.hpp>
#include <Log/Log.hpp>
#include <Engine/Input/Input.hpp>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <Game/GameClient.hpp>

bool Engine::run(int fps)
{
	constexpr double FRAME_TIME_CAP = 2.0;

	double lastFrameStartTime = Time::currentTime();
	double frameStartTime;
	double frameTime;

	double accumulatedTime = 0.0;
	const double updateTime = 1.0 / fps;

	GameClient game;

	while (Window::shouldClose() == false)
	{
		frameStartTime = Time::currentTime();
		frameTime = frameStartTime - lastFrameStartTime;
		lastFrameStartTime = frameStartTime;

		if (frameTime > FRAME_TIME_CAP)
			frameTime = FRAME_TIME_CAP;

		accumulatedTime += frameTime;

		while (accumulatedTime >= updateTime)
		{
			{
				Time::update(updateTime);
				Input::update();
				glfwPollEvents();
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();

				game.update(Time::deltaTime());
				if (Input::isKeyDown(KeyCode::R)) {
					return false;
				}

				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
				Window::update();
			}
			accumulatedTime -= updateTime;
		}
	}
	return true;
}

void Engine::stop()
{
	Window::close();
}


void Engine::init(int windowWidth, int windowHeight, std::string_view windowTitle)
{
	initGlfw();
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	Window::init(windowWidth, windowHeight, windowTitle);
	initOpenGl();
	initImGui();

	if (!InitializeYojimbo()) {
		LOG_FATAL("failed to initialize Yojimbo");
	}
	yojimbo_log_level(YOJIMBO_LOG_LEVEL_INFO);
	srand((unsigned int)time(NULL));
}

void Engine::terminate()
{
	ShutdownYojimbo();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	Window::terminate();
	glfwTerminate();
}

void Engine::initGlfw()
{
	if (glfwInit() == GLFW_FALSE)
	{
		LOG_FATAL("failed to initialize GLFW");
	}
	glfwSetErrorCallback(glfwErrorCallback);
}

void Engine::initOpenGl()
{
	if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == false)
	{
		LOG_FATAL("failed to initialize OpenGL");
	}


	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(openGlErrorCallback, nullptr);
	}
	else
	{
		LOG_ERROR("failed to initialize debug output");
	}
}

void Engine::initImGui()
{
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui::StyleColorsDark();
	io.Fonts->AddFontFromFileTTF("assets/fonts/RobotoMono-Regular.ttf", 20);
	/*io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;*/
	auto& style = ImGui::GetStyle();
	style.WindowRounding = 5.0f;

	ImGui_ImplGlfw_InitForOpenGL(Window::handle(), true);
	ImGui_ImplOpenGL3_Init("#version 430");
}

void Engine::glfwErrorCallback(int errorCode, const char* errorMessage)
{
	LOG_FATAL("glfw error %d %s", errorCode, errorMessage);
}

void Engine::openGlErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
		return;

	// Fragment shader recompiled due to state change.
	if (id == 2)
		return;

	// glLineWidth deprecated.
	if (id == 7)
		return;

	std::string errorMessage = "source: ";

	switch (source)
	{
		case GL_DEBUG_SOURCE_API:             errorMessage += "api"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   errorMessage += "window system"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: errorMessage += "shader compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     errorMessage += "third party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:     errorMessage += "application"; break;
		case GL_DEBUG_SOURCE_OTHER:           errorMessage += "other"; break;
	}

	errorMessage += " type: ";

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:               errorMessage += "error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: errorMessage += "deprecated behaviour"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  errorMessage += "undefined behaviour"; break;
		case GL_DEBUG_TYPE_PORTABILITY:         errorMessage += "portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         errorMessage += "performance"; break;
		case GL_DEBUG_TYPE_MARKER:              errorMessage += "marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          errorMessage += "push group"; break;
		case GL_DEBUG_TYPE_POP_GROUP:           errorMessage += "pop group"; break;
		case GL_DEBUG_TYPE_OTHER:               errorMessage += "other"; break;
	}

	errorMessage += " severity: ";

	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:         errorMessage += "high"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:       errorMessage += "medium"; break;
		case GL_DEBUG_SEVERITY_LOW:          errorMessage += "low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: errorMessage += "notification"; break;
	} 

	errorMessage += " message: ";
	errorMessage += message;
	if (id == 1286)
		LOG_ERROR("OpenGL error %s", errorMessage.c_str());
	else
		LOG_FATAL("OpenGL error %s", errorMessage.c_str());

}
