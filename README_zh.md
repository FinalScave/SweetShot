简体中文 | [English](./README.md)

# SweetShot

SweetShot 是一个 Core-first、语法感知的代码渲染工具包，由 SweetLine 提供语法分析能力。它可以把源代码转换成可复用的渲染场景，并输出高质量 SVG 或 HTML。

## 构建

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

SweetLine 通过 CMake FetchContent 从 `https://github.com/FinalScave/SweetLine.git` 拉取，并固定到 `f8aa883b50f2e748098420917fbef5865b55a861`。

## CLI

```bash
build/bin/sweetshot main.cpp -o main.svg
build/bin/sweetshot main.cpp --theme github-dark --lines 20:60 --focus 32:38 -o part.svg
cat main.cpp | build/bin/sweetshot --lang cpp -o stdin.html
```

当前支持 SVG 和 HTML 输出。PNG 与其他图片后端计划基于同一套 Core render scene model 继续扩展。
