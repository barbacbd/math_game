
cmake_minimum_required(VERSION 2.6)

project(MathGameServer)

# Set flags for external sources
set(CMAKE_C_FLAGS "{CMAKE_C_FLAGS} -I/usr/include/mysql -DBIG_JOINS=1 -fno-strict-aliasing -g")

# Set other external libraries:
# MySQL Client
# PTHREADS
set(OTHER_LIBS "-lmysqlclient -lpthread -lz -lm -lrt -ldl")

add_executable(server passivesock.c gameserver.c queryFile.c readwrite.c)
