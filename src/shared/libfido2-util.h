/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include "macro.h"

#if HAVE_LIBFIDO2
#include <fido.h>

extern int (*sym_fido_assert_allow_cred)(fido_assert_t *, const unsigned char *, size_t);
extern void (*sym_fido_assert_free)(fido_assert_t **);
extern size_t (*sym_fido_assert_hmac_secret_len)(const fido_assert_t *, size_t);
extern const unsigned char* (*sym_fido_assert_hmac_secret_ptr)(const fido_assert_t *, size_t);
extern fido_assert_t* (*sym_fido_assert_new)(void);
extern int (*sym_fido_assert_set_clientdata_hash)(fido_assert_t *, const unsigned char *, size_t);
extern int (*sym_fido_assert_set_extensions)(fido_assert_t *, int);
extern int (*sym_fido_assert_set_hmac_salt)(fido_assert_t *, const unsigned char *, size_t);
extern int (*sym_fido_assert_set_rp)(fido_assert_t *, const char *);
extern int (*sym_fido_assert_set_up)(fido_assert_t *, fido_opt_t);
extern size_t (*sym_fido_cbor_info_extensions_len)(const fido_cbor_info_t *);
extern char **(*sym_fido_cbor_info_extensions_ptr)(const fido_cbor_info_t *);
extern void (*sym_fido_cbor_info_free)(fido_cbor_info_t **);
extern fido_cbor_info_t* (*sym_fido_cbor_info_new)(void);
extern void (*sym_fido_cred_free)(fido_cred_t **);
extern size_t (*sym_fido_cred_id_len)(const fido_cred_t *);
extern const unsigned char* (*sym_fido_cred_id_ptr)(const fido_cred_t *);
extern fido_cred_t* (*sym_fido_cred_new)(void);
extern int (*sym_fido_cred_set_clientdata_hash)(fido_cred_t *, const unsigned char *, size_t);
extern int (*sym_fido_cred_set_extensions)(fido_cred_t *, int);
extern int (*sym_fido_cred_set_rk)(fido_cred_t *, fido_opt_t);
extern int (*sym_fido_cred_set_rp)(fido_cred_t *, const char *, const char *);
extern int (*sym_fido_cred_set_type)(fido_cred_t *, int);
extern int (*sym_fido_cred_set_user)(fido_cred_t *, const unsigned char *, size_t, const char *, const char *, const char *);
extern int (*sym_fido_cred_set_uv)(fido_cred_t *, fido_opt_t);
extern void (*sym_fido_dev_free)(fido_dev_t **);
extern int (*sym_fido_dev_get_assert)(fido_dev_t *, fido_assert_t *, const char *);
extern int (*sym_fido_dev_get_cbor_info)(fido_dev_t *, fido_cbor_info_t *);
extern void (*sym_fido_dev_info_free)(fido_dev_info_t **, size_t);
extern int (*sym_fido_dev_info_manifest)(fido_dev_info_t *, size_t, size_t *);
extern const char* (*sym_fido_dev_info_manufacturer_string)(const fido_dev_info_t *);
extern const char* (*sym_fido_dev_info_product_string)(const fido_dev_info_t *);
extern fido_dev_info_t* (*sym_fido_dev_info_new)(size_t);
extern const char* (*sym_fido_dev_info_path)(const fido_dev_info_t *);
extern const fido_dev_info_t* (*sym_fido_dev_info_ptr)(const fido_dev_info_t *, size_t);
extern bool (*sym_fido_dev_is_fido2)(const fido_dev_t *);
extern int (*sym_fido_dev_make_cred)(fido_dev_t *, fido_cred_t *, const char *);
extern fido_dev_t* (*sym_fido_dev_new)(void);
extern int (*sym_fido_dev_open)(fido_dev_t *, const char *);
extern const char* (*sym_fido_strerr)(int);

int dlopen_libfido2(void);

static inline void fido_cbor_info_free_wrapper(fido_cbor_info_t **p) {
        if (*p)
                sym_fido_cbor_info_free(p);
}

static inline void fido_assert_free_wrapper(fido_assert_t **p) {
        if (*p)
                sym_fido_assert_free(p);
}

static inline void fido_dev_free_wrapper(fido_dev_t **p) {
        if (*p)
                sym_fido_dev_free(p);
}

static inline void fido_cred_free_wrapper(fido_cred_t **p) {
        if (*p)
                sym_fido_cred_free(p);
}

#endif
