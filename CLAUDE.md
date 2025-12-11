# Claude Code Instructions for BuildUpVerb Plugin

## Current Version: 1.0.34 (December 10, 2024)

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
  - `PluginEditor.cpp/h`: User interface with hardware-style design
  - `FreeverbWrapper.cpp/h`: Reverb algorithm wrapper
  - Other `.cpp/.hpp` files: Reverb algorithm components

- **build/**: Build directory (created by CMake)
  - Contains compiled plugins after building

- **BUILD_NOTES.md**: Track all changes, versions, and issues (currently v1.0.31)
- **SIGNAL_ROUTING.md**: Documents the signal flow and processing order
- **build_and_install.sh**: Automated build and install script

## Common Tasks

### Adding a New Parameter
1. Add parameter in `PluginProcessor.cpp` (createParameterLayout)
2. Add UI control in `PluginEditor.cpp` (KnobComponent class)
3. Update parameter handling in processBlock
4. Add attachment in KnobComponent constructor
5. Update BUILD_NOTES.md with the change

### Modifying the UI
1. Edit `PluginEditor.cpp` for layout changes
2. Use the hardware-style LookAndFeel for consistency
3. Current layout: 900x600 pixels with sections for Filter, Noise, Tremolo, Riser, Delay
4. Test with different window sizes

### Building and Testing
1. Run `./build_and_install.sh`
2. The script will:
   - Check if BUILD_NOTES.md is updated
   - Build the plugin
   - Install to system directories
   - Create backups of existing plugins

## Important Technical Details

### Signal Flow (v1.0.31+)
1. Input → Build Up Check (bypass if 0)
2. Filter Processing (if active)
3. Noise Generation (including Vocoded modes)
4. Reverb Processing (AFTER noise for proper vocoder release)
5. Riser/Tremolo/Delay/Width effects
6. Output with gain compensation

### Filter Implementation
- Slopes: 6/12/18/24 dB/oct (1-4 cascaded stages)
- Types: High Pass, Low Pass, Dual Sweep
- Each stage uses StateVariableTPTFilter
- Drive option with soft-clip saturation

### Noise System
- Types: White, Pink, Vinyl, Vocoded White
- Vocoded mode uses per-channel envelope following
- Vocoder Release parameter controls sustain (up to 20x slower)
- Noise ONLY plays when Build Up > 0

### Key Fixes in Recent Versions
- v1.0.32: Removed Vocoded Pink, fixed clicking with smoother coefficients
- v1.0.31: Reverb processes AFTER noise (fixes vocoder+reverb interaction)
- v1.0.30: Per-channel vocoder smoothing prevents clicks
- v1.0.29: Fixed shared envelope bug in vocoder
- v1.0.28: Dramatic vocoder release (20x range)
- v1.0.27: Removed gating from vocoder

## Quick Commands

```bash
# Build and install
./build_and_install.sh

# Just build without installing
cd build && cmake --build . --config Release

# Install VST3 manually
cp -R build/BuildUpVerb_artefacts/Release/VST3/BuildUp\ Reverb.vst3 ~/Library/Audio/Plug-Ins/VST3/

# Clean build
rm -rf build && mkdir build && cd build && cmake .. && cmake --build . --config Release
```

## Complete Parameter List
- **buildUp** (0-100%) - Main control, bypasses at 0
- **filterIntensity** (0-100%) - Filter automation amount
- **filterType** - High Pass, Low Pass, Dual Sweep
- **filterSlope** - 6/12/18/24 dB/oct
- **filterResonance** (0.5-4.0) - Q factor
- **filterDrive** (0-100%) - Saturation amount
- **reverbMix** (0-100%) - Wet/dry mix
- **noiseAmount** (0-100%) - Noise generator level
- **noiseType** - White, Pink, Vinyl, Vocoded White
- **noiseGate** (0-100%) - Threshold for vocoder
- **vocoderRelease** (0-100%) - Release time multiplier
- **tremoloRate** (0.1-20 Hz) - LFO speed
- **tremoloDepth** (0-100%) - Amplitude modulation
- **riserAmount** (0-100%) - Riser generator level
- **riserType** - Sine, Saw, Square, Noise Sweep, Sub Drop
- **riserRelease** (0.01-5s) - Envelope release time
- **stereoWidth** (0-200%) - Stereo enhancement
- **smartPan** (0-100%) - Auto-pan amount
- **delayTime** - 1/2, 1/3, 1/4, 1/8, 1/16 (tempo-synced)
- **delayMix** (0-100%) - Delay wet level
- **delayFeedback** (0-90%) - Delay regeneration
- **autoGain** - Automatic gain compensation
- **macro** (0-100%) - Macro control
- **macroMode** - Off, Subtle, Aggressive, Epic, Custom

## Debugging Tips
- Check Console.app for plugin loading errors
- Use `pluginval` for validation
- Check BUILD_NOTES.md for known issues and version history
- Vocoder issues: Check per-channel envelope levels
- Noise issues: Verify Build Up dependency
- The plugin window is 900x600 pixels

## Critical Implementation Notes
1. **Smoothing coefficients**: Build Up uses 0.995, noise uses 0.99, vocoder uses 0.9999
2. **Per-channel processing**: Vocoder uses static arrays for L/R independence
3. **Signal order**: Reverb MUST process after noise for vocoder to work
4. **Reset behavior**: All levels reset immediately when Build Up = 0
5. **Vocoder smoothing**: Attack 0.998, Release 0.9998, Output 0.9999 for click-free operation