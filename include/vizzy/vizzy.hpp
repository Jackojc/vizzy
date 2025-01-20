#ifndef VIZZY_HPP
#define VIZZY_HPP

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/color.h>
#include <fmt/ostream.h>

#include <vizzy/macro.hpp>
#include <vizzy/util.hpp>
#include <vizzy/log.hpp>
#include <vizzy/gl.hpp>
#include <vizzy/env.hpp>

// Definitions
namespace vizzy {
#define VIZZY_EXE "Vizzy"

#define VIZZY_WINDOW_WIDTH  1920
#define VIZZY_WINDOW_HEIGHT 1080

// Bindings were generated as 4.6 core
#define VIZZY_OPENGL_VERSION_MAJOR 4
#define VIZZY_OPENGL_VERSION_MINOR 6
}  // namespace vizzy

#endif
