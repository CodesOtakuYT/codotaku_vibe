#define SDL_MAIN_USE_CALLBACKS
#define SDL_IMPL
#include <SDL3/SDL_main.h>
#include <sdl.hpp>
#include <gpu_context.hpp>
#include <resource_manager.hpp>
#include <uploader.hpp>
#include <camera.hpp>
#include <scene.hpp>
#include <geometry.hpp>
#include <renderer.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace {

constexpr int kPerRing = 5;
constexpr int kRings = 8;
constexpr int kInstanceCount = kPerRing * kRings;

auto MakeTestScene() -> Scene {
    Scene scene;
    auto cubeIdx = scene.AddGeometry(CreateCubeGeometry());
    auto pyrIdx = scene.AddGeometry(CreatePyramidGeometry());
    for (int r = 0; r < kRings; ++r) {
        auto geo = r % 2 == 0 ? cubeIdx : pyrIdx;
        for (int p = 0; p < kPerRing; ++p) {
            scene.AddInstance(geo, p % 2);
        }
    }
    return scene;
}

} // namespace

class Engine : public App {
public:
    explicit Engine(std::span<char *> args)
        : gpu_("Codotaku Vibe Engine", glm::ivec2(800, 600))
        , resources_(gpu_.Device(), uploader_)
        , uploader_(gpu_.Device())
        , scene_(MakeTestScene())
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
        time_ += dt;

        auto size = gpu_.Size();
        if (size != last_size_) {
            last_size_ = size;
            renderer_.Resize(size);
        }

        for (int r = 0; r < kRings; ++r) {
            float radius = 25.0f + r * 25.0f;
            float ringOff = r * 0.3f;
            for (int p = 0; p < kPerRing; ++p) {
                int idx = r * kPerRing + p;
                auto &inst = scene_.GetInstance(idx);
                float a = time_ * (0.2f + r * 0.04f) + p * 6.2832f / kPerRing + ringOff;
                float h = (r - kRings / 2) * 12.0f + 4.0f * sinf(time_ * 0.4f + r);
                glm::vec3 pos{radius * cosf(a), h, radius * sinf(a)};
                inst.transform = glm::translate(glm::mat4{1}, pos) *
                                 glm::rotate(glm::mat4{1}, time_ * (0.3f + r * 0.05f + p * 0.1f),
                                             glm::normalize(glm::vec3{p + 1, r + 1, p + r + 1}));
            }
        }

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
    float time_ = 0;
};

std::unique_ptr<App> CreateApp(std::span<char *> args) {
    return std::make_unique<Engine>(args);
}
