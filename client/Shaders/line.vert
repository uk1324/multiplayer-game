#version 430 core

layout(location = 0) in vec3 vertexPosition; 
layout(location = 1) in vec2 vertexTexturePosition; 

uniform mat3x2 transform;
out vec2 texturePosition;
out vec3 p;

void main() {
    texturePosition = vertexTexturePosition;
    p = vertexPosition;
    //gl_Position = vec4(transform * vec3(vertexPosition.xy, 1.0), 0.0, 1.0);
    //gl_Position = vec4(transform * vec3(vertexPosition.xy, 1.0), -vertexPosition.z, 1.0);
    // float z;
    // if (vertexPosition.z == -1.0) {
    //     z = (1.0 - texturePosition.y - 0.5) * 2.0;
    // } else {
    //     z = ((1.0 - texturePosition.y) - 0.5) * 2.0;
    // }

    //gl_Position = vec4(transform * vec3(vertexPosition.xy, 1.0), ((1.0 - texturePosition.y) - 0.5) * 2.0, 1.0);
    //gl_Position = vec4(transform * vec3(vertexPosition.xy, 1.0), ((texturePosition.y) - 0.5) * 2.0, 1.0);
    //gl_Position = vec4(transform * vec3(vertexPosition.xy, 1.0), abs(((texturePosition.y) - 0.5) * 2.0), 1.0);
    gl_Position = vec4(transform * vec3(vertexPosition.xy, 1.0), ((texturePosition.y) - 0.5) * 2.0, 1.0);
    //gl_Position = vec4(transform * vec3(vertexPosition.xy, 1.0), vertexPosition.z, 1.0);
    //gl_Position = vec4(transform * vec3(vertexPosition.xy, 1.0), vertexPosition.z, 1.0);
    //gl_Position = vec4(transform * vec3(vertexPosition.xy, 1.0), ((1.0 - texturePosition.y) - 0.5) * 2.0, 1.0);
    //gl_Position = vec4(vertexPosition, 0.0, 1.0);
}
