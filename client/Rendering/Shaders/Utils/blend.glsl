vec3 blend(vec3 old, vec3 new, float newAlpha) {
    return old * (1.0 - newAlpha) + new * newAlpha;
}
