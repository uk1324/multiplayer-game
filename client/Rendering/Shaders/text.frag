#version 430 core

in vec2 texturePosition; 

in float smoothing; 
out vec4 fragColor;

/*generated end*/

uniform sampler2D fontAtlas;

#include "Utils/noise.glsl"

uniform float time;

void main() {
	fragColor = texture2D(fontAtlas, texturePosition);
	return;
	fragColor.x -= 0.5;
	vec2 p = texturePosition;
	p.x += time / 15.0;
	fragColor.x += octave01(p * 25.0, 4) * 0.5;
	fragColor.x -= 0.1;
	fragColor = vec4(vec3(fragColor.x), 1.0);
	float d = smoothstep(0.0, 0.05, fragColor.x);
	fragColor = vec4(vec3(d), d);
	//fragColor = vec4(vec3(1), 1);

	//float dist = texture2D(fontAtlas, texturePosition).r;
	//float d = dist;
	////d = abs(d);
	//d -= 0.1;
	//d += octave01(texturePosition * 25.0 + time, 4) * 0.2;
	////d = abs(d);
	//float smoothing = 0.1;
	//d = smoothstep(0.0, 0.4, d);
	//if (d == 1.0) {
	//	fragColor = vec4(1.0, 0.0, 0.0, 1.0);
	//} else {
	//	vec3 c = vec3(1,1./4.,1./16.) * exp(4.*d - 1.);
	//	fragColor = vec4(c, d);
	//}

	//float d = texture2D(fontAtlas, texturePosition).r;
	////vec3 c = vec3(1,1./4.,1./16.) * exp(4.*d - 1.);
	//fragColor = vec4(vec3(mod(d + time * 1, 1.0)), d);

	//float d = texture2D(fontAtlas, texturePosition).r;
	//fragColor = vec4(vec3(d), d);

	//if (d - 0.5 < 0) {
	//	fragColor = vec4(vec3(d), 1.0);
	//} else {
	//	fragColor = vec4(vec3(1), 1.0);
	//}
	//d -= 0.5;
	//d *= 15.0;
	//d = smoothstep(0.0, 0.05, d);
	//d -= 0.5;
}
