#version 430 core

in vec2 worldPos;

out vec4 fragColor;

uniform vec2 line[50];
uniform int lineLength;

float lineSegmentSdf(vec2 p, vec2 start, vec2 end) {
    vec2 t = normalize(end - start);
    vec2 n = vec2(-t.y, t.x);
    float ld = dot(n, start);
    float d = dot(p, n) - ld;
    float dAlong = dot(p, t);
    float dAlongStart = dot(start, t);
    float dAlongEnd = dot(end, t);
    float along = clamp(dAlong, dAlongStart, dAlongEnd);
    vec2 cloestPointOnLine = along * t + ld * n;
    return distance(p, cloestPointOnLine);
}


float distanceAlong(vec2 p, vec2 start, vec2 end) {
    vec2 t = normalize(end - start);
    vec2 n = vec2(-t.y, t.x);
    float ld = dot(n, start);
    float d = dot(p, n) - ld;
    float dAlong = dot(p, t);
    float dAlongStart = dot(start, t);
    return dAlong - dAlongStart;
}

vec4 blend(vec4 src, vec4 dest)
{
	return vec4(src.a * src.rgb + (1 - src.a) * dest.rgb, 1);
}

void main() {
	float cameraZoom = 0.5;
	float smallCellSize = 0.1;
	float halfSmallCellSize = smallCellSize / 2.0;
	// Translate so every cell contains a cross of lines instead of a square. When a square is used it contains 4 lines which makes coloring specific lines difficult. Centering it makes it so there are only 2 lines in each cell.
	vec2 pos = worldPos + vec2(halfSmallCellSize, halfSmallCellSize);
	vec2 posInCell = mod(abs(pos), smallCellSize) - vec2(halfSmallCellSize, halfSmallCellSize); // Translate [0, 0] to center of cell.

	float dVertical = abs(dot(posInCell, vec2(1.0, 0.0)));
	float dHorizontal = abs(dot(posInCell, vec2(0.0, 1.0)));
	float width = 0.0015 / cameraZoom;
	float interpolationWidth = width / 5.0f;
	// Flip colors by making the second argument to smoothstep smaller than the first one.
	dVertical = smoothstep(width, width - interpolationWidth, dVertical);
	dHorizontal = smoothstep(width, width - interpolationWidth, dHorizontal);

	vec3 col0 = vec3(0.12, 0.12, 0.12);
	vec3 col1 = col0 / 2.0;
	ivec2 cellPos = ivec2(floor(pos / smallCellSize));
	vec3 colVertical = (cellPos.x % 4 == 0) ? col0 : col1;
	vec3 colHorizontal = (cellPos.y % 4 == 0) ? col0 : col1;
	colVertical *= dVertical;
	colHorizontal *= dHorizontal;

	//fragColor = vec4(cameraPos, 0.0f, 1.0);
	vec4 c = vec4(max(colVertical, colHorizontal), 1.0);
	fragColor = vec4(max(colVertical, colHorizontal), 1.0);

	//vec2 p = worldPos;
	//float d = 1000.0;
	//float distanceAlong;
	//for (int i = 0; i < lineLength - 1; i++) {
	//	vec2 current = line[i].xy;
	//	vec2 next = line[i + 1].xy;
	//	d = min(d, lineSegmentSdf(p, current, next));
	//	//d = min(d, distance(current, p));
	//}
	////float ad = smoothstep(0.0, 0.01, d - 0.1);
	////ad = 1.0 - ad;
	////d = ad;
	//d;
	//d = smoothstep(0.0, 0.4, d);
	//d = 1.0 - d;
	//
	//vec4 col = vec4(vec3(d), d);
	////fragColor = vec4(vec3(d), 1.0);
	//fragColor = col;
}