#include "light.glsl"

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
    vec3 mat_diffuse = texture(material_diffuse, io_texture_coordinates).rgb;
    float mat_specular = texture(material_diffuse, io_texture_coordinates).r;
    
    vec3 normal = normalize(io_normal);
    vec3 view_direction = normalize(view_position - io_fragment_position);

    vec3 result = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i < DIRECTIONAL_LIGHT_COUNT; ++i) {
        vec3 ambient = directional_lights[i].ambient * directional_lights[i].ambient;
        float n_dot_l = dot(normal, directional_lights[i].direction);
        vec3 reflect_dir = reflect(-directional_lights[i].direction, normal);
    
        vec3 diffuse = n_dot_l * mat_diffuse * directional_lights[i].diffuse;
    
        float specular_intensity = pow(max(dot(reflect_dir, view_direction), 0), mat_specular);
        vec3 specular = specular_intensity * directional_lights[i].specular;

        result += ambient + diffuse + specular;
    }

    for (int i = 0; i < POINT_LIGHT_COUNT; ++i) {
        vec3 light_to_frag = io_fragment_position - point_lights[i].position;
        float light_distance_squared = dot(light_to_frag, light_to_frag);
	    float light_distance = sqrt(light_distance_squared);
        vec3 light_direction = light_to_frag / light_distance;

        float attenuation = 1.0 / (point_lights[i].constant + point_lights[i].linear * light_distance + point_lights[i].quadratic * light_distance_squared);

        vec3 ambient = point_lights[i].ambient * point_lights[i].ambient;
        float n_dot_l = dot(normal, light_direction);
        vec3 reflect_dir = reflect(-light_direction, normal);

        vec3 diffuse = mat_diffuse * point_lights[i].diffuse;

        float specular_intensity = pow(max(dot(reflect_dir, view_direction), 0), mat_specular);
        vec3 specular = specular_intensity * point_lights[i].specular;

        result += (ambient + diffuse + specular) * attenuation;
    }

    for (int i = 0; i < SPOT_LIGHT_COUNT; ++i) {
        vec3 frag_to_light = spot_lights[i].position - io_fragment_position;
        float light_distance_squared = dot(frag_to_light, frag_to_light);
	    float light_distance = sqrt(light_distance_squared);
        vec3 light_direction = frag_to_light / light_distance;

        float theta = dot(spot_lights[i].direction, -light_direction);
        float epsilon = spot_lights[i].outer_cutoff - spot_lights[i].inner_cutoff;
	    float intensity = smoothstep(0.0, 1.0, (theta - spot_lights[i].outer_cutoff) / epsilon);
    
        vec3 ambient = spot_lights[i].ambient * spot_lights[i].ambient;
        float n_dot_l = dot(normal, spot_lights[i].direction);
        vec3 reflect_dir = reflect(-light_direction, normal);

        float attenuation = intensity / (spot_lights[i].constant + spot_lights[i].linear * light_distance + spot_lights[i].quadratic * light_distance_squared);

        vec3 diffuse = n_dot_l * mat_diffuse * spot_lights[i].diffuse;

        float specular_intensity = pow(max(dot(reflect_dir, view_direction), 0), mat_specular);
        vec3 specular = specular_intensity * spot_lights[i].specular;

        result += (ambient + diffuse + specular) * attenuation;
    }

    out_fragment_color = vec4(result, 1.0);
}
