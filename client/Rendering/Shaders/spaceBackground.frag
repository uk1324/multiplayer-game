#version 430 core

uniform float time; 
uniform vec3 color0; 
uniform float scale0; 
uniform vec3 color1; 
uniform float scale1; 

in vec2 worldPosition; 
out vec4 fragColor;

/*generated end*/

#include "Utils/noise.glsl"

float hash2to1(vec2 p) {
	p = fract(p * vec2(1534.12, 521.23));
	p += dot(p, p + vec2(0.24, 1.26));
	return fract(p.x * p.y);
}

vec3 star(vec2 p, float size) {
	vec3 col;
	float circle = max(0.0, 0.6 - distance(p, vec2(0.0)));
	float d = 1.0 - abs(p.x) * abs(p.y) * 100.0;
	d = size / distance(p, vec2(0.0));
	d *= circle;
	col = vec3(d);
	return max(col, 0.0);
}

void main() {
	vec2 p = worldPosition;

	p *= 2.0;
	vec3 col;
	vec2 gridP = floor(p);
	vec2 pInGrid = fract(p);
	pInGrid -= 0.5;
	//pInGrid *= 2.0;
	//float gripPHash = hash2to1(gripP);
	//vec2 offset = vec2(gripPHash, fract(gripPHash * 12.1232));
	//p = pInGrid + offset;
	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			vec2 offset = vec2(x, y);
			//offset = vec2(0);
			float n = hash2to1(gridP + offset);
			float t = time * fract(n * 152) * 5.0;
			float size = ((sin(n * t) + 1.0) / 2.0 + 0.1) * fract(n * 122.12) * 0.003;
			size = max(0.001, size);
			size *= 2.0;
			col += star(pInGrid - offset - vec2(n, fract(n * 32.12)), size);
			//col += star(p);
		}
	}

	{
		vec2 p = worldPosition;
		p *= 5;
		float cloud = max(octave01(p * 0.03, 1) - 0.4, 0.0);
		cloud *= octave01(p * 0.2 + time * 0.03, 1);
		vec3 cloudColor;
		{
			cloudColor = mix(vec3(0), vec3(color0), cloud);
		}
		//{
		//	cloud = 1.0 - cloud;
		//	cloudColor = pow(vec3(.1, .7, .8), vec3(10.0 * cloud));
		//	//cloudColor = pow(color0, vec3(10.0 * cloud));
		//}
		cloudColor *= 3.0;
		col += cloudColor;
	}



	fragColor = vec4(col, 1.0);
}

//void main() {
//	vec2 p = worldPosition;
//	p *= 5.0;
//	float d0 = octave01(p * scale0, 2) - 0.3;
//	float d1 = octave01(p * scale1, 2);
//
//	vec3 c0 = vec3(d0 - 0.) * color0;
//	vec3 c1 = vec3(d1) * color1;
//
//	vec3 col = vec3(0.0);
//	col = vec3(d0);
//	d0 = clamp(d0, 0.05, 1.0);
//	//d0 *= d1;
//	col = mix(color0 * 1.5, color1 * 1.5, d1) * d0;
//	//col += c0;
//	//col *= c1;
//	//col += c1 / 2.0;
//	//fragColor = blend(vec4(c0, 0.5), vec4(c1, 0.5));
//	fragColor = vec4(col, 1.0);
//}
