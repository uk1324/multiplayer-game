#pragma once

#include <imgui/imgui.h>
#include <shared/Utils/Types.hpp>
#include <shared/Math/Vec2.hpp>
#include <shared/Math/Vec3.hpp>
#include <shared/Math/Vec4.hpp>

namespace Gui {

void update();

void inputI32(const char* name, i32& value);
void inputFloat(const char* name, float& value);
void sliderFloat(const char* name, float& value, float min, float max);
void inputVec2(const char* name, Vec2& value);
void inputVec3(const char* name, Vec3& value);
void checkbox(const char* name, bool& value);
void inputColor(const char* name, Vec4& value);

bool beginPropertyEditor();
void endPropertyEditor();
void popPropertyEditor();

bool node(const char* name);
void nodeEnd();

}

#define GUI_PROPERTY_EDITOR(editorCode) \
	do { \
		if (Gui::beginPropertyEditor()) { \
			{ editorCode; } \
			Gui::endPropertyEditor(); \
		} \
		Gui::popPropertyEditor(); \
	} while (false)