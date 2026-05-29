#include <catch2/catch_amalgamated.hpp>

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
