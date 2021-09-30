# G-Audio
<p align="center">
  <img width="300" height="115" src="images/g-audio-logo.png">
</p>
A cross-platform LabVIEW library for audio device playback and capture, and for reading and writing audio files.

## What's New?
* Audio playback and capture!
    * Continuous audio input / output
    * Multiple audio backends supported (WASAPI, DirectSound, Core Audio, PulseAudio, ALSA, etc)
    * Simultaneous playback on multiple devices
    * Enumerate audio devices on-the-fly ([a requested feature / bug of lvsound2](https://forums.ni.com/t5/LabVIEW-Idea-Exchange/Update-the-lvsound2-library-Sound-Input-Sound-Output-VIs-to/idi-p/2049098?profile.language=en))

## Features
* Support for audio playback
* Support for audio capture
* Playback and capture using selectable backends (WASAPI, DirectSound, Core Audio, PulseAudio, ALSA, etc)
* Support for reading MP3, FLAC, Ogg Vorbis, and WAV formats
* Support for writing WAV (PCM, IEEE Float, with Sony Wave64 support for large files)
* Unicode path support (UTF-8)
* Cross-platform, 32-bit and 64-bit
* Simple to use API

![The G-Audio library API](images/g-audio-palettes.png?raw=true "The G-Audio library API")

The library is designed to be cross platform, with compiled libraries available for Windows, macOS, and Linux.

## Comparison
### Audio Playback & Capture
Feature                       | G-Audio             | [LabVIEW Sound](https://zone.ni.com/reference/en-XX/help/371361R-01/lvconcepts/soundvis/) | [WaveIO](https://www.zeitnitz.eu/scms/waveio)
------------------------------|---------------------|-------------------------|--------------
Selectable backend            | :heavy_check_mark:  | :x:                     | :heavy_check_mark: (WASAPI, WinMM, ASIO)
Cross-platform                | :heavy_check_mark:  | :heavy_check_mark:¹     | :x: (Windows only)
Runtime device enumeration    | :heavy_check_mark:  | :x:²                    | :heavy_check_mark:
Audio device playback         | :heavy_check_mark:  | :heavy_check_mark:      | :heavy_check_mark:
Audio device capture          | :heavy_check_mark:  | :heavy_check_mark:      | :heavy_check_mark:

¹ *The backend for Linux is OSS, which was deprecated in Linux kernel version 2.5 in favor of ALSA. OSS support is not included out-of-the-box by any of the Linux distributions supported by LabVIEW 2020.*

² *LabVIEW Sound only enumerates devices when lvsound2.dll is loaded, and can't detect newly added or removed devices unless the entire library is unloaded and loaded again. See [this idea exchange entry](https://forums.ni.com/t5/LabVIEW-Idea-Exchange/Update-the-lvsound2-library-Sound-Input-Sound-Output-VIs-to/idi-p/2049098?profile.language=en) for details.*

### Audio Files
Feature                      | G-Audio | [LabVIEW Sound](https://zone.ni.com/reference/en-XX/help/371361R-01/lvconcepts/soundvis/) | [LabVIEW Audio DataPlugin](https://www.ni.com/example/25044/en/)
-----------------------------|---------------------|---------------------|-------------------------
Read WAV (PCM)               | :heavy_check_mark:  | :heavy_check_mark:  | :heavy_check_mark:
Read WAV (IEEE Float)        | :heavy_check_mark:  | :heavy_check_mark:  | :heavy_check_mark:
Read MP3                     | :heavy_check_mark:  | :x:                 | :heavy_check_mark:
Sample Accurate MP3 Length?  | :heavy_check_mark:  | -                   | :x:
Read FLAC                    | :heavy_check_mark:  | :x:                 | :x:
Read Ogg Vorbis              | :heavy_check_mark:  | :x:                 | :x:
Read WMA                     | :x:                 | :x:                 | :heavy_check_mark:
Write WAV (PCM)              | :heavy_check_mark:  | :heavy_check_mark:  | :heavy_check_mark:
Write WAV (IEEE Float)       | :heavy_check_mark:  | :heavy_check_mark:¹ | :heavy_check_mark:¹
Write WAV (64-bit Float)     | :heavy_check_mark:  | :x:                 | :x:
Write Non-WAV formats        | :x:                 | :x:                 | :x:
Large file support (>2GB)    | :heavy_check_mark:² | :x:                 | :x:
Unicode paths                | :heavy_check_mark:³ | :x:                 | :x:
Cross-platform               | :heavy_check_mark:  | :heavy_check_mark:⁴ | :x: (Windows only)

¹ *While LabVIEW Sound and LabVIEW Audio DataPlugin both support writing IEEE Float WAV files, writing the file and then reading it back shows they are **not** lossless. Both the read and write functions appear to be lossy (a file written by LV Sound and read with G-Audio will not be equal to the original data, as will a file written by G-Audio and read by LV Sound). dr_wav's (and by extension, G-Audio's) IEEE Float WAV write and read functions are lossless.*

² *64-bit version of LabVIEW required.*

³ *The path must be a UTF-8 encoded string (not path type) when passed to the Path input of the audio file functions. LabVIEW's file dialog and path controls don't support unicode, so getting the UTF-8 path into LabVIEW will require additional effort. String controls do support unicode with the `UseUnicode=TRUE` flag in **labview.ini** but are encoded as UTF-16 LE, so will require conversion to UTF-8 before use with G-Audio.*

⁴ *While LabVIEW Sound is cross-platform, testing under macOS and Linux shows writing 32-bit IEEE Float is unsupported.*

## License
This library is built using public domain audio decoders and libraries. As such, this library is also made available in the public domain. See [LICENSE](LICENSE) for details.

## Installation
G-Audio is published on [vipm.io](https://www.vipm.io/package/dataflow_g_lib_g_audio/), and can be installed using VI Package Manager (VIPM). The packages are also available as [github releases](https://github.com/dataflowg/g-audio/releases) and can be installed manually using VIPM.

If you want to include the library directly in your project, download or clone the repo and place the [G-Audio folder](https://github.com/dataflowg/g-audio/tree/main/src/LabVIEW/G-Audio) in your source directory, then add `G-Audio.lvlib` to your LabVIEW project.

## Usage
See the example VIs in [Examples](src/LabVIEW/G-Audio/Examples) to write, read, playback, and capture audio files.

Unit tests are included and can be run individually, or with the [AST Unit Tester](https://www.autosofttech.net/documents/ast-unit-tester).

### Supported Data Types
The `Playback Audio`, `Capture Audio`, `Audio File Read`, and `Audio File Write` VIs are malleable, and accept waveform arrays, waveforms, 2D arrays, and 1D arrays with types U8, I16, I32, SGL, and DBL (20 combinations in total). All audio data is processed in its native format, and then converted to the requested format if necessary. For WAV and FLAC files, any conversions are considered lossy, so check the file format before loading to ensure everything is lossless.

![Supported data types](images/g-audio-data-types.png?raw=true "Supported data types")

#### Malleable VIs and broken wires
If a malleable VI has broken wire inputs and errors about unsupported types, even though the type is supported, try hold Ctrl and click the run arrow. This will force LabVIEW to recompile the VI, and should hopefully fix those broken wires.

## Playback and Capture Buffering
The underlying mechanism for playback and capture is a callback made from the backend, where it requests the next block of audio data to be sent to the audio device, or the next available block read from the audio device.

The diagram below shows the audio data flow during playback. The functions `Playback Audio.vim` and the backend callback run asynchronously. The ring buffer sits between the two, and keeps track of the next block of audio to be read (by the callback) and written (from LabVIEW). It's critical sufficient audio data is written to the ring buffer during playback, and read from the buffer during capture to ensure there are no audio glitches.

Definitions used when referring to the diagram:

Term   | Definition
-------|--------
Sample | Single unit of audio data, typically an I16 or SGL
Frame  | Group of samples equal to number of channels
Period | 10ms of audio data (sample rate / 100). Typically 441 or 480 frames.

![The G-Audio library API](images/playback.png?raw=true "The G-Audio library API")

The *period* size should be regarded as the minimum buffer size when configuring the audio device. Note that the number of frames requested in the callback routine isn't necessarily fixed, and can be larger than a single *period*.

## Compiling
Under Windows, Microsoft Visual Studio Community 2019 is used to compile and test the DLL called by LabVIEW.

Under macOS, XCode 11.5 is used to compile the shared framework.

Under Linux, run the `make.sh` script to compile the shared object library, or manually compile with the command `g++ -shared -fPIC -o g_audio_x64.so *.cpp -lm -lpthread -ldl`.

## Libraries
This library uses the following public domain libraries. Massive thanks to these authors.
* [miniaudio.h](https://github.com/mackron/miniaudio) by mackron
* [dr_flac.h](https://github.com/mackron/dr_libs) by mackron
* [dr_wav.h](https://github.com/mackron/dr_libs) by mackron
* [minimp3](https://github.com/lieff/minimp3) by lieff
* [stb_vorbis.c](https://github.com/nothings/stb) by nothings
* [thread.h](https://github.com/mattiasgustavsson/libs) by mattiasgustavsson
