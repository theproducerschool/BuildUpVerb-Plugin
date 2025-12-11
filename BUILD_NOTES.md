# BuildUp Reverb - Build Notes

## Current Version: 1.3.1
**Date**: December 11, 2024

### Recent Changes
- Added body in high-mids while keeping crispy highs
- Bands: 1.5kHz, 3kHz, 6kHz, 12kHz (better balance)
- 80% high-passed noise (was 90%) for more body
- Gains: 2x, 3x, 6x, 12x (progressive boost)
- Wider bands in low-mids for fuller sound

### Known Issues
- Persistent "fast LFO/ringing" artifact even with constant noise output
- Issue appears to be in signal chain AFTER vocoder processing
- Reverb parameters still use smoothedBuildUp which might cause modulation

### Build Instructions
1. Navigate to build directory: `cd build`
2. Configure with CMake: `cmake -DCMAKE_BUILD_TYPE=Release ..`
3. Build: `cmake --build . --config Release`
4. Install VST3: `cp -R BuildUpVerb_artefacts/Release/VST3/BuildUp\ Reverb.vst3 ~/Library/Audio/Plug-Ins/VST3/`
5. Install AU: `cp -R BuildUpVerb_artefacts/Release/AU/BuildUp\ Reverb.component ~/Library/Audio/Plug-Ins/Components/`

### Version History

#### v1.3.1 (2024-12-11)
- User: "highs are good, noise needs body in high-mids"
- Shifted bands down slightly for body
- Less aggressive high-pass filtering
- Better gain distribution across bands
- Maintains crispness while adding fullness

#### v1.3.0 (2024-12-11)
- "Still not bright enough" - going EXTREME
- Focused entirely on presence/crisp high end
- High-pass filtered noise carrier
- Massive 16kHz boost (15x)
- Double emphasis filter stages

#### v1.2.9 (2024-12-11)
- Extreme brightness adjustments per user request
- "Really high focused" sound achieved
- Shifted frequency bands up significantly
- Heavy high-frequency boosting (8x on 14kHz band)
- High-shelf emphasis filter added

#### v1.2.8 (2024-12-11)
- Brightened 4-band vocoder output
- High frequency bands (3.6kHz, 10.8kHz) boosted
- Added white noise mix for presence
- Wider filter bandwidths
- User reported output was too muffled/low

#### v1.2.7 (2024-12-11)
- Switching to VocoderFilterbank.cpp implementation
- Already has proper filter initialization
- 4 bandpass filters with envelope followers
- Single noise source filtered through 4 bands
- More stable than previous FFT approach

#### v1.2.6 (2024-12-11)
- Reverted to simple vocoder after 4-band had no output
- Filter initialization was problematic
- Back to working single envelope follower
- Need different approach for multi-band

#### v1.2.5 (2024-12-11)
- Upgraded from simple envelope follower to 4-band vocoder
- Each frequency band tracked independently
- More characteristic vocoder sound like Ableton
- Bandpass filters with appropriate Q values
- Should give better frequency separation

#### v1.2.4 (2024-12-11)
- Found ringing source: vocoder envelope follower coefficients
- Attack was too fast (0.1f) causing envelope oscillation
- Reduced to 0.01f for smoother tracking
- Also smoothed release coefficient base
- Should eliminate the "fast LFO" artifact

#### v1.2.3 (2024-12-11)
- Critical fix: constant 0.1f was DC offset, not noise
- User showed spectrum analyzer - signal was below 50Hz!
- Now outputs proper white noise: (random.nextFloat() - 0.5f) * 2.0f * 0.2f
- This will finally let us hear if there's ringing/modulation

#### v1.2.2 (2024-12-11)
- Found noise IS in buffer (0.1) but not audible
- Added debug for final output levels and compensation
- Checking if gain compensation is zeroing the signal
- Also checking if early bypass is triggered

#### v1.2.1 (2024-12-11)
- Debug build to trace missing noise output
- Added DBG statements throughout signal chain
- Will output to console: parameter values, buffer contents, mix levels
- User reports "simply no noise again" - need to diagnose

#### v1.2.0 (2024-12-11)
- Critical fix: reverb mixing was silencing the vocoder output
- When reverb buffer is cleared but reverbWetLevel > 0, it attenuates dry signal
- Bypassed entire reverb mixing section for testing
- User reported "no noise at all" - this was the cause

#### v1.1.9 (2024-12-11)
- Changed to absolute constant output (0.1f) without vocoderGain
- Added debug logging to track vocoderGain values
- This test isolates whether vocoderGain is modulating
- User reported vocoder section not giving constant noise
- Critical test to find modulation source

#### v1.1.8 (2024-12-11)
- Bypassed reverb processing entirely (reverbBuffer.clear() only)
- Testing if reverb parameter modulation causes the ringing
- Reverb parameters use smoothedBuildUp which changes continuously
- This build will determine if reverb is the source of artifacts

#### v1.1.7 (2024-12-11)
- Testing constant noise output to isolate ringing artifact
- Found that even constant noise (0.1f * vocoderGain) has the "fast LFO" artifact
- Discovered issue is NOT in vocoder algorithm but elsewhere in signal chain
- Filters already bypassed in code, testing if reverb modulation is the cause
- Using VocoderGated.cpp with constant output for diagnosis

#### v1.1.6 (2024-12-11)
- Fixed "not smooth noise" issue in vocoder
- Single noise source now filtered through 4 bands (like real vocoder)
- Added SmoothNoiseGenerator class with 3-pole lowpass filtering
- Proper stereo processing with independent noise generators
- Extra output smoothing stage to eliminate any remaining roughness

#### v1.1.5 (2024-12-11)
- Added debugging for FFT timing and buffer boundaries
- Implemented filterbank vocoder alternative (VocoderFilterbank.cpp)
- Testing different overlap percentages (87.5%)
- Using proper JUCE FFT API (performFrequencyOnlyForwardTransform)
- Filterbank approach may be more stable than FFT

#### v1.1.4 (2024-12-10)
- Completely rewrote vocoder to fix fast LFO artifacts
- Per-channel FFT processing with independent state
- Proper circular buffer management
- Direct sample-by-sample processing with FFT at hop intervals
- Should finally sound smooth like Ableton's vocoder

#### v1.1.3 (2024-12-10)
- Increased overlap to 75% (was 50%) for smoother output
- Fixed window compensation with proper scaling factor
- Better overlap buffer timing to eliminate flutter
- Cleaner separation of FFT processing and sample output
- Attack time now 5ms for natural response

#### v1.1.2 (2024-12-10)
- Fixed "fast LFO" artifacts in vocoder output
- Proper overlap-add FFT processing with 50% hop size
- Phase coherent noise generation
- Smoother attack/release coefficients
- Eliminated clicking and crackling in vocoded signal

#### v1.1.1 (2024-12-10)  
- Reduced from 32 bands to 4 bands like Ableton
- Exact frequency bands matching Ableton's vocoder
- Better release time calculation using exponential decay
- Noise carrier with random phase per band
- More characteristic lo-fi vocoder sound

#### v1.1.0 (2024-12-10)
- Complete vocoder rewrite using FFT
- 1024-sample FFT with 50% overlap
- 32 logarithmically-spaced frequency bands
- Random phase generation for noise carrier
- Removed White, Pink, and Vinyl noise types
- UI now shows "VOCODER" label instead of combo box
- Release control affects band smoothing (0.95 to 0.9999)

#### v1.0.35 (2024-12-10)
- Refined white noise generation for vocoder (averaged two random values)
- Implemented triple smoothing layer for envelope (two-pass smoothing)
- Added gentle lowpass filter after DC blocker (~10kHz cutoff)
- Ensures no vinyl-like crackling contamination in vocoded signal

#### v1.0.34 (2024-12-10)
- Fixed vocoder clicking during input signal playback
- Changed to RMS-style envelope detection for smoother tracking
- Added DC blocking filter (highpass at ~20Hz) to remove offset
- Attack coefficient now 0.9995 (was 0.998)
- DC blocker state properly reset when Build Up = 0

#### v1.0.33 (2024-12-10)
- Fixed noise type combo box mapping after removing Vocoded Pink
- Parameter value calculation now uses /3.0f for 4 noise types
- Vocoded White now correctly selects case 3 in processBlock

#### v1.0.32 (2024-12-10)
- Removed Vocoded Pink noise type to simplify interface
- Fixed clicking in Vocoded White with smoother envelope coefficients
- Attack coefficient: 0.998 (was 0.995)
- Base release coefficient: 0.9998 (was 0.9997)
- Vocoder smoothing: 0.9999 (was 0.9995)

#### v1.0.31 (2024-12-10)
- Major signal flow fix - reverb now processes after noise generation
- Vocoder release now works properly when reverb is active
- Fixed static noise lingering by using faster smoothing coefficient
- Proper reset of all per-channel envelope and vocoder levels
- Filter and riser processing moved to main buffer

#### v1.0.30 (2024-12-10)
- Fixed vocoder clicking on sustained synths with per-channel smoothing
- Fixed noise playing when Build Up at 0 - removed fade-out processing
- Increased vocoder smoothing to 0.9995 for click-free operation
- Per-channel vocoder levels array prevents stereo interference
- Immediate noise reset when Build Up is off

#### v1.0.29 (2024-12-10)
- Fixed critical vocoder bug - was using shared envelope across channels
- Implemented per-channel envelope tracking for proper stereo vocoding
- Fixed noise dependency on Build Up knob (was checking smoothedBuildUp)
- Vocoder now works correctly with independent L/R envelope following

#### v1.0.28 (2024-12-10)
- Dramatically increased vocoder release range (20x at maximum)
- Doubled vocoder output level for better prominence
- Reduced envelope compression from ^0.7 to ^0.85
- Maximum release coefficient now 0.99999 for extremely long tails
- Base release coefficient increased to 0.9997

#### v1.0.27 (2024-12-10)
- Fixed vocoder gating issue - removed gate threshold entirely
- Vocoder now uses continuous envelope following with no gating
- Smoother, more natural vocoder response with long tail
- Envelope compression (^0.7) for more consistent levels
- Extra smoothing layer (0.998) on vocoder output

#### v1.0.26 (2024-12-10)
- Added vocoder release parameter with UI knob
- Release time increases dynamically with build up level
- Vocoder noise becomes more sustained at higher build up values
- Release multiplier: 1x to 5x based on build up and release knob

#### v1.0.25 (2024-12-10)
- Fixed clicking when noise + reverb are combined
- Reverb parameters (room size, damping, wet level, width) now use smoothedBuildUp
- Prevents sudden parameter changes in freeverb algorithm
- Completes the noise clicking fix from v1.0.24

#### v1.0.24 (2024-12-10)
- Root cause fix: Build Up parameter now smoothed before use
- Simplified to single smoothing layer with 0.9995 coefficient
- All noise types (white, pink, vinyl, vocoded) now click-free
- Smoother parameter response overall

#### v1.0.23 (2024-12-10)
- Fixed C++ variable-length array warning
- Implemented triple-smoothing for vocoder: noise level + envelope + vocoder output
- Envelope follower now per-channel, per-sample
- Moved static variable to member variable

#### v1.0.22 (2024-12-10)
- Rewrote vocoder to calculate envelope first
- Noise is now modulated during generation
- Fixed all clicking artifacts in vocoded modes
- Both vocoded white and pink noise work smoothly

#### v1.0.21 (2024-12-10)
- Fixed vocoder implementation - removed FFT clicking
- Uses simple envelope follower for vocoder effect
- Fixed UI combo box scaling for proper noise type selection
- More stable and CPU-efficient vocoder

#### v1.0.20 (2024-12-10)
- Added vocoder-based noise types for unique texture effects
- Implemented FFT spectral analysis (1024 sample window)
- Spectral envelope extraction with smoothing
- Overlap-add synthesis for smooth vocoded output
- Two new noise types: Vocoded White and Vocoded Pink

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
- Test with reverb completely bypassed
- Check if smoothedBuildUp modulation of reverb causes the artifact
- Once ringing is fixed, re-enable proper vocoder algorithm
- Future: Multiple carrier types (white, vinyl, synth)

**Testing Focus**:
- Isolate source of ringing artifact in signal chain
- Test with reverb bypassed
- Verify constant noise has no modulation

**Expected Issues**:
- None currently identified