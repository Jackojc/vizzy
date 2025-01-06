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

enum : uint64_t {
	OPT_HELP = 1 << 0,
};

int main(int argc, const char* argv[]) {
	try {
		uint64_t flags;
		std::string_view filename;

		auto parser = conflict::parser {
			conflict::option { { 'h', "help", "show help" }, flags, OPT_HELP },
			conflict::string_option { { 'f', "file", "input file" }, "filename", filename },
		};

		parser.apply_defaults();

		auto status = parser.parse(argc - 1, argv + 1);

		switch (status.err) {
			case conflict::error::invalid_option: {
				vizzy::die("invalid option '{}'", status.what1);
			}

			case conflict::error::invalid_argument: {
				vizzy::die("invalid argument '{}' for '{}'", status.what1, status.what2);
			}

			case conflict::error::missing_argument: {
				vizzy::die("missing argument '{}'", status.what1);
			}

			case conflict::error::ok: break;
		}

		if (flags & OPT_HELP) {
			parser.print_help();
			return EXIT_SUCCESS;
		}

		if (filename.empty()) {
			vizzy::die("no file specified");
		}

		// Everything is okay
		sol::state lua;
		lua.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::string, sol::lib::io);

		std::string script = vizzy::read_file(filename);
		lua.script(script);

		// Setup SDL
		if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
			vizzy::die("SDL_Init failed!");
		}

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, VIZZY_OPENGL_VERSION_MAJOR);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, VIZZY_OPENGL_VERSION_MINOR);

		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

		SDL_Window* window = SDL_CreateWindow(VIZZY_EXE,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			VIZZY_WINDOW_WIDTH,
			VIZZY_WINDOW_HEIGHT,
			SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

		if (!window) {
			vizzy::die("SDL_CreateWindow failed! SDL: {}", SDL_GetError());
		}

		// Setup OpenGL
		SDL_GLContext ogl = SDL_GL_CreateContext(window);

		if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
			vizzy::die("gladLoadGLLoader failed!");
		}

		// Event loop
		SDL_Event ev;
		while (true) {
			if (SDL_PollEvent(&ev)) {
				if (ev.type == SDL_QUIT) {
					break;
				}

				else if (ev.type == SDL_WINDOWEVENT_RESIZED) {
					int w, h;
					SDL_GL_GetDrawableSize(window, &w, &h);
					glViewport(0, 0, w, h);
					break;
				}

				else if (ev.type == SDL_KEYUP && ev.key.keysym.sym == SDLK_ESCAPE) {
					break;
				}
			}

			glClearColor(1.f, 0.f, 1.f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT);

			SDL_GL_SwapWindow(window);
		}

		// Cleanup
		SDL_DestroyWindow(window);
		SDL_GL_DeleteContext(ogl);

		SDL_Quit();
	}

	catch (vizzy::Fatal e) {
		fmt::print(std::cerr, fmt::runtime(e.what()));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
