#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  float left;
  float right;
} p_audio;

enum p_parameter_type {
  PARAMETER_TYPE__DOUBLE = 0,
  PARAMETER_TYPE__BOOLEAN = 1,
  PARAMETER_TYPE__ENUM = 2,
};

typedef struct {
  int id;
  char *name;
  float defaultValue;
  float minValue;
  float maxValue;
  enum p_parameter_type type;
  const char enumTypeValues[16][16];
} p_parameter;

typedef struct p_plugin p_plugin;
struct p_plugin {
  char *id;
  p_audio (*processAudio)(p_plugin *plugin, float leftIn, float rightIn);
  void (*handleMidiNoteOn)(p_plugin *plugin, int16_t key);
  void (*handleMidiNoteOff)(p_plugin *plugin, int16_t key);
  p_parameter *parameters;
  float **parameterValues;
  uint32_t parameterCount;
};

static int16_t MIDI_NOTE = 0;

p_audio synth_processAudio(p_plugin *plugin, float leftIn, float rightIn) {
  double frequency = 440.0 * pow(2.0, (MIDI_NOTE - 69) / 12.0);
  bool noteOn = MIDI_NOTE != 0;
  static double phase = 0.0;
  double sampleRate = 44100.0;

  float sample = 0.0f;
  if (noteOn) {
    sample = (float)((2.0 * (phase)-1.0)); // saw wave [-1,1]
    sample = sinf(2.0f * M_PI * phase);    // sine
    phase += frequency / sampleRate;
    if (phase >= 1.0)
      phase -= 1.0;
  }
  return (p_audio){.left = sample, .right = sample};
}

void synth_handleMidiNoteOn(p_plugin *plugin, int16_t key) { MIDI_NOTE = key; }
void synth_handleMidiNoteOff(p_plugin *plugin, int16_t key) { MIDI_NOTE = 0; }

p_audio drive_processAudio(p_plugin *plugin, float leftIn, float rightIn) {
  float drive = *(plugin->parameterValues[0]);
  int mode = (int)*(plugin->parameterValues[1]);
  float mix = *(plugin->parameterValues[2]);

  float out_l, out_r;
  out_l = 0;
  out_r = 0;

  float tl = leftIn * (1.0 + drive);
  float tr = rightIn * (1.0 + drive);

  // Obviously this is inefficient but
  switch (mode) {
  case 0: {
    tl = (tl > 1 ? 1 : tl < -1 ? -1 : tl);
    tr = (tr > 1 ? 1 : tr < -1 ? -1 : tr);
  } break;
  case 1: {
    tl = (tl > 1 ? 1 : tl < -1 ? -1 : tl);
    tl = 1.5 * tl - 0.5 * tl * tl * tl;

    tr = (tr > 1 ? 1 : tr < -1 ? -1 : tr);
    tr = 1.5 * tr - 0.5 * tr * tr * tr;
  } break;
  case 2: {
    tl = sin(2.0 * M_PI * tl);
    tr = sin(2.0 * M_PI * tr);
  } break;
  }

  out_l = mix * tl + (1.0 - mix) * leftIn;
  out_r = mix * tr + (1.0 - mix) * rightIn;
  return (p_audio){
      out_l,
      out_r,
  };
}

p_parameter drive_parameters[] = {
    {
        0,
        "Drive",
        0.,
        -1,
        6,
        PARAMETER_TYPE__DOUBLE,
    },
    {
        1,
        "Mix",
        0.5,
        0,
        1,
        PARAMETER_TYPE__DOUBLE,
    },
    {
        2,
        "Mode",
        0.,
        0.,
        2,
        PARAMETER_TYPE__ENUM,
        {"HARD", "SOFT", "FOLD"},
    },
};

p_plugin p_plugins[2] = {{
                             .id = "io.tinyclub.tiny-synth",
                             .processAudio = synth_processAudio,
                             .handleMidiNoteOn = synth_handleMidiNoteOn,
                             .handleMidiNoteOff = synth_handleMidiNoteOff,
                             .parameters = {},
                             .parameterCount = 0,
                         },
                         {
                             .id = "io.tinyclub.tiny-drive",
                             .processAudio = drive_processAudio,
                             .parameters = drive_parameters,
                             .parameterCount = 3,
                         }};

char *P_GetParameterCurrentValueAsText(p_parameter *parameter, double value) {
  char *str = malloc(16);
  switch (parameter->type) {
  case PARAMETER_TYPE__DOUBLE:
    sprintf(str, "%f", value);
    break;
  case PARAMETER_TYPE__ENUM:
    sprintf(str, "%s", parameter->enumTypeValues[(int)value]);
    break;
  case PARAMETER_TYPE__BOOLEAN:
    if (value > 0) {
      sprintf(str, "%s", "true");
    } else {
      sprintf(str, "%s", "false");
    }
    break;
  };
  return str;
}
