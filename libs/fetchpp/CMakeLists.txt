project(fetchpp CXX)
cmake_minimum_required(VERSION 3.4)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_library(fetchpp
  include/fetchpp/fetchpp.hpp
  include/fetchpp/field_arg.hpp
  include/fetchpp/field.hpp
  include/fetchpp/message.hpp
  include/fetchpp/process_one.hpp
  include/fetchpp/cache_mode.hpp

  include/fetchpp/alias/beast.hpp
  include/fetchpp/alias/http.hpp
  include/fetchpp/alias/net.hpp

  include/fetchpp/url.hpp

  src/url.cpp
  src/field_arg.cpp
  src/cache_mode.cpp
  src/request.cpp

)

target_include_directories(fetchpp
  PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(fetchpp PUBLIC CONAN_PKG::Boost)


if(ENABLE_TESTING)
  add_subdirectory(tests)
endif()
