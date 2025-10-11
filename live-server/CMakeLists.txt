enable_language(C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags for live-server
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Source files
set(LIVE_SERVER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/mongoose.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/log.c
)

# Create executable with full path specification
add_executable(live-server ${LIVE_SERVER_SOURCES})

# Explicitly set the language for the target and disable Qt auto-generation
set_target_properties(live-server PROPERTIES
    LINKER_LANGUAGE C
    AUTOMOC OFF
    AUTORCC OFF
    AUTOUIC OFF
)

# Link libraries (pthread for threading support)
target_link_libraries(live-server pthread)

# Copy the executable to the main build directory after building
add_custom_command(TARGET live-server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:live-server> ${CMAKE_BINARY_DIR}/live-server-bin
    COMMENT "Copying live-server to main build directory"
)

# Install target
install(TARGETS live-server DESTINATION bin)

# Option to download mongoose if not present
option(DOWNLOAD_MONGOOSE "Download mongoose.c and mongoose.h if not present" ON)

if(DOWNLOAD_MONGOOSE)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/mongoose.c")
        message(STATUS "Downloading mongoose.c to src/...")
        file(DOWNLOAD
            "https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/mongoose.c"
            SHOW_PROGRESS
        )
    endif()
    
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/mongoose.h")
        message(STATUS "Downloading mongoose.h to include/...")
        file(DOWNLOAD
            "https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/include/mongoose.h"
            SHOW_PROGRESS
        )
    endif()
endif()

# Create public directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/public)

# Create include and src directories if they don't exist
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C Flags: ${CMAKE_C_FLAGS}")
message(STATUS "Source directory: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Binary directory: ${CMAKE_CURRENT_BINARY_DIR}")