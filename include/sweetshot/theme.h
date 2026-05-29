#ifndef SWEETSHOT_THEME_H
#define SWEETSHOT_THEME_H

#include <cstdint>
#include <string>
#include <unordered_map>

namespace sweetshot {
  namespace token_style_id {
    inline constexpr int32_t kDefault = 0;
    inline constexpr int32_t kKeyword = 1;
    inline constexpr int32_t kString = 2;
    inline constexpr int32_t kNumber = 3;
    inline constexpr int32_t kComment = 4;
    inline constexpr int32_t kClass = 5;
    inline constexpr int32_t kMethod = 6;
    inline constexpr int32_t kVariable = 7;
    inline constexpr int32_t kPunctuation = 8;
    inline constexpr int32_t kAnnotation = 9;
    inline constexpr int32_t kPreprocessor = 10;
    inline constexpr int32_t kMacro = 11;
    inline constexpr int32_t kLifetime = 12;
    inline constexpr int32_t kSelector = 13;
    inline constexpr int32_t kBuiltin = 14;
    inline constexpr int32_t kUrl = 15;
    inline constexpr int32_t kProperty = 16;
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

    TextStyle StyleForToken(int32_t style_id) const;
  };

  Theme DefaultTheme();
  Theme MonokaiTheme();
  Theme DraculaTheme();
  Theme GithubLightTheme();
  Theme GithubDarkTheme();
  Theme OneDarkTheme();
  Theme SolarizedDarkTheme();
  Theme NordTheme();
  Theme BuiltinTheme(const std::string& name);
}

#endif
