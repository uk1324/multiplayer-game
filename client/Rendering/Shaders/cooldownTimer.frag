#version 430 core

in vec2 position; 

in float t; 
in vec2 size; 
out vec4 fragColor;

/*generated end*/

float sdParallelogram(in vec2 p, float wi, float he, float sk) {
    vec2  e  = vec2(sk,he);
    float e2 = sk*sk + he*he;

    p = (p.y<0.0)?-p:p;
    // horizontal edge
    vec2  w = p - e; w.x -= clamp(w.x,-wi,wi);
    vec2  d = vec2(dot(w,w), -w.y);
    // vertical edge
    float s = p.x*e.y - p.y*e.x;
    p = (s<0.0)?-p:p;
    vec2  v = p - vec2(wi,0); v -= e*clamp(dot(v,e)/e2,-1.0,1.0);
    d = min( d, vec2(dot(v,v), wi*he-abs(s)));
    return sqrt(d.x)*sign(-d.y);
}

// TODO: Dynamic smoothing.
void main() {
    float width = size.x / size.y;
	vec2 p = vec2(position.x * width, position.y);
    //vec2 p = position;
    float smoothing = 0.05;
    float skew = 0.2;
    float outerWidth = width - skew * 2.0 - smoothing;
    float outerHeight = 1.0 - smoothing;

	float outerD = sdParallelogram(p, outerWidth, outerHeight, skew);
    outerD = smoothstep(smoothing, 0.0, outerD);
    vec3 outerColor = vec3(65, 62, 56) / 255;
    vec3 color = vec3(outerD) * outerColor;
    float alpha = outerD;

    float innerHeight = outerHeight - 0.2;
    vec2 parallelogramSideVector = -normalize(vec2(skew, outerHeight));
    vec2 parallelogramSideNormal = vec2(-parallelogramSideVector.y, parallelogramSideVector.x);
    float innerWidth = outerWidth - 0.4;
    float activeD = sdParallelogram(p, innerWidth, innerHeight, skew * (innerHeight / outerHeight));
    activeD = smoothstep(smoothing, 0.0, activeD);

    float dist = dot(parallelogramSideNormal, p) + innerWidth;
    float pt = dist / (innerWidth * 2.0);

    float currentProgress = t * innerWidth;
    float progress = smoothstep(t * innerWidth + smoothing, t * innerWidth, pt * innerWidth);
    //vec3 innerActiveColor = vec3(, 0, 0);
    vec3 innerColor = mix(vec3(0.05), pow(vec3(.1, .7, .8), vec3(4.0 * (pt + 0.3))), progress);
    //vec3 innerColor = vec3(progress);
    color = mix(color, innerColor, activeD);
    color += pow(abs(smoothstep(0.5, 0.0, abs(p.y - 0.2))) / 2.0 * (1.0 - pt), 2) * progress;
    color *= smoothstep(2.0, 0.0, abs(p.y));

	fragColor = vec4(color, alpha);
    
	//fragColor = vec4(position, 0.0, 1.0);
}
