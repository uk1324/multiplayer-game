#pragma once

#include <engine/Math/Vec2.hpp>
#include <engine/Math/Vec3.hpp>
#include <engine/Utils/StringStream.hpp>
#include <imgui/imgui.h>
#include <vector>

namespace Debug {
	void update(float dt);

	void drawCircle(Vec2 pos, float radius, Vec3 color = Vec3(1.0f));

	void scrollInput(float& value, float scalePerSecond = 10.0f);

	// TODO: dragablePoint() // id or source line.
	struct Circle {
		Vec2 pos;
		float radius;
		Vec3 color;
	};
	extern std::vector<Circle> circles;
}

#define chk(name) static bool name = false; ImGui::Checkbox(#name, &name); if (name)
#define chkbox(name) static bool name = false; ImGui::Checkbox(#name, &name);
#define floatin(name, value) static float name = value; ImGui::InputFloat(#name, &name)
#define intin(name, value) static int name = value; ImGui::InputInt(#name, &name)

template<typename T>
auto dbgImplementation(T&& value, const char* expressionString) -> T&& {
	thread_local StringStream stream;
	stream.string().clear();
	stream << value;
	printf("%s = %s\n", expressionString, stream.string().c_str());
	return value;
}

#define dbg(value) debugImplementation(value, #value)