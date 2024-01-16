/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "varlink-io.elogind.service.h"

static VARLINK_DEFINE_METHOD(Ping);

static VARLINK_DEFINE_METHOD(Reload);

static VARLINK_DEFINE_METHOD(
                SetLogLevel,
                VARLINK_DEFINE_INPUT(level, VARLINK_INT, 0));

VARLINK_DEFINE_INTERFACE(
                io_elogind_service,
                "io.elogind.service",
                &vl_method_Ping,
                &vl_method_Reload,
                &vl_method_SetLogLevel);
