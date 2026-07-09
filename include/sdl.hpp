#pragma once

#include <SDL3/SDL.h>

#include <exception>
#include <span>
#include <streambuf>
#include <string>
#include <string_view>
#include <iostream>
#include <stdexcept>
#include <format>
#include <source_location>
#include <memory>

class SDLLogStreambuf : public std::streambuf {
public:
    explicit SDLLogStreambuf(const SDL_LogPriority priority);

protected:
    int_type overflow(int_type c) override;

    int sync() override;

    std::streamsize xsputn(const char *ptr, std::streamsize count) override;

private:
    void flushBuffer();

    SDL_LogPriority priority_;
    std::string buffer_;
};

class SDLLogRedirector {
public:
    SDLLogRedirector();

    ~SDLLogRedirector();

    SDLLogRedirector(const SDLLogRedirector &) = delete;

    SDLLogRedirector &operator=(const SDLLogRedirector &) = delete;

private:
    SDLLogStreambuf cout_buffer_;
    SDLLogStreambuf cerr_buffer_;
    std::streambuf *old_cout_;
    std::streambuf *old_cerr_;
};

class SDLException : public std::runtime_error {
public:
    explicit SDLException(const std::source_location &loc = std::source_location::current())
        : std::runtime_error{
            std::format("{}:{}:{} ({}) : {}", loc.file_name(), loc.line(), loc.column(), loc.function_name(),
                        SDL_GetError())
        } {
    }
};

inline void chk(bool result) {
    if (!result) throw SDLException{};
}

template<typename T>
auto chk(T *result) -> T * {
    if (!result) throw SDLException{};
    return result;
}

template<typename T>
auto chk(T result) -> T {
    if (!result) throw SDLException{};
    return result;
}

class App {
public:
    App() = default;

    virtual ~App() = default;

    virtual auto Event(const SDL_Event *event) -> SDL_AppResult = 0;

    virtual auto Iterate() -> SDL_AppResult = 0;

private:
    SDLLogRedirector logger_redirector_;
};

extern std::unique_ptr<App> CreateApp(std::span<char *> args);

#ifdef SDL_IMPL

SDLLogStreambuf::SDLLogStreambuf(const SDL_LogPriority priority) : priority_{priority} {
}

auto SDLLogStreambuf::overflow(int_type c) -> int_type {
    if (c == traits_type::eof()) return traits_type::eof();
    if (c == '\n') flushBuffer();
    else buffer_ += traits_type::to_char_type(c);
    return c;
}

auto SDLLogStreambuf::sync() -> int {
    flushBuffer();
    return 0;
}

auto SDLLogStreambuf::xsputn(const char *ptr, std::streamsize count) -> std::streamsize {
    std::string_view view(ptr, count);
    if (const auto pos = view.find('\n'); pos == std::string_view::npos) {
        buffer_.append(view);
    } else {
        buffer_.append(view, 0, pos);
        flushBuffer();
        if (pos + 1 < count) xsputn(ptr + pos + 1, count - pos - 1);
    }
    return count;
}

void SDLLogStreambuf::flushBuffer() {
    if (buffer_.empty()) return;
    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, priority_, "%s", buffer_.c_str());
    buffer_.clear();
}

SDLLogRedirector::SDLLogRedirector()
    : cout_buffer_(SDL_LOG_PRIORITY_INFO),
      cerr_buffer_(SDL_LOG_PRIORITY_ERROR) {
    old_cout_ = std::cout.rdbuf(&cout_buffer_);
    old_cerr_ = std::cerr.rdbuf(&cerr_buffer_);
}

SDLLogRedirector::~SDLLogRedirector() {
    std::cout.rdbuf(old_cout_);
    std::cerr.rdbuf(old_cerr_);
}

void *operator new(std::size_t size) {
    if (size == 0) size = 1;
    void *ptr = SDL_malloc(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void *operator new[](std::size_t size) {
    if (size == 0) size = 1;
    void *ptr = SDL_malloc(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void *operator new(std::size_t size, std::align_val_t al) {
    if (size == 0) size = 1;
    void *ptr = SDL_aligned_alloc(static_cast<size_t>(al), size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void *operator new[](std::size_t size, std::align_val_t al) {
    if (size == 0) size = 1;
    void *ptr = SDL_aligned_alloc(static_cast<size_t>(al), size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
    if (size == 0) size = 1;
    return SDL_malloc(size);
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept {
    if (size == 0) size = 1;
    return SDL_malloc(size);
}

void operator delete(void *ptr) noexcept { SDL_free(ptr); }
void operator delete[](void *ptr) noexcept { SDL_free(ptr); }
void operator delete(void *ptr, std::size_t) noexcept { SDL_free(ptr); }
void operator delete[](void *ptr, std::size_t) noexcept { SDL_free(ptr); }
void operator delete(void *ptr, std::align_val_t) noexcept { SDL_aligned_free(ptr); }
void operator delete[](void *ptr, std::align_val_t) noexcept { SDL_aligned_free(ptr); }
void operator delete(void *ptr, std::size_t, std::align_val_t) noexcept { SDL_aligned_free(ptr); }
void operator delete[](void *ptr, std::size_t, std::align_val_t) noexcept { SDL_aligned_free(ptr); }

auto SDL_AppInit(void **appstate, int argc, char *argv[]) -> SDL_AppResult try {
    auto app = CreateApp(std::span{argv, static_cast<size_t>(argc)});
    *appstate = app.release();
    return SDL_APP_CONTINUE;
} catch (const std::exception &e) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Initialization failed: %s", e.what());
    return SDL_APP_FAILURE;
}

auto SDL_AppEvent(void *appstate, SDL_Event *event) -> SDL_AppResult try {
    auto *app = static_cast<App *>(appstate);
    return app->Event(event);
} catch (const std::exception &e) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Event error: %s", e.what());
    return SDL_APP_FAILURE;
}

auto SDL_AppIterate(void *appstate) -> SDL_AppResult try {
    auto *app = static_cast<App *>(appstate);
    return app->Iterate();
} catch (const std::exception &e) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Iterate error: %s", e.what());
    return SDL_APP_FAILURE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    delete static_cast<App *>(appstate);
}

#endif
