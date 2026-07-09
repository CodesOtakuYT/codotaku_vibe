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

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace {

auto MakeTestScene() -> Scene {
    Scene scene;
    auto cubeIdx = scene.AddGeometry(Scene::CreateCube());
    auto pyrIdx = scene.AddGeometry(Scene::CreatePyramid());
    scene.AddInstance(cubeIdx, 0);
    scene.AddInstance(cubeIdx, 1);
    scene.AddInstance(pyrIdx, 0);
    return scene;
}

} // namespace

class Engine : public App {
public:
    explicit Engine(std::span<char *> args)
        : gpu_("Codotaku Vibe Engine", glm::ivec2(800, 600))
        , resources_(gpu_.Device())
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

        auto &i0 = scene_.GetInstance(0);
        float r0 = time_ * 0.5f;
        i0.transform = glm::rotate(glm::mat4{1}, r0, {0, 1, 0});

        auto &i1 = scene_.GetInstance(1);
        glm::vec3 p1{15.0f * cosf(r0 * 0.7f), 0.0f, 15.0f * sinf(r0 * 0.7f)};
        i1.transform = glm::translate(glm::mat4{1}, p1) *
                       glm::rotate(glm::mat4{1}, time_ * 0.8f, {1, 0, 0});

        auto &i2 = scene_.GetInstance(2);
        glm::vec3 p2{-20.0f * cosf(r0 * 0.5f), 10.0f + 5.0f * sinf(r0 * 0.3f), -20.0f * sinf(r0 * 0.5f)};
        i2.transform = glm::translate(glm::mat4{1}, p2) *
                       glm::rotate(glm::mat4{1}, time_ * 0.3f, {0, 0, 1});

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
