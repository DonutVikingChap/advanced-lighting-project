#ifndef PASSKEY_HPP
#define PASSKEY_HPP

template <typename T>
class passkey final {
	friend T;
	constexpr passkey() noexcept {}
};

#endif
