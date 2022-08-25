/*
 * Copyright (C) 2022 Dylan Van Assche <me@dylanvanassche.be>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "config.h"

#include <fwupdplugin.h>

#include <fcntl.h>
#include <string.h>

#include "fu-dd-device.h"

#define PROC_CMDLINE	 "/proc/cmdline"
#define MAX_CMDLINE_SIZE 4096
#define BOOT_SLOT_ARG	 "androidboot.slot_suffix"
#define ABL_VERSION_ARG	 "androidboot.abl.version"
#define SERIAL_ARG	 "androidboot.serialno"
#define UNKNOWN_VERSION	 "0.0.UNKNOWN"

struct _FuDdDevice {
	FuUdevDevice parent_instance;
	gchar *label;
	gchar *boot_slot;
	gchar *type_uuid;
	gchar *vendor;
	gchar *vendor_id;
};

G_DEFINE_TYPE(FuDdDevice, fu_dd_device, FU_TYPE_UDEV_DEVICE)

static void
fu_dd_device_to_string(FuDevice *device, guint idt, GString *str)
{
	FuDdDevice *self = FU_DD_DEVICE(device);

	g_debug("----------------- fu_dd_device_to_string -----------");

	if (self->boot_slot)
		fu_common_string_append_kv(str, idt, "BootSlot", self->boot_slot);

	if (self->label)
		fu_common_string_append_kv(str, idt, "Label", self->label);

	if (self->type_uuid)
		fu_common_string_append_kv(str, idt, "Type UUID", self->type_uuid);
}

static gboolean
fu_dd_device_probe(FuDevice *device, GError **error)
{
	FuDdDevice *self = FU_DD_DEVICE(device);
	GUdevDevice *udev_device = fu_udev_device_get_dev(FU_UDEV_DEVICE(device));
	gboolean matches_boot_slot = FALSE;

	g_debug("----------------- fu_dd_device_probe -----------");

	/* FuUdevDevice->probe */
	if (!FU_DEVICE_CLASS(fu_dd_device_parent_class)->probe(device, error))
		return FALSE;

	g_debug("Parent class probe done");

	/* set the physical ID */
	if (!fu_udev_device_set_physical_id(FU_UDEV_DEVICE(device), "block", error))
		return FALSE;

	if (g_udev_device_has_property(udev_device, "ID_PART_ENTRY_NAME")) {
		g_debug("has_property OK");
		self->label = g_udev_device_get_property(udev_device, "ID_PART_ENTRY_NAME");
		g_debug("Partition label: '%s'", self->label);

		/* Use label as device name */
		fwupd_device_set_name(device, self->label);

		/* If the device has A/B partitioning, compare boot slot to only expose partitions
		 * in-use */
		if (self->boot_slot && g_str_has_suffix(self->label, self->boot_slot))
			matches_boot_slot = TRUE;
	}

	if (g_udev_device_has_property(udev_device, "ID_PART_ENTRY_TYPE")) {
		self->type_uuid = g_udev_device_get_property(udev_device, "ID_PART_ENTRY_TYPE");
		g_debug("Partition type UUID: '%s'", self->type_uuid);
	}

	g_debug("probe OK");

	/* Support all partitions with a type UUID */
	if (self->type_uuid) {
		if (self->boot_slot) {
			if (matches_boot_slot)
				return TRUE;
			else {
				g_set_error_literal(error,
						    FWUPD_ERROR,
						    FWUPD_ERROR_NOT_SUPPORTED,
						    "device is on a different bootslot");
				return FALSE;
			}
		}
		return TRUE;
	}

	g_set_error_literal(error, FWUPD_ERROR, FWUPD_ERROR_NOT_SUPPORTED, "device not supported");
	return FALSE;
}

static gboolean
fu_dd_device_setup(FuDevice *device, GError **error)
{
	FuDdDevice *self = FU_DD_DEVICE(device);
	g_autofree gchar *guid, *guid_label, *guid_label_slot = NULL;
	g_autofree gchar *guid_id, *guid_id_label, *guid_id_label_slot = NULL;
	g_autofree gchar *vendor_id = NULL;

	g_debug("----------------- fu_dd_device_setup -----------");

	/*
	 * Some devices don't have unique TYPE UUIDs, add the partition label to make them truly
	 * unique Devices have a fixed partition scheme anyway because they originally have Android
	 * which has such requirements.
	 */

	/* GUID based on label, boot slot, and type UUID */
	if (self->label && self->boot_slot) {
		guid_id_label_slot = g_strdup_printf("DRIVE\\%s_E478-FA50&LABEL=%s&SLOT=%s",
						     self->type_uuid,
						     self->label,
						     self->boot_slot);
		g_debug("guid_id_label_slot OK");
		fu_device_add_instance_id(device, guid_id_label_slot);
		guid_label_slot = fwupd_guid_hash_string(guid_id_label_slot);
		fu_device_add_guid(device, guid_label_slot);
	}

	g_debug("label and boot slot GUID OK");

	/* GUID based on label, and type UUID */
	if (self->label) {
		guid_id_label =
		    g_strdup_printf("DRIVE\\%s_E478-FA50&LABEL=%s", self->type_uuid, self->label);
		fu_device_add_instance_id(device, guid_id_label);
		guid_label = fwupd_guid_hash_string(guid_id_label);
		fu_device_add_guid(device, guid_label);
	}

	g_debug("label GUID OK");

	/* GUID based on type UUID */
	guid_id = g_strdup_printf("DRIVE\\%s_E478-FA50", self->type_uuid);
	fu_device_add_instance_id(device, guid_id);
	guid = fwupd_guid_hash_string(guid_id);
	fu_device_add_guid(device, guid);

	g_debug("GUID OK");

	if (self->vendor) {
		fu_device_set_vendor(device, self->vendor);
		fu_device_add_vendor_id(device, self->vendor_id);
	}

	g_debug("vendor OK");

	/* Success */
	return TRUE;
}

static gboolean
fu_dd_device_open(FuDevice *device, GError **error)
{
	return TRUE;
}

static gboolean
fu_dd_device_close(FuDevice *device, GError **error)
{
	return TRUE;
}

static gboolean
fu_dd_device_write_firmware(FuDevice *device,
			    FuFirmware *firmware,
			    FuProgress *progress,
			    FwupdInstallFlags flags,
			    GError **error)
{
	return TRUE;
}

static gboolean
fu_dd_device_set_quirk_kv(FuDevice *device, const gchar *key, const gchar *value, GError **error)
{
	FuDdDevice *self = FU_DD_DEVICE(device);

	g_debug("----------------- fu_dd_device_set_quirk_kv -----------");

	/* load from quirks */
	if (g_strcmp0(key, "DdVendor") == 0) {
		self->vendor = g_strdup(value);
		return TRUE;
	}

	if (g_strcmp0(key, "DdVendorId") == 0) {
		self->vendor_id = g_strdup(value);
		return TRUE;
	}

	/* failed */
	g_set_error_literal(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "quirk key not supported");
	return FALSE;
}

static void
fu_dd_device_finalize(GObject *obj)
{
	G_OBJECT_CLASS(fu_dd_device_parent_class)->finalize(obj);
}

static void
fu_dd_device_init(FuDdDevice *self)
{
	gint rc;
	gint fd;
	char pcmd[MAX_CMDLINE_SIZE];
	gchar **args = NULL;
	gchar **key_value = NULL;
	gboolean has_abl_version = FALSE;

	g_debug("----------------- fu_dd_device_init -----------");
	fu_device_add_protocol(FU_DEVICE(self), "be.dylanvanassche.dd");
	fu_device_set_version_format(FU_DEVICE(self), FWUPD_VERSION_FORMAT_PLAIN);
	fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_REQUIRE_AC);

	self->label = NULL;
	self->type_uuid = NULL;
	self->boot_slot = NULL;
	self->vendor = NULL;
	self->vendor_id = NULL;

	/* Detect A/B partitioning scheme and retrieve active boot slot */
	fd = open(PROC_CMDLINE, O_RDONLY);
	rc = read(fd, pcmd, MAX_CMDLINE_SIZE);
	close(fd);

	if (rc > 0) {
		args = g_strsplit(pcmd, " ", -1);
		for (guint i = 0; args[i] != NULL; i++) {
			key_value = g_strsplit(args[i], "=", -1);

			if (!(key_value[0] && key_value[1]))
				continue;

			if (g_strcmp0(key_value[0], BOOT_SLOT_ARG) == 0) {
				self->boot_slot = g_strdup(key_value[1]);
				g_info("Found A/B partitioning scheme, active bootslot: '%s'",
				       self->boot_slot);
			}

			if (g_strcmp0(key_value[0], ABL_VERSION_ARG) == 0) {
				g_info("Found ABL version: '%s'", key_value[1]);
				fu_device_set_version(FU_DEVICE(self), g_strdup(key_value[1]));
				has_abl_version = TRUE;
			}

			if (g_strcmp0(key_value[0], SERIAL_ARG) == 0) {
				g_info("Found serial number: '%s'", key_value[1]);
				fu_device_set_serial(FU_DEVICE(self), g_strdup(key_value[1]));
			}
		}
	} else
		g_warning("Failed to read /proc/cmdline (%d)", rc);

	/*
	 * Fallback for ABL without version reporting, fwupd will always provide an upgrade in this
	 * case. Once upgraded, the version reporting will be available and the update notification
	 * will disappear
	 */
	if (!has_abl_version) {
		g_info("No version information available");
		fu_device_set_version(FU_DEVICE(self), UNKNOWN_VERSION);
	}
}

static void
fu_dd_device_class_init(FuDdDeviceClass *klass)
{
	g_debug("----------------- fu_dd_device_class_init -----------");
	FuDeviceClass *klass_device = FU_DEVICE_CLASS(klass);
	GObjectClass *klass_object = G_OBJECT_CLASS(klass);
	klass_object->finalize = fu_dd_device_finalize;
	klass_device->probe = fu_dd_device_probe;
	klass_device->setup = fu_dd_device_setup;
	klass_device->open = fu_dd_device_open;
	klass_device->close = fu_dd_device_close;
	klass_device->write_firmware = fu_dd_device_write_firmware;
	klass_device->to_string = fu_dd_device_to_string;
	klass_device->set_quirk_kv = fu_dd_device_set_quirk_kv;
}
