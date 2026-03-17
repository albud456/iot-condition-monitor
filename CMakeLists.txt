# Root CMakeLists.txt for ESP-IDF project

cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Point this to your AWS SDK component directory
#set(EXTRA_COMPONENT_DIRS "esp-aws-iot")

# Optional: include minimal build (only main + dependencies)
idf_build_set_property(MINIMAL_BUILD ON)

set(CMAKE_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/components/esp-aws-iot/port/include")

# Optional: include other custom components
# set(EXTRA_COMPONENT_DIRS "${EXTRA_COMPONENT_DIRS}" "components/bme680")

# Declare the project
project(iot-condition-monitor)
