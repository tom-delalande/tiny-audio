#include <lv2/core/lv2.h>

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define DRIVE_URI "http://lv2plug.in/plugins/eg-amp"

typedef enum { AMP_GAIN = 0, AMP_INPUT = 1, AMP_OUTPUT = 2 } PortIndex;

typedef struct {
  // Port buffers
  const float *gain;
  const float *input;
  float *output;
} Amp;

static LV2_Handle instantiate(const LV2_Descriptor *descriptor, double rate,
                              const char *bundle_path,
                              const LV2_Feature *const *features) {
  Amp *amp = (Amp *)calloc(1, sizeof(Amp));

  return (LV2_Handle)amp;
}

static void connect_port(LV2_Handle instance, uint32_t port, void *data) {
  Amp *amp = (Amp *)instance;

  switch ((PortIndex)port) {
  case AMP_GAIN:
    amp->gain = (const float *)data;
    break;
  case AMP_INPUT:
    amp->input = (const float *)data;
    break;
  case AMP_OUTPUT:
    amp->output = (float *)data;
    break;
  }
}

static void activate(LV2_Handle instance) {}

/** Define a macro for converting a gain in dB to a coefficient. */
#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)

static void run(LV2_Handle instance, uint32_t n_samples) {
  const Amp *amp = (const Amp *)instance;

  const float gain = *(amp->gain);
  const float *const input = amp->input;
  float *const output = amp->output;

  const float coef = DB_CO(gain);

  for (uint32_t pos = 0; pos < n_samples; pos++) {
    output[pos] = input[pos] * coef;
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
