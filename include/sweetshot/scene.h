#ifndef SWEETSHOT_SCENE_H
#define SWEETSHOT_SCENE_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#include "sweetshot/input.h"
#include "sweetshot/theme.h"

namespace sweetshot {
  struct TextRun {
    std::size_t column {0};
    double x {0.0};
    double y {0.0};
    std::string text;
    std::string style_name;
    int32_t style_id {0};
    TextStyle style;
  };

  struct SceneLine {
    std::size_t source_line {0};
    std::string text;
    double y {0.0};
    bool focused {false};
    bool marked {false};
    bool line_number_visible {true};
    std::vector<TextRun> runs;
  };

  struct RenderScene {
    double width {0.0};
    double height {0.0};
    double char_width {0.0};
    double text_origin_x {0.0};
    std::string language;
    std::string file_name;
    Theme theme;
    RenderOptions options;
    std::vector<SceneLine> lines;
  };
}

#endif
