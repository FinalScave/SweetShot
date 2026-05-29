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

    std::string normalizeThemeName(std::string value) {
      std::string result;
      result.reserve(value.size());
      for (unsigned char ch : value) {
        if (std::isalnum(ch)) {
          result.push_back(static_cast<char>(std::tolower(ch)));
        }
      }
      return result;
    }

    int hexValue(char ch) {
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

    int hexByte(const std::string& value, std::size_t offset) {
      return hexValue(value[offset]) * 16 + hexValue(value[offset + 1]);
    }

    Rgb parseHexColor(const std::string& value) {
      if (value.size() != 7 || value.front() != '#') {
        return {};
      }
      return {hexByte(value, 1), hexByte(value, 3), hexByte(value, 5)};
    }

    std::string formatHexColor(const Rgb& color) {
      std::ostringstream output;
      output << "#" << std::hex << std::nouppercase << std::setfill('0')
             << std::setw(2) << color.red
             << std::setw(2) << color.green
             << std::setw(2) << color.blue;
      return output.str();
    }

    std::string blendColor(const std::string& base, const std::string& target, double ratio) {
      const Rgb base_color = parseHexColor(base);
      const Rgb target_color = parseHexColor(target);
      const double clamped = std::clamp(ratio, 0.0, 1.0);
      const auto mix = [clamped](int from, int to) {
        return static_cast<int>(std::round(static_cast<double>(from) * (1.0 - clamped)
                                           + static_cast<double>(to) * clamped));
      };
      return formatHexColor({
        mix(base_color.red, target_color.red),
        mix(base_color.green, target_color.green),
        mix(base_color.blue, target_color.blue)
      });
    }

    void applyFrameColors(Theme& theme) {
      theme.line_number_foreground = blendColor(theme.foreground, theme.background, 0.48);
      theme.line_number_background = blendColor(theme.background, theme.foreground, 0.08);
      theme.gutter_border = blendColor(theme.background, theme.foreground, 0.14);
      theme.indent_guide_foreground = blendColor(theme.background, theme.foreground, 0.35);
      theme.focus_background = blendColor(theme.background,
                                          theme.token_styles[token_style_id::Keyword].foreground, 0.20);
      theme.mark_background = blendColor(theme.background,
                                         theme.token_styles[token_style_id::String].foreground, 0.16);
    }

    Theme makeTheme(const std::string& name, const char* background, const char* foreground,
                    const TokenPalette& palette) {
      Theme theme;
      theme.name = name;
      theme.background = background;
      theme.foreground = foreground;
      theme.token_styles[token_style_id::Default] = {theme.foreground, ""};
      theme.token_styles[token_style_id::Keyword] = {palette.keyword, ""};
      theme.token_styles[token_style_id::String] = {palette.string, ""};
      theme.token_styles[token_style_id::Number] = {palette.number, ""};
      theme.token_styles[token_style_id::Comment] = {palette.comment, ""};
      theme.token_styles[token_style_id::Class] = {palette.klass, ""};
      theme.token_styles[token_style_id::Method] = {palette.method, ""};
      theme.token_styles[token_style_id::Variable] = {palette.variable, ""};
      theme.token_styles[token_style_id::Punctuation] = {palette.punctuation, ""};
      theme.token_styles[token_style_id::Annotation] = {palette.annotation, ""};
      theme.token_styles[token_style_id::Preprocessor] = {palette.preprocessor, ""};
      theme.token_styles[token_style_id::Macro] = {palette.macro, ""};
      theme.token_styles[token_style_id::Lifetime] = {palette.lifetime, ""};
      theme.token_styles[token_style_id::Selector] = {palette.selector, ""};
      theme.token_styles[token_style_id::Builtin] = {palette.builtin, ""};
      theme.token_styles[token_style_id::Url] = {palette.url, ""};
      theme.token_styles[token_style_id::Property] = {palette.property, ""};
      applyFrameColors(theme);
      return theme;
    }
  }

  TextStyle Theme::styleForToken(int32_t style_id) const {
    const auto it = token_styles.find(style_id);
    if (it != token_styles.end()) {
      return it->second;
    }
    const auto default_it = token_styles.find(token_style_id::Default);
    if (default_it != token_styles.end()) {
      return default_it->second;
    }
    return {foreground, ""};
  }

  Theme defaultTheme() {
    return makeTheme("default", "#1e1e1e", "#d4d4d4", {
      "#569cd6", "#bd63c5", "#e4fad5", "#60ae6f", "#4ec9b0", "#9cdcfe", "#9b9bc8", "#d69d85",
      "#fffd9b", "#569cd6", "#9b9bc8", "#4ec9b0", "#4ec9b0", "#569cd6", "#4fc1ff", "#9cdcfe"
    });
  }

  Theme monokaiTheme() {
    return makeTheme("monokai", "#272822", "#f8f8f2", {
      "#f92672", "#e6db74", "#ae81ff", "#75715e", "#a6e22e", "#a6e22e", "#f8f8f2", "#f8f8f2",
      "#e6db74", "#f92672", "#ae81ff", "#fd971f", "#a6e22e", "#66d9ef", "#66d9ef", "#a6e22e"
    });
  }

  Theme draculaTheme() {
    return makeTheme("dracula", "#282a36", "#f8f8f2", {
      "#ff79c6", "#f1fa8c", "#bd93f9", "#6272a4", "#8be9fd", "#50fa7b", "#f8f8f2", "#f8f8f2",
      "#ffb86c", "#ff79c6", "#bd93f9", "#ffb86c", "#50fa7b", "#8be9fd", "#8be9fd", "#50fa7b"
    });
  }

  Theme githubLightTheme() {
    return makeTheme("github-light", "#ffffff", "#24292f", {
      "#cf222e", "#0a3069", "#0550ae", "#6e7781", "#953800", "#8250df", "#953800", "#24292f",
      "#953800", "#cf222e", "#0550ae", "#953800", "#116329", "#0550ae", "#0550ae", "#0550ae"
    });
  }

  Theme githubDarkTheme() {
    return makeTheme("github-dark", "#0d1117", "#c9d1d9", {
      "#ff7b72", "#a5d6ff", "#79c0ff", "#8b949e", "#ffa657", "#d2a8ff", "#ffa657", "#c9d1d9",
      "#ffa657", "#ff7b72", "#79c0ff", "#ffa657", "#7ee787", "#79c0ff", "#79c0ff", "#79c0ff"
    });
  }

  Theme oneDarkTheme() {
    return makeTheme("one-dark", "#282c34", "#abb2bf", {
      "#c678dd", "#98c379", "#d19a66", "#5c6370", "#e5c07b", "#61afef", "#e06c75", "#abb2bf",
      "#e5c07b", "#c678dd", "#d19a66", "#56b6c2", "#e5c07b", "#56b6c2", "#61afef", "#61afef"
    });
  }

  Theme solarizedDarkTheme() {
    return makeTheme("solarized-dark", "#002b36", "#839496", {
      "#859900", "#2aa198", "#d33682", "#586e75", "#b58900", "#268bd2", "#cb4b16", "#839496",
      "#b58900", "#859900", "#cb4b16", "#d33682", "#268bd2", "#268bd2", "#268bd2", "#268bd2"
    });
  }

  Theme nordTheme() {
    return makeTheme("nord", "#2e3440", "#d8dee9", {
      "#81a1c1", "#a3be8c", "#b48ead", "#616e88", "#8fbcbb", "#88c0d0", "#d8dee9", "#eceff4",
      "#ebcb8b", "#81a1c1", "#b48ead", "#ebcb8b", "#8fbcbb", "#5e81ac", "#88c0d0", "#88c0d0"
    });
  }

  Theme builtinTheme(const std::string& name) {
    const std::string normalized = normalizeThemeName(name);
    if (normalized.empty() || normalized == "sweetlinedark" || normalized == "default" || normalized == "dark") {
      return defaultTheme();
    }
    if (normalized == "monokai") {
      return monokaiTheme();
    }
    if (normalized == "dracula") {
      return draculaTheme();
    }
    if (normalized == "onedark") {
      return oneDarkTheme();
    }
    if (normalized == "solarizeddark") {
      return solarizedDarkTheme();
    }
    if (normalized == "nord") {
      return nordTheme();
    }
    if (normalized == "githubdark") {
      return githubDarkTheme();
    }
    if (normalized == "githublight" || normalized == "light") {
      return githubLightTheme();
    }
    return defaultTheme();
  }
}
