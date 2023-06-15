#version 430 core

in vec2 texturePos;
in vec2 atlasOffset;
in vec2 size;
in vec4 color;

uniform sampler2D textureAtlas;

out vec4 fragColor;

void main() {
	vec2 offset = atlasOffset;
	vec2 pos = texturePos;
	pos *= size;
	pos += offset;
	pos.y = 1.0 - pos.y;
	fragColor = texture(textureAtlas, pos) * color;
}