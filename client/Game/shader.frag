#version 430 core

in vec2 texturePos;
in vec2 atlasOffset;
in vec2 size;

uniform sampler2D textureAtlas;

out vec4 fragColor;

void main() {
	vec2 offset = atlasOffset;
	offset.y = 1.0f - size.y + offset.y;
	fragColor = texture(textureAtlas, texturePos * size + offset);
}