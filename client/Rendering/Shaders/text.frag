#version 430 core

in vec2 texturePosition; 

in float smoothing; 
out vec4 fragColor;

/*generated end*/

uniform sampler2D fontAtlas;

#include "Utils/noise.glsl"

uniform bool show;
uniform int mode;

vec2 safeNormalize( in vec2 v )
{
   float len = length( v );
   len = ( len > 0.0 ) ? 1.0 / len : 0.0;
   return v * len;
}

// Not sure what is the best way to do anti aliasing.
// https://github.com/Chlumsky/msdfgen/issues/36 "width(sigDist) seems to be the wrong AA measure for MSDF"
// https://github.com/Chlumsky/msdfgen/issues/22 "Improved anti-aliasing. #22"
// https://discourse.libcinder.org/t/cinder-sdftext-initial-release-wip/171/27
void main() {
	//fragColor = texture2D(fontAtlas, texturePosition);
	//float smoothingWidth = smoothing;
	////float smoothingWidth = 1.0 / smoothing;
	////smoothingWidth *= 0.05;
	////smoothingWidth = 0.0; 
	////smoothingWidth = min(smoothingWidth, 0.2);
	//float f = fragColor.x - 0.5;
	//fragColor.x -= 0.5 - smoothingWidth;
	//vec2 p = texturePosition;
	////float d = smoothstep(0.0, smoothing * 0.001, fragColor.x);
	////float d = smoothstep(0.0, 1.0 / smoothing, fragColor.x);
	//float d = smoothstep(0.0, smoothingWidth, fragColor.x);
	//fragColor = vec4(vec3(d), d);
	////if (f > -smoothingWidth && f <= 0.0) {
	////	fragColor = vec4(vec3(1.0, 0.0, 0.0), 1.0);	
	////}

	// fwidth artifacts
	//float dst = texture2D(fontAtlas, texturePosition).r;
	//dst -= 0.5;
	//float aaf = fwidth(dst) * 15.0;
	//float alpha = smoothstep(0.0 - aaf, 0.0, dst);
	//vec4 color = vec4(vec3(1, 1, 1), alpha);
	//fragColor = color;

	//fragColor = texture2D(fontAtlas, texturePosition);
	//float smoothingWidth = smoothing;
	//float f = fragColor.x - 0.5;
	//smoothingWidth = min(smoothingWidth, 0.49);
	//fragColor.x -= 0.5 - smoothingWidth;
	//vec2 p = texturePosition;
	//float d = smoothstep(0.0, smoothingWidth, fragColor.x);
	//fragColor = vec4(vec3(d), d);
	//
	//if (show && f > -smoothingWidth && f <= 0.0) {
	//	fragColor = vec4(vec3(1.0, 0.0, 0.0), 1.0);	
	//}

    // sdf distance from edge (scalar)
    //float dist = texture2D(fontAtlas, texturePosition).r - 0.5;
    //vec2 ddist = vec2(dFdx(dist), dFdy(dist)) * 5.0;
    //float pixelDist = dist / length(ddist);
    //fragColor = vec4(vec3(1), (pixelDist)); 

	//float d = texture2D(fontAtlas, texturePosition).r - 0.5;
	//float derivX = dFdx(texturePosition.x);
	//float derivY = dFdy(texturePosition.y);
	//float gradientLength = length(vec2(derivX, derivY)) * 15.0;
	//d -= gradientLength;
	//d = smoothstep(0.0, gradientLength, d);
	////float thresholdWidth = 2.0 * gradientLength;
	////float d = clamp((distFromEdge / thresholdWidth) + 0.5, 0.0, 1.0);
	//fragColor = vec4(vec3(1), (d)); 

	if (mode == 0) {
		fragColor = texture2D(fontAtlas, texturePosition);
		float smoothingWidth = smoothing;
		float f = fragColor.x - 0.5;
		smoothingWidth = min(smoothingWidth, 0.49);
		fragColor.x -= 0.5 - smoothingWidth;
		vec2 p = texturePosition;
		float d = smoothstep(0.0, smoothingWidth, fragColor.x);
		fragColor = vec4(vec3(d), d);
	
		if (show && f > -smoothingWidth && f <= 0.0) {
			fragColor = vec4(vec3(1.0, 0.0, 0.0), 1.0);	
		}
	} else if (mode == 1) {
		float distFromEdge = texture2D(fontAtlas, texturePosition).r - 0.5;
		float derivX = dFdx(distFromEdge);
		float derivY = dFdy(distFromEdge);
		float gradientLength = length(vec2(derivX, derivY));
		float thresholdWidth = 2.0 * gradientLength;
		float d = clamp((distFromEdge / thresholdWidth) + 0.5, 0.0, 1.0);
		fragColor = vec4(vec3(1), (d)); 
	} else if (mode == 2) {
		float sampl = texture(fontAtlas, texturePosition).r;
		ivec2 sz = textureSize( fontAtlas, 0 );
		float dx = dFdx( texturePosition.x ) * sz.x;
		float dy = dFdy( texturePosition.y ) * sz.y;
		float toPixels = 8.0 * inversesqrt( dx * dx + dy * dy );
		////float sigDist = median( sampl.r, sampl.g, sampl.b ) - 0.5;
		float sigDist = sampl - 0.5;
		float opacity = clamp( sigDist * toPixels + 0.5, 0.0, 1.0 );
		fragColor = vec4(vec3(1), opacity);
	} else if (mode == 3) {
		float d = texture2D(fontAtlas, texturePosition).r - 0.5;
		float dx = dFdx(d);
		float dy = dFdy(d);
		fragColor = vec4(vec3(length(vec2(dx, dx))), 1.0);
	} else if (mode == 4) {
		// Probably doesn't work when rotated.
		ivec2 sz = textureSize( fontAtlas, 0 );
		float dx = dFdx(texturePosition.x) * sz.x;
		float dy = dFdy(texturePosition.y) * sz.y;
		fragColor = vec4(vec3(length(vec2(dx, dx))), 1.0);
	} else if (mode == 5) {
	vec2 uv = texturePosition * textureSize(fontAtlas, 0 );
		// Calculate derivates
		vec2 Jdx = dFdx( uv );
		vec2 Jdy = dFdy( uv );
		// Sample SDF texture (3 channels).
		vec3 sampl = texture( fontAtlas, texturePosition).rgb;
		// calculate signed distance (in texels).
		//float sigDist = median( sampl .r, sampl .g, sampl .b ) - 0.5;
		float sigDist = sampl.r - 0.5;
		// For proper anti-aliasing, we need to calculate signed distance in pixels. We do this using derivatives.
		vec2 gradDist = safeNormalize( vec2( dFdx( sigDist ), dFdy( sigDist ) ) );
		vec2 grad = vec2( gradDist.x * Jdx.x + gradDist.y * Jdy.x, gradDist.x * Jdx.y + gradDist.y * Jdy.y );
		// Apply anti-aliasing.
		const float kThickness = 0.125;
		const float kNormalization = kThickness * 0.5 * sqrt( 2.0 );
		float afwidth = min( kNormalization * length( grad ), 0.5 );
		float opacity = smoothstep( 0.0 - afwidth, 0.0 + afwidth, sigDist );
		// Apply pre-multiplied alpha with gamma correction.
		//Color.a = pow( uFgColor.a * opacity, 1.0 / 2.2 );
		//Color.rgb = uFgColor.rgb * Color.a;
		fragColor = vec4(vec3(1), opacity);
	}

	
}
