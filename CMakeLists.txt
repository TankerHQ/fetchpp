project(fetchpp CXX)

cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

option(WARN_AS_ERROR "add -Werror during compilation" OFF)
option(ENABLE_TESTING "Enable tests" ON)

if (WARN_AS_ERROR)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()

add_subdirectory(libs/fetchpp)

if (ENABLE_TESTING)
  enable_testing()
endif()

