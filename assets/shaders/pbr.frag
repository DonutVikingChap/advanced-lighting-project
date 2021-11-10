#include "light.glsl"

#define PI 3.14159265359
#define BASE_REFLECTIVITY 0.04
#define EPSILON 0.0001
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

uniform samplerCube cubemap_texture;
//uniform sampler2D brdf_lut;

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

float distribution_ggx(float n_dot_x, float roughness) {
	float a = roughness*roughness;
	float a_sq = a*a;
	float denominator_root = (n_dot_x * n_dot_x * (a_sq - 1.0) + 1.0);
	return a_sq / (PI * denominator_root * denominator_root);
}

float geometry_schlick_ggx(float n_dot_x, float k) {
	return n_dot_x / (n_dot_x * (1.0 - k) + k);
}

float geometry_smith(float n_dot_v, float n_dot_l, float a)
{
	float k = pow(a+1.0, 2)/8.0;
	float ggx_1 = geometry_schlick_ggx(n_dot_v, k);
	float ggx_2 = geometry_schlick_ggx(n_dot_l, k);
	return ggx_1*ggx_2;
}

vec3 fresnel_schlick(float n_dot_l, vec3 f_0) {
	return f_0 + (1.0 - f_0) * pow(clamp(1.0 - n_dot_l, 0.0, 1.0), 5.0);;
}

vec3 fresnel_schlick_roughness(float n_dot_l, vec3 f_0, float roughness) {
	return f_0 + (max(vec3(1.0 - roughness), f_0) - f_0) * pow(clamp(1.0 - n_dot_l, 0.0, 1.0), 5.0);
}

vec3 pbr(
	vec3 normal,
	vec3 view_direction,
	vec3 light_direction,
	float n_dot_v,
	vec3 light_color,
	vec3 albedo,
	float metallic,
	float roughness,
	vec3 reflectivity
) {
	vec3 half_vector =  normalize(light_direction + view_direction);
	float n_dot_h = max(dot(normal, half_vector), 0.0);
	float n_dot_l = max(dot(normal, light_direction), 0.0);
	vec3 r = reflect(-view_direction, normal);

	float d = distribution_ggx(n_dot_h, roughness);
	vec3 f = fresnel_schlick(max(0.0, dot(half_vector, view_direction)), reflectivity);
	float g = geometry_smith(n_dot_v, n_dot_l, roughness);

	vec3 k_d = (vec3(1.0) - f)*(1.0 - metallic);
	vec3 diffuse = albedo / PI;
	vec3 specular = d*f*g*(1.0 / (EPSILON + max(4.0 * n_dot_l * n_dot_v, 0.0)));

	return (k_d * diffuse + specular) * light_color * n_dot_l;
}

void main() {	
	vec3 albedo = pow(texture(material_albedo, io_texture_coordinates).rgb, vec3(2.2)); //convert from sRGB to linear
	float roughness = texture(material_roughness, io_texture_coordinates).r;
	float metallic = texture(material_metallic, io_texture_coordinates).r;

	vec3 reflectivity = mix(vec3(BASE_REFLECTIVITY), albedo, metallic);

	vec3 normal = normalize(io_normal);
	vec3 tangent = normalize(io_tangent);
	vec3 bitangent = normalize(io_bitangent);
	mat3 TBN = mat3(io_tangent, io_tangent, io_normal);

	normal = texture(material_normal, io_texture_coordinates).rgb;
	normal = normal * 2.0 - 1.0;
	normal = normalize(TBN * normal);

	vec3 irradiance = vec3(0.03);//texture(irradiance_cubemap_texture, normal).rgb;

	vec3 view_direction = normalize(view_position - io_fragment_position);
	float n_dot_v = max(dot(normal, view_direction), 0.0);

	vec3 ambient = vec3(0.0);
	{
		vec3 f = fresnel_schlick_roughness(n_dot_v, reflectivity, roughness);

		vec3 k_d = vec3(1.0) - f;
		vec3 diffuse = irradiance*albedo;
		/*
		float lod = roughness * MAX_REFLECTION_LOD;
		vec3 cubemap_color = textureCubeLod(cubemap_texture, r, lod).rgb;
		vec2 env_brdf = texture2D(brdf_lut, vec2(n_dot_v, roughness)).xy;
		vec3 specular = cubemap_color * (f * env_brdf.x + env_brdf.y);
		*/
		ambient = k_d * diffuse;// + specular;
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

	// gamma correction
	vec3 color = Lo + ambient;
	color /= (color + vec3(1.0));
	color = pow(color, vec3(1.0/2.2));

	out_fragment_color = vec4(color, 1.0);
	
}
