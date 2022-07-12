# Building and Packaging G-Audio

## Development Environment

The OS and software version selection is dependent on the versions supported by LabVIEW 2020, which is the LabVIEW version used to develop G-Audio. All development is done in virtual machines, except for Raspberry Pi which is on a physical Raspberry Pi 3B+.

OS | LV2020 Compatible Version | Link
---|---------------------------|------
Windows | Windows 8.1, 10 | [Compatibility table]( https://www.ni.com/en-us/support/documentation/compatibility/17/labview-and-microsoft-windows-compatibility.html)
macOS | macOS 10.14, 10.15 | [Compatibility table]( https://www.ni.com/en-us/support/documentation/compatibility/18/labview-and-macos-compatibility.html)
Linux | OpenSUSE 15.0, 15.1; CentOS 7, 8; RHEL 7, 8 | [Compatibility table]( https://www.ni.com/en-us/support/documentation/compatibility/20/labview-and-linux-os-compatibility.html)

Virtualization uses VMWare Player 16 patched with [Auto-Unlocker](https://github.com/paolo-projects/auto-unlocker/releases).

### Windows 10 VM

This is the primary dev environment.

Software | Links | Notes
---------|-------|------
LabVIEW 2020 Community edition for Windows (32-bit) | [Download](https://www.ni.com/en-us/support/downloads/software-products/download.labview.html#343639) |
Visual Studio 2019 Community edition | [Download](https://visualstudio.microsoft.com/vs/older-downloads/#visual-studio-2019-and-other-products) |
VIPM 2020.3 (build 2540) | [Download](https://www.vipm.io/desktop/installer/vipm-20.3.2540-windows-setup.exe/) | Don't use 2021, it has a build bug when parsing a VI in the LINX toolkit.

### macOS 10.14 VM

macOS ISO available on [archive.org](https://archive.org/details/macos-collection).

Software | Links | Notes
--------------|-------|-------
LabVIEW 2020 SP1 Community edition for macOS (64-bit) | [Download](https://www.ni.com/en-us/support/downloads/software-products/download.labview.html#370210) |
Xcode 11.3.1 | [Download](https://developer.apple.com/services-account/download?path=/Developer_Tools/Xcode_11.3.1/Xcode_11.3.1.xip) | A comprehensive list of Xcode versions can be found on [xcodereleases.com](https://xcodereleases.com/)
VIPM 2020.3 (build 2540) | [Download](https://www.vipm.io/desktop/installer/vipm-20.3.2540-mac.zip/) |

### CentOS 8 VM

CentOS 8 is no longer supported by the maintainers, but is supported by LabVIEW 2020. After installing it, run the following commands to ensure the `yum` package manager can see the correct repos.

```
cd /etc/yum.repos.d/
sed -i 's/mirrorlist/#mirrorlist/g' /etc/yum.repos.d/CentOS-*
sed -i 's|#baseurl=http://mirror.centos.org|baseurl=http://vault.centos.org|g' /etc/yum.repos.d/CentOS-*
```

Software | Links | Notes
--------------|-------|-------
LabVIEW 2020 SP1 Community edition for Linux (64-bit) | [Download](https://www.ni.com/en-us/support/downloads/software-products/download.labview.html#370209) | The community edition ISO includes the pro LabVIEW version, no activation required. It isn't installed by the install script, but can be installed manually.
VIPM 2017.0 (build 2018) | [Download](https://www.vipm.io/desktop/installer/vipm-17.0.2018-linux.tar.gz/) | See below for installation notes.

#### Running VIPM on Linux

This was surprisingly hard. The Linux dev environment was originally OpenSUSE. After a series of issues getting VIPM to successfully launch, it then simply would not connect to LabVIEW. The OS was then changed to CentOS 8 which works with VIPM thanks to the help of [James McNally's post](https://forums.vipm.io/topic/2988-vipm-2017-on-centos-8/) on the VIPM forum.

1. Install support for the 32 bit LabVIEW RTE on 64 bit by running the following:

```
su
yum update libstdc++
yum install glibc.i686 libstdc++.i686 libXinerama.i686 libGL.so.1
```

2. Extract the VIPM tar.gz file, then edit `LabVIEW2015SP1RTE_Linux/INSTALL`. Find the line which begins with *RPMOPT* and add `--nodigest` at the end of the string. Save and close the file.

3. Follow the installation instructions in `instructions.txt` in the root of the VIPM archive.

4. To run VIPM, run the commands:

```
xhost si:localuser:root
su
cd /usr/local/JKI/VIPM
./vipm
```

VIPM should successfully launch, but may not be able to start LabVIEW 2020 Community edition, instead prompting with the activation dialog. This is due to LabVIEW only being activated under the user account, and not the root account which VIPM is required to run under. Attempting to activate LabVIEW under the root account then fails, as Firefox refuses to run as root user.

The workaround is to extract the LabVIEW pro / full rpm file from the ISO.

```
sudo rpm -i labview-2020-profull-exe-20.5.0.49152-0+f0.x86_64.rpm
```

### Raspberry Pi

Software | Links | Notes
---------|-------|------
Raspberry Pi OS | [Download](https://www.raspberrypi.com/software/operating-systems/#raspberry-pi-os-32-bit) | Desktop or Lite version
LINX library | [Download](https://www.vipm.io/package/ni_labview_linx_toolkit/) | This is already included with the community editions of LabVIEW. Use the *Tools > MakerHub > LINX > LINX Target Configuration...* wizard to install LINX to the Raspberry Pi.
ALSA and build packages | | See instructions below.

#### Package installation

SSH to the Raspberry Pi after installing the LINX environment, then enter the following commands to install the packages required for playing audio and building the G-Audio library.

```
sudo schroot -r -c lv
opkg update
opkg install alsa-lib
opkg install packagegroup-core-buildessential
opkg install --force-depends libc6-dev
opkg install --force-depends libgcc-s-dev
opkg install libstdc++-staticdev
opkg install git
opkg install alsa-lib-dev
exit
```

## Building the Shared Library

### Windows

Open the solution located at `src\C++\g_audio.sln`, then perform a batch build of both 32-bit and 64-bit releases. The resulting `g_audio_32.dll` and `g_audio_64.dll` files will be copied to `src\LabVIEW\G-Audio\lib\`.

### macOS

Open the Xcode project file location at `src/C++/xcode/g_audio_64.xcodeproj`. Build the library (hit build and run, or command+B). The resulting `g_audio_64.framework` library will be copied to `src/LabVIEW/G-Audio/lib/` with the following script:

```
cd $BUILT_PRODUCTS_DIR
rm -f g_audio_64.framework.zip
zip --symlinks -r g_audio_64.framework.zip g_audio_64.framework
cp -R g_audio_64.framework $PROJECT_DIR/../../LabVIEW/G-Audio/lib/
mv g_audio_64.framework.zip $PROJECT_DIR/../../LabVIEW/G-Audio/lib/
```

Note a zip file of the framework will also be created, reserving the symlinks within the framework folder. This zip file is included as part of the VIPM package, and is extracted during the VIPM package post installation.

### Linux

Change directory to `src/C++`, then run `chmod 755 make.sh; ./make.sh`. The resulting `g_audio_64.so` file will be copied to `src/LabVIEW/G-Audio/lib/`. The contents of make.sh are below:

```
#!/bin/bash
OUTPUT_PATH=../LabVIEW/G-Audio/lib

mkdir -p $OUTPUT_PATH
g++ -shared -fPIC -o g_audio_64.so *.cpp -lm -lpthread -ldl -O3
mv g_audio_64.so $OUTPUT_PATH
```

### Raspberry Pi

Ensure the Raspberry Pi has the necessary packages installed (alsa-lib, etc). SSH to the Pi, then issue the following commands to download the source code and build it.

```
sudo schroot -r -c lv
git clone https://github.com/dataflowg/g-audio
cd g-audio/src/C++
chmod 755 make_linx.sh
./make_linx.sh
exit
```

The resulting `g_audio_32.so` library will be placed in `/usr/lib` within the chroot (`/srv/chroot/labview/usr/lib`). This file should then be copied from the Pi and placed in `src\LabVIEW\G-Audio\lib\LINX` on the dev VM.

The contents of `make_linx.sh` are below:

```
#!/bin/bash
OUTPUT_PATH=/usr/lib

g++ -shared -fPIC -o g_audio_32.so *.cpp -lm -lpthread -ldl -std=c++11 -mfpu=neon -mfloat-abi=softfp -O3
mv g_audio_32.so $OUTPUT_PATH
```

## Packaging For VIPM

The packaging process for VIPM requires multiple steps to deal with issues relating to the combination of operating system targets and malleable VIs.

1. Open the VIPM build spec from `src\LabVIEW\VIPM\G-Audio.vipb`
2. Set the version number, then create the build.
3. Once the build has completed, open and run `src\LabVIEW\VIPM\Generate Post-Install.vi`. Point it to the newly created `.vip` file. This VI will then create a zip file in `src\LabVIEW\Post Install` containing the original versions of all the VIs and VIMs found in the VIPM package.
4. With the G-Audio build spec still open in VIPM, refresh the file list in Source File Settings. Set the version number back to the previously set value then build the package again, overwriting the existing package. This second rebuild is required to include the newly created zip file of original source VIs.

In theory the above could be performed using VIPM's packaging VIs, but they don't allow setting the major version to zero, while the user interface does. Once G-Audio reaches version 1.0 maybe this process can be revisited.

### Why so many steps?

Each *Call Library Function Node* (CLFN) in G-Audio is configured to call a library named `g_audio_*.*`. When search for the library, LabVIEW will automatically replace the two asterisks with the target bitness (32 or 64), and the OS specific library extension (dll, framework, or so). The final set of library names is below:

OS | Bitness | Library Name
---|---------|-------------
Windows | 32 | g_audio_32.dll
Windows | 64 | g_audio_64.dll
macOS | 64 | g_audio_64.framework
Linux | 64 | g_audio_64.so
Raspberry Pi | 32 | g_audio_32.so

When VIPM builds a package, it will replace the wildcards in the CLFN path with the host bitness and library type. When the package is installed on a target with a different bitness or OS, the VIs will be broken. See [this VIPM KB article](https://support.vipm.io/hc/en-us/articles/360053941572-VI-Package-Builder-removes-cross-platform-wildcards-from-Call-Library-Function-nodes-during-the-build-process) for more info.

The suggested workaround is to include a post-install script which updates all of the CLFN paths with the original wildcards. This works for VIs and top level malleable VIs, but does not work for malleable VIs on the block diagram. These malleable VIs are saved by LabVIEW as an instance VI within the calling VI. [Additional scripting](https://forums.vipm.io/topic/2427-call-library-function-node-32-and-64-bit/?do=findComment&comment=10107) is required to recursively identify instance VIs and update the CLFN paths. This works, but is incredibly slow.

Rather than trying to find and undo all of the CLFN path changes during the post-install stage, the G-Audio package includes a copy of the original, unmodified VIs along with the modified versions. After the modified versions are installed, a zip containing the original versions is extracted straight over the top of everything. Much faster and less error prone than the scripting approach.