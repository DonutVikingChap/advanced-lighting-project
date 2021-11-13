#ifndef LIGHT_GLSL
#define LIGHT_GLSL

struct DirectionalLight {
	vec3 direction;
	vec3 color;
	bool is_shadow_mapped;
	bool is_active;
};

struct PointLight {
	vec3 position;
	vec3 color;
	float constant;
	float linear;
	float quadratic;
	float shadow_near_plane;
	float shadow_far_plane;
	bool is_shadow_mapped;
	bool is_active;
};

struct SpotLight {
	vec3 position;
	vec3 direction;
	vec3 color;
	float constant;
	float linear;
	float quadratic;
	float inner_cutoff;
	float outer_cutoff;
	float shadow_near_plane;
	float shadow_far_plane;
	bool is_shadow_mapped;
	bool is_active;
};

#endif
