#include "../plugin/p_plugins.h"
#include "lv2/core/lv2.h"
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PASSTHROUGH_URI "http://example.org/passthrough"

const int8_t num_plugins = 1;
const char *i_plugins[] = {PASSTHROUGH_URI};

const char *plugin_lib = "./plugins.dylib";
void *plugin_lib_handle;
const char *plugin_bundle_path;
char *path;

typedef struct {
  const char *uri;
  const float *in_l;
  const float *in_r;
  float *out_l;
  float *out_r;

  float *parameters[16];
  p_plugin *plugin;
} Passthrough;

/* Instantiate the plugin */
static LV2_Handle instantiate(const LV2_Descriptor *descriptor, double rate,
                              const char *bundle_path,
                              const LV2_Feature *const *features) {

  plugin_bundle_path = bundle_path;
  Passthrough *passthrough = calloc(1, sizeof(Passthrough));
  passthrough->uri = descriptor->URI;
  return (LV2_Handle)passthrough;
}

static void activate(LV2_Handle instance) {
  Passthrough *self = (Passthrough *)instance;
  char path[1024];
  snprintf(path, sizeof(path), "%s/%s", plugin_bundle_path, plugin_lib);
  plugin_lib_handle = dlopen(path, RTLD_NOW);

  p_plugin *plugins = (p_plugin *)dlsym(plugin_lib_handle, "p_plugins");
  for (int i = 0; i < num_plugins; ++i) {
    if (strcmp(i_plugins[i], self->uri) == 0) {
      self->plugin = &plugins[i];
    }
  }
}

/* Connect ports */
static void connect_port(LV2_Handle instance, uint32_t port, void *data) {
  Passthrough *self = (Passthrough *)instance;
  switch (port) {
  case 0:
    self->in_l = (const float *)data;
    break;
  case 1:
    self->in_r = (const float *)data;
    break;
  case 2:
    self->out_l = (float *)data;
    break;
  case 3:
    self->out_r = (float *)data;
    break;
  default:
    self->parameters[port - 4] = (float *)data;
    break;
  }
}

/* Run processing */
static void run(LV2_Handle instance, uint32_t n_samples) {
  Passthrough *self = (Passthrough *)instance;

  if (!self->in_l || !self->in_r || !self->out_l || !self->out_r)
    return;

  for (uint32_t i = 0; i < n_samples; ++i) {
    p_audio output = self->plugin->processAudio(
        self->plugin, self->in_l[i], self->in_r[i], self->parameters);
    self->out_l[i] = output.left;
    self->out_r[i] = output.right;
  }
}
static void deactivate(LV2_Handle instance) {
  if (plugin_lib_handle)
    dlclose(plugin_lib_handle);
}
/* Cleanup */
static void cleanup(LV2_Handle instance) { free(instance); }

/* Plugin descriptor */
static const LV2_Descriptor descriptor = {
    PASSTHROUGH_URI,
    instantiate,
    connect_port,
    activate, // activate
    run,
    deactivate, // deactivate
    cleanup,
    NULL // extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index) {
  return (index == 0) ? &descriptor : NULL;
}
