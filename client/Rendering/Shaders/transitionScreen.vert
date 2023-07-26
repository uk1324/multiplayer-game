#version 430 core

layout(location = 0) in vec2 vertexPosition; 
layout(location = 1) in vec2 vertexTexturePosition; 
layout(location = 2) in vec2 instanceTranslation; 

/*generated end*/

void main() {
	gl_Position = vec4(vertexPosition + instanceTranslation, 0.0, 1.0);
}
