#version 430 core

layout(location = 0) in vec2 vertexPosition; 
layout(location = 1) in vec2 vertexTexturePosition; 
layout(location = 2) in mat3x2 instanceTransform; 
layout(location = 5) in float instanceTime; 
layout(location = 6) in vec3 instanceColor; 

out vec2 texturePosition; 

out float time; 
out vec3 color; 

void passToFragment() {
    time = instanceTime; 
    color = instanceColor; 
}

/*generated end*/

void main() {
    passToFragment();
    gl_Position = vec4(instanceTransform * vec3(vertexPosition, 1.0), 0.0, 1);
    texturePosition = vertexTexturePosition;
    texturePosition -= vec2(0.5);
    texturePosition *= 2.0;
}
