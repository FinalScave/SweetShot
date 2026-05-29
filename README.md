English | [简体中文](./README_zh.md)

# SweetShot

SweetShot is a code screenshot generator.

Syntax highlighting and themes come from SweetLine. Output can be SVG, PNG, or HTML. Use the CLI for README files, docs, blog posts, release notes, or generated assets. Use the C++ core when the renderer needs to live inside another app.

## What It Supports

- SVG, PNG, and HTML output
- Built-in themes
- Line numbers and line ranges
- Focus and mark ranges for calling out lines
- Indent guides for nested code blocks
- PNG rendering through `resvg` or `lunasvg`

## Build

You need CMake and a C++17 compiler. The default PNG backend is `resvg`, so the default build also needs Rust/Cargo.

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The build fetches SweetLine with CMake FetchContent from `https://github.com/FinalScave/SweetLine.git`. It is pinned to `f8aa883b50f2e748098420917fbef5865b55a861`.

Set `SWEETSHOT_PNG_BACKEND` if you want to choose the PNG renderer explicitly:

```bash
cmake -S . -B build -DSWEETSHOT_PNG_BACKEND=resvg
cmake -S . -B build-lunasvg -DSWEETSHOT_PNG_BACKEND=lunasvg
```

## CLI

The output format is picked from the output file extension.

```bash
build/bin/sweetshot main.cpp -o main.svg
build/bin/sweetshot main.cpp -o main.png
build/bin/sweetshot main.cpp -o main.png --scale 1
build/bin/sweetshot main.cpp --theme default --lines 20:60 --focus 32:38 -o part.svg
cat main.cpp | build/bin/sweetshot --lang cpp -o stdin.html
```

PNG output uses the configured backend. It renders at 2x scale by default so text stays sharp in normal use.

Common flags:

- `--scale <factor>` sets the PNG output scale. The default is `2`.
- `--theme <name>` selects a built-in theme.
- `--lines <start:end>` renders a one-based inclusive line range.
- `--focus <range-list>` highlights one-based lines.
- `--mark <range-list>` marks one-based lines.
- `--no-line-numbers` hides line numbers.
- `--no-indent-guides` hides indent guide lines.
- `--syntax-dir <path>` overrides the SweetLine syntax directory.
