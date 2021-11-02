#ifndef PASSKEY_HPP
#define PASSKEY_HPP

template <typename T>
class passkey {
	friend T;
	constexpr passkey() noexcept {}
};

#endif
