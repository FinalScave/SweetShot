#include "sweetshot/rasterizer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "lodepng.h"
#include "resvg.h"

namespace sweetshot {
  namespace {
    constexpr std::uint32_t kMaxImageDimension = 100000;
    constexpr std::uint64_t kMaxPixelCount = 100000000;

    struct Rgba {
      std::uint8_t red {0};
      std::uint8_t green {0};
      std::uint8_t blue {0};
      std::uint8_t alpha {0};
    };

    class ResvgOptionsHandle {
    public:
      ResvgOptionsHandle()
        : options_(resvg_options_create()) {
        if (options_ == nullptr) {
          throw std::runtime_error("Unable to create resvg options");
        }
      }

      ~ResvgOptionsHandle() {
        resvg_options_destroy(options_);
      }

      ResvgOptionsHandle(const ResvgOptionsHandle&) = delete;
      ResvgOptionsHandle& operator=(const ResvgOptionsHandle&) = delete;

      resvg_options* Get() const {
        return options_;
      }

    private:
      resvg_options* options_ {nullptr};
    };

    class ResvgTreeHandle {
    public:
      explicit ResvgTreeHandle(resvg_render_tree* tree)
        : tree_(tree) {
      }

      ~ResvgTreeHandle() {
        resvg_tree_destroy(tree_);
      }

      ResvgTreeHandle(const ResvgTreeHandle&) = delete;
      ResvgTreeHandle& operator=(const ResvgTreeHandle&) = delete;

      resvg_render_tree* Get() const {
        return tree_;
      }

    private:
      resvg_render_tree* tree_ {nullptr};
    };

    void InitResvgLogOnce() {
      static std::once_flag kInitFlag;
      std::call_once(kInitFlag, [] {
        resvg_init_log();
      });
    }

    std::string ResvgErrorMessage(int32_t error) {
      switch (error) {
        case RESVG_OK:
          return "ok";
        case RESVG_ERROR_NOT_AN_UTF8_STR:
          return "SVG data is not valid UTF-8";
        case RESVG_ERROR_FILE_OPEN_FAILED:
          return "resvg failed to open a referenced file";
        case RESVG_ERROR_MALFORMED_GZIP:
          return "SVG gzip data is malformed";
        case RESVG_ERROR_ELEMENTS_LIMIT_REACHED:
          return "SVG element limit was reached";
        case RESVG_ERROR_INVALID_SIZE:
          return "SVG has an invalid size";
        case RESVG_ERROR_PARSING_FAILED:
          return "resvg failed to parse SVG data";
        default:
          return "resvg returned an unknown error";
      }
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
      return -1;
    }

    std::uint8_t HexByte(std::string_view value, std::size_t offset) {
      const int high = HexValue(value[offset]);
      const int low = HexValue(value[offset + 1]);
      if (high < 0 || low < 0) {
        throw std::runtime_error("PNG background must use #RRGGBB");
      }
      return static_cast<std::uint8_t>(high * 16 + low);
    }

    Rgba ParseBackground(std::string_view background) {
      if (background.empty()) {
        return {};
      }
      if (background.size() != 7 || background.front() != '#') {
        throw std::runtime_error("PNG background must use #RRGGBB");
      }
      return {HexByte(background, 1), HexByte(background, 3), HexByte(background, 5), 255};
    }

    std::uint32_t ScaledDimension(float value, double scale) {
      if (value <= 0.0f || scale <= 0.0) {
        throw std::runtime_error("PNG scale and SVG size must be positive");
      }
      const double scaled = std::ceil(static_cast<double>(value) * scale);
      if (!std::isfinite(scaled) || scaled <= 0.0 || scaled > kMaxImageDimension) {
        throw std::runtime_error("PNG output dimensions are out of range");
      }
      return static_cast<std::uint32_t>(scaled);
    }

    void ValidateImageSize(std::uint32_t width, std::uint32_t height) {
      const std::uint64_t pixels = static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
      if (pixels == 0 || pixels > kMaxPixelCount) {
        throw std::runtime_error("PNG output dimensions are too large");
      }
    }

    void FillPixmap(std::vector<std::uint8_t>& pixmap, const Rgba& color) {
      if (color.alpha == 0) {
        return;
      }
      for (std::size_t index = 0; index < pixmap.size(); index += 4) {
        pixmap[index] = color.red;
        pixmap[index + 1] = color.green;
        pixmap[index + 2] = color.blue;
        pixmap[index + 3] = color.alpha;
      }
    }

    std::uint8_t UnpremultiplyChannel(std::uint8_t value, std::uint8_t alpha) {
      if (alpha == 0 || alpha == 255) {
        return value;
      }
      const int restored = static_cast<int>(std::lround(static_cast<double>(value) * 255.0 / alpha));
      return static_cast<std::uint8_t>(std::clamp(restored, 0, 255));
    }

    void UnpremultiplyPixmap(std::vector<std::uint8_t>& pixmap) {
      for (std::size_t index = 0; index < pixmap.size(); index += 4) {
        const std::uint8_t alpha = pixmap[index + 3];
        pixmap[index] = UnpremultiplyChannel(pixmap[index], alpha);
        pixmap[index + 1] = UnpremultiplyChannel(pixmap[index + 1], alpha);
        pixmap[index + 2] = UnpremultiplyChannel(pixmap[index + 2], alpha);
      }
    }

    std::vector<std::uint8_t> EncodePng(const std::vector<std::uint8_t>& rgba,
                                        std::uint32_t width,
                                        std::uint32_t height) {
      std::vector<unsigned char> encoded;
      const unsigned error = lodepng::encode(encoded,
                                             reinterpret_cast<const unsigned char*>(rgba.data()),
                                             width,
                                             height,
                                             LCT_RGBA,
                                             8);
      if (error != 0) {
        throw std::runtime_error(std::string("Unable to encode PNG: ") + lodepng_error_text(error));
      }
      return {encoded.begin(), encoded.end()};
    }
  }

  PngResult ResvgRasterizer::Rasterize(std::string_view svg, const PngOptions& options) {
    InitResvgLogOnce();

    ResvgOptionsHandle resvg_options;
    resvg_options_load_system_fonts(resvg_options.Get());

    resvg_render_tree* raw_tree = nullptr;
    const int32_t parse_result =
      resvg_parse_tree_from_data(svg.data(), svg.size(), resvg_options.Get(), &raw_tree);
    if (parse_result != RESVG_OK) {
      throw std::runtime_error(ResvgErrorMessage(parse_result));
    }

    ResvgTreeHandle tree(raw_tree);
    const resvg_size image_size = resvg_get_image_size(tree.Get());
    const std::uint32_t width = ScaledDimension(image_size.width, options.scale);
    const std::uint32_t height = ScaledDimension(image_size.height, options.scale);
    ValidateImageSize(width, height);

    std::vector<std::uint8_t> pixmap(static_cast<std::size_t>(width) * height * 4);
    FillPixmap(pixmap, ParseBackground(options.background));

    resvg_transform transform = resvg_transform_identity();
    transform.a = static_cast<float>(options.scale);
    transform.d = static_cast<float>(options.scale);
    resvg_render(tree.Get(), transform, width, height, reinterpret_cast<char*>(pixmap.data()));
    UnpremultiplyPixmap(pixmap);

    return {EncodePng(pixmap, width, height), width, height};
  }
}
