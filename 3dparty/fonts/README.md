# SweetShot Font Assets

This directory contains the fallback font embedded into SweetShot for server and container rendering.

- `NotoMono-Regular.ttf` is from the Noto project
- `NotoSansMonoCJKsc-Regular.otf` is from the Noto CJK project
- `Noto-LICENSE-OFL.txt` contains the SIL Open Font License text for the bundled Noto font

The embedded fonts let the resvg PNG backend render SVG text even when the host has no usable monospace or CJK fonts.
