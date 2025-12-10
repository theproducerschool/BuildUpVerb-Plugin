# Claude Code Instructions for BuildUpVerb Plugin

## Before Every Build

1. **Always check and update BUILD_NOTES.md** before making any changes:
   - Update the "Recent Changes" section with what you're modifying
   - Update the version number if it's a significant change
   - Add any new known issues discovered
   - Update the "Next Changes" section for planning

2. **Use the build script** instead of manual commands:
   ```bash
   ./build_and_install.sh
   ```

## Project Structure

- **Source/**: Contains all C++ source files
  - `PluginProcessor.cpp/h`: Audio processing and parameters
  - `PluginEditor.cpp/h`: User interface
  - `FreeverbWrapper.cpp/h`: Reverb algorithm wrapper
  - Other `.cpp/.hpp` files: Reverb algorithm components

- **build/**: Build directory (created by CMake)
  - Contains compiled plugins after building

- **BUILD_NOTES.md**: Track all changes, versions, and issues
- **build_and_install.sh**: Automated build and install script

## Common Tasks

### Adding a New Parameter
1. Add parameter in `PluginProcessor.cpp` (createParameterLayout)
2. Add UI control in `PluginEditor.cpp`
3. Update parameter handling in processBlock
4. Update BUILD_NOTES.md with the change

### Modifying the UI
1. Edit `PluginEditor.cpp` for layout changes
2. Use the hardware-style LookAndFeel for consistency
3. Test with different window sizes

### Building and Testing
1. Run `./build_and_install.sh`
2. The script will:
   - Check if BUILD_NOTES.md is updated
   - Build the plugin
   - Install to system directories
   - Create backups of existing plugins

## Important Notes

- Filter slopes are implemented with cascaded filters (2 stages per 12dB)
- Maximum 4 filter stages currently defined
- UI uses a dark hardware-style theme
- Plugin supports both VST3 and AU formats on macOS

## Quick Commands

```bash
# Build and install
./build_and_install.sh

# Just build without installing
cd build && cmake --build . --config Release

# Clean build
rm -rf build && mkdir build && cd build && cmake .. && cmake --build . --config Release
```

## Current Parameter List
- buildUp (0-100%)
- filterIntensity (0-100%)
- filterType (High Pass, Low Pass, Dual Sweep)
- filterSlope (12/24/36/48 dB/oct)
- reverbMix (0-100%)
- noiseAmount (0-100%)
- noiseType (White, Pink)
- tremoloRate (0.1-10 Hz)
- tremoloDepth (0-100%)
- riserAmount (0-100%)
- riserType (Sine, Saw, Square, Sub)
- width (0-100%)
- macro (0-100%)
- filterDrive (0-100%) - currently not shown in UI

## Debugging Tips
- Check Console.app for plugin loading errors
- Use `pluginval` for validation
- Check BUILD_NOTES.md for known issues
- The plugin window is 900x600 pixels