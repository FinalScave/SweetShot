#ifndef SWEETSHOT_INPUT_H
#define SWEETSHOT_INPUT_H

#include <cstddef>
#include <string>
#include <vector>

#include "sweetshot/theme.h"

namespace sweetshot {
  enum class LongLineMode {
    Clip,
    Wrap
  };

  struct LineRange {
    bool enabled {false};
    std::size_t start_line {0};
    std::size_t line_count {0};
  };

  struct RenderOptions {
    bool show_line_numbers {true};
    bool show_indent_guides {true};
    std::size_t tab_size {4};
    double font_size {14.0};
    double line_height {22.0};
    double padding_x {24.0};
    double padding_y {0};
    double gutter_width {48.0};
    double max_width {1200.0};
    std::string font_family {
      "ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, \"Liberation Mono\", monospace"
    };
    LongLineMode long_line_mode {LongLineMode::Clip};
    std::size_t max_columns {120};
    std::vector<std::size_t> focus_lines;
    std::vector<std::size_t> mark_lines;
    LineRange line_range;
  };

  struct RenderInput {
    std::string source_text;
    std::string file_name;
    std::string language_hint;
    std::string syntax_directory;
    RenderOptions options;
    Theme theme {builtinTheme("sweetline-dark")};
  };
}

#endif
