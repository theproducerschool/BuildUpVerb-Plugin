#!/bin/bash

# BuildUp Reverb - Build and Install Script
# This script builds the plugin and installs it to the system plugin directories

echo "==================================="
echo "BuildUp Reverb - Build & Install"
echo "==================================="

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the script's directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build"

# Function to print colored output
print_status() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}!${NC} $1"
}

# Check if BUILD_NOTES.md exists
if [ -f "$SCRIPT_DIR/BUILD_NOTES.md" ]; then
    echo ""
    echo "Recent changes from BUILD_NOTES.md:"
    echo "-----------------------------------"
    # Extract recent changes section
    awk '/### Recent Changes/,/### Known Issues/' "$SCRIPT_DIR/BUILD_NOTES.md" | sed '1d;$d'
    echo "-----------------------------------"
    echo ""
    read -p "Have you updated BUILD_NOTES.md with your changes? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        print_warning "Please update BUILD_NOTES.md before building!"
        echo "Opening BUILD_NOTES.md..."
        open "$SCRIPT_DIR/BUILD_NOTES.md"
        exit 1
    fi
else
    print_error "BUILD_NOTES.md not found!"
    exit 1
fi

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    print_status "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

# Navigate to build directory
cd "$BUILD_DIR"

# Configure with CMake
echo ""
print_status "Configuring with CMake..."
if cmake -DCMAKE_BUILD_TYPE=Release ..; then
    print_status "CMake configuration successful"
else
    print_error "CMake configuration failed"
    exit 1
fi

# Build the project
echo ""
print_status "Building plugin..."
if cmake --build . --config Release; then
    print_status "Build successful"
else
    print_error "Build failed"
    exit 1
fi

# Install VST3
echo ""
print_status "Installing VST3 plugin..."
VST3_SOURCE="$BUILD_DIR/BuildUpVerb_artefacts/Release/VST3/BuildUp Reverb.vst3"
VST3_DEST="$HOME/Library/Audio/Plug-Ins/VST3/"

if [ -d "$VST3_SOURCE" ]; then
    # Backup existing plugin if it exists
    if [ -d "$VST3_DEST/BuildUp Reverb.vst3" ]; then
        print_warning "Backing up existing VST3 plugin..."
        mv "$VST3_DEST/BuildUp Reverb.vst3" "$VST3_DEST/BuildUp Reverb.vst3.backup"
    fi
    
    cp -R "$VST3_SOURCE" "$VST3_DEST"
    print_status "VST3 plugin installed to: $VST3_DEST"
else
    print_error "VST3 build not found at: $VST3_SOURCE"
fi

# Install AU
echo ""
print_status "Installing AU plugin..."
AU_SOURCE="$BUILD_DIR/BuildUpVerb_artefacts/Release/AU/BuildUp Reverb.component"
AU_DEST="$HOME/Library/Audio/Plug-Ins/Components/"

if [ -d "$AU_SOURCE" ]; then
    # Backup existing plugin if it exists
    if [ -d "$AU_DEST/BuildUp Reverb.component" ]; then
        print_warning "Backing up existing AU plugin..."
        mv "$AU_DEST/BuildUp Reverb.component" "$AU_DEST/BuildUp Reverb.component.backup"
    fi
    
    cp -R "$AU_SOURCE" "$AU_DEST"
    print_status "AU plugin installed to: $AU_DEST"
else
    print_error "AU build not found at: $AU_SOURCE"
fi

# Summary
echo ""
echo "==================================="
echo "Build and Install Complete!"
echo "==================================="
echo ""
echo "Next steps:"
echo "1. Restart your DAW or rescan plugins"
echo "2. Test the changes listed in BUILD_NOTES.md"
echo "3. Update BUILD_NOTES.md with any issues found"
echo ""

# Ask if user wants to open BUILD_NOTES.md to add test results
read -p "Open BUILD_NOTES.md to add test results? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    open "$SCRIPT_DIR/BUILD_NOTES.md"
fi