#include "./p_plugins.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

float running_average;
// DSP Notes
// Doubling the amplitude is a 6db increase
// Halving it is a 6db decrease

p_audio drive_processAudio(p_plugin *plugin, float leftIn, float rightIn,
                           float **parameterValues) {
  printf("Drive - Process Audio\n");
  float out_l, out_r;
  out_l = 0;
  out_r = 0;
  float cutoff = 0.0f;

  float r = ((float)rand() / RAND_MAX) * 0.02;

  float drive = 1.2f;
  float value = drive + r;

  if (leftIn > cutoff) {
    out_l = leftIn * value;
  }
  if (rightIn > cutoff) {
    out_r = rightIn * value;
  }
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

p_plugin p_plugins[1] = {{
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
