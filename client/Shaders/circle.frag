#version 430 core

in vec2 position; 

in vec4 color; 
in float smoothing; 
in float width; 

out vec4 fragColor;

/*generated end*/

#include "Utils/sdf.glsl"

void main() {
	vec2 p = position;
	
	float radius = 1.0 - smoothing;
	float d = ringSdf(p, vec2(0.0), radius, width + smoothing);
	d = smoothstep(smoothing, 0.0, d);
	fragColor = vec4(vec3(d), 1.0) * color;
	fragColor.a *= d;
}
