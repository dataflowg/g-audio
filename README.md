# G-Audio
<p align="center">
  <img width="300" height="115" src="images/g-audio-logo.png">
</p>
<p align="center">
A cross-platform LabVIEW library for audio device playback and capture, and for reading and writing audio files.
</p>

<p align="center">
    <a href="#whats-new">What's New?</a> -
    <a href="#features">Features</a> -
    <a href="#installation">Installation</a> -
    <a href="#usage">Usage</a> -
	<a href="#compiling">Compiling</a> -
	<a href="#comparison">Comparison</a> - 
	<a href="#license">License</a> - 
	<a href="#acknowledgments">Acknowledgments</a>
</p>

## <a id="whats-new"></a>What's New?
* Raspberry Pi / LINX support!
    * See the [Installation](#installation) section to get started.
* Metadata tag reading support (ID3v2, ID3v1, Vorbis Comments, RIFF INFO)
* Advanced device configuration options

## <a id="features"></a>Features
* Support for audio playback and capture, including loopback capture
* Playback and capture using selectable backends (WASAPI, DirectSound, Core Audio, PulseAudio, ALSA, etc)
* Multi-channel audio mixer
* Read MP3, FLAC, Ogg Vorbis, and WAV formats
* Read metadata tags (ID3v2, ID3v1, Vorbis Comments, RIFF INFO)
* Write WAV format (PCM and IEEE Float, with Sony Wave64 support for large files)
* Unicode path support (UTF-8)
* Cross-platform (Windows, macOS, Linux, Raspberry Pi / LINX), 32-bit and 64-bit
* Simple to use API

![The G-Audio library API](images/g-audio-palettes.png?raw=true "The G-Audio library API")

## <a id="installation"></a>Installation
G-Audio is published on [vipm.io](https://www.vipm.io/package/dataflow_g_lib_g_audio/), and can be installed using VI Package Manager (VIPM). The packages are also available as [github releases](https://github.com/dataflowg/g-audio/releases) and can be installed manually using VIPM.

If you want to include the library directly in your project, download or clone the repo and place the [G-Audio folder](https://github.com/dataflowg/g-audio/tree/main/src/LabVIEW/G-Audio) in your source directory, then add `G-Audio.lvlib` to your LabVIEW project.

### Raspberry Pi / LINX Installation
Before beginning, ensure your board has the LINX toolkit installed and SSH is enabled.

#### 1. Install G-Audio
Download and install the VIPM package from the github project dev branch:

https://github.com/dataflowg/g-audio/raw/dev/src/LabVIEW/VIPM/dataflow_g_lib_g_audio-0.3.0.2-dev.vip

#### 2. Install ALSA to chroot
SSH into the LINX target (using PuTTy or similar) and run the commands:
```
sudo schroot -r -c lv
opkg update
opkg install alsa-lib
exit
```
#### 3. Copy `g_audio_32.so` to the target
*Note: This library has been compiled for armv7a processors. Other architectures may need to build the library before it can be used. See the [Compiling](#compiling) section.*

SCP / SFTP to the LINX target (using WinSCP or similar) and copy the library file located in `<vi.lib>\Dataflow_G\G-Audio\lib\LINX\g_audio_32.so` to the `/srv/chroot/labview/usr/lib` folder.

Alternatively, SSH to the LINX target and download the library direct from github:
```
cd /srv/chroot/labview/usr/lib
wget https://github.com/dataflowg/g-audio/raw/dev/src/LabVIEW/G-Audio/lib/LINX/g_audio_32.so
```

#### 4. Make some noise!
Copy some audio files to your device using SCP or SFTP and place them in `/home/pi` or your preferred location. Create a LabVIEW project, add your LINX target, then the examples in the Add-ons >> G-Audio >> Examples >> LINX palette.

If you need to adjust the volume output, SSH into the LINX target and run `alsamixer`. Press F6, then select the physical audio device (not Default). Use the up and down arrow keys to adjust the volume, or use numbers 1-9 to set the volume in 10% increments. Press Esc to save and exit the mixer.

## <a id="usage"></a>Usage
See the example VIs in [Examples](src/LabVIEW/G-Audio/Examples) to write, read, playback, and capture audio files.

Unit tests are included and can be run individually, or with the [AST Unit Tester](https://www.autosofttech.net/documents/ast-unit-tester).

### Supported Data Types
The `Playback Audio`, `Capture Audio`, `Audio File Read`, and `Audio File Write` VIs are malleable, and accept waveform arrays, waveforms, 2D arrays, and 1D arrays with types U8, I16, I32, SGL, and DBL (20 combinations in total). All audio data is processed in its native format, and then converted to the requested format if necessary. For WAV and FLAC files, any conversions are considered lossy, so check the file format before loading to ensure everything is lossless.

![Supported data types](images/g-audio-data-types.png?raw=true "Supported data types")

#### Malleable VIs and broken wires
If a malleable VI has broken wire inputs and errors about unsupported types, even though the type is supported, try hold Ctrl and click the run arrow. This will force LabVIEW to recompile the VI, and should hopefully fix those broken wires.

### Supported Metadata Tags
G-Audio supports reading ID3v2 and ID3v1 tags from MP3 files, Vorbis Comments from FLAC and Ogg Vorbis files, and RIFF INFO data from WAV files. All tag data is returned as a string array from `Read Audio File Tags.vi`, where each string is in the form `FIELD=Value`. Depending on the tag format, `FIELD` is directly from the tag (in the case of Vorbis Comments), or mapped to a commonly named field.

Field | Description | ID3v2 | ID3v1 | RIFF INFO
------|-------------|-------|-------|-----------
TITLE | The title of the track. | `TIT2`, `TT2` | Title | `INAM`
ARTIST | The track artist. | `TPE1`, `TP1` | Artist | `IART`
ALBUMARTIST | The album artist. | `TPE2`, `TP2` | :x: | :x:
ALBUM | The album title. | `TALB`, `TAL` | Album | `IPRD`
GENRE | The track's genre. | `TCON`, `TCO` | Genre ID | `IGNR`
DATE | The release date, typically the year. | `TYER`, `TYE` | Year | `ICRD`
TRACKNUMBER | The track's number in an album. | `TRCK`, `TRK` (**nn** / NN) | Comment[29] | `ITRK`
TRACKTOTAL | The total number of tracks in an album. | `TRCK`, `TRK` (nn / **NN**) | :x: | :x:
DISCNUMBER | The disc number within an album. | `TPOS`, `TPA` (**nn** / NN) | :x: | :x:
DISCTOTAL | The total number of discs in an album. | `TPOS`, `TPA` (nn / **NN**) | :x: | :x:
BPM | Beats Per Minute of the track. | `TBPM`, `TBP` | :x: | :x:
COMMENT | Notes and comments on the track. | `COMM`, `COM` | Comment[0-27] | `ICMT`

The tag field mapping is based on the [Tag Mapping article](https://wiki.hydrogenaud.io/index.php?title=Tag_Mapping) on the hydrogenaudio wiki.

## <a id="compiling"></a>Compiling
Under Windows, Microsoft Visual Studio Community 2019 is used to compile and test the DLL called by LabVIEW.

Under macOS, XCode 11.5 is used to compile the shared framework.

Under Linux, run the `make.sh` script to compile the shared object library, or manually compile with the command `g++ -shared -fPIC -o g_audio_64.so *.cpp -lm -lpthread -ldl`.

Under Raspberry Pi / LINX, run the following commands from SSH:
```
sudo schroot -r -c lv
opkg update
opkg install packagegroup-core-buildessential
opkg install --force-depends libc6-dev
opkg install --force-depends libgcc-s-dev
opkg install libstdc++-staticdev
opkg install git
opkg install alsa-lib-dev
git clone https://github.com/dataflowg/g-audio
cd g-audio/src/C++
git checkout dev
g++ -shared -fPIC -o g_audio_32.so *.cpp -lm -lpthread -ldl -std=c++11 -mfpu=neon -mfloat-abi=softfp
cp g_audio_32.so /usr/lib
exit
```

## <a id="comparison"></a>Comparison
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
Read metadata tags           | :heavy_check_mark:  | :x:                 | :x:
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

## Playback and Capture Buffering
The underlying mechanism for playback and capture is a callback made from the backend, where it requests the next block of audio data to be sent to the audio device, or the next available block read from the audio device.

The diagram below shows the audio data flow during playback. The function `Playback Audio.vim` and the backend callback run asynchronously. The ring buffer sits between the two, and keeps track of the next block of audio to be read (by the callback) and written (from LabVIEW). It's critical sufficient audio data is written to the ring buffer during playback, and read from the buffer during capture, to ensure there are no audio glitches.

Definitions used when referring to the diagram:

Term   | Definition
-------|--------
Sample | Single unit of audio data, typically an I16 or SGL
Frame  | Group of samples equal to number of channels
Period | 10ms of audio data (sample rate / 100). Typically 441 or 480 frames.

![The G-Audio library API](images/playback.png?raw=true "The G-Audio library API")

The *period* size should be regarded as the minimum buffer size when configuring the audio device. Note that the number of frames requested in the callback routine isn't necessarily fixed, and can be larger than a single *period*.

## <a id="license"></a>License
This library is built using public domain audio decoders and libraries. As such, this library is also made available in the public domain. See [LICENSE](LICENSE) for details.

## <a id="acknowledgments"></a>Acknowledgments
This library uses the following public domain libraries. Massive thanks to these authors.
* [miniaudio.h](https://github.com/mackron/miniaudio) by mackron
* [dr_flac.h](https://github.com/mackron/dr_libs) by mackron
* [dr_wav.h](https://github.com/mackron/dr_libs) by mackron
* [minimp3](https://github.com/lieff/minimp3) by lieff
* [stb_vorbis.c](https://github.com/nothings/stb) by nothings
* [thread.h](https://github.com/mattiasgustavsson/libs) by mattiasgustavsson
