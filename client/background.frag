#version 430 core

in vec2 worldPos;

out vec4 fragColor;

// '%' for netagive numbers is undefined.
// https://www.gamedev.net/forums/topic/676242-glsl-modulo-strange-behavior-with-negative-number/5277715/
// In this implementation it seems to work correctly only for negative values with are negated powers of 2.
int rem(int x, int y) {
	return abs(x) % y;
	//return x - y * int(floor(float(x) / float(y)));
}

#include "Rendering/Shaders/Utils/noise.glsl"

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
	int smallCellsInABigCell = 5;
	vec3 colVertical = (rem(cellPos.x, smallCellsInABigCell) == 0) ? col0 : col1;
	vec3 colHorizontal = (rem(cellPos.y, smallCellsInABigCell) == 0) ? col0 : col1;
	colVertical *= dVertical;
	colHorizontal *= dHorizontal;

	vec4 c = vec4(max(colVertical, colHorizontal), 1.0);
	fragColor = vec4(max(colVertical, colHorizontal), 1.0);
}