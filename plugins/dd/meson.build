if get_option('plugin_dd').require(gudev.found(),
    error_message: 'gudev is needed for plugin_dd').allowed()
cargs = ['-DG_LOG_DOMAIN="FuPluginDd"']

install_data(['dd.quirk'],
  install_dir: join_paths(datadir, 'fwupd', 'quirks.d')
)

shared_module('fu_plugin_dd',
  fu_hash,
  sources : [
    'fu-plugin-dd.c',
    'fu-dd-device.c',
  ],
  include_directories : [
    root_incdir,
    fwupd_incdir,
    fwupdplugin_incdir,
  ],
  install : true,
  install_dir: plugin_dir,
  link_with : [
    fwupd,
    fwupdplugin,
  ],
  c_args : cargs,
  dependencies : [
    plugin_deps,
  ],
)
endif
