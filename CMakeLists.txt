# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

add_compile_definitions(LV_CONF_INCLUDE_SIMPLE LV_FONT_MONTSERRAT_12=1)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(sharp_screen_project)
