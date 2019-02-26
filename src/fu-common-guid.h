/*
 * Copyright (C) 2017-2018 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

gboolean	 fu_common_guid_is_plausible	(const guint8	*buf);

G_END_DECLS
