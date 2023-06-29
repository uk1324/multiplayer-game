#include <client/Camera.hpp>
#include <engine/Math/Utils.hpp>
#include <engine/Math/Aabb.hpp>
#include <engine/Window.hpp>
#include <engine/Input/Input.hpp>

Camera::Camera(Vec2 pos, float zoom)
	: pos{ pos }
	, zoom{ zoom }
	, aspectRatio{ Window::aspectRatio() } // This is here to prevent bugs from not having the aspect ratio initialized correctly. If the camera is passed into the renderer this should set correctly after the frist frame, but if it is only created in the local scope it is really easy to forget to initialize it.
{}

Mat3x2 Camera::toNdc() const {
	return Mat3x2::scale(Vec2(1.0f / aspectRatio, 1.0f));
}

Mat3x2 Camera::cameraTransform() const {
	return Mat3x2::translate(-pos) * Mat3x2::scale(Vec2(zoom));
}

Mat3x2 Camera::clipSpaceToWorldSpace() const {
	return (cameraTransform() * toNdc()).inversed();
}

Mat3x2 Camera::makeTransform(Vec2 pos, float rotation, Vec2 scale) {
	const auto objectToWorld = Mat3x2::scale(scale) * Mat3x2::rotate(rotation) * Mat3x2::translate(Vec2(pos.x, pos.y));
	return objectToWorld * cameraTransform() * toNdc();
}


//auto Camera::posInGrid(Vec2 p, Vec2 gridCenter, float gridSize, Vec2T<i64> gridCellSize) -> Vec2T<i64> {
//	const Vec2 size{ gridSize * gridCellSize.xOverY(), gridSize };
//	const auto textureBox = Aabb::fromPosSize(gridCenter, size);
//	auto gridPos = p - Vec2{ textureBox.min.x, textureBox.max.y };
//	gridPos /= size;
//	gridPos.y = -gridPos.y;
//	return Vec2T<i64>{ (gridPos* Vec2{ gridCellSize }).applied(floor) };
//}
//
//auto Camera::interpolateTo(Vec2 desiredPos, float speed) -> void {
//	pos = lerp(pos, desiredPos, speed);
//}
//
//auto Camera::cameraTransform() const -> Mat3x2 {
//	return Mat3x2::translate(-pos * Vec2{ 1.0f, aspectRatio })* Mat3x2::scale(Vec2{ zoom });
//}
//
//auto Camera::height() const -> float {
//	return 2.0f / aspectRatio / zoom;
//}
//
//auto Camera::width() const -> float {
//	return 2.0f / zoom;
//}
//
//auto Camera::aabb() -> Aabb {
//	const auto halfSize = Vec2{ width(), height() } / 2.0f;
//	return Aabb::fromCorners(pos - halfSize, pos + halfSize);
//}
//
//auto Camera::setWidth(float width) -> void {
//	zoom = 2.0f / width;
//}
//
//auto Camera::setHeight(float height) -> void {
//	setWidth(height * aspectRatio);
//}
//
//auto Camera::fitAabbInView(const Aabb& aabb) -> void {
//	pos = aabb.center();
//	setWidth(aabb.size().x);
//	if (aabb.size().y > height()) {
//		setHeight(aabb.size().y);
//	}
//}
//
//auto Camera::cursorPos() const -> Vec2 {
//	return screenSpaceToCameraSpace(Input::cursorPos());
//}
//
////auto Camera::scrollOnCursorPos() -> void {
////	if (const auto scroll = Input::scrollDelta(); scroll != 0.0f) {
////		const auto cursorPosBeforeScroll = cursorPos();
////		const auto scrollSpeed = 15.0f * abs(scroll);
////		const auto scrollIncrement = pow(scrollSpeed, Time::deltaTime());
////		if (scroll > 0.0f) zoom *= scrollIncrement;
////		else zoom /= scrollIncrement;
////
////		pos -= (cursorPos() - cursorPosBeforeScroll);
////	}
////}
////
////auto Camera::moveOnWasd() -> void {
////	Vec2 dir{ 0.0f };
////	if (Input::isKeyHeld(KeyCode::W)) dir.y += 1.0f;
////	if (Input::isKeyHeld(KeyCode::S)) dir.y -= 1.0f;
////	if (Input::isKeyHeld(KeyCode::D)) dir.x += 1.0f;
////	if (Input::isKeyHeld(KeyCode::A)) dir.x -= 1.0f;
////	pos += dir.normalized() * Time::deltaTime() / zoom;
////}
//
//auto Camera::screenSpaceToCameraSpace(Vec2 screenSpacePos) const -> Vec2 {
//	return (screenSpacePos * Vec2{ 1.0f, 1.0f / aspectRatio } / zoom) + pos;
//}
//
//auto Camera::cameraSpaceToScreenSpace(Vec2 cameraSpacePos) const -> Vec2 {
//	return (cameraSpacePos - pos) * zoom * Vec2 { 1.0f, aspectRatio };
//}
//
//auto Camera::heightIfWidthIs(float width) const -> float {
//	return width / aspectRatio;
//}