#include <client/Debug.hpp>
#include <engine/Input/Input.hpp>
#include <engine/Window.hpp>

static float elapsed = 0.0f;
static float dt = 0.0f;
static Mat3x2 clipSpaceToWorldSpace = Mat3x2::identity;

static std::string drawStrStrings;

void Debug::update(const Camera& camera, float dt) {
	circles.clear();
	lines.clear();
	drawStrStrings.clear();
	texts.clear();
	elapsed += dt;
	::dt = dt;
	clipSpaceToWorldSpace = camera.clipSpaceToWorldSpace();
	alreadyDraggedAPointThisFrame = false;
}

void Debug::drawCircle(Vec2 pos, float radius, Vec3 color) {
	circles.push_back({ pos, radius, color });
}

void Debug::drawPoint(Vec2 pos, Vec3 color) {
	circles.push_back({ pos, 35.0f / Window::size().y, color });
}

void Debug::drawLine(Vec2 pos, Vec2 end, Vec3 color, std::optional<float> width) {
	lines.push_back({ pos, end, width, color });
}

void Debug::drawPolygon(std::span<Vec2> vertices, Vec3 color, std::optional<float> width) {
	if (vertices.size() < 2)
		return;

	int previous = vertices.size() - 1;
	for (int i = 0; i < vertices.size(); previous = i, i++) {
		Debug::drawLine(vertices[previous], vertices[i], color, width);
		previous = i;
	}
}


auto Debug::drawStr(Vec2 pos, const char* text, const Vec3& color, float height) -> void {
	const auto start = drawStrStrings.size();
	drawStrStrings += text;
	texts.push_back(Text{ pos, std::string_view(drawStrStrings).substr(start, strlen(text)), color, height });
}

void Debug::scrollInput(float& value, float scalePerSecond) {
	if (Input::scrollDelta() > 0.0f)
		value *= pow(scalePerSecond, dt);
	if (Input::scrollDelta() < 0.0f)
		value /= pow(scalePerSecond, dt);
}

void Debug::dragablePoint(Vec2& point) {
	const auto cursorPos = Input::cursorPosClipSpace() * clipSpaceToWorldSpace;
	Debug::drawPoint(point, Vec3(1.0f, 0.0f, 0.0f));

	if (Input::isMouseButtonDown(MouseButton::LEFT) && !currentlyDraggedPoint.has_value() && cursorPos.distanceTo(point) < 0.05) {
		currentlyDraggedPoint = point;
	}

	if (Input::isMouseButtonUp(MouseButton::LEFT)) {
		currentlyDraggedPoint = std::nullopt;
	}

	if (currentlyDraggedPoint.has_value() 
		&& *currentlyDraggedPoint == point 
		&& !alreadyDraggedAPointThisFrame /* Prevent multiple points being dragged */) {
		point = cursorPos;
		currentlyDraggedPoint = cursorPos;
		alreadyDraggedAPointThisFrame = true;
	}
}

std::vector<Debug::Circle> Debug::circles;
std::vector<Debug::Line> Debug::lines;
std::vector<Debug::Text> Debug::texts;

std::optional<Vec2> Debug::currentlyDraggedPoint;
bool Debug::alreadyDraggedAPointThisFrame;