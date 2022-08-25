/*
 * Copyright (C) 2022 Dylan Van Assche <me@dylanvanassche.be>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include <fwupdplugin.h>

#include "fu-dd-device.h"

static void
fu_plugin_dd_init(FuPlugin *plugin)
{
	fu_plugin_add_device_gtype(plugin, FU_TYPE_DD_DEVICE);
	fu_plugin_add_udev_subsystem(plugin, "block");
}

void
fu_plugin_init_vfuncs(FuPluginVfuncs *vfuncs)
{
	vfuncs->build_hash = FU_BUILD_HASH;
	vfuncs->init = fu_plugin_dd_init;
}
