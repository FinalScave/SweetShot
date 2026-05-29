# SweetShot 路线与技术方案

## 1. 定位

SweetShot 是一个基于 SweetLine 的语法感知代码渲染工具包。

它不应该只被定义成一个 CLI 截图工具。更合理的结构是：以 `SweetShot Core` 为中心，在它之上提供多个产品宿主。

```text
SweetShot Core
  -> CLI
  -> WASM/Web
  -> Desktop App
  -> Flutter/Mobile
  -> SDK/Embedding
  -> Golden Rendering Tests
```

第一阶段的目标应该非常具体：

```text
source code -> SweetLine token spans -> theme mapping -> layout -> render backend -> output
```

SweetLine 负责语法分析，SweetShot 负责把分析结果转成高质量视觉输出。

## 2. 设计原则

- 先做 `Core`，再做具体产品宿主。
- 渲染逻辑不能散落在 CLI、Web、App、SDK 里重复实现。
- SVG、HTML、图片生成等 `Render Backends` 属于 `SweetShot Core`。
- CLI、Web、App 主要负责输入、预览、交互、导出体验和平台能力接入。
- 第一版应该稳定、可脚本化、视觉质量够好，而不是功能堆得很满。
- 能力要贴近 SweetLine 已经具备的优势：语言路由、token spans、主题映射、行范围切片、diff 高亮、缩进参考线。

## 3. 总体架构

```text
SweetShot Core
  Input Model
  SweetLine Analysis Bridge
  Theme System
  Layout Engine
  Render Scene Model
  Render Backends
  Export Metadata

Products / Hosts
  CLI
  WASM/Web
  Desktop App
  Flutter/Mobile
  SDK Bindings
  Golden Rendering Test Runner
```

核心判断是：`Render Backends` 应该在 `Core` 里。因为 SVG、HTML、PNG 这些输出格式本质上是把同一个渲染模型输出成不同结果，不应该由每个宿主各自实现。

## 4. SweetShot Core

### 4.1 Input Model

`Core` 应该接收归一化后的输入，而不是直接处理所有宿主环境里的输入来源。

建议字段：

```text
source_text
file_name
language_hint
line_range
focus_lines
mark_lines
tab_size
theme
render_options
```

支持的输入类型：

- 普通代码文本
- 由宿主读取后的文件内容
- 由 CLI 提供的 stdin 内容
- Markdown 代码块内容
- Diff 文本

CLI 可以读文件和 stdin，Web 可以读 textarea 和文件上传，App 可以读剪贴板或本地文件。但这些平台细节不应该进入 `Core`。`Core` 只接收文本和渲染选项。

### 4.2 SweetLine Analysis Bridge

这一层负责封装 SweetLine 的使用方式，让产品宿主不直接接触复杂的引擎细节。

职责：

- 编译并缓存语法规则。
- 根据文件名或语言提示选择语法。
- 静态渲染时调用 `TextAnalyzer`。
- 大文件或指定行范围渲染时使用 `DocumentAnalyzer` 和行切片能力。
- 将 SweetLine 的 `TokenSpan` 转成 SweetShot 内部 token 模型。
- 按需获取 SweetLine 的缩进参考线分析结果。

初始分析模式：

- 全量代码分析
- 指定行范围分析
- Diff 语法分析
- Markdown fenced code block 分析

### 4.3 Theme System

`Theme System` 负责把 SweetLine 的 style name 或 style ID 映射成可渲染的视觉样式。

主题字段可以包含：

```text
background
foreground
selection_background
line_number_foreground
line_number_background
indent_guide_color
focus_background
marked_add_background
marked_remove_background
token_styles
```

Token 样式字段：

```text
foreground
background
font_style
font_weight
underline
```

主题来源：

- SweetShot 内置主题
- 外部 JSON 主题
- SweetLine inline styles
- 未来导入 VS Code / TextMate 主题

第一版可以内置少量主题：

- GitHub Light
- GitHub Dark
- One Dark
- Dracula
- Solarized Light
- Solarized Dark

### 4.4 Layout Engine

`Layout Engine` 负责把 token 转成带坐标的视觉对象。

职责：

- 字体选择和 fallback 策略
- 行高计算
- 字符宽度测量
- Tab 展开
- 行号 gutter 布局
- Padding 和内容区域计算
- 可选标题栏布局
- Focus line 背景布局
- Added/removed line 背景布局
- 缩进参考线位置计算
- 长行处理

长行模式：

```text
clip
wrap
scale-down
horizontal-scroll-model
```

第一版建议只支持 `clip` 和 `wrap`。`scale-down` 和 `horizontal-scroll-model` 可以后置。

布局层最容易踩坑的点：

- CJK 宽度
- Emoji 和复杂 Unicode
- Ligatures
- 字体是否存在
- 跨平台文字测量差异
- 高 DPI 输出

建议 `Core` 定义布局抽象，让不同图片后端可以使用平台相关的字体测量能力，但不改变上层产品行为。

### 4.5 Render Scene Model

`Render Scene Model` 是布局和具体输出格式之间的中间表示。

它描述“要画什么”，但不绑定 SVG、HTML、Canvas、Skia 或平台图形 API。

建议包含的概念：

```text
document_size
background_rects
text_runs
line_number_runs
indent_lines
focus_rects
mark_rects
header_items
metadata
```

这样做的好处：

- CLI、Web、App 可以复用同一个输出模型。
- SVG 和 HTML 输出更容易保持确定性。
- PNG 渲染可以有多个后端实现。
- Golden rendering tests 可以先比较 scene JSON，再比较像素结果。

### 4.6 Render Backends

`Render Backends` 属于 `SweetShot Core`。

它们负责把 `Render Scene Model` 转换成具体输出：

```text
renderToSvg(scene) -> SVG string
renderToHtml(scene) -> HTML string
renderToImage(scene) -> image buffer
renderToScene(input) -> render scene
```

初始后端：

- SVG renderer
- HTML renderer
- PNG renderer abstraction

建议优先级：

1. SVG renderer
2. HTML renderer
3. PNG renderer

SVG 适合作为第一后端，因为它可以保留文本、无限缩放、结果相对确定，也比像素输出更容易测试。

PNG 仍然很重要，因为社交分享、README 图片、聊天工具、部分平台嵌入场景都更依赖 PNG。

### 4.7 Export Metadata

SweetShot 应该支持可选元数据导出。

元数据可以包含：

```text
source_hash
file_name
language
theme
line_range
focus_lines
sweetline_version
sweetshot_version
created_at
```

这些信息可以用于复现、调试、golden tests、缓存 key 和后续自动化流程。

## 5. Products / Hosts

### 5.1 CLI

CLI 是最快验证 `Core` 质量的产品入口。

示例命令：

```bash
sweetshot main.cpp -o main.svg
sweetshot main.cpp -o main.png --theme github-dark
cat main.cpp | sweetshot --lang cpp -o main.svg
sweetshot main.cpp --lines 20:60 --focus 32:38 -o part.png
git diff | sweetshot --lang diff -o diff.svg
```

CLI 职责：

- 解析参数。
- 读取文件或 stdin。
- 根据输出路径选择格式。
- 调用 `SweetShot Core`。
- 写出 SVG、HTML、PNG 或元数据文件。
- 支持 CI 和脚本化使用。

CLI 不应该拥有渲染逻辑。

### 5.2 WASM/Web

Web 版本用于让 SweetShot 在浏览器里可试用、可预览、可分享。

使用场景：

- 在线预览
- 主题预览
- 代码转图片
- 文档站 demo
- 分享型渲染 playground

Web 职责：

- 文本编辑输入
- 文件上传
- 主题选择
- 实时预览
- 下载按钮
- 剪贴板接入

Web 宿主应该通过 WASM 调用 `Core`，复用同一套主题、布局和渲染模型。

### 5.3 Desktop App

Desktop App 可以服务不想用命令行的用户。

使用场景：

- 拖拽代码文件
- 粘贴剪贴板代码
- 导出前预览
- 批量导出
- 主题自定义

App 仍然应该使用 `Core` 完成分析、布局和渲染。

### 5.4 Flutter/Mobile

移动端可以作为后续产品宿主。

使用场景：

- 粘贴代码并生成分享图
- 移动端查看代码片段
- 生成适合社交平台的代码图片

Flutter 是合理路线，因为 SweetLine 已经有 Flutter/Dart FFI 支持。

### 5.5 SDK / Embedding

SweetShot 后续应该可以作为嵌入式库使用。

可能的消费者：

- 文档生成工具
- 代码审查系统
- AI 编程产品
- Git 客户端
- 博客生成器
- 内部开发者门户

SDK 接口应该保持小而稳定：

```text
createRenderer(config)
renderSource(input, options)
renderDiff(input, options)
renderMarkdownBlocks(input, options)
renderToSvg(result)
renderToHtml(result)
renderToImage(result)
```

## 6. 具体功能范围

### Phase 1: Core Rendering MVP

目标：让基础链路跑通。

`Core` 功能：

- Source text input
- File name 或 language hint
- SweetLine token analysis
- Theme mapping
- Basic layout
- Line numbers
- Padding
- SVG output
- HTML output
- 如果图片后端可用，则支持 basic PNG output

CLI 功能：

- 文件输入
- stdin 输入
- 根据扩展名选择输出格式
- 主题选择
- 语言提示

示例：

```bash
sweetshot main.cpp -o main.svg
sweetshot main.cpp --theme github-dark -o main.png
```

### Phase 2: 日常代码图片能力

目标：让 SweetShot 能用于日常文档和分享。

`Core` 功能：

- 行范围渲染
- Focus lines
- Highlighted lines
- 非 focus 行淡化
- 标题栏
- 可选窗口样式
- 可配置字体、字号、行高
- Tab size
- `clip` 和 `wrap` 两种长行模式

示例：

```bash
sweetshot main.cpp --lines 20:60 --focus 32:38 -o part.svg
```

### Phase 3: Diff Rendering

目标：支持除普通代码外最实用的开发者分享场景。

`Core` 功能：

- Diff input mode
- Added/removed line backgrounds
- Diff gutter markers
- 如果可行，后续支持 diff hunk 内部的原语言高亮

示例：

```bash
git diff | sweetshot --lang diff -o diff.png
```

第一版可以先把 diff 当成独立语法渲染。后续再考虑在 changed hunks 内做原语言语法高亮。

### Phase 4: Markdown Batch Rendering

目标：接入文档工作流。

功能：

- 解析 Markdown fenced code blocks
- 每个代码块生成一张图
- 使用 fence 的语言提示
- 生成稳定的输出文件名
- 可选生成 manifest

示例：

```bash
sweetshot md README.md --out docs/images
```

这部分大多属于 CLI/host 逻辑，但每个代码块的渲染仍然应该由 `Core` 完成。

### Phase 5: Golden Rendering Tests

目标：帮助 SweetLine 和 SweetShot 维护视觉稳定性。

功能：

- 把语法样例渲染成 SVG/PNG
- 存储 golden outputs
- 比较 render scenes
- 必要时比较像素输出
- 输出视觉 diff

示例：

```bash
sweetshot test syntaxes/java.json tests/files/example.java
```

这个能力对 SweetLine grammar 维护尤其有价值。

### Phase 6: WASM/Web Demo

目标：让 SweetShot 容易试用和传播。

功能：

- 浏览器代码输入
- 语言选择
- 主题选择
- 实时预览
- SVG/PNG 下载
- 复制图片
- 复制 SVG/HTML

Web demo 既可以作为产品入口，也可以作为 `Core` 可移植性的验证工具。

## 7. 早期明确不做的内容

这些方向后续可能有价值，但不建议进入第一版：

- 完整视觉画布编辑器
- 视频导出
- Agent replay
- AI 自动讲解
- 自动函数级语义裁剪
- 复杂项目代码地图
- Presentation builder
- 完整 IDE 式编辑器

第一版应该集中在高质量渲染、清晰 API 边界和可靠输出。

## 8. 关键技术决策

### 8.1 主要图片后端

可选路线：

- Skia
- Cairo/Pango
- Apple 平台使用 CoreGraphics
- Web 使用 Browser Canvas
- 各平台原生图片 API

建议：

- 先做 SVG 和 HTML，因为它们更容易获得确定性结果。
- 等 layout model 稳定后再做 PNG。
- PNG 放在 backend abstraction 后面。

### 8.2 字体处理

需要决定：

- 是否内置默认 monospace 字体。
- 缺失字体时如何 fallback。
- 是否支持 ligatures。
- CJK 和 emoji 如何处理。

建议：

- 测试和打包版本使用可预测的默认字体。
- 允许用户指定字体。
- 默认关闭 ligatures，保证代码布局稳定。

### 8.3 主题格式

需要决定：

- SweetShot 原生 theme JSON 格式。
- 如何兼容 SweetLine inline styles。
- 后续是否支持导入 VS Code 主题。

建议：

- 先定义一个小而稳定的 SweetShot theme format。
- 后续再加 importer。

### 8.4 输出格式优先级

建议顺序：

1. SVG
2. HTML
3. PNG
4. WebP
5. PDF

SVG 和 HTML 更容易保留文本。PNG 更适合分享和平台嵌入。

### 8.5 Core API 边界

`Core` 不应该做：

- 解析 CLI 参数。
- 打开原生文件选择框。
- 调用浏览器剪贴板 API。
- 管理 App 窗口。
- 持有产品特定状态。

`Core` 应该做：

- 分析代码。
- 构建布局。
- 构建 render scene。
- 输出 SVG、HTML、image buffer。
- 导出 metadata。

## 9. 建议模块布局

可能的仓库结构：

```text
sweetshot/
  core/
    include/
    src/
      input/
      analysis/
      theme/
      layout/
      scene/
      render/
      export/
    themes/
    tests/
  cli/
  wasm/
  app/
  flutter/
  docs/
  examples/
```

`Core` 模块候选：

```text
SweetShotInput
SweetShotOptions
SweetShotTheme
SweetShotAnalyzer
SweetShotLayout
SweetShotScene
SweetShotSvgRenderer
SweetShotHtmlRenderer
SweetShotImageRenderer
SweetShotMetadata
```

## 10. MVP 验收标准

MVP 达标时应该可以做到：

- 把一个源代码文件渲染成 SVG。
- 把 stdin 渲染成 SVG。
- 根据文件名或 language hint 选择语法。
- 应用至少两个内置主题。
- 可选显示行号。
- 渲染指定行范围。
- 高亮 focus lines。
- 输出对测试友好的确定性结果。
- 通过同一套 `Core` 支撑 CLI。

MVP stretch goals：

- PNG output
- HTML output
- Diff rendering
- Metadata export

## 11. 推荐第一阶段实现路径

1. 定义 `SweetShot Core` 的 input/options/theme/scene 类型。
2. 实现 SweetLine analysis bridge。
3. 实现最小可用 layout engine。
4. 实现 SVG renderer。
5. 实现基于 `Core` 的 CLI wrapper。
6. 增加内置主题。
7. 增加行号、行范围和 focus lines。
8. 增加 HTML renderer。
9. 增加 PNG backend abstraction。
10. 增加 diff mode 和 Markdown batch mode。

这条路径比较稳。它能保证后续 CLI、Web、App、SDK 都复用同一套 `Core`，而不是各自重新实现渲染。

## 12. 一句话总结

SweetShot 应该是一个 Core-first、语法感知的代码渲染工具包：它把 SweetLine 的分析结果转成可复用的 SVG、HTML 和图片输出，并服务 CLI、Web、App、SDK、文档和测试工作流。
