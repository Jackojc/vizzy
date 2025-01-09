#ifndef VIZZY_HPP
#define VIZZY_HPP

#include <iostream>
#include <utility>

#include <array>
#include <optional>
#include <string_view>

#include <ostream>
#include <filesystem>

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/color.h>
#include <fmt/ostream.h>

// Definitions
namespace vizzy {
#define VIZZY_EXE "Vizzy"

#define VIZZY_WINDOW_WIDTH  1280
#define VIZZY_WINDOW_HEIGHT 720

// Bindings were generated as 4.3 core
#define VIZZY_OPENGL_VERSION_MAJOR 4
#define VIZZY_OPENGL_VERSION_MINOR 3
}  // namespace vizzy

// Concepts
namespace vizzy {
	template <typename T, typename U>
	concept Same = std::is_same_v<T, U>;
}

// Macros
namespace vizzy {

// Mark as unused.
#define VIZZY_IMPL_UNUSED0()
#define VIZZY_IMPL_UNUSED1(a)             (void)(a)
#define VIZZY_IMPL_UNUSED2(a, b)          (void)(a), VIZZY_IMPL_UNUSED1(b)
#define VIZZY_IMPL_UNUSED3(a, b, c)       (void)(a), VIZZY_IMPL_UNUSED2(b, c)
#define VIZZY_IMPL_UNUSED4(a, b, c, d)    (void)(a), VIZZY_IMPL_UNUSED3(b, c, d)
#define VIZZY_IMPL_UNUSED5(a, b, c, d, e) (void)(a), VIZZY_IMPL_UNUSED4(b, c, d, e)

#define VIZZY_VA_NUM_ARGS_IMPL(_0, _1, _2, _3, _4, _5, N, ...) N
#define VIZZY_VA_NUM_ARGS(...)                                 VIZZY_VA_NUM_ARGS_IMPL(100, ##__VA_ARGS__, 5, 4, 3, 2, 1, 0)

#define VIZZY_UNUSED_IMPL_(nargs) VIZZY_IMPL_UNUSED##nargs
#define VIZZY_UNUSED_IMPL(nargs)  VIZZY_UNUSED_IMPL_(nargs)
#define VIZZY_UNUSED(...)         VIZZY_UNUSED_IMPL(VIZZY_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

// String macros
#define VIZZY_CSTR(s) /* Convert c-string into a string_view before decaying to pointer. */ \
	std::string_view { \
		s, ((const char*)s) + (sizeof(s) - 1) \
	}

#define VIZZY_STR_IMPL_(x) #x
#define VIZZY_STR(x)       VIZZY_STR_IMPL_(x)

#define VIZZY_CAT_IMPL_(x, y) x##y
#define VIZZY_CAT(x, y)       VIZZY_CAT_IMPL_(x, y)

#define VIZZY_VAR(x) VIZZY_CAT(var_, VIZZY_CAT(x, VIZZY_CAT(__LINE__, _)))

// Location info
#define VIZZY_LINEINFO "[" __FILE__ ":" VIZZY_STR(__LINE__) "]"

#define VIZZY_LOCATION_FILE __FILE__
#define VIZZY_LOCATION_LINE VIZZY_STR(__LINE__)
#define VIZZY_LOCATION_FUNC __func__

// Utils
#define VIZZY_MAX(a, b) ((a > b) ? a : b)
#define VIZZY_MIN(a, b) ((a < b) ? a : b)
}  // namespace vizzy

// Logging
namespace vizzy {

#define VIZZY_RESET "\x1b[0m"
#define VIZZY_BOLD  "\x1b[1m"

#define VIZZY_FG_BLACK   "\x1b[30m"
#define VIZZY_FG_RED     "\x1b[31m"
#define VIZZY_FG_GREEN   "\x1b[32m"
#define VIZZY_FG_YELLOW  "\x1b[33m"
#define VIZZY_FG_BLUE    "\x1b[34m"
#define VIZZY_FG_MAGENTA "\x1b[35m"
#define VIZZY_FG_CYAN    "\x1b[36m"
#define VIZZY_FG_WHITE   "\x1b[37m"

#define VIZZY_FG_BLACK_BRIGHT   "\x1b[90m"
#define VIZZY_FG_RED_BRIGHT     "\x1b[91m"
#define VIZZY_FG_GREEN_BRIGHT   "\x1b[92m"
#define VIZZY_FG_YELLOW_BRIGHT  "\x1b[93m"
#define VIZZY_FG_BLUE_BRIGHT    "\x1b[94m"
#define VIZZY_FG_MAGENTA_BRIGHT "\x1b[95m"
#define VIZZY_FG_CYAN_BRIGHT    "\x1b[96m"
#define VIZZY_FG_WHITE_BRIGHT   "\x1b[97m"

#define VIZZY_BG_BLACK   "\x1b[40m"
#define VIZZY_BG_RED     "\x1b[41m"
#define VIZZY_BG_GREEN   "\x1b[42m"
#define VIZZY_BG_YELLOW  "\x1b[43m"
#define VIZZY_BG_BLUE    "\x1b[44m"
#define VIZZY_BG_MAGENTA "\x1b[45m"
#define VIZZY_BG_CYAN    "\x1b[46m"
#define VIZZY_BG_WHITE   "\x1b[47m"

#define VIZZY_BG_BLACK_BRIGHT   "\x1b[100m"
#define VIZZY_BG_RED_BRIGHT     "\x1b[101m"
#define VIZZY_BG_GREEN_BRIGHT   "\x1b[102m"
#define VIZZY_BG_YELLOW_BRIGHT  "\x1b[103m"
#define VIZZY_BG_BLUE_BRIGHT    "\x1b[104m"
#define VIZZY_BG_MAGENTA_BRIGHT "\x1b[105m"
#define VIZZY_BG_CYAN_BRIGHT    "\x1b[106m"
#define VIZZY_BG_WHITE_BRIGHT   "\x1b[107m"

// Log colours
#define VIZZY_COLOUR_DEBUG    VIZZY_FG_CYAN_BRIGHT
#define VIZZY_COLOUR_TRACE    VIZZY_FG_MAGENTA_BRIGHT
#define VIZZY_COLOUR_WARN     VIZZY_FG_BLUE
#define VIZZY_COLOUR_ERROR    VIZZY_FG_RED
#define VIZZY_COLOUR_OKAY     VIZZY_FG_GREEN
#define VIZZY_COLOUR_EXPR     VIZZY_FG_MAGENTA
#define VIZZY_COLOUR_HERE     VIZZY_FG_YELLOW_BRIGHT
#define VIZZY_COLOUR_FUNCTION VIZZY_BG_BLUE VIZZY_FG_BLACK

#define VIZZY_LOG_LEVELS \
	X(Debug, ".", "debg", VIZZY_COLOUR_DEBUG) \
	X(Trace, "-", "trce", VIZZY_COLOUR_TRACE) \
	X(Warn, "*", "warn", VIZZY_COLOUR_WARN) \
	X(Error, "!", "fail", VIZZY_COLOUR_ERROR) \
	X(Okay, "^", "okay", VIZZY_COLOUR_OKAY) \
	X(Expr, "=", "expr", VIZZY_COLOUR_EXPR) \
	X(Here, "/", "here", VIZZY_COLOUR_HERE) \
	X(Function, ">", "func", VIZZY_COLOUR_FUNCTION)

#define X(x, y, z, w) x,
	enum class LogKind {
		VIZZY_LOG_LEVELS
	};
#undef X

	namespace detail {
#define X(x, y, z, w) VIZZY_CSTR(y),
		inline std::array log_to_str = { VIZZY_LOG_LEVELS };
#undef X

#define X(x, y, z, w) VIZZY_CSTR(z),
		inline std::array log_human_to_str = { VIZZY_LOG_LEVELS };
#undef X

#define X(x, y, z, w) VIZZY_CSTR(w),
		inline std::array log_colour_to_str = { VIZZY_LOG_LEVELS };
#undef X

	}  // namespace detail

	[[nodiscard]] inline std::string_view log_to_str(LogKind log) {
		return detail::log_to_str[static_cast<size_t>(log)];
	}

	[[nodiscard]] inline std::string_view log_human_to_str(LogKind log) {
		return detail::log_human_to_str[static_cast<size_t>(log)];
	}

	[[nodiscard]] inline std::string_view log_colour_to_str(LogKind log) {
		return detail::log_colour_to_str[static_cast<size_t>(log)];
	}

	// Log as human readable string by default.
	inline std::ostream& operator<<(std::ostream& os, LogKind log) {
		return (os << detail::log_human_to_str[static_cast<size_t>(log)]);
	}

#undef VIZZY_LOG_LEVELS

}  // namespace vizzy

template <>
struct fmt::formatter<vizzy::LogKind>: fmt::ostream_formatter {};

namespace vizzy {

	struct LogInfo {
		std::string_view file;
		std::string_view line;
		std::string_view func;
	};

	namespace detail {
		// Case where we have a format string and additional argument.
		template <typename T, typename... Ts>
		inline decltype(auto) log_fmt(std::ostream& os, std::string_view fmt, T&& arg, Ts&&... args) {
			fmt::print(os, fmt::runtime(fmt), std::forward<T>(arg), std::forward<Ts>(args)...);
		}

		// First argument is not a string but we still want to print it so we supply default format string.
		template <typename T>
		inline decltype(auto) log_fmt(std::ostream& os, T&& arg) {
			fmt::print(os, "{}", std::forward<T>(arg));
		}
	}  // namespace detail

	template <typename... Ts>
	inline decltype(auto) log(std::ostream& os, LogKind kind, std::optional<LogInfo> info, Ts&&... args) {
		auto colour = vizzy::log_colour_to_str(kind);
		auto sigil = vizzy::log_to_str(kind);
		auto human = vizzy::log_human_to_str(kind);

		fmt::print(os,
			"{}[{}]" VIZZY_RESET
			" "
			"{}[{}]" VIZZY_RESET " ",
			colour,
			sigil,
			colour,
			human);

		if (info.has_value()) {
			auto [file, line, func] = info.value();

			std::string_view func_name = func == "operator()" ? "<lamba>" : func;
			std::string_view separator = (sizeof...(Ts) > 0) ? VIZZY_FG_BLACK_BRIGHT "│" VIZZY_RESET : "";

			auto path = std::filesystem::relative(file).native();

			fmt::print(os, "`" VIZZY_BOLD "{}" VIZZY_RESET "` " VIZZY_FG_BLACK_BRIGHT "│" VIZZY_RESET " ", func_name);
			fmt::print(os, VIZZY_FG_BLACK_BRIGHT "({}:{})" VIZZY_RESET " {} ", path, line, separator);
		}

		if constexpr (sizeof...(Ts) > 0) {
			detail::log_fmt(os, std::forward<Ts>(args)...);
		}

		fmt::print(os, "\n");
	}

	template <typename... Ts>
	inline decltype(auto) log(LogKind kind, std::optional<LogInfo> info, Ts&&... args) {
		return log(std::cerr, kind, info, std::forward<Ts>(args)...);
	}

	template <typename... Ts>
	inline decltype(auto) log(LogKind kind, Ts&&... args) {
		return log(std::cerr, kind, std::nullopt, std::forward<Ts>(args)...);
	}

	namespace detail {
		template <typename T>
		inline decltype(auto) inspect(
			std::ostream& os, std::optional<LogInfo> info, std::string_view expr_str, T&& expr) {
			log(os, LogKind::Expr, info, "{} = {}", expr_str, std::forward<T>(expr));
			return std::forward<T>(expr);
		}
	}

// Log with file location info included.
#define VIZZY_LOG(...) \
	do { \
		[VIZZY_VAR(func) = VIZZY_LOCATION_FUNC]<typename... VIZZY_VAR(Ts)>( \
			vizzy::LogKind VIZZY_VAR(kind), VIZZY_VAR(Ts)&&... VIZZY_VAR(args)) { \
			vizzy::log(std::cerr, \
				VIZZY_VAR(kind), \
				vizzy::LogInfo { VIZZY_LOCATION_FILE, VIZZY_LOCATION_LINE, VIZZY_VAR(func) }, \
				std::forward<VIZZY_VAR(Ts)>(VIZZY_VAR(args))...); \
		}(__VA_ARGS__); \
	} while (0)

// Unfortunately these convenience macros require at least one argument due to quirk of __VA_ARGS__.
#define VIZZY_DEBUG(...) \
	do { \
		VIZZY_LOG(vizzy::LogKind::Debug, __VA_ARGS__); \
	} while (0)

#define VIZZY_TRACE(...) \
	do { \
		VIZZY_LOG(vizzy::LogKind::Trace, __VA_ARGS__); \
	} while (0)

#define VIZZY_WARN(...) \
	do { \
		VIZZY_LOG(vizzy::LogKind::Warn, __VA_ARGS__); \
	} while (0)

#define VIZZY_ERROR(...) \
	do { \
		VIZZY_LOG(vizzy::LogKind::Error, __VA_ARGS__); \
	} while (0)

#define VIZZY_OKAY(...) \
	do { \
		VIZZY_LOG(vizzy::LogKind::Okay, __VA_ARGS__); \
	} while (0)

#define VIZZY_FUNCTION() \
	do { \
		VIZZY_LOG(vizzy::LogKind::Function); \
	} while (0)

// Evaluate expression and return its result while also printing both. (Useful for debugging something inside a complex
// expression without needing to create temporary variables)
#define VIZZY_INSPECT(expr) \
	[&, VIZZY_VAR(func) = VIZZY_LOCATION_FUNC]() { \
		return vizzy::detail::inspect(std::cerr, \
			vizzy::LogInfo { VIZZY_LOCATION_FILE, VIZZY_LOCATION_LINE, VIZZY_VAR(func) }, \
			VIZZY_STR(expr), \
			(expr)); \
	}()

// The printf debuggers dream
#define VIZZY_WHEREAMI() \
	do { \
		VIZZY_LOG(vizzy::LogKind::Here, \
			VIZZY_FG_RED "Y" VIZZY_FG_RED_BRIGHT "O" VIZZY_FG_YELLOW "U" VIZZY_RESET " " VIZZY_FG_GREEN \
						 "A" VIZZY_FG_BLUE "R" VIZZY_FG_MAGENTA "E" VIZZY_RESET " " VIZZY_FG_MAGENTA_BRIGHT \
						 "H" VIZZY_FG_RED "E" VIZZY_FG_RED_BRIGHT "R" VIZZY_FG_YELLOW "E" VIZZY_RESET); \
	} while (0)

	// Fatal errors
	class Fatal: public std::runtime_error {
		using runtime_error::runtime_error;
	};

	template <typename T, typename... Ts>
	[[noreturn]] inline void die(T&& arg, Ts&&... args) {
		std::stringstream ss;

		if constexpr (std::is_same_v<T, LogInfo>) {
			vizzy::log(ss, LogKind::Error, arg, std::forward<Ts>(args)...);
		}

		else {
			vizzy::log(ss, LogKind::Error, std::nullopt, std::forward<T>(arg), std::forward<Ts>(args)...);
		}

		throw Fatal { ss.str() };
	}

#define VIZZY_DIE(...) \
	do { \
		[VIZZY_VAR(func) = VIZZY_LOCATION_FUNC]<typename... Ts>(Ts&&... VIZZY_VAR(args)) { \
			vizzy::die(vizzy::LogInfo { VIZZY_LOCATION_FILE, VIZZY_LOCATION_LINE, VIZZY_VAR(func) }, \
				std::forward<Ts>(VIZZY_VAR(args))...); \
		}(__VA_ARGS__); \
	} while (0)
}  // namespace vizzy

// Utilities
namespace vizzy {
	template <typename T, typename... Ts>
	[[nodiscard]] constexpr bool any(T&& first, Ts&&... rest) {
		return ((std::forward<T>(first) or std::forward<Ts>(rest)) or ...);
	}

	template <typename T, typename... Ts>
	[[nodiscard]] constexpr bool all(T&& first, Ts&&... rest) {
		return ((std::forward<T>(first) and std::forward<Ts>(rest)) and ...);
	}

	template <typename T, typename... Ts>
	[[nodiscard]] constexpr bool none(T&& first, Ts&&... rest) {
		return ((not std::forward<T>(first) and not std::forward<Ts>(rest)) and ...);
	}

	template <typename T, typename... Ts>
	[[nodiscard]] constexpr bool eq_all(T&& first, Ts&&... rest) {
		return ((std::forward<T>(first) == std::forward<Ts>(rest)) and ...);
	}

	template <typename T, typename... Ts>
	[[nodiscard]] constexpr bool eq_any(T&& first, Ts&&... rest) {
		return ((std::forward<T>(first) == std::forward<Ts>(rest)) or ...);
	}

	template <typename T, typename... Ts>
	[[nodiscard]] constexpr bool eq_none(T&& first, Ts&&... rest) {
		return ((std::forward<T>(first) != std::forward<Ts>(rest)) and ...);
	}

	inline std::string_view trim(std::string_view s) {
		auto it = s.begin();
		while (it != s.end() and isspace(*it)) {
			++it;
		}

		auto rit = s.rbegin();
		while (rit != s.rend() and isspace(*rit)) {
			++rit;
		}

		return { it, rit.base() };
	}
}  // namespace vizzy

// IO
namespace vizzy {

	inline std::string read_file(std::filesystem::path path) {
		try {
			std::filesystem::path cur = path;

			while (std::filesystem::is_symlink(cur)) {
				std::filesystem::path tmp = std::filesystem::read_symlink(cur);

				if (tmp == cur) {
					die("symlink '{}' resolves to itself", path.string());
				}

				cur = tmp;
			}

			if (std::filesystem::is_directory(cur) or std::filesystem::is_other(cur)) {
				die("'{}' is not a file", path.string());
			}

			if (not std::filesystem::exists(cur)) {
				die("file '{}' not found", path.string());
			}

			std::ifstream is(cur);

			if (not is.is_open()) {
				die("cannot read '{}'", path.string());
			}

			std::stringstream ss;
			ss << is.rdbuf();

			return ss.str();
		}

		catch (const std::filesystem::filesystem_error&) {
			die("cannot read '{}'", path.string());
		}

		die("unknown error when trying to read '{}'", path.string());
	}

}  // namespace vizzy

#endif
