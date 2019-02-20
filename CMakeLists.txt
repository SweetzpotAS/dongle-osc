cmake_minimum_required(VERSION 3.6)
project(dongle-osc LANGUAGES CXX VERSION 1.0.0)

add_executable(dongle-osc dongle-osc.cpp)
target_include_directories(dongle-osc PRIVATE thirdparty/oscpp/include)
target_compile_features(dongle-osc PUBLIC cxx_std_14)
target_compile_options(dongle-osc PRIVATE -Werror -Wall -Wextra)

install(TARGETS dongle-osc DESTINATION bin/)

set(CPACK_PACKAGE_CONTACT "support@sweetzpot.com")
list(APPEND CPACK_GENERATOR "TBZ2")

set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

include(CPack)
