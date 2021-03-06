#ifndef GAMMA_GLSL
#define GAMMA_GLSL

vec3 gamma_correct(vec3 color) {
	return pow(color, vec3(1.0 / GAMMA));
}

#endif
