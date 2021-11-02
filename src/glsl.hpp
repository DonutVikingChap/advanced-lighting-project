#ifndef GLSL_HPP
#define GLSL_HPP

#include "opengl.hpp"

#include <glm/glm.hpp> // glm::...
#include <type_traits> // std::is_same_v

static_assert(std::is_same_v<float, GLfloat>);
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat2 = glm::mat2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;
using sampler2D = GLint;
using sampler2DShadow = GLint;
using sampler2DArray = GLint;
using sampler2DArrayShadow = GLint;
using samplerCube = GLint;
using samplerCubeShadow = GLint;

#endif
