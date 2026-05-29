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
}

TEST_CASE("Renderer produces highlighted scenes and outputs") {
  sweetshot::RenderInput input;
  input.source_text = "int main() {\n  return 42;\n}\n";
  input.file_name = "main.cpp";
  input.language_hint = "cpp";
  input.syntax_directory = SWEETSHOT_TEST_SYNTAX_DIR;
  input.theme = sweetshot::builtinTheme("github-dark");
  input.options.focus_lines = {1};

  sweetshot::Renderer renderer;
  const sweetshot::RenderScene scene = renderer.renderScene(input);
  REQUIRE(scene.lines.size() == 4);
  REQUIRE(scene.language == "cpp");
  const auto return_run = std::find_if(scene.lines[1].runs.begin(), scene.lines[1].runs.end(), [](const auto& run) {
    return run.text == "return";
  });
  REQUIRE(return_run != scene.lines[1].runs.end());
  REQUIRE(return_run->style_id == sweetshot::token_style_id::Keyword);
  REQUIRE(return_run->style.foreground == "#ff7b72");
  REQUIRE(return_run->style.bold);

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
