/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include <unistd.h>

#include "alloc-util.h"
#include "conf-parser.h"
#include "constants.h"
#include "device-util.h"
#include "devnum-util.h"
#include "errno-util.h"
#include "fd-util.h"
#include "fileio.h"
#include "hibernate-util.h"
#include "log.h"
#include "macro.h"
#include "path-util.h"
#include "sleep-config.h"
#include "stat-util.h"
#include "stdio-util.h"
#include "string-table.h"
#include "string-util.h"
#include "strv.h"
#include "time-util.h"

#define DEFAULT_SUSPEND_ESTIMATION_USEC (1 * USEC_PER_HOUR)

static const char* const sleep_operation_table[_SLEEP_OPERATION_MAX] = {
        [SLEEP_SUSPEND]                = "suspend",
        [SLEEP_HIBERNATE]              = "hibernate",
        [SLEEP_HYBRID_SLEEP]           = "hybrid-sleep",
        [SLEEP_SUSPEND_THEN_HIBERNATE] = "suspend-then-hibernate",
};

DEFINE_STRING_TABLE_LOOKUP(sleep_operation, SleepOperation);

SleepConfig* sleep_config_free(SleepConfig *sc) {
        if (!sc)
                return NULL;

        for (SleepOperation i = 0; i < _SLEEP_OPERATION_CONFIG_MAX; i++) {
                strv_free(sc->modes[i]);
                strv_free(sc->states[i]);
        }

        return mfree(sc);
}

int parse_sleep_config(SleepConfig **ret) {
        _cleanup_(sleep_config_freep) SleepConfig *sc = NULL;
        int allow_suspend = -1, allow_hibernate = -1, allow_s2h = -1, allow_hybrid_sleep = -1;

        assert(ret);

        sc = new(SleepConfig, 1);
        if (!sc)
                return log_oom();

        *sc = (SleepConfig) {
                .hibernate_delay_usec = USEC_INFINITY,
        };

        const ConfigTableItem items[] = {
                { "Sleep", "AllowSuspend",              config_parse_tristate, 0, &allow_suspend                  },
                { "Sleep", "AllowHibernation",          config_parse_tristate, 0, &allow_hibernate                },
                { "Sleep", "AllowSuspendThenHibernate", config_parse_tristate, 0, &allow_s2h                      },
                { "Sleep", "AllowHybridSleep",          config_parse_tristate, 0, &allow_hybrid_sleep             },

                { "Sleep", "SuspendMode",               config_parse_strv,     0, sc->modes + SLEEP_SUSPEND       },
                { "Sleep", "SuspendState",              config_parse_strv,     0, sc->states + SLEEP_SUSPEND      },
                { "Sleep", "HibernateMode",             config_parse_strv,     0, sc->modes + SLEEP_HIBERNATE     },
                { "Sleep", "HibernateState",            config_parse_strv,     0, sc->states + SLEEP_HIBERNATE    },
                { "Sleep", "HybridSleepMode",           config_parse_strv,     0, sc->modes + SLEEP_HYBRID_SLEEP  },
                { "Sleep", "HybridSleepState",          config_parse_strv,     0, sc->states + SLEEP_HYBRID_SLEEP },

                { "Sleep", "HibernateDelaySec",         config_parse_sec,      0, &sc->hibernate_delay_usec       },
                { "Sleep", "SuspendEstimationSec",      config_parse_sec,      0, &sc->suspend_estimation_usec    },
                {}
        };

        (void) config_parse_config_file("sleep.conf", "Sleep\0",
                                        config_item_table_lookup, items,
                                        CONFIG_PARSE_WARN, NULL);

        /* use default values unless set */
        sc->allow[SLEEP_SUSPEND] = allow_suspend != 0;
        sc->allow[SLEEP_HIBERNATE] = allow_hibernate != 0;
        sc->allow[SLEEP_HYBRID_SLEEP] = allow_hybrid_sleep >= 0 ? allow_hybrid_sleep
                : (allow_suspend != 0 && allow_hibernate != 0);
        sc->allow[SLEEP_SUSPEND_THEN_HIBERNATE] = allow_s2h >= 0 ? allow_s2h
                : (allow_suspend != 0 && allow_hibernate != 0);

        if (!sc->states[SLEEP_SUSPEND])
                sc->states[SLEEP_SUSPEND] = strv_new("mem", "standby", "freeze");
        if (!sc->modes[SLEEP_HIBERNATE])
                sc->modes[SLEEP_HIBERNATE] = strv_new("platform", "shutdown");
        if (!sc->states[SLEEP_HIBERNATE])
                sc->states[SLEEP_HIBERNATE] = strv_new("disk");
        if (!sc->modes[SLEEP_HYBRID_SLEEP])
                sc->modes[SLEEP_HYBRID_SLEEP] = strv_new("suspend", "platform", "shutdown");
        if (!sc->states[SLEEP_HYBRID_SLEEP])
                sc->states[SLEEP_HYBRID_SLEEP] = strv_new("disk");
        if (sc->suspend_estimation_usec == 0)
                sc->suspend_estimation_usec = DEFAULT_SUSPEND_ESTIMATION_USEC;

        /* Ensure values set for all required fields */
        if (!sc->states[SLEEP_SUSPEND] || !sc->modes[SLEEP_HIBERNATE]
            || !sc->states[SLEEP_HIBERNATE] || !sc->modes[SLEEP_HYBRID_SLEEP] || !sc->states[SLEEP_HYBRID_SLEEP])
                return log_oom();

        *ret = TAKE_PTR(sc);

        return 0;
}

int sleep_state_supported(char **states) {
        _cleanup_free_ char *supported_sysfs = NULL;
        const char *found;
        int r;

        if (strv_isempty(states))
                return log_debug_errno(SYNTHETIC_ERRNO(ENOMSG), "No sleep state configured.");

        if (access("/sys/power/state", W_OK) < 0)
                return log_debug_errno(errno, "/sys/power/state is not writable: %m");

        r = read_one_line_file("/sys/power/state", &supported_sysfs);
        if (r < 0)
                return log_debug_errno(r, "Failed to read /sys/power/state: %m");

        r = string_contains_word_strv(supported_sysfs, NULL, states, &found);
        if (r < 0)
                return log_debug_errno(r, "Failed to parse /sys/power/state: %m");
        if (r > 0) {
                log_debug("Sleep state '%s' is supported by kernel.", found);
                return true;
        }

        if (DEBUG_LOGGING) {
                _cleanup_free_ char *joined = strv_join(states, " ");
                log_debug("None of the configured sleep states are supported by kernel: %s", strnull(joined));
        }
        return false;
}

int sleep_mode_supported(char **modes) {
        _cleanup_free_ char *supported_sysfs = NULL;
        int r;

        /* Unlike state, kernel has its own default choice if not configured */
        if (strv_isempty(modes)) {
                log_debug("No sleep mode configured, using kernel default.");
                return true;
        }

        if (access("/sys/power/disk", W_OK) < 0)
                return log_debug_errno(errno, "/sys/power/disk is not writable: %m");

        r = read_one_line_file("/sys/power/disk", &supported_sysfs);
        if (r < 0)
                return log_debug_errno(r, "Failed to read /sys/power/disk: %m");

        for (const char *p = supported_sysfs;;) {
                _cleanup_free_ char *word = NULL;
                char *mode;
                size_t l;

                r = extract_first_word(&p, &word, NULL, 0);
                if (r < 0)
                        return log_debug_errno(r, "Failed to parse /sys/power/disk: %m");
                if (r == 0)
                        break;

                mode = word;
                l = strlen(word);

                if (mode[0] == '[' && mode[l - 1] == ']') {
                        mode[l - 1] = '\0';
                        mode++;
                }

                if (strv_contains(modes, mode)) {
                        log_debug("Disk sleep mode '%s' is supported by kernel.", mode);
                        return true;
                }
        }

        if (DEBUG_LOGGING) {
                _cleanup_free_ char *joined = strv_join(modes, " ");
                log_debug("None of the configured hibernation power modes are supported by kernel: %s", strnull(joined));
        }
        return false;
}

static int can_sleep_internal(const SleepConfig *sleep_config, SleepOperation operation, bool check_allowed);

static bool can_s2h(const SleepConfig *sleep_config) {

        static const SleepOperation operations[] = {
                SLEEP_SUSPEND,
                SLEEP_HIBERNATE,
        };

        int r;

        if (!clock_supported(CLOCK_BOOTTIME_ALARM)) {
                log_debug("CLOCK_BOOTTIME_ALARM is not supported.");
                return false;
        }

        for (size_t i = 0; i < ELEMENTSOF(operations); i++) {
                r = can_sleep_internal(sleep_config, operations[i], false);
                if (IN_SET(r, 0, -ENOSPC)) {
                        log_debug("Unable to %s system.", sleep_operation_to_string(operations[i]));
                        return false;
                }
                if (r < 0)
                        return log_debug_errno(r, "Failed to check if %s is possible: %m", sleep_operation_to_string(operations[i]));
        }

        return true;
}

static int can_sleep_internal(
                const SleepConfig *sleep_config,
                SleepOperation operation,
                bool check_allowed) {

        assert(operation >= 0);
        assert(operation < _SLEEP_OPERATION_MAX);

        if (check_allowed && !sleep_config->allow[operation]) {
                log_debug("Sleep mode \"%s\" is disabled by configuration.", sleep_operation_to_string(operation));
                return false;
        }

        if (operation == SLEEP_SUSPEND_THEN_HIBERNATE)
                return can_s2h(sleep_config);

        if (sleep_state_supported(sleep_config->states[operation]) <= 0 ||
            sleep_mode_supported(sleep_config->modes[operation]) <= 0)
                return false;

        if (operation == SLEEP_SUSPEND)
                return true;

        if (!enough_swap_for_hibernation())
                return -ENOSPC;

        return true;
}

int can_sleep(SleepOperation operation) {
        _cleanup_(sleep_config_freep) SleepConfig *sleep_config = NULL;
        int r;

        r = parse_sleep_config(&sleep_config);
        if (r < 0)
                return r;

        return can_sleep_internal(sleep_config, operation, true);
}
