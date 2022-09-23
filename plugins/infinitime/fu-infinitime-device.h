/*
 * Copyright (C) 2022 Dylan Van Assche <me@dylanvanassche.be>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <fwupdplugin.h>

#define FU_TYPE_INFINITIME_DEVICE (fu_infinitime_device_get_type())
G_DECLARE_FINAL_TYPE(FuInfinitimeDevice, fu_infinitime_device, FU, INFINITIME_DEVICE, FuBluezDevice)
