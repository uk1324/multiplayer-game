#version 430 core

in vec2 texturePosition; 

in float smoothing; 
in vec4 color; 
out vec4 fragColor;

/*generated end*/

uniform sampler2D fontAtlas;

#include "Utils/noise.glsl"

// Read: "Font rendering anti aliasing methods comparison demo" commit
void main() {
	float d = texture(fontAtlas, texturePosition).r;
	float smoothingWidth = min(smoothing, 0.49);
	d -= 0.5 - smoothingWidth;
	vec2 p = texturePosition;
	d = smoothstep(0.0, smoothingWidth, d);
	fragColor = vec4(vec3(d), d) * color;
}
