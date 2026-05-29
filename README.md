English | [简体中文](./README_zh.md)

# SweetShot

SweetShot is a core-first, syntax-aware code rendering toolkit powered by SweetLine. It turns source code into reusable render scenes and high-quality SVG, PNG, or HTML output.

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

SweetLine is fetched with CMake FetchContent from `https://github.com/FinalScave/SweetLine.git` and pinned to `f8aa883b50f2e748098420917fbef5865b55a861`.

## CLI

```bash
build/bin/sweetshot main.cpp -o main.svg
build/bin/sweetshot main.cpp -o main.png
build/bin/sweetshot main.cpp -o main.png --scale 1
build/bin/sweetshot main.cpp --theme default --lines 20:60 --focus 32:38 -o part.svg
cat main.cpp | build/bin/sweetshot --lang cpp -o stdin.html
```

Supported output formats are SVG, PNG, and HTML. PNG output uses the native resvg backend and defaults to 2x scale for sharper text.
