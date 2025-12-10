# BuildUp Reverb - Signal Routing Documentation

## Overview
This document describes the signal flow and routing in the BuildUp Reverb plugin v1.0.17.

## Signal Flow Diagram

```
Input Signal
    |
    v
[Envelope Follower] --> Used for noise gating
    |
    v
[Filter Section] --> High Pass / Low Pass / Dual Sweep
    |                (Automated by Build Up knob)
    |                (Drive knob adds saturation)
    |
    v
[Noise Generator] --> White/Pink/Vinyl noise
    |                 (Gated by envelope follower)
    |                 (Amount controlled by Build Up)
    |
    v
[Riser Synthesis] --> Sine/Saw/Square/Noise Sweep/Sub Drop
    |                 (Pitch rises with Build Up)
    |
    v
[Main Signal Path] <------ This is where dry signal exists
    |
    +---> [Reverb Processing] --> Separate buffer
    |           |                  (Only if Reverb Mix > 0)
    |           |                  (Freeverb algorithm)
    |           v
    |     [Reverb Buffer]
    |
    v
[Tremolo Effect] --> Applied to main signal
    |                (Independent of Build Up)
    |
    v
[Stereo Width] --> M/S processing on main signal
    |              (Independent of Build Up)
    |
    v
[Smart Pan] --> Linked to tremolo LFO
    |           (Independent of Build Up)
    |
    v
[Delay Effect] --> Tempo-synced delay
    |              (Independent of Build Up)
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

## Key Routing Principles

### 1. Independent Effects (v1.0.17+)
The following effects work independently of the Build Up knob position:
- **Tremolo** - Only controlled by Tremolo Rate and Depth knobs
- **Stereo Width** - Only controlled by Width knob
- **Smart Pan** - Only controlled by Smart Pan knob
- **Delay** - Only controlled by Delay Mix, Time, and Feedback

### 2. Build Up Dependent Effects
These effects are intentionally linked to the Build Up knob:
- **Filter Automation** - Filter frequency sweeps based on Build Up position
- **Noise Amount** - Noise level scales with Build Up (core build-up effect)
- **Riser Pitch** - Riser frequency increases with Build Up
- **Reverb Wet Level** - Final reverb amount = Build Up × Reverb Mix

### 3. Enhancements (v1.0.18+)
- **Noise Filtering** - Noise now gets filtered through the same filter as the main signal
- **Improved Delay** - Delay buffer properly clears when mix is 0%

### 4. Processing Order
1. **Input** → Filter → Noise → Riser → **Main Buffer**
2. **Main Buffer** → Tremolo → Width → Pan → Delay → **Processed Buffer**
3. **Reverb** processed in parallel, mixed at the end
4. **Final Mix** = Processed Buffer + (Reverb Buffer × Mix Amount)

## Parameter Interactions

### Build Up Knob (0-100%)
- Controls filter frequency automation
- Scales noise generator output
- Controls riser pitch sweep
- Multiplies with Reverb Mix for final reverb amount
- Does NOT affect: Tremolo, Delay, Smart Pan, Width

### Reverb Mix Knob (0-100%)
- Sets base reverb amount
- Actual reverb = Build Up × Reverb Mix
- Example: Build Up 50%, Reverb 60% = 30% reverb in output

### Filter Section
- **Type**: High Pass, Low Pass, or Dual Sweep
- **Intensity**: How much the filter moves with Build Up
- **Slope**: 6/12/18/24 dB per octave
- **Resonance**: Filter Q/resonance amount
- **Drive**: Soft saturation on filtered signal

### Delay Section
- **Mix**: Dry/wet balance of delay (0-100%)
- **Time**: Tempo-synced divisions (1/2, 1/3, 1/4, 1/8, 1/16)
- **Feedback**: Delay repeats (0-90%, internally limited to prevent runaway)
- Syncs to host tempo, defaults to 120 BPM if not available

## Important Notes

### Version History
- **v1.0.15 and earlier**: All effects required Build Up > 0 to function
- **v1.0.16**: Attempted to separate effects from reverb buffer
- **v1.0.17**: Successfully made effects independent of Build Up position
- **v1.0.18**: Made noise independent, added noise filtering, fixed delay bleed

### Design Philosophy
The BuildUp Reverb is designed as a "riser" effect plugin where:
- The main Build Up knob creates the classic filter sweep/noise rise effect
- Additional effects (tremolo, delay, pan) can enhance but work independently
- This allows for more creative flexibility - you can use just the delay, or just the tremolo, without needing the full build-up effect

### Buffer Management
- Main audio buffer: Contains the dry signal and all effects except reverb
- Reverb buffer: Separate buffer for reverb processing only
- Delay buffers: Separate L/R buffers for delay line (up to 2 seconds)
- All processing is done at the host's sample rate

## Gain Staging

### Auto Gain Compensation
When enabled, applies gentle gain reduction based on active effects:
- Reverb contribution: 5% reduction at full
- Filter intensity: 2% reduction at full
- Noise amount: 3% reduction at full
- Riser amount: 2% reduction at full
- Tremolo depth: 1% reduction at full

### Mix Compensation
Additional gain reduction based on reverb wet level to prevent overload:
- Compensation = 1.0 / (1.0 + reverbWetLevel × 0.2)
- Helps maintain consistent output level

## Troubleshooting

### Effects Not Working?
1. Check the specific effect's knob (not Build Up)
2. For reverb: Need both Build Up AND Reverb Mix > 0
3. For filter: Need Build Up AND Filter Intensity > 0
4. Independent effects should work at any Build Up position (v1.0.17+)

### Too Loud/Quiet?
1. Enable Auto Gain for automatic compensation
2. Check individual effect levels
3. Reverb mix affects overall gain - adjust accordingly

### No Delay Sync?
1. Plugin attempts to read host tempo
2. Falls back to 120 BPM if host doesn't provide tempo
3. Delay times are musical divisions of the beat