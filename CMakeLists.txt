project(fetchpp CXX)

cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

option(WARN_AS_ERROR "add -Werror during compilation" OFF)
option(ENABLE_TESTING "Enable tests" ON)
option(WITH_COVERAGE "Enable coverage" OFF)
option(WARN_AS_ERROR "Add -Werror during compilation" OFF)

if (WARN_AS_ERROR)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()

if (ENABLE_TESTING)
  enable_testing()
endif()

# CMAKE_C_FLAGS and the like are _strings_, not lists.
# So, we need a macro so that we can rewrite the values
# in place, and avoid appending the flags twice
macro(fetchpp_add_flags var flags)
  string(FIND "${${var}}" ${flags} _res)
  if(${_res} EQUAL "-1")
    set(${var} "${${var}} ${flags}")
  endif()
endmacro()

if (MINGW)
  # optimization flags are missing by default, and cause "file too big" errors
  fetchpp_add_flags(CMAKE_CXX_FLAGS_DEBUG "-Og")
  fetchpp_add_flags(CMAKE_CXX_FLAGS_RELEASE "-O3")
  fetchpp_add_flags(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g")
  fetchpp_add_flags(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
endif()

if(${WITH_COVERAGE})
  if(WIN32)
    message(WARNING "WITH_COVERAGE ignored on Windows")
  else()
    message(STATUS "Building with coverage")
    fetchpp_add_flags(CMAKE_C_FLAGS "--coverage")
    fetchpp_add_flags(CMAKE_CXX_FLAGS "--coverage")
    fetchpp_add_flags(CMAKE_EXE_LINKER_FLAGS "--coverage")
    fetchpp_add_flags(CMAKE_SHARED_LINKER_FLAGS "--coverage")
    fetchpp_add_flags(CMAKE_MODULE_LINKER_FLAGS "--coverage")
  endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # warns about void foo(); (should be void foo(void);).
  fetchpp_add_flags(CMAKE_C_FLAGS "-Wstrict-prototypes")
  set(COMMON_COMPILE_FLAGS "-Wall -Wextra -pedantic -Wmissing-declarations -Werror=return-type")
  if(WARN_AS_ERROR)
    fetchpp_add_flags(CMAKE_CXX_FLAGS "-Werror")
  endif()
elseif(MSVC)
  set(COMMON_COMPILE_FLAGS "/MP /bigobj")
endif()

fetchpp_add_flags(CMAKE_C_FLAGS ${COMMON_COMPILE_FLAGS})
fetchpp_add_flags(CMAKE_CXX_FLAGS ${COMMON_COMPILE_FLAGS})

add_subdirectory(libs/fetchpp)
