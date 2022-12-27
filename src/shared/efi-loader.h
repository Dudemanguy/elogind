/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <sys/stat.h>

#include "efivars-fundamental.h"
#include "efivars.h"

/* Various calls that interface with EFI variables implementing https://systemd.io/BOOT_LOADER_INTERFACE */

#if ENABLE_EFI


#if 0 /// UNNEEDED by elogind
#endif // 0
int efi_loader_get_device_part_uuid(sd_id128_t *ret);
int efi_loader_get_boot_usec(usec_t *ret_firmware, usec_t *ret_loader);

int efi_loader_get_entries(char ***ret);

int efi_loader_get_features(uint64_t *ret);
int efi_stub_get_features(uint64_t *ret);

int efi_loader_get_config_timeout_one_shot(usec_t *ret);
int efi_loader_update_entry_one_shot_cache(char **cache, struct stat *cache_stat);

#if 0 /// UNNEEDED by elogind
#endif // 0

#else


#if 0 /// UNNEEDED by elogind
static inline int efi_loader_get_device_part_uuid(sd_id128_t *u) {
        return -EOPNOTSUPP;
}

static inline int efi_loader_get_boot_usec(usec_t *firmware, usec_t *loader) {
        return -EOPNOTSUPP;
}
#endif // 0

static inline int efi_loader_get_entries(char ***ret) {
        return -EOPNOTSUPP;
}

static inline int efi_loader_get_features(uint64_t *ret) {
        return -EOPNOTSUPP;
}

static inline int efi_stub_get_features(uint64_t *ret) {
        return -EOPNOTSUPP;
}

static inline int efi_loader_get_config_timeout_one_shot(usec_t *ret) {
        return -EOPNOTSUPP;
}

static inline int efi_loader_update_entry_one_shot_cache(char **cache, struct stat *cache_stat) {
        return -EOPNOTSUPP;
}

#if 0 /// UNNEEDED by elogind
#endif // 0

#endif

bool efi_loader_entry_name_valid(const char *s);

#if 0 /// UNNEEDED in elogind
#endif // 0
