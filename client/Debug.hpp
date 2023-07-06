#pragma once

#include <engine/Math/Vec2.hpp>
#include <engine/Math/Vec3.hpp>
#include <engine/Utils/StringStream.hpp>
#include <client/Camera.hpp>
#include <imgui/imgui.h>
#include <vector>
#include <source_location>

namespace Debug {
	void update(const Camera& camera, float dt);

	static constexpr Vec3 DEFAULT_COLOR(1.0f);

	void drawCircle(Vec2 pos, float radius, Vec3 color = DEFAULT_COLOR);
	void drawPoint(Vec2 pos, Vec3 color = DEFAULT_COLOR);
	void drawLine(Vec2 pos, Vec2 end, Vec3 color = DEFAULT_COLOR, std::optional<float> width = std::nullopt);
	void drawRay(Vec2 start, Vec2 ray, Vec3 color = DEFAULT_COLOR, std::optional<float> width = std::nullopt);
	void drawPolygon(std::span<Vec2> vertices, Vec3 color = DEFAULT_COLOR, std::optional<float> width = std::nullopt);

	auto drawStr(Vec2 pos, const char* text, const Vec3& color = DEFAULT_COLOR, float height = 0.1f) -> void;
	template<typename T>
	auto drawText(Vec2 pos, const T& value, const Vec3& color = DEFAULT_COLOR, float height = 0.1f) -> void;

	void scrollInput(float& value, float scalePerSecond = 10.0f);
	void keyboardMovementInput(Vec2& pos, float speed = 1.0f);
	// Cannot use the source location, because things like functions and loops have the same source location for all iterations and calls.
	void dragablePoint(Vec2& point);

	struct Circle {
		Vec2 pos;
		float radius;
		Vec3 color;
	};
	extern std::vector<Circle> circles;

	struct Line {
		Vec2 start;
		Vec2 end;
		std::optional<float> width;
		Vec3 color;
	};
	extern std::vector<Line> lines;

	struct Text {
		Vec2 pos;
		std::string_view text;
		Vec3 color;
		float height;
	};
	extern std::vector<Text> texts;

	extern std::optional<Vec2> currentlyDraggedPoint;
	extern bool alreadyDraggedAPointThisFrame;

	template<typename T>
	void text(T&& value) {
		thread_local StringStream stream;
		stream.string().clear();
		stream << value;
		ImGui::Text("%s", stream.string().c_str());
	}

	template<typename T>
	auto drawText(Vec2 pos, const T& value, const Vec3& color, float height) -> void {
		thread_local StringStream stream;
		stream.string().clear();
		stream << value;
		drawStr(pos, stream.string().data(), color, height);
	}
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
	return std::move(value);
}

#define dbg(value) debugImplementation(value, #value)