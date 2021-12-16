#ifndef CALCULATOR_HPP
#define CALCULATOR_HPP

#include <algorithm> // std::min, std::max
#include <cmath> // std::exp, std::sqrt, std::pow, std::abs, std::sin, std::cos, std::tan, std::asin, std::acos, std::atan, std::log10, std::log, std::round, std::floor, std::ceil, std::atan2, std::remainder
#include <cstddef>      // std::size_t
#include <cstdint>      // std::uint8_t
#include <cstdio>       // std::sscanf
#include <cstdlib>      // std::abs
#include <fmt/format.h> // fmt::format
#include <memory>       // std::unique_ptr, std::make_unique
#include <stdexcept>    // std::runtime_error
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <type_traits>  // std::is_integral_v, std::is_unsigned_v

// X(name, str, type, precedence)
#define ENUM_CALCULATOR_OPERATIONS(X) \
	X(invalid, "", invalid, 0) \
	X(constant, "", constant, 0) \
	X(variable, "", variable, 0) \
	X(left_parenthesis, "(", left_parenthesis, 0) \
	X(right_parenthesis, ")", right_parenthesis, 0) \
	X(separator, ",", separator, 0) \
	X(exp, "exp", unary_function, 0) \
	X(sqrt, "sqrt", unary_function, 0) \
	X(sin, "sin", unary_function, 0) \
	X(cos, "cos", unary_function, 0) \
	X(tan, "tan", unary_function, 0) \
	X(asin, "asin", unary_function, 0) \
	X(acos, "acos", unary_function, 0) \
	X(atan, "atan", unary_function, 0) \
	X(log, "log", unary_function, 0) \
	X(ln, "ln", unary_function, 0) \
	X(round, "round", unary_function, 0) \
	X(floor, "floor", unary_function, 0) \
	X(ceil, "ceil", unary_function, 0) \
	X(abs, "abs", unary_function, 0) \
	X(min, "min", binary_function, 0) \
	X(max, "max", binary_function, 0) \
	X(atan2, "atan2", binary_function, 0) \
	X(pow, "pow", unary_function, 0) \
	X(not_, "!", unary_operator, 6) \
	X(bitwise_not, "~", unary_operator, 6) \
	X(negative, "-", unary_operator, 6) \
	X(modulo, "%", binary_operator_left_associative, 4) \
	X(multiply, "*", binary_operator_left_associative, 4) \
	X(divide, "/", binary_operator_left_associative, 4) \
	X(add, "+", binary_operator_left_associative, 3) \
	X(subtract, "-", binary_operator_left_associative, 3) \
	X(left_shift, "<<", binary_operator_left_associative, 3) \
	X(right_shift, ">>", binary_operator_left_associative, 3) \
	X(and_, "&&", binary_operator_left_associative, 2) \
	X(bitwise_and, "&", binary_operator_left_associative, 2) \
	X(or_, "||", binary_operator_left_associative, 1) \
	X(bitwise_or, "|", binary_operator_left_associative, 1) \
	X(bitwise_xor, "^", binary_operator_left_associative, 1) \
	X(equal, "==", binary_operator_left_associative, 0) \
	X(not_equal, "!=", binary_operator_left_associative, 0) \
	X(less_than, "<", binary_operator_left_associative, 0) \
	X(less_than_or_equal, "<=", binary_operator_left_associative, 0) \
	X(greater_than, ">", binary_operator_left_associative, 0) \
	X(greater_than_or_equal, ">=", binary_operator_left_associative, 0)

struct calculator_error : std::runtime_error {
	explicit calculator_error(const auto& message)
		: std::runtime_error(message) {}
};

template <typename T>
class calculator final {
public:
	struct default_variable_finder final {
		[[nodiscard]] auto operator()(std::string_view, const T*&, bool&) const -> bool {
			return false;
		}
	};

	template <typename VariableFinder = default_variable_finder>
	void parse(std::string_view str, VariableFinder variable_finder = {}) {
		struct queued_token final {
			constexpr queued_token() noexcept = default;

			constexpr queued_token(calculator_token token, token_type type) noexcept
				: token(token)
				, type(type) {}

			calculator_token token{};
			token_type type{};
		};

		// Shunting yard algorithm.
		auto output_queue = std::vector<queued_token>{};
		auto operator_stack = std::vector<queued_token>{};

		auto current_token = queued_token{};
		while (true) {
			while (!str.empty() && is_whitespace(str.front())) {
				str.remove_prefix(1);
			}
			if (str.empty()) {
				break;
			}
			current_token.token = read_token(current_token.type, str);
			current_token.type = current_token.token.op.type();
			switch (current_token.type) {
				case token_type::constant: [[fallthrough]];
				case token_type::variable: output_queue.emplace_back(std::move(current_token.token), current_token.type); break;
				case token_type::unary_function: [[fallthrough]];
				case token_type::binary_function: [[fallthrough]];
				case token_type::unary_operator: operator_stack.emplace_back(std::move(current_token.token), current_token.type); break;
				case token_type::separator:
					while (!operator_stack.empty() && operator_stack.back().type != token_type::left_parenthesis) {
						output_queue.push_back(std::move(operator_stack.back()));
						operator_stack.pop_back();
					}
					if (operator_stack.empty()) {
						throw calculator_error{"Mismatched parentheses"};
					}
					break;
				case token_type::binary_operator_left_associative: [[fallthrough]];
				case token_type::binary_operator_right_associative:
					while (!operator_stack.empty() &&
						((operator_stack.back().type == token_type::binary_operator_left_associative &&
							 current_token.token.op.precedence() <= operator_stack.back().token.op.precedence()) ||
							(operator_stack.back().type == token_type::binary_operator_right_associative &&
								current_token.token.op.precedence() < operator_stack.back().token.op.precedence()) ||
							(operator_stack.back().type == token_type::unary_operator && current_token.token.op.precedence() < operator_stack.back().token.op.precedence()))) {
						output_queue.push_back(std::move(operator_stack.back()));
						operator_stack.pop_back();
					}
					operator_stack.emplace_back(std::move(current_token.token), current_token.type);
					break;
				case token_type::left_parenthesis: operator_stack.emplace_back(std::move(current_token.token), current_token.type); break;
				case token_type::right_parenthesis:
					while (!operator_stack.empty() && operator_stack.back().type != token_type::left_parenthesis) {
						output_queue.push_back(std::move(operator_stack.back()));
						operator_stack.pop_back();
					}
					if (operator_stack.empty()) {
						throw calculator_error{"Mismatched parentheses"};
					}
					operator_stack.pop_back();
					if (!operator_stack.empty() &&
						(operator_stack.back().type == token_type::unary_function || operator_stack.back().type == token_type::binary_function ||
							operator_stack.back().type == token_type::unary_operator)) {
						output_queue.push_back(std::move(operator_stack.back()));
						operator_stack.pop_back();
					}
					break;
				default: throw calculator_error{"Invalid token"};
			}
		}
		while (!operator_stack.empty()) {
			if (operator_stack.back().type == token_type::left_parenthesis || operator_stack.back().type == token_type::right_parenthesis) {
				throw calculator_error{"Mismatched parentheses"};
			}
			output_queue.push_back(std::move(operator_stack.back()));
			operator_stack.pop_back();
		}

		// Convert output queue to tree.
		auto nodes = std::vector<node>{};
		for (const auto& queued : output_queue) {
			switch (queued.type) {
				case token_type::constant: {
					const auto buffer = std::string{queued.token.str};
					if constexpr (std::is_integral_v<T>) {
						auto value = 0ll;
						std::sscanf(buffer.c_str(), "%lli", &value);
						nodes.emplace_back(static_cast<T>(value));
					} else {
						auto value = 0.0;
						std::sscanf(buffer.c_str(), "%lf", &value);
						nodes.emplace_back(static_cast<T>(value));
					}
					break;
				}
				case token_type::variable: {
					const T* variable = nullptr;
					auto is_constant = false;
					if (!variable_finder(queued.token.str, variable, is_constant) || !variable) {
						throw calculator_error{fmt::format("Unknown variable \"{}\"", queued.token.str)};
					}
					if (is_constant) {
						nodes.emplace_back(*variable);
					} else {
						nodes.emplace_back(variable);
					}
					break;
				}
				case token_type::unary_function: {
					if (nodes.empty()) {
						throw calculator_error{fmt::format("Unary function \"{}\" is missing parameters", queued.token.op.string())};
					}
					auto param_a = std::move(nodes.back());
					nodes.pop_back();
					nodes.emplace_back(queued.token.op, std::make_unique<node>(std::move(param_a)));
					break;
				}
				case token_type::binary_function: {
					if (nodes.size() < 2) {
						throw calculator_error{fmt::format("Binary function \"{}\" is missing parameters", queued.token.op.string())};
					}
					auto param_b = std::move(nodes.back());
					nodes.pop_back();
					auto param_a = std::move(nodes.back());
					nodes.pop_back();
					nodes.emplace_back(queued.token.op, std::make_unique<node>(std::move(param_a)), std::make_unique<node>(std::move(param_b)));
					break;
				}
				case token_type::unary_operator: {
					if (nodes.empty()) {
						throw calculator_error{fmt::format("Unary operator \"{}\" is missing parameters", queued.token.op.string())};
					}
					auto param_a = std::move(nodes.back());
					nodes.pop_back();
					nodes.emplace_back(queued.token.op, std::make_unique<node>(std::move(param_a)));
					break;
				}
				case token_type::binary_operator_left_associative: [[fallthrough]];
				case token_type::binary_operator_right_associative: {
					if (nodes.size() < 2) {
						throw calculator_error{fmt::format("Binary operator \"{}\" is missing parameters", queued.token.op.string())};
					}
					auto param_b = std::move(nodes.back());
					nodes.pop_back();
					auto param_a = std::move(nodes.back());
					nodes.pop_back();
					nodes.emplace_back(queued.token.op, std::make_unique<node>(std::move(param_a)), std::make_unique<node>(std::move(param_b)));
					break;
				}
				default: break;
			}
		}

		if (nodes.empty()) {
			throw calculator_error{"No expression"};
		}

		if (nodes.size() > 1) {
			throw calculator_error{"Too many expressions"};
		}

		m_root = std::move(nodes.front());
	}

	auto optimize() -> void {
		m_root.optimize();
	}

	[[nodiscard]] auto evaluate() const -> T {
		return m_root.evaluate();
	}

	[[nodiscard]] auto is_valid() const noexcept -> bool {
		return m_root.op.id != operation_id::invalid;
	}

private:
	static constexpr auto pi = 3.14159265358979323846264338327950288;

	[[nodiscard]] static constexpr auto is_whitespace(char ch) noexcept -> bool {
		return ch == ' ' || ch == '\t';
	}

	[[nodiscard]] static constexpr auto is_letter(char ch) noexcept -> bool {
		return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
	}

	[[nodiscard]] static constexpr auto is_decimal_digit(char ch) noexcept -> bool {
		return ch >= '0' && ch <= '9';
	}

	[[nodiscard]] static constexpr auto is_hexadecimal_digit(char ch) noexcept -> bool {
		return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
	}

	enum class token_type : std::uint8_t {
		invalid,
		constant,
		variable,
		left_parenthesis,
		right_parenthesis,
		separator,
		unary_function,
		binary_function,
		unary_operator,
		binary_operator_left_associative,
		binary_operator_right_associative,
	};

	enum class operation_id : std::uint8_t {
#define X(name, str, type, precedence) name,
		ENUM_CALCULATOR_OPERATIONS(X)
#undef X
	};

	struct operation final {
		operation_id id{};

		[[nodiscard]] constexpr auto string() const noexcept -> std::string_view {
			switch (id) {
#define X(name, str, type, precedence) \
	case operation_id::name: return str;
				ENUM_CALCULATOR_OPERATIONS(X)
#undef X
			}
			return std::string_view{};
		}

		[[nodiscard]] constexpr auto type() const noexcept -> token_type {
			switch (id) {
#define X(name, str, type, precedence) \
	case operation_id::name: return token_type::type;
				ENUM_CALCULATOR_OPERATIONS(X)
#undef X
			}
			return token_type::invalid;
		}

		[[nodiscard]] constexpr auto precedence() const noexcept -> int {
			switch (id) {
#define X(name, str, type, precedence) \
	case operation_id::name: return precedence;
				ENUM_CALCULATOR_OPERATIONS(X)
#undef X
			}
			return 0;
		}

		[[nodiscard]] constexpr auto is_operator() const noexcept -> bool {
			const auto t = type();
			return id != operation_id::negative &&
				(t == token_type::unary_operator || t == token_type::binary_operator_left_associative || t == token_type::binary_operator_right_associative ||
					t == token_type::left_parenthesis || t == token_type::right_parenthesis || t == token_type::separator);
		}

		[[nodiscard]] static constexpr auto get_string_operation(std::string_view token_string) noexcept -> operation {
#define X(name, str, type, precedence) \
	if constexpr (token_type::type == token_type::unary_function || token_type::type == token_type::binary_function) { \
		if (token_string == std::string_view{str}) { \
			return operation{operation_id::name}; \
		} \
	}
			ENUM_CALCULATOR_OPERATIONS(X)
#undef X
			return operation{operation_id::variable};
		}

		[[nodiscard]] static constexpr auto get_operator_operation(std::string_view token_string) noexcept -> operation {
			if (token_string.size() >= 2) {
#define X(name, str, type, precedence) \
	if constexpr (operation{operation_id::name}.is_operator() && operation{operation_id::name}.string().size() > 1) { \
		if (token_string == std::string_view{str}) { \
			return operation{operation_id::name}; \
		} \
	}
				ENUM_CALCULATOR_OPERATIONS(X)
#undef X
			}

#define X(name, str, type, precedence) \
	if constexpr (operation{operation_id::name}.is_operator() && operation{operation_id::name}.string().size() == 1) { \
		if (token_string.front() == std::string_view{str}.front()) { \
			return operation{operation_id::name}; \
		} \
	}
			ENUM_CALCULATOR_OPERATIONS(X)
#undef X
			return operation{operation_id::invalid};
		}

		[[nodiscard]] constexpr auto operator==(const operation&) const noexcept -> bool = default;
	};

	class node final {
	public:
		node() noexcept
			: m_constant(T{}) {}

		~node() = default;

		explicit node(T constant) noexcept
			: m_op(operation{operation_id::constant})
			, m_constant(constant) {}

		explicit node(const T* variable) noexcept
			: m_op(operation{operation_id::variable})
			, m_variable(variable) {}

		node(operation op, std::unique_ptr<node> a)
			: m_op(op)
			, m_a(std::move(a)) {}

		node(operation op, std::unique_ptr<node> a, std::unique_ptr<node> b)
			: m_op(op)
			, m_a(std::move(a))
			, m_b(std::move(b)) {}

		node(const node&) = delete;

		node(node&& other) noexcept {
			*this = std::move(other);
		}

		auto operator=(const node&) -> node& = delete;

		auto operator=(node&& other) noexcept -> node& {
			m_op = other.m_op;
			switch (m_op.id) {
				case operation_id::constant:
					m_constant = other.m_constant;
					m_a.reset();
					m_b.reset();
					break;
				case operation_id::variable:
					m_variable = other.m_variable;
					m_a.reset();
					m_b.reset();
					break;
				default:
					m_a = std::move(other.m_a);
					m_b = std::move(other.m_b);
					break;
			}
			return *this;
		}

		[[nodiscard]] auto evaluate() const -> T {
			switch (m_op.id) {
				case operation_id::invalid: return T{0};
				case operation_id::constant: return m_constant;
				case operation_id::variable: return *m_variable;
				case operation_id::left_parenthesis: return T{0};
				case operation_id::right_parenthesis: return T{0};
				case operation_id::separator: return T{0};
				case operation_id::exp: return static_cast<T>(std::exp(m_a->evaluate()));
				case operation_id::sqrt: return static_cast<T>(std::sqrt(m_a->evaluate()));
				case operation_id::sin: return static_cast<T>(std::sin(m_a->evaluate()));
				case operation_id::cos: return static_cast<T>(std::cos(m_a->evaluate()));
				case operation_id::tan: return static_cast<T>(std::tan(m_a->evaluate()));
				case operation_id::asin: return static_cast<T>(std::asin(m_a->evaluate()));
				case operation_id::acos: return static_cast<T>(std::acos(m_a->evaluate()));
				case operation_id::atan: return static_cast<T>(std::atan(m_a->evaluate()));
				case operation_id::log: return static_cast<T>(std::log10(m_a->evaluate()));
				case operation_id::ln: return static_cast<T>(std::log(m_a->evaluate()));
				case operation_id::round: return static_cast<T>(std::round(m_a->evaluate()));
				case operation_id::floor: return static_cast<T>(std::floor(m_a->evaluate()));
				case operation_id::ceil: return static_cast<T>(std::ceil(m_a->evaluate()));
				case operation_id::abs:
					if constexpr (std::is_unsigned_v<T>) {
						return m_a->evaluate();
					} else {
						return std::abs(m_a->evaluate());
					}
				case operation_id::min: return std::min(m_a->evaluate(), m_b->evaluate());
				case operation_id::max: return std::max(m_a->evaluate(), m_b->evaluate());
				case operation_id::atan2: return std::atan2(m_a->evaluate(), m_b->evaluate());
				case operation_id::pow: return static_cast<T>(std::pow(m_a->evaluate(), m_b->evaluate()));
				case operation_id::not_: return !m_a->evaluate(); ;
				case operation_id::bitwise_not: return ~m_a->evaluate(); ;
				case operation_id::negative: return -m_a->evaluate();
				case operation_id::modulo:
					if constexpr (std::is_integral_v<T>) {
						return m_a->evaluate() % m_b->evaluate();
					} else {
						return std::remainder(m_a->evaluate(), m_b->evaluate());
					}
				case operation_id::multiply: return m_a->evaluate() * m_b->evaluate();
				case operation_id::divide: return m_a->evaluate() / m_b->evaluate();
				case operation_id::add: return m_a->evaluate() + m_b->evaluate();
				case operation_id::subtract: return m_a->evaluate() - m_b->evaluate();
				case operation_id::left_shift: return m_a->evaluate() << m_b->evaluate();
				case operation_id::right_shift: return m_a->evaluate() >> m_b->evaluate();
				case operation_id::and_: return m_a->evaluate() && m_b->evaluate();
				case operation_id::bitwise_and: return m_a->evaluate() & m_b->evaluate();
				case operation_id::or_: return m_a->evaluate() || m_b->evaluate();
				case operation_id::bitwise_or: return m_a->evaluate() | m_b->evaluate();
				case operation_id::bitwise_xor: return m_a->evaluate() ^ m_b->evaluate();
				case operation_id::equal: return m_a->evaluate() == m_b->evaluate();
				case operation_id::not_equal: return m_a->evaluate() != m_b->evaluate();
				case operation_id::less_than: return m_a->evaluate() < m_b->evaluate();
				case operation_id::less_than_or_equal: return m_a->evaluate() <= m_b->evaluate();
				case operation_id::greater_than: return m_a->evaluate() > m_b->evaluate();
				case operation_id::greater_than_or_equal: return m_a->evaluate() >= m_b->evaluate();
			}
			return 0;
		}

		auto optimize() -> bool {
			switch (m_op.type()) {
				case token_type::constant: return true;
				case token_type::variable: return false;
				case token_type::unary_function: [[fallthrough]];
				case token_type::unary_operator: {
					if (m_a->optimize()) {
						m_constant = evaluate();
						m_op = operation_id::constant;
						m_a.reset();
						return true;
					}
					return false;
				}
				case token_type::binary_function: [[fallthrough]];
				case token_type::binary_operator_left_associative: [[fallthrough]];
				case token_type::binary_operator_right_associative: {
					const auto optimized_a = m_a->optimize();
					const auto optimized_b = m_b->optimize();
					if (optimized_a && optimized_b) {
						m_constant = evaluate();
						m_op = operation_id::constant;
						m_a.reset();
						m_b.reset();
						return true;
					}
					return false;
				}
			}
			return false;
		}

	private:
		operation m_op{operation_id::invalid};
		union {
			T m_constant;
			const T* m_variable;
		};
		std::unique_ptr<node> m_a{};
		std::unique_ptr<node> m_b{};
	};

	struct calculator_token final {
		operation op{};
		std::string_view str{};
	};

	static auto read_token(token_type previous_token_type, std::string_view& str) -> calculator_token {
		auto result = calculator_token{};
		if (str.empty()) {
			return result;
		}

		if (is_decimal_digit(str.front())) {
			if (previous_token_type == token_type::right_parenthesis || previous_token_type == token_type::variable) {
				result.op = operation{operation_id::multiply};
			} else {
				auto i = std::size_t{1};
				if (str.starts_with("0x")) {
					i = 2;
					while (i < str.size() && (is_hexadecimal_digit(str[i]))) {
						++i;
					}
				} else {
					auto dot = false;
					while (i < str.size() && (is_decimal_digit(str[i]) || (str[i] == '.' && !dot))) {
						if (str[i] == '.') {
							dot = true;
						}
						++i;
					}
				}
				result.str = str.substr(0, i);
				str.remove_prefix(i);
				result.op = operation{operation_id::constant};
			}
		} else if (is_letter(str.front()) || str.front() == '_') {
			if (previous_token_type == token_type::right_parenthesis || previous_token_type == token_type::constant) {
				result.op = operation{operation_id::multiply};
			} else {
				auto i = std::size_t{1};
				while (i < str.size() && (is_letter(str[i]) || is_decimal_digit(str[i]) || str[i] == '_')) {
					++i;
				}
				result.str = str.substr(0, i);
				str.remove_prefix(i);
				result.op = operation::get_string_operation(result.str);
				if (result.op.type() == token_type::unary_function || result.op.type() == token_type::binary_function) {
					result.str = {};
				}
			}
		} else {
			result.op = operation::get_operator_operation(str.substr(0, 2));
			if (result.op.id == operation_id::left_parenthesis &&
				(previous_token_type == token_type::right_parenthesis || previous_token_type == token_type::constant || previous_token_type == token_type::variable)) {
				result.op = operation{operation_id::multiply};
			} else {
				str.remove_prefix(result.op.string().size());
				if (result.op.id == operation_id::subtract &&
					(previous_token_type == token_type::invalid || previous_token_type == token_type::binary_operator_left_associative ||
						previous_token_type == token_type::binary_operator_right_associative || previous_token_type == token_type::left_parenthesis)) {
					result.op = operation{operation_id::negative};
				}
			}
		}
		return result;
	}

	node m_root{};
};

#undef ENUM_CALCULATOR_OPERATIONS

#endif
