---
name: sweetshot-code-screenshot
description: Generate code screenshots and embeddable code blocks with SweetShot. Use when an agent needs to turn source files or stdin snippets into PNG, SVG, or HTML for README files, docs, blog posts, release notes, visual examples, or automated code screenshot assets.
---

# SweetShot Code Screenshot

Use SweetShot to render source code as PNG, SVG, or HTML. Prefer the CLI bundled with this skill; do not assume a project-local build directory exists.

## Locate The CLI

Resolve the skill directory as the folder containing this `SKILL.md`, then choose the current platform binary:

```text
bin/windows/sweetshot.exe
bin/linux/x86_64/sweetshot
bin/linux/aarch64/sweetshot
bin/macos/x86_64/sweetshot
bin/macos/aarch64/sweetshot
```

Use `x86_64` for x64/amd64 machines and `aarch64` for arm64/aarch64 machines. Windows currently uses the single `bin/windows/sweetshot.exe` binary.

If the matching binary is missing or not executable, ask for a SweetShot CLI path or use one explicitly provided by the user. Do not fall back to repository `build/` paths unless the user asks for local development behavior.

## Locate Syntaxes

Always pass `--syntax-dir` when invoking SweetShot.

Use the user's syntax directory when one is provided. Otherwise, use the bundled syntax directory next to this skill:

```text
syntaxes
```

The bundled directory contains SweetLine syntax files, excluding inlineStyle syntaxes.

## Basic Usage

The output format is selected by the output extension:

In the examples below, replace `sweetshot` with the resolved skill-bundled binary path and replace `<syntax-dir>` with the selected syntax directory.

```bash
sweetshot path/to/file.cpp --syntax-dir <syntax-dir> -o output.png
sweetshot path/to/file.cpp --syntax-dir <syntax-dir> -o output.svg
sweetshot path/to/file.cpp --syntax-dir <syntax-dir> -o output.html
```

For stdin:

```bash
cat path/to/file.cpp | sweetshot --lang cpp --syntax-dir <syntax-dir> -o output.png
```

Use quotes around paths with spaces.

## Recommended Defaults

- Use PNG for README images and documentation screenshots
- Use SVG when the user wants a scalable vector artifact
- Use HTML when the user wants an embeddable code block
- Keep PNG at the default 3x scale unless the user asks for smaller files or sharper output
- Use `--scale 4` for high-resolution marketing or hero images
- Use `--scale 1` only when exact CSS-pixel dimensions or small files matter
- Keep line ranges short, usually 15-40 lines
- Prefer real project code over artificial demo snippets
- Prefer snippets with nested `if`, `for`, `switch`, classes, or callbacks when showing indent guides
- Avoid snippets dominated by large strings, generated code, or repetitive data tables

## Common Flags

```bash
sweetshot src/file.cpp --syntax-dir <syntax-dir> --lines 40:80 -o docs/images/example.png
sweetshot src/file.cpp --syntax-dir <syntax-dir> --lines 40:80 --focus 52:61 -o docs/images/example.png
sweetshot src/file.cpp --syntax-dir <syntax-dir> --lines 40:80 --mark 66:70 -o docs/images/example.png
sweetshot src/file.cpp --syntax-dir <syntax-dir> --theme vscode-dark -o docs/images/example.png
sweetshot src/file.cpp --syntax-dir <syntax-dir> --scale 4 -o docs/images/example.png
```

- `--lines <start:end>` renders a one-based inclusive line range
- `--focus <range-list>` highlights important lines, such as `12` or `12:18,24`
- `--mark <range-list>` marks supporting lines
- `--theme <name>` selects a built-in theme
- `--scale <factor>` controls PNG raster scale
- `--no-line-numbers` hides line numbers
- `--no-indent-guides` hides indent guide lines
- `--lang <name>` overrides language detection for stdin or unusual filenames
- `--syntax-dir <path>` sets the SweetLine syntax directory

## README Image Workflow

For README or docs images:

1. Pick a real source file from the repository
2. Choose a compact line range that shows useful structure
3. Use `--focus` to highlight the important block
4. Write PNG output under `docs/images/` unless the user gives another path
5. Verify the file exists and inspect the image when visual quality matters
6. Reference it from Markdown with a relative path or an HTML `<img>` tag with a display width

Example:

```bash
sweetshot src/lunasvg_rasterizer.cpp --syntax-dir <syntax-dir> --lines 84:112 --focus 87:111 -o docs/images/readme-indent-guides.png
```

Markdown:

```html
<img src="docs/images/readme-indent-guides.png" width="760" alt="SweetShot indent guides example">
```

## Output Checks

After generating an image, check at least:

- The output file exists and is non-empty
- PNG dimensions are high enough for the target display size
- Text is not clipped
- The selected code range is readable
- Indent guides are visible when the user wants to showcase code structure

For PNG quality complaints, regenerate from SweetShot with a larger `--scale`; do not upscale an already-rendered PNG.
