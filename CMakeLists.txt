cmake_minimum_required(VERSION 3.15)

project(simple-module-scanner LANGUAGES CXX VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(lyra)

add_executable(sms src/sms.cpp)
target_link_libraries(sms PRIVATE lyra)
target_compile_options(sms PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8 /permissive- /diagnostics:caret>)
