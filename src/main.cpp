#define SDL_MAIN_USE_CALLBACKS
#define SDL_IMPL
#include <SDL3/SDL_main.h>
#include <sdl.hpp>
#include <gpu_context.hpp>
#include <resource_manager.hpp>
#include <uploader.hpp>
#include <camera.hpp>
#include <scene.hpp>
#include <renderer.hpp>

class Engine : public App {
public:
    explicit Engine(std::span<char *> args)
        : gpu_("Codotaku Vibe Engine", glm::ivec2(800, 600))
        , resources_(gpu_.Device())
        , uploader_(gpu_.Device())
        , renderer_(&gpu_, &resources_, uploader_, scene_, Renderer::MsaaMode::None)
    {}

    auto Event(const SDL_Event *event) -> SDL_AppResult override {
        switch (event->type) {
            case SDL_EVENT_QUIT:
                return SDL_APP_SUCCESS;
            default:
                camera_.Event(*event);
                return SDL_APP_CONTINUE;
        }
    }

    auto Iterate() -> SDL_AppResult override {
        SDL_GPUCommandBuffer *cmdbuf;
        SDL_GPUTexture *swapchain;
        if (!gpu_.BeginFrame(cmdbuf, swapchain))
            return SDL_APP_CONTINUE;

        auto now = SDL_GetTicks();
        float dt = (now - last_ticks_) / 1000.0f;
        last_ticks_ = now;

        auto size = gpu_.Size();
        if (size != last_size_) {
            last_size_ = size;
            renderer_.Resize(size);
        }

        scene_.Update(dt);
        auto viewProj = camera_.ViewProjMatrix(size);
        renderer_.Render(cmdbuf, swapchain, viewProj, scene_);
        gpu_.EndFrame(cmdbuf);
        return SDL_APP_CONTINUE;
    }

private:
    GPUContext gpu_;
    ResourceManager resources_;
    Uploader uploader_;
    OrbitCamera camera_;
    Scene scene_;
    Renderer renderer_;
    Uint64 last_ticks_{SDL_GetTicks()};
    glm::ivec2 last_size_{800, 600};
};

std::unique_ptr<App> CreateApp(std::span<char *> args) {
    return std::make_unique<Engine>(args);
}
