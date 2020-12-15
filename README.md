# G-Audio
<p align="center">
  <img width="300" height="115" src="g-audio-logo.png">
</p>
A LabVIEW library for reading and writing audio files. Currently supports reading MP3, FLAC, Ogg Vorbis, and WAV encoded audio files, and writing WAV files. The library has 32-bit and 64-bit support, though only the decoders for FLAC and WAV utilize the full 64-bit address space.

## Features
* Support for reading MP3, FLAC, Ogg Vorbis, and WAV formats
* Support for writing WAV (PCM, IEEE Float, with Sony Wave64 support for large files)
* UTF-8 path support
* Cross-platform
* Thread-safe
* Simple to use API

![The G-Audio library API](g-audio-library.png?raw=true "The G-Audio library API")

The library is designed to be cross platform, with compiled libraries available for Windows and Linux. MacOS should also work (it builds with zig c++), but is untested.

## Comparison
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
UTF-8 path support           | :heavy_check_mark:³ | :x:                 | :x:
Cross-platform               | :heavy_check_mark: (Win + Linux tested)   | :heavy_check_mark:⁴   | :x: (Windows only)

¹ *While LabVIEW Sound and LabVIEW Audio DataPlugin both support writing IEEE Float WAV files, writing the file and then reading it back shows they are **not** lossless. Both the read and write functions appear to be lossy (a file written by LV Sound and read with G-Audio will not be equal to the original data, as will a file written by G-Audio and read by LV Sound). dr_wav's (and by extension, G-Audio's) IEEE Float WAV write and read functions are lossless.*

² *64-bit version of LabVIEW required*

³ *The underlying library supports UTF-8 paths, and is best used with LabVIEW NXG. Wrappers and workarounds for LabVIEW's unicode support may be added in future.*

⁴ *While LabVIEW Sound is cross-platform, testing under Linux showed writing 32-bit IEEE Float didn't work.*

## License
This library is built using public domain audio decoders and libraries. As such, this library is also made available in the public domain. See [LICENSE](LICENSE) for details.

## Usage
See the example VIs in [Examples](src/LabVIEW/Examples) to write, read, and playback audio files.

Unit tests are included and can be run individually, or with the [AST Unit Tester](https://www.autosofttech.net/documents/ast-unit-tester).

### Supported Data Types
The `Audio File Read` and `Audio File Write` VIs are malleable, and accept waveform arrays, waveforms, 2D arrays, and 1D arrays with types U8, I16, I32, SGL, and DBL (20 combinations in total). All audio data is read in its native format, and then converted to the requested format if necessary. For WAV and FLAC files, any conversions are considered lossy, so check the file format before loading to ensure everything is lossless.

![Supported data types](g-audio-data-types.png?raw=true "Supported data types")

## Compiling
Under Windows, Microsoft Visual Studio Community 2019 is used to compile and test the DLL called by LabVIEW.

For Linux and MacOS, cross-compilation is performed from Windows using [zig c++](https://andrewkelley.me/post/zig-cc-powerful-drop-in-replacement-gcc-clang.html). Grab the [latest zig](https://ziglang.org/download/) and place a copy in `src\C++\zig`, then run the batch file [zig-make.bat](src\C++\zig-make.bat). This will build the libraries and place them in `src\LabVIEW\G-Audio\lib`.

## Libraries
This library uses the following public domain libraries:
* [minimp3](https://github.com/lieff/minimp3) by lieff
* [stb_vorbis.c](https://github.com/nothings/stb) by nothings
* [dr_flac.h](https://github.com/mackron/dr_libs) by mackron
* [dr_wav.h](https://github.com/mackron/dr_libs) by mackron
* [thread.h](https://github.com/mattiasgustavsson/libs) by mattiasgustavsson
