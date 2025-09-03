#include "../plugin/p_plugins.c"
#include <lv2/core/lv2.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DRIVE_URI "http://lv2plug.in/plugins/eg-amp"

typedef enum { AMP_INPUT = 3, AMP_OUTPUT = 4 } PortIndex;

typedef struct {
  // Port buffers
  const float *input;
  float *output;

  p_plugin *plugin;
} Amp;

static LV2_Handle instantiate(const LV2_Descriptor *descriptor, double rate,
                              const char *bundle_path,
                              const LV2_Feature *const *features) {
  Amp *amp = (Amp *)calloc(1, sizeof(Amp));
  // TODO: This is just hard-coded to the drive plugin
  amp->plugin = &p_plugins[1];
  return (LV2_Handle)amp;
}

// I don't know what this does
static void connect_port(LV2_Handle instance, uint32_t port, void *data) {
  Amp *amp = (Amp *)instance;

  switch (port) {
  case 3:
    amp->input = (const float *)data;
    break;
  case 4:
    amp->output = (float *)data;
    break;
  default:
    amp->plugin->parameterValues[port] = (float *)data;
    break;
  }
}

static void activate(LV2_Handle instance) {}

static void run(LV2_Handle instance, uint32_t n_samples) {
  const Amp *amp = (const Amp *)instance;

  const float *const input = amp->input;
  float *const output = amp->output;

  for (uint32_t pos = 0; pos < n_samples; pos++) {
    // p_audio output =
    //     amp->plugin->processAudio(amp->plugin, input[pos], input[pos]);
    output[pos] = input[pos];
  }
}

static void deactivate(LV2_Handle instance) {}

static void cleanup(LV2_Handle instance) { free(instance); }

static const void *extension_data(const char *uri) { return NULL; }

static const LV2_Descriptor descriptor = {.URI = DRIVE_URI,
                                          .instantiate = instantiate,
                                          .connect_port = connect_port,
                                          .activate = activate,
                                          .run = run,
                                          .deactivate = deactivate,
                                          .cleanup = cleanup,
                                          .extension_data = extension_data};

LV2_SYMBOL_EXPORT const LV2_Descriptor *lv2_descriptor(uint32_t index) {
  if (index == 0) {
    return &descriptor;
  }
  return NULL;
}
