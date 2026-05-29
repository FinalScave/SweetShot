#include "sweetshot/rasterizer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "lodepng.h"
#include "lunasvg.h"

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

    std::uint32_t LunaBackground(const Rgba& color) {
      return (static_cast<std::uint32_t>(color.red) << 24) |
             (static_cast<std::uint32_t>(color.green) << 16) |
             (static_cast<std::uint32_t>(color.blue) << 8) |
             static_cast<std::uint32_t>(color.alpha);
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

    std::vector<std::uint8_t> CopyBitmapRgba(const lunasvg::Bitmap& bitmap,
                                             std::uint32_t width,
                                             std::uint32_t height) {
      const int bitmap_width = bitmap.width();
      const int bitmap_height = bitmap.height();
      if (bitmap_width < 0 || bitmap_height < 0 ||
          static_cast<std::uint32_t>(bitmap_width) != width ||
          static_cast<std::uint32_t>(bitmap_height) != height) {
        throw std::runtime_error("LunaSVG returned an unexpected bitmap size");
      }

      const std::size_t row_size = static_cast<std::size_t>(width) * 4;
      const int stride = bitmap.stride();
      if (stride < static_cast<int>(row_size)) {
        throw std::runtime_error("LunaSVG returned an invalid bitmap stride");
      }

      const std::uint8_t* source = bitmap.data();
      if (source == nullptr) {
        throw std::runtime_error("LunaSVG returned empty bitmap data");
      }

      std::vector<std::uint8_t> rgba(row_size * static_cast<std::size_t>(height));
      for (std::uint32_t row = 0; row < height; ++row) {
        const std::uint8_t* row_source = source + static_cast<std::size_t>(row) * static_cast<std::size_t>(stride);
        std::uint8_t* row_target = rgba.data() + static_cast<std::size_t>(row) * row_size;
        std::copy(row_source, row_source + row_size, row_target);
      }
      return rgba;
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

  PngResult LunaSvgRasterizer::Rasterize(std::string_view svg, const PngOptions& options) {
    std::unique_ptr<lunasvg::Document> document = lunasvg::Document::loadFromData(svg.data(), svg.size());
    if (!document) {
      throw std::runtime_error("LunaSVG failed to parse SVG data");
    }

    const std::uint32_t width = ScaledDimension(document->width(), options.scale);
    const std::uint32_t height = ScaledDimension(document->height(), options.scale);
    ValidateImageSize(width, height);

    lunasvg::Bitmap bitmap = document->renderToBitmap(static_cast<int>(width),
                                                      static_cast<int>(height),
                                                      LunaBackground(ParseBackground(options.background)));
    if (bitmap.isNull()) {
      throw std::runtime_error("LunaSVG failed to render bitmap");
    }
    bitmap.convertToRGBA();

    return {EncodePng(CopyBitmapRgba(bitmap, width, height), width, height), width, height};
  }

  std::shared_ptr<SvgRasterizer> CreateDefaultSvgRasterizer() {
    return std::make_shared<LunaSvgRasterizer>();
  }
}
