#include <stdint.h>
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

typedef struct {
  float left;
  float right;
} p_audio;

typedef struct p_plugin p_plugin;
struct p_plugin {
  char *id;
  p_audio (*processAudio)(p_plugin *plugin, float leftIn, float rightIn,
                          float **parameterValues);
  p_parameter *parameters;
  float **parameterValues;
  uint32_t parameterCount;
};

p_plugin p_plugins[1];
