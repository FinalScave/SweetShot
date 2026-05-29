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

  return 0;
}
