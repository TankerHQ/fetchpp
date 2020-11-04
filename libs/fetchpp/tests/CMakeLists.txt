project(fetchpp-functional-tests)

add_library(test_helpers
  helpers/format.cpp
  helpers/test_domain.cpp
  helpers/https_connect.cpp
)
target_link_libraries(test_helpers CONAN_PKG::skyr-url CONAN_PKG::boost CONAN_PKG::fmt CONAN_PKG::nlohmann_json)

add_executable(test_unitary
  test_request.cpp
  test_url.cpp
  test_proxy_match.cpp
  main.cpp
)
target_link_libraries(test_unitary test_helpers fetchpp CONAN_PKG::skyr-url CONAN_PKG::catch2 CONAN_PKG::fmt CONAN_PKG::nlohmann_json)

add_executable(test_functional
  tests.cpp
  test_process_one.cpp
  main.cpp
)
target_link_libraries(test_functional test_helpers fetchpp CONAN_PKG::catch2 CONAN_PKG::fmt CONAN_PKG::nlohmann_json)

add_executable(test_transport
  test_transport.cpp
  main.cpp
)
target_link_libraries(test_transport test_helpers fetchpp CONAN_PKG::catch2 CONAN_PKG::fmt CONAN_PKG::nlohmann_json)

add_executable(test_proxy_transport
  test_tunnel_transport.cpp
  main.cpp
)
target_link_libraries(test_proxy_transport test_helpers fetchpp CONAN_PKG::catch2 CONAN_PKG::fmt CONAN_PKG::nlohmann_json)

add_executable(test_session
  test_session.cpp
  main.cpp
)
target_link_libraries(test_session test_helpers fetchpp CONAN_PKG::catch2 CONAN_PKG::fmt CONAN_PKG::nlohmann_json)

add_executable(test_client
  test_client.cpp
  main.cpp
)
target_link_libraries(test_client test_helpers fetchpp CONAN_PKG::catch2 CONAN_PKG::fmt CONAN_PKG::nlohmann_json)

add_test(NAME test_unitary COMMAND test_unitary)
add_test(NAME test_transport COMMAND test_transport)

if(UNIX AND NOT APPLE)
  add_test(NAME test_proxy_transport COMMAND test_proxy_transport)
else()
  set(FETCHPP_TEST_FILTER "~[proxy]")
  message(STATUS "disabling proxy testing")
endif()

add_test(NAME test_session COMMAND test_session ${FETCHPP_TEST_FILTER})
add_test(NAME test_client COMMAND test_client ~[stop]${FETCHPP_TEST_FILTER})
add_test(NAME test_functional COMMAND test_functional)
