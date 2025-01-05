include(ExternalProject)
cmake_minimum_required(VERSION 3.27)

project(vizzy
	VERSION 1.0
	DESCRIPTION "MIDI Visualiser")

ExternalProject_Add(luajit_project
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/deps/luajit"
  PREFIX "${CMAKE_SOURCE_DIR}/deps/luajit"
  BUILD_IN_SOURCE true
  CONFIGURE_COMMAND ""
  BUILD_COMMAND make
  INSTALL_COMMAND ""
)

ExternalProject_Get_Property(luajit_project install_dir)

add_library(luajit STATIC IMPORTED)
set_property(TARGET luajit PROPERTY IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/luajit/src/libluajit.a)

add_dependencies(luajit luajit_project)

add_executable(${PROJECT_NAME} src/vizzy.cpp src/glad.c)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/deps/sanitizers-cmake/cmake" ${CMAKE_MODULE_PATH})
find_package(Sanitizers)
add_sanitizers(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC deps/conflict/include)
target_include_directories(${PROJECT_NAME} PUBLIC deps/fmt/include)
target_include_directories(${PROJECT_NAME} PUBLIC deps/libremidi/include)
target_include_directories(${PROJECT_NAME} PUBLIC deps/sol2/include)
target_include_directories(${PROJECT_NAME} PUBLIC deps/luajit/src)
target_include_directories(${PROJECT_NAME} PUBLIC include)

target_link_libraries(${PROJECT_NAME} PRIVATE luajit)

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
