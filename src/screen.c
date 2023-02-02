#include <SDL2/SDL.h>
#include <blackhv/framebuffer.h>
#include <blackhv/memory.h>
#include <blackhv/types.h>
#include <stdio.h>
#include <stdlib.h>

struct screen
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    u64 framebuffer_phys;
};

static void screen_update(vm_t *vm)
{
    if (vm == NULL || vm->screen == NULL)
        return;

    void *pixels;
    int pitch;

    if (SDL_LockTexture(vm->screen->texture, NULL, &pixels, &pitch) < 0)
    {
        printf("Couldn't lock texture: %s\n", SDL_GetError());
        return;
    }

    // Read guest framebuffer into SDL texture pixels.
    memory_read(vm,
                vm->screen->framebuffer_phys,
                pixels,
                FB_WIDTH * FB_HEIGHT * FB_BPP);
    SDL_UnlockTexture(vm->screen->texture);
}

s64 screen_init(vm_t *vm, u64 framebuffer_phys)
{
    if (memory_alloc(vm,
                     framebuffer_phys,
                     align_up(FB_WIDTH * FB_HEIGHT * FB_BPP),
                     MEMORY_FRAMEBUFFER)
        == 0)
    {
        fprintf(stderr, "Failed to allocate framebuffer memory.\n");
        return 0;
    }

    SDL_Init(SDL_INIT_VIDEO);

    screen_t *screen = malloc(sizeof(screen_t));

    screen->window = SDL_CreateWindow("blackhv",
                                      SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED,
                                      FB_WIDTH,
                                      FB_HEIGHT,
                                      0);
    if (screen->window == NULL)
    {
        printf("SDL ERROR: Creating window\n");
        free(screen);
        return 0;
    }

    SDL_Delay(100);

    screen->renderer = SDL_CreateRenderer(screen->window, -1, 0);
    if (screen->renderer == NULL)
    {
        printf("SDL ERROR: creating renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(screen->window);
        free(screen);
        return 0;
    }

    screen->texture = SDL_CreateTexture(screen->renderer,
                                        SDL_PIXELFORMAT_ARGB8888,
                                        SDL_TEXTUREACCESS_STREAMING,
                                        FB_WIDTH,
                                        FB_HEIGHT);
    if (screen->texture == NULL)
    {
        printf("SDL ERROR: Could not create texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(screen->renderer);
        SDL_DestroyWindow(screen->window);
        free(screen);
        return 0;
    }

    screen->framebuffer_phys = framebuffer_phys;
    vm->screen = screen;
    return 1;
}

void *screen_run(void *params)
{
    vm_t *vm = (vm_t *)params;
    if (vm == NULL || vm->screen == NULL)
    {
        return NULL;
    }

    SDL_Event e;
    while (1)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                return NULL;
            }
        }

        // Calculates to 60 fps
        SDL_Delay(1000 / 60);

        screen_update(vm);

        SDL_RenderClear(vm->screen->renderer);
        SDL_RenderCopy(vm->screen->renderer, vm->screen->texture, NULL, NULL);
        SDL_RenderPresent(vm->screen->renderer);
    }

    return NULL;
}

void screen_uninit(vm_t *vm)
{
    if (vm == NULL || vm->screen == NULL)
    {
        return;
    }

    SDL_DestroyTexture(vm->screen->texture);
    SDL_DestroyRenderer(vm->screen->renderer);
    SDL_DestroyWindow(vm->screen->window);
    SDL_Quit();
}
