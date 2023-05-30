#version 430 core

uniform sampler2DArray textureArraySampler;
uniform uint textureIndex;

in vec2 textureCoord;

out vec4 fragColor;

void main()
{
	fragColor = texture(textureArraySampler, vec3(textureCoord, textureIndex));
}