# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2021 Andy Holmes <andrew.g.r.holmes@gmail.com>

# Dependencies
plugin_mpris_deps = [
  libvalent_core_dep,
  libvalent_media_dep,
]

# Sources
plugin_mpris_sources = files([
  'mpris-plugin.c',
  'valent-mpris-plugin.c',
  'valent-mpris-player.c',
  'valent-mpris-player-provider.c',
  'valent-mpris-remote.c',
])

plugin_mpris_include_directories = [include_directories('.')]

# Resources
plugin_mpris_info = i18n.merge_file(
   input: 'mpris.plugin.desktop.in',
  output: 'mpris.plugin',
  po_dir: po_dir,
    type: 'desktop',
)

plugin_mpris_resources = gnome.compile_resources('mpris-resources',
                                                 'mpris.gresource.xml',
        c_name: 'mpris',
  dependencies: [plugin_mpris_info],
)
plugin_mpris_sources += plugin_mpris_resources

# Static Build
plugin_mpris = static_library('plugin-mpris',
                              plugin_mpris_sources,
  include_directories: plugin_mpris_include_directories,
         dependencies: plugin_mpris_deps,
               c_args: plugins_c_args + release_args,
)

#plugins_link_args += ['-Wl,--undefined=valent_mpris_plugin_register_types']
plugins_static += [plugin_mpris]

