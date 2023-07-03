#version 430 core

layout(location = 0) in vec3 vertexPosition; 
layout(location = 1) in vec2 vertexTexturePosition; 

uniform mat3x2 transform;
uniform float len;
out vec2 texturePosition;
out vec3 p;

void main() {
    texturePosition = vertexTexturePosition;
    p = vertexPosition;
    gl_Position = vec4(transform * vec3(vertexPosition.xy, 1.0), texturePosition.x / len, 1.0);
}
