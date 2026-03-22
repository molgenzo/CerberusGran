# PocketGranules

An open-source 5-engine granular synthesizer with per-grain DSP capabilities, built in JUCE/C++.

> **Work in Progress** — actively under development. Features and UI may change.

## Features

### Granular Engine
- **5 independent grain heads** with shared lock-free ring buffer
- **Live audio input** granulation — captures DAW bus in real-time
- **File mode** — drag-and-drop audio file loading
- **32 grains per head** (160 total), pre-allocated for zero audio-thread allocation
- **Continuous grain shape morphing** — smooth interpolation between Square, Trapezoid, Sine/Hamming, Triangle, and Saw Down windows
- **Reverse probability** — per-head control (0–100%) for the chance each grain plays in reverse
- **Freeze** — stops ring buffer capture, locking the current audio in place

### Per-Head Parameters
- Position, Spread, Rate, Length, Pitch, Gain, Reverse, Grain Shape

### Per-Head FX Chain
Each grain head has its own independent DSP chain:
- **Filter** — State Variable TPT (LP / HP / BP), cutoff + resonance
- **Bitcrusher** — bit depth reduction + sample rate reduction
- **Delay** — up to 2 seconds, with feedback and mix
- **Reverb** — room size, damping, mix

FX slots are selectable per head — choose which effects go in which slot to build a custom chain.

### UI
- Dark-themed vertical column layout with custom LookAndFeel
- 5 color-coded engine columns (Purple, Teal, Coral, Pink, Blue)
- Minimal outlined rotary knobs with accent-colored indicators
- Live waveform display with head position markers, spread visualization, and grain playhead bars
- Collapsible FX accordion cards with active dots and triangle chevrons
- Grain shape knob with live shape preview
- Global controls: Freeze, Live/File mode, Master Gain (dB), Dry/Wet

### Technical
- Built with JUCE and C++17
- AU and VST3 plugin formats
- ~129 automatable parameters with full state save/load
- Real-time safe — zero allocation, zero locks in audio thread
- Lock-free ring buffer with atomic write position
- Per-head intermediate buffers for independent FX processing
- Anti-click fade ramps on all grain window shapes

## Building

```bash
cd CerberusGran
mkdir build && cd build
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build .
```

Plugins are auto-installed to `/Library/Audio/Plug-Ins/` (AU and VST3).

## License

Open source. License TBD.
