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

<a href='https://flathub.org/apps/io.github.dprietob.luma'><img width='196' alt='Download on Flathub' src='https://flathub.org/api/badge?locale=en'/></a>

## Your data stays on your device

Luma works with local projects. Each project is a self-contained folder with a
`.luma` file (JSON) and a `tracklist/` directory where imported tracks are
copied. Track paths are stored relative to the project, so a project folder is
fully portable — move it or back it up and it just works. App preferences such
as window size, audio cache size and recent projects are stored natively with
QSettings. There are no accounts and no online services; nothing leaves your
computer.

## Developing and Building

Build the Flatpak from the command line:

```shell
flatpak install flathub org.kde.Platform//6.9 org.kde.Sdk//6.9
flatpak-builder --user --install --force-clean build-dir io.github.dprietob.luma.yml
flatpak run io.github.dprietob.luma
```

Or build natively with CMake and Ninja. You need a C++17 compiler, CMake ≥ 3.21,
Qt 6 and the audio development libraries. On Fedora:

```shell
sudo dnf install gcc-c++ cmake ninja-build qt6-qtbase-devel \
    qt6-qtdeclarative-devel qt6-qtshadertools portaudio-devel \
    libsndfile-devel mpg123-devel soundtouch-devel
```

```shell
rm -rf build && cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build && ./build/luma
```

Run the test suite with `ctest --test-dir build`.

## License

Luma is released under the GPL-3.0-or-later license.

[Flathub]: https://flathub.org/apps/io.github.dprietob.luma
