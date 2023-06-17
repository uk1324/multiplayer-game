#version 430 core

layout (location = 0) in vec2 vertexPos;
layout (location = 1) in vec2 vertexTexturePos;
layout (location = 2) in mat3x2 instanceTransform;
layout (location = 5) in vec2 instanceAtlasOffset;
layout (location = 6) in vec2 instanceSize;
layout (location = 7) in vec4 instanceColor;

out vec2 texturePos;
out vec2 atlasOffset;
out vec2 size;
out vec4 color;
/*generated end*/
void main() {
	gl_Position = vec4(instanceTransform * vec3(vertexPos, 1.0), 0.0, 1);
	texturePos = vertexTexturePos;	
	atlasOffset = instanceAtlasOffset;
	size = instanceSize;
	color = instanceColor;
}