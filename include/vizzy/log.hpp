#ifndef VIZZY_LOG_HPP
#define VIZZY_LOG_HPP

#include <array>
#include <string_view>
#include <optional>

#include <filesystem>
#include <iostream>

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/color.h>
#include <fmt/ostream.h>

#include <vizzy/macro.hpp>

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

}  // namespace vizzy

#endif
