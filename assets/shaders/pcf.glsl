#ifndef PCF_GLSL
#define PCF_GLSL

#define PCF_FILTER_SAMPLE_COUNT 16
const vec2 pcf_filter_sample_offsets[PCF_FILTER_SAMPLE_COUNT] = vec2[](
	vec2(-0.94201624, -0.39906216),
	vec2(0.94558609, -0.76890725),
	vec2(-0.094184101, -0.92938870),
	vec2(0.34495938, 0.29387760),
	vec2(-0.91588581, 0.45771432),
	vec2(-0.81544232, -0.87912464),
	vec2(-0.38277543, 0.27676845),
	vec2(0.97484398, 0.75648379),
	vec2(0.44323325, -0.97511554),
	vec2(0.53742981, -0.47373420),
	vec2(-0.26496911, -0.41893023),
	vec2(0.79197514, 0.19090188),
	vec2(-0.24188840, 0.99706507),
	vec2(-0.81409955, 0.91437590),
	vec2(0.19984126, 0.78641367),
	vec2(0.14383161, -0.14100790));

float pcf_filter(sampler2DShadow shadow_map, vec2 uv, float z, float filter_radius) {
    vec2 texel_size = 1.0 / textureSize(shadow_map, 0);
    float visibility = 0.0;
    for (int i = 0; i < PCF_FILTER_SAMPLE_COUNT; ++i) {
        vec2 offset = pcf_filter_sample_offsets[i] * texel_size * filter_radius;
        visibility += texture(shadow_map, vec3(uv + offset, z));
    }
    return visibility / float(PCF_FILTER_SAMPLE_COUNT);
}

float pcf_filter_array(sampler2DArrayShadow shadow_map, vec2 uv, float layer, float z, float filter_radius) {
    vec2 texel_size = 1.0 / textureSize(shadow_map, 0).xy;
	float visibility = 0.0;
	for (int i = 0; i < PCF_FILTER_SAMPLE_COUNT; ++i) {
		vec2 offset = pcf_filter_sample_offsets[i] * texel_size * filter_radius;
		visibility += texture(shadow_map, vec4(uv + offset, layer, z));
	}
	return visibility / float(PCF_FILTER_SAMPLE_COUNT);
}

#define PCF_FILTER_CUBE_SAMPLE_COUNT 20
const vec3 pcf_filter_cube_sample_offsets[PCF_FILTER_CUBE_SAMPLE_COUNT] = vec3[](
    vec3(1.0, 1.0, 1.0),  vec3(1.0, -1.0, 1.0),  vec3(-1.0, -1.0, 1.0),  vec3(-1.0, 1.0, 1.0),
    vec3(1.0, 1.0, -1.0), vec3(1.0, -1.0, -1.0), vec3(-1.0, -1.0, -1.0), vec3(-1.0, 1.0, -1.0),
    vec3(1.0, 1.0, 0.0),  vec3(1.0, -1.0, 0.0),  vec3(-1.0, -1.0, 0.0),  vec3(-1.0, 1.0, 0.0),
    vec3(1.0, 0.0, 1.0),  vec3(-1.0, 0.0, 1.0),  vec3(1.0, 0.0, -1.0),   vec3(-1.0, 0.0, -1.0),
    vec3(0.0, 1.0, 1.0),  vec3(0.0, -1.0, 1.0),  vec3(0.0, -1.0, -1.0),  vec3(0.0, 1.0, -1.0));

float pcf_filter_cube(samplerCubeShadow shadow_map, vec3 direction, float z, float filter_radius) {
    float visibility = 0.0;
    for (int i = 0; i < PCF_FILTER_CUBE_SAMPLE_COUNT; ++i) {
        vec3 offset = pcf_filter_cube_sample_offsets[i] * filter_radius;
        visibility += texture(shadow_map, vec4(direction + offset, z));
    }
    return visibility / float(PCF_FILTER_CUBE_SAMPLE_COUNT);
}

float pcss_blocker_search_size(float z, float light_size, float near_z) {
    return light_size * (z - near_z * 0.5) / z;
}

float pcss_average_blocker_z(out float blocker_count, sampler2DArray depth_map, vec2 uv, float layer, float z, float light_size, float near_z) {
    float search_size = pcss_blocker_search_size(z, light_size, near_z);
    float blocker_sum = 0.0;
    blocker_count = 0.0;
    for (int i = 0; i < PCF_FILTER_SAMPLE_COUNT; ++i) {
        vec2 offset = pcf_filter_sample_offsets[i] * search_size;
        float depth = texture(depth_map, vec3(uv + offset, layer)).r;
        if (depth < z) {
            blocker_sum += depth;
            blocker_count += 1.0;
        }
    }
    return blocker_sum / blocker_count;
}

float pcss_penumbra_size(float z, float blocker_z) {
    return (z - blocker_z) / blocker_z;
}

float pcss(sampler2DArrayShadow shadow_map, sampler2DArray depth_map, vec2 uv, float layer, float z, float light_size, float near_z) {
    return pcf_filter_array(shadow_map, uv, layer, z, 0.1); // TODO: Remove

    float blocker_count = 0.0;
    float average_blocker_z = pcss_average_blocker_z(blocker_count, depth_map, uv, layer, z, light_size, near_z);
    if (blocker_count < 1.0) {
        return 1.0;
    }

    float penumbra_size = pcss_penumbra_size(z, average_blocker_z);
    float filter_radius = penumbra_size * light_size * near_z / z;
    return pcf_filter_array(shadow_map, uv, layer, z, filter_radius);
}

#endif
