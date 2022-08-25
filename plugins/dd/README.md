# dd

## Introduction

This plugin is used to update hardware that use partitions to store their firmware on.

## Firmware Format

The daemon will decompress the cabinet archive and extract a firmware blob as a Raw Disk File
in the IMG format. The firmware blob will be flashed to the partitions with a label matching
the `DdPartitionLabel` quirk.

This plugin supports the following protocol ID:

* be.dylanvanassche.dd

## GUID Generation

These devices use the standard USB DeviceInstanceId values, e.g.

* `USB\VID_18D1&PID_4EE0&REV_0001`
* `USB\VID_18D1&PID_4EE0`
* `USB\VID_18D1`

## Update Behavior

TODO

## Quirk Use

This plugin uses the following plugin-specific quirk:

### DdPartitionLabel

Partition label of the partition to flash.

Since: 1.8.1

## Vendor ID Security

TODO

## External Interface Access

This plugin requires read/write access to `/dev/bus/usb`.
