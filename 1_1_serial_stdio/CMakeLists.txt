cmake_minimum_required(VERSION 3.16.0)

if(DEFINED ENV{IDF_PATH})
    include($ENV{IDF_PATH}/tools/cmake/project.cmake)
    project(lab11_serial_stdio)
else()
    message("IDF_PATH not set, this file is only for ESP-IDF native builds.")
endif()