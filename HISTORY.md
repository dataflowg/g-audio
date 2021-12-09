# G-Audio Release History
### v0.4.0 - 2021-12-xx
* Raspberry Pi / LINX support
* Advanced device configuration (exclusive mode, period size, num periods)
* New VI `Get Audio Device Configuration.vi` used to query info of an already configured device
* `Query Audio Devices.vi` returns additional device info (default device, min/max sample rate, min/max channels, supported formats)
* Prevent duplicate device configuration on PulseAudio backend (issue #10), warn of duplicate device config on other backends

##### API Changes
* `Configure Audio Device.vi`:
    * Added `Audio Device Advanced Config.ctl` input
    * Added `Audio Device Config.ctl` and `Audio Device Advanced Config.ctl` outputs which contain the device's actual configuration, and can differ from the requested configuration
* Exclusive Mode flag moved from `Audio Device Config.ctl` to `Audio Device Advanced Config.ctl`



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