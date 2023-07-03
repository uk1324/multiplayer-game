#version 430 core

in vec2 position; 

in vec4 color; 
in float smoothing; 
in float lineWidth; 
in float lineLength; 
out vec4 fragColor;

/*generated end*/

#include "Utils/colorConversion.glsl"

void main() {
	//float d = dot(vec2(0.0, 1.0), position);;
	//d += 1.0;
	//d = abs(d);
	float len = lineLength;
	//len -= lineWidth;
	float x = position.x * len;
	float d;
	if (x < lineWidth || x > len - lineWidth) {
		//fragColor = vec4(vec3(1.0, 0.0, 0.0), 1);
		float clampedX = clamp(x, lineWidth, len - lineWidth);
		d = distance(vec2(x, position.y * lineWidth), vec2(clampedX, 0.0));
	} else {
		d = abs(position.y) * lineWidth;
		//fragColor = vec4(vec3(0.0, 1.0, 0.0), 1);
	}
	d /= lineWidth;
	d -= 1.0 - smoothing;
	d = smoothstep(smoothing, 0.0, d);
	//d -= 0.2;
	//d -= 0.01;
	//d = smoothstep(lineWidth + 0.01, lineWidth, d);
	//d = distance(vec2(x, position.y * lineWidth), vec2(lineWidth, 0.0));
	//d -= 0.2; 
	//d = smoothstep(0.0, 0.2, d);
	fragColor = vec4(color.rgb, d);
	//fragColor = vec4(color.rgb, d * color.a);
	//x = position.x;
	//fragColor = vec4(hsv2rgb(vec3(x, 1.0, 1.0)), 1);
	//fragColor = vec4(vec3(position, 0.0), 1);
	//fragColor = vec4(vec3(position, 0.0), 1);
	//fragColor = vec4(vec3(d), 1);
}
