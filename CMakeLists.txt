cmake_minimum_required(VERSION 3.15)

set(CMAKE_C_STANDARD 11 )

project(server C)
add_executable(server server.c config.h player.h world.h world.c server.h beasts.h player.c beasts.c)

project(client C)
add_executable(client client.c config.h)

project(beasts C)
add_executable(beasts config.h beasts_manager.c)


target_link_libraries(server "ncurses" "pthread")
target_link_libraries(client "ncurses" "pthread")
target_link_libraries(beasts "ncurses" "pthread")


