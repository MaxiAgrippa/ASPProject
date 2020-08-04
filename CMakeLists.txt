cmake_minimum_required(VERSION 3.16)
project(ASPProject C)

set(CMAKE_C_STANDARD 99)

add_executable(main src/main.c)

add_executable(Server src/S/Server.c)

add_executable(Client src/C/Client.c)

add_executable(unbind src/unbind.c)