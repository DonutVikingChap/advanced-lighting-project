#include "light.glsl"

#define PI 3.1415
#define BASE_REFLECTIVITY 0.04

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
uniform sampler2D material_specular;
uniform sampler2D material_metallic;

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

float normal_tr_ggx(vec3 n, vec3 h, float a) {
    float a_sq = a*a;
    float n_dot_h = dot(n, h);
    float denominator_root = (PI*n_dot_h*n_dot_h*(a_sq-1.0) + 1.0);
    return a_sq/(denominator_root*denominator_root);
}

float geometry_schlick_ggx(float d, float k) {
    return d/(d*(1.0-k) + k);
}

float geometry_smith(vec3 n, vec3 v, vec3 l, float k) {
    float n_dot_v = dot(n, v);
    float n_dot_l = dot(n, l);
    return geometry_schlick_ggx(n_dot_v, k) * geometry_schlick_ggx(n_dot_l, k);
}

vec3 fresnel_schlick(vec3 h, vec3 v, vec3 f_0) {
    return f_0 + (1.0 - f_0)*pow(1 - dot(h,v), 5);
}

vec3 pbr(
    vec3 normal,
    vec3 view_direction,
    vec3 light_direction,
    vec3 light_color,
    vec3 albedo,
    float k_d,
    float metallic,
    float roughness,
    vec3 reflectivity
) {
    vec3 half_vector = -normalize(light_direction + view_direction);
    float n_dot_l = dot(normal, light_direction);
    
    float k = pow(roughness + 1 , 2);
    float d = normal_tr_ggx(normal, half_vector, roughness);
    vec3 f = fresnel_schlick(half_vector, -view_direction, reflectivity);
    float g = geometry_smith(io_normal, -view_direction, light_direction, k);

    vec3 diffuse = albedo * (1/PI);
    vec3 specular = d*f*g/(4*(-view_direction));
    return (k_d * diffuse + specular) * light_color * n_dot_l;
}

void main() {
    vec3 albedo = texture(material_albedo, io_texture_coordinates).rgb;
    float roughness = texture(material_roughness, io_texture_coordinates).r;
    float metallic = texture(material_metallic, io_texture_coordinates).r;
    float specular = texture(material_specular, io_texture_coordinates).r;
    vec3 reflectivity = mix(vec3(BASE_REFLECTIVITY), albedo, metallic);

    vec3 normal = normalize(io_normal);
    vec3 tangent = normalize(io_tangent);
    vec3 bitangent = normalize(io_bitangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    normal = texture(material_normal, io_texture_coordinates).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(TBN * normal);
    
    normal = normal * 2.0 - 1.0;
    normalize(TBN * normal);
    float k_d = (1.0-specular);
    vec3 view_direction = normalize(view_position - io_fragment_position);

    vec3 result = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i < DIRECTIONAL_LIGHT_COUNT; ++i) {
        if (!directional_lights[i].is_active) {
            continue;
        }
        result += directional_lights[i].ambient * directional_lights[i].ambient;
        result +=
            pbr(
                normal,
                view_direction,
                directional_lights[i].direction,
                directional_lights[i].color,
                albedo,
                k_d,
                metallic,
                roughness,
                reflectivity);
    }
    for (int i = 0; i < POINT_LIGHT_COUNT; ++i) {
        if (!point_lights[i].is_active) {
            continue;
        }

        vec3 light_to_frag = io_fragment_position - point_lights[i].position;
        float light_distance_squared = dot(light_to_frag, light_to_frag);
	    float light_distance = sqrt(light_distance_squared);
        vec3 light_direction = light_to_frag / light_distance;
        float attenuation = 1.0 / (point_lights[i].constant + point_lights[i].linear * light_distance + point_lights[i].quadratic * light_distance_squared);
        
        result += point_lights[i].ambient;
        result += attenuation * pbr(
                normal,
                view_direction,
                light_direction,
                point_lights[i].color,
                albedo,
                k_d,
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
    
        vec3 ambient = spot_lights[i].ambient * spot_lights[i].ambient;

        float attenuation = intensity / (spot_lights[i].constant + spot_lights[i].linear * light_distance + spot_lights[i].quadratic * light_distance_squared);
        
        result += spot_lights[i].ambient;
        result +=
            attenuation * pbr(
                io_normal,
                view_direction,
                light_direction,
                spot_lights[i].color,
                albedo,
                k_d,
                metallic,
                roughness,
                reflectivity);
    }
    out_fragment_color = vec4(result, 1.0);
}
