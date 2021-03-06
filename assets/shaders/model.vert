layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_tangent;
layout (location = 3) in vec3 in_bitangent;
layout (location = 4) in vec2 in_texture_coordinates;
layout (location = 5) in vec2 in_lightmap_coordinates;

out vec3 io_fragment_position;
out float io_fragment_depth;
out vec3 io_normal;
out vec3 io_tangent;
out vec3 io_bitangent;
out vec2 io_texture_coordinates;
out vec2 io_lightmap_coordinates;
out vec4 io_fragment_positions_in_directional_light_space[DIRECTIONAL_LIGHT_COUNT * CSM_CASCADE_COUNT];
out vec4 io_fragment_positions_in_spot_light_space[SPOT_LIGHT_COUNT];

uniform mat4 projection_matrix;
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform mat3 normal_matrix;

uniform vec2 lightmap_offset;
uniform vec2 lightmap_scale;

uniform mat4 directional_shadow_matrices[DIRECTIONAL_LIGHT_COUNT * CSM_CASCADE_COUNT];
uniform mat4 spot_shadow_matrices[SPOT_LIGHT_COUNT];

void main() {
	io_fragment_position = vec3(model_matrix * vec4(in_position, 1.0));
	vec4 fragment_in_view_space = view_matrix * vec4(io_fragment_position, 1.0);
	io_fragment_depth = fragment_in_view_space.z;
	io_normal = normal_matrix * in_normal;
	io_tangent = normal_matrix * in_tangent;
	io_bitangent = normal_matrix * in_bitangent;
	io_texture_coordinates = in_texture_coordinates;
	io_lightmap_coordinates = lightmap_offset + in_lightmap_coordinates * lightmap_scale;

#for LIGHT_INDEX 0, DIRECTIONAL_LIGHT_COUNT
#for CASCADE_LEVEL 0, CSM_CASCADE_COUNT
	io_fragment_positions_in_directional_light_space[LIGHT_INDEX * CSM_CASCADE_COUNT + CASCADE_LEVEL] = directional_shadow_matrices[LIGHT_INDEX * CSM_CASCADE_COUNT + CASCADE_LEVEL] * vec4(io_fragment_position, 1.0);
#endfor
#endfor

#for LIGHT_INDEX 0, SPOT_LIGHT_COUNT
	io_fragment_positions_in_spot_light_space[LIGHT_INDEX] = spot_shadow_matrices[LIGHT_INDEX] * vec4(io_fragment_position, 1.0);
#endfor

	gl_Position = projection_matrix * fragment_in_view_space;
}
