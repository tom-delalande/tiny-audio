#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./i_clap_ui.c"
#include "./p_plugins.c"
#include "./t_log.c"
#include "clap/events.h"
#include "clap/ext/gui.h"
#include "clap/ext/params.h"
#include "clap/plugin.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <clap/clap.h>
#include <pthread.h>

typedef struct {
  clap_plugin_t plugin;
  const clap_host_t *host;
  const clap_host_latency_t *hostLatency;
  const clap_host_log_t *hostLog;
  const clap_host_thread_check_t *hostThreadCheck;
  const clap_host_params_t *hostParams;

  p_plugin *p;

  SDL_Window *window;
  SDL_Renderer *renderer;
  bool running;
  pthread_t thread;
} i_plugin;

// Plugin Definitions

static const clap_plugin_descriptor_t i_pluginDescriptionSynth = {
    .clap_version = CLAP_VERSION_INIT,
    .id = "io.tinyclub.tiny-synth",
    .name = "Tiny Synth",
    .vendor = "tinyclub",
    .url = "tinyclub.io",
    .manual_url = "",
    .support_url = "",
    .version = "1.0.0",
    .description = "A tiny synth",
    .features = (const char *[]){CLAP_PLUGIN_FEATURE_INSTRUMENT,
                                 CLAP_PLUGIN_FEATURE_SYNTHESIZER, NULL},
};

static const clap_plugin_descriptor_t i_pluginDescriptionDrive = {
    .clap_version = CLAP_VERSION_INIT,
    .id = "io.tinyclub.tiny-drive",
    .name = "Tiny Drive",
    .vendor = "tinyclub",
    .url = "tinyclub.io",
    .manual_url = "",
    .support_url = "",
    .version = "1.0.0",
    .description = "A tiny drive",
    .features = (const char *[]){CLAP_PLUGIN_FEATURE_AUDIO_EFFECT,
                                 CLAP_PLUGIN_FEATURE_STEREO, NULL},
};

static void I_EventProcess(p_plugin *plug, const clap_event_header_t *hdr);

/////////////////////////////
// clap_plugin_audio_ports //
/////////////////////////////

static uint32_t I_AudioPortsCount(const clap_plugin_t *plugin, bool is_input) {
  return 1;
}

static bool I_AudioPortsGet(const clap_plugin_t *plugin, uint32_t index,
                            bool is_input, clap_audio_port_info_t *info) {
  if (index > 0)
    return false;
  info->id = 0;
  if (is_input)
    snprintf(info->name, sizeof(info->name), "%s", "Stereo In");
  else
    snprintf(info->name, sizeof(info->name), "%s", "Distorted Output");
  info->channel_count = 2;
  info->flags = CLAP_AUDIO_PORT_IS_MAIN;
  info->port_type = CLAP_PORT_STEREO;
  info->in_place_pair = CLAP_INVALID_ID;
  return true;
}

static const clap_plugin_audio_ports_t i_audioPorts = {
    .count = I_AudioPortsCount,
    .get = I_AudioPortsGet,
};

static uint32_t I_NotePortsCount(const clap_plugin_t *plugin, bool is_input) {
  return 1;
}

static bool I_NotePortsGet(const clap_plugin_t *plugin, uint32_t index,
                           bool is_input, clap_note_port_info_t *info) {
  if (!is_input || index > 0)
    return false;
  info->id = 0;
  snprintf(info->name, sizeof(info->name), "MIDI In");
  info->supported_dialects = CLAP_NOTE_DIALECT_CLAP;
  info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
  return true;
}
static const clap_plugin_note_ports_t i_notePorts = {
    .count = I_NotePortsCount,
    .get = I_NotePortsGet,
};

//////////////////
// clap_latency //
//////////////////

uint32_t I_LatencyGet(const clap_plugin_t *plugin) { return 1; }

static const clap_plugin_latency_t i_latency = {
    .get = I_LatencyGet,
};

//////////////////
// clap_porams //
//////////////////

uint32_t I_ParamCount(const clap_plugin_t *clap_plugin) {
  p_plugin *plugin = ((i_plugin *)clap_plugin->plugin_data)->p;
  return plugin->parameterCount;
}
bool I_ParamInfo(const clap_plugin_t *clap_plugin, uint32_t param_index,
                 clap_param_info_t *param_info) {
  p_plugin *plugin = ((i_plugin *)clap_plugin->plugin_data)->p;
  if (param_index >= plugin->parameterCount) {
    return false;
  }
  p_parameter param = plugin->parameters[param_index];
  param_info->id = param.id;
  strncpy(param_info->name, param.name, CLAP_NAME_SIZE);
  param_info->module[0] = 0;
  param_info->default_value = param.defaultValue;
  param_info->min_value = param.minValue;
  param_info->max_value = param.maxValue;
  int flags;
  switch (param.type) {
  case PARAMETER_TYPE__ENUM:
    flags =
        CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_STEPPED | CLAP_PARAM_IS_ENUM;
    break;
  case PARAMETER_TYPE__BOOLEAN:
    flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_STEPPED;
    break;
  case PARAMETER_TYPE__DOUBLE:
    flags = CLAP_PARAM_IS_AUTOMATABLE;
    break;
  }
  param_info->flags = flags;
  param_info->cookie = NULL;
  return true;
}
bool I_ParamValue(const clap_plugin_t *clap_plugin, clap_id param_id,
                  double *value) {
  p_plugin *plugin = ((i_plugin *)clap_plugin->plugin_data)->p;
  if (param_id >= plugin->parameterCount) {
    return false;
  }
  p_parameter parameter = plugin->parameters[param_id];
  *value = parameter.currentValue;
  return true;
}
bool I_ParamValueToText(const clap_plugin_t *clap_plugin, clap_id param_id,
                        double value, char *display, uint32_t size) {
  p_plugin *plugin = ((i_plugin *)clap_plugin->plugin_data)->p;
  if (param_id >= plugin->parameterCount) {
    return false;
  }
  p_parameter parameter = plugin->parameters[param_id];
  char *text = P_GetParameterCurrentValueAsText(&parameter, value);
  snprintf(display, size, "%s", text);
  free(text);
  return true;
}
bool I_ParamTextToValue(const clap_plugin_t *plugin, clap_id param_id,
                        const char *display, double *value) {
  // I'm not going to bother to support this
  return false;
}
void I_EventFlush(const clap_plugin_t *plugin, const clap_input_events_t *in,
                  const clap_output_events_t *out) {
  p_plugin *plug = plugin->plugin_data;
  int s = in->size(in);
  int q;
  for (q = 0; q < s; ++q) {
    const clap_event_header_t *hdr = in->get(in, q);

    I_EventProcess(plug, hdr);
  }
}

static const clap_plugin_params_t I_params = {
    .count = I_ParamCount,
    .get_info = I_ParamInfo,
    .get_value = I_ParamValue,
    .value_to_text = I_ParamValueToText,
    .text_to_value = I_ParamTextToValue,
    .flush = I_EventFlush,
};

bool I_StateSave(const clap_plugin_t *clap_plugin,
                 const clap_ostream_t *stream) {
  p_plugin *plugin = ((i_plugin *)clap_plugin->plugin_data)->p;
  assert(sizeof(float) == 4);
  assert(sizeof(int32_t) == 4);

  int buffersize = sizeof(int32_t) + plugin->parameterCount * sizeof(double);
  char *buffer = malloc(buffersize);

  int32_t version = 1;
  memcpy(buffer, &version, sizeof(int32_t));
  for (int i = 0; i < plugin->parameterCount; i++) {
    p_parameter parameter = plugin->parameters[i];
    memcpy(buffer + sizeof(int32_t) + sizeof(double) * i,
           &(parameter.currentValue), sizeof(double));
  }

  int written = 0;
  char *curr = buffer;
  while (written != buffersize) {
    int thiswrite = stream->write(stream, curr, buffersize - written);
    if (thiswrite < 0)
      return false;
    curr += thiswrite;
    written += thiswrite;
  }

  return true;
}

bool I_StateLoad(const clap_plugin_t *clap_plugin,
                 const clap_istream_t *stream) {
  p_plugin *plugin = ((i_plugin *)clap_plugin->plugin_data)->p;
  int buffersize = sizeof(int32_t) + plugin->parameterCount * sizeof(double);
  char *buffer = malloc(buffersize);

  int read = 0;
  char *curr = buffer;
  while (read != buffersize) {
    int thisread = stream->read(stream, curr, buffersize - read);
    if (thisread < 0)
      return false;
    curr += thisread;
    read += thisread;
  }

  int32_t version;
  memcpy(&version, buffer, sizeof(int32_t));
  for (int i = 0; i < plugin->parameterCount; i++) {
    double *value = malloc(sizeof(double));
    memcpy(value, buffer + sizeof(int32_t) + sizeof(double) * i,
           sizeof(double));
    // FIXME: If this doesn't work it needs to be a pointer
    plugin->parameters[i].currentValue = *value;
    free(value);
  }

  return true;
}
static const clap_plugin_state_t I_state = {.save = I_StateSave,
                                            .load = I_StateLoad};

/////////////////
// clap_plugin //
/////////////////

static bool I_Init(const struct clap_plugin *plugin) {
  i_plugin *plug = plugin->plugin_data;

  plug->hostLog = plug->host->get_extension(plug->host, CLAP_EXT_LOG);
  plug->hostThreadCheck =
      plug->host->get_extension(plug->host, CLAP_EXT_THREAD_CHECK);
  plug->hostLatency = plug->host->get_extension(plug->host, CLAP_EXT_LATENCY);

  p_plugin *p;
  const int N = sizeof(p_plugins) / sizeof(p_plugins[0]);
  for (int i = 0; i < N; ++i)
    if (!strcmp(plugin->desc->id, p_plugins[i].id))
      p = &p_plugins[i];

  plug->p = p;

  return true;
}

static void I_Destroy(const struct clap_plugin *plugin) {
  i_plugin *plug = plugin->plugin_data;
  free(plug);
}

static bool I_Activate(const struct clap_plugin *plugin, double sample_rate,
                       uint32_t min_frames_count, uint32_t max_frames_count) {
  return true;
}

static void I_Deactivate(const struct clap_plugin *plugin) {}

static bool I_ProcessingStart(const struct clap_plugin *plugin) { return true; }

static void I_ProcessingStop(const struct clap_plugin *plugin) {}

static void I_Reset(const struct clap_plugin *plugin) {}

static void I_EventProcess(p_plugin *plugin, const clap_event_header_t *hdr) {
  if (hdr->space_id == CLAP_CORE_EVENT_SPACE_ID) {
    switch (hdr->type) {
    case CLAP_EVENT_PARAM_VALUE: {
      const clap_event_param_value_t *ev =
          (const clap_event_param_value_t *)hdr;
      plugin->handleParameterChanged(plugin, ev->param_id, ev->value);
      break;
    }
    case CLAP_EVENT_NOTE_ON: {
      const clap_event_note_t *ev = (const clap_event_note_t *)hdr;
      plugin->handleMidiNoteOn(plugin, ev->key);
      break;
    }
    case CLAP_EVENT_NOTE_OFF: {
      const clap_event_note_t *ev = (const clap_event_note_t *)hdr;
      plugin->handleMidiNoteOff(plugin, ev->key);
      break;
    }
    }
  }
}

static clap_process_status I_Process(const struct clap_plugin *clap_plugin,
                                     const clap_process_t *process) {
  p_plugin *plugin = ((i_plugin *)clap_plugin->plugin_data)->p;
  const uint32_t nframes = process->frames_count;
  const uint32_t nev = process->in_events->size(process->in_events);
  uint32_t ev_index = 0;
  uint32_t next_ev_frame = nev > 0 ? 0 : nframes;

  for (uint32_t i = 0; i < nframes;) {
    /* handle every events that happrens at the frame "i" */
    while (ev_index < nev && next_ev_frame == i) {
      const clap_event_header_t *hdr =
          process->in_events->get(process->in_events, ev_index);
      if (hdr->time != i) {
        next_ev_frame = hdr->time;
        break;
      }

      I_EventProcess(plugin, hdr);
      ++ev_index;

      if (ev_index == nev) {
        // we reached the end of the event list
        next_ev_frame = nframes;
        break;
      }
    }

    // /* process every samples until the next event */
    for (; i < next_ev_frame; ++i) {
      const float leftIn = process->audio_inputs[0].data32[0][i];
      const float rightIn = process->audio_inputs[0].data32[1][i];
      p_audio output = plugin->processAudio(plugin, leftIn, rightIn);
      process->audio_outputs[0].data32[0][i] = output.left;
      process->audio_outputs[0].data32[1][i] = output.right;
    }
  }

  return CLAP_PROCESS_CONTINUE;
}

static const void *I_GetExtension(const struct clap_plugin *plugin,
                                  const char *id) {
  if (!strcmp(id, CLAP_EXT_LATENCY))
    return &i_latency;
  if (!strcmp(id, CLAP_EXT_AUDIO_PORTS))
    return &i_audioPorts;
  if (!strcmp(id, CLAP_EXT_NOTE_PORTS))
    return &i_notePorts;
  if (!strcmp(id, CLAP_EXT_PARAMS))
    return &I_params;
  if (!strcmp(id, CLAP_EXT_STATE))
    return &I_state;
  if (!strcmp(id, CLAP_EXT_GUI))
    return &gui_ext;
  return NULL;
}

static void I_OnMainThread(const struct clap_plugin *plugin) {
  // FIXME: This probably shouldn't use gui_show (and have a dedicated function
  // instead)
  gui_show(plugin);
  i_plugin *plugin_data = plugin->plugin_data;
  plugin_data->host->request_callback(plugin_data->host);
}

clap_plugin_t *I_Create_Synth(const clap_host_t *host) {
  i_plugin *p = calloc(1, sizeof(*p));
  host->request_callback(host);
  p->host = host;
  p->plugin.desc = &i_pluginDescriptionSynth;
  p->plugin.plugin_data = p;
  p->plugin.init = I_Init;
  p->plugin.destroy = I_Destroy;
  p->plugin.activate = I_Activate;
  p->plugin.deactivate = I_Deactivate;
  p->plugin.start_processing = I_ProcessingStart;
  p->plugin.stop_processing = I_ProcessingStop;
  p->plugin.reset = I_Reset;
  p->plugin.process = I_Process;
  p->plugin.get_extension = I_GetExtension;
  p->plugin.on_main_thread = I_OnMainThread;

  // Don't call into the host here

  return &p->plugin;
}
clap_plugin_t *I_Create_Drive(const clap_host_t *host) {
  i_plugin *p = calloc(1, sizeof(*p));
  host->request_callback(host);
  p->host = host;
  p->plugin.desc = &i_pluginDescriptionDrive;
  p->plugin.plugin_data = p;
  p->plugin.init = I_Init;
  p->plugin.destroy = I_Destroy;
  p->plugin.activate = I_Activate;
  p->plugin.deactivate = I_Deactivate;
  p->plugin.start_processing = I_ProcessingStart;
  p->plugin.stop_processing = I_ProcessingStop;
  p->plugin.reset = I_Reset;
  p->plugin.process = I_Process;
  p->plugin.get_extension = I_GetExtension;
  p->plugin.on_main_thread = I_OnMainThread;

  // Don't call into the host here

  return &p->plugin;
}

/////////////////////////
// clap_plugin_factory //
/////////////////////////

static struct {
  const clap_plugin_descriptor_t *desc;
  clap_plugin_t *(*create)(const clap_host_t *host);
} s_plugins[] = {
    {
        .desc = &i_pluginDescriptionSynth,
        .create = I_Create_Synth,
    },
    {
        .desc = &i_pluginDescriptionDrive,
        .create = I_Create_Drive,
    },
};

static uint32_t
plugin_factory_get_plugin_count(const struct clap_plugin_factory *factory) {
  return sizeof(s_plugins) / sizeof(s_plugins[0]);
}

static const clap_plugin_descriptor_t *
plugin_factory_get_plugin_descriptor(const struct clap_plugin_factory *factory,
                                     uint32_t index) {
  return s_plugins[index].desc;
}

static const clap_plugin_t *
plugin_factory_create_plugin(const struct clap_plugin_factory *factory,
                             const clap_host_t *host, const char *plugin_id) {
  if (!clap_version_is_compatible(host->clap_version)) {
    return NULL;
  }

  const int N = sizeof(p_plugins) / sizeof(p_plugins[0]);
  for (int i = 0; i < N; ++i)
    if (!strcmp(plugin_id, p_plugins[i].id))
      return s_plugins[i].create(host);

  return NULL;
}

static const clap_plugin_factory_t s_plugin_factory = {
    .get_plugin_count = plugin_factory_get_plugin_count,
    .get_plugin_descriptor = plugin_factory_get_plugin_descriptor,
    .create_plugin = plugin_factory_create_plugin,
};

////////////////
// clap_entry //
////////////////

static bool entry_init(const char *plugin_path) {
  // called only once, and very first
  return true;
}

static void entry_deinit(void) {
  // called before unloading the DSO
}

static const void *entry_get_factory(const char *factory_id) {
  if (!strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID))
    return &s_plugin_factory;
  return NULL;
}

CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
    .clap_version = CLAP_VERSION_INIT,
    .init = entry_init,
    .deinit = entry_deinit,
    .get_factory = entry_get_factory,
};
