#include "light.glsl"

#define PI 3.1415
#define BASE_REFLECTIVITY 0.04
#define EPSILON 0.00001
#define AMBIENT_LIGHT_STRENGTH 0.1

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

float normal_tr_ggx(float d, float a) {
    float a_sq = a*a;
    float denominator_root = (d*d*(a_sq-1.0) + 1.0);
    return a_sq/(PI*denominator_root*denominator_root);
}

float geometry_schlick_ggx(float d, float k) {
    return d/(d*(1.0-k) + k);
}

vec3 fresnel_schlick(float cos_theta, vec3 f_0) {
    return f_0 + (1.0 - f_0)*pow(1.0 - cos_theta, 5);
}

vec3 pbr(
    vec3 normal,
    vec3 view_direction,
    vec3 light_direction,
    vec3 light_color,
    vec3 albedo,
    float metallic,
    float roughness,
    vec3 reflectivity
) {
    vec3 half_vector =  normalize(light_direction + view_direction);
    float cos_omega = max(dot(normal, half_vector), 0.0);
    float cos_theta = max(dot(normal, light_direction), 0.0);
    
    float k = pow(roughness + 1 , 2);
    float d = normal_tr_ggx(cos_omega, roughness);
    vec3 f = fresnel_schlick(max(0.0, dot(half_vector, view_direction)), reflectivity);
    float g = geometry_schlick_ggx(cos_omega, k)*geometry_schlick_ggx(cos_theta, k);
    
    vec3 k_d = mix(vec3(1.0) - f, vec3(0.0), metallic);
    vec3 diffuse = albedo;
    vec3 specular = d*f*g * 1.0/max(EPSILON, 4.0*cos_theta*cos_omega);
    return (k_d * diffuse + specular) * light_color * cos_theta;
}

void main() {
    vec3 albedo = texture(material_albedo, io_texture_coordinates).rgb;
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
    normal = io_normal;

    vec3 view_direction = normalize(view_position - io_fragment_position);

    vec3 ambient = vec3(0.0);
    {
        vec3 f = fresnel_schlick(AMBIENT_LIGHT_STRENGTH, reflectivity);
        vec3 k_d = mix(vec3(1.0) - f, vec3(0.0), metallic);
        ambient = k_d * albedo;
    }

    vec3 result = vec3(0.0);
    for (int i = 0; i < DIRECTIONAL_LIGHT_COUNT; ++i) {
        if (!directional_lights[i].is_active) {
            continue;
        }
        result +=
            pbr(
                normal,
                view_direction,
                -directional_lights[i].direction,
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
        
        result += attenuation * pbr(
                normal,
                view_direction,
                light_direction,
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
        
        result +=
            attenuation * pbr(
                io_normal,
                view_direction,
                light_direction,
                spot_lights[i].color,
                albedo,
                metallic,
                roughness,
                reflectivity);
    }
    out_fragment_color = vec4(result + ambient, 1.0);
}
