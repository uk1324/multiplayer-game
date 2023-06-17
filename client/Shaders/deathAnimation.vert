#version 430 core

layout(location = 0) in vec2 vertexPosition; 
layout(location = 1) in vec2 vertexTexturePosition; 
layout(location = 2) in mat3x2 instanceTransform; 
layout(location = 5) in float instanceTime; 

out vec2 texturePosition; 

out float time; 

void passToFragment() {
    time = instanceTime; 
}
/*generated end*/

void main() {
    passToFragment();
    texturePosition = vertexTexturePosition;
    gl_Position = vec4(instanceTransform * vec3(vertexPosition, 1.0), 0.0, 1);
}
