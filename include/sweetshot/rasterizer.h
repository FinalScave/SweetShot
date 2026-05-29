#ifndef SWEETSHOT_RASTERIZER_H
#define SWEETSHOT_RASTERIZER_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sweetshot {
  struct PngOptions {
    double scale {1.0};
    std::string background;
  };

  struct PngResult {
    std::vector<std::uint8_t> bytes;
    std::uint32_t width {0};
    std::uint32_t height {0};
  };

  class SvgRasterizer {
  public:
    virtual ~SvgRasterizer() = default;

    virtual PngResult Rasterize(std::string_view svg, const PngOptions& options) = 0;
  };
}

#endif
