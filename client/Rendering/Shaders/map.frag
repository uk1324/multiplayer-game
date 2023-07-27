#version 430 core

in vec2 position; 
out vec4 fragColor;

/*generated end*/

#include "Utils/sdf.glsl"
#include "Utils/blend.glsl"

void main() {
	float smoothing = 0.02;

	vec2 p = position;
	float d = ringSdf(p, vec2(0.0), 1.0 - smoothing, 0.03);

	float a;
	vec3 col;

	d = smoothstep(smoothing, 0.0, d);
	float outlineA = d;
	vec3 outlineCol = d * vec3(0, 1, 1) * 0.7;

	d = 1.0 - step(0.0, length(p) - 1.0 + smoothing);
	vec3 insideCol = d * vec3(0, 1, 1);
	float insideA = min(d, 0.3);

	a = max(outlineA, insideA);
	col = outlineCol;
	col = blend(insideCol, outlineCol, outlineA);


	fragColor = vec4(col, a);
}
