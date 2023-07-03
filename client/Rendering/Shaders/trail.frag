#version 430 core

out vec4 outColor;
in vec2 texturePosition;
uniform bool color;
in vec3 p;
#include "Utils/colorConversion.glsl";
#include "Utils/noise.glsl";
uniform float time;
uniform float len;
void main() {
    vec2 texturePos = texturePosition;
    texturePos.y -= 0.5;
    texturePos.y *= 2.0;
    texturePos.y = abs(texturePos.y);


    if (color) {
        float a = texturePos.x / len;
        //outColor = vec4(texturePos, 0.0, 1.0 - texturePos.y);
        //outColor = vec4(vec3(texturePos.y), texturePosition.y);
        //texturePos.y /= texturePos.x;
        //texturePos.y -= octave01(vec2(texturePos.x / len * 10.0, 0.0), 4) * 0.2;
        vec2 p = texturePosition;
        p.x /= len;
        p.x += time;
        float d = octave01(p, 4);
        p = texturePos;
        p.x /= len;
        d *= 1.0 - p.y;
        d *= a;
        //d = 1.0 - a;
        //d *= 1.0 - a;
        d *= 1.0 - smoothstep(0.2, 0.0, 1.0 - p.x);
        vec3 c = vec3(1,1./4.,1./16.) * exp(4.*d - 1.);
        outColor = vec4(vec3(d), d);
        outColor = vec4(c, d);

    } else {
        outColor = vec4(1.0);
    }
    //outColor = vec4(hsv2rgb(vec3(5.0 * texturePos.x, 1.0, 1.0)), 1.0);
}
