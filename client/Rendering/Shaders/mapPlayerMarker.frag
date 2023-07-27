#version 430 core

in vec2 position; 

in vec3 color; 
out vec4 fragColor;

/*generated end*/

void main() {
	vec2 p = position;
	p.x = abs(p.x);
	p.y += 0.7;
	float smoothing = 0.15;
	float d = dot(p, normalize(vec2(-1.0, 1.0)));
	d = min(d, 1.0 - p.y);
	//float d = max(p.y, );
	//float d = length(position);
	//d -= 1.0 - smoothing;
	d = smoothstep(0.0, smoothing, d);
	//fragColor = vec4(vec3(d), 1.0);
	fragColor = vec4(vec3(1.0, 0.0, 0.0), d * 0.7);
}
