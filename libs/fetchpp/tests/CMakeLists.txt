project(fetchpp-functional-tests)

add_library(test_helpers
  helpers/format.cpp
  helpers/test_domain.cpp
  helpers/https_connect.cpp
  helpers/worker_fixture.cpp
  helpers/server_certificate.cpp
)

find_package(fmt REQUIRED)
find_package(Catch2 REQUIRED)

target_link_libraries(test_helpers skyr-url::skyr-url Boost::Boost libressl::libressl fmt::fmt nlohmann_json::nlohmann_json)

add_library(test_main
  main.cpp)
target_link_libraries(test_main Catch2::Catch2)

add_library(fake_server
  helpers/fake_server.cpp
)
target_link_libraries(fake_server fetchpp fmt::fmt nlohmann_json::nlohmann_json)

add_executable(test_unitary
  test_request.cpp
  test_url.cpp
  test_proxy_match.cpp
)
target_link_libraries(test_unitary test_main test_helpers fetchpp fmt::fmt nlohmann_json::nlohmann_json)

add_executable(test_functional
  tests.cpp
)
target_link_libraries(test_functional test_main test_helpers fetchpp fmt::fmt nlohmann_json::nlohmann_json)

add_executable(test_transport
  test_transport.cpp
)
target_link_libraries(test_transport test_main test_helpers fetchpp fmt::fmt nlohmann_json::nlohmann_json)

add_executable(test_proxy_transport
  test_tunnel_transport.cpp
)
target_link_libraries(test_proxy_transport test_main test_helpers fetchpp fmt::fmt nlohmann_json::nlohmann_json)

add_executable(test_session
  test_session.cpp
)
target_link_libraries(test_session test_main test_helpers fetchpp fmt::fmt nlohmann_json::nlohmann_json)

add_executable(test_client
  test_client.cpp
)
target_link_libraries(test_client test_main test_helpers fetchpp fmt::fmt nlohmann_json::nlohmann_json)

add_executable(test_process
  test_process_one.cpp
)
target_link_libraries(test_process test_main test_helpers fetchpp fmt::fmt nlohmann_json::nlohmann_json)

add_test(NAME test_unitary COMMAND test_unitary)
add_test(NAME test_transport COMMAND test_transport)
add_test(NAME test_process COMMAND test_process)

if(UNIX AND NOT APPLE)
  add_test(NAME test_proxy_transport COMMAND test_proxy_transport)
else()
  set(FETCHPP_TEST_FILTER "~[proxy]")
  message(STATUS "disabling proxy testing")
endif()

add_test(NAME test_session COMMAND test_session ${FETCHPP_TEST_FILTER})
add_test(NAME test_client COMMAND test_client ${FETCHPP_TEST_FILTER})
add_test(NAME test_functional COMMAND test_functional)
