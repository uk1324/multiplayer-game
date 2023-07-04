#version 430 core

in vec2 texturePosition; 

in vec3 color; 
in float time; 
out vec4 fragColor;

/*generated end*/

#include "Utils/sdf.glsl"
#include "Utils/noise.glsl"

void main() {
	vec2 p = texturePosition;
	p -= vec2(0.5);
	p *= 2.0;

	float t = time;

	float d;
	float l0 = 0.5;
	float l1 = 0.5;
	float a;
	float smoothing = 0.21;
	if (t < l0) {
		t /= l0;
		t = 1.0 - t;
		d = ringSdf(p, vec2(0.0), 0.5 * t, 0.05 * t) - perlin01(p * 7.0) * 0.05;
		d = smoothstep(smoothing, 0.0, d);	
		//d *= 1.0 - t;
		//a = 1.0 - t;
		//a = t;
		a = 1.0 - 4.0 * pow(t - 0.5, 2.0);
	} else {
		t -= l0;
		t /= l1;
		d = circleSdf(p, vec2(0.0), 1.0 * t - smoothing) - perlin01(p * 7.0) * 0.05;
		d = smoothstep(smoothing, 0.0, d);
		//d *= 1.0 - length(p);
		//d = 1.0 - d;
		a = 1.0 - t;
	}

	vec3 col = color.rgb * max(0.5, octave01(p * 4.0, 3));
	col = color * max(0.5, octave01(p * 4.0, 3));
	fragColor = mix(vec4(col, 0.0), vec4(col, 1.0), a) * d;
}
