project(fetchpp-functional-tests)

add_library(test_helpers
  helpers/format.cpp
  helpers/test_domain.cpp
)
target_link_libraries(test_helpers CONAN_PKG::Boost CONAN_PKG::fmt CONAN_PKG::jsonformoderncpp)

add_executable(test_functional
  
  tests.cpp
  test_process_one.cpp
  unit_test.cpp
  main.cpp
)
target_link_libraries(test_functional test_helpers fetchpp CONAN_PKG::Catch2 CONAN_PKG::fmt CONAN_PKG::jsonformoderncpp)

add_executable(test_transport
  test_transport.cpp
  main.cpp
)
target_link_libraries(test_transport test_helpers fetchpp CONAN_PKG::Catch2 CONAN_PKG::fmt CONAN_PKG::jsonformoderncpp)

add_executable(test_session
  test_session.cpp
  main.cpp
)
target_link_libraries(test_session test_helpers fetchpp CONAN_PKG::Catch2 CONAN_PKG::fmt CONAN_PKG::jsonformoderncpp)

add_executable(test_client
  test_client.cpp
  main.cpp
)
target_link_libraries(test_client test_helpers fetchpp CONAN_PKG::Catch2 CONAN_PKG::fmt CONAN_PKG::jsonformoderncpp)

add_test(NAME test_transport COMMAND test_transport)
add_test(NAME test_session COMMAND test_session)
add_test(NAME test_client COMMAND test_client)
add_test(NAME test_functional COMMAND test_functional)
