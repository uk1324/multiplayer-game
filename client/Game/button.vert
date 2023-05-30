#version 430 core

layout (location = 0) in vec2 vertexPos;

uniform vec2 size;
uniform vec2 pos;

out vec2 textureCoord;

void main()
{
	gl_Position = vec4(vertexPos * size + pos, 0, 1);
}