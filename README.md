This repository contains C/C++ examples related to Skia, Google's 2D graphics library. The primary purpose of this repository is to extract and isolate examples from the upstream Skia codebase so that they can be read, built, and experimented with independently. It is particularly focused on facilitating porting or rewriting efforts for skia-python.

- Purpose:
  - Porting and rewriting Skia functionality for skia-python.
  - Providing standalone, buildable examples of Skia's usage in C/C++.

On Wayland-based platforms (i.e. very new Linux systems), setting `SDL_VIDEODRIVER=x11`
may be needed for the SDL example.

`decode_everthing` needs `skia_use_libjxl_decode=true` for jpegxl decoding.
`use_skresources` needs the whole of the static library.
