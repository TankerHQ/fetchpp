project(fetchpp CXX)
cmake_minimum_required(VERSION 3.4)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_library(fetchpp
  include/fetchpp/fetch.hpp
  include/fetchpp/get.hpp
  include/fetchpp/post.hpp
  include/fetchpp/version.hpp

  include/fetchpp/core/detail/async_http_result.hpp
  include/fetchpp/core/detail/connect_composer.hpp
  include/fetchpp/core/detail/coroutine.hpp
  include/fetchpp/core/detail/fetch.hpp
  include/fetchpp/core/detail/http_stable_async.hpp
  include/fetchpp/core/detail/session_work_queue.hpp
  include/fetchpp/core/cache_mode.hpp
  include/fetchpp/core/field_arg.hpp
  include/fetchpp/core/connect.hpp
  include/fetchpp/core/transport_traits.hpp
  include/fetchpp/core/options.hpp
  include/fetchpp/core/process_one.hpp
  include/fetchpp/core/redirect_handling.hpp
  
  include/fetchpp/http/detail/json_body.hpp
  include/fetchpp/http/authorization.hpp
  include/fetchpp/http/content_type.hpp
  include/fetchpp/http/field.hpp
  include/fetchpp/http/headers.hpp
  include/fetchpp/http/json_body.hpp
  include/fetchpp/http/request.hpp
  include/fetchpp/http/response.hpp
  include/fetchpp/http/simple_response.hpp
  include/fetchpp/http/url.hpp

  include/fetchpp/alias/beast.hpp
  include/fetchpp/alias/error_code.hpp
  include/fetchpp/alias/http.hpp
  include/fetchpp/alias/net.hpp
  include/fetchpp/alias/tcp.hpp

  src/core/cache_mode.cpp
  src/core/basic_transport.cpp
  src/core/session_work_queue.cpp
  src/http/detail/json_body.cpp
  src/http/detail/request.cpp
  src/http/authorization.cpp
  src/http/content_type.cpp
  src/http/field_arg.cpp
  src/http/response.cpp
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
target_compile_definitions(fetchpp PUBLIC BOOST_BEAST_SEPARATE_COMPILATION BOOST_BEAST_USE_STD_STRING_VIEW)

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
