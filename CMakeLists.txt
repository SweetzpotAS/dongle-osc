cmake_minimum_required(VERSION 3.6)
project(osc-bridge LANGUAGES CXX VERSION 1.0.0)

add_executable(osc-bridge osc-bridge.cpp)
target_include_directories(osc-bridge PRIVATE thirdparty/oscpp/include)
target_compile_features(osc-bridge PUBLIC cxx_std_14)
target_compile_options(osc-bridge PRIVATE -Werror -Wall -Wextra)

install(TARGETS osc-bridge DESTINATION bin/)

set(CPACK_PACKAGE_CONTACT "support@sweetzpot.com")
list(APPEND CPACK_GENERATOR "TBZ2")

set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

include(CPack)
