#include "./p_plugins.h"
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

const char *plugin_lib = "./plugins.dylib";

const int8_t num_plugins = 1;
const char *i_plugins[] = {"http://example.org/passthrough"};

int main() {
  void *handle = dlopen(plugin_lib, RTLD_NOW);
  p_plugin *p_plugins = (p_plugin *)dlsym(handle, "p_plugins");

  printf("%s", p_plugins[0].id);
  float a = 1.0f;
  float b = 1.0f;
  float c = 1.0f;
  float *parameters[16] = {&a, &b, &c};
  p_plugin *y;

  for (int i = 0; i < num_plugins; ++i) {
    if (strcmp(i_plugins[i], "http://example.org/passthrough") == 0) {
      y = &p_plugins[i];
    }
  }

  y->processAudio(y, 0.0f, 0.0f, parameters);
  return 0;
}
