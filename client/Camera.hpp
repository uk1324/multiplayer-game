#pragma once

#include <engine/Math/Mat3x2.hpp>
#include <engine/Math/Aabb.hpp>

// When zoom is 1 the width goes from -1 to 1. So the width is 2.
struct Camera {
	Camera(Vec2 pos = Vec2{ 0.0f }, float zoom = 1.0f);

	Mat3x2 toNdc() const;
	Mat3x2 cameraTransform() const;
	Mat3x2 clipSpaceToWorldSpace() const;
	Mat3x2 makeTransform(Vec2 pos, float rotation, Vec2 scale);
	/*auto posInGrid(Vec2 pos, Vec2 gridCenter, float gridSize, Vec2T<i64> gridCellSize) -> Vec2T<i64>;
	auto interpolateTo(Vec2 desiredPos, float speed) -> void;
	auto cameraTransform() const -> Mat3x2;
	auto screenSpaceToCameraSpace(Vec2 screenSpacePos) const->Vec2;*/
	//auto cameraSpaceToScreenSpace(Vec2 cameraSpacePos) const->Vec2;
	//auto heightIfWidthIs(float width) const -> float;
	//auto height() const -> float;
	//auto width() const -> float;
	//auto aabb() -> Aabb;
	//auto setWidth(float width) -> void;
	//auto setHeight(float height) -> void;
	//auto fitAabbInView(const Aabb& aabb) -> void;
	//auto cursorPos() const->Vec2;
	//auto scrollOnCursorPos() -> void;
	//auto moveOnWasd() -> void;

	Vec2 pos;
	float zoom;
	// Aspect ratio should be width / height.
	float aspectRatio;
};