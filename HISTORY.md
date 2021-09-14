# G-Audio Release History
### v0.2.1 - 2021-09
* Added `Get Audio File Position.vi` (issue #2)
* Codec detection based on file signature (issue #3)
* UTF-8 strings can be used as the path when opening files
    * LabVIEW's unicode support treats strings as UTF-16 LE, so will require a conversion
* Added `Open Audio File Write (WAV) (UTF-8).vi` to polymorphic VI `Open Audio File Write.vi`
* API Changes
    * Removed Codec input from `Open Audio File Read`, `Quick Load Audio File`, `Get Audio File Info`
    * `Open Audio File Read`, `Quick Load Audio File`, `Get Audio File Info` are now malleable, accepting path type or string type as the Path input to support UTF-8 string paths


### v0.2.0 - 2021-09-05
* Playback and capture


### v0.1.0 - 2021-07-16
* Initial release