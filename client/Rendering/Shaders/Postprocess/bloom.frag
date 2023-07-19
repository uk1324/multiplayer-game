#version 430 core

in vec2 texturePosition; 
out vec4 fragColor;

/*generated end*/

uniform sampler2D colorBuffer;

void main() {
	fragColor = vec4(texture(colorBuffer, texturePosition).rgb, 1.0);
}
