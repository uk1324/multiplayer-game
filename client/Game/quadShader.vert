#version 430 core

layout (location = 0) in vec2 vertexPos;
layout (location = 1) in vec2 vertexTexturePos;

uniform mat3x2 transform;

out vec2 texturePos;

void main() {
	gl_Position = vec4(transform * vec3(vertexPos, 1.0), 0.0, 1);
	texturePos = vertexTexturePos;	
}