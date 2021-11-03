#version 330 core

#define DIRECTIONAL_LIGHT_COUNT 1
#define POINT_LIGHT_COUNT 2
#define SPOT_LIGHT_COUNT 1
#define CSM_CASCADE_COUNT 8

struct DirectionalLight {
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
    bool shadow_mapped;
    bool active;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
    float shadow_near_plane;
    float shadow_far_plane;
    bool shadow_mapped;
    bool active;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
    float inner_cutoff;
    float outer_cutoff;
    float shadow_near_plane;
    float shadow_far_plane;
    bool shadow_mapped;
    bool active;
};

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

uniform sampler2D material_diffuse;
uniform sampler2D material_specular;
uniform sampler2D material_normal;
uniform float material_shininess;

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

void main() {
    out_fragment_color = vec4(texture(material_diffuse, io_texture_coordinates).rgb, 1.0);
}