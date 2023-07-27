#version 430 core

layout(location = 0) in vec2 vertexPosition; 
layout(location = 1) in vec2 vertexTexturePosition; 
layout(location = 2) in mat3x2 instanceTransform; 
layout(location = 5) in vec3 instanceColor; 

out vec2 position; 

out vec3 color; 

void passToFragment() {
    color = instanceColor; 
}

/*generated end*/

void main() {
	gl_Position = vec4(instanceTransform * vec3(vertexPosition, 1.0), 0.0, 1.0);
	position = vertexTexturePosition;
	position -= vec2(0.5);
	//position -= vec2(0.5, 0.0);
	position *= 2.0;
	passToFragment();
}
