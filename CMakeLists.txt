cmake_minimum_required(VERSION 3.15)

set(CMAKE_C_STANDARD 11 )

project(server C)
add_executable(server server.c config.h player.h world.h world.c server.h)

project(client C)
add_executable(client client.c config.h player.h world.h world.c server.h)

target_link_libraries(server "ncurses" "pthread")
target_link_libraries(client "ncurses" "pthread")

