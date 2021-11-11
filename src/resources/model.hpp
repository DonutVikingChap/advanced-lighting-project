#ifndef MODEL_HPP
#define MODEL_HPP

#include "../core/glsl.hpp"
#include "../core/opengl.hpp"
#include "mesh.hpp"
#include "texture.hpp"

#include <algorithm>            // std::ranges::find_if
#include <assimp/Importer.hpp>  // Assimp::Importer
#include <assimp/postprocess.h> // ai...
#include <assimp/scene.h>       // ai...
#include <cstdint>              // std::uint8_t
#include <fmt/format.h>         // fmt::format
#include <iterator>             // std::distance
#include <memory>               // std::shared_ptr
#include <span>                 // std::span
#include <stdexcept>            // std::runtime_error, std::exception
#include <string>               // std::string
#include <tuple>                // std::tuple
#include <utility>              // std::move
#include <vector>               // std::vector

struct model_error : std::runtime_error {
	explicit model_error(const auto& message)
		: std::runtime_error(message) {}
};

struct model_vertex final {
	vec3 position{};
	vec3 normal{};
	vec3 tangent{};
	vec3 bitangent{};
	vec2 texture_coordinates{};
};

using model_index = GLuint;

struct model_material final {
	std::uint8_t albedo_texture_offset{};
	std::uint8_t normal_texture_offset{};
	std::uint8_t roughness_texture_offset{};
	std::uint8_t metallic_texture_offset{};
};

class model_mesh final {
public:
	static constexpr auto primitive_type = GLenum{GL_TRIANGLES};
	static constexpr auto index_type = GLenum{GL_UNSIGNED_INT};

	model_mesh(std::span<const model_vertex> vertices, std::span<const model_index> indices, const model_material& material)
		: m_mesh(GL_STATIC_DRAW, GL_STATIC_DRAW, vertices, indices,
			  std::tuple{
				  &model_vertex::position,
				  &model_vertex::normal,
				  &model_vertex::tangent,
				  &model_vertex::bitangent,
				  &model_vertex::texture_coordinates,
			  })
		, m_material(material)
		, m_vertex_count(vertices.size())
		, m_index_count(indices.size()) {}

	[[nodiscard]] auto material() const noexcept -> const model_material& {
		return m_material;
	}

	[[nodiscard]] auto vertex_count() const noexcept -> std::size_t {
		return m_vertex_count;
	}

	[[nodiscard]] auto index_count() const noexcept -> std::size_t {
		return m_index_count;
	}

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_mesh.get();
	}

private:
	mesh<model_vertex, model_index> m_mesh;
	model_material m_material;
	std::size_t m_vertex_count;
	std::size_t m_index_count;
};

struct model_texture_info final {
	std::string filename{};
	bool hdr = false;
};

class model final {
public:
	[[nodiscard]] static auto load(const char* filename) -> model {
		auto result = model{};
		auto importer = Assimp::Importer{};
		const auto* const scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace);
		if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || !scene->mRootNode) {
			throw model_error{fmt::format("Failed to load model \"{}\": {}", filename, importer.GetErrorString())};
		}
		try {
			result.add_node(*scene->mRootNode, *scene);
		} catch (const std::exception& e) {
			throw model_error{fmt::format("Failed to load model \"{}\": {}", filename, e.what())};
		}
		return result;
	}

	[[nodiscard]] auto meshes() const noexcept -> std::span<const model_mesh> {
		return m_meshes;
	}

	[[nodiscard]] auto texture_info() const noexcept -> std::span<const model_texture_info> {
		return m_textures;
	}

private:
	model() noexcept = default;

	[[nodiscard]] auto add_texture(const aiMaterial& mat, aiTextureType type, const char* default_name) -> std::uint8_t {
		auto name = aiString{};
		if (const auto texture_count = mat.GetTextureCount(type); texture_count == 0u) {
			name = default_name;
		} else if (texture_count == 1u) {
			mat.GetTexture(type, 0u, &name);
		} else {
			throw model_error{"Materials cannot have multiple textures of the same type."};
		}
		if (const auto it = std::ranges::find_if(m_textures, [&](const auto& info) { return info.filename == name.C_Str(); }); it != m_textures.end()) {
			return static_cast<std::uint8_t>(std::distance(m_textures.begin(), it));
		}
		const auto offset = static_cast<std::uint8_t>(m_textures.size());
		auto filename = std::string{name.C_Str()};
		const auto hdr = filename.ends_with(".hdr");
		m_textures.emplace_back(std::move(filename), hdr);
		return offset;
	}

	auto add_mesh(const aiMesh& mesh, const aiScene& scene) -> void {
		const auto zero_vector = aiVector3D{};
		auto vertices = std::vector<model_vertex>{};
		for (auto i = 0u; i < mesh.mNumVertices; ++i) {
			const auto& position = mesh.mVertices[i];
			const auto& normal = mesh.mNormals[i];
			const auto& tangent = (mesh.mTangents) ? mesh.mTangents[i] : zero_vector;
			const auto& bitangent = (mesh.mBitangents) ? mesh.mBitangents[i] : zero_vector;
			const auto& texture_coordinates = (mesh.mTextureCoords[0]) ? mesh.mTextureCoords[0][i] : zero_vector;
			vertices.push_back(model_vertex{
				.position = vec3{position.x, position.y, position.z},
				.normal = vec3{normal.x, normal.y, normal.z},
				.tangent = vec3{tangent.x, tangent.y, tangent.z},
				.bitangent = vec3{bitangent.x, bitangent.y, bitangent.z},
				.texture_coordinates = vec2{texture_coordinates.x, texture_coordinates.y},
			});
		}

		auto indices = std::vector<model_index>{};
		for (auto i = 0u; i < mesh.mNumFaces; ++i) {
			const auto& face = mesh.mFaces[i];
			for (auto j = 0u; j < face.mNumIndices; ++j) {
				indices.push_back(face.mIndices[j]);
			}
		}

		const auto& mat = *scene.mMaterials[mesh.mMaterialIndex];
		const auto material = model_material{
			.albedo_texture_offset = add_texture(mat, aiTextureType_DIFFUSE, "default_albedo.png"),
			.normal_texture_offset = add_texture(mat, aiTextureType_NORMALS, "default_normal.png"),
			.roughness_texture_offset = add_texture(mat, aiTextureType_SPECULAR, "default_roughness.png"),
			.metallic_texture_offset = add_texture(mat, aiTextureType_SHININESS, "default_metallic.png"),
		};

		m_meshes.emplace_back(vertices, indices, material);
	}

	auto add_node(const aiNode& node, const aiScene& scene) -> void {
		for (auto i = 0u; i < node.mNumMeshes; ++i) {
			add_mesh(*scene.mMeshes[node.mMeshes[i]], scene);
		}
		for (auto i = 0u; i < node.mNumChildren; ++i) {
			add_node(*node.mChildren[i], scene);
		}
	}

	std::vector<model_mesh> m_meshes{};
	std::vector<model_texture_info> m_textures{};
};

class textured_model final {
public:
	textured_model(std::shared_ptr<model> model, std::vector<std::shared_ptr<texture>> textures)
		: m_model(std::move(model))
		, m_meshes(m_model->meshes())
		, m_textures(std::move(textures)) {}

	[[nodiscard]] auto original() const noexcept -> const std::shared_ptr<model>& {
		return m_model;
	}

	[[nodiscard]] auto meshes() const noexcept -> std::span<const model_mesh> {
		return m_meshes;
	}

	[[nodiscard]] auto texture_info() const noexcept -> std::span<const model_texture_info> {
		return m_model->texture_info();
	}

	[[nodiscard]] auto textures() const noexcept -> std::span<const std::shared_ptr<texture>> {
		return m_textures;
	}

private:
	std::shared_ptr<model> m_model;
	std::span<const model_mesh> m_meshes;
	std::vector<std::shared_ptr<texture>> m_textures;
};

#endif
