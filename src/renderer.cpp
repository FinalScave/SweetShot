#include "sweetshot/renderer.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "sweetline/highlight.h"
#include "sweetline/util.h"

#ifndef SWEETSHOT_DEFAULT_SYNTAX_DIR
#define SWEETSHOT_DEFAULT_SYNTAX_DIR ""
#endif

namespace sweetshot {
  struct RendererState {
    struct CachedEngine {
      sweetline::SharedPtr<sweetline::HighlightEngine> engine;
      bool directory_compiled {false};
    };

    mutable std::mutex mutex;
    mutable std::unordered_map<std::string, CachedEngine> engines;
  };

  namespace {
    struct HighlightSegment {
      std::size_t start_column {0};
      std::size_t end_column {0};
      std::string text;
      int32_t style_id {0};
    };

    struct AnalysisResult {
      std::string language;
      std::vector<std::vector<HighlightSegment>> lines;
    };

    std::string effectiveSyntaxDirectory(const RenderInput& input, const RendererConfig& config) {
      if (!input.syntax_directory.empty()) {
        return input.syntax_directory;
      }
      if (!config.syntax_directory.empty()) {
        return config.syntax_directory;
      }
      return SWEETSHOT_DEFAULT_SYNTAX_DIR;
    }

    bool isInlineStyleSyntaxFile(const std::filesystem::path& path) {
      const std::string filename = path.filename().string();
      const std::string suffix = "-inlineStyle.json";
      return filename.size() >= suffix.size()
        && filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    std::vector<std::string> listSyntaxFiles(const std::string& syntax_directory) {
      std::vector<std::string> result;
      if (syntax_directory.empty() || !std::filesystem::is_directory(syntax_directory)) {
        return result;
      }
      for (const auto& entry : std::filesystem::directory_iterator(syntax_directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json"
            && !isInlineStyleSyntaxFile(entry.path())) {
          result.push_back(entry.path().string());
        }
      }
      std::sort(result.begin(), result.end());
      return result;
    }

    std::string syntaxCacheKey(const std::string& syntax_directory, std::size_t tab_size) {
      return syntax_directory + "\n" + std::to_string(std::max<std::size_t>(tab_size, 1));
    }

    void registerTokenStyles(const sweetline::SharedPtr<sweetline::HighlightEngine>& engine) {
      engine->registerStyleName("default", token_style_id::Default);
      engine->registerStyleName("keyword", token_style_id::Keyword);
      engine->registerStyleName("string", token_style_id::String);
      engine->registerStyleName("number", token_style_id::Number);
      engine->registerStyleName("comment", token_style_id::Comment);
      engine->registerStyleName("class", token_style_id::Class);
      engine->registerStyleName("method", token_style_id::Method);
      engine->registerStyleName("variable", token_style_id::Variable);
      engine->registerStyleName("punctuation", token_style_id::Punctuation);
      engine->registerStyleName("annotation", token_style_id::Annotation);
      engine->registerStyleName("preprocessor", token_style_id::Preprocessor);
      engine->registerStyleName("macro", token_style_id::Macro);
      engine->registerStyleName("lifetime", token_style_id::Lifetime);
      engine->registerStyleName("selector", token_style_id::Selector);
      engine->registerStyleName("builtin", token_style_id::Builtin);
      engine->registerStyleName("url", token_style_id::Url);
      engine->registerStyleName("property", token_style_id::Property);
    }

    RendererState::CachedEngine& cachedEngine(RendererState& state, const RenderInput& input,
                                              const RendererConfig& config) {
      const std::string syntax_directory = effectiveSyntaxDirectory(input, config);
      const std::string key = syntaxCacheKey(syntax_directory, input.options.tab_size);
      RendererState::CachedEngine& cached = state.engines[key];
      if (cached.engine == nullptr) {
        sweetline::HighlightConfig highlight_config = sweetline::HighlightConfig::kDefault;
        highlight_config.tab_size = static_cast<int32_t>(std::max<std::size_t>(input.options.tab_size, 1));
        cached.engine = sweetline::makeSharedPtr<sweetline::HighlightEngine>(highlight_config);
        registerTokenStyles(cached.engine);
      }
      return cached;
    }

    void compileSyntaxDirectory(RendererState::CachedEngine& cached, const std::string& syntax_directory) {
      if (cached.directory_compiled) {
        return;
      }

      std::vector<std::string> pending = listSyntaxFiles(syntax_directory);

      while (!pending.empty()) {
        std::vector<std::string> next;
        bool compiled_any = false;
        for (const std::string& syntax_file : pending) {
          try {
            cached.engine->compileSyntaxFromFile(syntax_file);
            compiled_any = true;
          } catch (const sweetline::SyntaxCompileError&) {
            next.push_back(syntax_file);
          }
        }

        if (!compiled_any) {
          break;
        }
        pending = std::move(next);
      }

      cached.directory_compiled = true;
    }

    struct AnalyzerSelection {
      sweetline::SharedPtr<sweetline::TextAnalyzer> analyzer;
      std::string language;
    };

    AnalyzerSelection createAnalyzerForInput(RendererState::CachedEngine& cached, const RenderInput& input) {
      const sweetline::SharedPtr<sweetline::HighlightEngine>& engine = cached.engine;
      sweetline::SharedPtr<sweetline::SyntaxRule> rule;
      if (!input.language_hint.empty()) {
        rule = engine->getSyntaxRuleByName(input.language_hint);
        if (rule == nullptr) {
          rule = engine->getSyntaxRuleByFileName("input." + input.language_hint);
        }
      }

      if (rule == nullptr && !input.file_name.empty()) {
        rule = engine->getSyntaxRuleByFileName(input.file_name);
      }
      if (rule == nullptr) {
        return {};
      }

      AnalyzerSelection selection;
      selection.language = rule->name;
      selection.analyzer = engine->createAnalyzerBySyntaxName(rule->name);
      return selection;
    }

    std::string expandTabs(const std::string& text, std::size_t tab_size) {
      const std::size_t safe_tab_size = std::max<std::size_t>(tab_size, 1);
      std::string result;
      result.reserve(text.size());
      std::size_t column = 0;

      for (char ch : text) {
        if (ch == '\t') {
          const std::size_t spaces = safe_tab_size - (column % safe_tab_size);
          result.append(spaces, ' ');
          column += spaces;
        } else {
          result.push_back(ch);
          if (ch == '\n' || ch == '\r') {
            column = 0;
          } else {
            ++column;
          }
        }
      }
      return result;
    }

    std::vector<std::string> splitLines(const std::string& text) {
      std::vector<std::string> result;
      std::string current;

      for (std::size_t index = 0; index < text.size(); ++index) {
        const char ch = text[index];
        if (ch == '\r') {
          if (index + 1 < text.size() && text[index + 1] == '\n') {
            ++index;
          }
          result.push_back(current);
          current.clear();
        } else if (ch == '\n') {
          result.push_back(current);
          current.clear();
        } else {
          current.push_back(ch);
        }
      }

      result.push_back(current);
      return result;
    }

    std::size_t charCount(const std::string& text) {
      return sweetline::Utf8Util::countChars(text);
    }

    std::string utf8Substr(const std::string& text, std::size_t start, std::size_t count) {
      return sweetline::Utf8Util::utf8Substr(text, start, count);
    }

    AnalysisResult analyzeInput(const RenderInput& input, const RendererConfig& config, RendererState& state,
                                const std::vector<std::string>& source_lines) {
      AnalysisResult result;
      result.lines.resize(source_lines.size());

      const std::string syntax_directory = effectiveSyntaxDirectory(input, config);
      std::lock_guard<std::mutex> lock(state.mutex);
      RendererState::CachedEngine& cached = cachedEngine(state, input, config);
      compileSyntaxDirectory(cached, syntax_directory);
      AnalyzerSelection selection = createAnalyzerForInput(cached, input);
      if (selection.analyzer == nullptr) {
        return result;
      }
      result.language = selection.language;
      sweetline::SharedPtr<sweetline::DocumentHighlight> highlight = selection.analyzer->analyzeText(input.source_text);
      if (highlight == nullptr) {
        return result;
      }

      const std::size_t line_count = std::min(highlight->lines.size(), source_lines.size());
      for (std::size_t line_index = 0; line_index < line_count; ++line_index) {
        const sweetline::LineHighlight& line = highlight->lines[line_index];
        for (const sweetline::TokenSpan& span : line.spans) {
          if (span.range.end.column <= span.range.start.column) {
            continue;
          }
          HighlightSegment segment;
          segment.start_column = span.range.start.column;
          segment.end_column = span.range.end.column;
          segment.text = utf8Substr(source_lines[line_index], segment.start_column,
                                    segment.end_column - segment.start_column);
          segment.style_id = span.style_id;
          result.lines[line_index].push_back(std::move(segment));
        }
      }
      return result;
    }

    std::vector<HighlightSegment> buildRunsForLine(const std::string& line,
                                                   const std::vector<HighlightSegment>& highlighted) {
      std::vector<HighlightSegment> result;
      const std::size_t total_columns = charCount(line);
      std::size_t cursor = 0;

      for (const HighlightSegment& segment : highlighted) {
        if (segment.start_column > cursor) {
          HighlightSegment plain;
          plain.start_column = cursor;
          plain.end_column = segment.start_column;
          plain.text = utf8Substr(line, cursor, segment.start_column - cursor);
          result.push_back(std::move(plain));
        }
        if (!segment.text.empty()) {
          result.push_back(segment);
        }
        cursor = std::max(cursor, segment.end_column);
      }

      if (cursor < total_columns) {
        HighlightSegment plain;
        plain.start_column = cursor;
        plain.end_column = total_columns;
        plain.text = utf8Substr(line, cursor, total_columns - cursor);
        result.push_back(std::move(plain));
      }

      if (result.empty()) {
        HighlightSegment empty;
        result.push_back(std::move(empty));
      }
      return result;
    }

    double estimateCharWidth(const RenderOptions& options) {
      return std::max(1.0, std::round(options.font_size * 0.62 * 100.0) / 100.0);
    }

    std::size_t maxColumnsForWidth(const RenderOptions& options, double text_origin_x, double char_width) {
      const double available_width = options.max_width - text_origin_x - options.padding_x;
      if (available_width <= 0.0) {
        return 0;
      }
      return static_cast<std::size_t>(std::floor(available_width / char_width));
    }

    std::size_t constrainedColumnLimit(const RenderOptions& options, double text_origin_x, double char_width) {
      std::size_t limit = options.max_columns;
      const std::size_t width_columns = maxColumnsForWidth(options, text_origin_x, char_width);
      if (width_columns > 0) {
        limit = limit > 0 ? std::min(limit, width_columns) : width_columns;
      }
      return limit;
    }

    struct VisualRange {
      std::size_t start_column {0};
      std::size_t end_column {0};
      bool line_number_visible {true};
    };

    std::vector<VisualRange> visualRangesForLine(std::size_t total_columns, const RenderOptions& options,
                                                 std::size_t column_limit) {
      std::vector<VisualRange> ranges;
      if (total_columns == 0) {
        ranges.push_back({});
        return ranges;
      }

      if (options.long_line_mode == LongLineMode::Wrap && column_limit > 0) {
        for (std::size_t start = 0; start < total_columns; start += column_limit) {
          ranges.push_back({start, std::min(start + column_limit, total_columns), start == 0});
        }
        return ranges;
      }

      const std::size_t end_column =
        options.long_line_mode == LongLineMode::Clip && column_limit > 0
          ? std::min(total_columns, column_limit)
          : total_columns;
      ranges.push_back({0, end_column, true});
      return ranges;
    }

    std::vector<HighlightSegment> sliceSegmentsForRange(const std::string& line,
                                                        const std::vector<HighlightSegment>& segments,
                                                        std::size_t start_column,
                                                        std::size_t end_column) {
      std::vector<HighlightSegment> result;
      for (const HighlightSegment& segment : segments) {
        const std::size_t start = std::max(segment.start_column, start_column);
        const std::size_t end = std::min(segment.end_column, end_column);
        if (end <= start) {
          continue;
        }

        HighlightSegment sliced;
        sliced.start_column = start;
        sliced.end_column = end;
        sliced.text = utf8Substr(line, start, end - start);
        sliced.style_id = segment.style_id;
        result.push_back(std::move(sliced));
      }

      if (result.empty()) {
        HighlightSegment empty;
        empty.start_column = start_column;
        empty.end_column = start_column;
        result.push_back(std::move(empty));
      }
      return result;
    }

    std::unordered_set<std::size_t> toSet(const std::vector<std::size_t>& values) {
      return std::unordered_set<std::size_t>(values.begin(), values.end());
    }

    std::string escapeXml(const std::string& value) {
      std::string result;
      result.reserve(value.size());
      for (char ch : value) {
        switch (ch) {
          case '&':
            result += "&amp;";
            break;
          case '<':
            result += "&lt;";
            break;
          case '>':
            result += "&gt;";
            break;
          case '"':
            result += "&quot;";
            break;
          case '\'':
            result += "&#39;";
            break;
          default:
            result.push_back(ch);
            break;
        }
      }
      return result;
    }

    std::string cssTextStyle(const TextStyle& style) {
      std::ostringstream css;
      if (!style.foreground.empty()) {
        css << "color:" << style.foreground << ";";
      }
      if (!style.background.empty()) {
        css << "background:" << style.background << ";";
      }
      if (style.bold) {
        css << "font-weight:700;";
      }
      if (style.italic) {
        css << "font-style:italic;";
      }
      if (style.underline) {
        css << "text-decoration:underline;";
      }
      return css.str();
    }

    std::string svgFontStyle(const TextStyle& style) {
      std::ostringstream attributes;
      if (!style.foreground.empty()) {
        attributes << " fill=\"" << escapeXml(style.foreground) << "\"";
      }
      if (style.bold) {
        attributes << " font-weight=\"700\"";
      }
      if (style.italic) {
        attributes << " font-style=\"italic\"";
      }
      if (style.underline) {
        attributes << " text-decoration=\"underline\"";
      }
      return attributes.str();
    }
  }

  Renderer::Renderer(RendererConfig config)
    : config_(std::move(config)), state_(std::make_shared<RendererState>()) {
  }

  RenderScene Renderer::renderScene(const RenderInput& input) const {
    RenderInput normalized_input = input;
    normalized_input.source_text = expandTabs(input.source_text, input.options.tab_size);
    std::vector<std::string> source_lines = splitLines(normalized_input.source_text);
    AnalysisResult analysis = analyzeInput(normalized_input, config_, *state_, source_lines);

    const std::size_t total_lines = source_lines.size();
    std::size_t start_line = 0;
    std::size_t end_line = total_lines;
    if (input.options.line_range.enabled) {
      start_line = std::min(input.options.line_range.start_line, total_lines);
      end_line = std::min(total_lines, start_line + input.options.line_range.line_count);
    }

    const std::unordered_set<std::size_t> focus_lines = toSet(input.options.focus_lines);
    const std::unordered_set<std::size_t> mark_lines = toSet(input.options.mark_lines);

    RenderScene scene;
    scene.theme = input.theme;
    scene.options = input.options;
    scene.language = analysis.language;
    scene.file_name = input.file_name;
    scene.char_width = estimateCharWidth(input.options);
    scene.text_origin_x = input.options.padding_x
      + (input.options.show_line_numbers ? input.options.gutter_width : 0.0);
    const std::size_t column_limit = constrainedColumnLimit(input.options, scene.text_origin_x, scene.char_width);

    std::size_t max_visual_columns = 0;

    for (std::size_t line_index = start_line; line_index < end_line; ++line_index) {
      const std::vector<HighlightSegment> highlighted =
        line_index < analysis.lines.size() ? analysis.lines[line_index] : std::vector<HighlightSegment> {};
      std::vector<HighlightSegment> segments = buildRunsForLine(source_lines[line_index], highlighted);
      const std::size_t total_columns = charCount(source_lines[line_index]);
      for (const VisualRange& range : visualRangesForLine(total_columns, input.options, column_limit)) {
        SceneLine scene_line;
        scene_line.source_line = line_index;
        scene_line.text = utf8Substr(source_lines[line_index], range.start_column,
                                     range.end_column - range.start_column);
        scene_line.y = input.options.padding_y
          + static_cast<double>(scene.lines.size()) * input.options.line_height;
        scene_line.focused = focus_lines.find(line_index) != focus_lines.end();
        scene_line.marked = mark_lines.find(line_index) != mark_lines.end();
        scene_line.line_number_visible = range.line_number_visible;
        max_visual_columns = std::max(max_visual_columns, charCount(scene_line.text));

        for (const HighlightSegment& segment :
             sliceSegmentsForRange(source_lines[line_index], segments, range.start_column, range.end_column)) {
          TextRun run;
          run.column = segment.start_column;
          run.x = scene.text_origin_x
            + static_cast<double>(segment.start_column - range.start_column) * scene.char_width;
          run.y = scene_line.y + input.options.font_size;
          run.text = segment.text;
          run.style_id = segment.style_id;
          run.style = input.theme.styleForToken(segment.style_id);
          scene_line.runs.push_back(std::move(run));
        }
        scene.lines.push_back(std::move(scene_line));
      }
    }

    scene.width = std::min(input.options.max_width,
                           scene.text_origin_x + input.options.padding_x
                           + static_cast<double>(max_visual_columns) * scene.char_width);
    scene.height = input.options.padding_y * 2.0
      + static_cast<double>(scene.lines.size()) * input.options.line_height;

    return scene;
  }

  std::string Renderer::renderToSvg(const RenderInput& input) const {
    const RenderScene scene = renderScene(input);
    std::ostringstream svg;

    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << scene.width
        << "\" height=\"" << scene.height << "\" viewBox=\"0 0 " << scene.width
        << " " << scene.height << "\">\n";
    svg << "<rect width=\"100%\" height=\"100%\" fill=\"" << escapeXml(scene.theme.background) << "\"/>\n";

    if (scene.options.show_line_numbers) {
      svg << "<rect x=\"0\" y=\"0\" width=\"" << scene.options.gutter_width + scene.options.padding_x
          << "\" height=\"" << scene.height << "\" fill=\""
          << escapeXml(scene.theme.line_number_background) << "\"/>\n";
      svg << "<line x1=\"" << scene.text_origin_x - scene.options.padding_x * 0.5
          << "\" y1=\"0\" x2=\"" << scene.text_origin_x - scene.options.padding_x * 0.5
          << "\" y2=\"" << scene.height << "\" stroke=\"" << escapeXml(scene.theme.gutter_border) << "\"/>\n";
    }

    svg << "<g font-family=\"" << escapeXml(scene.options.font_family) << "\" font-size=\""
        << scene.options.font_size << "\" xml:space=\"preserve\">\n";
    for (const SceneLine& line : scene.lines) {
      if (line.marked) {
        svg << "<rect x=\"0\" y=\"" << line.y << "\" width=\"" << scene.width
            << "\" height=\"" << scene.options.line_height << "\" fill=\""
            << escapeXml(scene.theme.mark_background) << "\"/>\n";
      } else if (line.focused) {
        svg << "<rect x=\"0\" y=\"" << line.y << "\" width=\"" << scene.width
            << "\" height=\"" << scene.options.line_height << "\" fill=\""
            << escapeXml(scene.theme.focus_background) << "\"/>\n";
      }

      if (scene.options.show_line_numbers && line.line_number_visible) {
        svg << "<text x=\"" << scene.text_origin_x - scene.options.padding_x
            << "\" y=\"" << line.y + scene.options.font_size << "\" text-anchor=\"end\" fill=\""
            << escapeXml(scene.theme.line_number_foreground) << "\">"
            << (line.source_line + 1) << "</text>\n";
      }

      for (const TextRun& run : line.runs) {
        if (run.text.empty()) {
          continue;
        }
        svg << "<text x=\"" << run.x << "\" y=\"" << run.y << "\""
            << svgFontStyle(run.style) << ">" << escapeXml(run.text) << "</text>\n";
      }
    }
    svg << "</g>\n</svg>\n";
    return svg.str();
  }

  std::string Renderer::renderToHtml(const RenderInput& input) const {
    const RenderScene scene = renderScene(input);
    std::ostringstream html;

    html << "<!doctype html>\n<html><head><meta charset=\"utf-8\"><style>\n";
    html << ".sweetshot{box-sizing:border-box;width:" << scene.width << "px;background:"
         << scene.theme.background << ";color:" << scene.theme.foreground << ";font-family:"
         << scene.options.font_family << ";font-size:" << scene.options.font_size
         << "px;line-height:" << scene.options.line_height << "px;padding:"
         << scene.options.padding_y << "px 0;overflow:hidden;font-variant-ligatures:none;}\n";
    html << ".sweetshot-line{display:flex;min-height:" << scene.options.line_height
         << "px;white-space:pre;}\n";
    html << ".sweetshot-line.focus{background:" << scene.theme.focus_background << ";}\n";
    html << ".sweetshot-line.mark{background:" << scene.theme.mark_background << ";}\n";
    html << ".sweetshot-gutter{box-sizing:border-box;width:"
         << scene.text_origin_x << "px;padding:0 " << scene.options.padding_x
         << "px 0 0;text-align:right;color:" << scene.theme.line_number_foreground
         << ";background:" << scene.theme.line_number_background << ";border-right:1px solid "
         << scene.theme.gutter_border << ";user-select:none;}\n";
    html << ".sweetshot-code{padding-left:" << scene.options.padding_x
         << "px;min-width:0;white-space:pre;}\n";
    html << "</style></head><body><div class=\"sweetshot\">";

    for (const SceneLine& line : scene.lines) {
      const char* state_class = line.marked ? " mark" : (line.focused ? " focus" : "");
      html << "<div class=\"sweetshot-line" << state_class << "\">";
      if (scene.options.show_line_numbers) {
        html << "<span class=\"sweetshot-gutter\">";
        if (line.line_number_visible) {
          html << (line.source_line + 1);
        }
        html << "</span>";
      }
      html << "<span class=\"sweetshot-code\">";
      for (const TextRun& run : line.runs) {
        if (run.text.empty()) {
          continue;
        }
        html << "<span style=\"" << cssTextStyle(run.style) << "\">"
             << escapeXml(run.text) << "</span>";
      }
      html << "</span></div>";
    }

    html << "</div></body></html>\n";
    return html.str();
  }
}
