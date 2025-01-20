#ifndef VIZZY_GL_HPP
#define VIZZY_GL_HPP

#include <functional>

#include <SDL2/SDL.h>
#include <glad/gl.h>
#include <glm/glm.hpp>

#include <vizzy/util.hpp>

namespace vizzy::gl::detail {
#define VIZZY_GL_ENUM(x) \
	case x: return #x;

	[[nodiscard]] inline std::string_view error_to_str(GLenum error) {
		switch (error) {
			VIZZY_GL_ENUM(GL_INVALID_ENUM)
			VIZZY_GL_ENUM(GL_INVALID_VALUE)
			VIZZY_GL_ENUM(GL_INVALID_OPERATION)
			VIZZY_GL_ENUM(GL_INVALID_FRAMEBUFFER_OPERATION)
			VIZZY_GL_ENUM(GL_OUT_OF_MEMORY)
			VIZZY_GL_ENUM(GL_STACK_UNDERFLOW)
			VIZZY_GL_ENUM(GL_STACK_OVERFLOW)
		}

		VIZZY_UNREACHABLE();
	}

	[[nodiscard]] inline std::string_view source_to_str(GLenum source) {
		switch (source) {
			VIZZY_GL_ENUM(GL_DEBUG_SOURCE_API)
			VIZZY_GL_ENUM(GL_DEBUG_SOURCE_WINDOW_SYSTEM)
			VIZZY_GL_ENUM(GL_DEBUG_SOURCE_SHADER_COMPILER)
			VIZZY_GL_ENUM(GL_DEBUG_SOURCE_THIRD_PARTY)
			VIZZY_GL_ENUM(GL_DEBUG_SOURCE_APPLICATION)
			VIZZY_GL_ENUM(GL_DEBUG_SOURCE_OTHER)
		}

		VIZZY_UNREACHABLE();
	}

	[[nodiscard]] inline std::string_view type_to_str(GLenum type) {
		switch (type) {
			VIZZY_GL_ENUM(GL_DEBUG_TYPE_ERROR)
			VIZZY_GL_ENUM(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR)
			VIZZY_GL_ENUM(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
			VIZZY_GL_ENUM(GL_DEBUG_TYPE_PORTABILITY)
			VIZZY_GL_ENUM(GL_DEBUG_TYPE_PERFORMANCE)
			VIZZY_GL_ENUM(GL_DEBUG_TYPE_MARKER)
			VIZZY_GL_ENUM(GL_DEBUG_TYPE_PUSH_GROUP)
			VIZZY_GL_ENUM(GL_DEBUG_TYPE_POP_GROUP)
			VIZZY_GL_ENUM(GL_DEBUG_TYPE_OTHER)
		}

		VIZZY_UNREACHABLE();
	}

	[[nodiscard]] inline std::string_view severity_to_str(GLenum severity) {
		switch (severity) {
			VIZZY_GL_ENUM(GL_DEBUG_SEVERITY_HIGH)
			VIZZY_GL_ENUM(GL_DEBUG_SEVERITY_MEDIUM)
			VIZZY_GL_ENUM(GL_DEBUG_SEVERITY_LOW)
			VIZZY_GL_ENUM(GL_DEBUG_SEVERITY_NOTIFICATION)
		}

		VIZZY_UNREACHABLE();
	}

#undef VIZZY_GL_ENUM

	[[nodiscard]] inline GLbitfield shader_type_to_bitfield(GLint kind) {
		switch (kind) {
			case GL_COMPUTE_SHADER: return GL_COMPUTE_SHADER_BIT;
			case GL_VERTEX_SHADER: return GL_VERTEX_SHADER_BIT;
			case GL_TESS_CONTROL_SHADER: return GL_TESS_CONTROL_SHADER_BIT;
			case GL_TESS_EVALUATION_SHADER: return GL_TESS_EVALUATION_SHADER_BIT;
			case GL_GEOMETRY_SHADER: return GL_GEOMETRY_SHADER_BIT;
			case GL_FRAGMENT_SHADER: return GL_FRAGMENT_SHADER_BIT;
		}

		VIZZY_UNREACHABLE();
	}

	[[nodiscard]] inline LogKind severity_to_logkind(GLenum severity) {
		switch (severity) {
			case GL_DEBUG_SEVERITY_NOTIFICATION: return LogKind::Okay;
			case GL_DEBUG_SEVERITY_LOW: return LogKind::Trace;
			case GL_DEBUG_SEVERITY_MEDIUM: return LogKind::Warn;
			case GL_DEBUG_SEVERITY_HIGH: return LogKind::Error;
		}

		VIZZY_UNREACHABLE();
	}

}  // namespace vizzy::gl::detail

// Getters
namespace vizzy::gl {
	template <typename F, typename... Ts>
	[[nodiscard]] inline decltype(auto) call(F&& fn, Ts&&... args) {
		decltype(auto) v = fn(std::forward<Ts>(args)...);

		if (GLenum status = glGetError(); status != GL_NO_ERROR) {
			vizzy::die("glGetError(): {}", detail::error_to_str(status));
		}

		return v;
	}

	template <typename F, typename... Ts>
		requires std::same_as<std::invoke_result_t<F, Ts...>, void>
	inline void call(F&& fn, Ts&&... args) {
		fn(std::forward<Ts>(args)...);

		if (GLenum status = glGetError(); status != GL_NO_ERROR) {
			vizzy::die("glGetError(): {}", detail::error_to_str(status));
		}
	}

	template <typename F, typename... Ts>
		requires std::invocable<F, Ts..., GLint*>
	[[nodiscard]] inline decltype(auto) get(F fn, Ts&&... args) {
		// INFO: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetProgramPipeline.xhtml
		// INFO: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetProgram.xhtml
		// INFO: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetShader.xhtml
		// INFO: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGet.xhtml

		GLint v = 0;

		call(fn, std::forward<Ts>(args)..., &v);
		return v;
	}

	[[nodiscard]] inline decltype(auto) gl_get_pipeline(GLuint pipeline, GLenum param) {
		return get(glGetProgramPipelineiv, pipeline, param);
	}

	[[nodiscard]] inline decltype(auto) gl_get_program(GLuint program, GLenum param) {
		return get(glGetProgramiv, program, param);
	}

	[[nodiscard]] inline decltype(auto) gl_get_shader(GLuint shader, GLenum param) {
		return get(glGetShaderiv, shader, param);
	}

	[[nodiscard]] inline decltype(auto) gl_get_integer(GLenum param) {
		return get(glGetIntegerv, param);
	}
}  // namespace vizzy::gl

// Wrappers
namespace vizzy::gl {
	[[nodiscard]] inline GLuint create_shader(GLenum kind, std::vector<std::string_view> sv) {
		VIZZY_FUNCTION();

		GLuint shader = call(glCreateShader, kind);

		VIZZY_DEBUG("shader type = {}", kind);

		if (shader == 0) {
			vizzy::die("glCreateShader failed!");
		}

		std::vector<const GLchar*> sources;
		std::transform(sv.begin(), sv.end(), std::back_inserter(sources), std::mem_fn(&decltype(sv)::value_type::data));

		call(glShaderSource, shader, sources.size(), sources.data(), nullptr);
		call(glCompileShader, shader);

		int ok = gl_get_shader(shader, GL_COMPILE_STATUS);
		int info_length = gl_get_shader(shader, GL_INFO_LOG_LENGTH);

		std::string info;
		info.resize(info_length, '\0');

		call(glGetShaderInfoLog, shader, info_length, nullptr, info.data());

		VIZZY_DEBUG("ok = {}", ok ? "true" : "false");
		VIZZY_DEBUG("info = '{}'", info.empty() ? "<empty>" : info);

		if (ok != GL_TRUE) {
			call(glDeleteShader, shader);
			vizzy::die("shader compilation failed! GL: {}", info);
		}

		VIZZY_OKAY("successfully compiled shader ({})", shader);

		return shader;
	}

	[[nodiscard]] inline GLuint create_program(std::vector<GLuint> shaders) {
		VIZZY_FUNCTION();

		GLuint program = call(glCreateProgram);
		// call(glProgramParameteri, program, GL_PROGRAM_SEPARABLE, GL_TRUE);

		if (program == 0) {
			vizzy::die("glCreateProgram failed!");
		}

		// Linking
		for (auto shader: shaders) {
			VIZZY_DEBUG("shader: {}", shader);
			call(glAttachShader, program, shader);
			call(glDeleteShader, shader);
		}

		call(glLinkProgram, program);

		int ok = gl_get_program(program, GL_LINK_STATUS);
		int info_length = gl_get_program(program, GL_INFO_LOG_LENGTH);
		int shader_count = gl_get_program(program, GL_ATTACHED_SHADERS);
		int binary_length = gl_get_program(program, GL_PROGRAM_BINARY_LENGTH);

		std::string info;
		info.resize(info_length, '\0');

		call(glGetProgramInfoLog, program, info_length, nullptr, info.data());

		VIZZY_DEBUG("ok = {}", ok ? "true" : "false");
		VIZZY_DEBUG("info = '{}'", info.empty() ? "<empty>" : info);
		VIZZY_DEBUG("binary length = {}b", binary_length);
		VIZZY_DEBUG("shader count = {}", shader_count);

		if (ok != GL_TRUE) {
			call(glDeleteProgram, program);
			vizzy::die("shader linking failed! GL: {}", info);
		}

		// Validation
		call(glValidateProgram, program);

		int valid = gl_get_program(program, GL_VALIDATE_STATUS);
		int validation_length = gl_get_program(program, GL_INFO_LOG_LENGTH);

		std::string validation;
		validation.resize(validation_length, '\0');

		call(glGetProgramInfoLog, program, validation_length, nullptr, validation.data());

		VIZZY_DEBUG("valid = {}", valid ? "true" : "false");
		VIZZY_DEBUG("validation = '{}'", validation.empty() ? "<empty>" : validation);

		if (valid != GL_TRUE) {
			call(glDeleteProgram, program);
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
			call(glUseProgramStages, pipeline, stage, program);
		}

		// Validation (INFO: https://docs.gl/es3/glValidateProgramPipeline)
		glValidateProgramPipeline(pipeline);

		int valid = gl_get_pipeline(pipeline, GL_VALIDATE_STATUS);
		int validation_length = gl_get_pipeline(pipeline, GL_INFO_LOG_LENGTH);

		std::string validation;
		validation.resize(validation_length, '\0');

		call(glGetProgramPipelineInfoLog, pipeline, validation_length, nullptr, validation.data());

		VIZZY_DEBUG("valid = {}", valid ? "true" : "false");
		VIZZY_DEBUG("validation = '{}'", validation.empty() ? "<empty>" : validation);

		if (valid != GL_TRUE) {
			call(glDeleteProgramPipelines, 1, &pipeline);
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

			call(glGetAttachedShaders, program, shaders_length, nullptr, shaders.data());

			GLbitfield stages = 0;

			for (auto shader: shaders) {
				VIZZY_DEBUG("shader = {}", shader);

				int kind = gl_get_shader(shader, GL_SHADER_TYPE);
				stages |= detail::shader_type_to_bitfield(kind);

				VIZZY_DEBUG("shader type = {}", kind);
			}

			VIZZY_DEBUG("stages = {}", stages);
			program_mapping.emplace_back(stages, program);
		}

		VIZZY_OKAY("successfully generated pipeline mapping ({})", program_mapping);
		return create_pipeline(program_mapping);
	}

}  // namespace vizzy::gl

// Debugging
namespace vizzy::gl {
	namespace detail {
		using PreCallback = void (*)(const char* name, GLADapiproc apiproc, int len_args, ...);
		using PostCallback = void (*)(void* ret, const char* name, GLADapiproc apiproc, int len_args, ...);

		// GLAD callbacks
		inline void pre_callback([[maybe_unused]] const char* name,
			[[maybe_unused]] GLADapiproc apiproc,
			[[maybe_unused]] int len_args,
			...) {
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
			[[maybe_unused]] GLuint id,
			GLenum severity,
			[[maybe_unused]] GLsizei length,
			const GLchar* message,
			[[maybe_unused]] const void* user_param) {
			auto source_str = detail::source_to_str(source);
			auto type_str = detail::type_to_str(type);
			// auto severity_str = detail::severity_to_str(severity);

			auto logkind = detail::severity_to_logkind(severity);

			vizzy::log(logkind, "[{} {}]: {}", source_str, type_str, message);
		}
	}  // namespace detail

	inline void setup_debug_callbacks() {
		VIZZY_FUNCTION();

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

		gladSetGLPreCallback(detail::pre_callback);
		gladSetGLPostCallback(detail::post_callback);

		if (gl_get_integer(GL_CONTEXT_FLAGS) & GL_CONTEXT_FLAG_DEBUG_BIT) {
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

			glDebugMessageCallback(detail::debug_callback, nullptr);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		}
	}
}  // namespace vizzy::gl

#endif
