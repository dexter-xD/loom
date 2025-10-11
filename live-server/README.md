# Live Server

A lightweight HTTP server with hot reload functionality for web development, similar to VS Code's Live Server extension.

## Features

- **Hot Reload**: Automatically reloads the browser when HTML, CSS, or JS files change
- **WebSocket Integration**: Real-time communication for instant updates
- **File Watching**: Uses inotify for efficient file system monitoring
- **Lightweight**: Built with C and Mongoose web server library
- **Cross-platform**: Works on Linux and other Unix-like systems

## Building

### Standalone Build (using Makefile)

```bash
# Build release version (recommended)
make

# Build debug version
make debug

# Clean build files
make clean

# Show all available targets
make help
```

### Integrated Build (with Loom text editor)

The live-server is automatically built when building the main Loom project:

```bash
# From the root directory
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
make -C build -j$(nproc)
```

## Usage

### Command Line Options

```bash
# Default - serve ./public on port 8080
./build/live-server

# Serve a specific directory
./build/live-server -d /var/www
./build/live-server --dir ./dist

# Change port
./build/live-server -p 3000
./build/live-server --port 5000

# Combine options
./build/live-server -d ./my-site -p 3000

# Quick serve - just pass directory or file
./build/live-server ./my-website
./build/live-server index.html    # Serves current directory

# Logging options
./build/live-server -v          # Verbose logging (debug mode)
./build/live-server --verbose   # Shows file watching and WebSocket events

./build/live-server -q          # Quiet mode (errors only)
./build/live-server --quiet     # Minimal output

# Help
./build/live-server -h
./build/live-server --help
```

### Makefile Shortcuts

```bash
# Build and run with default settings (port 8080, ./public)
make run

# Build and run with custom directory and port
make run-dir DIR=../test_files PORT=3000

# Build and test the executable
make test
```

## Integration with Loom Text Editor

When used with the Loom text editor, the live-server provides seamless integration:

1. **Menu Integration**: Start/stop server from Tools menu
2. **Smart Directory Detection**: Automatically serves the current file's directory
3. **Browser Integration**: One-click browser opening
4. **Status Updates**: Real-time feedback in the editor's status bar

### Configuration in Loom

Edit `config/config.lua`:

```lua
live_server = {
    enabled = true,
    port = "8080",
    auto_open_browser = false,
    default_directory = "",
    executable_path = "",
    log_level = "quiet"  -- "quiet", "normal", or "verbose"
}
```

**Log Level Options:**
- `"quiet"` - Only show errors (default, recommended for production)
- `"normal"` - Standard server output (startup messages, basic info)
- `"verbose"` - Detailed debug output (file watching, WebSocket events)

## How It Works

1. **File Monitoring**: Uses Linux inotify to watch for file changes
2. **WebSocket Connection**: Injects a WebSocket client script into HTML pages
3. **Hot Reload**: When files change, sends reload command to connected browsers
4. **Static Serving**: Serves static files with proper MIME types

## Dependencies

- **System**: Linux with inotify support
- **Libraries**: pthread (usually available by default)
- **Network**: Mongoose web server library (automatically downloaded)

## Installation

### System-wide Installation

```bash
# Install to /usr/local/bin
make install

# Uninstall
make uninstall
```

### Local Installation

The executable is built in `build/live-server` and can be used directly or copied anywhere in your PATH.

## Development

### Debug Build

```bash
# Build with debug symbols and verbose logging
make debug

# Run debug version
./build/live-server -d ./test_files
```

### Clean Build

```bash
# Remove build artifacts
make clean

# Remove everything including downloaded mongoose files
make distclean
```

## Troubleshooting

### Common Issues

1. **Permission Denied**: Make sure the executable has execute permissions
   ```bash
   chmod +x build/live-server
   ```

2. **Port Already in Use**: Change the port with `-p` option
   ```bash
   ./build/live-server -p 3001
   ```

3. **Files Not Reloading**: Ensure you're editing files in the served directory

### Verbose Output

For debugging, use verbose logging:
```bash
# Enable verbose logging to see file watching and WebSocket events
./build/live-server -v -d ./test_files

# Or build in debug mode for even more detailed output
make debug
./build/live-server -d ./test_files

# Quiet mode for minimal output
./build/live-server -q -d ./test_files
```

## License

This project is part of the Loom text editor and follows the same MIT license.