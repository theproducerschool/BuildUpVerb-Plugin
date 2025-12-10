# BuildUp Reverb - Build Notes

## Current Version: 1.0.19
**Date**: December 10, 2024

### Recent Changes
- Fixed clicking and artifacts in noise generation
- Added double-layer exponential smoothing for noise level changes
- Smoother fade-in/fade-out transitions for noise
- Improved noise level ramping with proper smoothing coefficients

### Known Issues
- Filter slope dropdown visibility issues (resolved)
- Drive knob visibility (resolved)

### Build Instructions
1. Navigate to build directory: `cd build`
2. Configure with CMake: `cmake -DCMAKE_BUILD_TYPE=Release ..`
3. Build: `cmake --build . --config Release`
4. Install VST3: `cp -R BuildUpVerb_artefacts/Release/VST3/BuildUp\ Reverb.vst3 ~/Library/Audio/Plug-Ins/VST3/`
5. Install AU: `cp -R BuildUpVerb_artefacts/Release/AU/BuildUp\ Reverb.component ~/Library/Audio/Plug-Ins/Components/`

### Version History

#### v1.0.19 (2024-12-10)
- Fixed clicking and artifacts in noise generation
- Implemented double-layer exponential smoothing (0.99 + 0.995 coefficients)
- Proper fade-out when Build Up or Noise Amount reaches zero
- All noise types (white, pink, vinyl) now use smoothed level

#### v1.0.18 (2024-12-10)
- Noise now gets filtered when filter is active
- Fixed delay buffer management - no more bleeding at 0% mix
- Improved signal routing for noise through main path
- Noise remains Build Up dependent (core build-up feature)

#### v1.0.17 (2024-12-10)
- Removed Build Up dependency from effects
- Effects now work independently at any Build Up position
- Each effect only responds to its own control knob
- Improved plugin usability and flexibility

#### v1.0.16 (2024-12-10)
- Fixed critical routing bug - effects no longer require reverb to be active
- Refactored signal processing chain
- Effects now process main buffer directly
- Reverb is mixed as a separate wet signal
- All controls now function independently as intended

#### v1.0.15 (2024-12-10)
- Added tempo-synced delay effect
- Delay time divisions: 1/2, 1/3, 1/4, 1/8, 1/16
- Mix control (0-100%) and feedback (0-90%)
- Syncs to host tempo automatically
- New delay section in UI layout

#### v1.0.14 (2024-12-10)
- Added Smart Pan knob for autopan effect
- Smart panning syncs with tremolo rate and phase
- Creates stereo movement with opposite phase for L/R
- Pan amount scales with Build Up intensity

#### v1.0.13 (2024-12-10)
- Smooth gain compensation without artifacts
- Better approach to unity gain that preserves reverb quality
- Mix compensation factor prevents gain increase

#### v1.0.12 (2024-12-10)
- Fixed the root cause of gain increase with Build Up movement
- Freeverb now outputs only wet signal (no dry passthrough)
- Proper unity gain mixing achieved

#### v1.0.11 (2024-12-10)
- Immediate bypass at top of processBlock
- Fixed the no-signal issue definitively
- Cleaner, more efficient bypass implementation

#### v1.0.10 (2024-12-10)
- True bypass implementation when Build Up = 0
- No processing, no gain changes, just pure signal passthrough
- Fixed the no-output issue definitively

#### v1.0.9 (2024-12-10)
- Fixed critical issue: no output when Build Up = 0
- Corrected bypass logic to allow dry signal passthrough
- Plugin is now truly transparent at Build Up = 0

#### v1.0.8 (2024-12-10)
- Fixed gain boost issue even when reverb mix is 0%
- Reverb buffer now properly cleared to prevent double dry signal
- True unity gain across all settings

#### v1.0.7 (2024-12-09)
- Fixed gain increase when adding reverb
- Proper unity gain compensation ensures constant loudness
- Dry + wet levels now properly normalized

#### v1.0.6 (2024-12-09)
- Implemented dry/wet crossfade for washout effect
- Dry signal reduces as Build Up increases for immersive reverb
- Kept reverb mix at 50% for instant usability

#### v1.0.5 (2024-12-09)
- Fixed bypass/transparency issue when Build Up = 0
- Plugin now has zero effect on audio when main knob is at minimum
- Improved bypass logic to prevent any gain changes

#### v1.0.4 (2024-12-09)
- Tamed resonance to prevent distortion
- Reduced maximum resonance from 10.0 to 4.0
- Made resonance scaling more musical

#### v1.0.3 (2024-12-09)
- Added DRIVE knob to filter section
- Drive applies soft-clipping saturation to filtered signal
- Works with all filter types and slopes

#### v1.0.2 (2024-12-09)
- Changed filter slopes to 6/12/18/24 dB/oct for better granularity
- Fixed cascading implementation for proper slope behavior
- Reduced resonance on cascaded stages to prevent buildup

#### v1.0.1 (2024-12-09)
- Added filter slope control (12/24/36/48 dB/oct)
- Fixed parameter naming inconsistencies
- UI layout improvements

#### v1.0.0 (Initial Release)
- Basic BuildUp effect with reverb
- Filter automation
- Noise generator
- Riser synthesis
- Tremolo effect
- Hardware-style UI

### TODO
- [ ] Re-enable Drive knob in UI
- [ ] Add more filter types
- [ ] Implement preset save/load functionality
- [ ] Add MIDI learn capability

### Notes for Next Build
_Edit this section before each build to track what you're working on:_

**Next Changes**:
- (Completed in v1.0.5)

**Testing Focus**:
- Test plugin transparency is working correctly
- Verify smooth transitions when moving Build Up knob

**Expected Issues**:
- None currently identified