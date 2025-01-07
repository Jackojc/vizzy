#include <type_traits>
#include <iostream>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <array>

#include <libremidi/libremidi.hpp>
#include <libremidi/reader.hpp>

#include <conflict/conflict.hpp>

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/color.h>
#include <fmt/ostream.h>

#include <glad/gl.h>
#include <cglm/struct.h>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <SDL2/SDL.h>

#include <vizzy/vizzy.hpp>

[[nodiscard]] inline std::string_view ogl_error_to_str(GLenum e) {
	switch (e) {
		case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
		case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
		case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
		case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
	}

	return {};
}

template <typename F, typename... Ts>
[[nodiscard]] inline decltype(auto) gl_call(F&& fn, Ts&&... args) {
	decltype(auto) v = fn(std::forward<Ts>(args)...);

	if (GLenum status = glGetError(); status != GL_NO_ERROR) {
		vizzy::die("glGetError(): {}", ogl_error_to_str(status));
	}

	return v;
}

template <typename F, typename... Ts>
	requires std::same_as<std::invoke_result_t<F, Ts...>, void>
inline void gl_call(F&& fn, Ts&&... args) {
	fn(std::forward<Ts>(args)...);

	if (GLenum status = glGetError(); status != GL_NO_ERROR) {
		vizzy::die("glGetError(): {}", ogl_error_to_str(status));
	}
}

[[nodiscard]] inline GLint gl_get_program(GLuint program, GLenum param) {
	GLint v = 0;

	gl_call(glGetProgramiv,
		program,
		param,
		&v);  // INFO: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetProgram.xhtml

	return v;
}

[[nodiscard]] inline GLint gl_get_shader(GLuint shader, GLenum param) {
	GLint v = 0;

	gl_call(glGetShaderiv,
		shader,
		param,
		&v);  // INFO: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetShader.xhtml

	return v;
}

[[nodiscard]] inline GLuint create_shader_program(GLenum kind, vizzy::Same<std::string_view> auto&&... sv) {
	// INFO: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glCreateShaderProgram.xhtml

	VIZZY_TRACE(kind);

	std::vector<const GLchar*> sources = { (sv.data(), ...) };
	GLuint program = gl_call(glCreateShaderProgramv, kind, sources.size(), sources.data());

	int ok = gl_get_program(program, GL_LINK_STATUS);
	int info_length = gl_get_program(program, GL_INFO_LOG_LENGTH);
	int binary_length = gl_get_program(program, GL_PROGRAM_BINARY_LENGTH);

	std::string info;
	info.resize(info_length, '\0');

	glGetProgramInfoLog(program, info_length, nullptr, info.data());

	VIZZY_DEBUG("ok = {}", ok ? "true" : "false");
	VIZZY_DEBUG("info = '{}'", info.empty() ? "<empty>" : vizzy::trim(info));
	VIZZY_DEBUG("binary length = {}b", binary_length);

	if (ok != GL_TRUE) {
		glDeleteProgram(program);
		vizzy::die("program compilation failed! GL: {}", vizzy::trim(info));
	}

	// Validation
	glValidateProgram(program);

	int valid = gl_get_program(program, GL_VALIDATE_STATUS);
	int validation_length = gl_get_program(program, GL_INFO_LOG_LENGTH);

	std::string validation;
	validation.resize(validation_length, '\0');

	glGetProgramInfoLog(program, validation_length, nullptr, validation.data());

	VIZZY_DEBUG("valid = {}", valid ? "true" : "false");
	VIZZY_DEBUG("validation = '{}'", validation.empty() ? "<empty>" : vizzy::trim(validation));

	if (valid != GL_TRUE) {
		glDeleteProgram(program);
		vizzy::die("program validation failed! GL: {}", vizzy::trim(validation));
	}

	return program;
}

// TODO: Rework this function to take an array of pairs mapping GLbitfield to programs.
// It turns out it's fine to have multiple shaders present in a program when using seperable
// programs.
// Because of this, it might be better to go back to the old implementation for create_shader_program
// where we explicitly do compilation and linking since it would allow us to compile mixed programs with
// more than one stage.
[[nodiscard]] inline GLuint create_program_pipeline(GLenum kind, vizzy::Same<GLuint> auto&&... programs) {
	// INFO: https://www.khronos.org/opengl/wiki/Shader_Compilation#Separate_programs

	GLuint pipeline;
	glGenProgramPipelines(1, &pipeline);

	constexpr auto fn = [&](GLuint program) {
		// 1. Get shader from program
		// 2. Get shader _type_
		// 3. glUseProgramStages with shader type

		int shaders_length = gl_get_program(program, GL_ATTACHED_SHADERS);

		// TODO: Is it possible to have a seperable program with multiple shaders
		// in it? If not, we need to check how many are attached and error if it's more than one.

		// std::vector<GLuint> shaders;
		// shaders.resize(shaders_length, 0);

		// glGetAttachedShaders(program, shaders_length, nullptr, shaders.data());

		std::vector<GLuint> shaders;
		shaders.resize(shaders_length, 0);

		glGetAttachedShaders(program, shaders_length, nullptr, shaders.data());
	};

	(fn(programs), ...);

	return pipeline;
}

enum : uint64_t {
	OPT_HELP = 1 << 0,
};

int main(int argc, const char* argv[]) {
	using namespace std::literals;

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
		SDL_GLContext gl = SDL_GL_CreateContext(window);

		int gl_version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);

		if (gl_version == 0) {
			vizzy::die("gladLoadGLLoader failed!");
		}

		std::cout << "Loaded OpenGL " << GLAD_VERSION_MAJOR(gl_version) << "." << GLAD_VERSION_MINOR(gl_version)
				  << std::endl;

		auto frag = create_shader_program(GL_FRAGMENT_SHADER, R"(
			#version 330 core

			in vec3 colour_out;
			out vec4 colour;

			void main() {
			    colour = vec4(colour_out, 1.0);
			}
		)"sv);

		auto vert = create_shader_program(GL_VERTEX_SHADER, R"(
			#version 330 core

			in vec2 position;
			in vec3 colour_in;

			out vec3 colour_out;

			void main() {
				gl_Position = vec4(position, 0.0, 1.0);
				colour_out = colour_in;
			}
		)"sv);

		GLuint pipeline;
		glGenProgramPipelines(1, &pipeline);

		glUseProgramStages(pipeline, GL_VERTEX_SHADER_BIT, vert);
		glUseProgramStages(pipeline, GL_FRAGMENT_SHADER_BIT, frag);

		// GLint colour_location = glGetUniformLocation(frag, "colour");
		// glProgramUniform4f(frag, colour_location, 1.f, 0.f, 0.f, 1.f);

		glBindProgramPipeline(pipeline);

		// VIZZY_DIE("fail");

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

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glBufferData(
			GL_ARRAY_BUFFER, vertices.size() * sizeof(decltype(vertices)::value_type), vertices.data(), GL_STATIC_DRAW);

		GLint pa = glGetAttribLocation(vert, "position");
		glVertexAttribPointer(pa, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

		GLint ca = glGetAttribLocation(vert, "colour_in");
		glVertexAttribPointer(ca, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, col));

		glEnableVertexAttribArray(pa);
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
		glDeleteProgramPipelines(1, &pipeline);

		glDeleteProgram(frag);
		glDeleteProgram(vert);

		SDL_DestroyWindow(window);
		SDL_GL_DeleteContext(gl);

		SDL_Quit();
	}

	catch (vizzy::Fatal e) {
		std::cerr << e.what();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
