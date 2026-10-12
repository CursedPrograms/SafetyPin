# Safety Pin

Electric violin wrecker: octave down (cello), sub bass, fat distortion, plate reverb.
64-bit VST3 + standalone app (ASIO, e.g. Focusrite Scarlett). Mono in, stereo out.

| Knob | Does |
|---|---|
| ANGER | drive |
| CELLO | real octave down; full = only the octave (cello) |
| GUTS | synth sub bass two octaves down |
| BOTTOM | bass shelf after the distortion (0 to +14 dB at 120 Hz) |
| SNARL | tone (dark to bright) |
| BASEMENT | reverb mix |
| DECAY | reverb length |
| LOUD | output level |
| NO FUTURE | +12 dB and hard clipping |

**Tuner:** standard violin tuning (G D A E, A = 440 Hz), listening to the clean input before any
effect. Shows note, cents and the nearest string; click it to mute the output while tuning.

ANGER at 0 and CELLO up gives a clean cello. CELLO and GUTS follow one note at a time; double stops confuse them.

## Build
Needs Visual Studio 2022 and CMake. JUCE 8.0.12 is downloaded into `build/` the first time.

    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release --parallel --target SafetyPin_VST3 SafetyPin_Standalone SafetyPinRender

- Standalone: `build/SafetyPin_artefacts/Release/Standalone/Safety Pin.exe`
- VST3: `build/SafetyPin_artefacts/Release/VST3/Safety Pin.vst3` (copy to `C:\Program Files\Common Files\VST3`)

## Standalone with the Focusrite
Options > Audio/MIDI Settings: driver type ASIO, device Focusrite USB ASIO, input = the one the
violin is plugged into, buffer 64-128 samples. Then click "Unmute Input" (JUCE mutes inputs on
first start to avoid feedback; use headphones or an amp, not a mic near the speakers).

## Offline test
`SafetyPinRender <dir>` writes `gui.png`, a synthetic violin and the effect's output at a few settings.
