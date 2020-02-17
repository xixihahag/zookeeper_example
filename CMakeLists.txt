cmake_minimum_required (VERSION 2.8)
project(zookeeper)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -ggdb -fPIC -pthread")

include_directories(/usr/local/zookeeper3.3.6/include/c-client-src)
link_directories(/usr/local/zookeeper3.3.6/lib)

add_executable(server server.cc)
target_link_libraries(server zookeeper_mt)

add_executable(client client.cc)
target_link_libraries(client zookeeper_mt)