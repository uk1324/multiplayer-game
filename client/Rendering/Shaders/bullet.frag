#version 430 core

in vec2 texturePosition; 

in vec3 color; 
out vec4 fragColor;

/*generated end*/

void main() {
	float d = length(texturePosition);
	d = smoothstep(1.0, 0.4, d);
	vec3 col;
	col = mix(color, vec3(1.0), d);
	fragColor = vec4(col, d);
}
