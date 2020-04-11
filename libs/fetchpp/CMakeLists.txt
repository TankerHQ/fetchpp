project(fetchpp CXX)
cmake_minimum_required(VERSION 3.4)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_library(fetchpp
  include/fetchpp/fetch.hpp
  include/fetchpp/get.hpp
  include/fetchpp/post.hpp
  include/fetchpp/field_arg.hpp
  include/fetchpp/field.hpp
  include/fetchpp/process_one.hpp
  include/fetchpp/cache_mode.hpp
  include/fetchpp/json_body.hpp
  include/fetchpp/response.hpp
  include/fetchpp/request.hpp

  include/fetchpp/detail/json_body.hpp
  include/fetchpp/detail/async_http_result.hpp
  include/fetchpp/detail/connect_composer.hpp

  include/fetchpp/alias/beast.hpp
  include/fetchpp/alias/http.hpp
  include/fetchpp/alias/net.hpp

  include/fetchpp/url.hpp

  src/detail/json_body.cpp

  src/url.cpp
  src/field_arg.cpp
  src/cache_mode.cpp
  src/request.cpp
  src/content_type.cpp
)

target_include_directories(fetchpp
  PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(fetchpp PUBLIC CONAN_PKG::Boost CONAN_PKG::jsonformoderncpp)

install(DIRECTORY include DESTINATION .)

install(TARGETS fetchpp
  EXPORT fetchpp
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)


if(ENABLE_TESTING)
  add_subdirectory(tests)
endif()
