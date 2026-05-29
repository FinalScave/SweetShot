#ifndef SWEETSHOT_THEME_H
#define SWEETSHOT_THEME_H

#include <cstdint>
#include <string>
#include <unordered_map>

namespace sweetshot {
  namespace token_style_id {
    inline constexpr int32_t Default = 0;
    inline constexpr int32_t Keyword = 1;
    inline constexpr int32_t String = 2;
    inline constexpr int32_t Number = 3;
    inline constexpr int32_t Comment = 4;
    inline constexpr int32_t Class = 5;
    inline constexpr int32_t Method = 6;
    inline constexpr int32_t Variable = 7;
    inline constexpr int32_t Punctuation = 8;
    inline constexpr int32_t Annotation = 9;
    inline constexpr int32_t Preprocessor = 10;
    inline constexpr int32_t Macro = 11;
    inline constexpr int32_t Lifetime = 12;
    inline constexpr int32_t Selector = 13;
    inline constexpr int32_t Builtin = 14;
    inline constexpr int32_t Url = 15;
    inline constexpr int32_t Property = 16;
  }

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
    std::string indent_guide_foreground;
    std::string focus_background;
    std::string mark_background;
    std::unordered_map<int32_t, TextStyle> token_styles;

    TextStyle styleForToken(int32_t style_id) const;
  };

  Theme sweetLineDarkTheme();
  Theme monokaiTheme();
  Theme draculaTheme();
  Theme githubLightTheme();
  Theme githubDarkTheme();
  Theme oneDarkTheme();
  Theme solarizedDarkTheme();
  Theme nordTheme();
  Theme builtinTheme(const std::string& name);
}

#endif
