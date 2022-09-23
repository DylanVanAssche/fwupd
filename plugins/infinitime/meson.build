cargs = ['-DG_LOG_DOMAIN="FuPluginInfinitime"']

if bluez.allowed()
  plugin_quirks += join_paths(meson.current_source_dir(), 'infinitime.quirk')
endif

if bluez.allowed()
shared_module('fu_plugin_infinitime',
  fu_hash,
  sources: [
    'fu-plugin-infinitime.c',
    'fu-infinitime-device.c',
  ],
  include_directories: [
    root_incdir,
    fwupd_incdir,
    fwupdplugin_incdir,
  ],
  install: true,
  install_dir: plugin_dir,
  link_with: [
    fwupd,
    fwupdplugin,
  ],
  c_args: cargs,
  dependencies: [
    plugin_deps,
  ],
)
endif
