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

      theme.token_styles["default"] = {theme.foreground, ""};
      theme.token_styles["keyword"] = {keyword, "", true};
      theme.token_styles["string"] = {string, ""};
      theme.token_styles["number"] = {number, ""};
      theme.token_styles["comment"] = {comment, "", false, true};
      theme.token_styles["class"] = {type, ""};
      theme.token_styles["type"] = {type, ""};
      theme.token_styles["method"] = {function, ""};
      theme.token_styles["function"] = {function, ""};
      theme.token_styles["variable"] = {variable, ""};
      theme.token_styles["punctuation"] = {punctuation, ""};
      theme.token_styles["operator"] = {punctuation, ""};
      theme.token_styles["annotation"] = {builtin, ""};
      theme.token_styles["builtin"] = {builtin, ""};
      theme.token_styles["preprocessor"] = {keyword, ""};
      theme.token_styles["macro"] = {keyword, ""};
      theme.token_styles["property"] = {variable, ""};
      theme.token_styles["selector"] = {function, ""};
      theme.token_styles["url"] = {string, "", false, false, true};
      theme.token_styles["tag"] = {keyword, ""};
      theme.token_styles["attribute"] = {variable, ""};
      theme.token_styles["constant"] = {number, ""};
      theme.token_styles["namespace"] = {type, ""};
      theme.token_styles["regexp"] = {string, ""};
      theme.token_styles["escape"] = {number, ""};
    }
  }

  TextStyle Theme::styleForToken(const std::string& style_name) const {
    const auto it = token_styles.find(style_name);
    if (it != token_styles.end()) {
      return it->second;
    }
    const auto default_it = token_styles.find("default");
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
    theme.token_styles["default"] = {theme.foreground, ""};
    theme.token_styles["keyword"] = {"#c678dd", "", true};
    theme.token_styles["string"] = {"#98c379", ""};
    theme.token_styles["number"] = {"#d19a66", ""};
    theme.token_styles["comment"] = {"#7f848e", "", false, true};
    theme.token_styles["class"] = {"#e5c07b", ""};
    theme.token_styles["type"] = {"#e5c07b", ""};
    theme.token_styles["method"] = {"#61afef", ""};
    theme.token_styles["function"] = {"#61afef", ""};
    theme.token_styles["variable"] = {"#e06c75", ""};
    theme.token_styles["punctuation"] = {"#abb2bf", ""};
    theme.token_styles["operator"] = {"#56b6c2", ""};
    theme.token_styles["annotation"] = {"#56b6c2", ""};
    theme.token_styles["builtin"] = {"#56b6c2", ""};
    theme.token_styles["preprocessor"] = {"#c678dd", ""};
    theme.token_styles["macro"] = {"#c678dd", ""};
    theme.token_styles["property"] = {"#e06c75", ""};
    theme.token_styles["selector"] = {"#61afef", ""};
    theme.token_styles["url"] = {"#98c379", "", false, false, true};
    theme.token_styles["tag"] = {"#e06c75", ""};
    theme.token_styles["attribute"] = {"#d19a66", ""};
    theme.token_styles["constant"] = {"#d19a66", ""};
    theme.token_styles["namespace"] = {"#e5c07b", ""};
    theme.token_styles["regexp"] = {"#98c379", ""};
    theme.token_styles["escape"] = {"#d19a66", ""};
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
