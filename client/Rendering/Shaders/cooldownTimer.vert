#version 430 core

layout(location = 0) in vec2 vertexPosition; 
layout(location = 1) in vec2 vertexTexturePosition; 
layout(location = 2) in mat3x2 instanceTransform; 
layout(location = 5) in float instanceT; 
layout(location = 6) in vec2 instanceSize; 

out vec2 position; 

out float t; 
out vec2 size; 

void passToFragment() {
    t = instanceT; 
    size = instanceSize; 
}

/*generated end*/

void main() {
    passToFragment();
    gl_Position = vec4(instanceTransform * vec3(vertexPosition, 1.0), 0.0, 1.0);
	position = vertexTexturePosition;
	position -= vec2(0.5);
	position *= 2.0;
}
