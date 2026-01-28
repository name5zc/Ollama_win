cmake_minimum_required(VERSION 3.20)
project(search_agent_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(search_agent_cpp main.cpp)
target_link_libraries(search_agent_cpp PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)
