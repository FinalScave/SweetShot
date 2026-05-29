#include "sweetshot/theme.h"

#include <algorithm>
#include <cctype>

namespace sweetshot {
  namespace {
    std::string lowerThemeName(std::string value) {
      std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
      });
      return value;
    }

    void addCommonStyles(Theme& theme, bool dark) {
      const std::string keyword = dark ? "#ff7b72" : "#cf222e";
      const std::string string = dark ? "#a5d6ff" : "#0a3069";
      const std::string number = dark ? "#79c0ff" : "#0550ae";
      const std::string comment = dark ? "#8b949e" : "#6e7781";
      const std::string type = dark ? "#ffa657" : "#953800";
      const std::string function = dark ? "#d2a8ff" : "#8250df";
      const std::string variable = dark ? "#ffa657" : "#953800";
      const std::string punctuation = dark ? "#c9d1d9" : "#24292f";
      const std::string builtin = dark ? "#7ee787" : "#116329";

      theme.token_styles[token_style_id::Default] = {theme.foreground, ""};
      theme.token_styles[token_style_id::Keyword] = {keyword, "", true};
      theme.token_styles[token_style_id::String] = {string, ""};
      theme.token_styles[token_style_id::Number] = {number, ""};
      theme.token_styles[token_style_id::Comment] = {comment, "", false, true};
      theme.token_styles[token_style_id::Class] = {type, ""};
      theme.token_styles[token_style_id::Method] = {function, ""};
      theme.token_styles[token_style_id::Variable] = {variable, ""};
      theme.token_styles[token_style_id::Punctuation] = {punctuation, ""};
      theme.token_styles[token_style_id::Annotation] = {builtin, ""};
      theme.token_styles[token_style_id::Builtin] = {builtin, ""};
      theme.token_styles[token_style_id::Preprocessor] = {keyword, ""};
      theme.token_styles[token_style_id::Macro] = {keyword, ""};
      theme.token_styles[token_style_id::Property] = {variable, ""};
      theme.token_styles[token_style_id::Lifetime] = {type, ""};
      theme.token_styles[token_style_id::Selector] = {function, ""};
      theme.token_styles[token_style_id::Url] = {string, "", false, false, true};
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

  Theme githubLightTheme() {
    Theme theme;
    theme.name = "github-light";
    theme.background = "#ffffff";
    theme.foreground = "#24292f";
    theme.line_number_foreground = "#8c959f";
    theme.line_number_background = "#f6f8fa";
    theme.gutter_border = "#d0d7de";
    theme.focus_background = "#fff8c5";
    theme.mark_background = "#ddf4ff";
    addCommonStyles(theme, false);
    return theme;
  }

  Theme githubDarkTheme() {
    Theme theme;
    theme.name = "github-dark";
    theme.background = "#0d1117";
    theme.foreground = "#c9d1d9";
    theme.line_number_foreground = "#6e7681";
    theme.line_number_background = "#161b22";
    theme.gutter_border = "#30363d";
    theme.focus_background = "#2d2513";
    theme.mark_background = "#1f2a36";
    addCommonStyles(theme, true);
    return theme;
  }

  Theme oneDarkTheme() {
    Theme theme;
    theme.name = "one-dark";
    theme.background = "#282c34";
    theme.foreground = "#abb2bf";
    theme.line_number_foreground = "#636d83";
    theme.line_number_background = "#21252b";
    theme.gutter_border = "#3b4048";
    theme.focus_background = "#333842";
    theme.mark_background = "#2b3a48";
    theme.token_styles[token_style_id::Default] = {theme.foreground, ""};
    theme.token_styles[token_style_id::Keyword] = {"#c678dd", "", true};
    theme.token_styles[token_style_id::String] = {"#98c379", ""};
    theme.token_styles[token_style_id::Number] = {"#d19a66", ""};
    theme.token_styles[token_style_id::Comment] = {"#7f848e", "", false, true};
    theme.token_styles[token_style_id::Class] = {"#e5c07b", ""};
    theme.token_styles[token_style_id::Method] = {"#61afef", ""};
    theme.token_styles[token_style_id::Variable] = {"#e06c75", ""};
    theme.token_styles[token_style_id::Punctuation] = {"#abb2bf", ""};
    theme.token_styles[token_style_id::Annotation] = {"#56b6c2", ""};
    theme.token_styles[token_style_id::Builtin] = {"#56b6c2", ""};
    theme.token_styles[token_style_id::Preprocessor] = {"#c678dd", ""};
    theme.token_styles[token_style_id::Macro] = {"#c678dd", ""};
    theme.token_styles[token_style_id::Property] = {"#e06c75", ""};
    theme.token_styles[token_style_id::Lifetime] = {"#e5c07b", ""};
    theme.token_styles[token_style_id::Selector] = {"#61afef", ""};
    theme.token_styles[token_style_id::Url] = {"#98c379", "", false, false, true};
    return theme;
  }

  Theme builtinTheme(const std::string& name) {
    const std::string normalized = lowerThemeName(name);
    if (normalized == "github-dark" || normalized == "dark") {
      return githubDarkTheme();
    }
    if (normalized == "one-dark" || normalized == "onedark") {
      return oneDarkTheme();
    }
    return githubLightTheme();
  }
}
