#ifndef VIZZY_ENVELOPE_HPP
#define VIZZY_ENVELOPE_HPP

#include <chrono>
#include <functional>

#include <libremidi/message.hpp>
#include <glad/gl.h>

#include <vizzy/log.hpp>

// Time
namespace vizzy {
	using clock = std::chrono::steady_clock;
	using timeunit = std::chrono::milliseconds;
	using timepoint = std::chrono::time_point<clock>;
}

// Easing functions
namespace vizzy {
	inline float linear(float start, float end, float time) {
		return (1.0f - time) * start + time * end;
	}
}

namespace vizzy {
	struct Stage {
		vizzy::timeunit duration;
		float target;
	};

	struct Segment {
		vizzy::timeunit start_time;
		vizzy::timeunit end_time;

		float start_amp;
		float end_amp;
	};

	struct Envelope {
		std::string_view name;

		std::function<bool(libremidi::message)> pattern;
		std::vector<Segment> segments;

		vizzy::timepoint trigger = vizzy::timepoint::max();

		float trigger_amplitude = .0f;
		float current_amplitude = .0f;
	};

	inline std::ostream& operator<<(std::ostream& os, const Stage& stage) {
		fmt::print(os, fmt::runtime("{{ .duration={}, .target={} }}"), stage.duration.count(), stage.target);

		return os;
	}

	inline std::ostream& operator<<(std::ostream& os, const Segment& segment) {
		fmt::print(os,
			fmt::runtime("{{ .start_time={}, .end_time={}, .start_amp={}, .end_amp={} }}"),
			segment.start_time,
			segment.end_time,
			segment.start_amp,
			segment.end_amp);

		return os;
	}

	inline std::ostream& operator<<(std::ostream& os, const Envelope& env) {
		fmt::print(os,
			fmt::runtime("{{ .name='{}', .segments={}, .trigger={}, .trigger_amplitude={}, .current_amplitude={} }}"),
			env.name,
			env.segments,
			env.trigger.time_since_epoch().count(),
			env.trigger_amplitude,
			env.current_amplitude);

		return os;
	}
}  // namespace vizzy

template <>
struct fmt::formatter<vizzy::Stage>: fmt::ostream_formatter {};

template <>
struct fmt::formatter<vizzy::Segment>: fmt::ostream_formatter {};

template <>
struct fmt::formatter<vizzy::Envelope>: fmt::ostream_formatter {};

namespace vizzy {
	inline decltype(auto) to_segments(std::vector<Stage> stages) {
		using namespace std::chrono_literals;

		std::vector<Segment> segments;

		vizzy::timeunit duration_so_far = 0s;
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

	inline decltype(auto) env_update(vizzy::Envelope env, vizzy::timepoint current_time) {
		if (env.segments.empty()) {
			return env;
		}

		auto env_relative_time = current_time - env.trigger;

		auto it = std::find_if(env.segments.begin(), env.segments.end(), [&](const auto& segment) {
			return env_relative_time >= segment.start_time and env_relative_time < segment.end_time;
		});

		auto index = std::distance(env.segments.begin(), it);

		if (it != env.segments.end()) {
			// Active stage
			auto [start_time, end_time, start_amp, end_amp] = *it;

			std::chrono::duration<float> stage_relative_time = env_relative_time - start_time;
			auto normalised_time = stage_relative_time / (end_time - start_time);

			float amp = index == 0 ? env.trigger_amplitude : start_amp;

			env.current_amplitude = linear(amp, end_amp, normalised_time);
		}

		else {
			env.current_amplitude = env.segments.front().start_amp;
		}

		return env;
	}

	inline decltype(auto) env_trigger(vizzy::Envelope env, libremidi::message msg) {
		if (env.pattern(msg)) {
			env.trigger_amplitude = env.current_amplitude;
			env.trigger = vizzy::clock::now();
		}

		return env;
	}

	inline decltype(auto) env_bind(vizzy::Envelope env, std::vector<GLuint>&& programs) {
		for (GLuint p: programs) {
			glUniform1f(glGetUniformLocation(p, env.name.data()), env.current_amplitude);
		}

		return env;
	}

}  // namespace vizzy

// Utils
namespace vizzy {
	inline decltype(auto) attack_release(vizzy::timeunit attack, vizzy::timeunit release) {
		return vizzy::to_segments({
			{ .duration = attack, .target = 1.f },
			{ .duration = release, .target = 0.f },
		});
	}

	inline decltype(auto) attack_hold_release(vizzy::timeunit attack, vizzy::timeunit hold, vizzy::timeunit release) {
		return vizzy::to_segments({
			{ .duration = attack, .target = 1.f },
			{ .duration = hold, .target = 1.f },
			{ .duration = release, .target = 0.f },
		});
	}
}

#endif
