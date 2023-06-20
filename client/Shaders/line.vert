#version 430 core

layout(location = 0) in vec2 vertexPosition; 
layout(location = 1) in vec2 vertexTexturePosition; 

uniform mat3x2 transform;
out vec2 texturePosition;

void main() {
    texturePosition = vertexTexturePosition;
    gl_Position = vec4(transform * vec3(vertexPosition, 1.0), 0.0, 1.0);
    //gl_Position = vec4(vertexPosition, 0.0, 1.0);
}
