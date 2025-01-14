#include <type_traits>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <deque>
#include <mutex>
#include <thread>
#include <limits>

#include <conflict/conflict.hpp>

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/color.h>
#include <fmt/ostream.h>

#include <SDL2/SDL.h>
#include <glad/gl.h>
#include <cglm/struct.h>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include <libremidi/libremidi.hpp>
#include <libremidi/reader.hpp>

#include <vizzy/vizzy.hpp>

[[nodiscard]] inline std::string_view ogl_error_to_str(GLenum error) {
	switch (error) {
		case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
		case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
		case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
		case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
	}

	vizzy::die("unknown error type");  // TODO: Implement unreachable macro
}

[[nodiscard]] inline GLbitfield ogl_shader_type_to_bitfield(GLint kind) {
	switch (kind) {
		case GL_COMPUTE_SHADER: return GL_COMPUTE_SHADER_BIT;
		case GL_VERTEX_SHADER: return GL_VERTEX_SHADER_BIT;
		case GL_TESS_CONTROL_SHADER: return GL_TESS_CONTROL_SHADER_BIT;
		case GL_TESS_EVALUATION_SHADER: return GL_TESS_EVALUATION_SHADER_BIT;
		case GL_GEOMETRY_SHADER: return GL_GEOMETRY_SHADER_BIT;
		case GL_FRAGMENT_SHADER: return GL_FRAGMENT_SHADER_BIT;
	}

	vizzy::die("unknown shader type");  // TODO: Implement unreachable macro
}

[[nodiscard]] inline std::string_view ogl_debug_source_to_str(GLenum source) {
	switch (source) {
		case GL_DEBUG_SOURCE_API: return "api";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "window system";
		case GL_DEBUG_SOURCE_SHADER_COMPILER: return "shader compiler";
		case GL_DEBUG_SOURCE_THIRD_PARTY: return "third party";
		case GL_DEBUG_SOURCE_APPLICATION: return "application";
		case GL_DEBUG_SOURCE_OTHER: return "other";
	}

	vizzy::die("unknown debug source");  // TODO: Implement unreachable macro
}

[[nodiscard]] inline std::string_view ogl_debug_type_to_str(GLenum type) {
	switch (type) {
		case GL_DEBUG_TYPE_ERROR: return "error";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "deprecated behaviour";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "undefined behaviour";
		case GL_DEBUG_TYPE_PORTABILITY: return "portability";
		case GL_DEBUG_TYPE_PERFORMANCE: return "performance";
		case GL_DEBUG_TYPE_MARKER: return "marker";
		case GL_DEBUG_TYPE_PUSH_GROUP: return "push group";
		case GL_DEBUG_TYPE_POP_GROUP: return "pop group";
		case GL_DEBUG_TYPE_OTHER: return "other";
	}

	vizzy::die("unknown debug type");  // TODO: Implement unreachable macro
}

[[nodiscard]] inline std::string_view ogl_debug_severity_to_str(GLenum severity) {
	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH: return "high";
		case GL_DEBUG_SEVERITY_MEDIUM: return "medium";
		case GL_DEBUG_SEVERITY_LOW: return "low";
		case GL_DEBUG_SEVERITY_NOTIFICATION: return "notify";
	}

	vizzy::die("unknown debug severity");  // TODO: Implement unreachable macro
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

[[nodiscard]] inline GLint gl_get_pipeline(GLuint pipeline, GLenum param) {
	GLint v = 0;

	gl_call(glGetProgramPipelineiv,
		pipeline,
		param,
		&v);  // INFO: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetProgramPipeline.xhtml

	return v;
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

[[nodiscard]] inline GLint gl_get_integer(GLenum param) {
	GLint v = 0;

	gl_call(glGetIntegerv, param,
		&v);  // INFO: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGet.xhtml

	return v;
}

[[nodiscard]] inline GLuint create_shader(GLenum kind, std::vector<std::string_view> sv) {
	VIZZY_FUNCTION();

	GLuint shader = gl_call(glCreateShader, kind);

	VIZZY_DEBUG("shader type = {}", kind);

	if (shader == 0) {
		vizzy::die("glCreateShader failed!");
	}

	std::vector<const GLchar*> sources;
	std::transform(sv.begin(), sv.end(), std::back_inserter(sources), std::mem_fn(&decltype(sv)::value_type::data));

	gl_call(glShaderSource, shader, sources.size(), sources.data(), nullptr);
	gl_call(glCompileShader, shader);

	int ok = gl_get_shader(shader, GL_COMPILE_STATUS);
	int info_length = gl_get_shader(shader, GL_INFO_LOG_LENGTH);

	std::string info;
	info.resize(info_length, '\0');

	gl_call(glGetShaderInfoLog, shader, info_length, nullptr, info.data());

	VIZZY_DEBUG("ok = {}", ok ? "true" : "false");
	VIZZY_DEBUG("info = '{}'", info.empty() ? "<empty>" : info);

	if (ok != GL_TRUE) {
		gl_call(glDeleteShader, shader);
		vizzy::die("shader compilation failed! GL: {}", info);
	}

	VIZZY_OKAY("successfully compiled shader ({})", shader);

	return shader;
}

[[nodiscard]] inline GLuint create_program(std::vector<GLuint> shaders) {
	VIZZY_FUNCTION();

	GLuint program = gl_call(glCreateProgram);
	// gl_call(glProgramParameteri, program, GL_PROGRAM_SEPARABLE, GL_TRUE);

	if (program == 0) {
		vizzy::die("glCreateProgram failed!");
	}

	// Linking
	for (auto shader: shaders) {
		VIZZY_DEBUG("shader: {}", shader);
		gl_call(glAttachShader, program, shader);
		gl_call(glDeleteShader, shader);
	}

	gl_call(glLinkProgram, program);

	int ok = gl_get_program(program, GL_LINK_STATUS);
	int info_length = gl_get_program(program, GL_INFO_LOG_LENGTH);
	int shader_count = gl_get_program(program, GL_ATTACHED_SHADERS);
	int binary_length = gl_get_program(program, GL_PROGRAM_BINARY_LENGTH);

	std::string info;
	info.resize(info_length, '\0');

	gl_call(glGetProgramInfoLog, program, info_length, nullptr, info.data());

	VIZZY_DEBUG("ok = {}", ok ? "true" : "false");
	VIZZY_DEBUG("info = '{}'", info.empty() ? "<empty>" : info);
	VIZZY_DEBUG("binary length = {}b", binary_length);
	VIZZY_DEBUG("shader count = {}", shader_count);

	if (ok != GL_TRUE) {
		gl_call(glDeleteProgram, program);
		vizzy::die("shader linking failed! GL: {}", info);
	}

	// Validation
	gl_call(glValidateProgram, program);

	int valid = gl_get_program(program, GL_VALIDATE_STATUS);
	int validation_length = gl_get_program(program, GL_INFO_LOG_LENGTH);

	std::string validation;
	validation.resize(validation_length, '\0');

	gl_call(glGetProgramInfoLog, program, validation_length, nullptr, validation.data());

	VIZZY_DEBUG("valid = {}", valid ? "true" : "false");
	VIZZY_DEBUG("validation = '{}'", validation.empty() ? "<empty>" : validation);

	if (valid != GL_TRUE) {
		gl_call(glDeleteProgram, program);
		vizzy::die("validation failed! GL: {}", validation);
	}

	VIZZY_OKAY("successfully linked program ({})", program);

	return program;
}

[[nodiscard]] inline GLuint create_shader_program(GLenum kind, std::vector<std::string_view> sv) {
	VIZZY_FUNCTION();
	return create_program({ create_shader(kind, sv) });
}

[[nodiscard]] inline GLuint create_pipeline(std::vector<std::pair<GLbitfield, GLuint>> programs) {
	// INFO: https://www.khronos.org/opengl/wiki/Shader_Compilation#Separate_programs

	VIZZY_FUNCTION();
	VIZZY_DEBUG("program count = {}", programs.size());

	GLuint pipeline;
	glGenProgramPipelines(1, &pipeline);

	// Attach programs to stages.
	for (auto [stage, program]: programs) {
		VIZZY_DEBUG("stage = {:#b}, program = {}", stage, program);
		gl_call(glUseProgramStages, pipeline, stage, program);
	}

	// Validation (INFO: https://docs.gl/es3/glValidateProgramPipeline)
	glValidateProgramPipeline(pipeline);

	int valid = gl_get_pipeline(pipeline, GL_VALIDATE_STATUS);
	int validation_length = gl_get_pipeline(pipeline, GL_INFO_LOG_LENGTH);

	std::string validation;
	validation.resize(validation_length, '\0');

	gl_call(glGetProgramPipelineInfoLog, pipeline, validation_length, nullptr, validation.data());

	VIZZY_DEBUG("valid = {}", valid ? "true" : "false");
	VIZZY_DEBUG("validation = '{}'", validation.empty() ? "<empty>" : validation);

	if (valid != GL_TRUE) {
		gl_call(glDeleteProgramPipelines, 1, &pipeline);
		vizzy::die("validation failed! GL: {}", validation);
	}

	VIZZY_OKAY("successfully generated pipeline ({})", pipeline);
	return pipeline;
}

[[nodiscard]] inline GLuint create_pipeline(std::vector<GLuint> programs) {
	// INFO: https://www.khronos.org/opengl/wiki/Shader_Compilation#Separate_programs

	VIZZY_FUNCTION();
	VIZZY_DEBUG("program count = {}", programs.size());

	std::vector<std::pair<GLbitfield, GLuint>> program_mapping;

	// Loop through programs and figure out what kind of shaders are attached to them.
	// We generate a bitfield of all of the stages inside the program and then store this
	// information in a mapping before passing it to the overloaded create_pipeline function which
	// expects explicit mappings between stages and programs.
	for (auto program: programs) {
		VIZZY_DEBUG("program = {}", program);

		int shaders_length = gl_get_program(program, GL_ATTACHED_SHADERS);

		std::vector<GLuint> shaders;
		shaders.resize(shaders_length, 0);

		gl_call(glGetAttachedShaders, program, shaders_length, nullptr, shaders.data());

		GLbitfield stages = 0;

		for (auto shader: shaders) {
			VIZZY_DEBUG("shader = {}", shader);

			int kind = gl_get_shader(shader, GL_SHADER_TYPE);
			stages |= ogl_shader_type_to_bitfield(kind);

			VIZZY_DEBUG("shader type = {}", kind);
		}

		VIZZY_DEBUG("stages = {}", stages);
		program_mapping.emplace_back(stages, program);
	}

	VIZZY_OKAY("successfully generated pipeline mapping ({})", program_mapping);
	return create_pipeline(program_mapping);
}

using PreCallback = void (*)(const char* name, GLADapiproc apiproc, int len_args, ...);
using PostCallback = void (*)(void* ret, const char* name, GLADapiproc apiproc, int len_args, ...);

inline void pre_callback(
	[[maybe_unused]] const char* name, [[maybe_unused]] GLADapiproc apiproc, [[maybe_unused]] int len_args, ...) {
	// VIZZY_WARN("PreCallback '{}'", name);
}

inline void post_callback([[maybe_unused]] void* ret,
	[[maybe_unused]] const char* name,
	[[maybe_unused]] GLADapiproc apiproc,
	[[maybe_unused]] int len_args,
	...) {
	// VIZZY_WARN("PostCallback '{}'", name);
}

inline void debug_callback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	[[maybe_unused]] GLsizei length,
	const GLchar* message,
	[[maybe_unused]] const void* user_param) {
	auto source_str = ogl_debug_source_to_str(source);
	auto type_str = ogl_debug_type_to_str(type);
	auto severity_str = ogl_debug_severity_to_str(severity);

	vizzy::log(vizzy::LogKind::Debug, "GL ({}) [{}] [{}] [{}] : {}", id, source_str, type_str, severity_str, message);
}

// Envelopes
namespace vizzy {
	using clock = std::chrono::steady_clock;
	using unit = std::chrono::milliseconds;
	using timepoint = std::chrono::time_point<clock>;
}

float interp(float start, float end, float time) {
	return (1.0f - time) * start + time * end;
}

struct Segment {
	vizzy::unit start_time;
	vizzy::unit end_time;

	float start_amp;
	float end_amp;
};

struct Stage {
	vizzy::unit duration;
	float target;
};

struct Envelope {
	std::string_view name;

	std::function<bool(libremidi::message)> pattern;
	std::vector<Segment> segments;

	vizzy::timepoint trigger;

	float starting_amplitude;
	float current_amplitude;
};

// struct EnvelopeState {
// 	std::vector<std::pair<std::string_view, size_t>> index_lookup;
// 	std::vector<Envelope> envelopes;
// };

// inline std::ostream& operator<<(std::ostream& os, Segment s) {
// 	fmt::print(os,
// 		fmt::runtime("{ .start_time = {}, .end_time = {}, .start_amp = {}, .end_amp = {} }"),
// 		s.start_time,
// 		s.end_time,
// 		s.start_amp,
// 		s.end_amp);

// 	return os;
// }

// inline std::ostream& operator<<(std::ostream& os, Stage s) {
// 	fmt::print(os, fmt::runtime("{ .duration = {}, .target = {} }"), s.duration, s.target);
// 	return os;
// }

// inline std::ostream& operator<<(std::ostream& os, Envelope e) {
// 	fmt::print(os,
// 		fmt::runtime("{ .name = {}, .trigger = {}, .segments = {} }"),
// 		e.name,
// 		std::chrono::duration_cast<vizzy::unit>(e.trigger.time_since_epoch()),
// 		e.segments);

// 	return os;
// }

// template <>
// struct fmt::formatter<Segment>: fmt::ostream_formatter {};

// template <>
// struct fmt::formatter<Stage>: fmt::ostream_formatter {};

// template <>
// struct fmt::formatter<Envelope>: fmt::ostream_formatter {};

inline float update_envelope(float continue_amp, vizzy::timepoint trigger, const std::vector<Segment>& segments) {
	auto current_time = vizzy::clock::now();
	auto env_relative_time = current_time - trigger;

	auto it = std::find_if(segments.begin(), segments.end(), [&](const auto& segment) {
		return env_relative_time >= segment.start_time and env_relative_time < segment.end_time;
	});

	auto index = std::distance(segments.begin(), it);

	if (it == segments.end()) {
		return 0.0;
	}

	// Active stage
	auto [start_time, end_time, start_amp, end_amp] = *it;

	std::chrono::duration<float> stage_relative_time = env_relative_time - start_time;
	auto normalised_time = stage_relative_time / (end_time - start_time);

	float amp = index == 0 ? continue_amp : start_amp;

	return interp(amp, end_amp, normalised_time);
}

inline std::vector<Segment> create_segments(const std::vector<Stage>& stages) {
	std::vector<Segment> segments;

	using namespace std::chrono_literals;

	vizzy::unit duration_so_far = 0s;
	float start_amp = 0.0;

	for (auto [duration, target]: stages) {
		auto start_time = duration_so_far;
		auto end_time = start_time + duration;

		segments.emplace_back(start_time, end_time, start_amp, target);

		duration_so_far += duration;
		start_amp = target;
	}

	return segments;
}

inline Envelope create_envelope(
	std::string_view name, std::function<bool(libremidi::message)> pattern, const std::vector<Stage>& stages) {
	Envelope env;

	env.name = name;
	env.pattern = pattern;
	env.segments = create_segments(stages);

	env.trigger = vizzy::timepoint::max();

	env.starting_amplitude = 0.0f;
	env.current_amplitude = 0.0f;

	return env;
}

inline void update_envelopes(std::vector<Envelope>& envelopes) {
	for (auto& [name, pattern, segments, trigger, starting_amplitude, current_amplitude]: envelopes) {
		current_amplitude = update_envelope(starting_amplitude, trigger, segments);
	}
}

inline void trigger_envelopes(std::vector<Envelope>& envelopes, libremidi::message msg) {
	for (auto& [name, pattern, segments, trigger, starting_amplitude, current_amplitude]: envelopes) {
		if (pattern(msg)) {
			starting_amplitude = current_amplitude;
			trigger = vizzy::clock::now();
		}
	}
}

inline void assign_envelopes(std::vector<Envelope>& envelopes, GLuint program) {
	for (auto& [name, pattern, segments, trigger, starting_amplitude, current_amplitude]: envelopes) {
		glUniform1f(glGetUniformLocation(program, name.data()), current_amplitude);
	}
}

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

		std::vector<Envelope> envelopes = {
			create_envelope("kick",
				[](libremidi::message msg) {
					return msg.get_channel() == 9 and msg.get_message_type() == libremidi::message_type::NOTE_ON and
						msg[1] == 36;
				},
				{
					{ 50ms, 1.0 },
					{ 75ms, 0.0 },
				}),
			create_envelope("snare",
				[](libremidi::message msg) {
					return msg.get_channel() == 9 and msg.get_message_type() == libremidi::message_type::NOTE_ON and
						msg[1] == 38;
				},
				{
					{ 50ms, 1.0 },
					{ 100ms, 0.0 },
				}),
			create_envelope("hat",
				[](libremidi::message msg) {
					return msg.get_channel() == 9 and msg.get_message_type() == libremidi::message_type::NOTE_ON and
						msg[1] == 46;
				},
				{
					{ 50ms, 1.0 },
					{ 25ms, 0.0 },
				}),
			create_envelope("tb303",
				[](libremidi::message msg) {
					return msg.get_message_type() == libremidi::message_type::NOTE_ON and msg.get_channel() == 11;
				},
				{
					{ 20ms, 1.0 },
					{ 200ms, 0.0 },
				}),
		};

		// MIDI
		libremidi::observer obs;

		VIZZY_TRACE("MIDI inputs:");
		for (const libremidi::input_port& port: obs.get_input_ports()) {
			VIZZY_OKAY("input: {}", port.port_name);
		}

		VIZZY_TRACE("MIDI outputs:");
		for (const libremidi::output_port& port: obs.get_output_ports()) {
			VIZZY_OKAY("output: {}", port.port_name);
		}

		auto midi_ev_callback = [&](const libremidi::message& message) {
			// VIZZY_DEBUG(message.get_channel());
			std::unique_lock lock { envelope_mutex };
			trigger_envelopes(envelopes, message);
		};

		// Create the midi object
		libremidi::midi_in midi { libremidi::input_configuration { .on_message = midi_ev_callback } };

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

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, VIZZY_OPENGL_VERSION_MAJOR);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, VIZZY_OPENGL_VERSION_MINOR);

		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

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

		int gl_version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
		if (gl_version == 0) {
			vizzy::die("gladLoadGLLoader failed!");
		}

		// Version information
		SDL_version sdl_compiled;
		SDL_version sdl_linked;

		SDL_VERSION(&sdl_compiled);
		SDL_GetVersion(&sdl_linked);

		VIZZY_DEBUG("SDL version (compiled): {}.{}.{}", sdl_compiled.major, sdl_compiled.minor, sdl_compiled.patch);
		VIZZY_DEBUG("SDL version (linked): {}.{}.{}", sdl_linked.major, sdl_linked.minor, sdl_linked.patch);

		VIZZY_DEBUG("OpenGL version: {}.{}", GLAD_VERSION_MAJOR(gl_version), GLAD_VERSION_MINOR(gl_version));

		// Platform information
		VIZZY_DEBUG("platform = {}", SDL_GetPlatform());
		VIZZY_DEBUG("cpus = {}", SDL_GetCPUCount());
		VIZZY_DEBUG("l1 cache = {}b", SDL_GetCPUCacheLineSize());
		VIZZY_DEBUG("system memory = {}MiB", SDL_GetSystemRAM());
		VIZZY_DEBUG("video driver = {}", SDL_GetCurrentVideoDriver());

		// Set debug callbacks
		gladSetGLPreCallback(pre_callback);
		gladSetGLPostCallback(post_callback);

		if (gl_get_integer(GL_CONTEXT_FLAGS) & GL_CONTEXT_FLAG_DEBUG_BIT) {
			VIZZY_OKAY("debugging enabled");

			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

			glDebugMessageCallback(debug_callback, nullptr);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		}

		// Setup shaders
		auto vert = create_shader(GL_VERTEX_SHADER, { R"(
			#version 460 core

			out vec3 out_coord;

			layout (location = 0) in vec3 in_coord;

			void main() {
				gl_Position = vec4(in_coord, 1);
				out_coord = in_coord;
			}
		)" });

		auto frag = create_shader(GL_FRAGMENT_SHADER, { R"(
			#version 460 core

			uniform float aspect;
			uniform float current_time;

			uniform float kick;
			uniform float snare;
			uniform float hat;

			uniform float tb303;

			out vec4 colour;
			in vec3 out_coord;

			void main() {
			    colour = vec4(length(out_coord.xy * kick), length(out_coord.xy * kick), length(out_coord.xy * kick), 1.0);
			}
		)" });

		auto program = create_program({ vert, frag });

		// auto pipeline = create_pipeline({
		// 	vert,
		// 	frag,
		// });

		glUseProgram(program);

		GLuint vao;
		GLuint vbo;
		GLuint ebo;

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

		GLfloat vertices[] = {
			-1,
			-1,
			0,  // bottom left corner
			-1,
			1,
			0,  // top left corner
			1,
			1,
			0,  // top right corner
			1,
			-1,
			0,
		};  // bottom right corner

		GLubyte indices[] = {
			0,
			1,
			2,  // first triangle (bottom left - top left - top right)
			0,
			2,
			3,
		};  // second triangle (bottom left - top right - bottom right)

		// vertex attributes
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

		// Event loop
		VIZZY_OKAY("loop");

		SDL_Event ev;
		bool running = true;

		auto loop_start = vizzy::clock::now();

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

			{
				std::unique_lock lock { envelope_mutex };

				update_envelopes(envelopes);
				assign_envelopes(envelopes, program);
			}

			int w, h;
			SDL_GL_GetDrawableSize(window, &w, &h);

			float aspect = static_cast<float>(w) / static_cast<float>(h);
			VIZZY_INSPECT(aspect);
			glUniform1f(glGetUniformLocation(program, "aspect"), aspect);

			std::chrono::duration<float> seconds = vizzy::clock::now() - loop_start;
			glUniform1f(glGetUniformLocation(program, "current_time"), seconds.count());

			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

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
