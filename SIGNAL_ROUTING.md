# BuildUp Reverb - Signal Routing Documentation

## Overview
This document describes the signal flow and routing in the BuildUp Reverb plugin v1.0.32.

## Signal Flow Diagram (v1.0.31+)

```
Input Signal
    |
    v
[Build Up Check] --> If Build Up = 0, bypass all processing
    |
    v
[Envelope Follower] --> Used for vocoder and noise gating
    |
    v
[Filter Section] --> High Pass / Low Pass / Dual Sweep
    |                (Automated by Build Up knob)
    |                (Drive knob adds saturation)
    |                (Now processes MAIN buffer directly)
    |
    v
[Noise Generator] --> White/Pink/Vinyl/Vocoded White
    |                 (Amount controlled by Build Up × Noise Amount)
    |                 (Vocoded types use per-channel envelope following)
    |                 (VOC RELEASE controls sustain time)
    |                 (Noise added to MAIN buffer)
    |
    v
[Main Signal Path] <-- Dry signal + filtered signal + noise
    |
    v
[Reverb Processing] --> NOW HAPPENS AFTER NOISE! (v1.0.31+)
    |                     (Processes the complete signal including vocoded noise)
    |                     (Uses smoothed Build Up for parameter changes)
    |                     (Freeverb algorithm in separate buffer)
    |
    v
[Riser Synthesis] --> Sine/Saw/Square/Noise Sweep/Sub Drop
    |                 (Pitch rises with Build Up)
    |                 (Added directly to main buffer)
    |
    v
[Tremolo Effect] --> Applied to main signal
    |                (Independent of Build Up)
    |                (Modulates amplitude with LFO)
    |
    v
[Stereo Width] --> M/S processing on main signal
    |              (Independent of Build Up)
    |              (0-200% width adjustment)
    |
    v
[Smart Pan] --> Linked to tremolo LFO
    |           (Independent of Build Up)
    |           (Creates stereo movement)
    |
    v
[Delay Effect] --> Tempo-synced delay
    |              (Independent of Build Up)
    |              (Properly clears when mix = 0)
    |
    v
[Reverb Mix] <---- Mix in reverb buffer here
    |              (Amount = Build Up × Reverb Mix)
    |
    v
[Auto Gain] --> Compensation for loudness
    |
    v
Output Signal
```

## Critical Changes in v1.0.31

### Major Signal Flow Reorganization
1. **Reverb moved to AFTER noise generation**
   - Previously: Input → Reverb → Add Noise → Output
   - Now: Input → Add Noise → Reverb → Output
   - This fixes vocoder release not working with reverb

2. **All processing on main buffer**
   - Filter, noise, and riser now process the main buffer directly
   - Reverb creates its own buffer from the processed signal
   - Ensures all effects interact properly

## Key Routing Principles

### 1. Independent Effects
The following effects work independently of the Build Up knob position:
- **Tremolo** - Only controlled by Tremolo Rate and Depth knobs
- **Stereo Width** - Only controlled by Width knob  
- **Smart Pan** - Only controlled by Smart Pan knob
- **Delay** - Only controlled by Delay Mix, Time, and Feedback

### 2. Build Up Dependent Effects
These effects are intentionally linked to the Build Up knob:
- **Filter Automation** - Filter frequency sweeps based on Build Up position
- **Noise Amount** - Noise level = Build Up × Noise Amount (FIXED in v1.0.31)
- **Riser Pitch** - Riser frequency increases with Build Up
- **Reverb Parameters** - Room size, damping, wet level scale with Build Up

### 3. Noise System Details (v1.0.20-32)
- **Noise Types**: White, Pink, Vinyl, Vocoded White
- **Build Up Dependency**: Noise ONLY plays when Build Up > 0.01
- **Vocoder Implementation**:
  - Per-channel envelope following (L/R independent)
  - No gating - continuous envelope modulation
  - VOC RELEASE parameter: 0-100% (controls release time multiplier)
  - Release multiplier: 1x to 20x based on Build Up and VOC RELEASE
  - Smoothing: 0.9999 for vocoder output, 0.99 for noise level

### 4. Processing Order (v1.0.31+)
1. **Input** → Build Up Check → Filter → **Main Buffer**
2. **Main Buffer** → Add Noise → **Main Buffer with Noise**
3. **Copy to Reverb Buffer** → Process Reverb → **Reverb Buffer**
4. **Main Buffer** → Add Riser → Tremolo → Width → Pan → Delay → **Processed Buffer**
5. **Final Mix** = Processed Buffer + (Reverb Buffer × Mix Amount)

## Parameter Interactions

### Build Up Knob (0-100%)
- At 0%: Complete bypass (no audio processing)
- Controls filter frequency automation
- Gates noise generator (noise only plays when > 0)
- Controls riser pitch sweep
- Modulates reverb parameters (room size, damping, etc.)
- Does NOT affect: Tremolo, Delay, Smart Pan, Width

### Vocoder Release Knob (0-100%)
- Only affects Vocoded White noise type
- At 0%: Normal envelope following
- At 100%: Up to 20x slower release when Build Up is high
- Creates dramatic sustained vocoder effects

### Reverb Processing
- Room Size: 0.3 + (Build Up × 0.65)
- Damping: 0.7 - (Build Up × 0.5)
- Wet Level: 0.3 + (Build Up × 0.5)
- Uses smoothed Build Up value to prevent parameter jumps

### Smoothing Coefficients
- Build Up: 0.995 (prevents clicks in parameter changes)
- Noise Level: 0.99 (faster response to prevent lingering)
- Vocoder Output: 0.9999 (ultra smooth to prevent clicks)
- Envelope Follower: Attack 0.998, Release 0.9998-0.99999

## Important Technical Notes

### Per-Channel Processing
- Vocoder uses static arrays for L/R channel independence
- Prevents cross-channel interference and clicking
- Each channel has its own envelope level and vocoder level

### Reset Behavior
When Build Up returns to 0:
- smoothedNoiseLevel → 0
- smoothedVocoderLevel → 0
- channelEnvelopeLevels[L/R] → 0
- channelVocoderLevels[L/R] → 0
- No fade out - immediate silence

### Version History Highlights
- **v1.0.32**: Removed Vocoded Pink, smoother vocoder coefficients
- **v1.0.31**: Major signal flow fix - reverb after noise
- **v1.0.30**: Per-channel vocoder smoothing
- **v1.0.29**: Fixed shared envelope bug
- **v1.0.28**: 20x vocoder release range
- **v1.0.27**: Removed gating from vocoder
- **v1.0.24-26**: Various vocoder improvements
- **v1.0.20**: Initial vocoder implementation
- **v1.0.17**: Made effects independent of Build Up
- **v1.0.16**: Fixed routing dependency issues

## Troubleshooting

### Vocoder Issues?
1. Check Build Up > 0 (vocoder needs Build Up active)
2. Check Noise Amount > 0
3. Select Vocoded White noise type
4. VOC RELEASE controls sustain amount
5. Input signal needed for envelope following

### Static Noise Playing?
1. Build Up must be > 0.01 for any noise
2. Check smoothedNoiseLevel is resetting
3. Verify per-channel levels are cleared

### No Vocoder Release with Reverb?
- Fixed in v1.0.31 - reverb now processes after noise
- Previous versions had reverb before noise (incorrect)

### Effects Not Working?
1. Check Build Up > 0 for filter/noise/riser
2. Independent effects work at any Build Up
3. Reverb needs both Build Up AND Reverb Mix > 0

## Buffer Management
- **Main buffer**: Primary processing path
- **Reverb buffer**: Created after noise is added (v1.0.31+)
- **Noise buffer**: Temporary buffer for noise generation
- **Delay buffers**: Separate L/R for delay line (2 sec max)
- **FFT buffers**: 1024 samples for vocoder analysis (unused in current implementation)