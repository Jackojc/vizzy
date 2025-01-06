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

GLuint compile_shader(GLenum kind, std::vector<std::string_view> sources) {
	GLuint shader = glCreateShader(kind);

	// Setup vector of string pointers so we can interface with the C function.
	std::vector<const GLchar*> sources_cstr;
	std::transform(
		sources.begin(), sources.end(), std::back_inserter(sources_cstr), std::mem_fn(&std::string_view::data));

	// Since we have string_views to work with, we can't guarantee they're null-terminated so we pass an explicit length
	// for each string_view.
	std::vector<GLint> sources_lengths;
	std::transform(
		sources.begin(), sources.end(), std::back_inserter(sources_lengths), std::mem_fn(&std::string_view::size));

	// Assemble many sources into single source.
	glShaderSource(shader,
		/* count = */ sources.size(),
		/* string = */ sources_cstr.data(),
		/* length = */ sources_lengths.data());

	glCompileShader(shader);

	// Check if compilation succeeded and get any potential info messages.
	int ok = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);

	int length = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

	std::string info;
	info.resize(length, '\0');

	glGetShaderInfoLog(shader, length, nullptr, info.data());

	if (ok != GL_TRUE) {
		vizzy::die("shader compilation failed! GL: {}", info);
	}

	// // Print final concatenated shader source.
	// int src_length = 0;
	// glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &src_length);

	// std::string src;
	// src.resize(src_length, '\0');

	// glGetShaderSource(shader, src_length, nullptr, src.data());

	return shader;
}

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
		// lua.script(script);

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

		if (window == nullptr) {
			vizzy::die("SDL_CreateWindow failed! SDL: {}", SDL_GetError());
		}

		// Setup OpenGL
		SDL_GLContext ogl = SDL_GL_CreateContext(window);

		if (gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress) == 0) {
			vizzy::die("gladLoadGLLoader failed!");
		}

		compile_shader(GL_FRAGMENT_SHADER,
			{
				"#version 330 core\n",
				"in vec3 colour_out;\n",
				"out vec4 colour;\n",
			});

		VIZZY_DIE("foo");

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
		std::cerr << e.what();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
