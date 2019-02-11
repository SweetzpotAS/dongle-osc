cmake_minimum_required(VERSION 3.6)
project(osc-bridge CXX)

add_executable(osc-bridge osc-bridge.cpp)
target_include_directories(osc-bridge PRIVATE thirdparty/oscpp/include)
target_compile_features(osc-bridge PUBLIC cxx_std_14)
target_compile_options(osc-bridge PRIVATE -Werror -Wall -Wextra)
