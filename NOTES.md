# BuildUpVerb Notes

## Status
- **Current Status:** 🚧 Stage 1
- **Version:** 1.0.0
- **Type:** Audio Effect (Reverb/Filter)

## Lifecycle Timeline

- **2024-11-27:** Plugin created - One-knob reverb/filter buildup effect

## Description

BuildUpVerb is a single-knob effect that combines reverb and high-pass filtering to create dramatic build-up effects. As the knob increases:
- Reverb room size and wetness increase
- High-pass filter frequency sweeps from 100Hz to 10kHz
- Reverb damping decreases for brighter sound
- Creates tension and energy perfect for transitions

## Parameters

- **Build Up** (0-100%): Master control for all effect parameters

## DSP Chain

1. Input signal splits to dry and reverb paths
2. Reverb processing with dynamic parameters
3. High-pass filter on reverb signal
4. Dry/wet mix based on buildup amount

## Known Issues

None

## Additional Notes

The plugin uses a WebView-based UI with a custom circular knob that responds to mouse drag, wheel, and double-click (reset). The visual feedback includes a progress arc that changes color from blue to purple as the buildup increases.