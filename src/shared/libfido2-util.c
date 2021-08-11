/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include "libfido2-util.h"

#if HAVE_LIBFIDO2
#include "alloc-util.h"
#include "dlfcn-util.h"
#include "log.h"

static void *libfido2_dl = NULL;

int (*sym_fido_assert_allow_cred)(fido_assert_t *, const unsigned char *, size_t) = NULL;
void (*sym_fido_assert_free)(fido_assert_t **) = NULL;
size_t (*sym_fido_assert_hmac_secret_len)(const fido_assert_t *, size_t) = NULL;
const unsigned char* (*sym_fido_assert_hmac_secret_ptr)(const fido_assert_t *, size_t) = NULL;
fido_assert_t* (*sym_fido_assert_new)(void) = NULL;
int (*sym_fido_assert_set_clientdata_hash)(fido_assert_t *, const unsigned char *, size_t) = NULL;
int (*sym_fido_assert_set_extensions)(fido_assert_t *, int) = NULL;
int (*sym_fido_assert_set_hmac_salt)(fido_assert_t *, const unsigned char *, size_t) = NULL;
int (*sym_fido_assert_set_rp)(fido_assert_t *, const char *) = NULL;
int (*sym_fido_assert_set_up)(fido_assert_t *, fido_opt_t) = NULL;
size_t (*sym_fido_cbor_info_extensions_len)(const fido_cbor_info_t *) = NULL;
char **(*sym_fido_cbor_info_extensions_ptr)(const fido_cbor_info_t *) = NULL;
void (*sym_fido_cbor_info_free)(fido_cbor_info_t **) = NULL;
fido_cbor_info_t* (*sym_fido_cbor_info_new)(void) = NULL;
void (*sym_fido_cred_free)(fido_cred_t **) = NULL;
size_t (*sym_fido_cred_id_len)(const fido_cred_t *) = NULL;
const unsigned char* (*sym_fido_cred_id_ptr)(const fido_cred_t *) = NULL;
fido_cred_t* (*sym_fido_cred_new)(void) = NULL;
int (*sym_fido_cred_set_clientdata_hash)(fido_cred_t *, const unsigned char *, size_t) = NULL;
int (*sym_fido_cred_set_extensions)(fido_cred_t *, int) = NULL;
int (*sym_fido_cred_set_rk)(fido_cred_t *, fido_opt_t) = NULL;
int (*sym_fido_cred_set_rp)(fido_cred_t *, const char *, const char *) = NULL;
int (*sym_fido_cred_set_type)(fido_cred_t *, int) = NULL;
int (*sym_fido_cred_set_user)(fido_cred_t *, const unsigned char *, size_t, const char *, const char *, const char *) = NULL;
int (*sym_fido_cred_set_uv)(fido_cred_t *, fido_opt_t) = NULL;
void (*sym_fido_dev_free)(fido_dev_t **) = NULL;
int (*sym_fido_dev_get_assert)(fido_dev_t *, fido_assert_t *, const char *) = NULL;
int (*sym_fido_dev_get_cbor_info)(fido_dev_t *, fido_cbor_info_t *) = NULL;
void (*sym_fido_dev_info_free)(fido_dev_info_t **, size_t) = NULL;
int (*sym_fido_dev_info_manifest)(fido_dev_info_t *, size_t, size_t *) = NULL;
const char* (*sym_fido_dev_info_manufacturer_string)(const fido_dev_info_t *) = NULL;
const char* (*sym_fido_dev_info_product_string)(const fido_dev_info_t *) = NULL;
fido_dev_info_t* (*sym_fido_dev_info_new)(size_t) = NULL;
const char* (*sym_fido_dev_info_path)(const fido_dev_info_t *) = NULL;
const fido_dev_info_t* (*sym_fido_dev_info_ptr)(const fido_dev_info_t *, size_t) = NULL;
bool (*sym_fido_dev_is_fido2)(const fido_dev_t *) = NULL;
int (*sym_fido_dev_make_cred)(fido_dev_t *, fido_cred_t *, const char *) = NULL;
fido_dev_t* (*sym_fido_dev_new)(void) = NULL;
int (*sym_fido_dev_open)(fido_dev_t *, const char *) = NULL;
const char* (*sym_fido_strerr)(int) = NULL;

int dlopen_libfido2(void) {
        _cleanup_(dlclosep) void *dl = NULL;
        int r;

        if (libfido2_dl)
                return 0; /* Already loaded */

        dl = dlopen("libfido2.so.1", RTLD_LAZY);
        if (!dl)
                return log_debug_errno(SYNTHETIC_ERRNO(EOPNOTSUPP),
                                       "libfido2 support is not installed: %s", dlerror());

        r = dlsym_many_and_warn(
                        dl,
                        LOG_DEBUG,
                        DLSYM_ARG(fido_assert_allow_cred),
                        DLSYM_ARG(fido_assert_free),
                        DLSYM_ARG(fido_assert_hmac_secret_len),
                        DLSYM_ARG(fido_assert_hmac_secret_ptr),
                        DLSYM_ARG(fido_assert_new),
                        DLSYM_ARG(fido_assert_set_clientdata_hash),
                        DLSYM_ARG(fido_assert_set_extensions),
                        DLSYM_ARG(fido_assert_set_hmac_salt),
                        DLSYM_ARG(fido_assert_set_rp),
                        DLSYM_ARG(fido_assert_set_up),
                        DLSYM_ARG(fido_cbor_info_extensions_len),
                        DLSYM_ARG(fido_cbor_info_extensions_ptr),
                        DLSYM_ARG(fido_cbor_info_free),
                        DLSYM_ARG(fido_cbor_info_new),
                        DLSYM_ARG(fido_cred_free),
                        DLSYM_ARG(fido_cred_id_len),
                        DLSYM_ARG(fido_cred_id_ptr),
                        DLSYM_ARG(fido_cred_new),
                        DLSYM_ARG(fido_cred_set_clientdata_hash),
                        DLSYM_ARG(fido_cred_set_extensions),
                        DLSYM_ARG(fido_cred_set_rk),
                        DLSYM_ARG(fido_cred_set_rp),
                        DLSYM_ARG(fido_cred_set_type),
                        DLSYM_ARG(fido_cred_set_user),
                        DLSYM_ARG(fido_cred_set_uv),
                        DLSYM_ARG(fido_dev_free),
                        DLSYM_ARG(fido_dev_get_assert),
                        DLSYM_ARG(fido_dev_get_cbor_info),
                        DLSYM_ARG(fido_dev_info_free),
                        DLSYM_ARG(fido_dev_info_manifest),
                        DLSYM_ARG(fido_dev_info_manufacturer_string),
                        DLSYM_ARG(fido_dev_info_new),
                        DLSYM_ARG(fido_dev_info_path),
                        DLSYM_ARG(fido_dev_info_product_string),
                        DLSYM_ARG(fido_dev_info_ptr),
                        DLSYM_ARG(fido_dev_is_fido2),
                        DLSYM_ARG(fido_dev_make_cred),
                        DLSYM_ARG(fido_dev_new),
                        DLSYM_ARG(fido_dev_open),
                        DLSYM_ARG(fido_strerr),
                        NULL);
        if (r < 0)
                return r;

        /* Note that we never release the reference here, because there's no real reason to, after all this
         * was traditionally a regular shared library dependency which lives forever too. */
        libfido2_dl = TAKE_PTR(dl);
        return 1;
}
#endif
