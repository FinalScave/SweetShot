#ifndef SWEETSHOT_THEME_H
#define SWEETSHOT_THEME_H

#include <string>
#include <unordered_map>

namespace sweetshot {
  struct TextStyle {
    std::string foreground;
    std::string background;
    bool bold {false};
    bool italic {false};
    bool underline {false};
  };

  struct Theme {
    std::string name;
    std::string background;
    std::string foreground;
    std::string line_number_foreground;
    std::string line_number_background;
    std::string gutter_border;
    std::string focus_background;
    std::string mark_background;
    std::unordered_map<std::string, TextStyle> token_styles;

    TextStyle styleForToken(const std::string& style_name) const;
  };

  Theme githubLightTheme();
  Theme githubDarkTheme();
  Theme oneDarkTheme();
  Theme builtinTheme(const std::string& name);
}

#endif
