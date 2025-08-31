cmake_minimum_required(VERSION 3.16)

# project configuration
project(loom VERSION 1.0.0 LANGUAGES CXX)

# set c++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# linux compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# ensure NDEBUG is set for release builds to disable debug logs
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DNDEBUG)
endif()

# find required packages
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Lua REQUIRED)

# enable qt moc for Qt5
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# source files
set(SOURCES
    src/main.cpp
    src/editor_window.cpp
    src/buffer.cpp
    src/lua_bridge.cpp
    src/syntax_highlighter.cpp
    src/syntax_rules.cpp
    src/plugin_manager.cpp
    src/code_editor.cpp
    src/line_number_area.cpp
)

# header files (needed for MOC processing)
set(HEADERS
    include/editor_window.h
    include/buffer.h
    include/lua_bridge.h
    include/syntax_highlighter.h
    include/syntax_rules.h
    include/plugin_manager.h
    include/code_editor.h
    include/line_number_area.h
)

include_directories(include)

# create resource file for Qt5
set(RESOURCE_FILES themes/gruvbox.qss)
qt5_add_resources(RESOURCES themes.qrc)

# create executable target
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

# link qt libraries and lua
target_link_libraries(${PROJECT_NAME} 
    Qt5::Core 
    Qt5::Widgets
    ${LUA_LIBRARIES}
)

# include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${LUA_INCLUDE_DIR}
)

# copy config and plugins folders to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/config ${CMAKE_BINARY_DIR}/config
    COMMENT "Copying config directory to build folder"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/plugins ${CMAKE_BINARY_DIR}/plugins
    COMMENT "Copying plugins directory to build folder"
)

# installation settings
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

# Install config and plugins directories with proper permissions
install(DIRECTORY config/ 
    DESTINATION share/${PROJECT_NAME}/config
    FILES_MATCHING PATTERN "*.lua"
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

install(DIRECTORY plugins/ 
    DESTINATION share/${PROJECT_NAME}/plugins
    FILES_MATCHING PATTERN "*.lua"
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

# Install themes directory
install(DIRECTORY themes/ 
    DESTINATION share/${PROJECT_NAME}/themes
    FILES_MATCHING PATTERN "*.qss"
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

# Install assets and documentation
install(FILES assets/icon.png DESTINATION share/${PROJECT_NAME}/assets)
install(FILES README.md DESTINATION share/doc/${PROJECT_NAME})

# Create desktop entry
configure_file(
    ${CMAKE_SOURCE_DIR}/loom.desktop.in
    ${CMAKE_BINARY_DIR}/loom.desktop
    @ONLY
)
install(FILES ${CMAKE_BINARY_DIR}/loom.desktop DESTINATION share/applications)
install(FILES assets/icon.png DESTINATION share/pixmaps RENAME loom.png)

# Install man page if it exists
if(EXISTS ${CMAKE_SOURCE_DIR}/loom.1)
    install(FILES loom.1 DESTINATION share/man/man1)
endif()

# CPack configuration for DEB packaging
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "loom")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A fast, minimal, and customizable text editor")
set(CPACK_PACKAGE_DESCRIPTION "Loom is a fast, minimal, and customizable text editor built with C++ and Qt, featuring Lua scripting for configuration and plugins. It includes syntax highlighting, multi-tab interface, customizable keybindings, and an extensible plugin system.")
set(CPACK_PACKAGE_MAINTAINER "dexter-xd <dexter-xd@users.noreply.github.com>")
set(CPACK_PACKAGE_CONTACT "dexter-xd <dexter-xd@users.noreply.github.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/dexter-xd/loom")
set(CPACK_PACKAGE_SECTION "editors")
set(CPACK_PACKAGE_PRIORITY "optional")

# DEB specific settings
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a (>= 5.12), libqt5widgets5 (>= 5.12), libqt5gui5 (>= 5.12), liblua5.4-0 | liblua5.3-0")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "clang-format, prettier, python3-black, nodejs, npm")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "stylua, rustfmt")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/debian/postinst;${CMAKE_SOURCE_DIR}/debian/postrm")

# Package file name format: loom_1.0.0_amd64.deb
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")

# Include CPack
include(CPack)

# build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Qt version: ${Qt5_VERSION}")
message(STATUS "Package version: ${PROJECT_VERSION}")