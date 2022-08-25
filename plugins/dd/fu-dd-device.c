/*
 * Copyright (C) 2022 Dylan Van Assche <me@dylanvanassche.be>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include <fwupdplugin.h>

#include <fcntl.h>
#include <string.h>

#include "fu-dd-device.h"

#define PROC_CMDLINE	 "/proc/cmdline"
#define MAX_CMDLINE_SIZE 4096
#define BOOT_SLOT_ARG	 "androidboot.slot_suffix"
#define ABL_VERSION_ARG	 "androidboot.abl.revision"
#define SERIAL_ARG	 "androidboot.serialno"
#define UNKNOWN_VERSION	 "0.0.UNKNOWN"
#define IO_TIMEOUT	 1500

struct _FuDdDevice {
	FuUdevDevice parent_instance;
	const char *label;
	const char *boot_slot;
	const char *uuid;
	const char *version;
	FuIOChannel *io_channel;
};

G_DEFINE_TYPE(FuDdDevice, fu_dd_device, FU_TYPE_UDEV_DEVICE)

static char *
fu_dd_extract_cmdline_arg_value(const char *key)
{
	gint rc;
	gint fd;
	char pcmd[MAX_CMDLINE_SIZE];
	g_auto(GStrv) args = NULL;
	g_auto(GStrv) key_value = NULL;

	fd = open(PROC_CMDLINE, O_RDONLY);
	rc = read(fd, pcmd, MAX_CMDLINE_SIZE);
	close(fd);

	if (rc > 0) {
		args = g_strsplit(pcmd, " ", -1);
		for (guint i = 0; args[i] != NULL; i++) {
			key_value = g_strsplit(args[i], "=", -1);

			if (!(key_value[0] && key_value[1]))
				continue;

			if (g_strcmp0(key_value[0], key) == 0)
				return g_strdup(key_value[1]);
		}
	} else
		g_warning("Failed to read %s (%d)", PROC_CMDLINE, rc);

	return NULL;
}

static void
fu_dd_device_to_string(FuDevice *device, guint idt, GString *str)
{
	FuDdDevice *self = FU_DD_DEVICE(device);

	g_debug("----------------- fu_dd_device_to_string -----------");

	if (self->boot_slot != NULL)
		fu_string_append(str, idt, "BootSlot", self->boot_slot);

	if (self->label != NULL)
		fu_string_append(str, idt, "Label", self->label);

	if (self->uuid != NULL)
		fu_string_append(str, idt, "UUID", self->uuid);
}

static gboolean
fu_dd_device_probe(FuDevice *device, GError **error)
{
	FuDdDevice *self = FU_DD_DEVICE(device);
	GUdevDevice *udev_device = fu_udev_device_get_dev(FU_UDEV_DEVICE(device));
	gboolean matches_boot_slot = FALSE;
	g_autofree char *serial = NULL;

	g_debug("----------------- fu_dd_device_probe -----------");

	/* FuUdevDevice->probe */
	if (!FU_DEVICE_CLASS(fu_dd_device_parent_class)->probe(device, error))
		return FALSE;

	/* set the physical ID */
	if (!fu_udev_device_set_physical_id(FU_UDEV_DEVICE(device), "block", error))
		return FALSE;

	/* extract label and check if it matches boot slot*/
	if (g_udev_device_has_property(udev_device, "ID_PART_ENTRY_NAME")) {
		self->label = g_udev_device_get_property(udev_device, "ID_PART_ENTRY_NAME");
		g_debug("Partition label: '%s'", self->label);

		/* Use label as device name */
		fu_device_set_name(device, self->label);

		/* If the device has A/B partitioning, compare boot slot to only expose partitions
		 * in-use */
		if (self->boot_slot != NULL && g_str_has_suffix(self->label, self->boot_slot))
			matches_boot_slot = TRUE;
	}

	/* extract partition UUID */
	if (g_udev_device_has_property(udev_device, "ID_PART_ENTRY_UUID")) {
		self->uuid = g_udev_device_get_property(udev_device, "ID_PART_ENTRY_UUID");
		g_debug("Partition UUID: '%s'", self->uuid);
	}

	/* Support all partitions with a UUID */
	if (self->uuid == NULL) {
		g_set_error_literal(error,
				    FWUPD_ERROR,
				    FWUPD_ERROR_NOT_SUPPORTED,
				    "device not supported");
		return FALSE;
	}

	/* Reject partitions from an inactive slot in an A/B scheme */
	if (self->boot_slot != NULL && !matches_boot_slot) {
		g_set_error_literal(error,
				    FWUPD_ERROR,
				    FWUPD_ERROR_NOT_SUPPORTED,
				    "device is on a different bootslot");
		return FALSE;
	}

	/* Set serial */
	serial = fu_dd_extract_cmdline_arg_value(SERIAL_ARG);
	fu_device_set_serial(FU_DEVICE(self), serial);

	/*
	 * Some devices don't have unique TYPE UUIDs, add the partition label to make them truly
	 * unique Devices have a fixed partition scheme anyway because they originally have Android
	 * which has such requirements.
	 */
	fu_device_add_instance_strsafe(device, "UUID", self->uuid);
	fu_device_add_instance_strsafe(device, "LABEL", self->label);
	fu_device_add_instance_strsafe(device, "SLOT", self->boot_slot);

	/* GUID based on UUID */
	fu_device_build_instance_id(device, error, "DRIVE", "UUID", NULL);

	/* GUID based on label, and UUID */
	fu_device_build_instance_id(device, error, "DRIVE", "UUID", "LABEL", NULL);

	/* GUID based on label, boot slot, and UUID */
	fu_device_build_instance_id(device, error, "DRIVE", "UUID", "LABEL", "SLOT", NULL);

	/* Success */
	return TRUE;
}

static gboolean
fu_dd_device_setup(FuDevice *device, GError **error)
{
	FuDdDevice *self = FU_DD_DEVICE(device);

	g_debug("----------------- fu_dd_device_setup -----------");

	/*
	 * Fallback for ABL without version reporting, fwupd will always provide an upgrade in this
	 * case. Once upgraded, the version reporting will be available and the update notification
	 * will disappear. If version reporting is available, the reported version is set.
	 */
	fu_device_set_version(FU_DEVICE(self),
			      self->version == NULL ? UNKNOWN_VERSION : self->version);

	/* Success */
	return TRUE;
}

static gboolean
fu_dd_device_open(FuDevice *device, GError **error)
{
	g_autoptr(GError) error_local = NULL;

	g_debug("----------------- fu_dd_device_open -----------");

	/* FuUdevDevice->open */
	if (!FU_DEVICE_CLASS(fu_dd_device_parent_class)->open(device, &error_local)) {
		if (g_error_matches(error_local, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED)) {
			g_set_error_literal(error,
					    FWUPD_ERROR,
					    FWUPD_ERROR_NOT_SUPPORTED,
					    error_local->message);
			return FALSE;
		}
		g_propagate_error(error, g_steal_pointer(&error_local));
		return FALSE;
	}

	return TRUE;
}

static gboolean
fu_dd_device_write_firmware(FuDevice *device,
			    FuFirmware *firmware,
			    FuProgress *progress,
			    FwupdInstallFlags flags,
			    GError **error)
{
	FuDdDevice *self = FU_DD_DEVICE(device);
	g_autofree gchar *fn = NULL;
	g_autoptr(GBytes) fw = NULL;
	g_autoptr(GFile) file = NULL;
	g_autoptr(GOutputStream) ostr = NULL;
	g_autoptr(GPtrArray) chunks = NULL;

	g_debug("----------------- fu_dd_device_write_firmware -----------");

	/* get blob */
	fw = fu_firmware_get_bytes(firmware, error);
	if (fw == NULL)
		return FALSE;
	chunks = fu_chunk_array_new_from_bytes(fw, 0x0, 0x0, 10 * 1024);

	/* progress */
	fu_progress_set_id(progress, G_STRLOC);
	fu_progress_set_steps(progress, chunks->len);

	if (g_getenv("FWUPD_ANDROID_BOOT_VERBOSE") != NULL)
		fu_dump_bytes(G_LOG_DOMAIN, "writing", fw);

	/* rewind */
	if (!fu_udev_device_seek(FU_UDEV_DEVICE(self), 0x0, error)) {
		g_prefix_error(error, "failed to rewind: ");
		return FALSE;
	}

	/* write each chunk */
	for (guint i = 0; i < chunks->len; i++) {
		FuChunk *chk = g_ptr_array_index(chunks, i);
		if (!fu_udev_device_pwrite(FU_UDEV_DEVICE(self),
					   fu_chunk_get_address(chk),
					   fu_chunk_get_data(chk),
					   fu_chunk_get_data_sz(chk),
					   error)) {
			g_prefix_error(error,
				       "failed to write @0x%x: ",
				       (guint)fu_chunk_get_address(chk));
			return FALSE;
		}
		fu_progress_step_done(progress);
	}

	/* success */
	return TRUE;
}

static gboolean
fu_dd_device_set_quirk_kv(FuDevice *device, const gchar *key, const gchar *value, GError **error)
{
	FuDdDevice *self = FU_DD_DEVICE(device);
	g_autofree char *tmp;

	g_debug("----------------- fu_dd_device_set_quirk_kv -----------");

	/* load from quirks */
	if (g_strcmp0(key, "DdAndroidbootVersionProperty") == 0) {
		self->version = fu_dd_extract_cmdline_arg_value(value);
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
	g_debug("----------------- fu_dd_device_init -----------");
	fu_device_add_protocol(FU_DEVICE(self), "be.dylanvanassche.dd");
	fu_device_set_version_format(FU_DEVICE(self), FWUPD_VERSION_FORMAT_PLAIN);
	fu_device_add_flag(FU_DEVICE(self), FWUPD_DEVICE_FLAG_REQUIRE_AC);

	self->label = NULL;
	self->uuid = NULL;
	self->boot_slot = fu_dd_extract_cmdline_arg_value(BOOT_SLOT_ARG);
	self->version = NULL;
}

static void
fu_dd_device_class_init(FuDdDeviceClass *klass)
{
	FuDeviceClass *klass_device = FU_DEVICE_CLASS(klass);
	GObjectClass *klass_object = G_OBJECT_CLASS(klass);

	g_debug("----------------- fu_dd_device_class_init -----------");

	klass_object->finalize = fu_dd_device_finalize;
	klass_device->probe = fu_dd_device_probe;
	klass_device->setup = fu_dd_device_setup;
	klass_device->open = fu_dd_device_open;
	klass_device->write_firmware = fu_dd_device_write_firmware;
	klass_device->to_string = fu_dd_device_to_string;
	klass_device->set_quirk_kv = fu_dd_device_set_quirk_kv;
}
