# IntervalOsc

Dual oscillator with interval offset for Daisy Patch.Init()

## Author

Nick Donaldson (Kip Schwinger)

## Description

Dual oscillator in which the second oscillator's frequency is offset by a quantized amount from the first oscillator's frequency
according to two different modes. The controls are nearly identical in both modes.

The CV_1 control for base frequency is quantized to standard `A=440Hz` semitones, meaning in the absence of any CV input on CV_5 jack,
the oscillators will always be "in tune" with standard `A=440Hz` equal temperament tuning.

---

## Controls

### Inputs

|    Name   |    Type      |    Description                                   |    Range    |
|    ---    |    ---       |    ---                                           |    ---      | 
|    CV_1   |    Knob      |    Base frequency for both oscillators. Quantized to semitones. |  E1 (41.2 Hz) – C6 (1046.5 Hz) |
|    CV_2   |    Knob      |    Frequency offset for second oscillator. Quantized - behavior depends on mode (read below) | +/- 2 octaves (interval mode) |
|    CV_3   |    Knob      |    Detune between the two oscillators | 0-25 cents |
|    CV_4   |    Knob      |    Pulse width of both oscillators (only applies when wave shape is rectangle) | ~ 0% – 100% |
|    CV_5   |  CV In Jack  |    1V/Oct input for base frequency. Unquantized Summed with CV_1 knob value to get final base frequency. | -5V – +5V (plus or minus 5 octaves) |
|    CV_6   |  CV In Jack  |    CV input for second oscillator offset. Quantized - behavior depends on mode (read below) | -5V – +5V |
|    CV_7   |  CV In Jack  |    TBD (currently unused) | N/A |
|    CV_8   |  CV In Jack  |    CV pulse width modulation input for both oscillators (only applies when wave shape is rectangle) | -5V – +5V |
|    B7     |  Button      |    Push to cycle through waveforms (see table below). Hold for >1 second to reset to first wave mode. | N/A |
|    B8     |  Switch      |    Select patch mode - Down = Interval Mode, Up = Pure Harmonic Mode | N/A |

All other inputs/controls unused.

### Outputs

|    Name    |    Description         |
|    ---     |    ---                 |
|  OUT_L[i]  |   Oscillator 1 output  |
|  OUT_R[i]  |   Oscillator 2 output  |

All other outputs unused.

---

## Modes

### Interval / Equal Temperament Mode (B8 Switch Down)

In this mode the frequency of the second oscillator will always be offset from the first by a quantized integer number of semitones
in standard equal temperament tuning. This is analogous to a fixed interval on a keyboard, e.g. setting the second oscillator to a fifth
(+7 semitones) above the first will cause the second oscillator to _always_ be a 5th above the first's pitch.

Changing the base frequency (CV_1 knob + CV_5 voltage input) will change the frequency of both oscillators while preserving their relative
intonation. Changing the offset (CV_2 knob + CV_6 voltage input) will change the relative pitch interval of the two oscillators.

By sequencing both CV_5 (base frequency) and CV_6 (interval offset) inputs, a 2-note chord melody can be sequenced.

### Pure Harmonic / Just Intonation Mode (B8 Switch Up)

In this mode the frequency of the second oscillator will always be a rational small-integer ratio multiple of the frequency of the first oscillator.
In practice this achieves a subset of just intonation pitch intervals, most of which are pure harmonic intervals (integer or integer-reciprocal multiples) of
the frequency of the first oscillator.

Changing the base frequency (CV_1 knob + CV_5 voltage input) will change the frequency of both oscillators while preserving the geometric ratio of their
frequencies. Changing the offset (CV_2 knob + CV_6 voltage input) will change the frequency multiplication factor for the second oscillator.

This mode is especially useful for audio rate cross-modulation synthesis techniques, e.g. using Mutable Instruments Warps, in which the output of the
cross-modulation will be less inharmonic if the fundamental frequency of both input signals are in exact- or nearly-exact harmonic ratios.

### Waveform Configurations 

Pushing the button (B7) will cycle through various subtractive-style waveforms for both oscillators in this order.
Holding the button for >1 second will reset back to the first mode (sin-sin).

|  Oscillator 1  |  Oscillator 2  |
|  ---           |  ---           |
|  Sin           |  Sin           |
|  Tri           |  Tri           |
|  Saw           |  Saw           |
|  Rect          |  Rect          |
|  Saw           |  Sin           |
|  Saw           |  Tri           |
|  Saw           |  Rect          |
|  Rect          |  Sin           |
|  Rect          |  Tri           |
|  Rect          |  Saw           |
