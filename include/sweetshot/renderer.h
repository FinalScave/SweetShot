#ifndef SWEETSHOT_RENDERER_H
#define SWEETSHOT_RENDERER_H

#include <memory>
#include <string>

#include "sweetshot/input.h"
#include "sweetshot/scene.h"

namespace sweetshot {
  struct RendererState;

  struct RendererConfig {
    std::string syntax_directory;
  };

  class Renderer {
  public:
    explicit Renderer(RendererConfig config = {});

    RenderScene renderScene(const RenderInput& input) const;
    std::string renderToSvg(const RenderInput& input) const;
    std::string renderToHtml(const RenderInput& input) const;

  private:
    RendererConfig config_;
    std::shared_ptr<RendererState> state_;
  };
}

#endif
