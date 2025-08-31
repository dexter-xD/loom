<div align="center">
  <img src="assets/icon.png" alt="Rhythm Logo" width="128" height="128">

# Loom

**A fast, minimal, and customizable text editor.**

Built with C++ and Qt, featuring Lua scripting for configuration and plugins

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![License](https://img.shields.io/badge/license-MIT-blue)]()
[![Qt Version](https://img.shields.io/badge/Qt-5.x-green)]()
[![C++ Standard](https://img.shields.io/badge/C++-17-blue)]()
[![Lua](https://img.shields.io/badge/Lua-5.1-lightblue.svg)]()

![Loom Banner](banner.png)

</div>

## Features

- **Lightweight & Fast** - Minimal resource usage with responsive text editing
- **Gruvbox Theme** - Beautiful, consistent dark theme for comfortable coding
- **Syntax Highlighting** - Support for multiple programming languages
- **Multi-Tab Interface** - Manage multiple files with tabbed interface
- **Customizable Keybindings** - Configure shortcuts to match your workflow
- **Line Numbers** - Optional line number display with relative numbering
- **Find & Replace** - Built-in search and replace functionality

### Plugin System
- **Extensible Architecture** - Lua-based plugin system for unlimited customization
- **AutoSave Plugin** - Automatic file saving with configurable intervals
- **AutoFormat Plugin** - Code formatting with external formatters (clang-format, prettier, black, stylua, rustfmt)
- **Plugin Management** - Easy loading, configuration, and error recovery

> **⚠️ <span style="color:red">IMPORTANT:</span>** The AutoFormat plugin requires external formatting tools to be installed on your system! Run `./scripts/install_formatters.sh` to install all required formatters automatically.

### Configuration System
- **Lua-Based Config** - Powerful configuration through `config/config.lua`
- **Runtime Configuration** - Change settings without restarting the editor
- **Plugin Configuration** - Individual plugin settings and toggles
- **Keybinding Customization** - Define custom keyboard shortcuts
- **Editor Settings** - Font, theme, tab width, line numbers, and more

## Quick Start

### Prerequisites

Before building Loom, ensure you have the following installed:

- **CMake** 3.16 or higher
- **Qt5** development libraries
- **Lua** 5.3 or 5.4 development libraries
- **C++17** compatible compiler (GCC, Clang, or MSVC)

### Installation

#### Linux (Ubuntu/Debian)
```bash
# Install dependencies
sudo apt update
sudo apt install cmake qt5-default qtbase5-dev liblua5.4-dev build-essential

# Clone the repository
git clone https://github.com/your-username/loom.git
cd loom

# Build using the provided script
chmod +x scripts/build_release.sh
./scripts/build_release.sh

# Run Loom
./scripts/run_loom.sh
```

#### Linux (Fedora/RHEL)
```bash
# Install dependencies
sudo dnf install cmake qt5-qtbase-devel lua-devel gcc-c++

# Clone and build
git clone https://github.com/your-username/loom.git
cd loom
chmod +x scripts/build_release.sh
./scripts/build_release.sh
```

#### macOS
```bash
# Install dependencies with Homebrew
brew install cmake qt@5 lua

# Clone and build
git clone https://github.com/your-username/loom.git
cd loom
chmod +x scripts/build_release.sh
./scripts/build_release.sh
```

#### Windows
1. Install Qt5 from the [official installer](https://www.qt.io/download)
2. Install Lua development libraries
3. Use Visual Studio or MinGW for compilation
4. Follow the manual build instructions below

### Build Options

#### Using Build Scripts (Recommended)

**Release Build (Optimized)**
```bash
./scripts/build_release.sh
```

**Debug Build (Development)**
```bash
./scripts/build_debug.sh
```

**Running Loom**
```bash
# Recommended way (suppresses Qt warnings)
./scripts/run_loom.sh [optional-file-path]

# Direct execution
./build/loom [optional-file-path]
```

#### Manual Build

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build the project
cmake --build . -j$(nproc)

# Run the editor
./loom
```

## Important: External Formatters for AutoFormat Plugin

**The AutoFormat plugin requires external formatting tools to be installed on your system.** The plugin uses these external formatters to provide professional code formatting:

- **clang-format** - For C/C++ code formatting
- **prettier** - For JavaScript, TypeScript, JSON, HTML, CSS formatting  
- **black** - For Python code formatting
- **stylua** - For Lua code formatting
- **rustfmt** - For Rust code formatting (comes with Rust toolchain)

### Quick Formatter Installation

I provide an automated installation script for all required formatters:

```bash
# Make the script executable and run it
chmod +x scripts/install_formatters.sh
./scripts/install_formatters.sh
```

The script will:
- Detect your operating system (Linux, macOS)
- Install system packages (clang-format, Node.js, Python)
- Install npm packages (prettier)
- Install Python packages (black)
- Install Rust packages (stylua) if Cargo is available
- Show you which formatters are installed and which are missing

### Manual Installation

If you prefer manual installation:

```bash
# Ubuntu/Debian
sudo apt install clang-format nodejs npm python3-pip
npm install -g prettier
pip3 install black
cargo install stylua  # if you have Rust installed

# macOS (with Homebrew)
brew install clang-format node python
npm install -g prettier
pip3 install black
cargo install stylua  # if you have Rust installed

# Fedora/RHEL
sudo dnf install clang-tools-extra nodejs npm python3-pip
npm install -g prettier
pip3 install black
cargo install stylua  # if you have Rust installed
```

**Note:** The AutoFormat plugin will automatically detect which formatters are available and only format files for languages where the corresponding formatter is installed.

## Configuration System

Loom uses a powerful Lua-based configuration system that allows you to customize every aspect of the editor.

### Configuration Structure

```
config/
└── config.lua          # Main configuration file
plugins/
├── autoformat.lua      # Auto-formatting plugin
└── autosave.lua       # Auto-save plugin
```

### Main Configuration (`config/config.lua`)

The main configuration file controls editor behavior, appearance, and keybindings:

```lua
config = {
    -- Editor Settings
    editor = {
        font_family = "JetBrains Mono",
        font_size = 12,
        tab_width = 8,
        show_line_numbers = true,
        word_wrap = true,
        auto_indent = true,
        highlight_current_line = true
    },
    
    -- Window Settings
    window = {
        width = 1224,
        height = 768,
        remember_size = true,
        remember_position = true
    },
    
    -- Keybindings
    keybindings = {
        ["Ctrl+S"] = "save_file",
        ["Ctrl+O"] = "open_file",
        ["Ctrl+N"] = "new_file",
        ["Ctrl+T"] = "new_tab",
        ["Ctrl+W"] = "close_file",
        ["Ctrl+Q"] = "quit_application",
        ["F11"] = "toggle_fullscreen",
        -- Add your custom keybindings here
    },
    
    -- Plugin Configuration
    plugins = {
        enabled = true,
        auto_load = true,
        error_recovery = true,
        
        -- Individual plugin settings
        autosave = {
            enabled = false,
            interval = 10, -- seconds
            save_on_focus_lost = false
        },
        
        autoformat = {
            enabled = true,
            format_on_save = true,
            use_external_formatters = true
        }
    }
}
```

### Configuration Functions

The configuration system provides helper functions:

```lua
-- Get configuration value with fallback
local font_size = get_config("editor.font_size", 12)

-- Set configuration value
set_config("editor.font_size", 14)
```

## Plugin System

Loom features a robust plugin system powered by Lua scripting. Plugins can extend functionality, add new features, and integrate with external tools.

### Available Plugins

#### AutoSave Plugin (`plugins/autosave.lua`)

Automatically saves your files at regular intervals to prevent data loss.

**Features:**
- Configurable save interval (default: 30 seconds)
- Smart saving (only saves when content changes)
- Status bar integration
- Save counter and timestamps

**Configuration:**
```lua
plugins = {
    autosave = {
        enabled = true,           -- Enable/disable autosave
        interval = 30,           -- Save interval in seconds
        save_on_focus_lost = false, -- Save when window loses focus
        backup_files = false     -- Create backup files
    }
}
```

**Usage:**
- Automatically runs in the background when enabled
- View status: Check status bar for save count and last save time
- Toggle: Use Lua console to call `autosave.toggle()`

#### AutoFormat Plugin (`plugins/autoformat.lua`)

Automatically formats your code using external formatters for consistent code style.

**Supported Languages & Formatters:**
- **C/C++**: `clang-format` (Google style)
- **JavaScript/TypeScript**: `prettier`
- **JSON**: `prettier`
- **HTML/CSS**: `prettier`
- **Lua**: `stylua`
- **Python**: `black`
- **Rust**: `rustfmt`

**Features:**
- Format on save (configurable)
- Manual formatting command
- External formatter integration
- Language auto-detection
- Cursor position preservation

**Configuration:**
```lua
plugins = {
    autoformat = {
        enabled = true,                    -- Enable/disable plugin
        format_on_save = true,            -- Format when saving files
        use_external_formatters = true    -- Use external tools
    }
}
```

**Required External Tools:**
```bash
# Install formatters as needed
npm install -g prettier          # JavaScript/TypeScript/JSON/HTML/CSS
pip install black               # Python
cargo install stylua           # Lua
# clang-format usually comes with clang
# rustfmt comes with Rust toolchain
```

**Usage:**
- Automatic: Enable `format_on_save` in configuration
- Manual: Call `autoformat.format_document()` from Lua console
- Status: Use `autoformat.show_status()` to see available formatters

### Plugin Management

#### Loading Plugins

Plugins are automatically loaded from the `plugins/` directory on startup. The plugin manager:

- Scans for `.lua` files in the plugins directory
- Validates plugin structure
- Initializes plugins with error recovery
- Provides status reporting

#### Plugin Configuration

Each plugin can be configured in the main `config.lua` file:

```lua
plugins = {
    enabled = true,        -- Global plugin system toggle
    auto_load = true,      -- Load plugins on startup
    error_recovery = true, -- Continue loading other plugins if one fails
    
    -- Plugin-specific settings
    plugin_name = {
        enabled = true,
        -- plugin-specific options
    }
}
```

#### Creating Custom Plugins

Create a new `.lua` file in the `plugins/` directory:

```lua
-- my_plugin.lua
my_plugin = {
    name = "my_plugin",
    version = "1.0",
    description = "My custom plugin",
    enabled = true
}

-- Initialize plugin
function my_plugin.initialize()
    print("My plugin initialized!")
    
    -- Register event handlers
    events.connect("file_saved", "my_plugin.on_file_saved")
end

-- Cleanup plugin
function my_plugin.cleanup()
    print("My plugin cleaned up!")
end

-- Event handler
function my_plugin.on_file_saved(event_name, file_path)
    print("File saved: " .. file_path)
end

-- Make event handler globally accessible
_G["my_plugin.on_file_saved"] = my_plugin.on_file_saved

print("My plugin loaded!")
```

## Theming

Loom uses the beautiful Gruvbox color scheme, providing a comfortable dark theme optimized for long coding sessions.

### Gruvbox Color Palette

The theme implements the complete Gruvbox palette:

- **Background**: Dark variants (#282828, #1d2021, #3c3836)
- **Foreground**: Light variants (#ebdbb2, #fbf1c7, #d5c4a1)
- **Accents**: Red, Green, Yellow, Blue, Purple, Aqua, Orange
- **Syntax**: Carefully chosen colors for optimal readability

### Theme Customization

The theme is defined in `themes/gruvbox.qss` using Qt stylesheets. You can customize:

- Color values
- Font settings (though fonts are controlled by configuration)
- UI element styling
- Tab appearance
- Scrollbar design

## Keybindings

### Default Keybindings

| Shortcut | Action |
|----------|--------|
| `Ctrl+N` | New file |
| `Ctrl+O` | Open file |
| `Ctrl+S` | Save file |
| `Ctrl+T` | New tab |
| `Ctrl+W` | Close current file |
| `Ctrl+Q` | Quit application |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `Ctrl+C` | Copy |
| `Ctrl+V` | Paste |
| `Ctrl+X` | Cut |
| `Ctrl+A` | Select all |
| `Ctrl+F` | Find |
| `Ctrl+H` | Replace |
| `F11` | Toggle fullscreen |
| `Ctrl+L` | Set language |
| `Ctrl+Shift+L` | Redetect language |

### Custom Keybindings

Add custom keybindings in `config/config.lua`:

```lua
keybindings = {
    ["Ctrl+Shift+F"] = "format_document",
    ["Ctrl+/"] = "toggle_comment",
    ["Ctrl+D"] = "duplicate_line",
    -- Add your custom bindings here
}
```

## Development

### Debug Build

For development and debugging:

```bash
./scripts/build_debug.sh
```

Debug builds include:
- Debug symbols for debugging
- Verbose logging output
- Development-friendly error messages
- Unoptimized code for easier debugging

### Logging

Loom includes a comprehensive logging system:

```cpp
// In C++ code
LOG_ERROR("Error message");
DEBUG_LOG("Debug message");

// In Lua plugins
editor.debug_log("Debug message from Lua")
```

### Testing

Test your changes with the provided sample files in `test_files/`:

```bash
# Test with different file types
./scripts/run_loom.sh test_files/sample.cpp
./scripts/run_loom.sh test_files/sample.js
./scripts/run_loom.sh test_files/sample.py
./scripts/run_loom.sh test_files/sample.lua
```

## Contributing

I welcome contributions! Here's how you can help:

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Commit** your changes (`git commit -m 'Add amazing feature'`)
4. **Push** to the branch (`git push origin feature/amazing-feature`)
5. **Open** a Pull Request

### Contribution Guidelines

- Follow the existing code style
- Add tests for new features
- Update documentation as needed
- Ensure all builds pass
- Test on multiple platforms when possible

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- **Textadept** - Inspiration for the minimal editor design
- **Gruvbox** - Beautiful color scheme
- **Qt Framework** - Cross-platform GUI toolkit
- **Lua** - Powerful scripting language
- **Contributors** - Everyone who helps improve Loom

## Support

- **Issues**: [GitHub Issues](https://github.com/dexter-xd/loom/issues)
- **Discussions**: [Join our Discord Server](https://discord.gg/GnDnjPe9)

---

<div align="center">

⭐ [Star this project](https://github.com/dexter-xd/loom) if you find it useful!

</div>