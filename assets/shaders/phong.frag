#version 330 core

#define DIRECTIONAL_LIGHT_COUNT 1
#define POINT_LIGHT_COUNT 2
#define SPOT_LIGHT_COUNT 1

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D normal;
    float shininess;
};

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

in vec3 io_fragment_position;
in float io_fragment_depth;
in vec3 io_normal;
in vec2 io_texture_coordinates;
in vec4 io_fragment_positions_in_directional_light_space[DIRECTIONAL_LIGHT_COUNT * CSM_CASCADE_COUNT];
in vec4 io_fragment_positions_in_spot_light_space[SPOT_LIGHT_COUNT];

out vec4 out_fragment_color;

uniform vec3 view_position;
uniform DirectionalLight directional_lights[DIRECTIONAL_LIGHT_COUNT];
uniform PointLight point_lights[POINT_LIGHT_COUNT];
uniform SpotLight spot_lights[SPOT_LIGHT_COUNT];
uniform Material material;

uniform sampler2DArrayShadow directional_shadow_maps[DIRECTIONAL_LIGHT_COUNT];
uniform sampler2DArray directional_depth_maps[DIRECTIONAL_LIGHT_COUNT];
uniform samplerCubeShadow point_shadow_maps[POINT_LIGHT_COUNT];
uniform sampler2DShadow spot_shadow_maps[SPOT_LIGHT_COUNT];

uniform float cascade_levels_frustum_depths[CSM_CASCADE_COUNT];

void main() {
    float material_diffuse = texture(material.diffuse, io_texture_coordinates)
    float material_specular = texture(material.diffuse, io_texture_coordinates)
    
    vec3 view_direction = normalize(view_position - io_fragment_position);

    vec3 result = vec3(0,0,0)

    for(int i = 0; i < DIRECTIONAL_LIGHT_COUNT; i++) {
        const vec3 ambient = directional_lights[i].ambient * directional_lights[i].color;
        const float n_dot_l = dot(io_normal, directional_lights[i].direction);
        const float reflect_dir = reflect(-directional_lights[i].direction, io_normal);
        
        const vec3 diffuse = n_dot_l * material_diffuse * directional_lights[i].color;
        
        const specular_intensity = pow(max(reflect_dir * view_direction, 0), material_specular);
        const vec3 specular = specular_intensity * vec3(1, 1, 1);

        result += (ambient + diffuse + specular) * directional_lights[i].color;
    }
    for(int i = 0; i < POINT_LIGHT_COUNT; i++) {
        const vec3 light_to_frag = io_fragment_position - point_lights[i].position;
        const float light_distance_squared = dot(light_difference, light_to_frag);
	    const float light_distance = sqrt(light_distance_squared);
        const vec3 light_direction = light_to_frag / light_distance;

        const float attenuation = 1.0 / (point_lights[i].constant + point_lights[i].linear * light_distance + point_lights[i].quadratic * light_distance_squared);

        const vec3 ambient = point_lights[i].ambient * point_lights[i].color;
        const float n_dot_l = dot(io_normal, light_direction);
        const float reflect_dir = reflect(-light_direction, io_normal);

        const vec3 diffuse = material_diffuse * directional_lights[i].color;

        const specular_intensity = pow(max(reflect_dir * view_direction, 0), material_specular);
        const vec3 specular = specular_intensity * vec3(1, 1, 1);

        result += (ambient + diffuse + specular) * attenuation * directional_lights[i].color;
    }
    for(int i = 0; i < SPOT_LIGHT_COUNT; i++) {
        const vec3 frag_to_light = spot_lights[i].position - io_fragment_position;
        const float light_distance_squared = dot(light_difference, frag_to_light);
	    const float light_distance = sqrt(light_distance_squared);
        const vec3 light_direction = frag_to_light / light_distance;

        const float theta = dot(spot_lights[i].direction, -light_direction);
        const float epsilon = spot_lights[i].outer_cutoff - spot_lights[i].inner_cutoff;
	    const float intensity = smoothstep(0.0, 1.0, (theta - spot_lights[i].outer_cutoff) / epsilon);
        
        const vec3 ambient = spot_lights[i].ambient * spot_lights[i].color;
        const float n_dot_l = dot(io_normal, spot_lights[i].direction);
        const float reflect_dir = reflect(-light_direction, io_normal);

        const float attenuation = intensity / (spot_lights[i].constant + spot_lights[i].linear * light_distance + spot_lights[i].quadratic * light_distance_squared);

        const vec3 diffuse = n_dot_l * material_diffuse * directional_lights[i].color;

        const specular_intensity = pow(max(reflect_dir * view_direction, 0), material_specular);
        const vec3 specular = specular_intensity * vec3(1, 1, 1);

        result += (ambient + diffuse + specular) * attenuation * directional_lights[i].color;
    }

    out_fragment_color = clamp(result, 0, 1);
}