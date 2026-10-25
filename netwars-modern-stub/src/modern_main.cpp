#include <SDL2/SDL.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>

namespace {
constexpr int kLogicalWidth = 3840;
constexpr int kLogicalHeight = 2160;
constexpr int kStarCount = 1800;
constexpr float kShipRadius = 26.0f;

struct Star {
    float x;
    float y;
    float z;
    float speed;
};

struct Point {
    float x;
    float y;
};

std::vector<Star> createStarfield() {
    std::vector<Star> stars;
    stars.reserve(kStarCount);
    for (int i = 0; i < kStarCount; ++i) {
        Star s{};
        s.x = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        s.y = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        s.z = 0.15f + (static_cast<float>(rand()) / RAND_MAX) * 1.0f;
        s.speed = 0.7f + (static_cast<float>(rand()) / RAND_MAX) * 2.2f;
        stars.push_back(s);
    }
    return stars;
}

void drawShip(SDL_Renderer* renderer, int cx, int cy, float angle, Uint8 color) {
    static const Point points[] = {
        {0.0f, -28.0f},
        {18.0f, 18.0f},
        {7.0f, 12.0f},
        {-7.0f, 12.0f},
        {-18.0f, 18.0f},
    };

    SDL_SetRenderDrawColor(renderer, color, color, 255, 255);
    Point prev = points[0];
    for (size_t i = 1; i < sizeof(points) / sizeof(points[0]); ++i) {
        Point p = points[i];
        float ca = std::cos(angle);
        float sa = std::sin(angle);
        float x1 = prev.x * ca - prev.y * sa;
        float y1 = prev.x * sa + prev.y * ca;
        float x2 = p.x * ca - p.y * sa;
        float y2 = p.x * sa + p.y * ca;

        SDL_RenderDrawLine(renderer,
                           static_cast<int>(cx + x1),
                           static_cast<int>(cy + y1),
                           static_cast<int>(cx + x2),
                           static_cast<int>(cy + y2));
        prev = p;
    }

    // close the polygon
    Point p = points[0];
    float ca = std::cos(angle);
    float sa = std::sin(angle);
    float x1 = prev.x * ca - prev.y * sa;
    float y1 = prev.x * sa + prev.y * ca;
    float x2 = p.x * ca - p.y * sa;
    float y2 = p.x * sa + p.y * ca;
    SDL_RenderDrawLine(renderer,
                       static_cast<int>(cx + x1),
                       static_cast<int>(cy + y1),
                       static_cast<int>(cx + x2),
                       static_cast<int>(cy + y2));
}

void renderStarfield(SDL_Renderer* renderer, std::vector<Star>& stars) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (auto& star : stars) {
        star.z -= star.speed * 0.01f;
        if (star.z <= 0.12f) {
            star.z = 1.0f + static_cast<float>(rand()) / RAND_MAX * 3.0f;
            star.x = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
            star.y = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        }

        float px = star.x / star.z;
        float py = star.y / star.z;
        int sx = static_cast<int>((px * 0.5f + 0.5f) * kLogicalWidth);
        int sy = static_cast<int>((py * 0.5f + 0.5f) * kLogicalHeight);

        if (sx >= 0 && sx < kLogicalWidth && sy >= 0 && sy < kLogicalHeight) {
            Uint8 brightness = static_cast<Uint8>(std::clamp(120.0f + (1.0f / star.z) * 180.0f, 0.0f, 255.0f));
            SDL_SetRenderDrawColor(renderer, brightness, brightness, 255, 255);
            SDL_RenderDrawPoint(renderer, sx, sy);
        }
    }

    const int centerX = kLogicalWidth / 2;
    const int centerY = kLogicalHeight / 2;
    drawShip(renderer, centerX, centerY, 0.0f, 220);
}

void renderHUD(SDL_Renderer* renderer, const char* statusText) {
    SDL_SetRenderDrawColor(renderer, 30, 40, 70, 180);
    SDL_Rect panel = {20, 20, 760, 120};
    SDL_RenderFillRect(renderer, &panel);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Surface* titleSurface = SDL_LoadBMP("/usr/share/pixmaps/gnome-software.png");
    if (titleSurface) {
        SDL_Texture* titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
        if (titleTexture) {
            SDL_Rect dst = {30, 30, 80, 80};
            SDL_RenderCopy(renderer, titleTexture, nullptr, &dst);
            SDL_DestroyTexture(titleTexture);
        }
        SDL_FreeSurface(titleSurface);
    }

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect status = {120, 48, 620, 48};
    SDL_RenderFillRect(renderer, &status);
    SDL_SetRenderDrawColor(renderer, 20, 20, 30, 255);
    SDL_RenderFillRect(renderer, &status);

    SDL_Color textColor{255, 255, 255, 255};
    SDL_Surface* textSurface = SDL_CreateRGBSurfaceWithFormat(0, 620, 48, 32, SDL_PIXELFORMAT_RGBA32);
    if (textSurface) {
        SDL_FillRect(textSurface, nullptr, SDL_MapRGBA(textSurface->format, 20, 20, 30, 255));
        SDL_FreeSurface(textSurface);
    }

    SDL_RenderDrawLine(renderer, 20, 160, kLogicalWidth - 20, 160);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

}  // namespace

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "Unable to initialize SDL: " << SDL_GetError() << '\n';
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "NETWARS Modernized",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1920, 1080,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        std::cerr << "Unable to create SDL window: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "Unable to create SDL renderer: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, kLogicalWidth, kLogicalHeight);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    std::vector<Star> stars = createStarfield();
    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
            }
        }

        renderStarfield(renderer, stars);
        renderHUD(renderer, "NETWARS // Debian Linux Trixie // 4K Ready");
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
