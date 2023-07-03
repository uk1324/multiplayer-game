#version 430 core

uniform vec4 color; 

in vec2 texturePosition; 
out vec4 fragColor;

/*generated end*/

uniform sampler2D colorBuffer;

void main() {
	vec3 v = vec3(0.0);
	vec2 pixelSize = 1.0 / vec2(textureSize(colorBuffer, 0));
	for (int x = -2; x <= 2; x++) {
		for (int y = -2; y <= 2; y++) {
			v += texture(colorBuffer, texturePosition + pixelSize * vec2(x, y)).rgb;
		}	
	}
	v /= 25;


	fragColor = vec4(v, 1.0);
	//fragColor = vec4(texture(colorBuffer, texturePosition).rgb * color.rgb, 1.0);
}
