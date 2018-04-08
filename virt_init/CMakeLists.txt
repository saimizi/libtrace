cmake_minimum_required (VERSION 2.8)

project(virt_init)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")

add_executable(virt_init virt_init.c)

set (CMAKE_INSTALL_PREFIX "/")
install (TARGETS virt_init DESTINATION usr/sbin)

