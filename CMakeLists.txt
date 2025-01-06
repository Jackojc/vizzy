include(ExternalProject)
cmake_minimum_required(VERSION 3.27)

project(vizzy
	VERSION 1.0
	DESCRIPTION "MIDI Visualiser")

find_package(SDL2 REQUIRED)
find_package(Lua51 REQUIRED)

add_executable(${PROJECT_NAME} src/vizzy.cpp src/glad.c)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/deps/sanitizers-cmake/cmake" ${CMAKE_MODULE_PATH})
find_package(Sanitizers)
add_sanitizers(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC ${SDL2_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${LUA_INCLUDE_DIR})

target_include_directories(${PROJECT_NAME} PUBLIC deps/conflict/include)
target_include_directories(${PROJECT_NAME} PUBLIC deps/fmt/include)
target_include_directories(${PROJECT_NAME} PUBLIC deps/libremidi/include)
target_include_directories(${PROJECT_NAME} PUBLIC deps/sol2/include)

target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(${PROJECT_NAME} PRIVATE ${LUA_LIBRARIES})

add_subdirectory(deps/libremidi/)
target_link_libraries(${PROJECT_NAME} PRIVATE libremidi)

add_subdirectory(deps/fmt/)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt)

add_subdirectory(deps/sol2/)
target_link_libraries(${PROJECT_NAME} PRIVATE sol2)

target_compile_options(${PROJECT_NAME} PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)
