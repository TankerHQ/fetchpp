project(fetchpp CXX)
cmake_minimum_required(VERSION 3.4)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_library(fetchpp
  include/fetchpp/fetch.hpp
  include/fetchpp/client.hpp
  include/fetchpp/get.hpp
  include/fetchpp/post.hpp
  include/fetchpp/version.hpp

  include/fetchpp/net/make_dispatch.hpp
  include/fetchpp/net/make_post.hpp

  include/fetchpp/core/detail/async_http_result.hpp
  include/fetchpp/core/detail/coroutine.hpp
  include/fetchpp/core/detail/fetch.hpp
  include/fetchpp/core/detail/client.hpp
  include/fetchpp/core/detail/http_stable_async.hpp
  include/fetchpp/core/detail/session_base.hpp
  include/fetchpp/core/detail/overloaded.hpp

  include/fetchpp/core/cache_mode.hpp
  include/fetchpp/core/field_arg.hpp
  include/fetchpp/core/endpoint.hpp
  include/fetchpp/core/basic_transport.hpp
  include/fetchpp/core/ssl_transport.hpp
  include/fetchpp/core/tcp_transport.hpp
  include/fetchpp/core/transport_traits.hpp
  include/fetchpp/core/options.hpp
  include/fetchpp/core/process_one.hpp
  include/fetchpp/core/redirect_handling.hpp

  include/fetchpp/http/authorization.hpp
  include/fetchpp/http/content_type.hpp
  include/fetchpp/http/field.hpp
  include/fetchpp/http/headers.hpp
  include/fetchpp/http/proxy.hpp
  include/fetchpp/http/request.hpp
  include/fetchpp/http/response.hpp
  include/fetchpp/http/url.hpp

  include/fetchpp/alias/beast.hpp
  include/fetchpp/alias/error_code.hpp
  include/fetchpp/alias/http.hpp
  include/fetchpp/alias/net.hpp
  include/fetchpp/alias/tcp.hpp
  include/fetchpp/alias/ssl.hpp

  src/core/cache_mode.cpp
  src/core/endpoint.cpp
  src/core/session_base.cpp
  src/core/client.cpp
  src/core/fetch.cpp
  src/http/detail/request.cpp
  src/http/authorization.cpp
  src/http/content_type.cpp
  src/http/field_arg.cpp
  src/http/proxy.cpp
  src/http/url.cpp
  src/beast.cpp
)

target_include_directories(fetchpp
  PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(fetchpp PUBLIC BOOST_BEAST_SEPARATE_COMPILATION)

target_link_libraries(fetchpp PUBLIC CONAN_PKG::boost CONAN_PKG::skyr-url CONAN_PKG::nlohmann_json CONAN_PKG::libressl)

install(DIRECTORY include DESTINATION .)

install(TARGETS fetchpp
  EXPORT fetchpp
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)


if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
