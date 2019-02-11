cmake_minimum_required(VERSION 3.6)
project(osc-bridge CXX)

add_executable(osc-bridge osc-bridge.cpp)
target_include_directories(osc-bridge PRIVATE thirdparty/oscpp/include)
