#include "light.glsl"
#include "gamma.glsl"
#include "math.glsl"
#include "pbr.glsl"
#include "pcf.glsl"

#define CSM_VISUALIZE_CASCADES 0
#define CSM_BLEND_SIZE 1.0
#define BASE_REFLECTIVITY 0.04
#define MAX_REFLECTION_LOD 4.0

const vec3 cascade_color_tints[] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0),
	vec3(1.0, 1.0, 0.0),
	vec3(1.0, 0.0, 1.0),
	vec3(0.0, 1.0, 1.0),
	vec3(1.0, 0.0, 1.0));

in vec3 io_fragment_position;
in float io_fragment_depth;
in vec3 io_normal;
in vec3 io_tangent;
in vec3 io_bitangent;
in vec2 io_texture_coordinates;
in vec2 io_lightmap_coordinates;
in vec4 io_fragment_positions_in_directional_light_space[DIRECTIONAL_LIGHT_COUNT * CSM_CASCADE_COUNT];
in vec4 io_fragment_positions_in_spot_light_space[SPOT_LIGHT_COUNT];

out vec4 out_fragment_color;

uniform vec3 view_position;

uniform sampler2D material_albedo;
uniform sampler2D material_normal;
uniform sampler2D material_roughness;
uniform sampler2D material_metallic;

uniform sampler2D lightmap_texture;
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

uniform float cascade_frustum_depths[CSM_CASCADE_COUNT];

float cube_depth(vec3 v, float near_z, float far_z) {
	float c1 = far_z / (far_z - near_z);
	float c0 = -near_z * c1;
	vec3 m = abs(v);
	float major = max(m.x, max(m.y, m.z));
	return (c1 * major + c0) / major;
}

vec3 tonemap(vec3 color) {
	return color / (color + vec3(1.0));
}

void main() {
	vec4 albedo_sample = texture(material_albedo, io_texture_coordinates);
#if USE_ALPHA_BLENDING
	float alpha = albedo_sample.a;
#else
	const float alpha = 1.0;
#endif
#if USE_ALPHA_TEST
	if (albedo_sample.a < 0.1) {
		discard;
	}
#endif
	vec3 albedo = pow(albedo_sample.rgb, vec3(2.2)); // Convert from sRGB to linear.
	float roughness = texture(material_roughness, io_texture_coordinates).r;
	float metallic = texture(material_metallic, io_texture_coordinates).r;
	vec3 lightmap = texture(lightmap_texture, io_lightmap_coordinates).rgb;

	vec3 reflectivity = mix(vec3(BASE_REFLECTIVITY), albedo, metallic);

	vec3 normal = normalize(io_normal);
	vec3 tangent = normalize(io_tangent);
	vec3 bitangent = normalize(io_bitangent);
	mat3 TBN = mat3(tangent, bitangent, normal);
	vec3 surface_normal = texture(material_normal, io_texture_coordinates).xyz * 2.0 - 1.0;
	normal = normalize(TBN * surface_normal);

	vec3 irradiance = texture(irradiance_cubemap_texture, normal).rgb;

	vec3 view_direction = normalize(view_position - io_fragment_position);
	float n_dot_v = max(dot(normal, view_direction), 0.0);

	vec3 ambient = vec3(0.0);
	{
		vec3 f = fresnel_schlick_roughness(n_dot_v, reflectivity, roughness);
		vec3 k_s = f;
		vec3 k_d = (vec3(1.0) - k_s) * (1.0 - metallic);
		vec3 diffuse = k_d * irradiance * albedo;
		float lod = roughness * MAX_REFLECTION_LOD;
		vec3 env_prefiltered_color = textureLod(prefilter_cubemap_texture, reflect(-view_direction, normal), lod).rgb;
		vec2 env_brdf = texture(brdf_lookup_table_texture, vec2(n_dot_v, roughness)).rg;
		vec3 specular = env_prefiltered_color * (f * env_brdf.r + env_brdf.g);
		ambient = (diffuse + specular) * lightmap;
	}

	vec3 Lo = vec3(0.0);

	int cascade_level_a = 0;
#for CASCADE_LEVEL 0, CSM_CASCADE_COUNT - 1
	cascade_level_a += int(io_fragment_depth < cascade_frustum_depths[CASCADE_LEVEL]);
#endfor
	int cascade_level_b = min(cascade_level_a + 1, CSM_CASCADE_COUNT - 1);
	
	float cascade_depth_a = cascade_frustum_depths[cascade_level_a];
	float cascade_depth_b = cascade_frustum_depths[cascade_level_b];

	float blend_near = cascade_depth_a + CSM_BLEND_SIZE;
	float blend_far = cascade_depth_a;

	float cascade_level_interpolation_alpha = (io_fragment_depth - blend_near) / (blend_far - blend_near);

#if CSM_VISUALIZE_CASCADES
	vec3 cascade_tint_color_a = vec3(1.0) + cascade_color_tints[cascade_level_a];
	vec3 cascade_tint_color_b = vec3(1.0) + cascade_color_tints[cascade_level_b];
	vec3 cascade_tint_color;
	if (cascade_level_interpolation_alpha <= 0.0) {
		cascade_tint_color = cascade_tint_color_a;
	} else {
		cascade_tint_color = mix(cascade_tint_color_a, cascade_tint_color_b, cascade_level_interpolation_alpha);
	}
#endif

#for LIGHT_INDEX 0, DIRECTIONAL_LIGHT_COUNT
	if (directional_lights[LIGHT_INDEX].is_active) {
		float visibility = 1.0;
		if (directional_lights[LIGHT_INDEX].is_shadow_mapped) {
#if BAKING
			vec3 projected_coordinates = io_fragment_positions_in_directional_light_space[LIGHT_INDEX * CSM_CASCADE_COUNT + cascade_level_a].xyz;
			visibility = texture(directional_shadow_maps[LIGHT_INDEX], vec4(projected_coordinates.xy, cascade_level_a, projected_coordinates.z));
#else
			//vec3 projected_coordinates = io_fragment_positions_in_directional_light_space[LIGHT_INDEX * CSM_CASCADE_COUNT + cascade_level_a].xyz;
			//float light_size = directional_shadow_uv_sizes[LIGHT_INDEX * CSM_CASCADE_COUNT + cascade_level_a];
			//float near_z = directional_shadow_near_planes[LIGHT_INDEX * CSM_CASCADE_COUNT + cascade_level_a];
			//visibility = pcss(directional_shadow_maps[LIGHT_INDEX], directional_depth_maps[LIGHT_INDEX], projected_coordinates.xy, cascade_level_a, projected_coordinates.z, light_size, near_z);
			if (cascade_level_interpolation_alpha <= 0.0) {
				vec3 projected_coordinates = io_fragment_positions_in_directional_light_space[LIGHT_INDEX * CSM_CASCADE_COUNT + cascade_level_a].xyz;
	    		visibility = pcf_filter_array(directional_shadow_maps[LIGHT_INDEX], projected_coordinates.xy, cascade_level_a, projected_coordinates.z, 4.0 / float(cascade_level_a + 1));
			} else {
				vec3 projected_coordinates_a = io_fragment_positions_in_directional_light_space[LIGHT_INDEX * CSM_CASCADE_COUNT + cascade_level_a].xyz;
				vec3 projected_coordinates_b = io_fragment_positions_in_directional_light_space[LIGHT_INDEX * CSM_CASCADE_COUNT + cascade_level_b].xyz;
				float visibility_a = pcf_filter_array(directional_shadow_maps[LIGHT_INDEX], projected_coordinates_a.xy, cascade_level_a, projected_coordinates_a.z, 4.0 / float(cascade_level_a + 1));
				float visibility_b = pcf_filter_array(directional_shadow_maps[LIGHT_INDEX], projected_coordinates_b.xy, cascade_level_b, projected_coordinates_b.z, 4.0 / float(cascade_level_b + 1));
				visibility = mix(visibility_a, visibility_b, cascade_level_interpolation_alpha);
			}
#endif
		}

		Lo += visibility * pbr(
			normal,
			view_direction,
			-directional_lights[LIGHT_INDEX].direction,
			n_dot_v,
			directional_lights[LIGHT_INDEX].color,
			albedo,
			metallic,
			roughness,
			reflectivity);
	}
#endfor

#for LIGHT_INDEX 0, POINT_LIGHT_COUNT
	if (point_lights[LIGHT_INDEX].is_active) {
		vec3 frag_to_light = point_lights[LIGHT_INDEX].position - io_fragment_position;
		float light_distance_squared = dot(frag_to_light, frag_to_light);
		float light_distance = sqrt(light_distance_squared);
		vec3 light_direction = frag_to_light / light_distance;
		
		float attenuation = 1.0 / (point_lights[LIGHT_INDEX].constant + point_lights[LIGHT_INDEX].linear * light_distance + point_lights[LIGHT_INDEX].quadratic * light_distance_squared);

		float visibility = 1.0;
		if (point_lights[LIGHT_INDEX].is_shadow_mapped) {
			float receiver_z = cube_depth(-frag_to_light, point_lights[LIGHT_INDEX].shadow_near_z, point_lights[LIGHT_INDEX].shadow_far_z);
#if BAKING
			visibility = texture(point_shadow_maps[LIGHT_INDEX], vec4(-frag_to_light, receiver_z));
#else
			visibility = pcf_filter_cube(point_shadow_maps[LIGHT_INDEX], -frag_to_light, receiver_z, point_lights[LIGHT_INDEX].shadow_filter_radius);
#endif
		}

		Lo += attenuation * visibility * pbr(
			normal,
			view_direction,
			light_direction,
			n_dot_v,
			point_lights[LIGHT_INDEX].color,
			albedo,
			metallic,
			roughness,
			reflectivity);
	}
#endfor

#for LIGHT_INDEX 0, SPOT_LIGHT_COUNT
	if (spot_lights[LIGHT_INDEX].is_active) {
		vec3 frag_to_light = spot_lights[LIGHT_INDEX].position - io_fragment_position;
		float light_distance_squared = dot(frag_to_light, frag_to_light);
		float light_distance = sqrt(light_distance_squared);
		vec3 light_direction = frag_to_light * (1.0 / light_distance);

		float theta = dot(light_direction, -spot_lights[LIGHT_INDEX].direction);
		float epsilon = spot_lights[LIGHT_INDEX].inner_cutoff - spot_lights[LIGHT_INDEX].outer_cutoff;
		float intensity = smoothstep(0.0, 1.0, (theta - spot_lights[LIGHT_INDEX].outer_cutoff) / epsilon);

		float attenuation = 1.0 / (spot_lights[LIGHT_INDEX].constant + spot_lights[LIGHT_INDEX].linear * light_distance + spot_lights[LIGHT_INDEX].quadratic * light_distance_squared);
		
		float visibility = 1.0;
		if (spot_lights[LIGHT_INDEX].is_shadow_mapped) {
			vec4 fragment_position_in_light_space = io_fragment_positions_in_spot_light_space[LIGHT_INDEX];
			vec3 projected_coordinates = fragment_position_in_light_space.xyz / fragment_position_in_light_space.w;
#if BAKING
			visibility = texture(spot_shadow_maps[LIGHT_INDEX], projected_coordinates);
#else
			visibility = pcf_filter(spot_shadow_maps[LIGHT_INDEX], projected_coordinates.xy, projected_coordinates.z, spot_lights[LIGHT_INDEX].shadow_filter_radius);
#endif
		}

		Lo += intensity * attenuation * visibility * pbr(
			normal,
			view_direction,
			light_direction,
			n_dot_v,
			spot_lights[LIGHT_INDEX].color,
			albedo,
			metallic,
			roughness,
			reflectivity);
	}
#endfor

#if CSM_VISUALIZE_CASCADES
	Lo *= cascade_tint_color;
#endif

#if BAKING
	out_fragment_color = vec4(Lo + ambient, (gl_FrontFacing) ? alpha : 0.0);
#else
	out_fragment_color = vec4(gamma_correct(tonemap(Lo + ambient)), alpha);
#endif
}
