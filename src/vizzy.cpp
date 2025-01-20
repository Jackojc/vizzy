#include <iostream>
#include <chrono>
#include <string_view>
#include <vector>
#include <mutex>

#include <conflict/conflict.hpp>

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/color.h>
#include <fmt/ostream.h>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <libremidi/libremidi.hpp>
#include <libremidi/reader.hpp>

#include <vizzy/vizzy.hpp>

// Commandline flags
enum : uint64_t {
	OPT_HELP = 1 << 0,
};

int main(int argc, const char* argv[]) {
	using namespace std::literals;

	try {
		// Parse arguments
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

		// Lua
		// sol::state lua;
		// lua.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::string, sol::lib::io);

		// std::string script = vizzy::read_file(filename);
		// lua.script(script);

		// Envelopes
		using namespace std::chrono_literals;

		std::mutex envelope_mutex;

		std::vector envelopes = {
			vizzy::Envelope {
				.name = "keyboard",
				.pattern =
					[](libremidi::message msg) {
						return msg.get_message_type() == libremidi::message_type::NOTE_ON and msg.get_channel() == 1;
					},
				.segments = vizzy::attack_release(50ms, 200ms),
			},
		};

		VIZZY_DEBUG(envelopes);

		// MIDI
		libremidi::observer obs;

		auto midi_callback = [&](const libremidi::message& msg) {
			VIZZY_DEBUG("channel = {}, message = {}", msg.get_channel(), msg);

			std::unique_lock lock { envelope_mutex };

			for (auto& env: envelopes) {
				env = vizzy::env_trigger(std::move(env), msg);
			}
		};

		libremidi::input_configuration midi_config { .on_message = midi_callback };
		libremidi::midi_in midi { midi_config };

		if (auto port = libremidi::midi1::in_default_port(); port.has_value()) {
			midi.open_port(port.value());
		}

		else {
			vizzy::die("no ports available");
		}

		// Setup window
		if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
			vizzy::die("SDL_Init failed!");
		}

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, VIZZY_OPENGL_VERSION_MAJOR);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, VIZZY_OPENGL_VERSION_MINOR);

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 1);

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

		if (int v = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress); v != 0) {
			VIZZY_OKAY("OpenGL {}.{}", GLAD_VERSION_MAJOR(v), GLAD_VERSION_MINOR(v));
		}

		else {
			vizzy::die("gladLoadGLLoader failed!");
		}

		// Callbacks
		vizzy::gl::setup_debug_callbacks();

		// Setup shaders
		auto vert = vizzy::gl::create_shader(GL_VERTEX_SHADER, { R"(
			#version 460 core

			uniform float aspect;
			uniform int frame;
			out vec3 position;

			layout (location = 0) in vec3 coord;

			void main() {
				gl_Position = vec4(coord.x, coord.y, coord.z, 1.0);
				position = vec3(coord.x / aspect, coord.y, coord.z);
			}
		)" });

		auto frag = vizzy::gl::create_shader(GL_FRAGMENT_SHADER, { R"(
			#version 460 core

			uniform float aspect;
			uniform float t;
			uniform int frame;

			uniform float keyboard;

			out vec4 colour;
			in vec3 position;

			float circle(vec2 p, float r, float blur) {
				float d = length(p);
				float c = smoothstep(r, r - blur, d);

				return c;
			}

			void main() {
				float cx = position.x + (sin(t * 2) / 2);
				float cy = position.y + (cos(t * 2) / 2);
			
				float c = circle(vec2(cx, cy), 0.3 + (keyboard * 0.5), .01 + (keyboard * 0.3));

				vec3 cc = vec3(position.xyz + .5 + vec3(cos(cx), sin(cy), 0.0)) * c;
			
			    colour = vec4(cc.xyz, 1.0);
			}
		)" });

		auto program = vizzy::gl::create_program({ vert, frag });

		// auto pipeline = create_pipeline({
		// 	vert,
		// 	frag,
		// });

		std::array verts = {
			glm::vec3 { -1.f, 1.f, 0.f },
			glm::vec3 { -1.f, -1.f, 0.f },
			glm::vec3 { 1.f, 1.f, 0.f },

			glm::vec3 { 1.f, 1.f, 0.f },
			glm::vec3 { -1.f, -1.f, 0.f },
			glm::vec3 { 1.f, -1.f, 0.f },
		};

		GLuint vao;
		GLuint vbo;

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		glEnableVertexAttribArray(0);

		glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(decltype(verts)::value_type), verts.data(), GL_STATIC_DRAW);
		glBindVertexArray(0);

		// Event loop
		VIZZY_OKAY("loop");

		SDL_Event ev;
		bool running = true;

		auto loop_start = vizzy::clock::now();

		size_t frame_count = 0;

		while (running) {
			while (SDL_PollEvent(&ev)) {
				switch (ev.type) {
					case SDL_QUIT: {
						running = false;
					} break;

					case SDL_WINDOWEVENT: {
						switch (ev.window.event) {
							case SDL_WINDOWEVENT_RESIZED:
							case SDL_WINDOWEVENT_SIZE_CHANGED: {
								int w, h;
								SDL_GL_GetDrawableSize(window, &w, &h);

								glViewport(0, 0, w, h);

								VIZZY_DEBUG("resize event: width = {}, height = {}", w, h);
							} break;

							default: break;
						}
					} break;

					case SDL_KEYUP: {
						switch (ev.key.keysym.sym) {
							case SDLK_ESCAPE: {
								running = false;
							} break;

							default: break;
						}
					} break;

					default: break;
				}
			}

			glClearColor(.0f, .0f, .0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glUseProgram(program);

			auto current_time = vizzy::clock::now();

			{
				std::unique_lock lock { envelope_mutex };

				for (auto& env: envelopes) {
					env = vizzy::env_update(std::move(env), current_time);
				}

				for (auto& env: envelopes) {
					env = vizzy::env_bind(std::move(env), { program });
				}
			}

			int w, h;
			SDL_GL_GetDrawableSize(window, &w, &h);

			float aspect = static_cast<float>(h) / static_cast<float>(w);
			glUniform1f(glGetUniformLocation(program, "aspect"), aspect);

			std::chrono::duration<float> seconds = current_time - loop_start;
			glUniform1f(glGetUniformLocation(program, "t"), seconds.count());

			glUniform1f(glGetUniformLocation(program, "frame"), frame_count);
			frame_count++;

			// Draw quad
			glBindVertexArray(vao);
			glDrawArrays(GL_TRIANGLES, 0, verts.size());
			glBindVertexArray(0);

			// Swap
			SDL_GL_SwapWindow(window);
		}

		// Cleanup
		// glDeleteFramebuffers(1, &fbo);

		// glDeleteProgramPipelines(1, &pipeline);

		// glDeleteProgram(frag);
		// glDeleteProgram(vert);

		glDeleteProgram(program);

		SDL_DestroyWindow(window);
		SDL_GL_DeleteContext(gl);

		SDL_Quit();
	}

	catch (vizzy::Fatal e) {
		std::cerr << e.what();
		return EXIT_FAILURE;
	}

	VIZZY_OKAY("done");
	return EXIT_SUCCESS;
}
