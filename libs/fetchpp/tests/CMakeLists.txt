project(fetchpp-functional-tests)

add_executable(test_functional
  helpers/format.cpp
  helpers/test_domain.cpp
  
  tests.cpp
  test_process_one.cpp
  main.cpp
)
target_link_libraries(test_functional fetchpp CONAN_PKG::Catch2 CONAN_PKG::fmt CONAN_PKG::jsonformoderncpp)

add_test(NAME test_functional COMMAND test_functional)
