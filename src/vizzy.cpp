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

GLuint create_shader(GLenum kind, std::vector<std::string_view> sources) {
	GLuint shader = glCreateShader(kind);

	if (shader == 0) {
		vizzy::die("glCreateShader failed!");
	}

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
	VIZZY_DEBUG("info = {}", info.empty() ? "<empty>" : info);

	if (ok != GL_TRUE) {
		glDeleteShader(shader);
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

GLuint create_shader(GLenum kind, std::string_view source) {
	return create_shader(kind, std::vector { source });
}

GLuint create_program(std::vector<GLuint> shaders) {
	GLuint program = glCreateProgram();

	if (program == 0) {
		vizzy::die("glCreateProgram failed!");
	}

	for (GLuint shader: shaders) {
		glAttachShader(program, shader);
		glDeleteShader(shader);  // We attached it so it won't be freed until the program is.
	}

	glLinkProgram(program);

	// Stats
	int shader_count = 0;
	int atomic_counter_buffers = 0;
	int active_attributes = 0;
	int active_uniforms = 0;
	int binary_length = 0;

	glGetProgramiv(program, GL_ATTACHED_SHADERS, &shader_count);
	glGetProgramiv(program, GL_ACTIVE_ATOMIC_COUNTER_BUFFERS, &atomic_counter_buffers);
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &active_attributes);
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &active_uniforms);
	glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &binary_length);

	VIZZY_DEBUG("shader count = {}", shader_count);
	VIZZY_DEBUG("atomic counter buffers = {}", atomic_counter_buffers);
	VIZZY_DEBUG("active attributes = {}", active_attributes);
	VIZZY_DEBUG("active uniforms = {}", active_uniforms);
	VIZZY_DEBUG("binary length = {}", binary_length);

	// Status
	int ok = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &ok);

	int length = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

	std::string info;
	info.resize(length, '\0');

	glGetProgramInfoLog(program, length, nullptr, info.data());
	VIZZY_DEBUG("info = {}", info.empty() ? "<empty>" : info);

	if (ok != GL_TRUE) {
		glDeleteProgram(program);
		vizzy::die("shader linking failed! GL: {}", info);
	}

	// Validation
	glValidateProgram(program);

	int valid = GL_FALSE;
	glGetProgramiv(program, GL_VALIDATE_STATUS, &valid);

	int validation_length = 0;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &validation_length);

	std::string validation;
	validation.resize(length, '\0');

	glGetProgramInfoLog(program, validation_length, nullptr, validation.data());
	VIZZY_DEBUG("validation = {}", validation.empty() ? "<empty>" : validation);

	if (ok != GL_TRUE) {
		glDeleteProgram(program);
		vizzy::die("validation failed! GL: {}", validation);
	}

	return program;
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

			case conflict::error::missing_argument: {
				vizzy::die("missing argument '{}'", status.what1);
			}

			case conflict::error::invalid_argument: {
				vizzy::die("invalid argument '{}' for '{}'", status.what1, status.what2);
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

		auto fs = create_shader(GL_FRAGMENT_SHADER,
			"#version 330 core\n"
			"in vec3 colour_out;\n"
			"out vec4 colour;\n"
			"void main() {\n"
			"    colour = vec4(colour_out, 1.0);\n"
			"}\n");

		auto vs = create_shader(GL_VERTEX_SHADER,
			"#version 330 core\n"
			"in vec2 position;\n"
			"in vec3 colour_in;\n"
			"out vec3 colour_out;\n"
			"void main() {\n"
			"gl_Position = vec4(position, 0.0, 1.0);\n"
			"colour_out = colour_in;\n"
			"}\n");

		auto program = create_program(std::vector { vs, fs });

		glUseProgram(program);

		// Triangle
		struct Vertex {
			vec2s pos;
			vec3s col;
		};

		std::array vertices = {
			Vertex { { { 0.0f, 0.5f } },     // Vertex 1 (X, Y)
				{ { 1.0f, 0.0f, 0.0f } } },  // Vertex 1 (R, G, B)

			Vertex { { { 0.5f, -0.5f } },    // Vertex 2 (X, Y)
				{ { 0.0f, 1.0f, 0.0f } } },  // Vertex 2 (R, G, B)

			Vertex { { { -0.5f, -0.5f } },   // Vertex 3 (X, Y)
				{ { 0.0f, 0.0f, 1.0f } } },  // Vertex 3 (R, G, B)
		};

		GLuint vao;
		GLuint vbo;

		glGenBuffers(1, &vbo);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size(), vertices.data(), GL_STATIC_DRAW);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		GLint pa = glGetAttribLocation(program, "position");
		glVertexAttribPointer(pa, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

		glEnableVertexAttribArray(pa);

		GLint ca = glGetAttribLocation(program, "colour_in");
		glVertexAttribPointer(ca, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, col));

		glEnableVertexAttribArray(ca);

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

			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			glDrawArrays(GL_TRIANGLES, 0, 3);

			SDL_GL_SwapWindow(window);
		}

		// Cleanup
		glDeleteProgram(program);

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
