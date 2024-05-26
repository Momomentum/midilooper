cmake_minimum_required(VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(midilooper)

set(LV2_INSTALL_DIR lib/lv2 CACHE PATH "Install path for lv2 lib")
set(LIB_EXT ".so")

find_package(PkgConfig)
pkg_check_modules(LV2 REQUIRED lv2)

# list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

include(CMakePrintHelpers)

# cmake_print_variables(WAYLAND_CLIENT_LIBRARIES)


set(SRCS
    src/midilooper.cpp
)

add_library(midilooper SHARED ${SRCS})
set_target_properties(midilooper PROPERTIES PREFIX "")

target_include_directories(midilooper PRIVATE ${LV2_INCLUDE_DIRS})
target_link_libraries(midilooper PRIVATE ${LV2_LIBRARIES})

configure_file(manifest.ttl.in 
    manifest.ttl
    @ONLY
)

install(TARGETS midilooper DESTINATION lib/lv2)

# Custom command to generate manifest.ttl
# add_custom_command(TARGET midilooper POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/manifest.ttl ${CMAKE_CURRENT_BINARY_DIR}/manifest.ttl
#     COMMENT "Copying manifest.ttl"
# )
