#version 430 core

out vec4 outColor;
in vec2 texturePosition;
uniform bool color;
in vec3 p;
#include "Utils/colorConversion.glsl";
#include "Utils/noise.glsl";

void main() {
    vec2 texturePos = texturePosition;
    texturePos.y -= 0.5;
    texturePos.y *= 2.0;
    texturePos.y = abs(texturePos.y);


    if (color) {
        //outColor = vec4(texturePos, 1.0, 1.0 - texturePos.y);
        //outColor = vec4(texturePos.x, 0.1, 0.0, 1.0);
        //outColor = vec4(hsv2rgb(vec3(texturePos.x * 5.0, texturePos.y, 1.0)), 1.0);
        //outColor = vec4(hsv2rgb(vec3(texturePos.x * 5.0, 1.0 - texturePos.y, 1.0)), 1.0);
        //outColor = vec4(vec3(1.0), 1.0 - texturePos.y * octave01(texturePos * 15.0, 3));
        //texturePos.x *= 5.0;
        outColor = vec4(vec3(1.0), (0.5 - texturePos.y / 2.0)  * octave01(texturePos * 1.0, 3));
        //outColor = vec4(vec3(1.0), (0.5 - texturePos.y / 2.0)  * octave01(texturePos * 1.0, 3));
        //outColor = vec4(vec3(1.0), 1.0 - texturePos.y);
        //outColor = vec4(texturePos.y);
    } else {
        outColor = vec4(1.0);
    }
    //outColor = vec4(vec3((p.z + 1.0) / 2.0), 1.0);
    //outColor = vec4(1.0);
    //outColor = vec4(texturePosition, 0.0, 1.0);
}
