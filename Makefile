# Makefile for Live Server
# This allows building the live-server independently from the main Loom project

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS_DEBUG = -g -O0 -DDEBUG
CFLAGS_RELEASE = -O3 -DNDEBUG

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Source files
SOURCES = $(SRC_DIR)/server.c $(SRC_DIR)/mongoose.c $(SRC_DIR)/log.c
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Target executable
TARGET = $(BUILD_DIR)/live-server

# Libraries
LIBS = -lpthread -lrt

# Default build type
BUILD_TYPE ?= release

# Set flags based on build type
ifeq ($(BUILD_TYPE),debug)
    CFLAGS += $(CFLAGS_DEBUG)
else
    CFLAGS += $(CFLAGS_RELEASE)
endif

# Default target
.PHONY: all
all: $(TARGET)

# Create build directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Download mongoose files if they don't exist
$(SRC_DIR)/mongoose.c:
	@echo "Downloading mongoose.c..."
	@mkdir -p $(SRC_DIR)
	@curl -s -o $(SRC_DIR)/mongoose.c https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.c

$(INCLUDE_DIR)/mongoose.h:
	@echo "Downloading mongoose.h..."
	@mkdir -p $(INCLUDE_DIR)
	@curl -s -o $(INCLUDE_DIR)/mongoose.h https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.h

# Compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) $(INCLUDE_DIR)/mongoose.h
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Link executable
$(TARGET): $(OBJECTS) | $(BUILD_DIR)
	@echo "Linking $(TARGET)..."
	@$(CC) $(OBJECTS) $(LIBS) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

# Debug build
.PHONY: debug
debug:
	@$(MAKE) BUILD_TYPE=debug

# Release build
.PHONY: release
release:
	@$(MAKE) BUILD_TYPE=release

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)

# Clean everything including downloaded files
.PHONY: distclean
distclean: clean
	@echo "Cleaning downloaded files..."
	@rm -f $(SRC_DIR)/mongoose.c $(INCLUDE_DIR)/mongoose.h

# Install target (optional)
.PHONY: install
install: $(TARGET)
	@echo "Installing live-server to /usr/local/bin..."
	@sudo cp $(TARGET) /usr/local/bin/live-server
	@sudo chmod +x /usr/local/bin/live-server
	@echo "Installation complete"

# Uninstall target (optional)
.PHONY: uninstall
uninstall:
	@echo "Removing live-server from /usr/local/bin..."
	@sudo rm -f /usr/local/bin/live-server
	@echo "Uninstallation complete"

# Test the server
.PHONY: test
test: $(TARGET)
	@echo "Testing live-server..."
	@$(TARGET) --help

# Run the server with default settings
.PHONY: run
run: $(TARGET)
	@echo "Starting live-server on port 8080..."
	@$(TARGET)

# Run the server with custom directory
.PHONY: run-dir
run-dir: $(TARGET)
	@echo "Starting live-server serving $(DIR) on port $(PORT)..."
	@$(TARGET) -d $(DIR) -p $(PORT)

# Show help
.PHONY: help
help:
	@echo "Live Server Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build live-server (default, release mode)"
	@echo "  debug     - Build in debug mode"
	@echo "  release   - Build in release mode"
	@echo "  clean     - Remove build artifacts"
	@echo "  distclean - Remove build artifacts and downloaded files"
	@echo "  install   - Install to /usr/local/bin (requires sudo)"
	@echo "  uninstall - Remove from /usr/local/bin (requires sudo)"
	@echo "  test      - Build and test the executable"
	@echo "  run       - Build and run with default settings"
	@echo "  run-dir   - Build and run with custom DIR and PORT"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Variables:"
	@echo "  BUILD_TYPE - debug or release (default: release)"
	@echo "  DIR        - Directory to serve (for run-dir target)"
	@echo "  PORT       - Port to use (for run-dir target)"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Build release version"
	@echo "  make debug              # Build debug version"
	@echo "  make clean              # Clean build files"
	@echo "  make test               # Build and test"
	@echo "  make run                # Build and run on port 8080"
	@echo "  make run-dir DIR=../test_files PORT=3000"

# Print build information
.PHONY: info
info:
	@echo "Build Information:"
	@echo "  Compiler: $(CC)"
	@echo "  Flags: $(CFLAGS)"
	@echo "  Build Type: $(BUILD_TYPE)"
	@echo "  Source Dir: $(SRC_DIR)"
	@echo "  Build Dir: $(BUILD_DIR)"
	@echo "  Target: $(TARGET)"
	@echo "  Libraries: $(LIBS)"
