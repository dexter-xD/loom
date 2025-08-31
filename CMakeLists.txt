cmake_minimum_required(VERSION 3.16)

# project configuration
project(loom VERSION 1.0.0 LANGUAGES CXX)

# set c++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# linux compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

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
install(DIRECTORY config/ DESTINATION share/${PROJECT_NAME}/config)
install(DIRECTORY themes/ DESTINATION share/${PROJECT_NAME}/themes)
install(DIRECTORY plugins/ DESTINATION share/${PROJECT_NAME}/plugins)

# build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Qt version: ${Qt5_VERSION}")