project(game_msgs)

# necessary for python builds
cmake_minimum_required(VERSION 3.4)


###############################################################################
#
# Try to use C++11
#
###############################################################################
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()


# set the path where install files will reside
set(CMAKE_INSTALL_PREFIX "/usr")


###############################################################################################################################
#
# create the temp library for the proto messages
#
###############################################################################################################################
set(PROTO_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/round.proto
	${CMAKE_CURRENT_SOURCE_DIR}/stats.proto
	${CMAKE_CURRENT_SOURCE_DIR}/user.proto
)

set(ProtobufIncludePath ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "Path to generated protobuf files for the simple game.")

find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

set_source_files_properties(${PROTO_SRCS} PROPERTIES COMPILE_FLAGS "-fPIC")

add_library(game_msgs_proto OBJECT ${PROTO_SRCS} ${PROTO_HDRS})

SET_PROPERTY(GLOBAL PROPERTY PROTO_SOURCES ${PROTO_SRCS})
SET_PROPERTY(GLOBAL PROPERTY PROTO_HEADERS ${PROTO_HDRS})

install(FILES ${PROTO_HDRS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/game/msgs)
###############################################################################################################################


###############################################################################################################################
set(BINDIR "${CMAKE_INSTALL_PREFIX}/bin")

if(EXISTS ${CMAKE_INSTALL_PREFIX}/lib64)
	  set(LIBDIR "${CMAKE_INSTALL_PREFIX}/lib64")
elseif(EXISTS ${CMAKE_INSTALL_PREFIX}/lib)
	  set(LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
endif()

include_directories(${CMAKE_INSTALL_PREFIX}/include)
link_directories(${LIBDIR})
###############################################################################################################################

add_library(game_msgs SHARED  $<TARGET_OBJECTS:game_msgs_proto>)


install( TARGETS game_msgs
	    	RUNTIME DESTINATION ${BINDIR}
                LIBRARY DESTINATION ${LIBDIR}
                ARCHIVE DESTINATION ${LIBDIR})

###############################################################################################################################
#
# Section: Python Module building
# Let's build the python module so that we can use these protobuf files with different versions of the software
#
#
#
###############################################################################################################################

PROTOBUF_GENERATE_PYTHON(PYTHON_PROTO_FILES ${PROTO_FILES})
set(PYTHON_SOURCES ${PYTHON_PROTO_FILES})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/install_module.sh.in ${CMAKE_BINARY_DIR}/python/install_module.sh)

# Move the blank init file into all subdirectories so that python knows that it is a submodule
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/__init__.py.in ${CMAKE_BINARY_DIR}/python/game/__init__.py)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/__init__.py.in ${CMAKE_BINARY_DIR}/python/game/msgs/__init__.py)

# setup.py defines the module
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in ${CMAKE_BINARY_DIR}/python/setup.py)

add_library(game_msgs_python SHARED ${PYTHON_PROTO_FILES})
set_target_properties(game_msgs_python PROPERTIES LINKER_LANGUAGE C)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/python/game/msgs/")

# finally, move all of the files we generated into this directory
add_custom_command(TARGET game_msgs_python PRE_BUILD COMMAND "mv" ARGS "${CMAKE_CURRENT_BINARY_DIR}/*.py" "${CMAKE_BINARY_DIR}/python/game/msgs/")

# execute the installation script that we just installed
install(CODE "execute_process(WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/python COMMAND ./install_module.sh)")


###############################################################################################################################
#
# Section: JAva Module building
# Let's build the java package so that we can use these protobuf files with different versions of the software
#
#
#
###############################################################################################################################



# Install a cmake file that will be used for finding this library 
# amond any other project no matter what system it si installed on
#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/RADSVisMsgsConfig.cmake DESTINATION /usr/local/share/RADSVisMsgs)