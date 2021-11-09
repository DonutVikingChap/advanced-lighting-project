#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include "../core/handle.hpp"
#include "../core/opengl.hpp"

class framebuffer final {
public:
	[[nodiscard]] static auto get_default() -> framebuffer& {
		static auto fb = framebuffer{GLuint{0}};
		return fb;
	}

	framebuffer() = default;

	[[nodiscard]] auto get() const noexcept -> GLuint {
		return m_fbo.get();
	}

private:
	explicit framebuffer(GLuint handle) noexcept
		: m_fbo(handle) {}

	struct framebuffer_deleter final {
		auto operator()(GLuint p) const noexcept -> void {
			glDeleteFramebuffers(1, &p);
		}
	};
	using framebuffer_ptr = unique_handle<framebuffer_deleter>;

	framebuffer_ptr m_fbo{[] {
		auto fbo = GLuint{};
		glGenFramebuffers(1, &fbo);
		if (fbo == 0) {
			throw opengl_error{"Failed to create framebuffer object!"};
		}
		return fbo;
	}()};
};

#endif
