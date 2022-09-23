/*
 * Copyright (C) 2022 Dylan Van Assche <me@dylanvanassche.be>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include "fu-infinitime-device.h"

#define INFINITIME_VERSION_UUID	      "00002a26-0000-1000-8000-00805f9b34fb"
#define INFINITIME_CONTROL_POINT_UUID "00001531-1212-efde-1523-785feabcd123"
#define INFINITIME_PACKET_UUID	      "00001532-1212-efde-1523-785feabcd123"

struct _FuInfinitimeDevice {
	FuBluezDevice parent_instance;
};

G_DEFINE_TYPE(FuInfinitimeDevice, fu_infinitime_device, FU_TYPE_BLUEZ_DEVICE)

static gboolean
fu_infinitime_device_probe(FuDevice *device, GError *error)
{
	GByteArray *data = NULL;

	data = fu_bluez_device_read(self, INFINITIME_VERSION_UUID, error);
	if (data == NULL)
		return FALSE;

	return TRUE;
}

static void
fu_infinitime_device_init(FuInfinitimeDevice *self)
{
	fu_device_add_protocol(FU_DEVICE(self), "io.infinitime");
	fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_UPDATABLE);
	fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_UNSIGNED_PAYLOAD);
}

static void
fu_infinitime_device_class_init(FuInfinitimeDeviceClass *klass)
{
	FuDeviceClass *klass_device = FU_DEVICE_CLASS(klass);

	klass_device->probe = fu_infinitime_device_probe;
}
