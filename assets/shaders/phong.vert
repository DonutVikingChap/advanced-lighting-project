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
    mat4 shadow_matrices[CSM_CASCADE_COUNT];
    float shadow_uv_sizes[CSM_CASCADE_COUNT];
    float shadow_near_planes[CSM_CASCADE_COUNT];
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
    mat4 shadow_matrix;
    bool shadow_mapped;
    bool active;
};

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texture_coordinates;

out vec3 io_fragment_position;
out float io_fragment_depth;
out vec3 io_normal;
out vec2 io_texture_coordinates;
out vec4 io_fragment_positions_in_directional_light_space[DIRECTIONAL_LIGHT_COUNT * CSM_CASCADE_COUNT];
out vec4 io_fragment_positions_in_spot_light_space[SPOT_LIGHT_COUNT];

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat3 normal_matrix;
uniform DirectionalLight directional_lights[DIRECTIONAL_LIGHT_COUNT];
uniform SpotLight spot_lights[SPOT_LIGHT_COUNT];

void main() {
	io_fragment_position = vec3(model_matrix * vec4(in_position, 1.0));
	vec4 fragment_in_view_space = view_matrix * vec4(io_fragment_position, 1.0);
	io_fragment_depth = fragment_in_view_space.z;
	io_normal = normal_matrix * in_normal;
	io_texture_coordinates = in_texture_coordinates;
	for (int i = 0; i < DIRECTIONAL_LIGHT_COUNT; ++i) {
		for (int cascade_level = 0; cascade_level < CSM_CASCADE_COUNT; ++cascade_level) {
			io_fragment_positions_in_directional_light_space[i * CSM_CASCADE_COUNT + cascade_level] = directional_lights[i].shadow_matrices[cascade_level] * vec4(io_fragment_position, 1.0);
		}
	}
	for (int i = 0; i < SPOT_LIGHT_COUNT; ++i) {
		io_fragment_positions_in_spot_light_space[i] = spot_lights[i].shadow_matrix * vec4(io_fragment_position, 1.0);
	}
	gl_Position = projection_matrix * fragment_in_view_space;
}