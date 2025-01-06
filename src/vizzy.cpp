#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <array>
#include <chrono>
#include <thread>
#include <filesystem>
#include <memory>
#include <algorithm>

#include <libremidi/libremidi.hpp>
#include <libremidi/reader.hpp>

#include <conflict/conflict.hpp>

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/color.h>
#include <fmt/ostream.h>

#include <glad/glad.h>
#include <cglm/struct.h>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <SDL2/SDL.h>

#include <vizzy/vizzy.hpp>

int main(int, const char*[]) {
	try {
		// VIZZY_LOG(vizzy::LogKind::Debug, "hello");
		// VIZZY_LOG(vizzy::LogKind::Trace, "hello");
		// VIZZY_LOG(vizzy::LogKind::Warn, "hello");
		// VIZZY_LOG(vizzy::LogKind::Error, "hello");
		// VIZZY_LOG(vizzy::LogKind::Okay, "hello");

		// VIZZY_DEBUG("hello");
		// VIZZY_TRACE("hello");
		// VIZZY_WARN("hello");
		// VIZZY_ERROR("hello");
		// VIZZY_OKAY("hello");

		// int x = VIZZY_INSPECT(5 + 5);
		// VIZZY_OKAY(x);

		// VIZZY_LOG(vizzy::LogKind::Debug);
		// VIZZY_LOG(vizzy::LogKind::Debug, 123);
		// VIZZY_LOG(vizzy::LogKind::Debug, "fiodfgh");

		// VIZZY_WHEREAMI();

		sol::state lua;
		lua.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::string, sol::lib::io);

		lua.script("print('hello from the other side')");

		// vizzy::die("fuck");
	}

	catch (vizzy::Fatal e) {
		fmt::print(std::cerr, fmt::runtime(e.what()));
	}

	return 0;
}
