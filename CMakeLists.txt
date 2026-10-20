cmake_minimum_required(VERSION 3.31)

project(IDKGameEngine VERSION 0.0.1 LANGUAGES C CXX)
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")
include(GNUInstallDirs)
include(CTest)

set(IDK_APP_NAME "game" CACHE STRING "Name of the optional game executable")
set(IDK_LIBIDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libidk" CACHE PATH "Path to libidk")
set(IDK_GAME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../idk_game" CACHE PATH "Path to the optional game")
set(IDK_CONTENT_BASEPATH "content" CACHE STRING "Installed content directory")

if(NOT EXISTS "${IDK_LIBIDK_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "libidk was not found at ${IDK_LIBIDK_DIR}. Set IDK_LIBIDK_DIR.")
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/CMakeLists.txt")
include_directories("${CMAKE_PREFIX_PATH}/include")
add_compile_definitions(IDK_CONTENT_BASEPATH="${IDK_CONTENT_BASEPATH}")
add_compile_options(-Werror -Wall -Wextra)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(IDK_DEBUG="1")
endif()

add_subdirectory("${IDK_LIBIDK_DIR}" "${CMAKE_BINARY_DIR}/libidk")

configure_file(InstallConfig.cmake.in InstallConfig.cmake @ONLY)
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/InstallConfig.cmake")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/content/" DESTINATION "${IDK_CONTENT_BASEPATH}")

add_subdirectory(asset)
add_subdirectory(common)
add_subdirectory(editor)
add_subdirectory(mapedit)
add_subdirectory(level)
add_subdirectory(engine)
add_subdirectory(gameplay)
add_subdirectory(gfx)

add_library(idkengine STATIC)
target_link_libraries(idkengine PUBLIC idkengine_common idkengine_gfx idkengine_engine)

if(EXISTS "${IDK_GAME_DIR}/CMakeLists.txt")
    set(IDK_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    add_subdirectory("${IDK_GAME_DIR}" "${CMAKE_BINARY_DIR}/game")
    add_dependencies(mapedit ${IDK_APP_NAME})
else()
    message(STATUS "Optional game not found at ${IDK_GAME_DIR}; editor and map editor remain available")
endif()

install(IMPORTED_RUNTIME_ARTIFACTS Steamworks::steam_api
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
