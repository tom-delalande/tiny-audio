#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char P_PLUGIN_ID[] = "io.tinyclub.tiny-plugin";
const char P_PLUGIN_NAME[] = "Tiny Plugin";
const char P_PLUGIN_VENDOR[] = "tinyclub";
const char P_PLUGIN_URL[] = "tinclub.io";
const char P_PLUGIN_VERSION[] = "1.0.0";
const char P_PLUGIN_DESCRIPTION[] = "None";

enum P_ClipType { HARD = 0, SOFT = 1, FOLD = 2 };

enum P_parameter_type {
  PARAMETER_TYPE__DOUBLE = 0,
  PARAMETER_TYPE__BOOLEAN = 1,
  PARAMETER_TYPE__ENUM = 2,
};

static int16_t MIDI_NOTE = 0;

typedef struct {
  int id;
  char *name;
  double defaultValue;
  double minValue;
  double maxValue;
  double currentValue;
  enum P_parameter_type type;
  const char enumTypeValues[16][16];
} P_parameter;

uint32_t P_GetAudioPortsCount() { return 1; }
uint32_t P_GetLatency() { return 1; }

P_parameter parameters[] = {
    {
        0,
        "Drive",
        0.,

        -1,
        6,
        0.,
        PARAMETER_TYPE__DOUBLE,
    },
    {
        1,
        "Mix",
        0.5,
        0,
        1,
        0.5,
        PARAMETER_TYPE__DOUBLE,
    },
    {
        2,
        "Mode",
        0.,
        0.,
        2,
        0.,
        PARAMETER_TYPE__ENUM,
        {"HARD", "SOFT", "FOLD"},
    },
};
const size_t parametersCount = sizeof(parameters) / sizeof(parameters[0]);

P_parameter P_GetParameter(int32_t index) { return parameters[index]; }
void P_SetParameter(int32_t index, double value) {
  parameters[index].currentValue = value;
}
char *P_GetParameterCurrentValueAsText(int32_t index, double value) {
  char *str = malloc(16);
  P_parameter parameter = P_GetParameter(index);
  switch (parameter.type) {
  case PARAMETER_TYPE__DOUBLE:
    sprintf(str, "%f", value);
    break;
  case PARAMETER_TYPE__ENUM:
    sprintf(str, "%s", parameter.enumTypeValues[(int)value]);
    break;
  case PARAMETER_TYPE__BOOLEAN:
    if (value > 0) {
      // TODO: Not sure if this is correct
      sprintf(str, "%s", "true");
    } else {
      sprintf(str, "%s", "false");
    }
    break;
  };
  return str;
}

uint32_t P_GetParametersCount() { return parametersCount; }

void P_HandleParameterChanged(int32_t index, double value) {
  parameters[index].currentValue = value;
}

void P_HandleMidiNoteOn(int16_t key) { MIDI_NOTE = key; }

void P_HandleMidiNoteOff(int16_t key) { MIDI_NOTE = 0; }

double P_GetMidiAudioOutput() { return (double)MIDI_NOTE; }

typedef struct {
  float left;
  float right;
} P_processAudioResponse;

P_processAudioResponse P_ProcessAudio(float inLeft, float inRight) {
  double drive = parameters[0].currentValue;
  int mode = (int)parameters[1].currentValue;
  double mix = parameters[2].currentValue;

  float out_l, out_r;
  out_l = 0;
  out_r = 0;

  float tl = inLeft * (1.0 + drive);
  float tr = inRight * (1.0 + drive);

  // Obviously this is inefficient but
  switch (mode) {
  case HARD: {
    tl = (tl > 1 ? 1 : tl < -1 ? -1 : tl);
    tr = (tr > 1 ? 1 : tr < -1 ? -1 : tr);
  } break;
  case SOFT: {
    tl = (tl > 1 ? 1 : tl < -1 ? -1 : tl);
    tl = 1.5 * tl - 0.5 * tl * tl * tl;

    tr = (tr > 1 ? 1 : tr < -1 ? -1 : tr);
    tr = 1.5 * tr - 0.5 * tr * tr * tr;
  } break;
  case FOLD: {
    tl = sin(2.0 * M_PI * tl);
    tr = sin(2.0 * M_PI * tr);
  } break;
  }

  out_l = mix * tl + (1.0 - mix) * inLeft;
  out_r = mix * tr + (1.0 - mix) * inRight;
  return (P_processAudioResponse){
      out_l,
      out_r,
  };
}
