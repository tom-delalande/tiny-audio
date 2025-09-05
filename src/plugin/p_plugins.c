#include "./p_plugins.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

p_audio drive_processAudio(p_plugin *plugin, float leftIn, float rightIn,
                           float **parameterValues) {
  printf("Drive - Process Audio\n");
  float drive = *(parameterValues[0]);
  float mix = *(parameterValues[1]);
  int mode = (int)*(parameterValues[2]);

  drive = 6.0f;

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
