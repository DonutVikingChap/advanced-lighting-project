#ifndef HANDLE_HPP
#define HANDLE_HPP

#include "opengl.hpp"

#include <utility> // std::exchange

template <typename Deleter>
class unique_handle final {
public:
	constexpr unique_handle() noexcept = default;

	constexpr explicit unique_handle(GLuint handle) noexcept
		: m_handle(handle) {}

	constexpr ~unique_handle() {
		Deleter{}(m_handle);
	}

	unique_handle(const unique_handle&) = delete;

	constexpr unique_handle(unique_handle&& other) noexcept
		: m_handle(other.release()) {}

	auto operator=(const unique_handle&) -> unique_handle& = delete;

	constexpr auto operator=(unique_handle&& other) noexcept -> unique_handle& {
		reset(other.release());
		return *this;
	}

	constexpr auto reset(GLuint handle = 0) noexcept -> void {
		Deleter{}(std::exchange(m_handle, handle));
	}

	constexpr auto release() noexcept -> GLuint {
		return std::exchange(m_handle, 0);
	}

	[[nodiscard]] constexpr auto get() const noexcept -> GLuint {
		return m_handle;
	}

	[[nodiscard]] constexpr explicit operator bool() const noexcept {
		return get() != 0;
	}

	[[nodiscard]] constexpr auto operator<=>(const unique_handle&) const = default;

private:
	GLuint m_handle = 0;
};

#endif
