#ifndef SWEETSHOT_RENDERER_H
#define SWEETSHOT_RENDERER_H

#include <memory>
#include <string>

#include "sweetshot/input.h"
#include "sweetshot/rasterizer.h"
#include "sweetshot/scene.h"

namespace sweetshot {
  struct RendererState;

  struct RendererConfig {
    std::string syntax_directory;
    std::shared_ptr<SvgRasterizer> png_rasterizer;
  };

  class Renderer {
  public:
    explicit Renderer(RendererConfig config = {});

    RenderScene RenderToScene(const RenderInput& input) const;
    std::string RenderToSvg(const RenderInput& input) const;
    std::string RenderToHtml(const RenderInput& input) const;
    PngResult RenderToPng(const RenderInput& input, const PngOptions& options = {}) const;

  private:
    RendererConfig config_;
    std::shared_ptr<RendererState> state_;
  };
}

#endif
