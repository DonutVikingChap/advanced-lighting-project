#ifndef UTF8_HPP
#define UTF8_HPP

#include <cstddef>     // std::ptrdiff_t
#include <iterator>    // std::forward_iterator_tag
#include <memory>      // std::addressof
#include <string_view> // std::string_view, std::u8string_view

inline constexpr auto utf8_error = char32_t{0xFFFFFFFF};

class utf8_iterator final {
public:
	using difference_type = std::ptrdiff_t;
	using value_type = char32_t;
	using reference = const value_type&;
	using pointer = const value_type*;
	using iterator_category = std::forward_iterator_tag;
	struct sentinel final {};

	constexpr utf8_iterator() noexcept = default;

	constexpr utf8_iterator(const char8_t* it, const char8_t* end) noexcept
		: m_it(it)
		, m_next(it)
		, m_end(end) {
		++*this;
	}

	[[nodiscard]] constexpr auto operator==(const utf8_iterator& other) const noexcept -> bool {
		return m_it == other.m_it;
	}

	[[nodiscard]] constexpr auto operator==(const sentinel&) const noexcept -> bool {
		return m_it == m_end;
	}

	[[nodiscard]] constexpr auto operator*() const noexcept -> reference {
		return m_code_point;
	}

	[[nodiscard]] constexpr auto operator->() const noexcept -> pointer {
		return &**this;
	}

	constexpr auto operator++() noexcept -> utf8_iterator& {
		m_it = m_next;
		m_code_point = next_code_point();
		return *this;
	}

	constexpr auto operator++(int) noexcept -> utf8_iterator {
		auto old = *this;
		++*this;
		return old;
	}

private:
	constexpr auto next_code_point() noexcept -> char32_t {
		if (m_next == m_end) {
			[[unlikely]] return utf8_error; // Reached end.
		}
		auto result = char32_t{};
		const auto c0 = *m_next++;
		if ((c0 & 0b10000000) == 0) { // 0-127
			[[likely]] result = c0;
		} else if ((c0 & 0b11100000) == 0b11000000) { // 128-2047
			if (m_end - m_next < 1) {
				m_next = m_end;
				[[unlikely]] return utf8_error; // Missing continuation.
			}
			const auto c1 = *m_next++;
			if ((c1 & 0b11000000) != 0b10000000) {
				[[unlikely]] return utf8_error; // Invalid continuation.
			}
			result = ((c0 & 0b11111)) << 6 | (c1 & 0b111111);
			if (result < 128) {
				[[unlikely]] return utf8_error; // Overlong sequence.
			}
		} else if ((c0 & 0b11110000) == 0b11100000) { // 2048-65535
			if (m_end - m_next < 2) {
				m_next = m_end;
				[[unlikely]] return utf8_error; // Missing continuation.
			}
			const auto c1 = *m_next++;
			const auto c2 = *m_next++;
			if ((c1 & 0b11000000) != 0b10000000 || (c2 & 0b11000000) != 0b10000000) {
				[[unlikely]] return utf8_error; // Invalid continuation.
			}
			result = ((c0 & 0b1111) << 12) | ((c1 & 0b111111) << 6) | (c2 & 0b111111);
			if (result < 2048) {
				[[unlikely]] return utf8_error; // Overlong sequence.
			}
			if (result >= 0xD800 && result <= 0xDFFF) {
				[[unlikely]] return utf8_error; // Surrogate code point.
			}
		} else if ((c0 & 0b11111000) == 0b11110000) { // 65536-1114111
			if (m_end - m_next < 3) {
				m_next = m_end;
				[[unlikely]] return utf8_error; // Missing continuation.
			}
			const auto c1 = *m_next++;
			const auto c2 = *m_next++;
			const auto c3 = *m_next++;
			if ((c1 & 0b11000000) != 0b10000000 || (c2 & 0b11000000) != 0b10000000 || (c3 & 0b11000000) != 0b10000000) {
				[[unlikely]] return utf8_error; // Invalid continuation.
			}
			result = ((c0 & 0b111) << 18) | ((c1 & 0b111111) << 12) | ((c2 & 0b111111) << 6) | (c3 & 0b111111);
			if (result < 65536) {
				[[unlikely]] return utf8_error; // Overlong sequence.
			}
			if (result > 1114111) {
				[[unlikely]] return utf8_error; // Invalid code point.
			}
		} else {
			[[unlikely]] return utf8_error; // Invalid byte.
		}
		return result;
	}

	const char8_t* m_it = nullptr;
	const char8_t* m_next = nullptr;
	const char8_t* m_end = nullptr;
	char32_t m_code_point = 0;
};

class utf8_view final {
public:
	using iterator = utf8_iterator;
	using difference_type = iterator::difference_type;
	using value_type = iterator::value_type;
	using reference = iterator::reference;
	using pointer = iterator::pointer;
	using iterator_category = iterator::iterator_category;
	using sentinel = iterator::sentinel;

	constexpr utf8_view() noexcept = default;

	constexpr utf8_view(std::u8string_view str) noexcept
		: m_begin(str.data(), str.data() + str.size()) {}

	explicit utf8_view(std::string_view str) noexcept
		: m_begin(reinterpret_cast<const char8_t*>(str.data()), reinterpret_cast<const char8_t*>(str.data() + str.size())) {}

	[[nodiscard]] constexpr auto begin() const noexcept -> const iterator& {
		return m_begin;
	}

	[[nodiscard]] constexpr auto end() const noexcept -> sentinel { // NOLINT(readability-convert-member-functions-to-static)
		return sentinel{};
	}

private:
	utf8_iterator m_begin{};
};

#endif
