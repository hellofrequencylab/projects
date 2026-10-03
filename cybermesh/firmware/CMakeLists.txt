cmake_minimum_required(VERSION 3.22)
# The portable floor core lives in ../core and is shared with the host tests.
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../core")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cybermesh)
