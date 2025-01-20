#ifndef VIZZY_MACRO_HPP
#define VIZZY_MACRO_HPP

// Macros
namespace vizzy {

// Mark as unused.
#define VIZZY_IMPL_UNUSED0()
#define VIZZY_IMPL_UNUSED1(a)             (void)(a)
#define VIZZY_IMPL_UNUSED2(a, b)          (void)(a), VIZZY_IMPL_UNUSED1(b)
#define VIZZY_IMPL_UNUSED3(a, b, c)       (void)(a), VIZZY_IMPL_UNUSED2(b, c)
#define VIZZY_IMPL_UNUSED4(a, b, c, d)    (void)(a), VIZZY_IMPL_UNUSED3(b, c, d)
#define VIZZY_IMPL_UNUSED5(a, b, c, d, e) (void)(a), VIZZY_IMPL_UNUSED4(b, c, d, e)

#define VIZZY_VA_NUM_ARGS_IMPL(_0, _1, _2, _3, _4, _5, N, ...) N
#define VIZZY_VA_NUM_ARGS(...)                                 VIZZY_VA_NUM_ARGS_IMPL(100, ##__VA_ARGS__, 5, 4, 3, 2, 1, 0)

#define VIZZY_UNUSED_IMPL_(nargs) VIZZY_IMPL_UNUSED##nargs
#define VIZZY_UNUSED_IMPL(nargs)  VIZZY_UNUSED_IMPL_(nargs)
#define VIZZY_UNUSED(...)         VIZZY_UNUSED_IMPL(VIZZY_VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

// String macros
#define VIZZY_CSTR(s) /* Convert c-string into a string_view before decaying to pointer. */ \
	std::string_view { \
		s, ((const char*)s) + (sizeof(s) - 1) \
	}

#define VIZZY_STR_IMPL_(x) #x
#define VIZZY_STR(x)       VIZZY_STR_IMPL_(x)

#define VIZZY_CAT_IMPL_(x, y) x##y
#define VIZZY_CAT(x, y)       VIZZY_CAT_IMPL_(x, y)

#define VIZZY_VAR(x) VIZZY_CAT(var_, VIZZY_CAT(x, VIZZY_CAT(__LINE__, _)))

// Location info
#define VIZZY_LINEINFO "[" __FILE__ ":" VIZZY_STR(__LINE__) "]"

#define VIZZY_LOCATION_FILE __FILE__
#define VIZZY_LOCATION_LINE VIZZY_STR(__LINE__)
#define VIZZY_LOCATION_FUNC __func__

// Utils
#define VIZZY_MAX(a, b) ((a > b) ? a : b)
#define VIZZY_MIN(a, b) ((a < b) ? a : b)

}  // namespace vizzy

#endif
