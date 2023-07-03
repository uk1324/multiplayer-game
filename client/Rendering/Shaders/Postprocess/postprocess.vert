#version 430 core

layout(location = 0) in vec2 vertexPosition; 
layout(location = 1) in vec2 vertexTexturePosition; 

out vec2 texturePosition;

void main() {
    gl_Position = vec4(vec3(vertexPosition, 0.0), 1.0);
	texturePosition = vertexTexturePosition;
}
