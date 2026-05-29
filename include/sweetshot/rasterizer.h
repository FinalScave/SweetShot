#ifndef SWEETSHOT_RASTERIZER_H
#define SWEETSHOT_RASTERIZER_H

#include <cstdint>
#include <memory>
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

#if defined(SWEETSHOT_PNG_BACKEND_RESVG)
  class ResvgRasterizer final : public SvgRasterizer {
  public:
    PngResult Rasterize(std::string_view svg, const PngOptions& options) override;
  };
#endif

#if defined(SWEETSHOT_PNG_BACKEND_LUNASVG)
  class LunaSvgRasterizer final : public SvgRasterizer {
  public:
    PngResult Rasterize(std::string_view svg, const PngOptions& options) override;
  };
#endif

  std::shared_ptr<SvgRasterizer> CreateDefaultSvgRasterizer();
}

#endif
