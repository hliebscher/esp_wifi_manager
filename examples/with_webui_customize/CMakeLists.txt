# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html

cmake_minimum_required(VERSION 3.16)

# Include component from parent directory
set(EXTRA_COMPONENT_DIRS "../../")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wifi_manager_webui_customize)

# Create SPIFFS partition image from www directory
spiffs_create_partition_image(storage www FLASH_IN_PROJECT)
