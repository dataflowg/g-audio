# G-Audio Release History
### v0.5.0 - 2023-07-xx
* New `Optimize Latency` config flag for audio devices, to help avoid dropouts with smaller buffer sizes. Setting enables:
    * Realtime thread priority for audio worker thread
    * "Pro Audio" thread characteristic on WASAPI backend
* New `Latency Loopback Example.vi` demonstrating latency reduction techniques
    * See `README.md` for additional info on reducing latency
* Automatically close any open audio files on VI abort
* Fix hang when clearing audio device while it is in process of stopping (#20)
* Fix for parsing empty ID3v2 comment
* Fix loading 8-bit jpeg album art

##### API Changes
* `Configure Audio Device.vi`:
    * `Audio Device Advanced Config.ctl` adds _Optimize Latency_ flag



### v0.4.0 - 2022-07-12
* Raspberry Pi / LINX support
* Support for reading metadata tags (ID3v2, ID3v1, Vorbis Comments, RIFF INFO)
* Support for reading embedded artwork
* Advanced device configuration (exclusive mode, period size, num periods)
* Device volume adjustment (feature request #13)
* Get the configuration settings of a configured device (compare requested config vs actual config)
* Query detailed device info (default device, native sample rates, native channels, native formats)
* Warn of duplicate device config
* Update miniaudio (fixes #10)
* Fix memory leak when using UTF-16 converted file name
* [VIPM] Reduced package installation time
* [VIPM] Fix install error 5000 under macOS (#15)

##### New APIs
* `Get Audio File Tags.vi`
* `Set Audio Device Volume.vi`
* `Get Audio Device Volume.vi`
* `Get Audio Device Configuration.vi`

##### API Changes
* `Configure Audio Device.vi`:
    * Added `Audio Device Advanced Config.ctl` input
    * Added `Audio Device Config.ctl` and `Audio Device Advanced Config.ctl` outputs which contain the device's actual configuration, and can differ from the requested configuration
* Exclusive Mode flag moved from `Audio Device Config.ctl` to `Audio Device Advanced Config.ctl`
* `Query Audio Devices.vi` returns additional device info (default device, native sample rates, native channels, native formats)



### v0.3.1 - 2021-12-17
* Fix broken example VIs when VIPM package installed to LabVIEW 64-bit (issue #12)



### v0.3.0 - 2021-11-09
* Multi-channel audio mixer
* Loopback audio capture (WASAPI only)
* Sample pad & music visualizer examples
* Fix malleable VI broken wires (issue #4)
* Fix mp3 info memory error (issue #8)
* Upmix / downmix audio data to match audio device (issue #9)
* Auto cleanup audio devices on VI abort



### v0.2.1 - 2021-09-23
* Added `Get Audio File Position.vi` (issue #2)
* Codec detection based on file signature (issue #3)
* UTF-8 strings can be used as the path when opening files
    * LabVIEW's unicode support treats strings as UTF-16 LE, so will require a conversion
* Automatically start audio device during playback and capture (issue #7)
* Fix misc. malleable VI issues (issue #4, #6)

##### API Changes
* Removed *Codec* input from `Open Audio File Read.vi`, `Quick Load Audio File.vi`, `Get Audio File Info.vi`
* `Open Audio File Read.vi`, `Quick Load Audio File.vi`, `Get Audio File Info.vi` are now polymorphic, accepting path type or string type as the Path input to support UTF-8 string paths



### v0.2.0 - 2021-09-05
* Playback and capture



### v0.1.0 - 2021-07-16
* Initial release