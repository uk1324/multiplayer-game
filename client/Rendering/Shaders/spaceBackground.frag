#version 430 core

uniform float time; 
uniform vec3 color0; 
uniform float scale0; 
uniform vec3 color1; 
uniform float scale1; 
uniform vec3 borderColor; 
uniform float borderRadius; 

in vec2 worldPosition; 
out vec4 fragColor;

/*generated end*/

#include "Utils/noise.glsl"
#include "Utils/sdf.glsl"

vec3 blend(vec3 old, vec3 new, float newAlpha) {
    return old * (1.0 - newAlpha) + new * newAlpha;
}

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

struct HexagonalPos {
	vec2 posInCell;
	vec2 cellPos;
};

// xy = position in cell, zw = cell position
HexagonalPos hexagonalPos(vec2 p, float radius) {
	p /= radius;
	vec2 cellSize = vec2(1.0, sqrt(3));
	vec2 halfCellSize = cellSize / 2.0;
	// Subtract halfSize  outside from both to center.

	vec2 a = mod(p, cellSize) - halfCellSize;
	vec2 b = mod(p - halfCellSize /* The center of the neighbouring hexagon is aligned with the side of this hexagon = myCenter + halfSize */, cellSize) - halfCellSize;

	//vec2 s = cellSize;
	//vec4 k = floor(vec4(p, p - vec2(0.5, 1)) / s.xyxy) + .5;
	//vec4 k2 = floor(vec4(p, p - vec2(0.5, 1)) / s.xyxy) + .5;
	//
	//vec2 a = (floor(vec4(p, p - vec2(0.5, 1)) / s.xyxy)).xy;
	//a = (k).xy;
	//vec2 b = (floor(vec4(p, p - vec2(0.5, 1)) / s.xyxy)).zw;
	//vec4 hC = floor(vec4(p, p - vec2(0.5, 1)) / s.xyxy) + .5;
	//vec2 a = hC.xy;
	//vec2 b = hC.zw;
	//a = p - a * s;
	//b = p - (b + 0.5) * s;
	//vec4 hC = floor(vec4(p, p - vec2(.5, 1))/s.xyxy) + .5;
    // Centering the coordinates with the hexagon centers above.

    //vec4 h = vec4(p - hC.xy*s, p - (hC.zw + .5)*s);
	//vec2 a = h.xy;
	//vec2 b = h.zw;

	// The distance between the centers of hexagons is equal in all directions so just take the min.
	vec2 posInCell = dot(a, a) <= dot(b, b) ? a : b;
	vec2 cellPos = p - posInCell;
	HexagonalPos result;
	result.posInCell = posInCell;
	result.cellPos = cellPos;
	return result;
}


//vec4 getHex(vec2 p)
//{    
//    const vec2 s = vec2(1, sqrt(3.0));
//    //vec4 hC = floor(vec4(p, p - vec2(0.5, 1.0)) / s.xyxy) + 0.5;
//    //vec2 n0 = hC.xy;
//    //vec2 n1 = hC.zw;
//
//	vec4 hC = floor(vec4(p, p - vec2(0.5, 1.0)) / s.xyxy) + 0.5;
//    vec2 n0 = hC.xy;
//    vec2 n1 = hC.zw;
//
//    vec4 h = vec4(p - n0*s, p - (n1 + .5)*s);
//    
//    vec2 a = h.xy;
//    vec2 b = h.zw;
//   
//    //return dot(a, a) < dot(b, b) 
//    //    ? vec4(a, hC.xy) 
//    //    : vec4(b, hC.zw);
//
//	return dot(a, a) < dot(b, b) 
//        ? vec4(a, p - a) 
//        : vec4(b, p - b);
//}

vec4 getHex(vec2 p)
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

vec4 HexCoords(vec2 uv) {
	vec2 r = vec2(1, 1.73);
    vec2 h = r*.5;
    
    vec2 a = mod(uv, r)-h;
    vec2 b = mod(uv-h, r)-h;
    
    vec2 gv = dot(a, a) < dot(b,b) ? a : b;

    vec2 id = uv-gv;
    return vec4(gv, id.x,id.y);
}

#include "Utils/colorConversion.glsl"

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
		//vec2 hexagonAabbSize = vec2(1.0, sqrt(3));
		//vec2 halfHexagonAabbSize = hexagonAabbSize / 2.0;
		//// Subtract halfSize  outside from both to center.
		//vec2 a = mod(p, hexagonAabbSize) - halfHexagonAabbSize;
		//vec2 b = mod(p - halfHexagonAabbSize /* The center of the neighbouring /hexagon/ is aligned with the side of this hexagon = myCenter + halfSize /*/, /hexagonAabbSize) - halfHexagonAabbSize;
		//// The distance between the centers of hexagons is equal in all directions /so /just take the min.
		//vec2 posInCell = dot(a, a) <= dot(b, b) ? a : b;
		//vec2 cellPos = p - posInCell;

		//p = abs(cellPos) / 20.0;
		//float d = hexagonSdf(posInCell);
		//float distanceToSide = hexagonAabbSize.x / 2.0;
		//d = distanceToSide - d;


		//float d = (0.25 - sdHexagon(posInCell, hexagonAabbSize.x / 4)) / 4.0; 
		//col = vec3(d);
		//vec3 c0 = borderColor;
		////col = vec3(d);
		//float smoothing = 0.05;
		//c0 = mix(vec3(1.0), c0, smoothstep(0.0, smoothing, d));
		//col = blend(col, c0, mix(0.1, 1.0, smoothstep(smoothing * 2.0, 0.0, d)));
		HexagonalPos h = hexagonalPos(p, 1.0);
		vec4 n = getHex(p);
		// Not sure why hexagonalPos is glitched.
		h.posInCell = n.xy;
		h.cellPos = n.zw;

		float d = abs(sdHexagon(h.posInCell, 0.5)); 
		//d = mix(d, 1.0, smoothstep(0.0, 0.1, -length(p) - 0.5));
		//float circle = length(p) - 2;
		//float circle = length(p) - 2;
		//float borderShape = sdHexagon(p, 2.0);
		float borderShape = length(p) - radius;
		d = subtract(d, borderShape);
		d = min(abs(borderShape), d);
		//col = vec3(d);
		vec3 c0 = borderColor;

		float smoothing = 0.05;
		c0 = mix(vec3(1.0), c0, smoothstep(0.0, smoothing, d));
		//col = c0;

		float opacitySmoothing = smoothing * 4.0;
		float opacity = mix(0.4, 1.0, smoothstep(opacitySmoothing, 0.0, d));
		//opacity += 0.3 * sin(time * 5.0 + hash2to1(h.cellPos) * 241.21) * hash2to1(h.cellPos) * 0.2;
		float n0 = hash2to1(h.cellPos);
		float n1 = fract(n0 * 123.23);
		//opacity += (hash2to1(h.cellPos) * 0.3 + ((sin(time * 1.0 + n1 * 56)) + 0.2) / 2.0 * 0.3) / 1.0;
		//opacity += hash2to1(h.cellPos) * 0.3;
		opacity += 0.2;
		opacity += (sin(time * 1.0 + n1 * 56) + 0.5) / 2.0 * 0.5;
		opacity *= smoothstep(-opacitySmoothing, 0.0, borderShape);
		col = blend(col, c0, opacity);
		//col = vec3(hash21(h.cellPos));
		//col = vec3(d);
		//col = vec3(h.cellPos, 0.0);


		//d = smoothstep(0.05, 0.0, d);
		//c0 = mix(c0, vec3(1.0), d);
		//d = mix(0.3, 1.0, d);
		//d *= smoothstep(1.0, 1.01, length(worldPosition));
		////col += (smoothstep(1.0, 1.1, length(p))) * c0;
		////col = vec3(d);
		//col = blend(col, c0, d);

		//d = clamp(d, -1.0, 1.0);
		//d = sqrt(d);
		//col = vec3(d);
		//d /= 0.5;
		//vec3 c = mix(vec3(1), vec3(13, 44, 84) / 255, d);
		//vec3 c = mix(vec3(1.0), c0, d);
		
		//if (d == 1.0) {
		//	c = c0;
		//	d = 0.1;
		//}
		//float n = hash2to1(cellPos);
		//d = mix(1.0, n, clamp(d, 0.0, 1.0));
		//col = vec3(d);

		//col = vec3(cellPos, 0.0);
		//col = hsv2rgb(vec3(cellPos.y / 12.0, 1.0, abs(cellPos.x)));
		//col = vec3(posInCell, 0.0);
		//col = vec3(cellPos / 2, 0.0);
		//col = c0;
		//col = vec3(d);

		//p = a;
		 //float d = hexagonSdf(p);
		 //
		 // Different hexagons lighting on and off.
		 // vec2(atan, d)
	}

	fragColor = vec4(col, 1.0);
}