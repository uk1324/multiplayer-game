#include <Engine/Input/Input.hpp>
#include <Engine/Window.hpp>
#include <imgui.h>

auto Input::isKeyDown(KeyCode key) -> bool {
	return keyDown[static_cast<size_t>(key)];
}

auto Input::isKeyDownWithAutoRepeat(KeyCode key) -> bool {
	return keyDownWithAutoRepeat[static_cast<usize>(key)];
}

auto Input::isKeyUp(KeyCode key) -> bool {
	return keyUp[static_cast<usize>(key)];
}

auto Input::isKeyHeld(KeyCode key) -> bool {
	return keyHeld[static_cast<usize>(key)];
}

auto Input::isMouseButtonDown(MouseButton button) -> bool {
	if (!ignoreImGuiWantCapture && ImGui::GetIO().WantCaptureMouse)
		return false;
	return isKeyDown(static_cast<KeyCode>(button));
}

auto Input::isMouseButtonUp(MouseButton button) -> bool {
	if (!ignoreImGuiWantCapture && ImGui::GetIO().WantCaptureMouse)
		return false;
	return isKeyUp(static_cast<KeyCode>(button));
}

auto Input::isMouseButtonHeld(MouseButton button) -> bool {
	if (!ignoreImGuiWantCapture && ImGui::GetIO().WantCaptureMouse)
		return false;
	return isKeyHeld(static_cast<KeyCode>(button));
}

auto Input::windowSpaceToClipSpace(Vec2 v) -> Vec2 {
	v /= (Window::size() / 2.0f);
	v.y = -v.y;
	v += Vec2{ -1.0f, 1.0f };
	return v;
}

auto Input::scrollDelta() -> float {
	if (!ignoreImGuiWantCapture && ImGui::GetIO().WantCaptureMouse)
		return 0.0f;
	return scrollDelta_;
}

auto Input::anyKeyPressed() -> bool {
	return anyKeyPressed_;
}

auto Input::update() -> void {
	keyDown.reset();
	keyDownWithAutoRepeat.reset();
	keyUp.reset();

	for (auto& [_, isDown] : buttonDown) isDown = false;
	for (auto& [_, isDown] : buttonDownWithAutoRepeat) isDown = false;
	for (auto& [_, isUp] : buttonUp) isUp = false;

	scrollDelta_ = 0.0f;
	anyKeyPressed_ = false;
}

static auto setIfAlreadyExists(std::unordered_map<int, bool>& map, int key, bool value) -> void {
	if (auto it = map.find(key); it != map.end())
		it->second = value;
}

static auto isMouseButton(u16 vkCode) -> bool {
	const auto code = static_cast<MouseButton>(vkCode);
	return code == MouseButton::LEFT || code == MouseButton::RIGHT || code == MouseButton::MIDDLE;
}

static auto isKeyboardKey(u16 vkCode) -> bool {
	return !isMouseButton(vkCode);
}

auto Input::onKeyDown(u16 virtualKeyCode, bool autoRepeat) -> void {
	if (virtualKeyCode >= VIRTUAL_KEY_COUNT)
		return;

	anyKeyPressed_ = true;

	if (!autoRepeat) {
		keyDown.set(virtualKeyCode);
		keyHeld.set(virtualKeyCode);
	}
	keyDownWithAutoRepeat.set(virtualKeyCode);


	const auto buttons = virtualKeyToButton.equal_range(virtualKeyCode);
	for (auto button = buttons.first; button != buttons.second; ++button) {
		const auto& [_, buttonCode] = *button;
		if (!autoRepeat) {
			setIfAlreadyExists(buttonDown, buttonCode, true);
			setIfAlreadyExists(buttonHeld, buttonCode, true);
		}
		setIfAlreadyExists(buttonDownWithAutoRepeat, buttonCode, true);
	}
}

auto Input::onKeyUp(u16 virtualKeyCode) -> void {
	if (virtualKeyCode >= VIRTUAL_KEY_COUNT)
		return;

	keyUp.set(virtualKeyCode);
	keyHeld.set(virtualKeyCode, false);

	const auto buttons = virtualKeyToButton.equal_range(virtualKeyCode);
	for (auto button = buttons.first; button != buttons.second; ++button) {
		const auto& [_, buttonCode] = *button;
		setIfAlreadyExists(buttonUp, buttonCode, true);
		setIfAlreadyExists(buttonHeld, buttonCode, false);
	}
}

auto Input::onMouseMove(Vec2 mousePos) -> void {
	cursorPosClipSpace_ = windowSpaceToClipSpace(mousePos);
	cursorPosWindowSpace_ = mousePos;
}

auto Input::onMouseScroll(float scroll) -> void {
	scrollDelta_ = scroll;
}

bool Input::ignoreImGuiWantCapture = false;

std::bitset<Input::VIRTUAL_KEY_COUNT> Input::keyDown;
std::bitset<Input::VIRTUAL_KEY_COUNT> Input::keyUp;
std::bitset<Input::VIRTUAL_KEY_COUNT> Input::keyDownWithAutoRepeat;
std::bitset<Input::VIRTUAL_KEY_COUNT> Input::keyHeld;

std::unordered_multimap<u8, int> Input::virtualKeyToButton;
std::unordered_map<int, bool> Input::buttonDown;
std::unordered_map<int, bool> Input::buttonDownWithAutoRepeat;
std::unordered_map<int, bool> Input::buttonUp;
std::unordered_map<int, bool> Input::buttonHeld;

Vec2 Input::cursorPosClipSpace_;
Vec2 Input::cursorPosWindowSpace_;
float Input::scrollDelta_;
bool Input::anyKeyPressed_;