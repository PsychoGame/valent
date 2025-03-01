// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

#define G_LOG_DOMAIN "valent-pa-mixer"

#include "config.h"

#include <gvc-mixer-control.h>
#include <gvc-mixer-stream.h>
#include <libvalent-core.h>
#include <libvalent-mixer.h>

#include "valent-pa-mixer.h"
#include "valent-pa-stream.h"


struct _ValentPaMixer
{
  ValentMixerControl  parent_instance;

  GvcMixerControl    *control;

  GHashTable         *inputs;
  GHashTable         *outputs;
  unsigned int        input;
  unsigned int        output;
  unsigned int        vol_max;
};

G_DEFINE_TYPE (ValentPaMixer, valent_pa_mixer, VALENT_TYPE_MIXER_CONTROL)


/*
 * Gvc Callbacks
 */
static void
on_default_sink_changed (GvcMixerControl *control,
                         unsigned int     id,
                         ValentPaMixer   *self)
{
  g_assert (VALENT_IS_PA_MIXER (self));

  self->output = id;
}

static void
on_default_source_changed (GvcMixerControl *control,
                           unsigned int     id,
                           ValentPaMixer   *self)
{
  g_assert (VALENT_IS_PA_MIXER (self));

  self->input = id;
}

static void
on_input_added (GvcMixerControl *control,
                unsigned int     stream_id,
                ValentPaMixer   *self)
{
  GvcMixerStream *gvc_stream;
  ValentMixerStream *stream;
  ValentMixerStreamFlags flags = VALENT_MIXER_STREAM_LOCAL;

  g_assert (VALENT_IS_PA_MIXER (self));

  gvc_stream = gvc_mixer_control_lookup_stream_id (control, stream_id);

  if G_UNLIKELY (gvc_stream == NULL)
    return;

  stream = g_object_new (VALENT_TYPE_PA_STREAM,
                         "base-stream", gvc_stream,
                         "flags",       flags | VALENT_MIXER_STREAM_SOURCE,
                         "vol-max",     self->vol_max,
                         NULL);

  g_hash_table_insert (self->inputs, GUINT_TO_POINTER (stream_id), stream);
  valent_mixer_control_emit_stream_added (VALENT_MIXER_CONTROL (self), stream);
}

static void
on_input_removed (GvcMixerControl *control,
                  unsigned int     stream_id,
                  ValentPaMixer   *self)
{
  ValentMixerStream *stream;

  g_assert (VALENT_IS_PA_MIXER (self));

  stream = g_hash_table_lookup (self->inputs, GUINT_TO_POINTER (stream_id));

  if G_UNLIKELY (stream == NULL)
    return;

  valent_mixer_control_emit_stream_removed (VALENT_MIXER_CONTROL (self), stream);
  g_hash_table_remove (self->inputs, GUINT_TO_POINTER (stream_id));
}

static void
on_output_added (GvcMixerControl *control,
                 unsigned int     stream_id,
                 ValentPaMixer   *self)
{
  GvcMixerStream *gvc_stream;
  ValentMixerStream *stream;
  ValentMixerStreamFlags flags = VALENT_MIXER_STREAM_LOCAL;

  g_assert (VALENT_IS_PA_MIXER (self));

  gvc_stream = gvc_mixer_control_lookup_stream_id (control, stream_id);

  if G_UNLIKELY (gvc_stream == NULL)
    return;

  stream = g_object_new (VALENT_TYPE_PA_STREAM,
                         "base-stream", gvc_stream,
                         "flags",       flags | VALENT_MIXER_STREAM_SINK,
                         "vol-max",     self->vol_max,
                         NULL);

  g_hash_table_insert (self->outputs, GUINT_TO_POINTER (stream_id), stream);
  valent_mixer_control_emit_stream_added (VALENT_MIXER_CONTROL (self), stream);
}

static void
on_output_removed (GvcMixerControl *control,
                   unsigned int     stream_id,
                   ValentPaMixer   *self)
{
  ValentMixerStream *stream;

  g_assert (VALENT_IS_PA_MIXER (self));

  stream = g_hash_table_lookup (self->inputs, GUINT_TO_POINTER (stream_id));

  if G_UNLIKELY (stream == NULL)
    return;

  valent_mixer_control_emit_stream_removed (VALENT_MIXER_CONTROL (self), stream);
  g_hash_table_remove (self->outputs, GUINT_TO_POINTER (stream_id));
}

static void
on_stream_changed (GvcMixerControl *gvc_control,
                   unsigned int     stream_id,
                   ValentPaMixer   *self)
{
  ValentMixerControl *control = VALENT_MIXER_CONTROL (self);
  ValentMixerStream *stream;

  stream = g_hash_table_lookup (self->outputs, GUINT_TO_POINTER (stream_id));

  if (stream == NULL)
    stream = g_hash_table_lookup (self->inputs, GUINT_TO_POINTER (stream_id));

  if (stream != NULL)
    valent_mixer_control_emit_stream_changed (control, stream);
}

static void
on_state_changed (GvcMixerControl      *control,
                  GvcMixerControlState  state,
                  ValentPaMixer        *self)
{
  g_autoptr (GSList) sources = NULL;
  g_autoptr (GSList) sinks = NULL;
  GvcMixerStream *stream;

  g_assert (VALENT_IS_PA_MIXER (self));

  if (state == GVC_STATE_FAILED)
    {
      g_warning ("Failed to open PulseAudio mixer control");
      return;
    }

  if (state != GVC_STATE_READY)
    return;

  /* Get current streams */
  sinks = gvc_mixer_control_get_sinks (self->control);

  for (const GSList *iter = sinks; iter; iter = iter->next)
    on_output_added (self->control, gvc_mixer_stream_get_id (iter->data), self);

  sources = gvc_mixer_control_get_sources (self->control);

  for (const GSList *iter = sources; iter; iter = iter->next)
    on_input_added (self->control, gvc_mixer_stream_get_id (iter->data), self);

  /* Get current defaults */
  self->vol_max = gvc_mixer_control_get_vol_max_norm (self->control);

  stream = gvc_mixer_control_get_default_sink (self->control);
  self->output = gvc_mixer_stream_get_id (stream);

  stream = gvc_mixer_control_get_default_source (self->control);
  self->input = gvc_mixer_stream_get_id (stream);

  /* Watch stream changes */
  g_object_connect (self->control,
                    "signal::default-sink-changed",   on_default_sink_changed,   self,
                    "signal::default-source-changed", on_default_source_changed, self,
                    "signal::input-added",            on_input_added,            self,
                    "signal::input-removed",          on_input_removed,          self,
                    "signal::output-added",           on_output_added,           self,
                    "signal::output-removed",         on_output_removed,         self,
                    "signal::stream-changed",         on_stream_changed,         self,
                    NULL);
}


/*
 * ValentMixerControl
 */
static ValentMixerStream *
valent_pa_mixer_get_default_input (ValentMixerControl *control)
{
  ValentPaMixer *self = VALENT_PA_MIXER (control);

  return g_hash_table_lookup (self->inputs, GUINT_TO_POINTER (self->input));
}

static ValentMixerStream *
valent_pa_mixer_get_default_output (ValentMixerControl *control)
{
  ValentPaMixer *self = VALENT_PA_MIXER (control);

  return g_hash_table_lookup (self->outputs, GUINT_TO_POINTER (self->output));
}

/*
 * GObject
 */
static void
valent_pa_mixer_init (ValentPaMixer *self)
{
  self->control = g_object_new (GVC_TYPE_MIXER_CONTROL,
                                "name", "Valent",
                                NULL);
  self->inputs = g_hash_table_new_full (NULL, NULL, NULL, g_object_unref);
  self->outputs = g_hash_table_new_full (NULL, NULL, NULL, g_object_unref);
}

static void
valent_pa_mixer_constructed (GObject *object)
{
  ValentPaMixer *self = VALENT_PA_MIXER (object);

  self->vol_max = gvc_mixer_control_get_vol_max_norm (self->control);

  /* Open the base mixer control */
  g_signal_connect (self->control,
                    "state-changed",
                    G_CALLBACK (on_state_changed),
                    self);

  if (!gvc_mixer_control_open (self->control))
    g_warning ("Failed to open GvcMixerControl");

  G_OBJECT_CLASS (valent_pa_mixer_parent_class)->constructed (object);
}

static void
valent_pa_mixer_dispose (GObject *object)
{
  ValentPaMixer *self = VALENT_PA_MIXER (object);

  /* Close the mixer */
  if (!gvc_mixer_control_close (self->control))
    g_warning ("Failed to close GvcMixerControl");

  g_signal_handlers_disconnect_by_data (self->control, self);

  /* Drop sources and sinks */
  g_hash_table_remove_all (self->inputs);
  g_hash_table_remove_all (self->outputs);

  G_OBJECT_CLASS (valent_pa_mixer_parent_class)->dispose (object);
}

static void
valent_pa_mixer_finalize (GObject *object)
{
  ValentPaMixer *self = VALENT_PA_MIXER (object);

  g_clear_pointer (&self->inputs, g_hash_table_unref);
  g_clear_pointer (&self->outputs, g_hash_table_unref);
  g_clear_object (&self->control);

  G_OBJECT_CLASS (valent_pa_mixer_parent_class)->finalize (object);
}

static void
valent_pa_mixer_class_init (ValentPaMixerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ValentMixerControlClass *control_class = VALENT_MIXER_CONTROL_CLASS (klass);

  object_class->constructed = valent_pa_mixer_constructed;
  object_class->dispose = valent_pa_mixer_dispose;
  object_class->finalize = valent_pa_mixer_finalize;

  control_class->get_default_input = valent_pa_mixer_get_default_input;
  control_class->get_default_output = valent_pa_mixer_get_default_output;
}

