
cmake_minimum_required(VERSION 2.6)

project(MathGameClient)

add_executable(client connectsock.c gameclient.c readwrite.c)
