#version 430 core

in vec2 position; 

in vec4 color; 
in float smoothing; 
out vec4 fragColor;

/*generated end*/

void main() {
	float d = distance(vec2(0), position);
	d -= 1.0 - smoothing;
	d = smoothstep(smoothing, 0.0 , d);
	fragColor = vec4(color.rgb, d * color.a);
}
