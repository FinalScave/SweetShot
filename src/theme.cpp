#include "sweetshot/theme.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace sweetshot {
  namespace {
    struct Rgb {
      int red {0};
      int green {0};
      int blue {0};
    };

    struct TokenPalette {
      const char* keyword;
      const char* string;
      const char* number;
      const char* comment;
      const char* klass;
      const char* method;
      const char* variable;
      const char* punctuation;
      const char* annotation;
      const char* preprocessor;
      const char* macro;
      const char* lifetime;
      const char* selector;
      const char* builtin;
      const char* url;
      const char* property;
    };

    std::string NormalizeThemeName(std::string value) {
      std::string result;
      result.reserve(value.size());
      for (unsigned char ch : value) {
        if (std::isalnum(ch)) {
          result.push_back(static_cast<char>(std::tolower(ch)));
        }
      }
      return result;
    }

    int HexValue(char ch) {
      if (ch >= '0' && ch <= '9') {
        return ch - '0';
      }
      if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
      }
      if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
      }
      return 0;
    }

    int HexByte(const std::string& value, std::size_t offset) {
      return HexValue(value[offset]) * 16 + HexValue(value[offset + 1]);
    }

    Rgb ParseHexColor(const std::string& value) {
      if (value.size() != 7 || value.front() != '#') {
        return {};
      }
      return {HexByte(value, 1), HexByte(value, 3), HexByte(value, 5)};
    }

    std::string FormatHexColor(const Rgb& color) {
      std::ostringstream output;
      output << "#" << std::hex << std::nouppercase << std::setfill('0')
             << std::setw(2) << color.red
             << std::setw(2) << color.green
             << std::setw(2) << color.blue;
      return output.str();
    }

    std::string BlendColor(const std::string& base, const std::string& target, double ratio) {
      const Rgb base_color = ParseHexColor(base);
      const Rgb target_color = ParseHexColor(target);
      const double clamped = std::clamp(ratio, 0.0, 1.0);
      const auto mix = [clamped](int from, int to) {
        return static_cast<int>(std::round(static_cast<double>(from) * (1.0 - clamped)
                                           + static_cast<double>(to) * clamped));
      };
      return FormatHexColor({
        mix(base_color.red, target_color.red),
        mix(base_color.green, target_color.green),
        mix(base_color.blue, target_color.blue)
      });
    }

    void ApplyFrameColors(Theme& theme) {
      theme.line_number_foreground = BlendColor(theme.foreground, theme.background, 0.48);
      theme.line_number_background = BlendColor(theme.background, theme.foreground, 0.08);
      theme.gutter_border = BlendColor(theme.background, theme.foreground, 0.14);
      theme.indent_guide_foreground = BlendColor(theme.background, theme.foreground, 0.35);
      theme.focus_background = BlendColor(theme.background,
                                          theme.token_styles[token_style_id::kKeyword].foreground, 0.20);
      theme.mark_background = BlendColor(theme.background,
                                         theme.token_styles[token_style_id::kString].foreground, 0.16);
    }

    Theme MakeTheme(const std::string& name, const char* background, const char* foreground,
                    const TokenPalette& palette) {
      Theme theme;
      theme.name = name;
      theme.background = background;
      theme.foreground = foreground;
      theme.token_styles[token_style_id::kDefault] = {theme.foreground, ""};
      theme.token_styles[token_style_id::kKeyword] = {palette.keyword, ""};
      theme.token_styles[token_style_id::kString] = {palette.string, ""};
      theme.token_styles[token_style_id::kNumber] = {palette.number, ""};
      theme.token_styles[token_style_id::kComment] = {palette.comment, ""};
      theme.token_styles[token_style_id::kClass] = {palette.klass, ""};
      theme.token_styles[token_style_id::kMethod] = {palette.method, ""};
      theme.token_styles[token_style_id::kVariable] = {palette.variable, ""};
      theme.token_styles[token_style_id::kPunctuation] = {palette.punctuation, ""};
      theme.token_styles[token_style_id::kAnnotation] = {palette.annotation, ""};
      theme.token_styles[token_style_id::kPreprocessor] = {palette.preprocessor, ""};
      theme.token_styles[token_style_id::kMacro] = {palette.macro, ""};
      theme.token_styles[token_style_id::kLifetime] = {palette.lifetime, ""};
      theme.token_styles[token_style_id::kSelector] = {palette.selector, ""};
      theme.token_styles[token_style_id::kBuiltin] = {palette.builtin, ""};
      theme.token_styles[token_style_id::kUrl] = {palette.url, ""};
      theme.token_styles[token_style_id::kProperty] = {palette.property, ""};
      ApplyFrameColors(theme);
      return theme;
    }
  }

  TextStyle Theme::StyleForToken(int32_t style_id) const {
    const auto it = token_styles.find(style_id);
    if (it != token_styles.end()) {
      return it->second;
    }
    const auto default_it = token_styles.find(token_style_id::kDefault);
    if (default_it != token_styles.end()) {
      return default_it->second;
    }
    return {foreground, ""};
  }

  Theme DefaultTheme() {
    return MakeTheme("default", "#1e1e1e", "#d4d4d4", {
      "#569cd6", "#bd63c5", "#e4fad5", "#60ae6f", "#4ec9b0", "#9cdcfe", "#9b9bc8", "#d69d85",
      "#fffd9b", "#569cd6", "#9b9bc8", "#4ec9b0", "#4ec9b0", "#569cd6", "#4fc1ff", "#9cdcfe"
    });
  }

  Theme MonokaiTheme() {
    return MakeTheme("monokai", "#272822", "#f8f8f2", {
      "#f92672", "#e6db74", "#ae81ff", "#75715e", "#a6e22e", "#a6e22e", "#f8f8f2", "#f8f8f2",
      "#e6db74", "#f92672", "#ae81ff", "#fd971f", "#a6e22e", "#66d9ef", "#66d9ef", "#a6e22e"
    });
  }

  Theme DraculaTheme() {
    return MakeTheme("dracula", "#282a36", "#f8f8f2", {
      "#ff79c6", "#f1fa8c", "#bd93f9", "#6272a4", "#8be9fd", "#50fa7b", "#f8f8f2", "#f8f8f2",
      "#ffb86c", "#ff79c6", "#bd93f9", "#ffb86c", "#50fa7b", "#8be9fd", "#8be9fd", "#50fa7b"
    });
  }

  Theme GithubLightTheme() {
    return MakeTheme("github-light", "#ffffff", "#24292f", {
      "#cf222e", "#0a3069", "#0550ae", "#6e7781", "#953800", "#8250df", "#953800", "#24292f",
      "#953800", "#cf222e", "#0550ae", "#953800", "#116329", "#0550ae", "#0550ae", "#0550ae"
    });
  }

  Theme GithubDarkTheme() {
    return MakeTheme("github-dark", "#0d1117", "#c9d1d9", {
      "#ff7b72", "#a5d6ff", "#79c0ff", "#8b949e", "#ffa657", "#d2a8ff", "#ffa657", "#c9d1d9",
      "#ffa657", "#ff7b72", "#79c0ff", "#ffa657", "#7ee787", "#79c0ff", "#79c0ff", "#79c0ff"
    });
  }

  Theme OneDarkTheme() {
    return MakeTheme("one-dark", "#282c34", "#abb2bf", {
      "#c678dd", "#98c379", "#d19a66", "#5c6370", "#e5c07b", "#61afef", "#e06c75", "#abb2bf",
      "#e5c07b", "#c678dd", "#d19a66", "#56b6c2", "#e5c07b", "#56b6c2", "#61afef", "#61afef"
    });
  }

  Theme SolarizedDarkTheme() {
    return MakeTheme("solarized-dark", "#002b36", "#839496", {
      "#859900", "#2aa198", "#d33682", "#586e75", "#b58900", "#268bd2", "#cb4b16", "#839496",
      "#b58900", "#859900", "#cb4b16", "#d33682", "#268bd2", "#268bd2", "#268bd2", "#268bd2"
    });
  }

  Theme NordTheme() {
    return MakeTheme("nord", "#2e3440", "#d8dee9", {
      "#81a1c1", "#a3be8c", "#b48ead", "#616e88", "#8fbcbb", "#88c0d0", "#d8dee9", "#eceff4",
      "#ebcb8b", "#81a1c1", "#b48ead", "#ebcb8b", "#8fbcbb", "#5e81ac", "#88c0d0", "#88c0d0"
    });
  }

  Theme BuiltinTheme(const std::string& name) {
    const std::string normalized = NormalizeThemeName(name);
    if (normalized.empty() || normalized == "sweetlinedark" || normalized == "default" || normalized == "dark") {
      return DefaultTheme();
    }
    if (normalized == "monokai") {
      return MonokaiTheme();
    }
    if (normalized == "dracula") {
      return DraculaTheme();
    }
    if (normalized == "onedark") {
      return OneDarkTheme();
    }
    if (normalized == "solarizeddark") {
      return SolarizedDarkTheme();
    }
    if (normalized == "nord") {
      return NordTheme();
    }
    if (normalized == "githubdark") {
      return GithubDarkTheme();
    }
    if (normalized == "githublight" || normalized == "light") {
      return GithubLightTheme();
    }
    return DefaultTheme();
  }
}
