#version 430 core

in vec2 texturePosition; 

in float time; 
in vec3 color; 
out vec4 fragColor;

/*generated end*/

#include "Utils/noise.glsl"
#include "Utils/colorConversion.glsl"

mat2 rot(float angle) {
	float s = sin(angle);
	float c = cos(angle);
	return mat2(vec2(s, c), vec2(c, -s));
}

float star(vec2 p) {
	float d = abs(p.x) * abs(p.y) * 200.0;
	d = max(0.0, 1.0 - d);
	d *= 1.0 - min(length(p) * 2.0, 1.0);
	return d;
}



void main() {
	vec2 p = texturePosition;
	vec3 col;

	//float a = 0.99;
	//d = smoothstep(a, a + 0.01, p.x);
	//float a = atan(p.y, p.x);
	
	//d = 0.02 / dot(p, p);
	//d = clamp(d, 0.0, 1.0);
	//d *= 2.0;
	//d = smoothstep(0.0, 0.1, d);
	//d *= octave01(p * 3.0 + time, 2);
	//d += 1.0 - ;
	//float d = star(p);
	//d *= star(rot(3.14 / 4.0) * p);
	//d = max(star(rot(3.14 / 4.0) * p), d);
	//d *= star(rot(3.14 / 4.0) * p);
	//min(d, star(rot(3.14 / 4.0) * p));
	//d = p.y;

	//float s = sin(time * 5.0);
	//s += 1.0;
	//s /= 2.0;
	//float d = (0.005) / dot(p, p);
	////d += s * 0.03;
	////d += s * perlin01(time) * 0.1;
	//d *= 1.0 - min(length(p) * 2.0, 1.0);
	//col = vec3(d);
	//float t = 0.0;
	//col = mix(hsv2rgb(vec3(t, 1.0, 1.0)), vec3(1), d);

	// r^2 / x^2 - inverse square
	// For which r is the falloff not visible at x = 1.
	// Not visible at opacity around 0.01.
	// r^2 / 1^2 = 0.01
	// r^2 = 0.01
	// r = 0.1
	float above1radius = 0.1;
	float d = (above1radius * above1radius) / dot(p, p);
	d *= max(0.0, 1.0 - smoothstep(0.5, 1.0, length(p)));
	float t = 0.0;
	col = mix(color, vec3(1), d);

	// Box
	//if (abs(p.x) > 0.98 || abs(p.y) > 0.98) {
	//	col = vec3(1.0);
	//	d = 1.0;
	//}

	//col = vec3(d);
	//d += s * 0.03;
	//d += s * perlin01(time) * 0.1;
	//d *= 1.0 - min(length(p) * 2.0, 1.0);
	//col = vec3(d);
	//float t = 0.0;
	//col = mix(hsv2rgb(vec3(t, 1.0, 1.0)), vec3(1), d);


	//d = abs(p.x * 5.0) * abs(p.y * 5.0) * 200.0 * (s + 0.9);
	//d = smoothstep(1.0, 0.0, d);
	//d *= 1.0 - length(p);
	//col = vec3(d);
	//fragColor = vec4(col, d);

	//float s = sin(time * 5.0);
	//s += 1.0;
	//s /= 2.0;
	//float d = (0.1) / dot(p, p);
	//float d = 0.01 / dot(p, p);
	//d /= 2.0;

	//d *= 1.0 - min(length(p) * 2.0, 1.0);
	//col = vec3(d);
	//col = mix(hsv2rgb(vec3(time, 1.0, 1.0)), vec3(1), d);
	fragColor = vec4(col, clamp(d, 0.0, 1.0));

	//fragColor = vec4(col, 1.0);
}
