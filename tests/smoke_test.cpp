#include <catch2/catch_amalgamated.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "lodepng.h"

#include "sweetshot/sweetshot.h"

namespace {
  void RequireContains(const std::string& value, const std::string& expected) {
    INFO("Expected output to contain: " << expected);
    REQUIRE(value.find(expected) != std::string::npos);
  }

  void RequireNotContains(const std::string& value, const std::string& unexpected) {
    INFO("Expected output to omit: " << unexpected);
    REQUIRE(value.find(unexpected) == std::string::npos);
  }

  std::size_t CountOccurrences(const std::string& value, const std::string& needle) {
    std::size_t count = 0;
    std::size_t offset = 0;
    while ((offset = value.find(needle, offset)) != std::string::npos) {
      ++count;
      offset += needle.size();
    }
    return count;
  }

  class RecordingRasterizer final : public sweetshot::SvgRasterizer {
  public:
    sweetshot::PngResult Rasterize(std::string_view svg, const sweetshot::PngOptions& options) override {
      ++calls;
      captured_svg = std::string(svg);
      captured_options = options;
      return {{0x89, 0x50, 0x4e, 0x47}, 24, 12};
    }

    int calls {0};
    std::string captured_svg;
    sweetshot::PngOptions captured_options;
  };

  std::vector<std::uint8_t> DecodeRgba(const std::vector<std::uint8_t>& png, unsigned& width, unsigned& height) {
    std::vector<unsigned char> decoded;
    const unsigned error = lodepng::decode(decoded, width, height, png, LCT_RGBA, 8);
    REQUIRE(error == 0);
    return {decoded.begin(), decoded.end()};
  }

  std::size_t CountNonBackgroundPixels(const std::vector<std::uint8_t>& rgba,
                                       std::uint8_t red,
                                       std::uint8_t green,
                                       std::uint8_t blue) {
    std::size_t count = 0;
    for (std::size_t index = 0; index + 3 < rgba.size(); index += 4) {
      if (rgba[index] != red || rgba[index + 1] != green || rgba[index + 2] != blue) {
        ++count;
      }
    }
    return count;
  }
}

TEST_CASE("Renderer produces highlighted scenes and outputs") {
  sweetshot::RenderInput input;
  input.source_text = "int main() {\n  return 42;\n}\n";
  input.file_name = "main.cpp";
  input.language_hint = "cpp";
  input.syntax_directory = SWEETSHOT_TEST_SYNTAX_DIR;
  input.options.focus_lines = {1};

  sweetshot::Renderer renderer;
  const sweetshot::RenderScene scene = renderer.RenderToScene(input);
  REQUIRE(scene.lines.size() == 4);
  REQUIRE(scene.language == "cpp");
  REQUIRE(scene.theme.name == "default");
  const auto return_run = std::find_if(scene.lines[1].runs.begin(), scene.lines[1].runs.end(), [](const auto& run) {
    return run.text == "return";
  });
  REQUIRE(return_run != scene.lines[1].runs.end());
  REQUIRE(return_run->style_id == sweetshot::token_style_id::kKeyword);
  REQUIRE(return_run->style.foreground == "#7aa2f7");
  REQUIRE(return_run->style.bold);

  const sweetshot::RenderScene cached_scene = renderer.RenderToScene(input);
  REQUIRE(cached_scene.language == scene.language);

  const std::string svg = renderer.RenderToSvg(input);
  RequireContains(svg, "<svg");
  RequireContains(svg, "return");

  const std::string html = renderer.RenderToHtml(input);
  RequireContains(html, "<!doctype html>");
  RequireContains(html, "sweetshot-line focus");
  RequireContains(html, "<div class=\"sweetshot\">");
  RequireNotContains(html, "<pre class=\"sweetshot\">");
}

TEST_CASE("Renderer emits SVG code lines as continuous tspans") {
  sweetshot::RenderInput input;
  input.source_text = "return value;";
  input.file_name = "main.cpp";
  input.language_hint = "cpp";
  input.syntax_directory = SWEETSHOT_TEST_SYNTAX_DIR;
  input.options.show_line_numbers = false;

  sweetshot::Renderer renderer;
  const std::string svg = renderer.RenderToSvg(input);
  REQUIRE(CountOccurrences(svg, "<text ") == 1);
  RequireContains(svg, "<tspan");
  RequireContains(svg, ">return</tspan>");
}

TEST_CASE("Renderer renders PNG through SVG rasterizer backend") {
  sweetshot::RenderInput input;
  input.source_text = "return 42;";
  input.file_name = "main.cpp";
  input.language_hint = "cpp";
  input.syntax_directory = SWEETSHOT_TEST_SYNTAX_DIR;

  sweetshot::PngOptions options;
  options.scale = 2.0;
  options.background = "#000000";

  auto rasterizer = std::make_shared<RecordingRasterizer>();
  sweetshot::RendererConfig config;
  config.png_rasterizer = rasterizer;
  const sweetshot::Renderer renderer(config);
  const sweetshot::PngResult png = renderer.RenderToPng(input, options);

  REQUIRE(rasterizer->calls == 1);
  RequireContains(rasterizer->captured_svg, "<svg");
  RequireContains(rasterizer->captured_svg, "return");
  REQUIRE(rasterizer->captured_options.scale == 2.0);
  REQUIRE(rasterizer->captured_options.background == "#000000");
  REQUIRE(png.bytes == std::vector<std::uint8_t> {0x89, 0x50, 0x4e, 0x47});
  REQUIRE(png.width == 24);
  REQUIRE(png.height == 12);
}

TEST_CASE("Renderer reports missing PNG rasterizer") {
  sweetshot::RenderInput input;
  input.source_text = "return 42;";

  const sweetshot::Renderer renderer;
  REQUIRE_THROWS_WITH(renderer.RenderToPng(input), "PNG rasterizer is not configured");
}

TEST_CASE("Default PNG rasterizer renders PNG bytes") {
  const std::shared_ptr<sweetshot::SvgRasterizer> rasterizer = sweetshot::CreateDefaultSvgRasterizer();
  REQUIRE(rasterizer);
  const sweetshot::PngResult png = rasterizer->Rasterize(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"3\" height=\"2\">"
    "<rect width=\"3\" height=\"2\" fill=\"#ff0000\"/></svg>",
    {});

  REQUIRE(png.width == 9);
  REQUIRE(png.height == 6);
  REQUIRE(png.bytes.size() > 8);
  REQUIRE(png.bytes[0] == 137);
  REQUIRE(png.bytes[1] == 80);
  REQUIRE(png.bytes[2] == 78);
  REQUIRE(png.bytes[3] == 71);
}

TEST_CASE("Default PNG rasterizer applies PNG scale") {
  const std::shared_ptr<sweetshot::SvgRasterizer> rasterizer = sweetshot::CreateDefaultSvgRasterizer();
  REQUIRE(rasterizer);
  sweetshot::PngOptions options;
  options.scale = 2.0;

  const sweetshot::PngResult png = rasterizer->Rasterize(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"3\" height=\"2\">"
    "<rect width=\"3\" height=\"2\" fill=\"#ff0000\"/></svg>",
    options);

  REQUIRE(png.width == 6);
  REQUIRE(png.height == 4);
}

#if defined(SWEETSHOT_PNG_BACKEND_RESVG)
TEST_CASE("Default PNG rasterizer renders text with the embedded fallback font") {
  const std::shared_ptr<sweetshot::SvgRasterizer> rasterizer = sweetshot::CreateDefaultSvgRasterizer();
  REQUIRE(rasterizer);

  sweetshot::PngOptions options;
  options.scale = 1.0;
  options.background = "#000000";

  const sweetshot::PngResult png = rasterizer->Rasterize(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"240\" height=\"64\">"
    "<rect width=\"240\" height=\"64\" fill=\"#000000\"/>"
    "<text x=\"16\" y=\"42\" font-family=\"monospace\" font-size=\"32\" fill=\"#ffffff\">ABC123</text>"
    "</svg>",
    options);

  unsigned width = 0;
  unsigned height = 0;
  const std::vector<std::uint8_t> rgba = DecodeRgba(png.bytes, width, height);
  REQUIRE(width == png.width);
  REQUIRE(height == png.height);
  REQUIRE(CountNonBackgroundPixels(rgba, 0, 0, 0) > 100);
}
#endif

TEST_CASE("Default PNG rasterizer writes compressed PNG bytes") {
  const std::shared_ptr<sweetshot::SvgRasterizer> rasterizer = sweetshot::CreateDefaultSvgRasterizer();
  REQUIRE(rasterizer);
  const sweetshot::PngResult png = rasterizer->Rasterize(
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"128\" height=\"128\">"
    "<rect width=\"128\" height=\"128\" fill=\"#ff0000\"/></svg>",
    {});

  const std::size_t raw_scanline_size = png.height * (png.width * 4 + 1);
  REQUIRE(png.bytes.size() < raw_scanline_size / 4);
}

TEST_CASE("Renderer emits indent guides") {
  const std::filesystem::path syntax_directory =
    std::filesystem::temp_directory_path() / "sweetshot-indent-guide-syntax-test";
  std::filesystem::remove_all(syntax_directory);
  std::filesystem::create_directories(syntax_directory);

  {
    std::ofstream syntax_file(syntax_directory / "plain.json");
    syntax_file << R"json({
  "name": "plain",
  "fileSuffixes": [".plain"],
  "states": {
    "default": [
      {
        "pattern": "\\w+",
        "style": "variable"
      }
    ]
  }
})json";
  }

  sweetshot::RenderInput input;
  input.source_text = "root\n    child\n        leaf\n    sibling";
  input.file_name = "sample.plain";
  input.syntax_directory = syntax_directory.string();

  sweetshot::Renderer renderer;
  sweetshot::RenderScene scene = renderer.RenderToScene(input);
  REQUIRE(scene.lines.size() == 4);
  REQUIRE(scene.lines[1].indent_guides.size() == 1);
  CHECK(scene.lines[1].indent_guides[0].column == 4);

  const auto has_inner_guide = std::any_of(scene.lines[2].indent_guides.begin(),
                                          scene.lines[2].indent_guides.end(), [](const auto& guide) {
    return guide.column == 8;
  });
  REQUIRE(has_inner_guide);

  const std::string svg = renderer.RenderToSvg(input);
  RequireContains(svg, "class=\"sweetshot-indent-guide\"");

  const std::string html = renderer.RenderToHtml(input);
  RequireContains(html, "sweetshot-indent-guide");

  input.options.show_indent_guides = false;
  scene = renderer.RenderToScene(input);
  REQUIRE(scene.lines[1].indent_guides.empty());
  RequireNotContains(renderer.RenderToSvg(input), "sweetshot-indent-guide");
  RequireNotContains(renderer.RenderToHtml(input), "sweetshot-indent-guide");

  std::filesystem::remove_all(syntax_directory);
}

TEST_CASE("Renderer keeps scoped indent guides inside scope markers") {
  const std::filesystem::path syntax_directory =
    std::filesystem::temp_directory_path() / "sweetshot-scoped-indent-guide-syntax-test";
  std::filesystem::remove_all(syntax_directory);
  std::filesystem::create_directories(syntax_directory);

  {
    std::ofstream syntax_file(syntax_directory / "brace.json");
    syntax_file << R"json({
  "name": "brace",
  "fileSuffixes": [".brace"],
  "states": {
    "default": [
      {
        "pattern": "[{}]",
        "style": "punctuation"
      }
    ]
  },
  "scopeRules": {
    "skips": [],
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

  sweetshot::RenderInput input;
  input.source_text = "{\n    child\n}";
  input.file_name = "sample.brace";
  input.syntax_directory = syntax_directory.string();
  input.options.padding_y = 20.0;

  sweetshot::Renderer renderer;
  const sweetshot::RenderScene scene = renderer.RenderToScene(input);
  REQUIRE(scene.lines.size() == 3);
  CHECK(scene.lines[0].indent_guides.empty());
  REQUIRE(scene.lines[1].indent_guides.size() == 1);
  CHECK(scene.lines[1].indent_guides[0].column == 0);
  CHECK(scene.lines[2].indent_guides.empty());

  const std::string svg = renderer.RenderToSvg(input);
  RequireContains(svg, "class=\"sweetshot-indent-guide\"");
  RequireContains(svg, "y1=\"42\"");
  RequireContains(svg, "y2=\"64\"");
  RequireNotContains(svg, "y1=\"20\"");
  RequireNotContains(svg, "y2=\"86\"");

  std::filesystem::remove_all(syntax_directory);
}

TEST_CASE("Builtin themes include editor default and VS Code dark") {
  const sweetshot::Theme default_theme = sweetshot::BuiltinTheme("");
  REQUIRE(default_theme.name == "default");
  REQUIRE(default_theme.background == "#1b1e24");
  REQUIRE(default_theme.foreground == "#d7dee9");
  REQUIRE(default_theme.line_number_foreground == "#5e6778");
  REQUIRE(default_theme.indent_guide_foreground == "#303746");
  REQUIRE(default_theme.focus_background == "#1e222a");
  REQUIRE(default_theme.mark_background == "#262e3e");
  REQUIRE(default_theme.StyleForToken(sweetshot::token_style_id::kKeyword).foreground == "#7aa2f7");
  REQUIRE(default_theme.StyleForToken(sweetshot::token_style_id::kKeyword).bold);
  REQUIRE(default_theme.StyleForToken(sweetshot::token_style_id::kComment).foreground == "#7a8294");
  REQUIRE(default_theme.StyleForToken(sweetshot::token_style_id::kComment).italic);
  REQUIRE(default_theme.StyleForToken(sweetshot::token_style_id::kBuiltin).foreground == "#7dcfff");

  const sweetshot::Theme vscode_dark_theme = sweetshot::BuiltinTheme("vscode-dark");
  REQUIRE(vscode_dark_theme.name == "vscode-dark");
  REQUIRE(vscode_dark_theme.background == "#1e1e1e");
  REQUIRE(vscode_dark_theme.foreground == "#d4d4d4");
  REQUIRE(vscode_dark_theme.StyleForToken(sweetshot::token_style_id::kKeyword).foreground == "#569cd6");

  REQUIRE(sweetshot::BuiltinTheme("default").name == "default");
  REQUIRE(sweetshot::BuiltinTheme("vscode").name == "vscode-dark");
  REQUIRE(sweetshot::BuiltinTheme("solarized dark").name == "solarized-dark");
  REQUIRE(sweetshot::BuiltinTheme("unknown").name == "default");
  REQUIRE(sweetshot::BuiltinTheme("nord").StyleForToken(sweetshot::token_style_id::kBuiltin).foreground == "#5e81ac");
}

TEST_CASE("Renderer uses SweetLine file name routing") {
  sweetshot::RenderInput input;
  input.source_text = "cmake_minimum_required(VERSION 3.16)\n";
  input.file_name = "CMakeLists.txt";
  input.syntax_directory = SWEETSHOT_TEST_SYNTAX_DIR;

  sweetshot::Renderer renderer;
  sweetshot::RenderScene scene = renderer.RenderToScene(input);
  REQUIRE(scene.language == "cmake");

  input.source_text = "FROM ubuntu:24.04\n";
  input.file_name = "Dockerfile";
  scene = renderer.RenderToScene(input);
  REQUIRE(scene.language == "dockerfile");
}

TEST_CASE("Renderer uses SweetLine routing for language hints") {
  sweetshot::RenderInput input;
  input.source_text = "const value = 1;\n";
  input.language_hint = "js";
  input.syntax_directory = SWEETSHOT_TEST_SYNTAX_DIR;

  sweetshot::Renderer renderer;
  sweetshot::RenderScene scene = renderer.RenderToScene(input);
  REQUIRE(scene.language == "javascript");

  input.source_text = "int main() { return 0; }\n";
  input.language_hint = "cpp";
  scene = renderer.RenderToScene(input);
  REQUIRE(scene.language == "cpp");
}

TEST_CASE("Renderer skips inline style syntax files") {
  const std::filesystem::path syntax_directory =
    std::filesystem::temp_directory_path() / "sweetshot-inline-style-syntax-test";
  std::filesystem::remove_all(syntax_directory);
  std::filesystem::create_directories(syntax_directory);

  {
    std::ofstream syntax_file(syntax_directory / "demo-inlineStyle.json");
    syntax_file << R"json({
  "name": "demo",
  "fileSuffixes": [".demo"],
  "states": {
    "default": [
      {
        "pattern": "\\b(token)\\b",
        "style": "keyword"
      }
    ]
  }
})json";
  }

  sweetshot::RenderInput input;
  input.source_text = "token\n";
  input.file_name = "sample.demo";
  input.syntax_directory = syntax_directory.string();

  sweetshot::Renderer renderer;
  const sweetshot::RenderScene scene = renderer.RenderToScene(input);
  REQUIRE(scene.language.empty());

  std::filesystem::remove_all(syntax_directory);
}

TEST_CASE("Renderer clips long lines") {
  sweetshot::RenderInput input;
  input.source_text = "0123456789abcdef";
  input.syntax_directory = "missing-syntax-dir";
  input.options.show_line_numbers = false;
  input.options.max_columns = 4;

  sweetshot::Renderer renderer;
  const sweetshot::RenderScene scene = renderer.RenderToScene(input);
  REQUIRE(scene.lines.size() == 1);
  REQUIRE(scene.lines[0].text == "0123");

  const std::string svg = renderer.RenderToSvg(input);
  RequireNotContains(svg, "4567");
}

TEST_CASE("Renderer wraps long lines into visual lines") {
  sweetshot::RenderInput input;
  input.source_text = "0123456789abcdef";
  input.syntax_directory = "missing-syntax-dir";
  input.options.show_line_numbers = false;
  input.options.max_columns = 4;
  input.options.long_line_mode = sweetshot::LongLineMode::kWrap;

  sweetshot::Renderer renderer;
  const sweetshot::RenderScene scene = renderer.RenderToScene(input);
  REQUIRE(scene.lines.size() == 4);
  REQUIRE(scene.lines[0].text == "0123");
  REQUIRE(scene.lines[1].text == "4567");
  REQUIRE(scene.lines[0].line_number_visible);
  REQUIRE_FALSE(scene.lines[1].line_number_visible);
}
