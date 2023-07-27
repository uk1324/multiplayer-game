#version 430 core

uniform float time; 
uniform vec3 color0; 
uniform vec3 borderColor; 
uniform float borderRadius; 

in vec2 worldPosition; 
out vec4 fragColor;

/*generated end*/

#include "Utils/noise.glsl"
#include "Utils/sdf.glsl"
#include "Utils/blend.glsl"

float hash2to1(vec2 p) {
	p = fract(p * vec2(1534.12, 521.23));
	p += dot(p, p + vec2(0.24, 1.26));
	return fract(p.x * p.y);
}

float hash21(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 star(vec2 p, float size) {
	vec3 col;
	float circle = max(0.0, 0.6 - distance(p, vec2(0.0)));
	float d = size / distance(p, vec2(0.0));
	d *= circle;
	col = vec3(d);
	return max(col, 0.0);
}

#define PI 3.14
#define TAU 6.28

mat2 rot(float angle) {
	float s = sin(angle);
	float c = cos(angle);
	return mat2(vec2(s, c), vec2(c, -s));
}

float hexagonSdf(vec2 p) {
	p = abs(p);
	float distanceToVertex = 1.0;
	float a = TAU / 6;
	vec2 sideNormal = vec2(cos(a), sin(a));
	float d = max(p.x, dot(sideNormal, p));
	return d;
}

float sdHexagon( in vec2 p, in float r ) {
	p *= rot(PI / 3);
    const vec3 k = vec3(-0.866025404,0.5,0.577350269);
    p = abs(p);
    p -= 2.0*min(dot(k.xy,p),0.0)*k.xy;
    p -= vec2(clamp(p.x, -k.z*r, k.z*r), r);
    return length(p)*sign(p.y);
}

vec4 hexagonalPosition(vec2 p)
{    
    const vec2 s = vec2(1, sqrt(3.0));
    vec4 hC = floor(vec4(p, p - vec2(.5, 1))/s.xyxy) + .5;
    vec2 a, b;
    a = floor(p / s) + 0.5;
    b = floor((p - vec2(0.5, 1.0)) / s) + 0.5;
    a = p - a * s;
    b = p - (b + 0.5)*s;
    return dot(a, a) < dot(b, b) 
        ? vec4(a, hC.xy) 
        : vec4(b, hC.zw + .5);
}

void main() {
	vec3 col = vec3(0.0);
	{
		vec2 p = worldPosition;
		p *= 2.0;
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

	{
		float scale = 2.0;
		vec2 p = worldPosition * scale;
		float radius = borderRadius * scale;
		vec4 hexagonalPosition = hexagonalPosition(p);
		// Not sure why hexagonalPos is glitched.
		vec2 posInCell = hexagonalPosition.xy;
		vec2 cellPos = hexagonalPosition.zw;

		float d = abs(sdHexagon(posInCell, 0.5)); 
		float borderShape = length(p) - radius;
		d = subtract(d, borderShape);
		d = min(abs(borderShape), d);
		//col = vec3(d);
		vec3 c0 = borderColor;

		if (borderShape < 0) {
			cellPos = vec2(0);
		}
		
		float smoothing = 0.05;
		c0 = mix(vec3(1.0), c0, smoothstep(0.0, smoothing, d));
		//col = c0;

		float opacitySmoothing = smoothing * 4.0;
		float opacity = mix(0.4, 1.0, smoothstep(opacitySmoothing, 0.0, d));
		float n0 = hash2to1(cellPos);
		opacity += 0.2;
		opacity += (sin(time * 1.0 + n0 * 56) + 0.5) / 2.0 * 0.5;
		opacity *= smoothstep(-opacitySmoothing, 0.0, borderShape);
		col = blend(col, c0, opacity);

		
	}

	fragColor = vec4(col, 1.0);
}