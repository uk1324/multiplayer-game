#version 430 core

out vec4 outColor;
in vec2 texturePosition;
uniform bool color;

void main() {
    vec2 texturePos = texturePosition;
    texturePos.y -= 0.5;
    texturePos.y *= 2.0;
    texturePos.y = abs(texturePos.y);

    if (color) {
        outColor = vec4(texturePos, 1.0, 1.0 - texturePos.y);
    } else {
        outColor = vec4(1.0);
    }
}
