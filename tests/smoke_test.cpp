#include <iostream>
#include <string>

#include "sweetshot/sweetshot.h"

namespace {
  int requireContains(const std::string& value, const std::string& expected) {
    if (value.find(expected) == std::string::npos) {
      std::cerr << "Expected output to contain: " << expected << "\n";
      return 1;
    }
    return 0;
  }

  int requireNotContains(const std::string& value, const std::string& unexpected) {
    if (value.find(unexpected) != std::string::npos) {
      std::cerr << "Expected output to omit: " << unexpected << "\n";
      return 1;
    }
    return 0;
  }
}

int main() {
  sweetshot::RenderInput input;
  input.source_text = "int main() {\n  return 42;\n}\n";
  input.file_name = "main.cpp";
  input.language_hint = "cpp";
  input.syntax_directory = SWEETSHOT_TEST_SYNTAX_DIR;
  input.theme = sweetshot::builtinTheme("github-dark");
  input.options.focus_lines = {1};

  sweetshot::Renderer renderer;
  const sweetshot::RenderScene scene = renderer.renderScene(input);
  if (scene.lines.size() != 4) {
    std::cerr << "Unexpected scene line count: " << scene.lines.size() << "\n";
    return 1;
  }
  if (scene.language != "cpp") {
    std::cerr << "Unexpected language: " << scene.language << "\n";
    return 1;
  }
  const sweetshot::RenderScene cached_scene = renderer.renderScene(input);
  if (cached_scene.language != scene.language) {
    std::cerr << "Unexpected cached render language: " << cached_scene.language << "\n";
    return 1;
  }

  const std::string svg = renderer.renderToSvg(input);
  if (const int result = requireContains(svg, "<svg"); result != 0) {
    return result;
  }
  if (const int result = requireContains(svg, "return"); result != 0) {
    return result;
  }

  const std::string html = renderer.renderToHtml(input);
  if (const int result = requireContains(html, "<!doctype html>"); result != 0) {
    return result;
  }
  if (const int result = requireContains(html, "sweetshot-line focus"); result != 0) {
    return result;
  }
  if (const int result = requireContains(html, "<div class=\"sweetshot\">"); result != 0) {
    return result;
  }
  if (const int result = requireNotContains(html, "<pre class=\"sweetshot\">"); result != 0) {
    return result;
  }

  sweetshot::RenderInput clip_input;
  clip_input.source_text = "0123456789abcdef";
  clip_input.syntax_directory = "missing-syntax-dir";
  clip_input.options.show_line_numbers = false;
  clip_input.options.max_columns = 4;
  const sweetshot::RenderScene clip_scene = renderer.renderScene(clip_input);
  if (clip_scene.lines.size() != 1 || clip_scene.lines[0].text != "0123") {
    std::cerr << "Unexpected clipped line output\n";
    return 1;
  }
  const std::string clip_svg = renderer.renderToSvg(clip_input);
  if (const int result = requireNotContains(clip_svg, "4567"); result != 0) {
    return result;
  }

  sweetshot::RenderInput wrap_input = clip_input;
  wrap_input.options.long_line_mode = sweetshot::LongLineMode::Wrap;
  const sweetshot::RenderScene wrap_scene = renderer.renderScene(wrap_input);
  if (wrap_scene.lines.size() != 4) {
    std::cerr << "Unexpected wrapped line count: " << wrap_scene.lines.size() << "\n";
    return 1;
  }
  if (wrap_scene.lines[0].text != "0123" || wrap_scene.lines[1].text != "4567") {
    std::cerr << "Unexpected wrapped line content\n";
    return 1;
  }
  if (!wrap_scene.lines[0].line_number_visible || wrap_scene.lines[1].line_number_visible) {
    std::cerr << "Unexpected wrapped line number visibility\n";
    return 1;
  }

  return 0;
}
