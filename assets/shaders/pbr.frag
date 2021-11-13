#include "light.glsl"
#include "gamma.glsl"
#include "math.glsl"
#include "pbr.glsl"

#define BASE_REFLECTIVITY 0.04
#define MAX_REFLECTION_LOD 4.0

in vec3 io_fragment_position;
in float io_fragment_depth;
in vec3 io_normal;
in vec3 io_tangent;
in vec3 io_bitangent;
in vec2 io_texture_coordinates;
in vec4 io_fragment_positions_in_directional_light_space[DIRECTIONAL_LIGHT_COUNT * CSM_CASCADE_COUNT];
in vec4 io_fragment_positions_in_spot_light_space[SPOT_LIGHT_COUNT];

out vec4 out_fragment_color;

uniform vec3 view_position;

uniform sampler2D material_albedo;
uniform sampler2D material_normal;
uniform sampler2D material_roughness;
uniform sampler2D material_metallic;

uniform samplerCube environment_cubemap_texture;
uniform samplerCube irradiance_cubemap_texture;
uniform samplerCube prefilter_cubemap_texture;
uniform sampler2D brdf_lookup_table_texture;

uniform DirectionalLight directional_lights[DIRECTIONAL_LIGHT_COUNT];
uniform PointLight point_lights[POINT_LIGHT_COUNT];
uniform SpotLight spot_lights[SPOT_LIGHT_COUNT];

uniform sampler2DArrayShadow directional_shadow_maps[DIRECTIONAL_LIGHT_COUNT];
uniform sampler2DArray directional_depth_maps[DIRECTIONAL_LIGHT_COUNT];
uniform float directional_shadow_uv_sizes[DIRECTIONAL_LIGHT_COUNT * CSM_CASCADE_COUNT];
uniform float directional_shadow_near_planes[DIRECTIONAL_LIGHT_COUNT * CSM_CASCADE_COUNT];

uniform samplerCubeShadow point_shadow_maps[POINT_LIGHT_COUNT];
uniform sampler2DShadow spot_shadow_maps[SPOT_LIGHT_COUNT];

uniform float cascade_levels_frustum_depths[CSM_CASCADE_COUNT];

vec3 tonemap(vec3 color) {
	return color / (color + vec3(1.0));
}

void main() {
	vec4 albedo_sample = texture(material_albedo, io_texture_coordinates);
#if USE_ALPHA_TEST
	float alpha = albedo_sample.a;
	if (alpha < 0.1) {
		discard;
	}
#else
	const float alpha = 0.0;
#endif
	vec3 albedo = pow(albedo_sample.rgb, vec3(2.2)); // Convert from sRGB to linear.
	float roughness = texture(material_roughness, io_texture_coordinates).r;
	float metallic = texture(material_metallic, io_texture_coordinates).r;

	vec3 reflectivity = mix(vec3(BASE_REFLECTIVITY), albedo, metallic);

	vec3 normal = normalize(io_normal);
	vec3 tangent = normalize(io_tangent);
	vec3 bitangent = normalize(io_bitangent);
	mat3 TBN = mat3(io_tangent, io_tangent, io_normal);

	vec3 surface_normal = texture(material_normal, io_texture_coordinates).rgb;
	surface_normal = surface_normal * 2.0 - 1.0;
	normal = normalize(TBN * surface_normal);

	vec3 irradiance = texture(irradiance_cubemap_texture, normal).rgb;

	vec3 view_direction = normalize(view_position - io_fragment_position);
	vec3 reflect_direction = reflect(-view_direction, normal);
	float n_dot_v = max(dot(normal, view_direction), 0.0);

	vec3 ambient = vec3(0.0);
	{
		vec3 f = fresnel_schlick_roughness(n_dot_v, reflectivity, roughness);
		vec3 k_s = f;
		vec3 k_d = (vec3(1.0) - k_s) * (1.0 - metallic);
		vec3 diffuse = k_d * irradiance * albedo;
		float lod = roughness * MAX_REFLECTION_LOD;
		vec3 env_prefiltered_color = textureLod(prefilter_cubemap_texture, reflect_direction, lod).rgb;
		vec2 env_brdf = texture(brdf_lookup_table_texture, vec2(n_dot_v, roughness)).rg;
		vec3 specular = env_prefiltered_color * (f * env_brdf.r + env_brdf.g);
		ambient = diffuse + specular;
	}

	vec3 Lo = vec3(0.0);
	for (int i = 0; i < DIRECTIONAL_LIGHT_COUNT; ++i) {
		if (!directional_lights[i].is_active) {
			continue;
		}

		Lo += pbr(
			normal,
			view_direction,
			-directional_lights[i].direction,
			n_dot_v,
			directional_lights[i].color,
			albedo,
			metallic,
			roughness,
			reflectivity);
	}
	for (int i = 0; i < POINT_LIGHT_COUNT; ++i) {
		if (!point_lights[i].is_active) {
			continue;
		}

		vec3 frag_to_light = point_lights[i].position - io_fragment_position;
		float light_distance_squared = dot(frag_to_light, frag_to_light);
		float light_distance = sqrt(light_distance_squared);
		vec3 light_direction = frag_to_light / light_distance;
		float attenuation = 1.0 / (point_lights[i].constant + point_lights[i].linear * light_distance + point_lights[i].quadratic * light_distance_squared);

		Lo += attenuation * pbr(
			normal,
			view_direction,
			light_direction,
			n_dot_v,
			point_lights[i].color,
			albedo,
			metallic,
			roughness,
			reflectivity);
	}
	for (int i = 0; i < SPOT_LIGHT_COUNT; ++i) {
		if (!spot_lights[i].is_active) {
			continue;
		}
		vec3 frag_to_light = spot_lights[i].position - io_fragment_position;
		float light_distance_squared = dot(frag_to_light, frag_to_light);
		float light_distance = sqrt(light_distance_squared);
		vec3 light_direction = frag_to_light / light_distance;

		float theta = dot(spot_lights[i].direction, -light_direction);
		float epsilon = spot_lights[i].outer_cutoff - spot_lights[i].inner_cutoff;
		float intensity = smoothstep(0.0, 1.0, (theta - spot_lights[i].outer_cutoff) / epsilon);

		float attenuation = intensity / (spot_lights[i].constant + spot_lights[i].linear * light_distance + spot_lights[i].quadratic * light_distance_squared);

		Lo += attenuation * pbr(
			normal,
			view_direction,
			light_direction,
			n_dot_v,
			spot_lights[i].color,
			albedo,
			metallic,
			roughness,
			reflectivity);
	}

	out_fragment_color = vec4(gamma_correct(tonemap(Lo + ambient)), alpha);
}
