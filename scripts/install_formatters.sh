#!/bin/bash

echo "Installing External Formatters for Loom AutoFormat Plugin"
echo "========================================================"

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

install_formatters() {
    echo "Detecting system and installing formatters..."
    echo ""
    
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if command_exists apt; then
            echo "Detected Ubuntu/Debian system"
            
            echo "Installing clang-format..."
            sudo apt update
            sudo apt install -y clang-format
            
            echo "Installing Node.js and npm..."
            sudo apt install -y nodejs npm
            
        elif command_exists dnf; then
            echo "Detected Fedora/RHEL system"
            
            echo "Installing clang-format..."
            sudo dnf install -y clang-tools-extra
            
            echo "Installing Node.js and npm..."
            sudo dnf install -y nodejs npm
            
        elif command_exists pacman; then
            echo "Detected Arch Linux system"
            
            echo "Installing clang-format..."
            sudo pacman -S --noconfirm clang
            
            echo "Installing Node.js and npm..."
            sudo pacman -S --noconfirm nodejs npm
        fi
        
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "Detected macOS system"
        
        if command_exists brew; then
            echo "Installing clang-format..."
            brew install clang-format
            
            echo "Installing Node.js..."
            brew install node
        else
            echo "Homebrew not found. Please install Homebrew first:"
            echo "https://brew.sh/"
            exit 1
        fi
    fi
    
    if command_exists npm; then
        echo ""
        echo "Installing npm packages..."
        
        echo "Installing prettier..."
        npm install -g prettier
    else
        echo "npm not found. Please install Node.js and npm first."
        exit 1
    fi
    
    if command_exists pip3; then
        echo ""
        echo "Installing Python packages..."
        
        echo "Installing black..."
        pip3 install black
    elif command_exists pip; then
        echo ""
        echo "Installing Python packages..."
        
        echo "Installing black..."
        pip install black
    else
        echo "pip not found. Please install Python and pip first."
    fi
    
    if command_exists cargo; then
        echo ""
        echo "Installing Rust packages..."
        
        echo "Installing stylua..."
        cargo install stylua
        
        echo "rustfmt should already be available with Rust toolchain"
    else
        echo ""
        echo "Cargo not found. To install Rust formatters:"
        echo "1. Install Rust: https://rustup.rs/"
        echo "2. Run: cargo install stylua"
        echo "3. rustfmt comes with Rust toolchain"
    fi
}

check_installed() {
    echo ""
    echo "Checking installed formatters..."
    echo "================================"
    
    formatters=(
        "clang-format:C/C++"
        "prettier:JavaScript/TypeScript/JSON/HTML/CSS"
        "black:Python"
        "stylua:Lua"
        "rustfmt:Rust"
    )
    
    for formatter_info in "${formatters[@]}"; do
        IFS=':' read -r formatter languages <<< "$formatter_info"
        if command_exists "$formatter"; then
            echo "✓ $formatter ($languages)"
        else
            echo "✗ $formatter ($languages) - not found"
        fi
    done
}
echo "This script will install external formatters required by the AutoFormat plugin:"
echo "- clang-format (C/C++)"
echo "- prettier (JavaScript/TypeScript/JSON/HTML/CSS)"
echo "- black (Python)"
echo "- stylua (Lua)"
echo "- rustfmt (Rust)"
echo ""

check_installed

echo ""
read -p "Do you want to install missing formatters? (y/N): " -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]]; then
    install_formatters
    echo ""
    echo "Installation complete!"
    echo ""
    check_installed
else
    echo "Installation cancelled."
fi

echo ""
echo "Note: You can manually install formatters using:"
echo "- clang-format: Usually comes with clang/LLVM"
echo "- prettier: npm install -g prettier"
echo "- black: pip install black"
echo "- stylua: cargo install stylua"
echo "- rustfmt: Comes with Rust toolchain"