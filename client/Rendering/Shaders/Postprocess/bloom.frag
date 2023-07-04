#version 430 core

uniform vec4 color; 

in vec2 texturePosition; 
out vec4 fragColor;

/*generated end*/

uniform sampler2D colorBuffer;

void main() {
	fragColor = vec4(texture(colorBuffer, texturePosition).rgb * color.rgb, 1.0);
}
