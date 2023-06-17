#version 430 core

uniform vec4 color; 

in vec2 texturePosition; 

in float time; 
out vec4 fragColor;

/*generated end*/

float circleSdf(vec2 p, vec2 center, float r) {
	return distance(center, p) - r;
}

float intersect(float d0, float d1){
    return max(d0, d1);
}

float subtract(float base, float subtraction){
    return intersect(base, -subtraction);
}

float ringSdf(vec2 p, vec2 center, float r0, float width) {
	return subtract(circleSdf(p, center, r0), circleSdf(p, center, r0 - width));
}

#define HASHSCALE 0.1031

float hash(float p)
{
	vec3 p3  = fract(vec3(p) * HASHSCALE);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

float fade(float t) { return t*t*t*(t*(6.*t-15.)+10.); }

float grad(float hash, float p)
{
    int i = int(1e4*hash);
	return (i & 1) == 0 ? p : -p;
}

float perlin(float p)
{
	float pi = floor(p), pf = p - pi, w = fade(pf);
    return mix(grad(hash(pi), pf), grad(hash(pi + 1.0), pf - 1.0), w) * 2.0;
}

float perlin01(float p)
{
	return (perlin(p) + 1) * 0.5;
}

#define M_PI 3.14159265358979323846

float rand(vec2 co){return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);}
float rand (vec2 co, float l) {return rand(vec2(rand(co), l));}
float rand (vec2 co, float l, float t) {return rand(vec2(rand(co, l), t));}

float perlin(vec2 p, float dim, float time) {
	vec2 pos = floor(p * dim);
	vec2 posx = pos + vec2(1.0, 0.0);
	vec2 posy = pos + vec2(0.0, 1.0);
	vec2 posxy = pos + vec2(1.0);
	
	float c = rand(pos, dim, time);
	float cx = rand(posx, dim, time);
	float cy = rand(posy, dim, time);
	float cxy = rand(posxy, dim, time);
	
	vec2 d = fract(p * dim);
	d = -0.5 * cos(d * M_PI) + 0.5;
	
	float ccx = mix(c, cx, d.x);
	float cycxy = mix(cy, cxy, d.x);
	float center = mix(ccx, cycxy, d.y);
	
	return center * 2.0 - 1.0;
}

// p must be normalized!
float perlin(vec2 p, float dim) {
	
	vec2 pos = floor(p * dim);
	vec2 posx = pos + vec2(1.0, 0.0);
	vec2 posy = pos + vec2(0.0, 1.0);
	vec2 posxy = pos + vec2(1.0);
		
	float c = rand(pos, dim);
	float cx = rand(posx, dim);
	float cy = rand(posy, dim);
	float cxy = rand(posxy, dim);
	
	vec2 d = fract(p * dim);
	d = -0.5 * cos(d * M_PI) + 0.5;
	
	float ccx = mix(c, cx, d.x);
	float cycxy = mix(cy, cxy, d.x);
	float center = mix(ccx, cycxy, d.y);
	
	return center * 2.0 - 1.0;
	return perlin(p, dim, 0.0);
}

vec2 hash(vec2 p)
{
	p = vec2( dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)) );
	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float perlin(vec2 p)
{
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;

	vec2  i = floor( p + (p.x+p.y)*K1 );
    vec2  a = p - i + (i.x+i.y)*K2;
    float m = step(a.y,a.x); 
    vec2  o = vec2(m,1.0-m);
    vec2  b = a - o + K2;
	vec2  c = a - 1.0 + 2.0*K2;
    vec3  h = max( 0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
	vec3  n = h*h*h*h*vec3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));
    return dot( n, vec3(70.0) );
}

float perlin01(vec2 p)
{
	return (perlin(p) + 1) * 0.5;
}

float octave01(vec2 p, int octaves) {
	int OCTAVES = 4;
    float amplitude = .5;
    float frequency = 0.;
	float value = 0.0;
    for (int i = 0; i < OCTAVES; i++) {
        value += amplitude * perlin01(p);
        p *= 2.;
        amplitude *= .5;
    }
	return value;
}

void main() {
	vec2 p = texturePosition;
	p -= vec2(0.5);
	p *= 2.0;

	float t = time;

	float d;
	float l0 = 0.5;
	float l1 = 0.5;
	float a;
	float smoothing = 0.01;
	if (t < l0) {
		t /= l0;
		t = 1.0 - t;
		d = ringSdf(p, vec2(0.0), 0.5 * t, 0.05 * t) - perlin01(p * 7.0) * 0.05;
		d = smoothstep(smoothing, 0.0, d);	
		a = 1.0 - t;
	} else {
		t -= l0;
		t /= l1;
		d = circleSdf(p, vec2(0.0), 1.0 * t - smoothing) - perlin01(p * 7.0) * 0.05;
		d = smoothstep(smoothing, 0.0, d);
		a = 1.0 - t;
	}

	vec3 col = color.rgb * max(0.5, octave01(p * 4.0, 3));
	//vec3 color = vec3(0.506, 0.204, 0.255) * max(0.5, octave01(p * 4.0, 3));
	fragColor = mix(vec4(col, 0.0), vec4(col, 1.0), a) * d;
}
