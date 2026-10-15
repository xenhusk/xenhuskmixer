# Xenhusk Mixer console scope

This is the contract for the console. The reviewer scores only against this list. A control that is not listed here is out of scope.

The window stays one screen in the existing Apple look: Segoe UI, solid panels, hairline borders, press-darkening, 1:1 faders and knobs, system light or dark palette, no decorative motion, no copied VoiceMeeter artwork or product names.

## Out of scope

- A kernel virtual-audio driver. Buses B1–B3 are mix buses that can be metered and recorded. They do not appear as Windows playback devices.
- Five simultaneous hardware outputs. A1 is the only device output.
- A physical insert jack. There is no External FX button.
- VBAN, MIDI, a license badge, and the Potato skin.

## Top bar

- Title `Xenhusk Mixer`.
- Hardware device name, sample rate, and buffer size.
- `Pro Audio` only while MMCSS holds that task.
- `Menu` opens System settings: the audio device selector, the A1 limiter, the strip input pairs, and Save and Load.
- Closing the window hides it and leaves the mixer running. The tray icon opens Restart Audio Engine, Show Xenhusk Mixer, Show Xenhusk Mixer (default position), Hide Xenhusk Mixer, Shut Down Xenhusk Mixer, and Exit Menu. Shut Down is what quits.
- `Save` writes the current mix to an XML file. `Load` reads that file back onto the strips, buses, EQ, dynamics, and effects.
- Load also accepts a VoiceMeeter settings XML file. It maps strip names, faders, mute, solo, mono, bus assigns, EQ bands, compressor, gate, denoise, sends, and the shared reverb. It does not create virtual devices.

## Eight strips

Default names: Mic, Music, In 3, In 4, In 5, Virtual 1, Virtual 2, Virtual 3. The name is editable and is stored in the settings file.

Each strip shows, and the audio callback applies:

- `EQ` toggle. While it is on, the 6-band EQ and the `Bass` and `Treble` knobs change the tone. While it is off, they are ignored. Right-click `EQ`, `Bass`, or `Treble` opens the EQ window.
- IntelliPan: `Color`, `Voice`, `Echo`. Right-click any of those knobs opens Position (pan, unity at center), Modulation depth, and the same three knobs.
- Dynamics: `Comp`, `Gate`, `Denoise`. At zero the block is bypassed. Gate at maximum, with the simple gate, silences a quiet signal and lets a full-scale signal through. Right-click each knob opens its parameters: compressor input gain, threshold, ratio, attack, release, knee, output gain, and auto makeup; gate threshold, range, attack, hold, and release; denoise amount. Moving a parameter in that window turns the block on and the callback uses those values.
- Special FX: `Reverb` and `Delay` sends, plus `Post`. Post uses the fader level for the sends. Pre uses the level before the fader. Right-click either send to open the shared effect.
- Routing: `A1` `A2` `A3` `A4` `A5` `B1` `B2` `B3`. On sends the strip to that bus at unity. Off sends silence. The default is A1 on and the other seven off.
- Fader from −60 dB to +12 dB, default 0 dB, with a one-decimal readout and a peak meter.
- `Solo`, `Mono`, `Mute`. Any soloed strip silences the strips that are not soloed. Mono copies the average of the two channels onto both. Mute silences the strip, including its sends.

Device input channels map in pairs onto the strips. System settings can choose which pair feeds each strip. Playback from the transport is mixed into the strip chosen in the transport.

## EQ window

Six bands. Each band has On, a type (Peak, Low shelf, High shelf, Low cut, High cut), frequency, gain, and Q. `Flat` sets every gain to 0 dB. `A` and `B` store and recall two memories for that strip. `All`, `1`, and `2` choose both channels or one channel. The curve drawn under the bands is the combined response. Copy and Paste copy the six bands.

## Reverb and delay windows

Reverb: decay, pre-delay, early amount, and the wet return. Presets `Room`, `Hall`, `Plate`, and `Chamber` set those three values. Delay: time, feedback, the wet return, and four taps with On, time, and gain. The first tap follows the main delay. At zero wet, nothing is added.

## Master

Eight buses labeled `A1` through `A5` and `B1` through `B3`.

Each bus has an `EQ` toggle with its own `Bass` and `Treble`, a fader (−60 dB to +12 dB, default 0 dB), a peak meter, `Mono`, and `Mute`. Right-click the bus `EQ` for the same 6-band EQ and a limiter. The limiter holds a peak above 0 dBFS at 0 dBFS.

`A1` is the hardware output. The other buses stay in the mix and on their meters.

Special FX returns: `Reverb` and `Delay` above the masters. Right-click either one for its window. At zero, nothing is added.

## Transport

- Choose an audio file, then `Play` and `Stop`. Playback mixes into the selected strip.
- `Record` and `Stop` write the selected bus to a WAV file. The time display advances while recording or playing.
- The record bus and the playback strip are labeled choices, defaulting to A1 and Virtual 1.

## Review scores

25 points each: program output, functionality, design, ease of use. Pass is 90. Missing a listed control, or a listed control that does not change the audio, is a functionality miss. A kernel driver, VBAN, or a second hardware output is not a miss.
