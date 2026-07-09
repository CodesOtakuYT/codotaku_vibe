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
    explicit Engine(std::span<char *> args) {
        gpu_ = std::make_unique<GPUContext>("Codotaku Vibe Engine", 800, 600);
        resources_ = std::make_unique<ResourceManager>(gpu_->Device());
        uploader_ = std::make_unique<Uploader>(gpu_->Device());
        camera_ = std::make_unique<OrbitCamera>();
        scene_ = std::make_unique<Scene>();
        renderer_ = std::make_unique<Renderer>(gpu_.get(), resources_.get(), *uploader_, Renderer::MsaaMode::None);
    }

    auto Event(const SDL_Event *event) -> SDL_AppResult override {
        switch (event->type) {
            case SDL_EVENT_QUIT:
                return SDL_APP_SUCCESS;
            default:
                camera_->Event(*event);
                return SDL_APP_CONTINUE;
        }
    }

    auto Iterate() -> SDL_AppResult override {
        SDL_GPUCommandBuffer *cmdbuf;
        SDL_GPUTexture *swapchain;
        if (!gpu_->BeginFrame(cmdbuf, swapchain))
            return SDL_APP_CONTINUE;

        auto now = SDL_GetTicks();
        float dt = (now - last_ticks_) / 1000.0f;
        last_ticks_ = now;

        int w = gpu_->Width(), h = gpu_->Height();
        if (w != last_w_ || h != last_h_) {
            last_w_ = w;
            last_h_ = h;
            renderer_->Resize(w, h);
        }

        scene_->Update(dt);
        auto viewProj = camera_->ViewProjMatrix(w, h);
        renderer_->Render(cmdbuf, swapchain, viewProj, *scene_);
        gpu_->EndFrame(cmdbuf);
        return SDL_APP_CONTINUE;
    }

private:
    std::unique_ptr<GPUContext> gpu_;
    std::unique_ptr<ResourceManager> resources_;
    std::unique_ptr<Uploader> uploader_;
    std::unique_ptr<OrbitCamera> camera_;
    std::unique_ptr<Scene> scene_;
    std::unique_ptr<Renderer> renderer_;
    Uint64 last_ticks_{SDL_GetTicks()};
    int last_w_ = 800, last_h_ = 600;
};

std::unique_ptr<App> CreateApp(std::span<char *> args) {
    return std::make_unique<Engine>(args);
}
