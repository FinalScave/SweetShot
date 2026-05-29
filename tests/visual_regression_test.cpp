#include <catch2/catch_amalgamated.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "sweetshot/sweetshot.h"

namespace {
  struct TempSyntaxDirectory {
    explicit TempSyntaxDirectory(const std::string& name)
      : path(std::filesystem::temp_directory_path() / name) {
      std::filesystem::remove_all(path);
      std::filesystem::create_directories(path);
    }

    ~TempSyntaxDirectory() {
      std::error_code ignored;
      std::filesystem::remove_all(path, ignored);
    }

    std::filesystem::path path;
  };

  void requireContains(const std::string& value, const std::string& expected) {
    INFO("Expected output to contain: " << expected);
    REQUIRE(value.find(expected) != std::string::npos);
  }

  void requireNotContains(const std::string& value, const std::string& unexpected) {
    INFO("Expected output to omit: " << unexpected);
    REQUIRE(value.find(unexpected) == std::string::npos);
  }

  std::size_t countOccurrences(const std::string& value, const std::string& needle) {
    std::size_t count = 0;
    std::size_t offset = 0;
    while ((offset = value.find(needle, offset)) != std::string::npos) {
      ++count;
      offset += needle.size();
    }
    return count;
  }

  void writeVisualSyntax(const std::filesystem::path& syntax_directory) {
    std::ofstream syntax_file(syntax_directory / "visual.json");
    syntax_file << R"json({
  "name": "visual",
  "fileSuffixes": [".visual"],
  "states": {
    "default": [
      {
        "pattern": "\\b(func|if|return)\\b",
        "style": "keyword"
      },
      {
        "pattern": "[{}();]",
        "style": "punctuation"
      }
    ]
  },
  "scopeRules": {
    "skips": [
      {
        "kind": "string",
        "start": "\"",
        "end": "\"",
        "escape": "\\"
      }
    ],
    "rules": [
      {
        "kind": "delimiter",
        "start": "{",
        "end": "}"
      }
    ]
  }
})json";
  }

  std::string boolText(bool value) {
    return value ? "yes" : "no";
  }

  std::string guideSnapshot(const std::vector<sweetshot::SceneIndentGuide>& guides) {
    if (guides.empty()) {
      return "none";
    }

    std::ostringstream output;
    for (std::size_t index = 0; index < guides.size(); ++index) {
      if (index > 0) {
        output << ",";
      }
      output << guides[index].column << "@" << guides[index].x;
    }
    return output.str();
  }

  std::string sceneLayoutSnapshot(const sweetshot::RenderScene& scene) {
    std::ostringstream output;
    output << "width=" << scene.width << " height=" << scene.height
           << " text_origin_x=" << scene.text_origin_x << " char_width=" << scene.char_width << "\n";
    for (std::size_t index = 0; index < scene.lines.size(); ++index) {
      const sweetshot::SceneLine& line = scene.lines[index];
      output << "line " << index << " source=" << line.source_line << " y=" << line.y
             << " line_number=" << boolText(line.line_number_visible)
             << " focus=" << boolText(line.focused)
             << " mark=" << boolText(line.marked)
             << " text=\"" << line.text << "\""
             << " guides=" << guideSnapshot(line.indent_guides) << "\n";
    }
    return output.str();
  }
}

TEST_CASE("Visual fixture keeps scoped code layout stable") {
  TempSyntaxDirectory syntax_directory("sweetshot-visual-regression-syntax-test");
  writeVisualSyntax(syntax_directory.path);

  sweetshot::RenderInput input;
  input.source_text = "func Example() {\n    if (ready) {\n        return \"ok\";\n    }\n}";
  input.file_name = "example.visual";
  input.syntax_directory = syntax_directory.path.string();
  input.options.focus_lines = {2};
  input.options.mark_lines = {1};

  sweetshot::Renderer renderer;
  const sweetshot::RenderScene scene = renderer.renderScene(input);
  const std::string snapshot = sceneLayoutSnapshot(scene);
  INFO(snapshot);
  REQUIRE(snapshot == R"snapshot(width=269.6 height=150 text_origin_x=72 char_width=8.68
line 0 source=0 y=20 line_number=yes focus=no mark=no text="func Example() {" guides=none
line 1 source=1 y=42 line_number=yes focus=no mark=yes text="    if (ready) {" guides=0@72
line 2 source=2 y=64 line_number=yes focus=yes mark=no text="        return "ok";" guides=0@72,4@106.72
line 3 source=3 y=86 line_number=yes focus=no mark=no text="    }" guides=0@72
line 4 source=4 y=108 line_number=yes focus=no mark=no text="}" guides=none
)snapshot");

  const std::string svg = renderer.renderToSvg(input);
  REQUIRE(countOccurrences(svg, "class=\"sweetshot-indent-guide\"") == 4);
  requireContains(svg, "<rect x=\"0\" y=\"42\" width=\"269.6\" height=\"22\" fill=");
  requireContains(svg, "<rect x=\"0\" y=\"64\" width=\"269.6\" height=\"22\" fill=");
  requireContains(svg, "<line class=\"sweetshot-indent-guide\" x1=\"106.72\" y1=\"64\" x2=\"106.72\" y2=\"86\"");
  requireNotContains(svg, "<tspan x=");

  const std::string html = renderer.renderToHtml(input);
  REQUIRE(countOccurrences(html, "class=\"sweetshot-indent-guide\"") == 4);
  requireContains(html, "sweetshot-line mark");
  requireContains(html, "sweetshot-line focus");
  requireContains(html, "left:58.72px");
}

TEST_CASE("Visual fixture keeps wrapped code layout stable") {
  sweetshot::RenderInput input;
  input.source_text = "0123456789abcdef";
  input.syntax_directory = "missing-syntax-dir";
  input.options.show_line_numbers = false;
  input.options.max_columns = 4;
  input.options.long_line_mode = sweetshot::LongLineMode::Wrap;

  sweetshot::Renderer renderer;
  const sweetshot::RenderScene scene = renderer.renderScene(input);
  const std::string snapshot = sceneLayoutSnapshot(scene);
  INFO(snapshot);
  REQUIRE(snapshot == R"snapshot(width=82.72 height=128 text_origin_x=24 char_width=8.68
line 0 source=0 y=20 line_number=yes focus=no mark=no text="0123" guides=none
line 1 source=0 y=42 line_number=no focus=no mark=no text="4567" guides=none
line 2 source=0 y=64 line_number=no focus=no mark=no text="89ab" guides=none
line 3 source=0 y=86 line_number=no focus=no mark=no text="cdef" guides=none
)snapshot");

  const std::string svg = renderer.renderToSvg(input);
  REQUIRE(countOccurrences(svg, "<text ") == 4);
  requireContains(svg, "width=\"82.72\"");
  requireContains(svg, "height=\"128\"");
  requireNotContains(svg, "456789");
}
