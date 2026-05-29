#include <catch2/catch_amalgamated.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

#include "sweetshot/sweetshot.h"

namespace {
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
}

TEST_CASE("Renderer produces highlighted scenes and outputs") {
  sweetshot::RenderInput input;
  input.source_text = "int main() {\n  return 42;\n}\n";
  input.file_name = "main.cpp";
  input.language_hint = "cpp";
  input.syntax_directory = SWEETSHOT_TEST_SYNTAX_DIR;
  input.options.focus_lines = {1};

  sweetshot::Renderer renderer;
  const sweetshot::RenderScene scene = renderer.renderScene(input);
  REQUIRE(scene.lines.size() == 4);
  REQUIRE(scene.language == "cpp");
  REQUIRE(scene.theme.name == "sweetline-dark");
  const auto return_run = std::find_if(scene.lines[1].runs.begin(), scene.lines[1].runs.end(), [](const auto& run) {
    return run.text == "return";
  });
  REQUIRE(return_run != scene.lines[1].runs.end());
  REQUIRE(return_run->style_id == sweetshot::token_style_id::Keyword);
  REQUIRE(return_run->style.foreground == "#569cd6");
  REQUIRE_FALSE(return_run->style.bold);

  const sweetshot::RenderScene cached_scene = renderer.renderScene(input);
  REQUIRE(cached_scene.language == scene.language);

  const std::string svg = renderer.renderToSvg(input);
  requireContains(svg, "<svg");
  requireContains(svg, "return");

  const std::string html = renderer.renderToHtml(input);
  requireContains(html, "<!doctype html>");
  requireContains(html, "sweetshot-line focus");
  requireContains(html, "<div class=\"sweetshot\">");
  requireNotContains(html, "<pre class=\"sweetshot\">");
}

TEST_CASE("Renderer emits SVG code lines as continuous tspans") {
  sweetshot::RenderInput input;
  input.source_text = "return value;";
  input.file_name = "main.cpp";
  input.language_hint = "cpp";
  input.syntax_directory = SWEETSHOT_TEST_SYNTAX_DIR;
  input.options.show_line_numbers = false;

  sweetshot::Renderer renderer;
  const std::string svg = renderer.renderToSvg(input);
  REQUIRE(countOccurrences(svg, "<text ") == 1);
  requireContains(svg, "<tspan");
  requireContains(svg, ">return</tspan>");
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
  sweetshot::RenderScene scene = renderer.renderScene(input);
  REQUIRE(scene.lines.size() == 4);
  REQUIRE(scene.lines[1].indent_guides.size() == 1);
  CHECK(scene.lines[1].indent_guides[0].column == 4);

  const auto has_inner_guide = std::any_of(scene.lines[2].indent_guides.begin(),
                                          scene.lines[2].indent_guides.end(), [](const auto& guide) {
    return guide.column == 8;
  });
  REQUIRE(has_inner_guide);

  const std::string svg = renderer.renderToSvg(input);
  requireContains(svg, "class=\"sweetshot-indent-guide\"");

  const std::string html = renderer.renderToHtml(input);
  requireContains(html, "sweetshot-indent-guide");

  input.options.show_indent_guides = false;
  scene = renderer.renderScene(input);
  REQUIRE(scene.lines[1].indent_guides.empty());
  requireNotContains(renderer.renderToSvg(input), "sweetshot-indent-guide");
  requireNotContains(renderer.renderToHtml(input), "sweetshot-indent-guide");

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

  sweetshot::Renderer renderer;
  const sweetshot::RenderScene scene = renderer.renderScene(input);
  REQUIRE(scene.lines.size() == 3);
  CHECK(scene.lines[0].indent_guides.empty());
  REQUIRE(scene.lines[1].indent_guides.size() == 1);
  CHECK(scene.lines[1].indent_guides[0].column == 0);
  CHECK(scene.lines[2].indent_guides.empty());

  const std::string svg = renderer.renderToSvg(input);
  requireContains(svg, "class=\"sweetshot-indent-guide\"");
  requireContains(svg, "y1=\"42\"");
  requireContains(svg, "y2=\"64\"");
  requireNotContains(svg, "y1=\"20\"");
  requireNotContains(svg, "y2=\"86\"");

  std::filesystem::remove_all(syntax_directory);
}

TEST_CASE("Builtin themes follow SweetLine defaults") {
  const sweetshot::Theme default_theme = sweetshot::builtinTheme("");
  REQUIRE(default_theme.name == "sweetline-dark");
  REQUIRE(default_theme.background == "#1e1e1e");
  REQUIRE(default_theme.foreground == "#d4d4d4");
  REQUIRE(default_theme.styleForToken(sweetshot::token_style_id::Keyword).foreground == "#569cd6");
  REQUIRE(default_theme.styleForToken(sweetshot::token_style_id::Builtin).foreground == "#569cd6");
  REQUIRE(default_theme.styleForToken(sweetshot::token_style_id::Property).foreground == "#9cdcfe");

  REQUIRE(sweetshot::builtinTheme("SweetLine Dark").name == "sweetline-dark");
  REQUIRE(sweetshot::builtinTheme("sweetline-dark").name == "sweetline-dark");
  REQUIRE(sweetshot::builtinTheme("solarized dark").name == "solarized-dark");
  REQUIRE(sweetshot::builtinTheme("unknown").name == "sweetline-dark");
  REQUIRE(sweetshot::builtinTheme("nord").styleForToken(sweetshot::token_style_id::Builtin).foreground == "#5e81ac");
}

TEST_CASE("Renderer uses SweetLine file name routing") {
  sweetshot::RenderInput input;
  input.source_text = "cmake_minimum_required(VERSION 3.16)\n";
  input.file_name = "CMakeLists.txt";
  input.syntax_directory = SWEETSHOT_TEST_SYNTAX_DIR;

  sweetshot::Renderer renderer;
  sweetshot::RenderScene scene = renderer.renderScene(input);
  REQUIRE(scene.language == "cmake");

  input.source_text = "FROM ubuntu:24.04\n";
  input.file_name = "Dockerfile";
  scene = renderer.renderScene(input);
  REQUIRE(scene.language == "dockerfile");
}

TEST_CASE("Renderer uses SweetLine routing for language hints") {
  sweetshot::RenderInput input;
  input.source_text = "const value = 1;\n";
  input.language_hint = "js";
  input.syntax_directory = SWEETSHOT_TEST_SYNTAX_DIR;

  sweetshot::Renderer renderer;
  sweetshot::RenderScene scene = renderer.renderScene(input);
  REQUIRE(scene.language == "javascript");

  input.source_text = "int main() { return 0; }\n";
  input.language_hint = "cpp";
  scene = renderer.renderScene(input);
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
  const sweetshot::RenderScene scene = renderer.renderScene(input);
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
  const sweetshot::RenderScene scene = renderer.renderScene(input);
  REQUIRE(scene.lines.size() == 1);
  REQUIRE(scene.lines[0].text == "0123");

  const std::string svg = renderer.renderToSvg(input);
  requireNotContains(svg, "4567");
}

TEST_CASE("Renderer wraps long lines into visual lines") {
  sweetshot::RenderInput input;
  input.source_text = "0123456789abcdef";
  input.syntax_directory = "missing-syntax-dir";
  input.options.show_line_numbers = false;
  input.options.max_columns = 4;
  input.options.long_line_mode = sweetshot::LongLineMode::Wrap;

  sweetshot::Renderer renderer;
  const sweetshot::RenderScene scene = renderer.renderScene(input);
  REQUIRE(scene.lines.size() == 4);
  REQUIRE(scene.lines[0].text == "0123");
  REQUIRE(scene.lines[1].text == "4567");
  REQUIRE(scene.lines[0].line_number_visible);
  REQUIRE_FALSE(scene.lines[1].line_number_visible);
}
