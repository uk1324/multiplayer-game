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