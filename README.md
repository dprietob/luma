<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

[![License: GPL-3.0-or-later](https://img.shields.io/badge/License-GPL--3.0--or--later-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Flathub Installs](https://img.shields.io/flathub/downloads/io.github.dprietob.luma?label=Installs)][Flathub]

<img src="assets/icons/luma_256.png?raw=true" width="128" alt="Luma icon">

# Luma

A real-time multitrack audio mixer for Linux.

![Luma](screenshots/main.png?raw=true)

Luma plays many audio tracks at once and lets you control each one
independently, with the look and feel of a professional studio tool. It is a
focused native app for sound technicians and broadcast operators.

Luma lets you:

- Play WAV, MP3, OGG and FLAC files on independent channels, all at once
- Control volume, pan, looping and timed fade in / fade out per channel
- Shape each channel with effects — reverb, delay, distortion, pitch and speed
  — plus a graphic equalizer
- Route every channel to a main or an auxiliary output, configured separately
- Save up to 10 presets (scenes) and switch between them instantly
- Automate playback with node-graph flows: chain channels with triggers and
  crossfades for hands-free sequences
- Keep your work in portable projects, with imported tracks copied alongside so
  nothing breaks if you move or rename the originals

It is available in English, Spanish, French, German, Italian, Portuguese and
Dutch, with a dense dark interface designed for low-light stage and studio use.

## A native app for Linux

Luma is built with C++17 and Qt 6 (Qt Quick / QML) and talks to the audio
hardware through PortAudio, so it runs on any ALSA / PulseAudio / PipeWire
setup. Audio decoding uses libsndfile (WAV/OGG/FLAC) and libmpg123 (MP3), and
independent pitch/tempo shifting uses SoundTouch.

## Your data stays on your device

Luma works with local projects. Each project is a self-contained folder with a
`.luma` file (JSON) and a `tracklist/` directory where imported tracks are
copied. Track paths are stored relative to the project, so a project folder is
fully portable — move it or back it up and it just works. App preferences such
as window size, audio cache size and recent projects are stored natively with
QSettings. There are no accounts and no online services; nothing leaves your
computer.

## Installing

### Debian and Ubuntu

Download `luma_<version>_amd64.deb` from the [latest release][releases] and
install it with `apt`, which pulls in the Qt and audio libraries it needs:

```shell
sudo apt install ./luma_1.0.0-1_amd64.deb
```

Luma needs Qt 6.8 or newer, so the package targets **Debian 13 (trixie) or
newer** and **Ubuntu 25.04 or newer**. On Ubuntu 24.04 LTS (Qt 6.4) use the
AppImage instead.

### Fedora

Download the `.rpm` that matches your Fedora release from the
[latest release][releases] and install it with `dnf`:

```shell
sudo dnf install ./luma-1.0.0-1.fc44.x86_64.rpm
```

QML type registration links against Qt's private API, so each package is tied to
the Qt version of the Fedora release it was built on. There is one `.rpm` per
supported release (`fc43`, `fc44`); pick yours, or use the AppImage.

### Arch Linux

Build and install the tagged release from source with the provided `PKGBUILD`:

```shell
cd packaging/arch && makepkg -si
```

### AppImage (any distribution)

A single self-contained file that bundles Qt and the audio libraries. It runs on
any distribution with glibc 2.35 or newer — Ubuntu 22.04+, Debian 12+,
Fedora 36+, Arch and derivatives:

```shell
chmod +x Luma-1.0.0-x86_64.AppImage
./Luma-1.0.0-x86_64.AppImage
```

Only the libraries that must come from the host are left out — the graphics
stack (Mesa / libglvnd), ALSA, fontconfig and D-Bus — so any graphical Linux
install already has what it needs. Audio is routed through ALSA, which is what
PipeWire and PulseAudio expose, so there is nothing to configure.

To get a menu entry and the `.luma` file association, install the AppImage with
a tool such as [Gear Lever][gearlever], or use a native package.

### Flatpak

Build and install from the manifest in this repository:

```shell
flatpak install flathub org.kde.Platform//6.11 org.kde.Sdk//6.11
flatpak-builder --user --install --force-clean build-dir io.github.dprietob.luma.yml
flatpak run io.github.dprietob.luma
```

## Building from source

Luma needs a C++17 compiler, CMake >= 3.21, Ninja, **Qt >= 6.8** (Qt Quick and
Qt SVG) and the development files of PortAudio, libsndfile, libmpg123 and
SoundTouch.

Install the dependencies for your distribution:

```shell
# Debian 13+ / Ubuntu 25.04+
sudo apt install build-essential cmake ninja-build pkg-config \
    qt6-base-dev qt6-declarative-dev qt6-l10n-tools qt6-tools-dev-tools \
    qt6-svg-plugins qml6-module-qtquick-controls qml6-module-qtquick-dialogs \
    qml6-module-qtquick-layouts qml6-module-qtqml-models \
    qml6-module-qtquick-window qml6-module-qttest \
    portaudio19-dev libsndfile1-dev libmpg123-dev libsoundtouch-dev
```

```shell
# Fedora 42+
sudo dnf install gcc-c++ cmake ninja-build pkgconf-pkg-config \
    qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtshadertools qt6-qtsvg \
    qt6-linguist portaudio-devel libsndfile-devel mpg123-devel soundtouch-devel
```

```shell
# Arch Linux
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-declarative \
    qt6-svg qt6-shadertools qt6-tools portaudio libsndfile mpg123 soundtouch
```

Then configure, build and run:

```shell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/luma
```

Run the test suite with `ctest --test-dir build`.

### Installing manually

The build installs the binary plus the desktop entry, icons, AppStream metadata
and the `.luma` MIME type, so Luma shows up in the application menu and opens
project files from the file manager:

```shell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
sudo cmake --install build
```

Some desktops need their caches refreshed afterwards:

```shell
sudo update-desktop-database /usr/share/applications
sudo update-mime-database /usr/share/mime
sudo gtk-update-icon-cache -f /usr/share/icons/hicolor
```

To install without root, point the prefix at your home directory — make sure
`~/.local/bin` is in your `PATH`:

```shell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="${HOME}/.local"
cmake --build build && cmake --install build
```

To uninstall, remove the files the install step recorded:

```shell
sudo xargs rm -v < build/install_manifest.txt
```

### Building the distributable packages

Each package is built inside a container of its target distribution, so the
binary links against that distribution's libraries. The host only needs podman
(or docker):

```shell
./packaging/build-packages.sh             # deb + rpm + appimage, into dist/
./packaging/build-packages.sh rpm         # just one target
FEDORA_VERSION=43 ./packaging/build-packages.sh rpm   # a different Fedora release
DEBIAN_SUITE=forky ./packaging/build-packages.sh deb  # a different Debian suite
ENGINE=docker ./packaging/build-packages.sh
```

The `.deb` and `.rpm` are produced with CPack (see [cmake/Packaging.cmake][cpack]),
and the AppImage with linuxdeploy (see [packaging/build-appimage.sh][appimage]).
Cutting a release means tagging the version and refreshing the `PKGBUILD`
(`pkgver` plus `updpkgsums` for the new source tarball's checksum), which is the
only recipe that is not built here.

The AppImage is built on Ubuntu 22.04 against the official Qt binaries, since no
distribution with an older glibc ships Qt 6.8, and with its own PortAudio built
without JACK so it does not depend on a JACK server being installed. Pushing a
version tag runs the same scripts in CI and publishes the results to the GitHub
release.

## License

Luma is released under the GPL-3.0-or-later license.

[Flathub]: https://flathub.org/apps/io.github.dprietob.luma
[releases]: https://github.com/dprietob/luma/releases/latest
[gearlever]: https://flathub.org/apps/it.mijorus.gearlever
[cpack]: cmake/Packaging.cmake
[appimage]: packaging/build-appimage.sh
