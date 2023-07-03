#version 430 core

layout(location = 0) in vec2 vertexPosition; 
layout(location = 1) in vec2 vertexTexturePosition; 
layout(location = 2) in mat3x2 instanceTransform; 
layout(location = 5) in vec4 instanceColor; 
layout(location = 6) in float instanceSmoothing; 
layout(location = 7) in float instanceLineWidth; 
layout(location = 8) in float instanceLineLength; 

out vec2 position; 

out vec4 color; 
out float smoothing; 
out float lineWidth; 
out float lineLength; 

void passToFragment() {
    color = instanceColor; 
    smoothing = instanceSmoothing; 
    lineWidth = instanceLineWidth; 
    lineLength = instanceLineLength; 
}

/*generated end*/

void main() {
    passToFragment();
    gl_Position = vec4(instanceTransform * vec3(vertexPosition, 1.0), 0.0, 1.0);
	position = vertexTexturePosition;
	position.y -= 0.5;
	position.y *= 2.0;
}
