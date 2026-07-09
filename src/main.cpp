#define SDL_MAIN_USE_CALLBACKS
#define SDL_IMPL
#include <SDL3/SDL_main.h>
#include <sdl.hpp>
#include <renderer.hpp>

class Engine : public App {
public:
    explicit Engine(std::span<char *> args) {
        chk(SDL_SetAppMetadata("Codotaku Vibe Engine", "0.0.1", "com.codotaku.engine"));
        chk(SDL_Init(SDL_INIT_VIDEO));
        window_ = chk(SDL_CreateWindow("Codotaku Vibe Engine", 800, 600, SDL_WINDOW_RESIZABLE));
        device_ = chk(SDL_CreateGPUDevice(
            SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
            true, nullptr));
        chk(SDL_ClaimWindowForGPUDevice(device_, window_));
        chk(SDL_ShowWindow(window_));
        renderer_ = std::make_unique<Renderer>(window_, device_);
    }

    ~Engine() {
        renderer_.reset();
        if (device_) {
            SDL_ReleaseWindowFromGPUDevice(device_, window_);
            SDL_DestroyGPUDevice(device_);
        }
    }

    auto Event(const SDL_Event *event) -> SDL_AppResult override {
        switch (event->type) {
            case SDL_EVENT_QUIT:
                return SDL_APP_SUCCESS;
            default:
                renderer_->Event(*event);
                return SDL_APP_CONTINUE;
        }
    }

    auto Iterate() -> SDL_AppResult override {
        auto now = SDL_GetTicks();
        float dt = (now - last_ticks_) / 1000.0f;
        last_ticks_ = now;
        renderer_->Render(dt);
        return SDL_APP_CONTINUE;
    }

private:
    SDL_Window *window_;
    SDL_GPUDevice *device_;
    std::unique_ptr<Renderer> renderer_;
    Uint64 last_ticks_{SDL_GetTicks()};
};

std::unique_ptr<App> CreateApp(std::span<char *> args) {
    return std::make_unique<Engine>(args);
}
