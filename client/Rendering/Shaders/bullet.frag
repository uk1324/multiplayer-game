#version 430 core

in vec2 texturePosition; 

in vec4 color; 
out vec4 fragColor;

/*generated end*/

void main() {
	float d = length(texturePosition);
	d = smoothstep(1.0, 0.4, d);
	vec3 col;
	col = mix(color.rgb, vec3(1.0), d);
	fragColor = vec4(col, d * color.a);
}
