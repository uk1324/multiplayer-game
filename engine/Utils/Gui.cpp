#include "Gui.hpp"
#include <string>

using namespace Gui;
using namespace ImGui;

thread_local std::string string;
static const char* prependWithHashHash(const char* str) {
	const auto offset = string.size();
	string += "##";
	string += str;
	string += '\0';
	return string.data() + offset;
}

static void leafNodeBegin(const char* name) {
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	ImGui::TreeNodeEx(name, flags);
	ImGui::TableSetColumnIndex(1);
	ImGui::SetNextItemWidth(-FLT_MIN);
}

void Gui::update() {
	string.clear();
}

// Remember to always use prependWithHashHash
void Gui::inputI32(const char* name, i32& value) {
	leafNodeBegin(name);
	ImGui::InputScalar(name, ImGuiDataType_S32, reinterpret_cast<void*>(&value));
}

void Gui::inputFloat(const char* name, float& value) {
	leafNodeBegin(name);
	ImGui::InputFloat(prependWithHashHash(name), &value);
}

void Gui::sliderFloat(const char* name, float& value, float min, float max) {
	leafNodeBegin(name);
	ImGui::SliderFloat(prependWithHashHash(name), &value, min, max);
}

void Gui::inputVec2(const char* name, Vec2& value) {
	leafNodeBegin(name);
	InputFloat2(name, value.data());
}

void Gui::inputVec3(const char* name, Vec3& value) {
	leafNodeBegin(name);
	InputFloat3(name, value.data());
}

void Gui::checkbox(const char* name, bool& value) {
	leafNodeBegin(name);
	ImGui::Checkbox(prependWithHashHash(name), &value);
}

void Gui::inputColor(const char* name, Vec4& value) {
	leafNodeBegin(name);
	ImGui::ColorEdit4(prependWithHashHash(name), value.data());
}

bool Gui::beginPropertyEditor() {
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
	return ImGui::BeginTable("split", 2, ImGuiTableFlags_Resizable);
}

void Gui::endPropertyEditor() {
	ImGui::EndTable();
}

void Gui::popPropertyEditor() {
	ImGui::PopStyleVar();
}

bool Gui::node(const char* name) {
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	bool nodeOpen = ImGui::TreeNode(name);
	ImGui::TableSetColumnIndex(1);
	return nodeOpen;
}

void Gui::nodeEnd() {
	ImGui::TreePop();
}