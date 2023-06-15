#version 430 core

layout (location = 0) in vec2 vertexPos;
layout (location = 1) in vec2 vertexTexturePos;

uniform mat3x2 cameraToWorld;

out vec2 worldPos;

void main() {
	gl_Position = vec4(vertexPos, 0.0, 1);
	worldPos = cameraToWorld * vec3(vertexPos, 1.0);
}