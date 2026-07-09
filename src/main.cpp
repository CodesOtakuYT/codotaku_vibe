#define SDL_IMPL
#include <sdl.hpp>

class Engine : public App {
public:
    explicit Engine(std::span<char *> args) {
        chk(SDL_SetAppMetadata("Codotaku Vibe Engine", "0.0.1", "com.codotaku.engine"));
        chk(SDL_Init(SDL_INIT_VIDEO));
        window_ = chk(SDL_CreateWindow("Codotaku Vibe Engine", 800, 600, SDL_WINDOW_RESIZABLE));
        chk(SDL_ShowWindow(window_));
    }

    ~Engine() {
    }

    auto Event(const SDL_Event *event) -> SDL_AppResult override {
        switch (event->type) {
            case SDL_EVENT_QUIT:
                return SDL_APP_SUCCESS;
            default:
                return SDL_APP_CONTINUE;
        }
    }

    auto Iterate() -> SDL_AppResult override {
        return SDL_APP_CONTINUE;
    }

private:
    SDL_Window *window_;
};

std::unique_ptr<App> CreateApp(std::span<char *> args) {
    return std::make_unique<Engine>(args);
}
