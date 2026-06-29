cmake_minimum_required(VERSION 3.14)
project(stringified LANGUAGES CXX)

# --- Library target ---
# stringified is header-only and dependency-free: one header (stringified.hh)
# that wraps a value as a std::string_view with optional backing storage, so a
# function taking strings can accept a std::string, a std::string_view, or a
# string literal uniformly and still get a null-terminated c_str() on demand.
# Nothing but the standard <string>/<string_view>/<ostream> headers behind it.
# So the target is a pure INTERFACE library that just puts the source dir on the
# consumer's include path.
add_library(stringified INTERFACE)
target_include_directories(stringified INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_features(stringified INTERFACE cxx_std_20)
# Alias so consumers can link `stringified::stringified` whether they add this
# via add_subdirectory or FetchContent.
add_library(stringified::stringified ALIAS stringified)

# Only build the test when this repo is the top-level project, so consumers
# (e.g. via FetchContent) don't build our tests.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  enable_testing()
  add_executable(stringified_test test/test.cc)
  target_link_libraries(stringified_test PRIVATE stringified)
  add_test(NAME stringified COMMAND stringified_test)
endif()
