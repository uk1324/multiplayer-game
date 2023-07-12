#version 430 core

in vec2 texturePosition; 

in vec3 color; 
out vec4 fragColor;

/*generated end*/

void main() {
	vec2 p = texturePosition;
	vec3 col;
	// r^2 / x^2 - inverse square
	// For which r is the falloff not visible at x = 1.
	// Not visible at opacity around 0.01.
	// r^2 / 1^2 = 0.01
	// r^2 = 0.01
	// r = 0.1
	float above1radius = 0.1;
	float d = (above1radius * above1radius) / dot(p, p);
	d *= max(0.0, 1.0 - smoothstep(0.5, 1.0, length(p)));
	col = mix(color, vec3(1), d);

	fragColor = vec4(col, clamp(d, 0.0, 1.0));

}
