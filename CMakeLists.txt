cmake_minimum_required(VERSION 3.10)
project(client_server_task)

set(CMAKE_CXX_STANDARD 17)
find_package(Threads)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/logins.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

add_executable(server server.cpp DataBase.cpp DataBase.h MessageQueue.cpp MessageQueue.h)
add_executable(client client.cpp)
target_link_libraries (server Threads::Threads)
target_link_libraries (client Threads::Threads)