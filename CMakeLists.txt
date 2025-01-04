cmake_minimum_required(VERSION 3.27)
project(vizzy CXX)

add_executable(vizzy src/vizzy.cpp)
target_compile_features(vizzy PRIVATE cxx_std_20)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/deps/sanitizers-cmake/cmake" ${CMAKE_MODULE_PATH})
find_package(Sanitizers)
add_sanitizers(vizzy)

target_include_directories(vizzy PUBLIC deps/conflict/include)
target_include_directories(vizzy PUBLIC deps/fmt/include)
target_include_directories(vizzy PUBLIC deps/libremidi/include)
target_include_directories(vizzy PUBLIC deps/commandline/src/)
target_include_directories(vizzy PUBLIC include)

add_subdirectory(deps/libremidi/)
target_link_libraries(vizzy PRIVATE libremidi)

add_subdirectory(deps/fmt/)
target_link_libraries(vizzy PRIVATE fmt)

add_subdirectory(deps/commandline/)
target_link_libraries(vizzy PRIVATE commandline)

target_compile_options(vizzy PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)
