#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "sweetshot/sweetshot.h"

namespace {
  struct CliOptions {
    std::string input_path;
    std::string output_path;
    std::string language;
    std::string theme {"default"};
    std::string syntax_directory;
    sweetshot::RenderOptions render_options;
  };

  void PrintUsage(std::ostream& output) {
    output << "Usage: sweetshot [file|-] -o output.svg [options]\n"
           << "\n"
           << "Options:\n"
           << "  -o, --output <path>       Write SVG or HTML output\n"
           << "  --lang <name>            Override language detection\n"
           << "  --theme <name>           default, monokai, dracula, one-dark, solarized-dark, nord, github-dark, or github-light\n"
           << "  --syntax-dir <path>      Override SweetLine syntax directory\n"
           << "  --lines <start:end>      Render a one-based inclusive line range\n"
           << "  --focus <range-list>     Highlight one-based lines, such as 4 or 4:8,12\n"
           << "  --mark <range-list>      Mark one-based lines, such as 4 or 4:8,12\n"
           << "  --no-line-numbers        Hide line numbers\n"
           << "  --no-indent-guides       Hide indent guide lines\n"
           << "  -h, --help               Show this help message\n";
  }

  std::string ReadStdin() {
    std::ostringstream buffer;
    buffer << std::cin.rdbuf();
    return buffer.str();
  }

  std::string ReadFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
      throw std::runtime_error("Unable to read input file: " + path);
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
  }

  void WriteFile(const std::string& path, const std::string& content) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
      throw std::runtime_error("Unable to write output file: " + path);
    }
    output << content;
  }

  std::size_t ParsePositiveLine(const std::string& value) {
    const std::size_t parsed = static_cast<std::size_t>(std::stoull(value));
    if (parsed == 0) {
      throw std::runtime_error("Line numbers are one-based");
    }
    return parsed - 1;
  }

  std::vector<std::string> Split(const std::string& value, char delimiter) {
    std::vector<std::string> result;
    std::string current;
    for (char ch : value) {
      if (ch == delimiter) {
        result.push_back(current);
        current.clear();
      } else {
        current.push_back(ch);
      }
    }
    result.push_back(current);
    return result;
  }

  void AppendLineToken(const std::string& token, std::vector<std::size_t>& lines) {
    if (token.empty()) {
      return;
    }

    const std::size_t separator = token.find(':');
    if (separator == std::string::npos) {
      lines.push_back(ParsePositiveLine(token));
      return;
    }

    const std::size_t start = ParsePositiveLine(token.substr(0, separator));
    const std::size_t end = ParsePositiveLine(token.substr(separator + 1));
    if (end < start) {
      throw std::runtime_error("Line range end must be greater than or equal to start");
    }
    for (std::size_t line = start; line <= end; ++line) {
      lines.push_back(line);
    }
  }

  std::vector<std::size_t> ParseLineList(const std::string& value) {
    std::vector<std::size_t> lines;
    for (const std::string& token : Split(value, ',')) {
      AppendLineToken(token, lines);
    }
    return lines;
  }

  sweetshot::LineRange ParseRenderRange(const std::string& value) {
    const std::size_t separator = value.find(':');
    if (separator == std::string::npos) {
      const std::size_t line = ParsePositiveLine(value);
      return {true, line, 1};
    }

    const std::size_t start = ParsePositiveLine(value.substr(0, separator));
    const std::size_t end = ParsePositiveLine(value.substr(separator + 1));
    if (end < start) {
      throw std::runtime_error("Line range end must be greater than or equal to start");
    }
    return {true, start, end - start + 1};
  }

  bool HasExtension(const std::string& path, const std::string& extension) {
    std::filesystem::path output_path(path);
    return output_path.extension() == extension;
  }

  std::string RequireValue(int& index, int argc, char* argv[], const std::string& option) {
    if (index + 1 >= argc) {
      throw std::runtime_error("Missing value for " + option);
    }
    ++index;
    return argv[index];
  }

  CliOptions ParseArgs(int argc, char* argv[]) {
    CliOptions options;

    for (int index = 1; index < argc; ++index) {
      const std::string arg = argv[index];
      if (arg == "-h" || arg == "--help") {
        PrintUsage(std::cout);
        std::exit(0);
      }
      if (arg == "-o" || arg == "--output") {
        options.output_path = RequireValue(index, argc, argv, arg);
      } else if (arg == "--lang") {
        options.language = RequireValue(index, argc, argv, arg);
      } else if (arg == "--theme") {
        options.theme = RequireValue(index, argc, argv, arg);
      } else if (arg == "--syntax-dir") {
        options.syntax_directory = RequireValue(index, argc, argv, arg);
      } else if (arg == "--lines") {
        options.render_options.line_range = ParseRenderRange(RequireValue(index, argc, argv, arg));
      } else if (arg == "--focus") {
        options.render_options.focus_lines = ParseLineList(RequireValue(index, argc, argv, arg));
      } else if (arg == "--mark") {
        options.render_options.mark_lines = ParseLineList(RequireValue(index, argc, argv, arg));
      } else if (arg == "--no-line-numbers") {
        options.render_options.show_line_numbers = false;
      } else if (arg == "--no-indent-guides") {
        options.render_options.show_indent_guides = false;
      } else if (!arg.empty() && arg.front() == '-') {
        throw std::runtime_error("Unknown option: " + arg);
      } else if (options.input_path.empty()) {
        options.input_path = arg;
      } else {
        throw std::runtime_error("Unexpected argument: " + arg);
      }
    }

    if (options.output_path.empty()) {
      throw std::runtime_error("Missing output path");
    }
    return options;
  }
}

int main(int argc, char* argv[]) {
  try {
    const CliOptions cli = ParseArgs(argc, argv);

    sweetshot::RenderInput input;
    input.file_name = cli.input_path == "-" ? "" : cli.input_path;
    input.language_hint = cli.language;
    input.syntax_directory = cli.syntax_directory;
    input.options = cli.render_options;
    input.theme = sweetshot::BuiltinTheme(cli.theme);
    input.source_text = cli.input_path.empty() || cli.input_path == "-" ? ReadStdin() : ReadFile(cli.input_path);

    sweetshot::Renderer renderer;
    std::string output;
    if (HasExtension(cli.output_path, ".html")) {
      output = renderer.RenderToHtml(input);
    } else if (HasExtension(cli.output_path, ".svg")) {
      output = renderer.RenderToSvg(input);
    } else {
      throw std::runtime_error("Only SVG and HTML output are supported right now");
    }

    WriteFile(cli.output_path, output);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "sweetshot: " << error.what() << "\n";
    std::cerr << "Run sweetshot --help for usage.\n";
    return 1;
  }
}
