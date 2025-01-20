#ifndef VIZZY_UTIL_HPP
#define VIZZY_UTIL_HPP

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <sstream>
#include <optional>

#include <vizzy/macro.hpp>
#include <vizzy/log.hpp>

// Concepts
namespace vizzy {
	template <typename T, typename U>
	concept Same = std::is_same_v<T, U>;
}

// Fatal errors
namespace vizzy {
	class Fatal: public std::runtime_error {  // Fatal error type that is caught in main.
		using runtime_error::runtime_error;
	};

	template <typename T, typename... Ts>
	[[noreturn]] inline void die(T&& arg, Ts&&... args) {
		std::stringstream ss;

		// Contains location information
		if constexpr (std::is_same_v<T, LogInfo>) {
			vizzy::log(ss, LogKind::Error, arg, std::forward<Ts>(args)...);
		}

		// Does not.
		else {
			vizzy::log(ss, LogKind::Error, std::nullopt, std::forward<T>(arg), std::forward<Ts>(args)...);
		}

		throw Fatal { ss.str() };
	}

	// DIE macro includes location information whereas direct call does not.
#define VIZZY_DIE(...) \
	do { \
		[VIZZY_VAR(func) = VIZZY_LOCATION_FUNC]<typename... Ts>(Ts&&... VIZZY_VAR(args)) { \
			vizzy::die(vizzy::LogInfo { VIZZY_LOCATION_FILE, VIZZY_LOCATION_LINE, VIZZY_VAR(func) }, \
				std::forward<Ts>(VIZZY_VAR(args))...); \
		}(__VA_ARGS__); \
	} while (0)

#define VIZZY_UNREACHABLE() \
	do { \
		vizzy::die(vizzy::LogInfo { VIZZY_LOCATION_FILE, VIZZY_LOCATION_LINE, VIZZY_LOCATION_FUNC }, "unreachable!"); \
	} while (0)

}  // namespace vizzy

// Utilities
namespace vizzy {
	// Comparisons
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

	// Trim surrounding whitespace
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
