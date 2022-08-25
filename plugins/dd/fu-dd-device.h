/*
 * Copyright (C) 2022 Dylan Van Assche <me@dylanvanassche.be>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <fwupdplugin.h>

#define FU_TYPE_DD_DEVICE (fu_dd_device_get_type())

G_DECLARE_FINAL_TYPE(FuDdDevice, fu_dd_device, FU, DD_DEVICE, FuUdevDevice)
