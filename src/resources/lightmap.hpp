#ifndef LIGHTMAP_HPP
#define LIGHTMAP_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "../render/model_renderer.hpp"
#include "../render/shadow_renderer.hpp"
#include "../render/skybox_renderer.hpp"
#include "scene.hpp"
#include "texture.hpp"

#include <array>                // std::array
#include <cstddef>              // std::size_t
#include <cstdint>              // std::uint32_t
#include <cstring>              // std::memcpy
#include <functional>           // std::function
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <lightmapper.h>        // lm..., LM_...
#include <memory>               // std::unique_ptr, std::shared_ptr, std::make_shared
#include <mutex>                // std::mutex, std::lock_guard
#include <stdexcept>            // std::runtime_error
#include <string_view>          // std::string_view
#include <type_traits>          // std::is_same_v
#include <unordered_map>        // std::unordered_map
#include <utility>              // std::move
#include <vector>               // std::vector
#include <xatlas.h>             // xatlas::...

struct lightmap_error : std::runtime_error {
	explicit lightmap_error(const auto& message)
		: std::runtime_error(message) {}
};

class lightmap_generator final {
public:
	static constexpr auto lightmap_channel_count = std::size_t{4};
	static constexpr auto lightmap_padding = std::size_t{4};
	static constexpr auto lightmap_internal_format = GLint{GL_RGBA16F};
	static constexpr auto lightmap_format = GLenum{GL_RGBA};
	static constexpr auto lightmap_type = GLenum{GL_FLOAT};
	static constexpr auto lightmap_options = texture_options{
		.max_anisotropy = 1.0f,
		.repeat = false,
		.use_linear_filtering = true,
		.use_mip_map = true,
	};
	static constexpr auto hemisphere_size = 64;
	static constexpr auto near_z = 0.001f;
	static constexpr auto far_z = 100.0f;
	static constexpr auto interpolation_passes = 2;
	static constexpr auto interpolation_threshold = 0.01f;
	static constexpr auto camera_to_surface_distance_modifier = 0.0f;

	using progress_callback = std::function<bool(std::string_view category, std::size_t bounce_index, std::size_t bounce_count, std::size_t object_index, std::size_t object_count,
		std::size_t mesh_index, std::size_t mesh_count, float progress)>;

	static auto generate_lightmap_coordinates(scene& scene, const progress_callback& callback) -> void {
		static_assert(std::is_same_v<model_index, GLuint> && sizeof(model_index) == 4, "This function assumes 32-bit model indices.");

		struct progress_data final {
			std::size_t object_index;
			std::size_t object_count;
			std::size_t mesh_index;
			std::size_t mesh_count;
			const progress_callback& callback;
			std::mutex mutex{};

			auto update(std::string_view category, float progress) -> bool {
				auto lock = std::lock_guard{mutex};
				return callback(category, 0, 0, object_index, object_count, mesh_index, mesh_count, progress);
			}
		};

		auto scene_progress = progress_data{
			.object_index = 0,
			.object_count = scene.objects.size(),
			.mesh_index = 0,
			.mesh_count = 0,
			.callback = callback,
		};

		static constexpr auto default_coordinates = std::array<vec2, 4>{
			vec2{0.0f, 0.0f},
			vec2{0.0f, 1.0f},
			vec2{1.0f, 0.0f},
			vec2{1.0f, 1.0f},
		};
		static constexpr auto coordinate_indices = std::array<model_index, 6>{
			model_index{0},
			model_index{1},
			model_index{3},
			model_index{0},
			model_index{3},
			model_index{2},
		};

		auto object_atlas = atlas_ptr{xatlas::Create()};
		auto generated_model_coordinate_scales = std::unordered_map<model*, vec2>{};
		for (auto& object : scene.objects) {
			const auto [it, inserted] = generated_model_coordinate_scales.try_emplace(object.model_ptr.get());
			if (inserted) {
				auto atlas = atlas_ptr{xatlas::Create()};
				xatlas::SetProgressCallback(
					atlas.get(),
					[](xatlas::ProgressCategory category, int progress, void* user_data) -> bool {
						return static_cast<progress_data*>(user_data)->update(xatlas::StringForEnum(category), static_cast<float>(progress) * 0.01f);
					},
					&scene_progress);
				scene_progress.mesh_index = 0;
				scene_progress.mesh_count = object.model_ptr->meshes().size();
				for (auto& mesh : object.model_ptr->meshes()) {
					if (const auto error = xatlas::AddMesh(atlas.get(),
							xatlas::MeshDecl{
								.vertexPositionData = glm::value_ptr(mesh.vertices()[0].position),
								.vertexNormalData = glm::value_ptr(mesh.vertices()[0].normal),
								.indexData = mesh.indices().data(),
								.vertexCount = static_cast<std::uint32_t>(mesh.vertices().size()),
								.vertexPositionStride = static_cast<std::uint32_t>(sizeof(model_vertex)),
								.vertexNormalStride = static_cast<std::uint32_t>(sizeof(model_vertex)),
								.indexCount = static_cast<std::uint32_t>(mesh.indices().size()),
								.indexFormat = xatlas::IndexFormat::UInt32, // NOTE: 32-bit model index assumed here.
							});
						error != xatlas::AddMeshError::Success) {
						throw lightmap_error{xatlas::StringForEnum(error)};
					}
					++scene_progress.mesh_index;
				}
				scene_progress.mesh_index = 0;
				scene_progress.mesh_count = 0;
				xatlas::Generate(atlas.get(),
					xatlas::ChartOptions{},
					xatlas::PackOptions{
						.padding = static_cast<std::uint32_t>(lightmap_padding),
					});

				const auto scale = vec2{static_cast<float>(atlas->width), static_cast<float>(atlas->height)};
				if (static_cast<std::size_t>(atlas->meshCount) != object.model_ptr->meshes().size()) {
					throw lightmap_error{"Invalid mesh count!"};
				}
				auto mesh_index = std::size_t{0};
				for (auto& mesh : object.model_ptr->meshes()) {
					const auto& new_mesh = atlas->meshes[mesh_index];

					const auto new_vertex_count = static_cast<std::size_t>(new_mesh.vertexCount);
					auto new_vertices = std::vector<model_vertex>{};
					new_vertices.reserve(new_vertex_count);
					for (const auto& new_vertex : std::span{new_mesh.vertexArray, new_vertex_count}) {
						const auto old_vertex_index = static_cast<std::size_t>(new_vertex.xref);
						if (old_vertex_index >= mesh.vertices().size()) {
							throw lightmap_error{"Invalid old vertex index!"};
						}
						const auto& old_vertex = mesh.vertices()[old_vertex_index];
						new_vertices.push_back(model_vertex{
							.position = old_vertex.position,
							.normal = old_vertex.normal,
							.tangent = old_vertex.tangent,
							.bitangent = old_vertex.bitangent,
							.texture_coordinates = old_vertex.texture_coordinates,
							.lightmap_coordinates = vec2{new_vertex.uv[0], new_vertex.uv[1]} / scale,
						});
					}

					const auto new_index_count = static_cast<std::size_t>(new_mesh.indexCount);
					auto new_indices = std::vector<model_index>{};
					new_indices.reserve(new_index_count);
					for (const auto& new_index : std::span{new_mesh.indexArray, new_index_count}) {
						const auto new_vertex_index = static_cast<model_index>(new_index);
						if (new_vertex_index >= new_vertex_count) {
							throw lightmap_error{"Invalid new vertex index!"};
						}
						new_indices.push_back(new_vertex_index);
					}

					mesh.set_vertices(std::move(new_vertices), std::move(new_indices));
					++mesh_index;
				}
				it->second = scale;
			}
			const auto scale = it->second;
			const auto coordinates = std::array<vec2, 4>{
				vec2{0.0f, 0.0f},
				vec2{0.0f, scale.y},
				vec2{scale.x, 0.0f},
				vec2{scale.x, scale.y},
			};
			xatlas::AddUvMesh(object_atlas.get(),
				xatlas::UvMeshDecl{
					.vertexUvData = coordinates.data(),
					.indexData = coordinate_indices.data(),
					.vertexCount = static_cast<std::uint32_t>(coordinates.size()),
					.vertexStride = static_cast<std::uint32_t>(sizeof(coordinates[0])),
					.indexCount = static_cast<std::uint32_t>(coordinate_indices.size()),
					.indexFormat = xatlas::IndexFormat::UInt32, // NOTE: 32-bit model index assumed here.
				});
			++scene_progress.object_index;
		}

		xatlas::AddUvMesh(object_atlas.get(),
			xatlas::UvMeshDecl{
				.vertexUvData = default_coordinates.data(),
				.indexData = coordinate_indices.data(),
				.vertexCount = static_cast<std::uint32_t>(default_coordinates.size()),
				.vertexStride = static_cast<std::uint32_t>(sizeof(default_coordinates[0])),
				.indexCount = static_cast<std::uint32_t>(coordinate_indices.size()),
				.indexFormat = xatlas::IndexFormat::UInt32, // NOTE: 32-bit model index assumed here.
			});
		scene_progress.object_index = 0;
		scene_progress.object_count = 0;
		scene_progress.mesh_index = 0;
		scene_progress.mesh_count = 0;
		scene_progress.update("Packing object coordinates", 0.0f);
		xatlas::Generate(object_atlas.get(),
			xatlas::ChartOptions{
				.useInputMeshUvs = true,
			},
			xatlas::PackOptions{
				.padding = static_cast<std::uint32_t>(lightmap_padding),
				.rotateChartsToAxis = false,
				.rotateCharts = false,
			});
		const auto atlas_scale = vec2{static_cast<float>(object_atlas->width), static_cast<float>(object_atlas->height)};
		if (static_cast<std::size_t>(object_atlas->meshCount) != scene.objects.size() + 1) {
			throw lightmap_error{"Invalid object count!"};
		}
		auto object_index = std::size_t{0};
		for (auto& object : scene.objects) {
			const auto& atlas_object = object_atlas->meshes[object_index];
			if (static_cast<std::size_t>(atlas_object.vertexCount) != default_coordinates.size()) {
				throw lightmap_error{"Invalid coordinate count!"};
			}
			const auto& min_vertex = atlas_object.vertexArray[0];
			if (static_cast<std::size_t>(min_vertex.xref) != 0) {
				throw lightmap_error{"Invalid minimum coordinate index!"};
			}
			const auto& max_vertex = atlas_object.vertexArray[3];
			if (static_cast<std::size_t>(max_vertex.xref) != 3) {
				throw lightmap_error{"Invalid maximum coordinate index!"};
			}
			const auto min_coordinates = vec2{min_vertex.uv[0], min_vertex.uv[1]} / atlas_scale;
			const auto max_coordinates = vec2{max_vertex.uv[0], max_vertex.uv[1]} / atlas_scale;
			if (min_coordinates.x > max_coordinates.x || min_coordinates.y > max_coordinates.y) {
				throw lightmap_error{"Invalid lightmap object coordinates!"};
			}
			object.lightmap_offset = min_coordinates;
			object.lightmap_scale = max_coordinates - min_coordinates;
			++object_index;
		}
		const auto& default_atlas_object = object_atlas->meshes[object_index];
		if (static_cast<std::size_t>(default_atlas_object.vertexCount) != default_coordinates.size()) {
			throw lightmap_error{"Invalid default coordinate count!"};
		}
		const auto& default_min_vertex = default_atlas_object.vertexArray[0];
		if (static_cast<std::size_t>(default_min_vertex.xref) != 0) {
			throw lightmap_error{"Invalid default minimum coordinate index!"};
		}
		const auto& default_max_vertex = default_atlas_object.vertexArray[3];
		if (static_cast<std::size_t>(default_max_vertex.xref) != 3) {
			throw lightmap_error{"Invalid default maximum coordinate index!"};
		}
		const auto default_min_coordinates = vec2{default_min_vertex.uv[0], default_min_vertex.uv[1]} / atlas_scale;
		const auto default_max_coordinates = vec2{default_max_vertex.uv[0], default_max_vertex.uv[1]} / atlas_scale;
		if (default_min_coordinates.x > default_max_coordinates.x || default_min_coordinates.y > default_max_coordinates.y) {
			throw lightmap_error{"Invalid default lightmap object coordinates!"};
		}
		scene.default_lightmap_offset = default_min_coordinates;
		scene.default_lightmap_scale = default_max_coordinates - default_min_coordinates;
		scene_progress.update("Packing object coordinates", 1.0f);
	}

	static auto reset_lightmap(scene& scene, vec3 sky_color) -> void {
		const auto sky_pixel = std::array<float, 4>{sky_color.x, sky_color.y, sky_color.z, 0.0f};
		scene.lightmap = std::make_shared<texture>(texture::create_2d(lightmap_internal_format, 1, 1, lightmap_format, lightmap_type, sky_pixel.data(), lightmap_options));
		scene.default_lightmap_offset = vec2{0.0f, 0.0f};
		scene.default_lightmap_scale = vec2{1.0f, 1.0f};
	}

	static auto bake_lightmap(scene& scene, vec3 sky_color, std::size_t resolution, std::size_t bounce_count, const progress_callback& callback) -> void {
		static_assert(std::is_same_v<model_index, GLuint> && sizeof(model_index) == 4, "This function assumes 32-bit model indices.");

		{
			auto shadow_baker = shadow_renderer{};
			for (const auto& light : scene.directional_lights) {
				shadow_baker.draw_directional_light(light);
			}
			for (const auto& light : scene.point_lights) {
				shadow_baker.draw_point_light(light);
			}
			for (const auto& light : scene.spot_lights) {
				shadow_baker.draw_spot_light(light);
			}
			for (const auto& object : scene.objects) {
				shadow_baker.draw_model(object.model_ptr, object.transform);
			}
			shadow_baker.render(mat4{1.0f}); // TODO: Make a view matrix that covers the entire scene.
		}

		auto model_baker = model_renderer{true};
		auto skybox_baker = skybox_renderer{};

		const auto width = static_cast<int>(resolution);
		const auto height = static_cast<int>(resolution);
		const auto channel_count = static_cast<int>(lightmap_channel_count);

		lightmapper_ptr lightmapper{
			lmCreate(hemisphere_size, near_z, far_z, sky_color.x, sky_color.y, sky_color.z, interpolation_passes, interpolation_threshold, camera_to_surface_distance_modifier)};
		if (!lightmapper) {
			throw lightmap_error{"Failed to initialize lightmapper!"};
		}

		for (auto bounce_index = std::size_t{0}; bounce_index < bounce_count; ++bounce_index) {
			auto pixels = std::vector<float>(resolution * resolution * lightmap_channel_count, 0.0f);
			lmSetTargetLightmap(lightmapper.get(), pixels.data(), width, height, channel_count);

			auto object_index = std::size_t{0};
			for (auto& object : scene.objects) {
				auto mesh_index = std::size_t{0};
				for (const auto& mesh : object.model_ptr->meshes()) {
					auto lightmap_coordinates = std::vector<vec2>();
					lightmap_coordinates.reserve(mesh.vertices().size());
					for (const auto& vertex : mesh.vertices()) {
						lightmap_coordinates.push_back(object.lightmap_offset + vertex.lightmap_coordinates * object.lightmap_scale);
					}
					lmSetGeometry(lightmapper.get(),
						glm::value_ptr(object.transform),
						LM_FLOAT,
						glm::value_ptr(mesh.vertices()[0].position),
						sizeof(model_vertex),
						LM_FLOAT,
						glm::value_ptr(mesh.vertices()[0].normal),
						sizeof(model_vertex),
						LM_FLOAT,
						lightmap_coordinates.data(),
						sizeof(lightmap_coordinates[0]),
						static_cast<int>(mesh.indices().size()),
						LM_UNSIGNED_INT, // NOTE: 32-bit model index assumed here.
						mesh.indices().data());
					auto viewport = std::array<int, 4>{};
					auto view_matrix = mat4{};
					auto projection_matrix = mat4{};
					while (lmBegin(lightmapper.get(), viewport.data(), glm::value_ptr(view_matrix), glm::value_ptr(projection_matrix))) {
						try {
							glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
							model_baker.resize(projection_matrix);
							skybox_baker.resize(projection_matrix);
							skybox_baker.draw_skybox(scene.sky->original());
							model_baker.draw_lightmap(scene.lightmap);
							model_baker.draw_environment(scene.sky);
							for (const auto& light : scene.directional_lights) {
								model_baker.draw_directional_light(light);
							}
							for (const auto& light : scene.point_lights) {
								model_baker.draw_point_light(light);
							}
							for (const auto& light : scene.spot_lights) {
								model_baker.draw_spot_light(light);
							}
							for (const auto& object : scene.objects) {
								model_baker.draw_model(object.model_ptr, object.transform, object.lightmap_offset, object.lightmap_scale);
							}
							model_baker.render(view_matrix, vec3{inverse(view_matrix)[3]});
							skybox_baker.render(mat3{view_matrix});
							if (!callback("Baking lightmaps",
									bounce_index,
									bounce_count,
									object_index,
									scene.objects.size(),
									mesh_index,
									object.model_ptr->meshes().size(),
									lmProgress(lightmapper.get()))) {
								throw lightmap_error{"Baking cancelled!"};
							}
						} catch (...) {
							lmEnd(lightmapper.get());
							throw;
						}
						lmEnd(lightmapper.get());
					}
					++mesh_index;
				}
				++object_index;
			}

			const auto lightmap_scale = vec2{static_cast<float>(resolution), static_cast<float>(resolution)};
			const auto default_offset = scene.default_lightmap_offset * lightmap_scale;
			const auto default_scale = scene.default_lightmap_scale * lightmap_scale;
			const auto default_x_begin = static_cast<std::size_t>(default_offset.x);
			const auto default_x_end = static_cast<std::size_t>(default_offset.x + default_scale.x);
			const auto default_y_begin = static_cast<std::size_t>(default_offset.y);
			const auto default_y_end = static_cast<std::size_t>(default_offset.y + default_scale.y);
			if (default_x_begin > default_x_end || default_y_begin > default_y_end || default_x_end > resolution || default_y_end > resolution) {
				throw lightmap_error{"Invalid default lightmap pixel coordinates!"};
			}
			for (auto y = default_y_begin; y < default_y_end; ++y) {
				for (auto x = default_x_begin; x < default_x_end; ++x) {
					auto* const pixel = &pixels[((y * resolution) + x) * lightmap_channel_count];
					const auto sky_pixel = std::array<float, 4>{sky_color.x, sky_color.y, sky_color.z, 0.0f};
					std::memcpy(pixel, sky_pixel.data(), lightmap_channel_count);
				}
			}

			auto temp = std::vector<float>(pixels.size(), 0.0f);
			for (int i = 0; i < 16; ++i) {
				lmImageDilate(pixels.data(), temp.data(), width, height, channel_count);
				lmImageDilate(temp.data(), pixels.data(), width, height, channel_count);
			}
			lmImageSmooth(pixels.data(), temp.data(), width, height, channel_count);
			lmImageDilate(temp.data(), pixels.data(), width, height, channel_count);

			scene.lightmap = std::make_shared<texture>(
				texture::create_2d(lightmap_internal_format, resolution, resolution, lightmap_format, lightmap_type, pixels.data(), lightmap_options));
		}
	}

private:
	struct atlas_deleter final {
		auto operator()(xatlas::Atlas* p) const noexcept -> void {
			xatlas::Destroy(p);
		}
	};
	using atlas_ptr = std::unique_ptr<xatlas::Atlas, atlas_deleter>;

	struct lightmapper_deleter final {
		auto operator()(lm_context* p) const noexcept -> void {
			lmDestroy(p);
		}
	};
	using lightmapper_ptr = std::unique_ptr<lm_context, lightmapper_deleter>;
};

#endif
