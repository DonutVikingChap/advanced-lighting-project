#ifndef PREPROCESSOR_HPP
#define PREPROCESSOR_HPP

#include <algorithm>     // std::all_of, std::find, std::find_if, std::find_if_not, std::search
#include <charconv>      // std::from_chars, std::errc
#include <cstddef>       // std::size_t, std::ptrdiff_t
#include <cstdint>       // std::uint8_t
#include <fmt/format.h>  // fmt::format, fmt::to_string
#include <fstream>       // std::ifstream
#include <sstream>       // std::ostringstream
#include <stdexcept>     // std::runtime_error
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::move
#include <vector>        // std::vector

struct preprocessor_error : std::runtime_error {
	explicit preprocessor_error(std::string_view filename, std::size_t line_number, std::string_view message)
		: std::runtime_error(fmt::format("{}:{}: {}", filename, line_number, message)) {}
};

struct preprocessor_environment final {
	struct function_macro_definition final {
		std::vector<std::string> parameters{};
		std::string definition{};
		bool variadic = false;
	};

	using name_set = std::unordered_set<std::string>;
	using macro_definitions = std::unordered_map<std::string_view, std::string>;
	using function_macro_definitions = std::unordered_map<std::string_view, function_macro_definition>;

	name_set defined_names{};
	macro_definitions macros{};
	function_macro_definitions function_macros{};
};

class preprocessor final {
public:
	using file_content_map = std::unordered_map<std::string, std::string>;

	static auto process_file(std::string_view filename, std::string input, std::vector<std::string>& output, preprocessor_environment& environment, file_content_map& file_cache)
		-> void {
		auto line_number = std::size_t{0};
		auto& file_contents = file_cache.try_emplace(std::string{filename}).first->second;
		file_contents = std::move(input);
		preprocessor{filename, line_number, file_contents, output, environment, file_cache, true}.process_file();
	}

private:
	enum class terminator_token : std::uint8_t {
		end_of_input,
		elif_directive,
		else_directive,
		endif_directive,
		endfor_directive,
	};

	using argument_map = std::unordered_map<std::string_view, std::string>;

	static constexpr auto macro_file = std::string_view{"__FILE__"};
	static constexpr auto macro_line = std::string_view{"__LINE__"};
	static constexpr auto macro_variadic_arguments = std::string_view{"__VA_ARGS__"};

	static constexpr auto directive_include = std::string_view{"include"};
	static constexpr auto directive_for = std::string_view{"for"};
	static constexpr auto directive_endfor = std::string_view{"endfor"};
	static constexpr auto directive_define = std::string_view{"define"};
	static constexpr auto directive_undef = std::string_view{"undef"};
	static constexpr auto directive_ifdef = std::string_view{"ifdef"};
	static constexpr auto directive_ifndef = std::string_view{"ifndef"};
	static constexpr auto directive_if = std::string_view{"if"};
	static constexpr auto directive_elif = std::string_view{"elif"};
	static constexpr auto directive_else = std::string_view{"else"};
	static constexpr auto directive_endif = std::string_view{"endif"};
	static constexpr auto directive_error = std::string_view{"error"};

	[[nodiscard]] static constexpr auto is_whitespace(char ch) noexcept -> bool {
		return ch == ' ' || ch == '\t' || ch == '\n';
	}

	[[nodiscard]] static constexpr auto is_valid_definition_char(char ch) noexcept -> bool {
		return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_';
	}

	preprocessor(std::string_view filename, std::size_t& line_number, std::string_view input, std::vector<std::string>& output, preprocessor_environment& environment,
		file_content_map& file_cache, bool active) noexcept
		: m_filename(filename)
		, m_line_number(line_number)
		, m_input(input)
		, m_output(output)
		, m_environment(environment)
		, m_file_cache(file_cache)
		, m_active(active) {}

	auto process_file() -> void {
		switch (process()) {
			case terminator_token::end_of_input: break;
			case terminator_token::elif_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected elif"};
			case terminator_token::else_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected else"};
			case terminator_token::endif_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected endif"};
			case terminator_token::endfor_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected endfor"};
		}
	}

	[[nodiscard]] auto process() -> terminator_token {
		auto line = std::string{};
		while (!m_input.empty()) {
			const auto line_end = m_input.find('\n');
			auto current_line = m_input.substr(0, line_end);
			m_input.remove_prefix((line_end == std::string_view::npos) ? m_input.size() : line_end + 1);
			if (const auto comment_pos = current_line.find("//"); comment_pos != std::string::npos) {
				current_line = current_line.substr(0, comment_pos);
			}

			line = current_line;
			while (!line.empty() && line.back() == '\\') {
				const auto additional_line_end = m_input.find('\n');
				auto additional_line = m_input.substr(0, additional_line_end);
				m_input.remove_prefix((additional_line_end == std::string_view::npos) ? m_input.size() : additional_line_end + 1);
				if (const auto comment_pos = additional_line.find("//"); comment_pos != std::string::npos) {
					additional_line = additional_line.substr(0, comment_pos);
				}

				line.back() = '\n';
				line.append(additional_line);
				++m_line_number;
			}
			++m_line_number;

			if (const auto begin = line.find_first_not_of(" \t\n"); begin != std::string::npos) {
				if (line[begin] == '#') {
					auto str = std::string_view{line}.substr(begin + 1);
					if (str.compare(0, directive_include.size(), directive_include) == 0) {
						str.remove_prefix(directive_include.size());
						process_include(str);
						continue;
					}
					if (str.compare(0, directive_for.size(), directive_for) == 0) {
						str.remove_prefix(directive_for.size());
						process_for(str);
						continue;
					}
					if (str.compare(0, directive_define.size(), directive_define) == 0) {
						str.remove_prefix(directive_define.size());
						process_define(str);
					} else if (str.compare(0, directive_undef.size(), directive_undef) == 0) {
						str.remove_prefix(directive_undef.size());
						process_undef(str);
					} else if (str.compare(0, directive_ifdef.size(), directive_ifdef) == 0) {
						str.remove_prefix(directive_ifdef.size());
						process_ifdef(str);
						continue;
					} else if (str.compare(0, directive_ifndef.size(), directive_ifndef) == 0) {
						str.remove_prefix(directive_ifndef.size());
						process_ifndef(str);
						continue;
					} else if (str.compare(0, directive_if.size(), directive_if) == 0) {
						if (m_active) {
							line.push_back('\n');
							m_output.push_back(std::move(line));
						}
						process_if();
						continue;
					} else if (str.compare(0, directive_elif.size(), directive_elif) == 0) {
						if (m_active) {
							line.push_back('\n');
							m_output.push_back(std::move(line));
						}
						return terminator_token::elif_directive;
					} else if (str.compare(0, directive_else.size(), directive_else) == 0) {
						return terminator_token::else_directive;
					} else if (str.compare(0, directive_endif.size(), directive_endif) == 0) {
						return terminator_token::endif_directive;
					} else if (str.compare(0, directive_error.size(), directive_error) == 0) {
						str.remove_prefix(directive_error.size());
						if (const auto message_begin = str.find_first_not_of(" \t\n"); message_begin != std::string_view::npos) {
							throw preprocessor_error{m_filename, m_line_number, str.substr(message_begin)};
						}
						throw preprocessor_error{m_filename, m_line_number, "Missing error message"};
					} else if (str.compare(0, directive_endfor.size(), directive_endfor) == 0) {
						return terminator_token::endfor_directive;
					}
					if (m_active) {
						line.push_back('\n');
						m_output.push_back(std::move(line));
					}
				} else if (m_active) {
					expand_line(line, nullptr, "");
					const auto comment_string = fmt::format("// file=\"{}\", line={}", m_filename, m_line_number);
					for (auto i = std::size_t{0}; i < line.size(); ++i) {
						if (line[i] == '\n') {
							line.insert(i, comment_string);
							i += comment_string.size();
						}
					}
					line.append(comment_string);
					line.push_back('\n');
					m_output.push_back(std::move(line));
				}
			}
		}
		return terminator_token::end_of_input;
	}

	[[nodiscard]] auto read_index(std::string_view str, std::size_t& i) -> std::size_t {
		i = str.find_first_not_of(" \t\n", i);
		if (i == std::string_view::npos) {
			throw preprocessor_error{m_filename, m_line_number, "Missing index"};
		}
		const auto begin = i;
		i = str.find_first_not_of(" \t\n", begin + 1);
		const auto index_string = str.substr(begin, i - begin);
		auto result = std::size_t{};
		if (std::from_chars(index_string.data(), index_string.data() + index_string.size(), result).ec != std::errc{}) {
			throw preprocessor_error{m_filename, m_line_number, "Invalid index"};
		}
		return result;
	}

	auto expand_line(std::string& line, const argument_map* args, std::string_view va_args) -> void {
		auto expand_again = bool{};
		do {
			expand_again = false;
			for (auto it = std::find_if(line.begin(), line.end(), is_valid_definition_char); it != line.end();) {
				auto end = std::find_if_not(it + 1, line.end(), is_valid_definition_char);
				const auto name = std::string_view{&*it, static_cast<std::size_t>(end - it)};

				const auto replace = [&](std::string_view str) -> void {
					if (str == name) {
						it = end;
					} else {
						if (it != line.begin() && *(it - 1) == '#') {
							auto stringified = std::string{};
							stringified.reserve(str.size() + 2);
							stringified.push_back('\"');
							stringified.append(str);
							stringified.push_back('\"');
							str = stringified;
							--it;
						}
						const auto offset = it - line.begin();
						line.replace(it, end, str);
						it = line.begin() + offset + static_cast<std::ptrdiff_t>(str.size());
						expand_again = true;
					}
				};

				constexpr auto find_argument = [](const argument_map* args, std::string_view name) -> const std::string* {
					if (args) {
						if (const auto arg_it = args->find(name); arg_it != args->end()) {
							return &arg_it->second;
						}
					}
					return nullptr;
				};

				const auto read_arguments = [&](argument_map& new_args, std::string& new_va_args, bool variadic, const std::vector<std::string>& parameters) -> void {
					const auto arguments_begin = static_cast<std::size_t>(end - line.begin());
					if (arguments_begin >= line.size() || line[arguments_begin] != '(') {
						throw preprocessor_error{m_filename, m_line_number, "Missing arguments"};
					}
					const auto arguments_inner_begin = arguments_begin + 1;
					auto arguments_inner_end = std::string::npos;
					auto arguments_end = std::string::npos;

					auto parenthesis_level = 0u;
					auto reading_arg = false;
					auto arg_count = std::size_t{0};
					for (auto i = arguments_inner_begin; true; ++i) {
						if (i >= line.size()) {
							throw preprocessor_error{m_filename, m_line_number, fmt::format("Missing end of argument list for \"{}\"", name)};
						}

						if (line[i] == ')') {
							if (parenthesis_level == 0) {
								arguments_inner_end = i;
								arguments_end = arguments_inner_end + 1;
								break;
							}
							--parenthesis_level;
						} else if (line[i] == '(') {
							++parenthesis_level;
						} else if (line[i] == ',' || is_whitespace(line[i])) {
							if (reading_arg && parenthesis_level == 0 && line[i] == ',') {
								reading_arg = false;
								++arg_count;
							}
						} else {
							reading_arg = true;
						}
					}
					end = line.begin() + static_cast<std::ptrdiff_t>(arguments_end);

					auto param_str = line.substr(arguments_inner_begin, arguments_inner_end - arguments_inner_begin);
					expand_line(param_str, args, va_args);

					auto argv = std::vector<std::string_view>{};
					auto arg_begin = std::size_t{0};
					parenthesis_level = 0u;
					reading_arg = false;
					for (auto i = std::size_t{0}; true; ++i) {
						if (i >= param_str.size()) {
							if (reading_arg) {
								argv.emplace_back(param_str.data() + arg_begin, i - arg_begin);
							}
							break;
						}

						if (param_str[i] == ')') {
							if (parenthesis_level == 0) {
								throw preprocessor_error{m_filename, m_line_number, "Mismatched parentheses"};
							}
							--parenthesis_level;
							if (parenthesis_level == 0) {
								if (!reading_arg) {
									throw preprocessor_error{m_filename, m_line_number, "Mismatched parentheses"};
								}
								reading_arg = false;
								argv.emplace_back(param_str.data() + arg_begin, i + 1 - arg_begin);
							}
						} else if (param_str[i] == '(') {
							if (!reading_arg) {
								reading_arg = true;
								arg_begin = i;
							}
							++parenthesis_level;
						} else if (param_str[i] == ',' || is_whitespace(param_str[i])) {
							if (reading_arg && parenthesis_level == 0 && param_str[i] == ',') {
								reading_arg = false;
								argv.emplace_back(param_str.data() + arg_begin, i - arg_begin);
							}
						} else if (!reading_arg) {
							reading_arg = true;
							arg_begin = i;
						}
					}

					if (variadic && argv.size() >= parameters.size()) {
						for (auto i = std::size_t{0}; i < argv.size(); ++i) {
							if (i < parameters.size()) {
								if (const auto& [it, inserted] = new_args.try_emplace(parameters[i], argv[i]); !inserted) {
									it->second = argv[i];
								}
							} else {
								if (new_va_args.empty()) {
									new_va_args = argv[i];
								} else {
									new_va_args.append(", ");
									new_va_args.append(argv[i]);
								}
							}
						}

						expand_line(new_va_args, args, va_args);
					} else if (argv.size() == parameters.size()) {
						for (auto i = std::size_t{0}; i < argv.size(); ++i) {
							if (const auto& [it, inserted] = new_args.try_emplace(parameters[i], argv[i]); !inserted) {
								it->second = argv[i];
							}
						}
					} else {
						throw preprocessor_error{
							m_filename, m_line_number, fmt::format("Invalid number of arguments arguments for \"{}\" ({}/{})", name, argv.size(), parameters.size())};
					}
				};

				if (name == macro_file) {
					replace(m_filename);
				} else if (name == macro_line) {
					replace(fmt::to_string(m_line_number));
				} else if (name == macro_variadic_arguments) {
					auto str = std::string{va_args};
					expand_line(str, args, va_args);
					replace(str);
				} else if (const auto* const arg = find_argument(args, name)) {
					auto str = std::string{*arg};
					if (str != name) {
						expand_line(str, args, va_args);
					}
					replace(str);
				} else if (const auto it_macro = m_environment.macros.find(name); it_macro != m_environment.macros.end()) {
					auto str = std::string{it_macro->second};
					if (str != name) {
						expand_line(str, args, va_args);
					}
					replace(str);
				} else if (const auto it_function_macro = m_environment.function_macros.find(name); it_function_macro != m_environment.function_macros.end()) {
					auto new_args = argument_map{};
					auto new_va_args = std::string{};
					read_arguments(new_args, new_va_args, it_function_macro->second.variadic, it_function_macro->second.parameters);
					auto str = std::string{it_function_macro->second.definition};
					if (str != name) {
						expand_line(str, &new_args, new_va_args);
					}
					replace(str);
				} else {
					it = end;
				}

				const auto concat_token = std::string_view{"##"};
				if (const auto concat_token_it = std::search(it, line.end(), concat_token.begin(), concat_token.end()); concat_token_it != line.end()) {
					if (const auto next_token_it = std::find_if(concat_token_it + static_cast<std::ptrdiff_t>(concat_token.size()), line.end(), is_valid_definition_char);
						next_token_it != line.end()) {
						if (std::all_of(it, concat_token_it, is_whitespace) &&
							std::all_of(concat_token_it + static_cast<std::ptrdiff_t>(concat_token.size()), next_token_it, is_whitespace)) {
							const auto offset = it - line.begin();
							line.erase(it, next_token_it);
							it = line.begin() + offset;
							expand_again = true;
							continue;
						}
					}
				}

				it = std::find_if(it, line.end(), is_valid_definition_char);
			}
		} while (expand_again);
	}

	auto define_macro(std::string_view name, std::string_view definition) -> void {
		m_environment.function_macros.erase(name);
		const auto name_it = m_environment.defined_names.emplace(name).first;
		const auto it = m_environment.macros.try_emplace(*name_it).first;
		it->second = definition;
	}

	auto define_function_macro(std::string_view name, std::string_view parameters, std::string_view definition) -> void {
		m_environment.macros.erase(name);
		const auto name_it = m_environment.defined_names.emplace(name).first;
		const auto it = m_environment.function_macros.try_emplace(*name_it).first;
		auto parameter_begin = std::size_t{0};
		auto parameter_end = std::size_t{0};
		while ((parameter_begin = parameters.find_first_not_of(" \t\n,", parameter_begin)) != std::string_view::npos) {
			parameter_end = parameters.find_first_of(" \t\n,", parameter_begin);
			const auto parameter = parameters.substr(parameter_begin, parameter_end - parameter_begin);
			if (parameter == "...") {
				if (it->second.variadic) {
					throw preprocessor_error{m_filename, m_line_number, "Variadic parameters before end of parameter list"};
				}
				it->second.variadic = true;
			} else {
				it->second.parameters.emplace_back(parameter);
			}
		}
		it->second.definition = definition;
	}

	auto process_include(std::string_view str) -> void {
		const auto begin = str.find_first_not_of(" \t");
		if (begin == std::string_view::npos) {
			throw preprocessor_error{m_filename, m_line_number, "Missing filename"};
		}
		const auto quote_char = str[begin];
		if (quote_char != '\"' && quote_char != '<') {
			throw preprocessor_error{m_filename, m_line_number, "Invalid filename quote"};
		}
		const auto quote_begin = begin + 1;
		const auto quote_end = str.find(quote_char, quote_begin);
		if (quote_end == std::string_view::npos) {
			throw preprocessor_error{m_filename, m_line_number, "Missing end quote"};
		}
		const auto quote = str.substr(quote_begin, quote_end - quote_begin);
		const auto [it, inserted] = m_file_cache.try_emplace(std::string{quote});
		const auto filename_prefix = m_filename.substr(0, m_filename.rfind('/') + 1);
		auto included_filename = std::string{filename_prefix} + it->first;
		if (inserted) {
			auto included_file = std::ifstream{};
			if (quote_char == '\"') {
				included_file.open(included_filename);
			}
			if (!included_file) {
				included_filename.erase(0, filename_prefix.size());
				included_file.open(included_filename);
				if (!included_file) {
					throw preprocessor_error{m_filename, m_line_number, fmt::format("Failed to open included file \"{}\"", included_filename)};
				}
			}
			auto stream = std::ostringstream{};
			stream << included_file.rdbuf();
			it->second = std::move(stream).str();
		}

		auto line_number = std::size_t{0};
		switch (preprocessor{m_filename, line_number, it->second, m_output, m_environment, m_file_cache, m_active}.process()) {
			case terminator_token::end_of_input: break;
			case terminator_token::elif_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected elif"};
			case terminator_token::else_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected else"};
			case terminator_token::endif_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected endif"};
			case terminator_token::endfor_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected endfor"};
		}
	}

	auto process_for(std::string_view str) -> void {
		const auto name_begin = str.find_first_not_of(" \t\n");
		if (name_begin == std::string_view::npos) {
			throw preprocessor_error{m_filename, m_line_number, "Missing name"};
		}
		const auto name_end = str.find_first_of(" \t\n(", name_begin);
		const auto name = std::string{str.substr(name_begin, name_end - name_begin)};

		auto line = std::string{str.substr(name_end)};
		expand_line(line, nullptr, "");
		auto i = std::size_t{0};
		auto index = read_index(line, i);
		const auto end_index = read_index(line, i);
		const auto input = m_input;
		while (index < end_index) {
			m_input = input;
			define_macro(name, fmt::to_string(index));
			switch (process()) {
				case terminator_token::end_of_input: throw preprocessor_error{m_filename, m_line_number, "Missing endfor"};
				case terminator_token::elif_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected elif"};
				case terminator_token::else_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected else"};
				case terminator_token::endif_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected endif"};
				case terminator_token::endfor_directive: break;
			}
			++index;
		}
	}

	auto process_define(std::string_view str) -> void {
		const auto name_begin = str.find_first_not_of(" \t\n");
		if (name_begin == std::string_view::npos) {
			throw preprocessor_error{m_filename, m_line_number, "Missing name"};
		}
		const auto name_end = str.find_first_of(" \t\n(", name_begin);
		const auto params_begin = (name_end == std::string_view::npos) ? std::string_view::npos : (str[name_end] == '(') ? name_end + 1 : std::string_view::npos;
		const auto params_end = (params_begin == std::string_view::npos) ? std::string_view::npos : str.find(')', params_begin);
		if (params_begin != std::string_view::npos && params_end == std::string_view::npos) {
			throw preprocessor_error{m_filename, m_line_number, "Missing end of parameter list"};
		}
		const auto def_begin = (params_end == std::string_view::npos) ? name_end : params_end + 1;
		const auto name = str.substr(name_begin, name_end - name_begin);
		if (!std::all_of(name.begin(), name.end(), is_valid_definition_char)) {
			throw preprocessor_error{m_filename, m_line_number, fmt::format("Invalid characters in definition name \"{}\"", name)};
		}
		const auto definition = (def_begin >= str.size()) ? std::string_view{} : str.substr(def_begin + 1);
		if (params_begin == std::string_view::npos) {
			define_macro(name, definition);
		} else {
			define_function_macro(name, str.substr(params_begin, params_end - params_begin), definition);
		}
	}

	auto process_undef(std::string_view str) -> void {
		const auto name_begin = str.find_first_not_of(" \t\n");
		if (name_begin == std::string_view::npos) {
			throw preprocessor_error{m_filename, m_line_number, "Missing name"};
		}
		const auto name_end = str.find_first_of(" \t\n(", name_begin);
		const auto name = std::string{str.substr(name_begin, name_end - name_begin)};
		m_environment.function_macros.erase(name);
		m_environment.macros.erase(name);
		m_environment.defined_names.erase(name);
	}

	auto process_ifdef(std::string_view str) -> void {
		const auto name_begin = str.find_first_not_of(" \t\n");
		if (name_begin == std::string_view::npos) {
			throw preprocessor_error{m_filename, m_line_number, "Missing name"};
		}
		const auto name_end = str.find_first_of(" \t\n(", name_begin);
		const auto name = std::string{str.substr(name_begin, name_end - name_begin)};
		process_conditional(m_environment.defined_names.count(name) != 0);
	}

	auto process_ifndef(std::string_view str) -> void {
		const auto name_begin = str.find_first_not_of(" \t\n");
		if (name_begin == std::string_view::npos) {
			throw preprocessor_error{m_filename, m_line_number, "Missing name"};
		}
		const auto name_end = str.find_first_of(" \t\n(", name_begin);
		const auto name = std::string{str.substr(name_begin, name_end - name_begin)};
		process_conditional(m_environment.defined_names.count(name) == 0);
	}

	auto process_conditional(bool condition) -> void {
		const auto was_active = m_active;
		m_active = was_active && condition;
		switch (process()) {
			case terminator_token::end_of_input: throw preprocessor_error{m_filename, m_line_number, "Missing endif"};
			case terminator_token::elif_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected elif"};
			case terminator_token::else_directive:
				m_active = was_active && !condition;
				switch (process()) {
					case terminator_token::end_of_input: throw preprocessor_error{m_filename, m_line_number, "Missing endif"};
					case terminator_token::elif_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected elif"};
					case terminator_token::else_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected else"};
					case terminator_token::endif_directive: break;
					case terminator_token::endfor_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected endfor"};
				}
				break;
			case terminator_token::endif_directive: break;
			case terminator_token::endfor_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected endfor"};
		}
		m_active = was_active;
	}

	auto process_if() -> void {
		while (true) {
			switch (process()) {
				case terminator_token::end_of_input: throw preprocessor_error{m_filename, m_line_number, "Missing endif"};
				case terminator_token::elif_directive: break;
				case terminator_token::else_directive: m_output.emplace_back("\n#else\n"); break;
				case terminator_token::endif_directive: m_output.emplace_back("\n#endif\n"); return;
				case terminator_token::endfor_directive: throw preprocessor_error{m_filename, m_line_number, "Unexpected endfor"};
			}
		}
	}

	std::string_view m_filename;
	std::size_t& m_line_number;
	std::string_view m_input;
	std::vector<std::string>& m_output;
	preprocessor_environment& m_environment;
	file_content_map& m_file_cache;
	bool m_active;
};

#endif
