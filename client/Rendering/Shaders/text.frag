#version 430 core

in vec2 texturePosition; 

in float smoothing; 
out vec4 fragColor;

/*generated end*/

uniform sampler2D fontAtlas;

#include "Utils/noise.glsl"

uniform float time;

void main() {
	fragColor = texture2D(fontAtlas, texturePosition);
	fragColor.x -= 0.5;
	vec2 p = texturePosition;
	p.x += time / 15.0;
	fragColor.x += octave01(p * 25.0, 4) * 0.5;
	fragColor.x -= 0.1;
	fragColor = vec4(vec3(fragColor.x), 1.0);
	float d = smoothstep(0.0, 0.05, fragColor.x);
	fragColor = vec4(vec3(d), d);
}
