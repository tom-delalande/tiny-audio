#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clap/ext/gui.h"
#include "clap/plugin.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <clap/clap.h>

// --- Globals for our UI ---
static SDL_Window *sdl_window = NULL;
static SDL_Renderer *sdl_renderer = NULL;
static bool sdl_running = false;

// --- GUI Callbacks for CLAP ---
static bool gui_is_api_supported(const clap_plugin_t *plugin, const char *api,
                                 bool is_floating) {
#if defined(_WIN32)
  return strcmp(api, CLAP_WINDOW_API_WIN32) == 0;
#elif defined(__APPLE__)
  return strcmp(api, CLAP_WINDOW_API_COCOA) == 0;
#else
  return strcmp(api, CLAP_WINDOW_API_X11) == 0 ||
         strcmp(api, CLAP_WINDOW_API_WAYLAND) == 0;
#endif
}

static bool gui_get_preferred_api(const clap_plugin_t *plugin, const char **api,
                                  bool *is_floating) {
#if defined(_WIN32)
  *api = CLAP_WINDOW_API_WIN32;
#elif defined(__APPLE__)
  *api = CLAP_WINDOW_API_COCOA;
#else
  *api = CLAP_WINDOW_API_X11;
#endif
  *is_floating = true; // true is easier to manage in SDL
  return true;
}

static bool gui_create(const clap_plugin_t *plugin, const char *api,
                       bool is_floating) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("SDL_Init failed: %s", SDL_GetError());
    return false;
  }

  sdl_window = SDL_CreateWindow("Tiny Audio Plugin", 200, 200, 0);
  if (!sdl_window)
    return false;

  sdl_renderer = SDL_CreateRenderer(sdl_window, NULL);
  if (!sdl_renderer)
    return false;

  sdl_running = true;
  return true;
}

static void gui_destroy(const clap_plugin_t *plugin) {
  if (sdl_renderer) {
    SDL_DestroyRenderer(sdl_renderer);
    sdl_renderer = NULL;
  }
  if (sdl_window) {
    SDL_DestroyWindow(sdl_window);
    sdl_window = NULL;
  }
  SDL_Quit();
  sdl_running = false;
}

static bool gui_set_parent(const clap_plugin_t *plugin,
                           const clap_window_t *window) {
  // For floating windows, you don't use parent.
  // If you wanted to embed, you'd need SDL3 native handle API (future support).
  return true;
}

static void gui_paint_frame() {
  SDL_SetRenderDrawColor(sdl_renderer, 30, 30, 30, 255);
  SDL_RenderClear(sdl_renderer);

  static float x = 0;
  if (++x > 400) {
    x = 0;
  }
  SDL_SetRenderDrawColor(sdl_renderer, 200, 50, 50, 255);
  SDL_RenderLine(sdl_renderer, x, 0, 400, 300);

  SDL_RenderPresent(sdl_renderer);
}

static bool gui_show(const clap_plugin_t *plugin) {
  // Simple pump: in a real plugin, run this in
  // host's timer or on demand
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_EVENT_QUIT) {
      sdl_running = false;
    }
  }
  if (sdl_running)
    gui_paint_frame();
  else
    // TODO: Make it close the plugin host window as well
    gui_destroy(plugin);
  return true;
}

static bool gui_hide(const clap_plugin_t *plugin) {
  // Could hide the SDL window instead of destroying
  SDL_HideWindow(sdl_window);
  return true;
}

// --- GUI extension struct ---
static const clap_plugin_gui_t gui_ext = {
    .is_api_supported = gui_is_api_supported,
    .get_preferred_api = gui_get_preferred_api,
    .create = gui_create,
    .destroy = gui_destroy,
    .set_scale = NULL,
    .get_size = NULL,
    .can_resize = NULL,
    .get_resize_hints = NULL,
    .adjust_size = NULL,
    .set_size = NULL,
    .set_parent = gui_set_parent,
    .set_transient = NULL,
    .suggest_title = NULL,
    .show = gui_show,
    .hide = gui_hide,
};
